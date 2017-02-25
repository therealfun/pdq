/* Copyright 1999, 2000 Jacob A. Langford
 *
 * pdq_usage.c
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
#include <string.h>
#include <stdlib.h>

#include "job.h"
#include "util.h"
#include "pdq_usage.h"
#include "printer.h"
#include "option.h"
#include "driver.h"
#include "interface.h"
#include "argument.h"

void pdq_process_argv (int argc, char *argv[], job_info *job, rc_items *rc,
   char ***file, int *nfiles) {

   int i, len;
   char *s, *argv0;

   /* Save calling sequence to add to logs */
   len = 0;
   for (i = 0; i < argc; i++) {
      len += strlen (argv[i]) + 1;
   }
   job->invoked_by = malloc ( len + 1 );
   if (job->invoked_by == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   strcpy (job->invoked_by, argv[0]);
   for (i = 1; i < argc; i++) {
      strcat (job->invoked_by, " ");
      strcat (job->invoked_by, argv[i]);
   }

   /* Save this for pdq_usage() calls */
   str_set(&job->argv0, argv[0]);
   argv0 = argv[0]; argc--;

   NEXT_ARGUMENT:
   if (argc > 0) {
      argv++; argc--; 
      if ( argv[0][0] == '-' ) {
      /* NEXT_CHARACTER:           Not currently used... no one char options */
	 argv[0]++;
	 switch (argv[0][0]) {
	    case '0': 
	       goto NEXT_ARGUMENT;
	    case '-': /* --long-style-option-names */
	       s = argv[0] + 1; 
	       if ( strcmp (s, "help") == 0 ) {
	          job->do_help = 1;
		  goto NEXT_ARGUMENT;
	       } else if ( strcmp (s, "debug-rc") == 0 ) {
	          rc->debug_rc = 1;
		  goto NEXT_ARGUMENT;
	       } else {
		  fprintf (stderr, "Unknown option --%s\n\n", s);
		  pdq_usage (argv0);
	       }
	    case 'h': 
	       job->do_help = 1;
	       goto NEXT_ARGUMENT;
	    case 'o': 
	       s = argv[0] + 1;
	       if (*s == 0) {
		  argv++; argc--;
		  s = argv[0];
	       }
	       if (add_items_by_string (s, &job->extra_driver_opt_list)) {
	          fprintf (stderr, "Error resolving -o option\n");
		  my_exit(1);
	       }
	       goto NEXT_ARGUMENT;
	    case 'O': 
	       s = argv[0] + 1;
	       if (*s == 0) {
		  argv++; argc--;
		  s = argv[0];
	       }
	       if (add_items_by_string (s, &job->extra_interface_opt_list)) {
	          fprintf (stderr, "Error resolving -O option\n");
		  my_exit(1);
	       }
	       goto NEXT_ARGUMENT;
	    case 'a': 
	       s = argv[0] + 1;
	       if (*s == 0) {
		  argv++; argc--;
		  s = argv[0];
	       }
	       if (add_item_pairs_by_string (s, &job->extra_driver_arg_list)){
	          fprintf (stderr, "Error resolving -a option\n");
		  my_exit(1);
	       }
	       goto NEXT_ARGUMENT;
	    case 'A': 
	       s = argv[0] + 1;
	       if (*s == 0) {
		  argv++; argc--;
		  s = argv[0];
	       }
	       if (add_item_pairs_by_string (s,
			&job->extra_interface_arg_list)) {
	          fprintf (stderr, "Error resolving -A option\n");
		  my_exit(1);
	       }
	       goto NEXT_ARGUMENT;
	    case 'd':
	       s = argv[0] + 1;
	       if (*s == 0) {
		  argv++; argc--;
		  s = argv[0];
	       }
	       str_reset (&job->printer, s);
	       goto NEXT_ARGUMENT;
	    case 'P': 
	       s = argv[0] + 1;
	       if (*s == 0) {
		  argv++; argc--;
		  s = argv[0];
	       }
	       str_reset (&job->printer, s);
	       goto NEXT_ARGUMENT;
	    default:
	       fprintf (stderr, "Unknown option -%c\n\n", argv[0][0]);
	       pdq_usage (argv0);
	 }
      }
      argc++;
   }



   *nfiles = argc;
   if (*nfiles > 0) {
      *file = malloc ( (*nfiles) * sizeof (char *));
      if (*file == NULL) {
         fprintf (stderr, "Not enough memory.");
      }
      for (i = 0; i < *nfiles; i++) {
	 str_set ((*file + i), argv[i]);
      }
   } 

}

void show_overview (char *argv0) {
   fprintf (stderr, 
      "Usage:\t%s [options] file1 file2 ... fileN\n"
      "options:\n\t-o driver_opt\n\t-a driver_arg=VALUE\n\t-O interface_opt\n\t"
      "-A interface_arg=VALUE\n\t-P printer | -d printer\n\t-h | --help\n"
      "\t--debug-rc        (Name rc files as they contribute resources.)\n\n"
      , argv0);

}

void pdq_usage (char *argv0) {

   show_overview (argv0);
   exit (1);

}

void show_printer_list (rc_items *rc) {

   dl_list *l;
   printer *p;

   l = first_list_element(rc->printer_list);
   if (l == NULL) {
      fprintf (stderr, "No printers are defined.  Run xpdq to create some.\n");
   } else {
      fprintf (stderr, "The following printers are available:\n");
      while (l != NULL) {
         p = (printer *)l->data; 
	 fprintf (stderr, "\t%s", p->name);
	 if (p->model != NULL) fprintf (stderr, "  -  %s", p->model);
	 if (p->location != NULL) fprintf (stderr, "  -  %s", p->location);
	 fprintf (stderr, "\n");
	 l = l->next;
      }
      if (rc->default_printer != NULL) {
         fprintf (stderr, "The default printer is %s\n", rc->default_printer);
      }
   }
}

void show_printer_options (rc_items *rc, char *p_name) {

   printer *p;
   driver *d;
   interface *i;
   option *o;
   choice *ch;
   argument *a;
   dl_list *l, *ll;

   l = find_in_list_by_string (rc->printer_list, p_name);
   if (l == NULL) {
      fprintf (stderr, "Printer %s is not defined.\n", p_name);
      show_printer_list (rc);
      return;
   }
   p = (printer *)l->data;
   fprintf (stderr, "Printer %s takes the following options:\n", p->name);
   /* Show all options and all declared arguments */

   l = find_in_list_by_string (rc->driver_list, p->driver);
   if (l == NULL) {
      fprintf (stderr, "Driver %s is not defined.\n", p->driver);
      return;
   }
   d = (driver *)l->data;

   l = find_in_list_by_string (rc->interface_list, p->interface);
   if (l == NULL) {
      fprintf (stderr, "Interface %s is not defined.\n", p->interface);
      return;
   }
   i = (interface *)l->data;

   fprintf (stderr, "Interface arguments:\n");
   l = first_list_element (i->arg_list);
   while (l != NULL) {
      a = (argument *)l->data;
      fprintf (stderr, "\t%s : %s\n", a->var, a->desc);
      l = l->next; 
   }

   fprintf (stderr, "Interface options:\n");
   l = first_list_element (i->option_list);
   while (l != NULL) {
      o = (option *)l->data;
      fprintf (stderr, "\t%s\n", o->desc);
      ll = first_list_element (o->choice_list);
      while (ll != NULL) {
         ch = (choice *)ll->data;
	 fprintf (stderr, "\t\t-o%s : %s\n", ch->name, ch->desc);
         ll = ll->next;
      }
      l = l->next; 
   }

   fprintf (stderr, "Driver arguments:\n");
   l = first_list_element (d->arg_list);
   while (l != NULL) {
      a = (argument *)l->data;
      fprintf (stderr, "\t%s : %s\n", a->var, a->desc);
      l = l->next; 
   }

   fprintf (stderr, "Driver options:\n");
   l = first_list_element (d->option_list);
   while (l != NULL) {
      o = (option *)l->data;
      fprintf (stderr, "\t%s\n", o->desc);
      ll = first_list_element (o->choice_list);
      while (ll != NULL) {
         ch = (choice *)ll->data;
	 fprintf (stderr, "\t\t-o%s : %s\n", ch->name, ch->desc);
         ll = ll->next;
      }
      l = l->next; 
   }


}
