/* Copyright 1999, 2000 Jacob A. Langford
 *
 * job.c
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
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <glob.h>
#include <time.h>

#include "job.h"
#include "util.h"
#include "list.h"

job_info *new_job_info (void) {

   job_info *job;

   job = malloc (sizeof (job_info));
   if (job == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }

   job->id = 0;
   job->input_filename = NULL;
   job->invoked_by = NULL;
   job->invoke_time = NULL;
   job->file_type = NULL;

   job->status = NULL;

   job->do_help = 0;
   job->argv0 = NULL;

   job->printer = NULL;
   job->driver = NULL;
   job->language_driver = NULL;
   job->interface = NULL;

   job->extra_driver_opt_list = NULL;
   job->extra_interface_opt_list = NULL;
   job->extra_driver_arg_list = NULL;
   job->extra_interface_arg_list = NULL;

   job->env_driver = NULL;
   job->env_interface = NULL;

   job->my_printer = NULL;
   job->my_driver = NULL;
   job->my_interface = NULL;
   job->my_language_driver = NULL;

   job->fd = 0;
   job->job_base = NULL;

   return (job);

}

void free_job_info (job_info *job) {

   if (job->input_filename != NULL) free (job->input_filename);
   if (job->argv0 != NULL) free (job->argv0);
   if (job->invoked_by != NULL) free (job->invoked_by);
   if (job->invoke_time != NULL) free (job->invoke_time);
   if (job->file_type != NULL) free (job->file_type);
   if (job->status != NULL) free (job->status);
   if (job->printer != NULL) free (job->printer);
   if (job->driver != NULL) free (job->driver);
   if (job->language_driver != NULL) free (job->language_driver);
   if (job->interface != NULL) free (job->interface);

   if (job->job_base != NULL) free (job->job_base);

   if (job->my_printer != NULL) free_printer (job->my_printer);
   if (job->my_driver != NULL) free_driver (job->my_driver);
   /* language_driver freed by free_driver */
   if (job->my_interface != NULL) free_interface (job->my_interface);

   list_iterate (job->extra_driver_opt_list, free); 
   job->extra_driver_opt_list = free_list (job->extra_driver_opt_list);
   list_iterate (job->extra_interface_opt_list, free); 
   job->extra_interface_opt_list = free_list (job->extra_interface_opt_list);
   list_iterate (job->extra_driver_arg_list, (void *)(void *)free_item_pair); 
   job->extra_driver_arg_list = free_list (job->extra_driver_arg_list);
   list_iterate (job->extra_interface_arg_list, (void *)(void *)free_item_pair); 
   job->extra_interface_arg_list = free_list (job->extra_interface_arg_list);

   free_environment (job->env_driver);
   free_environment (job->env_interface);

   free (job);

}

void write_status (job_info *job) {
   
   char *sfile, *stmpfile, *block;
   FILE *fd;

   if (job->job_base == NULL) return;

   sfile = malloc (strlen (job->job_base) + 8);
   stmpfile = malloc (strlen (job->job_base) + 11);
   if ((sfile == NULL) || (stmpfile == NULL)) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   sprintf (sfile, "%s.status", job->job_base);
   sprintf (stmpfile, "%s.statustmp", job->job_base);
   fd = fopen (stmpfile, "w+");
   if (fd == NULL) {
      fprintf (stderr, "Error opening %s.\n", stmpfile);
      perror (NULL);
      my_exit (1);
   }

   block = escape_block (job->input_filename, '{', '}');
   fprintf (fd, "input_filename = %s\n", block); 
   free (block);

   block = escape_block (job->invoke_time, '{', '}');
   fprintf (fd, "invoke_time = %s\n", block); 
   free (block);

   block = escape_block (job->invoked_by, '{', '}');
   fprintf (fd, "invoked_by = %s\n", block); 
   free (block);

   if (job->file_type != NULL) {
      block = escape_block (job->file_type, '{', '}');
      fprintf (fd, "file_type = %s\n", block); 
      free (block);
   }

   block = escape_block (job->status, '{', '}');
   fprintf (fd, "status = %s\n", block); 
   free (block);

   block = escape_block (job->job_base, '{', '}');
   fprintf (fd, "job_base = %s\n", block); 
   free (block);

   block = escape_block (job->printer, '{', '}');
   fprintf (fd, "printer = %s\n", block); 
   free (block);

   block = escape_block (job->interface, '{', '}');
   fprintf (fd, "interface = %s\n", block); 
   free (block);

   if (job->driver != NULL) {
      block = escape_block (job->driver, '{', '}');
      fprintf (fd, "driver = %s\n", block); 
      free (block);
   }

   if (job->language_driver != NULL) {
      block = escape_block (job->language_driver, '{', '}');
      fprintf (fd, "language_driver = %s\n", block); 
      free (block);
   }

   if (job->env_driver != NULL) {
      block = item_pair_list_to_str (job->env_driver);
      fprintf (fd, "env_driver = {%s}\n", block); 
      free (block);
   }

   if (job->env_interface != NULL) {
      block = item_pair_list_to_str (job->env_interface);
      fprintf (fd, "env_interface = {%s}\n", block); 
      free (block);
   }

   if (job->extra_driver_opt_list != NULL) {
      block = item_list_to_str (job->extra_driver_opt_list);
      fprintf (fd, "extra_driver_options = {%s}\n", block); 
      free (block);
   }

   if (job->extra_interface_opt_list != NULL) {
      block = item_list_to_str (job->extra_interface_opt_list);
      fprintf (fd, "extra_interface_options = {%s}\n", block); 
      free (block);
   }

   if (job->extra_driver_arg_list != NULL) {
      block = item_pair_list_to_str (job->extra_driver_arg_list);
      fprintf (fd, "extra_driver_args = {%s}\n", block); 
      free (block);
   }

   if (job->extra_interface_arg_list != NULL) {
      block = item_pair_list_to_str (job->extra_interface_arg_list);
      fprintf (fd, "extra_interface_args = {%s}\n", block); 
      free (block);
   }

   fclose (fd);
   if (rename (stmpfile, sfile) != 0) {
      fprintf (stderr, "Error overwriting status file.\n");
      perror (NULL);
      my_exit (1);
   }

}

void force_set_status (char *status, job_info *job) {

   char *d;
   set_status (status, job);

   /* Delete extra status file. */
   d = malloc (strlen(job->job_base) + 25);
   if (d == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   sprintf (d, "%s.extra_status", job->job_base);
   unlink (d);
   free (d);

}

void set_status (char *status, job_info *job) {

   str_reset (&job->status, status);
   write_status (job);

}

dl_list *job_list (char *job_dir) {

   glob_t globbuf;
   dl_list *l;
   int i, id;
   char *pat, *sd, *d;

   sd = tilde_expand (job_dir);

   l = NULL;

   pat = malloc (strlen(sd) + 25);
   if (pat == NULL) {
      fprintf (stderr, "Not enough memory");
      my_exit (1);
   }
   sprintf(pat, "%s/[0-9][0-9][0-9].status", sd);
   i = glob (pat, 0, NULL, &globbuf); 
   if (i != 0) {
      if (i != GLOB_NOMATCH) {
         /* Unknown error (or on some systems, no match) */
	 return (NULL);
      }
   } else {
      if (globbuf.gl_pathc == 0) {
         /* No match */
	 return (NULL);
      } 
   }

   for (i = 0; i <globbuf.gl_pathc; i++) {
      d = strrchr (globbuf.gl_pathv[i], '.');
      if (d == NULL) {
         fprintf (stderr, "Error in globbuf.\n");
	 my_exit (1);
      }
      *d = 0;
      id = atoi (d-3);
      l = append_to_list (l, (void *)id);
   }

   globfree (&globbuf);
   free (sd);

   return (l);

}

job_info *get_status_by_file (char *file) {

   job_info *j;
   parse_state *s;
   char *extra_status, *f;
   char extra_status_buf[1024];
   FILE *fd;
   int i;

   j = new_job_info();

   s = new_parse_state ();
   if (1 == try_initialize_parse_state (s, file) ) {
      free_parse_state (s);
      f = strrchr (file, '.');
      *f = 0;
      fprintf (stderr, "Don't know status of %s.\n", f-3);
      fprintf (stderr, "Please remove this file...\n");
      free_job_info (j);
      return (NULL);
   } 
   f = strrchr (file, '.');
   if (f == NULL) {
      fprintf (stderr, "What kind of status file is %s?\n", file);
      my_exit (1);
   } else {
      *f = 0;
      j->id = atoi(f-3);
   }

#define THIS j 
#define KEYWORD_PARSE(KEY) \
  (strcmp (s->token, #KEY) == 0 ) { \
     if (scan_next_block (s) == 0) { \
        free_job_info (j);\
	free_parse_state (s);\
	return (NULL); \
     } \
  str_reset (&THIS->KEY, s->token); \
} 
#define KEYWORD_TO_LIST_PARSE(KEY,LIST,FUNCTION) \
  (strcmp (s->token, #KEY) == 0 ) { \
     if (FUNCTION (s,&THIS->LIST) != 0) { \
        free_job_info (j);\
	free_parse_state (s);\
	return (NULL); \
     } \
} 
   while ( scan_next_block (s) ) {
      if      KEYWORD_PARSE (input_filename)
      else if KEYWORD_PARSE (invoked_by)
      else if KEYWORD_PARSE (file_type)
      else if KEYWORD_PARSE (job_base)
      else if KEYWORD_PARSE (status)
      else if KEYWORD_PARSE (invoke_time)
      else if KEYWORD_PARSE (printer)
      else if KEYWORD_PARSE (interface)
      else if KEYWORD_PARSE (driver)
      else if KEYWORD_PARSE (language_driver)
      else if KEYWORD_TO_LIST_PARSE (env_driver, env_driver,
      	 add_item_pairs_by_rc)
      else if KEYWORD_TO_LIST_PARSE (env_interface, env_interface,
      	 add_item_pairs_by_rc)
      else if KEYWORD_TO_LIST_PARSE (extra_driver_options,
       	 extra_driver_opt_list, add_items_by_rc)
      else if KEYWORD_TO_LIST_PARSE (extra_interface_options,
      	 extra_interface_opt_list, add_items_by_rc)
      else if KEYWORD_TO_LIST_PARSE (extra_driver_args,
       	 extra_driver_arg_list, add_item_pairs_by_rc)
      else if KEYWORD_TO_LIST_PARSE (extra_interface_args,
       	 extra_interface_arg_list, add_item_pairs_by_rc)
      else {
         /* Forgiveness... */
      }
   }

   free_parse_state (s);


   extra_status = malloc ( strlen (file) + 25);
   if (extra_status == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   sprintf (extra_status, "%s.extra_status", file);
   fd = fopen (extra_status, "r");
   if (fd != NULL) {
      fseek (fd, -1023, SEEK_END);
      i = fread (extra_status_buf, 1, 1023, fd);
      while (i > 0) {
         extra_status_buf [i] = 0;  /* Add NULL terminator */
	 f = strrchr (extra_status_buf, '\n');
	 if (f == NULL) { 
	    str_reset (&j->status, extra_status_buf);
	    break;
         }
	 if (  f != extra_status_buf + i - 1  ) {  
	    str_reset (&j->status, f + 1);
	    break;
	    
	 }
	 i--;
      }
   }
   free (extra_status);

   return (j);

}

void purge_old_jobs (char *job_dir, long job_history_duration) {

   glob_t globbuf;
   struct stat statbuf;
   int i, j, id;
   char  *pat, *d, *sd;
   time_t now;

   sd = tilde_expand (job_dir);
   now = time(NULL);
   pat = malloc (strlen(sd) + 25);
   if (pat == NULL) {
      fprintf (stderr, "Not enough memory");
      fprintf (stderr, "Failed to purge old jobs.\n");
      my_exit (1); 
   }
   sprintf(pat, "%s/[0-9][0-9][0-9].status", sd);
   i = glob (pat, 0, NULL, &globbuf); 
   free (pat);
   if (i != 0) {
      if (i != GLOB_NOMATCH) {
         fprintf (stderr, "Unknown error globbing for job id.\n");
	 perror (NULL);
         fprintf (stderr, "Failed to purge old jobs.\n");
	 my_exit (1);  
      } else {
         /* No match */
      }
   } else {
      if (globbuf.gl_pathc == 0) {
         return;
      } else {
	 for (i = 0; i < globbuf.gl_pathc; i++) {
	    j = stat (globbuf.gl_pathv [i], &statbuf);
	    if (j != -1) { /* Because another thread raced us to clean up? */
	       if ( (now - statbuf.st_mtime) >= job_history_duration) {
	          d = strrchr (globbuf.gl_pathv[i], '.');
		  *d = 0;
		  id = atoi (d-3);
		  delete_job (id, job_dir);
	       }
	    } 
	 }
      }
   }
   globfree (&globbuf);
   free (sd);
   
}

void delete_job (int id, char *job_dir) {

   glob_t globbuf;
   int i;
   char  *pat, *sd; 

   sd = tilde_expand (job_dir);
   pat = malloc (strlen(sd) + 25);
   if (pat == NULL) {
      fprintf (stderr, "Not enough memory");
      fprintf (stderr, "Failed to purge old jobs.\n");
      my_exit (1); 
   }
   sprintf(pat, "%s/%.3i.*", sd, id);
   i = glob (pat, 0, NULL, &globbuf); 
   free (pat);
   if (i != 0) {
      if (i != GLOB_NOMATCH) {
         fprintf (stderr, "Unknown error globbing for job id.\n");
	 perror (NULL);
         fprintf (stderr, "Failed to purge old jobs.\n");
	 my_exit (1); 
      } else {
         /* No match */
      }
   } else {
      if (globbuf.gl_pathc == 0) {
         return;
      } else {
	 for (i = 0; i < globbuf.gl_pathc; i++) {
	    if (-1 == unlink (globbuf.gl_pathv[i])) {
	       fprintf (stderr, "Error unlinking %s.\n", globbuf.gl_pathv[i]);
	       perror (NULL);
	    }
	 }
      }
   }
   globfree (&globbuf);
   free (sd);
}

void claim_job_id (job_info *job, rc_items *rc) {
    
   glob_t globbuf;
   struct stat statbuf;
   int i, claim_try, first_try;
   char *sd, *pat, *d;


   sd = tilde_expand (rc->job_dir); 

   /* Do what we can to make sure we have a job directory */
   CHECK_JOBDIR_AGAIN:
   i = stat (sd, &statbuf);
   if (i == -1) {
      if (errno == ENOENT) {
         i = mkdir (sd, 0700);
	 if (i != 0) {
	    /* Maybe another thread beat us to it */
	    if (i == EEXIST) goto CHECK_JOBDIR_AGAIN;
	    fprintf (stderr, "Error creating job directory %s\n", sd);
	    perror(NULL);
	    my_exit(1);
	 }
      } else {
	 fprintf (stderr, "Error reading job directory %s\n", sd);
	 perror (NULL);
	 my_exit (1);
      }
   }

   pat = malloc (strlen(sd) + 35);
   if (pat == NULL) {
      fprintf (stderr, "Not enough memory");
      my_exit (1);
   }
   sprintf(pat, "%s/[0-9][0-9][0-9].status", sd);
   i = glob (pat, 0, NULL, &globbuf); 
   free (pat);
   if (i != 0) {
      if (i != GLOB_NOMATCH) {
         fprintf (stderr, "Unknown error globbing for job id.\n");
	 perror (NULL);
	 my_exit (1);
      }
      claim_try = 1;
   } else {
      if (globbuf.gl_pathc == 0) {
         claim_try = 1;
      } else {
	 /* glob sorts everything, so let's go right to the end */
	 d = globbuf.gl_pathv [ globbuf.gl_pathc - 1];
	 d = strrchr (d, '.');
	 if (d == NULL) {
	    fprintf (stderr, "Error globbing for job id.\n");
	    perror (NULL);
	    my_exit (1);
	 }
	 *d = 0;
	 claim_try = atoi(d-3);
      }
   }
   globfree (&globbuf);


   first_try = claim_try;
   d = malloc ( strlen(sd) + 25 );
   if (d==NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }


   TRY_NEXT_ID:
   sprintf (d, "%s/%.3i.status", sd, claim_try);
   i = open (d, O_CREAT | O_EXCL, 0600);
   if (i == -1) {
      if (errno == EEXIST) {
         if (claim_try == 999) {  /* Roll back around */
	    claim_try = 1;
	 } else {
	    claim_try++;
	 }
	 if (claim_try == first_try) {
	    fprintf (stderr, "Couldn't get job id.\n"
	       "Perhaps job directory needs cleaned?\n");
	    my_exit(1);
	 }
	 goto TRY_NEXT_ID;
      } else {
         fprintf (stderr, "Error creating status file in "
	    "job dir %s\n", sd);  
	 perror(NULL);
	 my_exit(1);
      }
   } else  {
      /* we got an id... good */
      close (i);
   }

   job->id = claim_try;

   job->job_base = malloc (strlen(sd) + 15);
   if (job->job_base == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);;
   }
   sprintf(job->job_base, "%s/%.3i", sd, job->id);

   free (d);
   free (sd);

}

char *claim_tmpfile (char *job_dir) {
    
   struct stat statbuf;
   int i, claim_try;
   char *sd, *d, *tmpfile;

   sd = tilde_expand (job_dir); 

   /* Do what we can to make sure we have a job directory */
   CHECK_JOBDIR_AGAIN:
   i = stat (sd, &statbuf);
   if (i == -1) {
      if (errno == ENOENT) {
         i = mkdir (sd, 0700);
	 if (i != 0) {
	    /* Maybe another thread beat us to it */
	    if (i == EEXIST) goto CHECK_JOBDIR_AGAIN;
	    fprintf (stderr, "Error creating job directory %s\n", sd);
	    perror(NULL);
	    my_exit(1);
	 }
      } else {
	 fprintf (stderr, "Error reading job directory %s\n", sd);
	 perror (NULL);
	 my_exit (1);
      }
   }

   d = malloc ( strlen(sd) + 12 );
   if (d==NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }

   tmpfile = NULL;
   for (claim_try = 0; claim_try < 1000; claim_try++) {
      sprintf (d, "%s/tmp.%i", sd, claim_try);
      i = open (d, O_CREAT | O_EXCL, 0600);
      if (i == -1) {
         if (errno != EEXIST) {
            fprintf (stderr, "Error creating temp file in "
	       "job dir %s\n", sd);  
	    perror(NULL);
	    my_exit(1);
         }
      } else  {
        /* we got an id... good */
        close (i);
	tmpfile = d;
	break;
      }
   }

   free (sd);

   return (tmpfile);

}

job_info *get_status (int id, rc_items *rc) {

   struct stat statbuf;
   char *sd, *statfile, *extra_status;
   job_info *job;
   int i;

   sd = tilde_expand (rc->job_dir);
   statfile = malloc (strlen(sd) + 25);
   if (statfile == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   sprintf (statfile, "%s/%.3i.status", sd, id);

   i = stat (statfile, &statbuf);

   if (i == -1) {
      fprintf (stderr, "Couldn't stat %i\n", id);
      return (NULL);
   }

   job = get_status_by_file (statfile);
   free (statfile);
   if (job == NULL) {
      return (NULL);
   }
   job->last_stat = statbuf.st_mtime;

   extra_status = malloc (strlen(sd) + 25);
   if (extra_status == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   sprintf (extra_status, "%s/%.3i.extra_status", sd, job->id);
   i = stat (extra_status, &statbuf);

   if ( (i != -1) && (statbuf.st_mtime > job->last_stat) ) {
      job->last_stat = statbuf.st_mtime;
   }

   free (extra_status);
   free (sd);

   return (job);

      sprintf (statfile, "%s.extra_status", job->job_base);

}

int get_status_update (job_info **job, rc_items *rc) {

   struct stat statbuf;
   char *sd, *statfile, *extra_status;
   job_info *newjob, tmpjob;
   int i;
   time_t this_time;

   sd = tilde_expand (rc->job_dir);
   statfile = malloc (strlen(sd) + 25);
   if (statfile == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   sprintf (statfile, "%s/%.3i.status", sd, (*job)->id);
   extra_status = malloc (strlen(sd) + 25);
   if (extra_status == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   sprintf (extra_status, "%s/%.3i.extra_status", sd, (*job)->id);
   free (sd);

   i = stat (statfile, &statbuf);

   if (i == -1) {
      free_job_info (*job);
      *job = NULL;
      free (statfile);
      free (extra_status);
      return (1);
   }

   this_time = statbuf.st_mtime;

   i = stat (extra_status, &statbuf);

   if ( (i != -1) && (statbuf.st_mtime > this_time) ) {
      this_time = statbuf.st_mtime;
   }

   if ((*job)->last_stat == this_time) {
      free (statfile);
      free (extra_status);
      return (0);
   }

   newjob = get_status_by_file (statfile);


   (*job)->last_stat = this_time;

   str_reset (&(*job)->status, newjob->status);
   str_reset (&(*job)->file_type, newjob->file_type);

   free_job_info (newjob);
   free (statfile);
   free (extra_status);
   return (1);

}
