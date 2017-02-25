/* Copyright 1999, 2000 Jacob A. Langford
 *
 * xpdq_wizard.c
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

pwizard_state *new_pwizard() {

   pwizard_state *wizard;

   wizard = malloc (sizeof (pwizard_state));
   if (wizard == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   wizard->p = NULL;
   wizard->main_window = NULL;
   wizard->panel = NULL;
   wizard->panel_showing = 1;

   return (wizard);

}

void pwizard_free (pwizard_state *wizard) {

   gtk_widget_destroy (wizard->main_window);
   free (wizard);
   reconcile_printer_list ();

}

void pwizard_cancel (pwizard_state *wizard) {

   pwizard_free (wizard);

}

void pwizard_next (pwizard_state *wizard) {

   int i;
   GtkWidget *panel;

   i = wizard->panel_showing;

   switch (i) {
      case (1):
         panel = pwizard_add_create_panel_2 (wizard);
	 break;
      case (2):
         if (pwizard_check_name (wizard) != 0) return;
         panel = pwizard_add_create_panel_3 (wizard);
	 break;
      case (3):
         panel = pwizard_add_create_panel_4 (wizard);
	 break;
      case (4):
         if (pwizard_check_driver (wizard) != 0) return;
         panel = pwizard_add_create_panel_5 (wizard);
	 break;
      case (5):
         panel = pwizard_add_create_panel_6 (wizard);
	 break;
      case (6):
         if (pwizard_check_interface (wizard) != 0) return;
         panel = pwizard_add_create_panel_7 (wizard);
	 break;
      case (7):
         /* We finished... add the printer, destroy the wizard */
	 set_printer (wizard->p);
         rc->printer_list = append_to_list (rc->printer_list, wizard->p);
	 pwizard_free (wizard);
         return;
      default:
         return;
   }

   gtk_container_remove (GTK_CONTAINER(wizard->main_window), wizard->panel);
   wizard->panel = panel;
   gtk_container_add(GTK_CONTAINER(wizard->main_window), wizard->panel);
   wizard->panel_showing++;

}

void pwizard_prev (pwizard_state *wizard) {

   int i;
   GtkWidget *panel;

   i = wizard->panel_showing;

   switch (i) {
      case (2):
         panel = pwizard_add_create_panel_1 (wizard);
	 break;
      case (3):
         panel = pwizard_add_create_panel_2 (wizard);
	 break;
      case (4):
         panel = pwizard_add_create_panel_3 (wizard);
	 break;
      case (5):
         panel = pwizard_add_create_panel_4 (wizard);
	 break;
      case (6):
         panel = pwizard_add_create_panel_5 (wizard);
	 break;
      case (7):
         panel = pwizard_add_create_panel_6 (wizard);
	 break;
      default:
         return;
   }

   gtk_container_remove (GTK_CONTAINER(wizard->main_window), wizard->panel);
   wizard->panel = panel;
   gtk_container_add(GTK_CONTAINER(wizard->main_window), wizard->panel);
   wizard->panel_showing--;

}

GtkWidget *pwizard_add_create_panel_1 (pwizard_state *wizard) {

   GtkWidget *panel, *button, *hbox, *widget;

   panel = gtk_vbox_new (FALSE, 0);
   gtk_container_border_width (GTK_CONTAINER (panel), 10);
   gtk_widget_show (panel);

   widget = gtk_label_new (
      "This wizard will guide you through the\n"
      "installation of a printer."
   );
   gtk_label_set_justify (GTK_LABEL(widget), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start(GTK_BOX(panel), widget, TRUE, TRUE, 0);
   gtk_widget_show (widget);

   hbox = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(panel), hbox, FALSE, FALSE, 00);
   gtk_widget_show(hbox);

   button = gtk_button_new_with_label ("  Cancel  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_cancel), (gpointer)(wizard)); 
   gtk_widget_show(button);

   button = gtk_button_new_with_label ("  Next  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_next), (gpointer)(wizard)); 
   gtk_widget_show(button);

   return (panel);

}

GtkWidget *pwizard_add_create_panel_2 (pwizard_state *wizard) {

   GtkWidget *panel, *button, *hbox, *widget, *vbox, *event_box;
   GtkTooltips *tooltips;

   panel = gtk_vbox_new (FALSE, 0);
   gtk_container_border_width (GTK_CONTAINER (panel), 10);
   gtk_widget_show (panel);


   vbox = gtk_vbox_new(FALSE, 2);
   gtk_box_pack_start(GTK_BOX(panel), vbox, TRUE, TRUE, 00);
   gtk_container_border_width (GTK_CONTAINER (vbox), 30);
   gtk_widget_show(vbox);

   hbox = gtk_hbox_new(FALSE, 00);
   gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 00);
   gtk_widget_show(hbox);
   widget = gtk_label_new (
      "Items entered here will only affect how the printer\n"
      "is known by you."
   );
   gtk_label_set_justify (GTK_LABEL(widget), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
   gtk_widget_show (widget);



   event_box = gtk_event_box_new();
   gtk_box_pack_end(GTK_BOX(vbox), event_box, FALSE, FALSE, 0);
   gtk_widget_show(event_box);

   hbox = gtk_hbox_new(FALSE, 00);
   gtk_container_add (GTK_CONTAINER(event_box), hbox);
   gtk_widget_show (hbox);

   tooltips = gtk_tooltips_new();
   gtk_tooltips_set_tip (tooltips, event_box,
	 "The model field does not affect functionality; however, it "
	 "will be visible on the main xpdq screen.\n", NULL);

   widget = gtk_label_new("Model: ");
   gtk_box_pack_start(GTK_BOX(hbox),widget, FALSE, FALSE, 00);
   gtk_widget_show (widget);

   widget = gtk_entry_new();
   gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
   if (wizard->p->model != NULL) 
      gtk_entry_set_text(GTK_ENTRY(widget), wizard->p->model);
   gtk_signal_connect( GTK_OBJECT(widget), "changed",
	  GTK_SIGNAL_FUNC(pwizard_text_change), (gpointer)(&wizard->p->model)); 
   gtk_widget_show (widget);

   event_box = gtk_event_box_new();
   gtk_box_pack_end(GTK_BOX(vbox), event_box, FALSE, FALSE, 0);
   gtk_widget_show(event_box);

   hbox = gtk_hbox_new(FALSE, 00);
   gtk_container_add (GTK_CONTAINER(event_box), hbox);
   gtk_widget_show(hbox);

   tooltips = gtk_tooltips_new();
   gtk_tooltips_set_tip (tooltips, event_box,
	 "Perhaps the printer names you choose are more cute than meaningful; "
	 " then the location field will be a helpful reminder.\n", NULL);


   widget = gtk_label_new("Location: ");
   gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
   gtk_widget_show (widget);
   widget = gtk_entry_new();
   if (wizard->p->location != NULL) 
      gtk_entry_set_text(GTK_ENTRY(widget), wizard->p->location);
   gtk_signal_connect( GTK_OBJECT(widget), "changed",
	  GTK_SIGNAL_FUNC(pwizard_text_change), 
	  (gpointer)(&wizard->p->location)); 
   gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
   gtk_widget_show (widget);

   event_box = gtk_event_box_new();
   gtk_box_pack_end(GTK_BOX(vbox), event_box, FALSE, FALSE, 0);
   gtk_widget_show(event_box);

   hbox = gtk_hbox_new(FALSE, 00);
   gtk_container_add (GTK_CONTAINER(event_box), hbox);
   gtk_widget_show(hbox);

   tooltips = gtk_tooltips_new();
   gtk_tooltips_set_tip (tooltips, event_box,
	 "The name field will only affect how the printer is known to you.  "
	 "However, if you are running xpdq as the superuser, then changes will "
	 "affect all users of the system.\n", NULL); 

   widget = gtk_label_new("Printer name: ");
   gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
   gtk_widget_show (widget);

   widget = gtk_entry_new();
   if (wizard->p->name != NULL) 
      gtk_entry_set_text(GTK_ENTRY(widget), wizard->p->name);
   gtk_signal_connect( GTK_OBJECT(widget), "changed",
	  GTK_SIGNAL_FUNC(pwizard_text_change), (gpointer)(&wizard->p->name)); 
   gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
   gtk_widget_show (widget);

   hbox = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(panel), hbox, FALSE, FALSE, 00);
   gtk_widget_show(hbox);
   button = gtk_button_new_with_label ("  Cancel  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_cancel), (gpointer)(wizard)); 
   gtk_widget_show(button);
   button = gtk_button_new_with_label ("  Next  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_next), (gpointer)(wizard)); 
   gtk_widget_show(button);
   button = gtk_button_new_with_label ("  Prev  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_prev), (gpointer)(wizard)); 
   gtk_widget_show(button);

   return (panel);

}

GtkWidget *pwizard_add_create_panel_3 (pwizard_state *wizard) {

   GtkWidget *panel, *button, *hbox, *widget, *vbox, *scrolled;
   dl_list *list;
   driver *d;
   int i, row;
   char *dname;

   panel = gtk_vbox_new (FALSE, 0);
   gtk_container_border_width (GTK_CONTAINER (panel), 10);
   gtk_widget_show (panel);


   vbox = gtk_vbox_new(FALSE, 10);
   gtk_box_pack_start(GTK_BOX(panel), vbox, TRUE, TRUE, 00);
   gtk_container_border_width (GTK_CONTAINER (vbox), 30);
   gtk_widget_show(vbox);

   hbox = gtk_hbox_new(FALSE, 00);
   gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 00);
   gtk_widget_show(hbox);
   widget = gtk_label_new (
      "Select a print driver.  This will affect how documents\n"
      "get converted to a type understood by your printer."
   );
   gtk_label_set_justify (GTK_LABEL(widget), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
   gtk_widget_show (widget);

   hbox = gtk_hbox_new(FALSE, 00);
   gtk_box_pack_end(GTK_BOX(vbox), hbox, TRUE, TRUE, 00);
   gtk_widget_show(hbox);
   widget = gtk_clist_new (1);
   gtk_clist_column_titles_passive (GTK_CLIST(widget));
   gtk_clist_set_column_width (GTK_CLIST(widget), 0, 200);
   /*gtk_clist_set_policy (GTK_CLIST(widget), GTK_POLICY_AUTOMATIC,
      GTK_POLICY_AUTOMATIC);*/
   gtk_clist_set_selection_mode (GTK_CLIST(widget), GTK_SELECTION_BROWSE);
   list = first_list_element (rc->driver_list);
   if (list == NULL) {
      xpdq_error ("This system has no drivers.  Please make sure\n"
         "that there are drivers defined in /etc/printrc, ~/.printrc,\n"
	 "or in files included by /etc/printrc or ~/.printrc.");
   }
   while (list != NULL) {
      d = (driver *) list->data;
      check_driver_compatibility (d, rc); 
      row = gtk_clist_append (GTK_CLIST(widget), &d->name);
      if (d->compatibility == 2) {
	    gtk_clist_set_background (
		     GTK_CLIST (widget), row, error_color);
      }
      list = list->next;
   }
   if (wizard->p->driver != NULL) {
      for ( i = 0; 0 != gtk_clist_get_text(GTK_CLIST(widget), i, 0, &dname);
         i++) {
	 if ( strcmp (dname , wizard->p->driver) == 0 ) {
            gtk_clist_moveto (GTK_CLIST (widget), i, -1, 0.0, 0.0);
            gtk_clist_select_row (GTK_CLIST (widget), i, 0);
	    break;
	 }
      }
   }
   gtk_signal_connect( GTK_OBJECT(widget), "select_row",
	  GTK_SIGNAL_FUNC(pwizard_driver_selected), 
	  (gpointer)(wizard)); 
   if (wizard->p->driver == NULL)
      pwizard_driver_selected (widget, 0, 0, NULL, wizard);

   scrolled = gtk_scrolled_window_new (NULL, NULL);
   gtk_box_pack_start(GTK_BOX(hbox), scrolled, TRUE, TRUE, 0);
   gtk_container_add (GTK_CONTAINER (scrolled), widget);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_widget_show (scrolled);
   gtk_widget_show (widget);


   hbox = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(panel), hbox, FALSE, FALSE, 00);
   gtk_widget_show(hbox);
   button = gtk_button_new_with_label ("  Cancel  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_cancel), (gpointer)(wizard)); 
   gtk_widget_show(button);
   button = gtk_button_new_with_label ("  Next  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_next), (gpointer)(wizard)); 
   gtk_widget_show(button);
   button = gtk_button_new_with_label ("  Prev  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_prev), (gpointer)(wizard)); 
   gtk_widget_show(button);
   button = gtk_button_new_with_label ("  Info  ");
   gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_driver_info), (gpointer)(wizard)); 
   gtk_widget_show(button);

   return (panel);

}

GtkWidget *pwizard_add_create_panel_4 (pwizard_state *wizard) {

   GtkWidget *panel, *button, *hbox, *widget, *vbox, *scrolled, *safe_spot,
      *viewport, *event_box;
   GtkTooltips *tooltips;
   dl_list *l, *ll;
   driver *d;
   argument *a;
   item_pair *i;
   char *buf;

   l = find_in_list_by_string (rc->driver_list, wizard->p->driver);
   if (l  == NULL) {
      fprintf (stderr, "Error: can't find driver %s\n",
         wizard->p->interface);
      my_exit(1);
   }
   d = (driver *)l->data;

   panel = gtk_vbox_new (FALSE, 0);
   gtk_container_border_width (GTK_CONTAINER (panel), 10);
   gtk_widget_show (panel);

   vbox = gtk_vbox_new(FALSE, 2);
   gtk_box_pack_start(GTK_BOX(panel), vbox, TRUE, TRUE, 00);
   gtk_container_border_width (GTK_CONTAINER (vbox), 30);
   gtk_widget_show(vbox);

   hbox = gtk_hbox_new(FALSE, 00);
   gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 00);
   gtk_widget_show(hbox);
   widget = gtk_label_new (
      "The following parameters are for the selected driver:"
   );
   gtk_label_set_justify (GTK_LABEL(widget), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
   gtk_widget_show (widget);

   scrolled = gtk_scrolled_window_new (NULL, NULL);
   gtk_box_pack_end(GTK_BOX(vbox), scrolled, TRUE, TRUE, 00);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolled), 
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_widget_show (scrolled);

   safe_spot = gtk_vbox_new (FALSE, 2);
   viewport = gtk_viewport_new (NULL, NULL);
   gtk_viewport_set_shadow_type (GTK_VIEWPORT(viewport), GTK_SHADOW_NONE);
   gtk_widget_show (viewport);
   gtk_container_add( GTK_CONTAINER(viewport), safe_spot);
   gtk_container_add( GTK_CONTAINER(scrolled), viewport);

   gtk_widget_show (safe_spot);

   
   l = last_list_element (d->arg_list);
   while (l != NULL) {

      a = (argument *)l->data;

      ll = find_in_list_by_string(wizard->p->driver_arg_list, a->var); 
      if (ll != NULL) {
         i = (item_pair *)ll->data;
      } else {
         compress_whitespace (&a->help);
	 i = new_item_pair();
	 str_reset (&i->key, a->var);
	 str_reset (&i->value, a->def_value);
	 wizard->p->driver_arg_list = 
	    append_to_list (wizard->p->driver_arg_list, i);
      }

      event_box = gtk_event_box_new();
      gtk_box_pack_end(GTK_BOX(safe_spot), event_box, FALSE, FALSE, 0);
      gtk_widget_show(event_box);

      if (a->help != NULL) { 
	 tooltips = gtk_tooltips_new();
	 gtk_tooltips_set_tip (tooltips, event_box, a->help, NULL);
      }

      hbox = gtk_hbox_new(FALSE, 00);
      gtk_container_add (GTK_CONTAINER(event_box), hbox);
      gtk_container_border_width (GTK_CONTAINER (safe_spot), 5);
      gtk_widget_show(hbox);
      if (a->desc != NULL) {
	 buf = malloc (strlen (a->desc) + 3);
	 if (buf == NULL) {
	    fprintf (stderr, "Not enough memory.\n");
	    my_exit (1);
	 }
	 sprintf (buf, "%s: ", a->desc);
      } else {
	 buf = malloc (strlen (a->var) + 3);
	 if (buf == NULL) {
	    fprintf (stderr, "Not enough memory.\n");
	    my_exit (1);
	 }
	 sprintf (buf, "%s: ", a->var);
      }
      widget = gtk_label_new(buf);
      free (buf);
      gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
      gtk_widget_show (widget);
      widget = gtk_entry_new();
      if (i->value != NULL) 
	 gtk_entry_set_text(GTK_ENTRY(widget), i->value);
      gtk_signal_connect( GTK_OBJECT(widget), "changed",
	     GTK_SIGNAL_FUNC(pwizard_text_change), 
	     (gpointer)(&i->value)); 
      gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
      gtk_widget_show (widget);

      l = l->prev;

   }

   hbox = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(panel), hbox, FALSE, FALSE, 00);
   gtk_widget_show(hbox);
   button = gtk_button_new_with_label ("  Cancel  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_cancel), (gpointer)(wizard)); 
   gtk_widget_show(button);
   button = gtk_button_new_with_label ("  Next  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_next), (gpointer)(wizard)); 
   gtk_widget_show(button);
   button = gtk_button_new_with_label ("  Prev  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_prev), (gpointer)(wizard)); 
   gtk_widget_show(button);

   return (panel);

}


GtkWidget *pwizard_add_create_panel_5 (pwizard_state *wizard) {

   GtkWidget *panel, *button, *hbox, *widget, *vbox, *scrolled;
   dl_list *list;
   interface *d;
   int i;
   char *dname;

   panel = gtk_vbox_new (FALSE, 0);
   gtk_container_border_width (GTK_CONTAINER (panel), 10);
   gtk_widget_show (panel);

   vbox = gtk_vbox_new(FALSE, 10);
   gtk_box_pack_start(GTK_BOX(panel), vbox, TRUE, TRUE, 00);
   gtk_container_border_width (GTK_CONTAINER (vbox), 30);
   gtk_widget_show(vbox);

   hbox = gtk_hbox_new(FALSE, 00);
   gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 00);
   gtk_widget_show(hbox);
   widget = gtk_label_new (
      "Select an interface.  This reflects how your\n"
      "machine will communicate with the printer."
   );
   gtk_label_set_justify (GTK_LABEL(widget), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
   gtk_widget_show (widget);

   hbox = gtk_hbox_new(FALSE, 00);
   gtk_box_pack_end(GTK_BOX(vbox), hbox, TRUE, TRUE, 00);
   gtk_widget_show(hbox);
   widget = gtk_clist_new (1);
   gtk_clist_column_titles_passive (GTK_CLIST(widget));
   gtk_clist_set_column_width (GTK_CLIST(widget), 0, 200);
   /*gtk_clist_set_policy (GTK_CLIST(widget), GTK_POLICY_AUTOMATIC,
      GTK_POLICY_AUTOMATIC);*/
   gtk_clist_set_selection_mode (GTK_CLIST(widget), GTK_SELECTION_BROWSE);
   list = first_list_element (rc->interface_list);
   while (list != NULL) {
      d = (interface *) list->data;
      gtk_clist_append (GTK_CLIST(widget), &d->name);
      list = list->next;
   }
   if (wizard->p->interface != NULL) {
      for ( i = 0; 0 != gtk_clist_get_text(GTK_CLIST(widget), i, 0, &dname);
         i++) {
	 if ( strcmp (dname , wizard->p->interface) == 0 ) {
            gtk_clist_moveto (GTK_CLIST (widget), i, -1, 0.0, 0.0);
            gtk_clist_select_row (GTK_CLIST (widget), i, 0);
	    break;
	 }
      }
   }
   gtk_signal_connect( GTK_OBJECT(widget), "select_row",
	  GTK_SIGNAL_FUNC(pwizard_interface_selected), 
	  (gpointer)(wizard)); 
   if (wizard->p->interface == NULL)
      pwizard_interface_selected (widget, 0, 0, NULL, wizard);
   scrolled = gtk_scrolled_window_new (NULL, NULL);
   gtk_box_pack_start(GTK_BOX(hbox), scrolled, TRUE, TRUE, 0);
   gtk_container_add (GTK_CONTAINER (scrolled), widget);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_widget_show (scrolled);
   gtk_widget_show (widget);
   gtk_widget_show (widget);


   hbox = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(panel), hbox, FALSE, FALSE, 00);
   gtk_widget_show(hbox);
   button = gtk_button_new_with_label ("  Cancel  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_cancel), (gpointer)(wizard)); 
   gtk_widget_show(button);
   button = gtk_button_new_with_label ("  Next  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_next), (gpointer)(wizard)); 
   gtk_widget_show(button);
   button = gtk_button_new_with_label ("  Prev  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_prev), (gpointer)(wizard)); 
   gtk_widget_show(button);
   button = gtk_button_new_with_label ("  Info  ");
   gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_interface_info), (gpointer)(wizard)); 
   gtk_widget_show(button);

   return (panel);

}

GtkWidget *pwizard_add_create_panel_6 (pwizard_state *wizard) {

   GtkWidget *panel, *button, *hbox, *widget, *vbox, *scrolled, *safe_spot,
      *viewport, *event_box;
   GtkTooltips *tooltips;
   dl_list *l, *ll;
   interface *d;
   argument *a;
   item_pair *i;
   char *buf;

   l = find_in_list_by_string (rc->interface_list, wizard->p->interface);
   if (l  == NULL) {
      fprintf (stderr, "Error: can't find interface %s\n",
         wizard->p->interface);
      my_exit(1);
   }
   d = (interface *)l->data;

   panel = gtk_vbox_new (FALSE, 0);
   gtk_container_border_width (GTK_CONTAINER (panel), 10);
   gtk_widget_show (panel);

   vbox = gtk_vbox_new(FALSE, 2);
   gtk_box_pack_start(GTK_BOX(panel), vbox, TRUE, TRUE, 00);
   gtk_container_border_width (GTK_CONTAINER (vbox), 30);
   gtk_widget_show(vbox);

   hbox = gtk_hbox_new(FALSE, 00);
   gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 00);
   gtk_widget_show(hbox);
   widget = gtk_label_new (
      "The following parameters are for the selected interface:"
   );
   gtk_label_set_justify (GTK_LABEL(widget), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
   gtk_widget_show (widget);

   scrolled = gtk_scrolled_window_new (NULL, NULL);
   gtk_box_pack_end(GTK_BOX(vbox), scrolled, TRUE, TRUE, 00);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolled), 
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_widget_show (scrolled);

   safe_spot = gtk_vbox_new (FALSE, 2);
   viewport = gtk_viewport_new (NULL, NULL);
   gtk_viewport_set_shadow_type (GTK_VIEWPORT(viewport), GTK_SHADOW_NONE);
   gtk_widget_show (viewport);
   gtk_container_add( GTK_CONTAINER(viewport), safe_spot);
   gtk_container_add( GTK_CONTAINER(scrolled), viewport);

   gtk_widget_show (safe_spot);

   l = last_list_element (d->arg_list);
   while (l != NULL) {

      a = (argument *)l->data;

      ll = find_in_list_by_string(wizard->p->interface_arg_list, a->var); 
      if (ll != NULL) {
         i = (item_pair *)ll->data;
      } else {
         compress_whitespace (&a->help);
	 i = new_item_pair();
	 str_reset (&i->key, a->var);
	 str_reset (&i->value, a->def_value);
	 wizard->p->interface_arg_list = 
	    append_to_list (wizard->p->interface_arg_list, i);
      }

      event_box = gtk_event_box_new();
      gtk_box_pack_end(GTK_BOX(safe_spot), event_box, FALSE, FALSE, 0);
      gtk_widget_show(event_box);

      if (a->help != NULL) { 
	 tooltips = gtk_tooltips_new();
	 gtk_tooltips_set_tip (tooltips, event_box, a->help, NULL);
      }

      hbox = gtk_hbox_new(FALSE, 00);
      gtk_container_add (GTK_CONTAINER(event_box), hbox);
      gtk_container_border_width (GTK_CONTAINER (safe_spot), 5);
      gtk_widget_show(hbox);
      if (a->desc != NULL) {
	 buf = malloc (strlen (a->desc) + 3);
	 if (buf == NULL) {
	    fprintf (stderr, "Not enough memory.\n");
	    my_exit (1);
	 }
	 sprintf (buf, "%s: ", a->desc);
      } else {
	 buf = malloc (strlen (a->var) + 3);
	 if (buf == NULL) {
	    fprintf (stderr, "Not enough memory.\n");
	    my_exit (1);
	 }
	 sprintf (buf, "%s: ", a->var);
      }
      widget = gtk_label_new(buf);
      free (buf);
      gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
      gtk_widget_show (widget);
      widget = gtk_entry_new();
      if (i->value != NULL) 
	 gtk_entry_set_text(GTK_ENTRY(widget), i->value);
      gtk_signal_connect( GTK_OBJECT(widget), "changed",
	     GTK_SIGNAL_FUNC(pwizard_text_change), 
	     (gpointer)(&i->value)); 
      gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
      gtk_widget_show (widget);

      l = l->prev;

   }

   hbox = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(panel), hbox, FALSE, FALSE, 00);
   gtk_widget_show(hbox);
   button = gtk_button_new_with_label ("  Cancel  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_cancel), (gpointer)(wizard)); 
   gtk_widget_show(button);
   button = gtk_button_new_with_label ("  Next  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_next), (gpointer)(wizard)); 
   gtk_widget_show(button);
   button = gtk_button_new_with_label ("  Prev  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_prev), (gpointer)(wizard)); 
   gtk_widget_show(button);

   return (panel);

}

GtkWidget *pwizard_add_create_panel_7 (pwizard_state *wizard) {

   GtkWidget *panel, *button, *hbox, *widget;

   panel = gtk_vbox_new (FALSE, 0);
   gtk_container_border_width (GTK_CONTAINER (panel), 10);
   gtk_widget_show (panel);

   widget = gtk_label_new (
      "Note that other options for your printer can be configured\n"
      "by clicking the \"driver options\" or \"interface options\"\n"
      "buttons on the xpdq main screen.\n\n"
      "Press \"Finish\" to add the printer.\n"
      "Press \"Cancel\" to abort."
   );
   gtk_label_set_justify (GTK_LABEL(widget), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start(GTK_BOX(panel), widget, TRUE, TRUE, 0);
   gtk_widget_show (widget);

   hbox = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(panel), hbox, FALSE, FALSE, 00);
   gtk_widget_show(hbox);

   button = gtk_button_new_with_label ("  Cancel  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_cancel), (gpointer)(wizard)); 
   gtk_widget_show(button);

   button = gtk_button_new_with_label ("  Finish  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_next), (gpointer)(wizard)); 
   gtk_widget_show(button);

   button = gtk_button_new_with_label ("  Prev  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(pwizard_prev), (gpointer)(wizard)); 
   gtk_widget_show(button);

   return (panel);

}


void pwizard_text_change (GtkWidget *widget, char **text) {

   char *txt;
   txt = gtk_entry_get_text ( GTK_ENTRY(widget) );
   str_reset (text, txt);

}

void pwizard_driver_selected ( GtkWidget *widget, gint row, 
   gint column, GdkEventButton *event, pwizard_state *wizard ) {

   char *txt;
   gtk_clist_get_text(GTK_CLIST(widget), row, 0, &txt);
   str_reset (&wizard->p->driver, txt);

   if (wizard->p->driver_arg_list != NULL) {
      list_iterate (wizard->p->driver_arg_list,
             (void *)(void *)free_item_pair);
      wizard->p->driver_arg_list = free_list (wizard->p->driver_arg_list);
   }

   if (details_window != NULL) {
      pwizard_driver_info (wizard);
   }

   return;
}

void pwizard_interface_selected ( GtkWidget *widget, gint row, 
   gint column, GdkEventButton *event, pwizard_state *wizard ) {

   char *txt;
   gtk_clist_get_text(GTK_CLIST(widget), row, 0, &txt);
   str_reset (&wizard->p->interface, txt);

   if (wizard->p->interface_arg_list != NULL) {
      list_iterate (wizard->p->interface_arg_list,
             (void *)(void *)free_item_pair);
      wizard->p->interface_arg_list = free_list
         (wizard->p->interface_arg_list);
   }

   if (details_window != NULL) {
      pwizard_interface_info (wizard);
   }

   return;
}

int pwizard_check_name (pwizard_state *wizard) {

   dl_list *l;
   printer *p;
   char *buf;

   if (wizard->p->name == NULL) {
      xpdq_error ("A printer name must be specified.");
      return (1);
   }
   if (*wizard->p->name == 0) {
      xpdq_error ("A printer name must be specified.");
      return (1);
   }
   l = first_list_element (rc->printer_list);
   while (l != NULL) {
      p = (printer *) l->data;
      if ( strcmp (p->name, wizard->p->name) == 0 ) {
         buf = malloc (strlen (p->name) + 100);
	 if (buf == NULL) {
	    fprintf (stderr, "Not enough memory.\n");
	    my_exit (1);
	 }
	 sprintf (buf, "There is already a printer named\n%s.",
	 	p->name);
         xpdq_error (buf);
	 free (buf);
         return (1);
      }
      l = l->next;
   }
   return (0);
}

int pwizard_check_driver (pwizard_state *wizard) {

   dl_list *l, *l2;
   driver *d;
   item_pair *i;
   char *arg, *buf;
   int have_arg;

   l = find_in_list_by_string (rc->driver_list, wizard->p->driver);
   if (l  == NULL) {
      fprintf (stderr, "Error: can't find driver %s\n",
         wizard->p->driver);
      my_exit(1);
   }
   d = (driver *)l->data;


   l = first_list_element (d->required_arg_list);
   while (l != NULL) {
      arg = (char *)l->data;
      have_arg = 0;
      l2 = first_list_element (wizard->p->driver_arg_list);
      while (l2 != NULL) {
         i = (item_pair *)l2->data;
	 if (strcmp (i->key, arg) == 0) {
	    if (i->value != NULL) {
	       if (*i->value != 0) {
	          have_arg = 1;
	          break;
	       }
	    }
	 }
         l2 = l2->next;
      }
      if (have_arg == 0) {
         buf = malloc (strlen(arg) + 50);
	 sprintf (buf, "%s is a required argument.\n", arg);
	 xpdq_error (buf);
	 free (buf);
	 return (1);
      }
      l = l->next;
   }

   return (0);
}

int pwizard_check_interface (pwizard_state *wizard) {

   dl_list *l, *l2;
   interface *d;
   item_pair *i;
   char *arg, *buf;
   int have_arg;

   l = find_in_list_by_string (rc->interface_list, wizard->p->interface);
   if (l  == NULL) {
      fprintf (stderr, "Error: can't find interface %s\n",
         wizard->p->interface);
      my_exit(1);
   }
   d = (interface *)l->data;


   l = first_list_element (d->required_arg_list);
   while (l != NULL) {
      arg = (char *)l->data;
      have_arg = 0;
      l2 = first_list_element (wizard->p->interface_arg_list);
      while (l2 != NULL) {
         i = (item_pair *)l2->data;
	 if (strcmp (i->key, arg) == 0) {
	    if (i->value != NULL) {
	       if (*i->value != 0) {
	          have_arg = 1;
	          break;
	       }
	    }
	 }
         l2 = l2->next;
      }
      if (have_arg == 0) {
         buf = malloc (strlen(arg) + 50);
	 sprintf (buf, "%s is a required argument.\n", arg);
	 xpdq_error (buf);
	 free (buf);
	 return (1);
      }
      l = l->next;
   }

   return (0);
   return (0);
}


void pwizard_interface_info (pwizard_state *wizard) {

   dl_list *l;
   interface *d;

   l = find_in_list_by_string (rc->interface_list, wizard->p->interface);
   if (l == NULL) {
      fprintf (stderr, "Error: selected interface not found.\n");
      my_exit (1);
   }
   d = (interface *)l->data;

   show_details_window ();
   gtk_text_freeze ( GTK_TEXT(details_widget) );
   erase_old_details ();
   add_red_details (d->incompatibility_msg);
   add_details ("\n");
   if (d->help == NULL) {
      add_details ("No description of this interface is available.\n");
   } else {
      compress_whitespace(&d->help);
      add_details (d->help);
   }
   gtk_text_thaw ( GTK_TEXT(details_widget) );
}

void pwizard_driver_info (pwizard_state *wizard) {

   dl_list *l;
   driver *d;
   char *buf;

   l = find_in_list_by_string (rc->driver_list, wizard->p->driver);
   if (l == NULL) {
      fprintf (stderr, "Error: selected driver not found.\n");
      my_exit (1);
   }
   d = (driver *)l->data;

   show_details_window ();
   gtk_text_freeze ( GTK_TEXT(details_widget) );
   erase_old_details ();
   add_red_details (d->incompatibility_msg);
   add_details ("\n");
   if (d->help == NULL) {
      add_details ("No description of this driver is available.\n");
   } else {
      compress_whitespace(&d->help);
      add_details (d->help);
   }
   gtk_text_thaw ( GTK_TEXT(details_widget) );

}
