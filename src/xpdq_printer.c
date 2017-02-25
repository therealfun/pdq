/* Copyright 1999, 2000 Jacob A. Langford
 *
 * xpdq_printer.c
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

#include "xpdq.h"
#include "util.h"
#include "list.h"
#include "driver.h"
#include "interface.h"
#include "printer.h"
#include "argument.h"

void xpdq_add_printer (void) {

   pwizard_state *wizard;
   dl_list *list;

   list = first_list_element (rc->driver_list);
   if (list == NULL) {
      xpdq_error ("There are no printer drivers defined in\n"
         "/etc/printrc or ~/.printrc.  Without printer drivers,\n"
	 "the wizard cannot add printers.");
      return;
   }
   list = first_list_element (rc->interface_list);
   if (list == NULL) {
      xpdq_error ("There are no printer interfaces defined in\n"
         "/etc/printrc or ~/.printrc.  Without printer interfaces,\n"
	 "the wizard cannot add printers.");
      return;
   }
   wizard = new_pwizard ();
   wizard->p = new_printer ();
   /* create a main window */
   wizard->main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(GTK_WINDOW (wizard->main_window), "Xpdq add printer");
   gtk_signal_connect_object(GTK_OBJECT (wizard->main_window), "delete_event",
                      GTK_SIGNAL_FUNC(pwizard_cancel), (gpointer)(wizard));


   wizard->panel = pwizard_add_create_panel_1(wizard);
   gtk_container_add(GTK_CONTAINER(wizard->main_window), wizard->panel);
   wizard->panel_showing = 1;

   gtk_window_set_default_size(GTK_WINDOW(wizard->main_window), 500, 250);
   gtk_widget_show(wizard->main_window);


}

void xpdq_delete_printer (void) {

   dl_list *l;
   int row;
   printer *p;

   if (GTK_CLIST(printer_list_widget)->selection == NULL) return;
   row = (int) GTK_CLIST(printer_list_widget)->selection->data;
   p = gtk_clist_get_row_data (GTK_CLIST (printer_list_widget), row);
   if (p == NULL) {
      fprintf (stderr, "Should have found a printer for this row!\n");
      return;
   }
   delete_printer (p->name);
   l = find_in_list_by_string (rc->printer_list, p->name);
   if (l != NULL) rc->printer_list = remove_list_link (l);
   reconcile_printer_list ();
}

void xpdq_show_printer_status (void) {

   int row;
   printer *p;
   dl_list *l;
   char *buf;

   if (GTK_CLIST(printer_list_widget)->selection == NULL) return;
   row = (int) GTK_CLIST(printer_list_widget)->selection->data;
   p = gtk_clist_get_row_data (GTK_CLIST (printer_list_widget), row);
   if (p == NULL) {
      fprintf (stderr, "Should have found a printer for this row!\n");
      return;
   }

   show_details_window();
   gtk_text_freeze ( GTK_TEXT(details_widget) );
   erase_old_details ();

   add_details ("Status of printer ");;
   add_bold_details (p->name);;
   add_details (":\n\n");;

   buf = get_printer_status (p, rc);
   add_details (buf);
   free (buf);

   gtk_text_thaw ( GTK_TEXT(details_widget) );

}

void xpdq_set_as_default_printer (void) {

   int row;
   printer *p;
   char *buf;

   if (GTK_CLIST(printer_list_widget)->selection == NULL) return;
   row = (int) GTK_CLIST(printer_list_widget)->selection->data;
   p = gtk_clist_get_row_data (GTK_CLIST (printer_list_widget), row);
   if (p == NULL) {
      fprintf (stderr, "Should have found a printer for this row!\n");
      return;
   }

   buf = malloc (strlen(p->name) + 30);
   if (buf == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   sprintf (buf, "\n\ndefault_printer %s", p->name);
   rc_set ("default_printer", NULL, buf);
   free (buf);

}

void reconcile_printer_list (void) {

   printer *p;
   dl_list *l;
   int row;
   char *text[3];

   for (row = 0;
       (p = gtk_clist_get_row_data( 
       		GTK_CLIST(printer_list_widget), row) ) != NULL;
       row++) {

          if ( find_in_list (rc->printer_list, p) == NULL ) 
	     gtk_clist_remove (GTK_CLIST (printer_list_widget), row);
   }

   l = first_list_element (rc->printer_list);
   while (l != NULL) {
      p = (printer *)l->data;
      row = gtk_clist_find_row_from_data ( GTK_CLIST (printer_list_widget), p);
      if (row == -1) {
         text[0] = p->name;
         text[1] = p->model;
         text[2] = p->location;
         row = gtk_clist_append (GTK_CLIST (printer_list_widget), text);
         gtk_clist_set_row_data (GTK_CLIST (printer_list_widget), row, p);
	 if ( (rc->default_printer != NULL) &&
	      (strcmp(rc->default_printer, p->name) == 0) ) {
	          	gtk_clist_select_row (
				GTK_CLIST(printer_list_widget),
				row, 0);
         }
         verify_printer (row, p, 0);
      }
      l = l->next;
   }
   if (GTK_CLIST(printer_list_widget)->selection == NULL) return;
   row = (int) GTK_CLIST(printer_list_widget)->selection->data;
   gtk_clist_moveto ( GTK_CLIST(printer_list_widget), row, -1, 0.3, 0);
}

void show_printer_problems (void) {

   int row;
   printer *p;

   if (GTK_CLIST(printer_list_widget)->selection == NULL) return;
   row = (int) GTK_CLIST(printer_list_widget)->selection->data;
   p = gtk_clist_get_row_data (GTK_CLIST (printer_list_widget), row);
   if (p == NULL) {
      fprintf (stderr, "Should have found a job for this row!\n");
      return;
   }

   verify_printer (row, p, 1);

}

int verify_printer (int row, printer *p, int verbose) {

   int compatibility;

   dl_list *ll;
   interface *i;
   driver *d;
   char *err;

   if (verbose == 1) {
      show_details_window();
      gtk_text_freeze ( GTK_TEXT(details_widget) );
      erase_old_details ();
      add_details ("Verification of printer ");
      add_bold_details (p->name);
      add_details (":\n\n");
   }

   compatibility = 1;
   ll = find_in_list_by_string (rc->interface_list, p->interface);
   if (ll == NULL) {
      compatibility = 0;
      if (verbose == 1) {
	 add_details ("Interface ");
	 add_details (p->interface);
	 add_details (" is not defined in any of the rc files.");
      }
   } else {
      i = (interface *) ll->data;
      check_interface_compatibility (i, rc);
      if (i->compatibility == 2) {
	 compatibility = 0;
	 if (verbose == 1) {
	    add_details ("Interface ");
	    add_details (p->interface);
	    add_details (" had the following problems:\n");
	    add_details (i->incompatibility_msg);
	 }
      }
      err = verify_arguments (i->required_arg_list, NULL,
	     p->interface_arg_list);
      if (err != NULL) {
	 if (verbose == 1) {
	    add_details (err);
	 }
	 compatibility = 0;
	 free (err);
      }
   }
   ll = find_in_list_by_string (rc->driver_list, p->driver);
   if (ll == NULL) {
      compatibility = 0;
      if (verbose == 1) {
	 add_details ("Driver ");
	 add_details (p->driver);
	 add_details (" is not defined in any of the rc files.");
      }
   } else {
      d = (driver *) ll->data;
      check_driver_compatibility (d, rc);
      if (d->compatibility == 2) {
	 compatibility = 0;
	 if (verbose == 1) {
	    add_details ("Driver ");
	    add_details (p->driver);
	    add_details (" had the following problems:\n");
	    add_details (d->incompatibility_msg);
	 }
      }
      err = verify_arguments (d->required_arg_list, NULL,
	     p->driver_arg_list); 
      if (err != NULL) {
	 if (verbose == 1) {
	    add_details (err);
	 }
	 compatibility = 0;
	 free (err);
      }
   }
   if (compatibility == 0) {
	 gtk_clist_set_background (
		  GTK_CLIST (printer_list_widget), row, error_color);
         if (verbose == 1) {
	    add_details ("This printer will not work on your system.");
	 }
   } else {
	 gtk_clist_set_background (
		  GTK_CLIST (printer_list_widget), row, ok_color);
         if (verbose == 1) {
	    add_details ("This printer is configured properly and "
	    	"should work on your system.");
	 }
   }

   if (verbose == 1) {
      gtk_text_thaw ( GTK_TEXT(details_widget) );
   }

   return (compatibility);
}

void printer_selected ( GtkWidget *widget, gint row, 
   gint column, GdkEventButton *event) {

   if (driver_option_window != NULL) {
      free_option_state (driver_option_state);
      driver_option_state = NULL;
      twiddle_driver_options();
   }

   if (interface_option_window != NULL) {
      free_option_state (interface_option_state);
      interface_option_state = NULL;
      twiddle_interface_options();
   }

   return;
}
