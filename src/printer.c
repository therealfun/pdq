/* Copyright 1999, 2000 Jacob A. Langford
 *
 * printer.c
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

printer *new_printer (void) {

   printer *p;

   p = malloc (sizeof(printer));
   if (p == NULL) {
      fprintf (stderr, "Not enough memory\n");
   }

   p->name = NULL;
   p->model = NULL;
   p->location = NULL;

   p->driver = NULL;
   p->driver_option_list = NULL;
   p->driver_arg_list = NULL;

   p->interface = NULL;
   p->interface_arg_list = NULL;
   p->interface_option_list = NULL;

   return (p);

}

printer *copy_printer (printer *p) {

   printer *out;

   out = new_printer ();

   str_reset (&out->name, p->name);
   str_reset (&out->interface, p->interface);
   str_reset (&out->driver, p->driver);
   str_reset (&out->model, p->model);
   str_reset (&out->location, p->location);
   out->driver_option_list = copy_list ( 
   	p->driver_option_list, (void *)(void *) copy_item );
   out->driver_arg_list = copy_list ( 
   	p->driver_arg_list, (void *)(void *) copy_item_pair );
   out->interface_option_list = copy_list ( 
   	p->interface_option_list, (void *)(void *) copy_item );
   out->interface_arg_list = copy_list ( 
   	p->interface_arg_list, (void *)(void *) copy_item_pair );

   return (out);
}

int add_printer_by_rc (parse_state *s, dl_list **l, int debug_rc) {

   printer *p;
   parse_state *s2;
   dl_list *ltmp;
   int i;

   p = new_printer();

   if (  scan_next_block (s) == 0  ) {
       fprintf (stderr, "Printer name not found\n");
       return (1); 
   }
   str_reset (&p->name, s->token);

   if (  scan_next_block (s) == 0  ) {
       fprintf (stderr, "Block not found after printer %s\n", p->name);
       return (1); 
   }

   s2 = sub_parse_state (s);

  #define THIS p
  #define KEYWORD_PARSE(KEY) \
     (strcmp (s2->token, #KEY) == 0 ) { \
	if (scan_next_block (s2) == 0) { \
	   fprintf (stderr, "Found no block after " #KEY ".\n"); \
	   fprintf (stderr, "Error parsing printer %s\n", THIS->name);\
	   return (1); \
	} \
     str_reset (&THIS->KEY, s2->token); \
  } 
  #define DELETE_PARSE(KEY) \
     (strcmp (s2->token, #KEY) == 0 ) { \
        ltmp = find_in_list_by_string (*l, p->name);\
	if (ltmp != NULL) {\
	   free_printer ((printer *)ltmp->data);\
	   *l = remove_list_link (ltmp);\
	}\
        for (i = 0; i < debug_rc; i++)   fprintf (stdout, "  "); \
        if (debug_rc) fprintf (stdout, "DELETING printer %s in %s\n",\
			p->name, s->file_name); \
        free_printer (p);\
	return (0);\
  } 
  #define KEYWORD_TO_LIST_PARSE(KEY,LIST,FUNCTION) \
     (strcmp (s2->token, #KEY) == 0 ) { \
	if (FUNCTION (s2,&THIS->LIST) != 0) { \
	   fprintf (stderr, "Error parsing " #KEY ".\n"); \
	   fprintf (stderr, "Error parsing printer %s\n", THIS->name);\
	   return (1); \
	} \
  } 


   while ( scan_next_block (s2)  ) {
      if      KEYWORD_PARSE (interface)
      else if KEYWORD_PARSE (driver)
      else if KEYWORD_PARSE (location)
      else if KEYWORD_PARSE (model)
      else if KEYWORD_TO_LIST_PARSE (driver_opts, driver_option_list,
         add_items_by_rc)
      else if KEYWORD_TO_LIST_PARSE (driver_args, driver_arg_list,
         add_item_pairs_by_rc)
      else if KEYWORD_TO_LIST_PARSE (interface_opts, interface_option_list,
         add_items_by_rc)
      else if KEYWORD_TO_LIST_PARSE (interface_args, interface_arg_list,
         add_item_pairs_by_rc)
      else if DELETE_PARSE (delete)
      else {
         fprintf (stderr, "Unknown keyword \"%s\" parsing printer %s\n", 
	    s2->token, p->name);
	 return (1); 
      }
   }

   free_parse_state (s2);

   if (  (p->driver == NULL) || (p->interface == NULL)  ) {
      fprintf (stderr, "Must supply driver and interface for %s\n", p->name);
      return (1); 
   }

   for (i = 0; i < debug_rc; i++)   fprintf (stdout, "  ");

   /* Overwrite this if it already exists */
   ltmp = find_in_list_by_string (*l, p->name);
   if (ltmp == NULL) {
      *l = append_to_list (*l, (void *)p);
      if (debug_rc) fprintf (stdout, "defining printer %s in %s\n",
      		p->name, s->file_name);
   } else {
      free_printer (ltmp->data);
      *l = replace_in_list (*l, ltmp->data, (void *)p);
      if (debug_rc) fprintf (stdout, "REDEFINING printer %s in %s\n",
      		p->name, s->file_name);
   }
   return (0);
  
}

void free_printer (printer *p) {

   if (p->name != NULL) free (p->name);
   if (p->interface != NULL) free (p->interface);
   if (p->driver != NULL) free (p->driver);
   if (p->model != NULL) free (p->model); 
   if (p->location != NULL) free (p->location); 


   list_iterate ( p->driver_option_list, free ); 
   list_iterate ( p->interface_option_list, free );
   list_iterate ( p->driver_arg_list, (void *)(void *)free_item_pair );
   list_iterate ( p->interface_arg_list, (void *)(void *)free_item_pair );
   
   p->driver_option_list = free_list ( p->driver_option_list );
   p->driver_arg_list = free_list ( p->driver_arg_list );
   p->interface_arg_list = free_list ( p->interface_arg_list );
   p->interface_option_list = free_list ( p->interface_option_list );

   free (p);

}

char *printer_to_str (printer *p) {

   time_t t;
   long len;
   char *out;
   char *name, *location, *model, *driver, *interface,
      *interface_option_list, *interface_arg_list, *driver_option_list,
      *driver_arg_list;

   if (p == NULL) {
      return (NULL);
   } else {
      name = escape_block (p->name, '"', '"');
      model = escape_block (p->model, '"', '"');
      location = escape_block (p->location, '"', '"');
      driver = escape_block (p->driver, '"', '"');
      interface = escape_block (p->interface, '"', '"');
      interface_option_list = item_list_to_str (p->interface_option_list); 
      interface_arg_list = item_pair_list_to_str (p->interface_arg_list); 
      driver_option_list = item_list_to_str (p->driver_option_list); 
      driver_arg_list = item_pair_list_to_str (p->driver_arg_list); 
      len = 2048 +
         strlen (name) +
         strlen (location) +
         strlen (model) +
         strlen (driver) +
         strlen (interface) +
         strlen (driver_option_list) +
         strlen (driver_arg_list) +
         strlen (interface_option_list) +
         strlen (interface_arg_list); 

      out = malloc (len);
      if (out == NULL) {
         fprintf (stderr, "Not enough memory.\n");
	 my_exit (1);
      }
      time(&t);
      sprintf (out, 
         "\n\nprinter %s {\n"
	    "\t# Added by the wizard on %s" 
	    "\tlocation %s\n"
	    "\tmodel %s\n"
	    "\tdriver %s\n"
	    "\tinterface %s\n"
	    "\tdriver_opts {%s}\n"
	    "\tdriver_args {%s}\n"
	    "\tinterface_opts {%s}\n"
	    "\tinterface_args {%s}\n"
	 "}\n",
	             name, 
                     ctime(&t),
		     location, model, driver, interface,
		     driver_option_list, driver_arg_list,
		     interface_option_list, interface_arg_list     );
      
      free (name); 
      free (location); 
      free (model); 
      free (driver); 
      free (interface); 
      free (driver_option_list); 
      free (driver_arg_list); 
      free (interface_option_list);
      free (interface_arg_list); 

   }
   return (out);
}

void set_printer (printer *p) {

   char *buf;

   buf = printer_to_str (p);

   rc_set ("printer", p->name, buf);
   
   free (buf);

}

void delete_printer (char *name) {

   char *buf, *clean_name;

   clean_name = escape_block (name, '"', '"');
   buf = malloc (strlen (clean_name) + 30);
   if (buf == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   sprintf (buf, "\n\nprinter %s delete\n", clean_name);
   free (clean_name);

   /* Buf is just a backup, in case someone wants to delete a printer
    * that is defined somewhere they have no permissions.  Buf gets
    * added to their local rc file.
    */


   rc_delete ("printer", name, buf);
   
   free (buf);

}

char *get_printer_status (printer *p, rc_items *rc) {

   int err_count, fd[2], n;
   long msg_len, i;
   char *namespace, *script, *arg[2], **env, msg_buf[2048], *msg;
   dl_list *msg_list, *l, *selection_lists[2];
   pid_t pid;
   fd_set read_set;
   struct timeval tv;
   reaped_item *r;
   job_info *job;

   msg_len = 0;
   msg_list = NULL;

   i = 0;

   job = new_job_info ();
   job->my_printer = copy_printer (p);

   l = find_in_list_by_string (rc->interface_list, p->interface);
   if (l == NULL) {
      msg = malloc (strlen (p->interface) + 50);
      if (msg == NULL) {
         fprintf (stderr, "Not enough memory.\n");
         my_exit (1);
      }
      sprintf (msg, "Interface %s is not defined in any of the rc files.",
         p->interface);
      free_job_info (job);
      return (msg);
   }
   job->my_interface = copy_interface ((interface *) l->data);

   l = find_in_list_by_string (rc->driver_list, p->driver);
   if (l == NULL) {
      msg = malloc (strlen (p->driver) + 50);
      if (msg == NULL) {
         fprintf (stderr, "Not enough memory.\n");
         my_exit (1);
      }
      sprintf (msg, "driver %s is not defined in any of the rc files.",
         p->driver);
      free_job_info (job);
      return (msg);
   }
   job->my_driver = copy_driver ((driver *) l->data);

   setup_job_environments (job, rc);

   if (job->my_interface->status_exec != NULL) {
      namespace = claim_tmpfile (rc->job_dir); 
      script = create_script (namespace, job->my_interface->status_exec, NULL);
      free (namespace);

      env = env_list_to_env (job->env_interface);

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
	       fprintf (stderr, "CHILD: Error plumbing status script.\n");
	       perror (NULL);
	       exit (1);
	    }
	    if (STDERR_FILENO != dup2 (fd[1], STDERR_FILENO)) {
	       fprintf (stderr, "CHILD: Error plumbing status script.\n");
	       perror (NULL);
	       exit (1);
	    }
	    arg[0] = script;
	    arg[1] = NULL;
	    if (0 != execve (script, arg, env) ) {
	       fprintf (stderr, "CHILD: Error execing status script.\n");
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
			   " status_exec.");
			my_exit (1);
		     }
		  } 
	       }


            }

	    close (fd[0]);

	    if ( WIFEXITED(r->status) == 0 ) {
	       str_set (&msg, "status_exec script exited abnormally.\n");
	       msg_list = append_to_list (msg_list, msg);
	       msg_len += strlen(msg);
	    }
	    if ( WEXITSTATUS(r->status) != 0 ) {
	       str_set (&msg, "\nThe status_exec script exited with nonzero"
		  " status.\n");
	       msg_list = append_to_list (msg_list, msg);
	       msg_len += strlen(msg);
	    }
	    free_reaped_item (r);
      }

      free_env (env);
      unlink (script);
      free (script);
   }

   /* Construct message */
   if ( (msg_len + i) > 0 ) {
      msg = malloc (msg_len + 1 + i);
      if (msg == NULL) {
         fprintf (stderr, "Not enough memory.\n");
	 my_exit (1);
      }
      strncpy (msg, msg_buf, i);
      msg[i] = 0;
      l = first_list_element (msg_list);
      while (l != NULL) {
	 strcat (msg, (char *)l->data);
	 free (l->data);
         l = l->next;
      }
      free_list (msg_list);
   } else {
      msg = malloc (50);
      if (msg == NULL) {
         fprintf (stderr, "Not enough memory.\n");
	 my_exit (1);
      }
      sprintf (msg, "The status_exec script had no output.");
   }

   free_job_info (job);

   return (msg);

}
