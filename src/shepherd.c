/* Copyright 1999, 2000 Jacob A. Langford
 *
 * shepherd.c
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
#include <regex.h>
#include <string.h>
#include <glob.h>
#include <pwd.h>
#include <signal.h>


#include "util.h"
#include "option.h"
#include "argument.h"
#include "shepherd.h"
#include "reaper.h"
#include "environment.h"

void shepherd (job_info *job, rc_items *rc) {

   long send_try;
   struct stat statbuf;
   char status [512];

   /* Get job id... also set pdq->job_base */
   claim_job_id (job, rc);     

   /* Redirect stderr and stdout to a log file */
   redirect_output (job);

   set_status ("reading input", job);
   link_print_file (job, "raw"); 

   setup_job_environments (job, rc);  /* Must be done after link_print_file
   				       * so that job->input_filename can be
				       * set to "stdin" on some jobs.
				       */

   force_set_status ("analyzing input", job);
   analyze_print_file (job, rc, "raw");
   do_file_regx (job, rc);
   force_set_status ("converting", job);
   do_convert (job, rc, "raw", "converted");
   force_set_status ("filtering", job);
   do_filter (job, rc, "converted", "filtered");


   force_set_status ("sending", job);
   for (send_try = 0; send_try < rc->max_send_tries; send_try++) {
      sprintf (status, "sending (try %li)", send_try+1);
      set_status (status, job);
      if ( 0 == do_send_interface (job, rc, "filtered") ) {
         break;
      }
      sleep (rc->delay_between_tries); 
   }

   purge_old_jobs (rc->job_dir, rc->job_history_duration);

   if (send_try == rc->max_send_tries) {
      force_set_status ("aborted", job);
   }
}

void redirect_output (job_info *job) {

   int err_fd;
   char *err_fname;

   err_fname = malloc ( strlen(job->job_base) + 5 );
   if (err_fname == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   sprintf (err_fname, "%s.log", job->job_base);
   err_fd = open (err_fname, O_CREAT | O_TRUNC | O_WRONLY, 0600);
   if (err_fd == -1) {
      fprintf (stderr, "Couldn't create %s for error logging.\n", err_fname);
      perror (NULL);
      my_exit (1);
   }
   if ( dup2 (err_fd, 1) == -1 ) {
      fprintf (stderr, "Couldn't redirect stdout.\n");
      perror (NULL);
      my_exit (1);
   }
   if ( dup2 (err_fd, 2) == -1 ) {
      fprintf (stderr, "Couldn't redirect stderr.\n");
      perror (NULL);
      my_exit (1);
   }

   free (err_fname);

}


void link_print_file (job_info *job, char *output) {
   
   long i;
   char buf[1024], *data_fname;
   int in_fd, out_fd;

   data_fname = malloc ( strlen(job->job_base) + strlen(output) + 3 );
   if (data_fname == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   sprintf (data_fname, "%s.%s", job->job_base, output);

   out_fd = open (data_fname, O_CREAT | O_TRUNC | O_WRONLY, 0600);
   if (out_fd == -1) {
      fprintf (stderr, "Couldn't create %s for writing.\n", data_fname);
      perror (NULL);
   }
   free (data_fname);

   sleep (10);

   if (job->input_filename == NULL) {
      in_fd = 0;
      str_reset (&job->input_filename, "stdin");
      while ( (i=read(in_fd, buf, 1024)) > 0 ) {
         if (write (out_fd, buf, i) != i ) {
	    fprintf (stderr, "Error copying data file.\n");
	    perror (NULL);
	    my_exit (1);
	 }
      }
   } else {
      in_fd = job->fd;
      while ( (i=read(in_fd, buf, 1024)) > 0 ) {
         if (write (out_fd, buf, i) != i ) {
	    fprintf (stderr, "Error copying data file.\n");
	    perror (NULL);
	    my_exit (1);
	 }
      }
      close (in_fd);
   }

   close (out_fd);

}

char *create_script (char *base, char *script, char *ext) { 

   int i, fd;
   char *name;

   if (script == NULL) return (NULL);

   if (ext == NULL) {
      str_set(&name, base);
   } else {
      name = malloc ( strlen (base) + strlen (ext) + 2 );
      if (name == NULL) {
	 fprintf (stderr, "Not enough memory.\n");
	 my_exit (1);
      }
      sprintf (name, "%s.%s", base, ext);
   }

   fd = open (name, O_CREAT | O_TRUNC | O_WRONLY, 0700);
   if (0 != chmod (name, 0700)) {
      fprintf (stderr, "Error chmod'ing 0700 %s\n", name);
      my_exit (1);
   }
   if (fd == -1) {
      fprintf (stderr, "Couldn't create script file %s\n", name);
      perror (NULL);
      my_exit (1);
   }

   if ( (script[0] != '#') || (script[1] != '!') ) {
      if ( 11 != write (fd, "#! /bin/sh\n", 11)) {
	 fprintf (stderr, "IO error writing to script file.\n");
	 perror (NULL);
	 my_exit (1);
      }
   }

   i = strlen (script);
   if ( i != write (fd, script, i) ) {
      fprintf (stderr, "IO error writing to script file.\n");
      perror (NULL);
      my_exit (1);
   }

   close (fd);

   return (name);

}

void analyze_print_file (job_info *job, rc_items *rc, char *input) {

   int fd[2], n;
   long i;
   char *script, *arg[2], **env, buf[1024], *d, *e;
   dl_list *environment;
   pid_t pid;
   fd_set read_set;
   struct timeval tv;
   reaped_item *r;


   if (pipe(fd) != 0) {
      fprintf (stderr, "Error creating unnamed pipe.\n");
      perror (NULL);
      my_exit(1);
   }


   pid = fork();

   switch (pid) {
      case (-1) :
	 fprintf (stderr, "Error forking.\n");
	 perror (NULL);
	 my_exit (1);
      case (0) :
	 script = create_script (job->job_base, job->my_driver->filetype_exec, 
	    "driver_filetype");

	 /* Set up environment */
	 environment = copy_list (job->env_driver, 
	 	(void *)(void *)copy_item_pair);
	 d = malloc (strlen (job->job_base) + strlen(input) + 3);
	 sprintf (d, "%s.%s", job->job_base, input);
	 environment_set_item (environment, "INPUT", d);
	 env = env_list_to_env (environment);

	 if (STDOUT_FILENO != dup2 (fd[1], STDOUT_FILENO)) {
	    fprintf (stderr, "CHILD: Error plumbing filetype script.\n");
	    perror (NULL);
	    exit (1);
	 }
	 arg[0] = script;
	 arg[1] = NULL;
	 if (0 != execve (script, arg, env) ) {
	    fprintf (stderr, "CHILD: Error execing filetyp script.\n");
	    perror ("CHILD");
	    exit (1);
	 }
	 break;
      default:  

         i = 0;

	 FD_ZERO (&read_set);

	 while (  (r = get_reaped_by_pid (pid)) == NULL  ) {

	    FD_SET (fd[0], &read_set);
	    tv.tv_sec = 0;         /* Block for 100 ms */
	    tv.tv_usec = 100000;
	    n = select (fd[0] + 1, &read_set, NULL, NULL, &tv);
	 
	    if (n == -1) {
	       switch (errno) {
	          case (EINTR):
		     /* The child has most likely exited, so we need to
		      * select once more before we have the chance to abort.
		      */
		     FD_SET (fd[0], &read_set);
		     tv.tv_sec = 0;         /* Block for 100 ms */
		     tv.tv_usec = 100000;
		     n = select (fd[0] + 1, &read_set, NULL, NULL, &tv);
		     if (n != -1) break;
		  default:
		     perror ("select()");
		     my_exit (1);
	       }
	    } 

	    if ( n > 0 ) {
	       if (  FD_ISSET (fd[0],&read_set)  ) {
		  i = read(fd[0], buf, 1024);
		  if (i == -1) {
		     fprintf (stderr, "Error reading output of"
			" filetype_exec.");
		     my_exit (1);
		  }
	       } 
	    }


	 }


	 close (fd[0]);

	 if ( WIFEXITED(r->status) == 0 ) {
	    fprintf (stderr, "filetype_exec script exited abnormally.\n");
	    my_exit (1);
	 }

	 if ( WEXITSTATUS(r->status) != 0 ) {
	    fprintf (stderr, "filetype_exec script "
	       "exited with nonzero status.\n");
	    my_exit (1);
	 }
	
	 free_reaped_item (r);

	 if (i == 1024) {
	    fprintf (stderr, "Warning, the output of"
	       " filetype_exec was truncated.\n");
	    buf[1023] = 0; 
	 } else {
	    buf[i] = 0;
	 }

	 d = buf;
	 /* Now strip out the program name if the file command included it */
	 e = malloc (strlen (job->job_base) + strlen (input) + 3);
	 if (e == NULL) {
	    fprintf (stderr, "Not enough memory.\n");
	    my_exit (1);
	 }
	 sprintf (e, "%s.%s", job->job_base, input);
	 i = strlen (e);		 /* path name */
	 if (0 == strncmp ( e, buf, i ))    d += i;
	 free (e);
	 while ( (*d == ':') || (*d == ' ') || (*d == '\t')) d++;
	 str_reset (&job->file_type, d);
	 i = strlen(job->file_type);
	 /* Strip \n */
	 if (job->file_type[i - 1] == '\n') job->file_type[i - 1] = 0; 


   }

}



void do_file_regx (job_info *job, rc_items *rc) { 
   
   int i;
   language_driver *ld;
   dl_list *list;
   regex_t preg;
   regmatch_t pmatch[1];

   list = first_list_element (job->my_driver->language_driver_list);
   while (list != NULL) {
      ld = (language_driver *) list->data;
      if (ld->filetype_regx == NULL) {
         i = regcomp (&preg, ld->name, REG_EXTENDED | REG_ICASE);
      } else {
         i = regcomp (&preg, ld->filetype_regx, REG_EXTENDED | REG_ICASE);
      }
      if (i != 0) {
         fprintf (stderr, "Error compiling regex pattern.\n");
	 my_exit (1);
      }
      i = regexec (&preg, job->file_type, 1, pmatch, 0);
      if (i == 0) {
         job->my_language_driver = ld;
         regfree (&preg);
         break;
      }
      regfree (&preg);
      list = list->next;
   }

   if (job->my_language_driver == NULL) {
      fprintf (stderr, "Don't know how to print this type of file:\n");
      fprintf (stderr, "%s\n", job->file_type);
      my_exit (1);
   }

   str_reset (&job->language_driver, job->my_language_driver->name);

}

void do_convert (job_info *job, rc_items *rc, char *input, char *output) {

   char *script, *arg[2], **env, *d;
   dl_list *environment;
   struct timeval tv;
   reaped_item *r;
   pid_t pid;


   pid = fork();

   switch (pid) {
      case -1:
	 perror ("Error forking");
	 my_exit (1);
      case 0:
	 script = create_script (job->job_base, 
	 	job->my_language_driver->convert_exec, "driver_convert");

	 /* Set up environment */
	 environment = copy_list (job->env_driver, 
	 	(void *)(void *)copy_item_pair);
	 d = malloc (strlen (job->job_base) + strlen(input) +
	 	strlen(output) + 3);
	 sprintf (d, "%s.%s", job->job_base, input);
	 environment_set_item (environment, "INPUT", d);
	 sprintf (d, "%s.%s", job->job_base, output);
	 environment_set_item (environment, "OUTPUT", d);
	 env = env_list_to_env (environment);

         arg[0] = script;
	 arg[1] = NULL;
	 if (0 != execve (script, arg, env) ) {
	    perror ("Error execing conversion script");
	    my_exit (1);
	 }
         break;
      default:

	 while (  (r = get_reaped_by_pid (pid)) == NULL  ) {
	    tv.tv_sec = 0;         /* Block for 100 ms */
	    tv.tv_usec = 100000;
	    select (0, NULL, NULL, NULL, &tv);
	 }

	 if ( WIFEXITED(r->status) == 0 ) {
	    fprintf (stderr, "Conversion script failed abnormally.\n");
	    my_exit (1);
	 }
	 if ( WEXITSTATUS(r->status) != 0 ) {
	    fprintf (stderr, "Conversion script exited with nonzero status.\n");
	    my_exit (1);
	 }
        
	 free_reaped_item (r);
   
   }

}

void do_filter (job_info *job, rc_items *rc, char *input, char *output) {

   char *script, *arg[2], **env, *d;
   dl_list *environment;
   pid_t pid;
   struct timeval tv;
   reaped_item *r;

   pid = fork();

   switch (pid) {
      case -1:
	 perror ("Error forking");
	 my_exit (1);
      case 0:
	 script = create_script (job->job_base, job->my_driver->filter_exec, 
	    "driver_filter");

	 /* Set up environment */
	 environment = copy_list (job->env_driver, 
	 	(void *)(void *)copy_item_pair);
	 d = malloc (strlen (job->job_base) + strlen(input) +
	 	strlen(output) + 3);
	 sprintf (d, "%s.%s", job->job_base, input);
	 environment_set_item (environment, "INPUT", d);
	 sprintf (d, "%s.%s", job->job_base, output);
	 environment_set_item (environment, "OUTPUT", d);
	 env = env_list_to_env (environment);

         arg[0] = script;
	 arg[1] = NULL;
	 if (0 != execve (script, arg, env) ) {
	    perror ("Error execing filter script");
	    my_exit (1);
	 }
         break;
      default:

	 while (  (r = get_reaped_by_pid (pid)) == NULL  ) {
	    tv.tv_sec = 0;         /* Block for 100 ms */
	    tv.tv_usec = 100000;
	    select (0, NULL, NULL, NULL, &tv);
	 }

	 if ( WIFEXITED(r->status) == 0 ) {
	    fprintf (stderr, "Filter script failed abnormally.\n");
	    my_exit (1);
	 }
	 if ( WEXITSTATUS(r->status) != 0 ) {
	    fprintf (stderr, "Filter script exited with nonzero status.\n");
	    my_exit (1);
	 }
        
	 free_reaped_item (r);
   
   }

}


int do_send_interface (job_info *job, rc_items *rc, char *input ) { 

   char *script, *cancel_flag, *arg[2], **env, *d;
   dl_list *environment;
   pid_t pid;
   reaped_item *r;
   int i;
   int cancelled;
   struct stat statbuf;
   struct timeval tv;


   cancelled = 0;

   cancel_flag = malloc ( strlen (job->job_base) + 25);
   if (cancel_flag == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   sprintf (cancel_flag, "%s.cancelled", job->job_base);

   pid = fork();

   switch (pid) {
      case -1:
	 perror ("Error forking");
	 my_exit (1);
      case 0:
	 script = create_script (job->job_base, job->my_interface->send_exec, 
	    "interface_send");
	 if (script == NULL) return (0);

	 /* Set up environment */
	 environment = copy_list (job->env_interface, 
	 	(void *)(void *)copy_item_pair);
	 d = malloc (strlen (job->job_base) + strlen(input) + 3);
	 sprintf (d, "%s.%s", job->job_base, input);
	 environment_set_item (environment, "INPUT", d);
	 env = env_list_to_env (environment);

	 close ( open (d, O_CREAT|O_RDONLY, 0600));    /* touch */
         arg[0] = script;
	 arg[1] = NULL;
	 setpgid (0,0); 
	 if (0 != execve (script, arg, env) ) {
	    perror ("Error execing interface send script");
	    exit (1);
	 }
         break;
      default:

         i = 0;

	 while (  (r = get_reaped_by_pid (pid)) == NULL  ) {


	    /* Check for cancel here */
	    if ( (cancelled == 0) &&
	         (stat (cancel_flag, &statbuf) == 0)  ) {

               /* First acknowledge that pdq will handle the cancel,
	        * so that xpdq knows to leave it alone
		*/
               sprintf (cancel_flag, "%s.ack_cancelled", job->job_base);
               if (-1 == close ( open (cancel_flag, 
	       		O_CREAT|O_WRONLY, 0600))) { ; /* mknod() */
	            fprintf (stderr, "Cannot open %s\n",  cancel_flag);
		    my_exit (0);
               }
               fprintf (stderr, "pdq has received a cancel request and is "
	       "sending the interface a TERM signal\n");

               /* Raise a signal for the interface_exec script
		*/
	       cancelled = 1;
	       if (-1 == killpg (pid, SIGTERM)) { 
		  perror ("Failed on kill");   
	       }

	       set_status ("cancelling...", job);

	    }

	    tv.tv_sec = 0;         /* Block for 100 ms */
	    tv.tv_usec = 100000;
	    select (0, NULL, NULL, NULL, &tv);
	 }

	 if (cancelled == 0) {

	    if ( WIFEXITED(r->status) == 0 ) {
	       fprintf (stderr, "Interface send script failed abnormally.\n");
	       return (1);
	    }
	    if ( WEXITSTATUS(r->status) != 0 ) {
	       fprintf (stderr, "Interface send script exited "
		   "with nonzero status.\n");
	       return (1);
	    }
        }
	free_reaped_item (r);
   }

   free (cancel_flag);

   if (cancelled == 1) {
      force_set_status ("cancelled (pdq)", job);
   } else {
      set_status ("finished", job);
   }

   return (0);

}

void do_cancel (job_info *job, rc_items *rc) {

   char *script, *arg[2], **env;
   pid_t pid;
   struct timeval tv;
   reaped_item *r;

   if (job->my_interface->cancel_exec == NULL) return;

   pid = fork();

   switch (pid) {
      case -1:
	 perror ("Error forking");
	 my_exit (1);
      case 0:
	 script = create_script (job->job_base, job->my_interface->cancel_exec, 
	    "cancel");

	 /* Set up environment */
	 env = env_list_to_env (job->env_interface);

         arg[0] = script;
	 arg[1] = NULL;
	 if (0 != execve (script, arg, env) ) {
	    perror ("Error execing cancel script");
	    my_exit (1);
	 }
         break;
      default:

	 while (  (r = get_reaped_by_pid (pid)) == NULL  ) {
	    tv.tv_sec = 0;         /* Block for 100 ms */
	    tv.tv_usec = 100000;
	    select (0, NULL, NULL, NULL, &tv);
	 }

	 if ( WIFEXITED(r->status) == 0 ) {
	    fprintf (stderr, "Cancel script failed abnormally.\n");
	    /* Failure here does not warrant an abort. */
	 }
	 if ( WEXITSTATUS(r->status) != 0 ) {
	    fprintf (stderr, "Cancel script exited with nonzero status.\n");
	    /* Failure here does not warrant an abort. */
	 }
        
	 free_reaped_item (r);
   
   }

   force_set_status ("cancelled (xpdq)", job);

}

void pdq_exit (int code) {
   static int count = 0;
   if (count++ > 0) _exit (code);  /* Prevent recursion */
   switch (code) {
      case 1:
         set_status ("aborted", job);
	 break;
      default:  /* Leave status unchanged */
	 break;
   }

   free_rc_items (rc);
   free_job_info (job);
   _exit (code);
}
