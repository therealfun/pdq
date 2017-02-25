/* Copyright 1999, 2000 Jacob A. Langford
 *
 * interface.c
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>


#include "interface.h"
#include "option.h"
#include "argument.h"
#include "parse.h"
#include "list.h"
#include "lex.h"
#include "util.h"
#include "job.h"
#include "environment.h"
#include "shepherd.h"
#include "reaper.h"

interface *new_interface (void) {

   interface *i;
  
   i = malloc (sizeof(interface));
   if (i == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   i->name = NULL;
   i->verify_exec = NULL;
   i->send_exec = NULL;
   i->status_exec = NULL;
   i->cancel_exec = NULL;
   i->help = NULL;
   i->incompatibility_msg = NULL;
   i->compatibility = 0;
   i->default_option_list = NULL;
   i->requirement_list = NULL;
   i->option_list = NULL;
   i->arg_list = NULL;
   i->required_arg_list = NULL;

   return (i);

}

interface *copy_interface (interface *i) {

   interface *out;

   out = new_interface();

   str_reset (&out->name, i->name);
   str_reset (&out->verify_exec, i->verify_exec);
   str_reset (&out->send_exec, i->send_exec);
   str_reset (&out->status_exec, i->status_exec);
   str_reset (&out->cancel_exec, i->cancel_exec);
   str_reset (&out->help, i->help);
   str_reset (&out->incompatibility_msg, i->incompatibility_msg);
   out->compatibility = i->compatibility;
   out->default_option_list =
   	copy_list ( i->default_option_list,  (void *)(void *)copy_item );
   out->option_list = copy_list ( i->option_list,  
   	(void *)(void *)copy_option );
   out->required_arg_list =
   	copy_list ( i->required_arg_list,  (void *)(void *)copy_item );
   out->arg_list = copy_list ( i->arg_list,  (void *)(void *)copy_argument );
   out->requirement_list =
   	copy_list ( i->requirement_list,  (void *)(void *)copy_item );

   return (out);

}

void free_interface (interface *i) {

   if (i->name != NULL) free (i->name);
   if (i->verify_exec != NULL) free (i->verify_exec);
   if (i->send_exec != NULL) free (i->send_exec);
   if (i->status_exec != NULL) free (i->status_exec);
   if (i->cancel_exec != NULL) free (i->cancel_exec);
   if (i->help != NULL) free (i->help);
   if (i->incompatibility_msg != NULL) free (i->incompatibility_msg);

   list_iterate ( i->default_option_list, free );
   i->default_option_list = free_list ( i->default_option_list );

   list_iterate ( i->requirement_list, free );
   i->requirement_list = free_list ( i->requirement_list );

   list_iterate (i->option_list, (void *)(void *)free_option);
   i->option_list = free_list (i->option_list);

   list_iterate (i->required_arg_list, free);
   i->required_arg_list = free_list (i->required_arg_list);

   list_iterate (i->arg_list, (void *)(void *)free_argument);
   i->arg_list = free_list (i->arg_list);

   free (i);
}

int add_interface_by_rc (parse_state *s, dl_list **l, int debug_rc) {

   interface *d;
   dl_list *ltmp;
   parse_state *s2;
   int i;

   d = new_interface();

   if (  scan_next_block (s) == 0  ) {
       fprintf (stderr, "Interface name not found\n");
       return (1); 
   }
   str_reset (&d->name, s->token);

   if (  scan_next_block (s) == 0  ) {
       fprintf (stderr, "Block not found after interface %s\n", d->name);
       return (1); 
   }

   s2 = sub_parse_state (s);

  #define THIS d
  #define KEYWORD_PARSE(KEY) \
     (strcmp (s2->token, #KEY) == 0 ) { \
	if (scan_next_block (s2) == 0) { \
	   fprintf (stderr, "Found no block after " #KEY ".\n"); \
	   fprintf (stderr, "Error parsing interface %s\n", THIS->name);\
	   return (1); \
	} \
     str_reset (&THIS->KEY, s2->token); \
  } 
  #define KEYWORD_TO_LIST_PARSE(KEY,LIST,FUNCTION) \
     (strcmp (s2->token, #KEY) == 0 ) { \
	if (FUNCTION (s2,&THIS->LIST) != 0) { \
	   fprintf (stderr, "Error parsing " #KEY ".\n"); \
	   fprintf (stderr, "Error parsing interface %s\n", THIS->name);\
	   return (1); \
	} \
  } 
   while ( scan_next_block (s2)  ) {
      if      KEYWORD_PARSE (verify_exec)
      else if KEYWORD_PARSE (send_exec)
      else if KEYWORD_PARSE (status_exec)
      else if KEYWORD_PARSE (cancel_exec)
      else if KEYWORD_PARSE (help)
      else if KEYWORD_TO_LIST_PARSE (requires, requirement_list,
         add_items_by_rc)
      else if KEYWORD_TO_LIST_PARSE (default_options, default_option_list,
         add_items_by_rc)
      else if KEYWORD_TO_LIST_PARSE (required_args, required_arg_list,
         add_items_by_rc)
      else if KEYWORD_TO_LIST_PARSE (argument, arg_list,
         add_argument_by_rc)
      else if KEYWORD_TO_LIST_PARSE (option, option_list,
         add_option_by_rc) 
      else {
         fprintf (stderr, "Unknown keyword \"%s\" parsing interface %s\n", 
	    s2->token, d->name);
	 return (1); 
      }
   }

   free_parse_state (s2);

   if ( d->send_exec == NULL ) {
      fprintf (stderr, "Interface %s must at least define send_exec.\n",
         d->name);
      return (1);
   } 

   for (i = 0; i < debug_rc; i++)   fprintf (stdout, "  ");

   /* Overwrite this if it already exists */
   ltmp = find_in_list_by_string (*l, d->name);
   if (ltmp == NULL) {
      *l = append_to_list (*l, (void *)d);
      if (debug_rc) fprintf (stdout, "defining interface %s in %s\n",
      		d->name, s->file_name);
   } else {
      free_interface (ltmp->data);
      *l = replace_in_list (*l, ltmp->data, (void *)d);
      if (debug_rc) fprintf (stdout, "REDEFINING interface %s in %s\n",
      		d->name, s->file_name);
   }

   return (0);
  
  
}

void check_interface_compatibility (interface *d, rc_items *rc) {
 
   int err_count, fd[2], err_verify_exec, n;
   long msg_len, i;
   char *namespace, *script, *arg[2], **env, msg_buf[2048], *msg;
   dl_list *environment, *msg_list, *l;
   pid_t pid;
   fd_set read_set;
   struct timeval tv;
   reaped_item *r;

   msg_len = 0;
   msg_list = NULL;
   err_verify_exec = 0;

   i = 0;
   if (d->verify_exec != NULL) {
      namespace = claim_tmpfile (rc->job_dir); 
      script = create_script (namespace, d->verify_exec, NULL);
      free (namespace);
      environment = environment_set_item (NULL, "PATH", 
        	rc->interface_command_path);
      env = env_list_to_env (environment);

      if (pipe(fd) != 0) {
	 fprintf (stderr, "Error creating unnamed pipe.\n");
	 perror (NULL);
	 unlink (script);
	 my_exit(1);
      }


      pid = fork();

      switch (pid) {
         case (-1) :
	    fprintf (stderr, "Error forking.\n");
	    perror (NULL);
	    unlink (script);
	    my_exit (1);
	 case (0) :
	    if (STDOUT_FILENO != dup2 (fd[1], STDOUT_FILENO)) {
	       fprintf (stderr, "CHILD: Error plumbing verify script.\n");
	       perror (NULL);
	       exit (1);
	    }
	    if (STDERR_FILENO != dup2 (fd[1], STDERR_FILENO)) {
	       fprintf (stderr, "CHILD: Error plumbing verify script.\n");
	       perror (NULL);
	       exit (1);
	    }
	    arg[0] = script;
	    arg[1] = NULL;
	    if (0 != execve (script, arg, env) ) {
	       fprintf (stderr, "CHILD: Error execing verify script.\n");
	       perror ("CHILD");
	       exit (1);
	    }
	    break;
	 default:  


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
		     i = read(fd[0], msg_buf, 2048);
		     if (i == -1) {
		        fprintf (stderr, "Error reading output of"
			   " verify_exec.");
			my_exit (1);
		     }
		  } 
	       }


            }

	    close (fd[0]);

	    if ( WIFEXITED(r->status) == 0 ) {
	       str_set (&msg, "verify_exec script exited abnormally.\n");
	       msg_list = append_to_list (msg_list, msg);
	       msg_len += strlen(msg);
	       err_verify_exec = 1;
	    }
	    if ( WEXITSTATUS(r->status) != 0 ) {
	       str_set (&msg, 
		  "\nThe verify_exec script exited with nonzero status,\n"
		  "which means that this interface is not compatible with your"
		  "system.\n\n");
	       msg_list = append_to_list (msg_list, msg);
	       msg_len += strlen(msg);
	       err_verify_exec = 1;
	    }
	    free_reaped_item (r);
      }

      free_env (env);
      list_iterate (environment, (void *)(void *)free_item_pair);
      free_list (environment);
      unlink (script);
      free (script);
   }

   err_count = 0;
   l = first_list_element (d->requirement_list);
   while (l != NULL) {
      if ( verify_executable_in_path ( (char *)l->data, 
      		rc->interface_command_path) != 0 ) {

         msg = malloc( strlen ((char *)l->data) + 20 );
         sprintf (msg, "Couldn't find %s\n", (char *)l->data);
	 msg_list = append_to_list (msg_list, msg);
	 msg_len += strlen(msg);

         err_count ++;
      }
      l = l->next;
   }

   if (err_count) {
      str_set (&msg, "Driver components missing.\n"
      "Perhaps interface_command_path is not set properly?");
      msg_list = append_to_list (msg_list, msg);
      msg_len += strlen(msg);
   }

   if (err_count || err_verify_exec) {
      d->compatibility = 2;
   } else {
      d->compatibility = 1;
   }

   /* Construct message */
   if ( (msg_len + i) > 0 ) {
      d->incompatibility_msg = malloc (msg_len + 1 + i);
      if (d->incompatibility_msg == NULL) {
         fprintf (stderr, "Not enough memory.\n");
	 my_exit (1);
      }
      strncpy (d->incompatibility_msg, msg_buf, i);
      d->incompatibility_msg[i] = 0;
      l = first_list_element (msg_list);
      while (l != NULL) {
	 strcat (d->incompatibility_msg, (char *)l->data);
	 free (l->data);
         l = l->next;
      }
      free_list (msg_list);
   }

}

