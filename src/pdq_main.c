/* Copyright 1999, 2000 Jacob A. Langford
 *
 * pdq_main.c
 *
 * CHANGES:
 *      Mon Jan 10 20:19:11 CST 2000 - JAL
 *          malloc(0) = 0 on AIX.  Causes trouble with STDIN
 *          also fix STDIN hack on fork which could be trouble.
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
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>

#include "rc_items.h"
#include "parse_rc.h"
#include "list.h"
#include "util.h"
#include "job.h"
#include "argument.h"
#include "pdq_usage.h"
#include "shepherd.h"
#include "reaper.h"

                 
rc_items *rc;     
job_info *job;   

void (*my_exit) (int code);

int main (int argc, char *argv[]) {

   pid_t pid;
   dl_list *l;
   time_t t;
   int err_count, i, ifile, *job_fd;
   char **file, *err;
   int nfiles;

   my_exit = pdq_exit;

   reaped_list = NULL;
   signal (SIGCHLD, reaper); 

   rc = new_rc_items();
   job = new_job_info();

   time(&t);
   str_reset (&job->invoke_time, ctime(&t));
   job->invoke_time [strlen(job->invoke_time)-1] = 0; /* Strip \n */

   pdq_process_argv (argc, argv, job, rc, &file, &nfiles);
   try_parse_rc_glob (PRINTRC_FILE, rc);
   try_parse_rc_glob ("~/.printrc", rc);

   if (rc->debug_rc)  exit (0);

   if (job->do_help) {
      show_overview (job->argv0);
      if (job->printer == NULL) {
         show_printer_list (rc);
      } else {
         show_printer_options (rc, job->printer);
      }
      exit (0);
   }

   /*****************************************************************
    * Select printer
    */

   if (  (job->printer == NULL)  ) {
      if (rc->default_printer == NULL) {
         fprintf (stderr, "Error.  No printer specified, "
       	         "and no default printer defined.\n");
         my_exit (1);
      } else {
         str_reset (&job->printer, rc->default_printer);
      }
   }

   l = find_in_list_by_string (rc->printer_list, job->printer);
   if (l == NULL) {
      fprintf (stderr, "Error.  Printer %s is not defined in any of "
            	"the rc files.\n", job->printer );
      my_exit (1);
   } 
   job->my_printer = (printer *)l->data;
   rc->printer_list = remove_list_link(l);

   list_iterate (rc->printer_list, (void *)(void *)free_printer);
   rc->printer_list = free_list (rc->printer_list);



   /*****************************************************************
    * Select and verify driver
    */

   l = find_in_list_by_string (rc->driver_list, job->my_printer->driver);
   if (l == NULL) {
      fprintf (stderr, "Error.  Driver %s is not defined in any of "
            	"the rc files.\n", job->my_printer->driver );
      my_exit (1);
   } 
   job->my_driver = (driver *)l->data;
   str_reset (&job->driver, job->my_driver->name);
   /* printf ("%s\n",  item_pair_list_to_str
    * (pdq->my_driver->required_arg_list));*/
   rc->driver_list = remove_list_link(l);
   list_iterate (rc->driver_list, (void *)(void *)free_driver);
   rc->driver_list = free_list (rc->driver_list);

   check_driver_compatibility (job->my_driver, rc);

   if (job->my_driver->compatibility == 2) {
       fprintf (stderr, "%s\nCannot use driver %s.\n", 
          job->my_driver->incompatibility_msg, job->my_driver->name);
       my_exit (1); 
   }
   err = verify_arguments (job->my_driver->required_arg_list,
   	      job->extra_driver_arg_list, job->my_printer->driver_arg_list);
   if (err != NULL) {
       fprintf (stderr, "%sCannot use driver %s.\n", err,
          job->my_driver->name);
       free (err);
       my_exit (1); 
   }


   /*****************************************************************
    * Select and verify interface
    */

   l = find_in_list_by_string (rc->interface_list,
   	job->my_printer->interface);
   if (l == NULL) {
      fprintf (stderr, "Error.  Interface %s is not defined in any of "
            	"the rc files.\n", job->my_printer->interface );
      my_exit (1);
   } 
   job->my_interface = (interface *)l->data;
   str_reset (&job->interface, job->my_interface->name);
   rc->interface_list = remove_list_link(l);
   list_iterate (rc->interface_list, (void *)(void *)free_interface);
   rc->interface_list = free_list (rc->interface_list);
   check_interface_compatibility (job->my_interface, rc);

   if (job->my_interface->compatibility == 2) {
       fprintf (stderr, "%s\nCannot use interface %s.\n", 
          job->my_interface->incompatibility_msg, job->my_interface->name);
       my_exit (1); 
   }
   err = verify_arguments (job->my_interface->required_arg_list,
   	      job->extra_interface_arg_list, 
	      job->my_printer->interface_arg_list);
   if (err != NULL) {
       fprintf (stderr, "%sCannot use interface %s.\n", err,
          job->my_interface->name);
       free (err);
       my_exit (1); 
   }



   /* We need to open the input files here, before the main thread returns.
    * Some applications, such as gv, create a tmp file, print the tmp file,
    * then immediately unlink the file.  So we need to hold that file open.
    */
   if (nfiles >= 0) {
      job_fd = malloc (sizeof(int) * (nfiles + 1));  /* (nfliles + 1) just
                                                      * to ensure we are
						      * mallocing something
						      * nonzero.
						      */
      if (job_fd == NULL) {
         fprintf (stderr, "Not enough memory.\n");
	 my_exit (1);
      }
      err_count = 0;
      for (i = 0; i < nfiles; i++) {
         job_fd[i] = open (file[i], O_RDONLY);
	 if (job_fd[i] == -1) {
	    fprintf (stderr, "Couldn't open %s for reading, ", 
		   file[i]);
	    perror (NULL);
	    err_count++;
	 }

      }
      if (err_count > 0) {
	 fprintf (stderr, "WARNING: %i of %i files will not be printed.\n",
	    err_count, nfiles);
      }
   }

   /* Fork once for each print job.  Main thread returns. 
    * Who's your daddy?  init
    */
   pid = 1;  /* So that parent will exit if there are no forks... */
   for (ifile = 0; ifile < nfiles; ifile++) {   /* 1/10/00 - JAL */

      if (job_fd[ifile] == -1) continue;

      pid = fork();
      if (pid == -1) {
         fprintf (stderr, "Can't fork.\n"); 
         my_exit (1);
      }
      if (pid == 0) break;
   }
   if (nfiles == 0) { /* 1/10/00 - JAL after modification, 
                       * last loop didn't execute.  Let's fork for 
                       * a stdin job.
		       */
      pid = fork();
      if (pid == -1) {
         fprintf (stderr, "Can't fork.\n"); 
         my_exit (1);
      }
   }

   if (pid != 0) return (0);  /* Main thread stops here */

   /* There are a lot of extra open files here... */
   if (nfiles >= 0) {
      for (i = 0; i < nfiles; i++) {
         if (ifile == i) continue;  /* Keep our own file open */
	 if (job_fd[i] == -1) continue;  
         close (job_fd[i]);
      }
   }

   str_reset (&job->printer , job->my_printer->name);
   str_reset (&job->interface , job->my_interface->name);
   str_reset (&job->driver , job->my_driver->name);
   job->fd = job_fd[ifile];

   /* Children become shepherds for the print job */

   if (nfiles != 0) {
      str_reset (&job->input_filename, file[ifile]);
   }

   shepherd (job, rc);  

   my_exit(0);
   
   return (0);   /* Unreached... stops compiler warnings */

}


