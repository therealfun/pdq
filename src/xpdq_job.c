/* Copyright 1999, 2000 Jacob A. Langford
 *
 * xpdq_job.c
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

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "xpdq.h"
#include "util.h"
#include "list.h"
#include "driver.h"
#include "interface.h"
#include "printer.h"
#include "argument.h"
#include "job.h"
#include "reaper.h"
#include "shepherd.h"

extern dl_list *job_info_list;

void xpdq_cancel_print_job (void) {

   struct timeval tv;
   struct stat statbuf;
   int row, i, j, fd;
   char *cancel_flag, *script, *arg[2], **env, *job_log_file;
   job_info *job;
   dl_list *l;
   pid_t pid;
   reaped_item *r;
   time_t t;

   if (GTK_CLIST(job_list_widget)->selection == NULL) return;
   row = (int) GTK_CLIST(job_list_widget)->selection->data;
   job = gtk_clist_get_row_data (GTK_CLIST (job_list_widget), row);
   if (job == NULL) {
      fprintf (stderr, "Should have found a job for this row!\n");
      return;
   }

   cancel_flag = malloc (strlen (job->job_base) + 24);
   if (cancel_flag == NULL) {
       fprintf (stderr, "Not enough memory.\n");
       my_exit (1);
   }
   sprintf (cancel_flag, "%s.cancelled", job->job_base);
   close ( open (cancel_flag, O_CREAT|O_WRONLY, 0600)) ; /* mknod() */


   sprintf (cancel_flag, "%s.ack_cancelled", job->job_base);

   for (j = 0; j < 50; j++) {
    
         /* Spend about 5 seconds looking for an acknowledgement
	  * from pdq.  This may be annoying to the user, but it 
	  * is an easy way for now.  A GTK timeout function is 
	  * probably more appropriate.  Cancel support is a royal 
	  * pain in the ass... 
          */ 

	 tv.tv_sec = 0;         /* Block for 100 ms */
	 tv.tv_usec = 100000;
	 select (0, NULL, NULL, NULL, &tv);

	 i = stat (cancel_flag, &statbuf);

	 if (i == 0) break;
   }


   if (i == -1) {         /* No acknowledgement ever got... */ 

      if (job->my_printer == NULL) {
	 l = find_in_list_by_string (rc->printer_list, job->printer);
	 if (l == NULL) {
	    xpdq_error ("Cannot cancel this job.\nThe printer is no longer\n"
	      "defined in the rc files.\n");
	    return;
	 }
	 job->my_printer = copy_printer ((printer *)l->data);
      }
      if (job->my_interface == NULL) {
	 l = find_in_list_by_string (rc->interface_list, 
	      job->my_printer->interface);
	 if (l == NULL) {
	    xpdq_error ("Cannot cancel this job.\nThe interface is no longer\n"
	      "defined in the rc files.\n");
	    return;
	 }
	 job->my_interface = copy_interface((interface *)l->data);
      }

      if (job->my_interface->cancel_exec == NULL) {
         xpdq_error (
	     "Cannot cancel this job.\n"
	     "It appears to have already completed,\n"
	     "and the printer's interface does not\n"
	     "define a way to cancel.\n" 
							     );
         return;
      }

      pid = fork();
      switch (pid) {
	 case -1:
	    xpdq_error ("Error forking: cannot cancel.");
	    return;
	 case 0:
	    job_log_file = malloc ( strlen (job->job_base) + 25 );
	    if (job_log_file == NULL) {
	       fprintf (stderr, "Not enough memory: cancel failed\n");
	       return;
	    }
	    sprintf (job_log_file, "%s.log", job->job_base);
	    fd = open (job_log_file, O_CREAT | O_APPEND | O_WRONLY, 0600);
	    free (job_log_file);
	    if (fd == -1) {
	       fprintf  (stderr, "Error opening job log file: "
			"cancel failed.\n");
	       return;
	    }

	    if (STDOUT_FILENO != dup2 (fd, STDOUT_FILENO)) {
	       fprintf (stderr, "Error redirecting stdout: cannot cancel.\n");
	       return;
	    }
	    if (STDERR_FILENO != dup2 (fd, STDERR_FILENO)) {
	       fprintf (stderr, "Error redirecting stderr: cannot cancel.\n");
	       return;
	    }
	    time(&t);
	    fprintf (stderr, "The pdq shepherd does not seem to still be "
	    	"running.\nxpdq executing cancel_exec at %s\n", ctime (&t));

	    script = create_script (job->job_base, 
	    		job->my_interface->cancel_exec, "cancel");

	    /* Set up environment */
	    env = env_list_to_env (job->env_interface);

	    arg[0] = script;
	    arg[1] = NULL;
	    if (0 != execve (script, arg, env) ) {
	       fprintf (stderr, "Could not execute cancel script.\n");
	       return;
	    }
	    break;
	 default:

	    while (  (r = get_reaped_by_pid (pid)) == NULL  ) {
	       tv.tv_sec = 0;         /* Block for 100 ms */
	       tv.tv_usec = 100000;
	       select (0, NULL, NULL, NULL, &tv);
	    }

	    if ( WIFEXITED(r->status) == 0 ) {
	       xpdq_error ("The cancel script failed abnormally.\n"
	                   "Click \"show details\" to check the log file.\n");
               force_set_status ("cancelled (xpdq, errors)", job);
               return;
	    }
	    if ( WEXITSTATUS(r->status) != 0 ) {
	       xpdq_error ("The cancel script exited with an error.\n"
	                   "Click \"show details\" to check the log file.\n");
               force_set_status ("cancelled (xpdq, errors)", job);
               return;
	    }
	   
	    free_reaped_item (r);
      
      }

      force_set_status ("cancelled (xpdq)", job);

   }

   update_job_info();

   free (cancel_flag);

}

void xpdq_show_job_details (void) {

   char buf[1024], *jdir;
   long len;
   int row;
   item_pair *i;
   FILE *log;
   job_info *job;
   dl_list *l;
   
   if (GTK_CLIST(job_list_widget)->selection == NULL) return;
   row = (int) GTK_CLIST(job_list_widget)->selection->data;
   job = gtk_clist_get_row_data (GTK_CLIST (job_list_widget), row);
   if (job == NULL) {
      fprintf (stderr, "Should have found a job for this row!\n");
      return;
   }
   show_details_window();
   gtk_text_freeze ( GTK_TEXT(details_widget) );
   erase_old_details ();
   add_bold_details ("**************************************************\n");
   add_bold_details (job->status);
   add_bold_details ("\n**************************************************\n");
   jdir = tilde_expand (rc->job_dir);
   sprintf (buf, "%s/%.3i.log", jdir, job->id);
   free (jdir);
   log = fopen (buf, "r");
   if (log != NULL) {
      add_bold_details ("\nContents of log: \n");
      while (1) {
         len = fread (buf, 1, 1023, log);
	 buf[len] = 0;
	 add_details (buf);
	 if (len < 1023) break;
      }
      fclose (log);
   }
   sprintf (buf, "\nDetailed information for job ");
   add_details (buf);
   sprintf (buf, "%.3i:\n\n", job->id);
   add_bold_details (buf);
   add_bold_details ("Invoked by: ");
   add_details (job->invoked_by);
   add_bold_details ("\nTime submitted: ");
   add_details (job->invoke_time);
   add_bold_details ("\nFile type: ");
   add_details (job->file_type);
   add_bold_details ("\nStatus: ");
   add_details (job->status);
   add_bold_details ("\nPrinter: ");
   add_details (job->printer);
   add_bold_details ("\n\nDriver: ");
   add_details (job->driver);
   add_bold_details ("\nInterface: ");
   add_details (job->interface);
   add_bold_details ("\nLanguage driver: ");
   add_details (job->language_driver);
   add_bold_details ("\nDriver environment:\n");
   l = first_list_element (job->env_driver);
   while (l != NULL) {
      i = (item_pair *)l->data;
      add_details ("\t");
      add_details (i->key);
      add_details (" = ");
      add_details (i->value);
      add_details ("\n");
      l = l->next;
   }
   add_bold_details ("\nInterface environment:\n");
   l = first_list_element (job->env_interface);
   while (l != NULL) {
      i = (item_pair *)l->data;
      add_details ("\t");
      add_details (i->key);
      add_details (" = ");
      add_details (i->value);
      add_details ("\n");
      l = l->next;
   }
   gtk_text_thaw ( GTK_TEXT(details_widget) );

   len = 0;

}

void xpdq_redirect_job (void) {

   char *dname;
   int row;
   job_info *job;

   if (GTK_CLIST(job_list_widget)->selection == NULL) return;
   row = (int) GTK_CLIST(job_list_widget)->selection->data;
   job = gtk_clist_get_row_data (GTK_CLIST (job_list_widget), row);
   if (job == NULL) {
      fprintf (stderr, "Should have found a job for this row!\n");
      return;
   }
   dname = malloc (strlen(job->job_base) + 10);
   if (dname == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   sprintf (dname, "%s.raw", job->job_base);
   xpdq_print_something (job->input_filename, dname);
   free (dname);
}

void xpdq_delete_job (void) {

   int row;
   job_info *job;

   if (GTK_CLIST(job_list_widget)->selection == NULL) return;
   row = (int) GTK_CLIST(job_list_widget)->selection->data;
   job = gtk_clist_get_row_data (GTK_CLIST (job_list_widget), row);
   if (job == NULL) {
      fprintf (stderr, "Should have found a job for this row!\n");
      return;
   }
   delete_job (job->id, rc->job_dir);
   update_job_info();

}

int update_job_info (void) {

   dl_list *jobs, *l, *lremove;
   job_info *job, *job_bak;
   int id, row;
   char *text[10], *b;

   /* First check on jobs we already know about */
   lremove = NULL;
   l = first_list_element (job_info_list);
   while (l != NULL) {
      job = (job_info *)l->data;
      job_bak = job;
      if ( get_status_update (&job, rc) ) {
	 row = gtk_clist_find_row_from_data (
	    GTK_CLIST (job_list_widget), job_bak);
	 if (row == -1) {
	    fprintf (stderr, "Should have found row for this job!\n");
	    my_exit (1);
	 }
         if (job == NULL) {
	    /* This job is gone... */
	    gtk_clist_remove (GTK_CLIST (job_list_widget), row);
	    lremove = l;
	 } else {
            gtk_clist_set_text ( GTK_CLIST (job_list_widget),
	        row, 1, job->status);
            gtk_clist_set_text ( GTK_CLIST (job_list_widget),
	        row, 3, job->file_type);
	    set_status_color ( job->status, job_list_widget, row);
            if (  (GTK_CLIST(job_list_widget)->selection != NULL) 
	       && (row == (int)GTK_CLIST(job_list_widget)->selection->data)
	       && (details_window != NULL) ) {
	          xpdq_show_job_details();
	    } 
	 }
      }
      l = l->next;
      if (lremove != NULL) {
         job_info_list = remove_list_link (lremove);
	 lremove = NULL;
      }
   }

   /* Now look for new jobs */
   jobs = job_list (rc->job_dir);
   text[0] = malloc (5);
   l = first_list_element (jobs);
   while (l != NULL) {
      id = (int)l->data; 
      if (  find_in_list_by_int (job_info_list, id) == NULL  ) {
         job = get_status (id, rc);
	 if (job == NULL) {
	    l = l->next;
	    continue;
	 }
         job_info_list = prepend_to_list (job_info_list, job);
	 sprintf (text[0], "%.3i", id);
	 text[1] = job->status;
	 /* Strip leading path from filename */
	 b = strrchr (job->input_filename, '/');
	 if (b == NULL) {
	    text[2] = job->input_filename;
	 } else {
	    text[2] = b+1;
	 }
	 text[3] = job->file_type;
	 text[4] = job->printer;
	 text[5] = job->invoke_time;
         row = gtk_clist_prepend (GTK_CLIST (job_list_widget), text);
         gtk_clist_set_row_data (GTK_CLIST (job_list_widget), 
	 	row, job);
	 set_status_color ( job->status, job_list_widget, row);
      }
      l = l->next;
   }
   free (text[0]);

   free_list (jobs);

   return (TRUE);  /* For gtk to keep calling us */

}

void set_status_color ( char *status, GtkWidget *job_list_widget, int row ) {
	    if (str_match(status, "bort") != 0) {
	       gtk_clist_set_background (
	         	GTK_CLIST (job_list_widget), row, error_color);
	    } else if (str_match (status, "abort") != 0) {
	       gtk_clist_set_background (
	       		GTK_CLIST (job_list_widget), row, error_color); 
	    } else if (str_match (status, "error") != 0) {
	       gtk_clist_set_background (
	       		GTK_CLIST (job_list_widget), row, error_color); 
	    } else if (str_match (status, "fail") != 0) {
	       gtk_clist_set_background (
	       		GTK_CLIST (job_list_widget), row, error_color); 
	    } else if (str_match (status, "finish") != 0) {
	       gtk_clist_set_background (
	       		GTK_CLIST (job_list_widget), row, ok_color); 
	    } else if (str_match (status, "success") != 0) {
	       gtk_clist_set_background (
	       		GTK_CLIST (job_list_widget), row, ok_color); 
	    } else if (str_match (status, "ok") != 0) {
	       gtk_clist_set_background (
	       		GTK_CLIST (job_list_widget), row, ok_color); 
	    } else if (str_match (status, "done") != 0) {
	       gtk_clist_set_background (
	       		GTK_CLIST (job_list_widget), row, ok_color); 
	    } else if (str_match (status, "cancel") != 0) {
	       gtk_clist_set_background (
	       		GTK_CLIST (job_list_widget), row, ok_color); 
	    } else {
	       gtk_clist_set_background (
	       		GTK_CLIST (job_list_widget), row, busy_color);
	    }
}

void job_selected ( GtkWidget *widget, gint row, 
   gint column, GdkEventButton *event) {

   if (details_window != NULL) {
      xpdq_show_job_details();      
   }

   return;
}
