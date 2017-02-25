/* Copyright 1999, 2000 Jacob A. Langford
 *
 * xpdq_option.c
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

#include <gtk/gtk.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "rc_items.h"
#include "parse_rc.h"
#include "list.h"
#include "job.h"
#include "util.h"
#include "printer.h"
#include "xpdq.h"
#include "reaper.h"
#include "argument.h"
#include "option.h"

option_state *new_option_state (void) {
   option_state *s;
   s = malloc (sizeof (option_state) );
   if (s == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   s->argument_list = NULL;
   s->option_list = NULL;
   s->option_widget_pairs = NULL;
   return (s);
}

void free_option_state (option_state *s) {

   list_iterate (s->argument_list, (void *)(void *)free_argument);
   list_iterate (s->option_list, (void *)(void *)free_option);
   list_iterate (s->option_widget_pairs, free);
   s->argument_list = free_list (s->argument_list);
   s->option_list = free_list (s->option_list);
   s->option_widget_pairs = free_list (s->option_widget_pairs);

}

void twiddle_driver_options (void) {

   GtkWidget *button, *hbox, *widget, *vbox, *scrolled, *safe_spot,
      *viewport, *event_box, *frame;
   GtkTooltips *tooltips;
   driver *d;
   printer *p;
   argument *a;
   choice *ch;
   option *o;
   pointer_pair *pp;
   dl_list *l, *cl;
   char *buf;
   int row, est_width;

   if (GTK_CLIST(printer_list_widget)->selection == NULL) return;
   row = (int) GTK_CLIST(printer_list_widget)->selection->data;
   p = gtk_clist_get_row_data (GTK_CLIST (printer_list_widget), row);
   if (p == NULL) {
      fprintf (stderr, "Should have found a printer for this row!\n");
      return;
   }
   l = find_in_list_by_string (rc->driver_list, p->driver);
   if (l == NULL) {
      fprintf (stderr, "Driver %s is not defined in any of the rc files!\n",
	 p->driver);
      return;
   }
   d = (driver *)l->data;

   if (driver_option_state == NULL) {
      driver_option_state = new_option_state ();
      driver_option_state->argument_list = copy_argument_list (d->arg_list);
      driver_option_state->option_list = copy_option_list (d->option_list);
      option_set_arg_defaults (driver_option_state->argument_list, 
      		p->driver_arg_list);
      option_set_opt_defaults (driver_option_state->option_list,
      		p->driver_option_list,
	   d->default_option_list);
   }

   if (driver_option_window == NULL) {

      driver_option_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_window_set_title(GTK_WINDOW (driver_option_window), "Xpdq driver options");
      gtk_signal_connect_object (GTK_OBJECT (driver_option_window), "delete_event",
			 GTK_SIGNAL_FUNC(twiddle_driver_options_cancel), NULL);

      gtk_window_set_default_size(GTK_WINDOW(driver_option_window), 450, 400);
      gtk_widget_show (driver_option_window);
   } else {
      gtk_container_remove (GTK_CONTAINER(driver_option_window), 
      		driver_option_panel);
   }

   driver_option_panel = gtk_vbox_new (FALSE, 0);
   gtk_container_border_width (GTK_CONTAINER (driver_option_panel), 10);
   gtk_container_add (GTK_CONTAINER (driver_option_window), driver_option_panel);
   gtk_widget_show (driver_option_panel);


   vbox = gtk_vbox_new(FALSE, 10);
   gtk_box_pack_start(GTK_BOX(driver_option_panel), vbox, TRUE, TRUE, 00);
   gtk_container_border_width (GTK_CONTAINER (vbox), 30);
   gtk_widget_show(vbox);

   hbox = gtk_hbox_new(FALSE, 00);
   gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 00);
   gtk_widget_show(hbox);
   buf = malloc (strlen(d->name) + strlen(p->name) + 50);
   sprintf (buf, "Choose options for %s:\n(driver %s)", 
   	p->name, d->name);;
   widget = gtk_label_new (buf);
   free (buf);
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
   gtk_container_border_width (GTK_CONTAINER (safe_spot), 5);
   gtk_widget_show (safe_spot);

   
   l = last_list_element (driver_option_state->argument_list);
   while (l != NULL) {

      a = (argument *)l->data;

      compress_whitespace (&a->help);

      event_box = gtk_event_box_new();
      gtk_box_pack_end(GTK_BOX(safe_spot), event_box, FALSE, FALSE, 0);
      gtk_widget_show(event_box);

      tooltips = gtk_tooltips_new();
      buf = malloc (strlen(a->var) + 
      	(a->help == NULL ? 0 : strlen(a->help)) + 30);
      if (buf == NULL) {
	 fprintf (stderr, "Not enough memory.\n");
	 my_exit (1);
      }
      if (a->help != NULL) { 
	 sprintf (buf, "%s\n(can be set with -a%s=)", a->help, a->var);
      } else {
	 sprintf (buf, "(can be set with -a%s=)", a->var);
      }
      gtk_tooltips_set_tip (tooltips, event_box, buf, NULL);
      free (buf);

      hbox = gtk_hbox_new(FALSE, 00);
      gtk_container_add (GTK_CONTAINER(event_box), hbox);
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
      if (a->def_value != NULL) 
	 gtk_entry_set_text(GTK_ENTRY(widget), a->def_value);
      gtk_signal_connect( GTK_OBJECT(widget), "changed",
	     GTK_SIGNAL_FUNC(pwizard_text_change), 
	     (gpointer)(&a->def_value)); 
      gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
      gtk_widget_show (widget);

      l = l->prev;

   }

   l = last_list_element (driver_option_state->option_list);
   while (l != NULL) {

      o = (option *)l->data;

      frame = gtk_frame_new (o->desc);
      gtk_box_pack_end(GTK_BOX(safe_spot), frame, FALSE, FALSE, 0);
      gtk_container_border_width (GTK_CONTAINER (frame), 5);
      gtk_widget_show (frame);

      est_width = 0;
      cl = first_list_element (o->choice_list);
      while (cl != NULL) {
         est_width += 4;
         est_width += strlen( ((choice *)cl->data)->desc );
         cl = cl->next;
      }
      if (est_width < 55) {
         hbox = gtk_hbox_new (FALSE, 00);              /* Don't ask... */
      } else {
         hbox = gtk_vbox_new (FALSE, 00);              /* Don't ask... */
      }
      gtk_container_add (GTK_CONTAINER(frame), hbox);
      gtk_container_border_width (GTK_CONTAINER (hbox), 10);
      gtk_widget_show (hbox);

      cl = first_list_element (o->choice_list);
      ch = (choice *)cl->data;
      compress_whitespace (&ch->help);
      widget = gtk_radio_button_new_with_label (NULL, ch->desc);
      gtk_box_pack_start (GTK_BOX(hbox), widget, FALSE, TRUE, 0);
      gtk_widget_show (widget);
      tooltips = gtk_tooltips_new();
      buf = malloc (strlen(ch->name) + 
      	(ch->help == NULL ? 0 : strlen(ch->help)) + 30);
      if (buf == NULL) {
	 fprintf (stderr, "Not enough memory.\n");
	 my_exit (1);
      }
      if (ch->help != NULL) {
         sprintf (buf, "%s\n(can be set with -o%s)", ch->help, ch->name);
      } else {
         sprintf (buf, "(can be set with -o%s)", ch->name);
      }
      gtk_tooltips_set_tip (tooltips, widget, buf, NULL);
      free (buf);
      pp = new_pointer_pair();
      pp->a = o;
      pp->b = ch;
      driver_option_state->option_widget_pairs = 
      		append_to_list (driver_option_state->option_widget_pairs, pp);
      gtk_signal_connect( GTK_OBJECT(widget), "toggled",
	     GTK_SIGNAL_FUNC(set_option_clicked), (gpointer) pp);
      cl = cl->next;
      while (cl != NULL) {
	 ch = (choice *)cl->data;
         compress_whitespace (&ch->help);
	 widget = gtk_radio_button_new_with_label(
	                  gtk_radio_button_group (GTK_RADIO_BUTTON
			  (widget)), ch->desc);
         gtk_box_pack_start (GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	 gtk_widget_show (widget);
	 tooltips = gtk_tooltips_new();
	 buf = malloc (strlen(ch->name) + 
	 	(ch->help == NULL ? 0 : strlen(ch->help)) + 30);
	 if (buf == NULL) {
	    fprintf (stderr, "Not enough memory.\n");
	    my_exit (1);
	 }
	 if (ch->help != NULL) {
	    sprintf (buf, "%s\n(can be set with -o%s)", ch->help, ch->name);
	 } else {
	    sprintf (buf, "(can be set with -o%s)", ch->name);
	 } 
	 gtk_tooltips_set_tip (tooltips, widget, buf, NULL);
	 free (buf);
	 if (0 == strcmp (ch->name, o->default_choice)) 
	    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
	 pp = new_pointer_pair();
	 pp->a = o;
	 pp->b = ch;
	 driver_option_state->option_widget_pairs = 
	     append_to_list (driver_option_state->option_widget_pairs, pp);
	 gtk_signal_connect( GTK_OBJECT(widget), "toggled",
		GTK_SIGNAL_FUNC(set_option_clicked), (gpointer) pp);
         cl = cl->next;
      }

      l = l->prev;

   }

   hbox = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(driver_option_panel), hbox, FALSE, FALSE, 00);
   gtk_widget_show(hbox);
   button = gtk_button_new_with_label ("  Okay  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 5);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(twiddle_driver_options_okay), NULL);
   gtk_widget_show(button);
   button = gtk_button_new_with_label ("  Cancel  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 5);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(twiddle_driver_options_cancel), NULL);
   gtk_widget_show(button);
   button = gtk_button_new_with_label ("  Set as default  ");
   gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(set_driver_options_as_default), NULL);
   gtk_widget_show(button);

}

void set_option_clicked (GtkWidget *radio_button, pointer_pair *pp) {

   choice *ch;
   option *o;

   /* I had to use the toggled signal rather than the clicked signal,
    * because clicked signals get emitted when I remove the panel, which
    * screws things up.
    */

   if ( gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(radio_button))
   	!= TRUE ) return;

   o = (option *)pp->a;
   ch = (choice *)pp->b;

   str_reset (&o->default_choice, ch->name);

}

void set_driver_options_as_default (void) {

   int row;
   printer *p;

   if (GTK_CLIST(printer_list_widget)->selection == NULL) return;
   row = (int) GTK_CLIST(printer_list_widget)->selection->data;
   p = gtk_clist_get_row_data (GTK_CLIST (printer_list_widget), row);
   if (p == NULL) {
      fprintf (stderr, "Should have found a printer for this row!\n");
      return;
   }

   list_iterate (p->driver_arg_list, (void *)(void *)free_item_pair);
   free_list (p->driver_arg_list);
   p->driver_arg_list = ip_list_from_arg_list (
   	driver_option_state->argument_list);

   list_iterate (p->driver_option_list, free);
   free_list (p->driver_option_list);
   p->driver_option_list = i_list_from_opt_list (
   	driver_option_state->option_list);

   set_printer (p);

   twiddle_driver_options_cancel ();

}

void set_interface_options_as_default (void) {

   int row;
   printer *p;

   if (GTK_CLIST(printer_list_widget)->selection == NULL) return;
   row = (int) GTK_CLIST(printer_list_widget)->selection->data;
   p = gtk_clist_get_row_data (GTK_CLIST (printer_list_widget), row);
   if (p == NULL) {
      fprintf (stderr, "Should have found a printer for this row!\n");
      return;
   }

   list_iterate (p->interface_arg_list, (void *)(void *)free_item_pair);
   free_list (p->interface_arg_list);
   p->interface_arg_list = ip_list_from_arg_list (
   	interface_option_state->argument_list);

   list_iterate (p->interface_option_list, free);
   free_list (p->interface_option_list);
   p->interface_option_list = i_list_from_opt_list (
   	interface_option_state->option_list);

   set_printer (p);

   twiddle_interface_options_cancel ();

}

void twiddle_interface_options_cancel (void) {
   if (interface_option_window != NULL) 
   	gtk_widget_destroy (interface_option_window);
   interface_option_window = NULL;
   free_option_state (interface_option_state);
   interface_option_state = NULL;
}

void twiddle_driver_options_cancel (void) {
   if (driver_option_window != NULL) 
   	gtk_widget_destroy (driver_option_window);
   driver_option_window = NULL;
   free_option_state (driver_option_state);
   driver_option_state = NULL;
}

void twiddle_interface_options_okay (void) {

   int row;
   printer *p;

   if (GTK_CLIST(printer_list_widget)->selection == NULL) return;
   row = (int) GTK_CLIST(printer_list_widget)->selection->data;
   p = gtk_clist_get_row_data (GTK_CLIST (printer_list_widget), row);
   if (p == NULL) {
      fprintf (stderr, "Should have found a printer for this row!\n");
      return;
   }

   list_iterate (p->interface_arg_list, (void *)(void *)free_item_pair);
   free_list (p->interface_arg_list);
   p->interface_arg_list = ip_list_from_arg_list (
   	interface_option_state->argument_list);

   list_iterate (p->interface_option_list, free);
   free_list (p->interface_option_list);
   p->interface_option_list = i_list_from_opt_list (
   	interface_option_state->option_list);

   twiddle_interface_options_cancel ();
}

void twiddle_driver_options_okay (void) {

   int row;
   printer *p;

   if (GTK_CLIST(printer_list_widget)->selection == NULL) return;
   row = (int) GTK_CLIST(printer_list_widget)->selection->data;
   p = gtk_clist_get_row_data (GTK_CLIST (printer_list_widget), row);
   if (p == NULL) {
      fprintf (stderr, "Should have found a printer for this row!\n");
      return;
   }

   list_iterate (p->driver_arg_list, (void *)(void *)free_item_pair);
   free_list (p->driver_arg_list);
   p->driver_arg_list = ip_list_from_arg_list (
   	driver_option_state->argument_list);

   list_iterate (p->driver_option_list, free);
   free_list (p->driver_option_list);
   p->driver_option_list = i_list_from_opt_list (
   	driver_option_state->option_list);

   twiddle_driver_options_cancel ();
}

void twiddle_interface_options (void) {

   GtkWidget *button, *hbox, *widget, *vbox, *scrolled, *safe_spot,
      *viewport, *event_box, *frame;
   GtkTooltips *tooltips;
   interface *d;
   printer *p;
   argument *a;
   choice *ch;
   option *o;
   pointer_pair *pp;
   dl_list *l, *cl;
   char *buf;
   int row, est_width;

   if (GTK_CLIST(printer_list_widget)->selection == NULL) return;
   row = (int) GTK_CLIST(printer_list_widget)->selection->data;
   p = gtk_clist_get_row_data (GTK_CLIST (printer_list_widget), row);
   if (p == NULL) {
      fprintf (stderr, "Should have found a printer for this row!\n");
      return;
   }
   l = find_in_list_by_string (rc->interface_list, p->interface);
   if (l == NULL) {
      fprintf (stderr, "Interface %s is not defined in any of the rc files!\n",
	 p->interface);
      return;
   }
   d = (interface *)l->data;

   if (interface_option_state == NULL) {
      interface_option_state = new_option_state ();
      interface_option_state->argument_list = copy_argument_list (d->arg_list);
      interface_option_state->option_list = copy_option_list (d->option_list);
      option_set_arg_defaults (interface_option_state->argument_list, 
      		p->interface_arg_list);
      option_set_opt_defaults (interface_option_state->option_list,
      		p->interface_option_list,
	   d->default_option_list);
   }

   if (interface_option_window == NULL) {

      interface_option_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_window_set_title(GTK_WINDOW (interface_option_window), "Xpdq interface options");
      gtk_signal_connect_object (GTK_OBJECT (interface_option_window), "delete_event",
			 GTK_SIGNAL_FUNC(twiddle_interface_options_cancel), NULL);

      gtk_window_set_default_size(GTK_WINDOW(interface_option_window), 450, 400);
      gtk_widget_show (interface_option_window);
   } else {
      gtk_container_remove (GTK_CONTAINER(interface_option_window), 
      		interface_option_panel);
   }

   interface_option_panel = gtk_vbox_new (FALSE, 0);
   gtk_container_border_width (GTK_CONTAINER (interface_option_panel), 10);
   gtk_container_add (GTK_CONTAINER (interface_option_window), interface_option_panel);
   gtk_widget_show (interface_option_panel);


   vbox = gtk_vbox_new(FALSE, 10);
   gtk_box_pack_start(GTK_BOX(interface_option_panel), vbox, TRUE, TRUE, 00);
   gtk_container_border_width (GTK_CONTAINER (vbox), 30);
   gtk_widget_show(vbox);

   hbox = gtk_hbox_new(FALSE, 00);
   gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 00);
   gtk_widget_show(hbox);
   buf = malloc (strlen(d->name) + strlen(p->name) + 50);
   sprintf (buf, "Choose options for %s:\n(interface %s)", 
   	p->name, d->name);;
   widget = gtk_label_new (buf);
   free (buf);
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
   gtk_container_border_width (GTK_CONTAINER (safe_spot), 5);
   gtk_widget_show (safe_spot);

   
   l = last_list_element (interface_option_state->argument_list);
   while (l != NULL) {

      a = (argument *)l->data;

      compress_whitespace (&a->help);

      event_box = gtk_event_box_new();
      gtk_box_pack_end(GTK_BOX(safe_spot), event_box, FALSE, FALSE, 0);
      gtk_widget_show(event_box);

      tooltips = gtk_tooltips_new();
      buf = malloc (strlen(a->var) + 
      	(a->help == NULL ? 0 : strlen(a->help)) + 30);
      if (buf == NULL) {
	 fprintf (stderr, "Not enough memory.\n");
	 my_exit (1);
      }
      if (a->help != NULL) { 
         sprintf (buf, "%s\n(can be set with -A%s=)", a->help, a->var);
      } else {
         sprintf (buf, "(can be set with -A%s=)", a->var);
      }
      gtk_tooltips_set_tip (tooltips, event_box, buf, NULL);
      free (buf);

      hbox = gtk_hbox_new(FALSE, 00);
      gtk_container_add (GTK_CONTAINER(event_box), hbox);
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
      if (a->def_value != NULL) 
	 gtk_entry_set_text(GTK_ENTRY(widget), a->def_value);
      gtk_signal_connect( GTK_OBJECT(widget), "changed",
	     GTK_SIGNAL_FUNC(pwizard_text_change), 
	     (gpointer)(&a->def_value)); 
      gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
      gtk_widget_show (widget);

      l = l->prev;

   }

   l = last_list_element (interface_option_state->option_list);
   while (l != NULL) {

      o = (option *)l->data;

      frame = gtk_frame_new (o->desc);
      gtk_box_pack_end(GTK_BOX(safe_spot), frame, FALSE, FALSE, 0);
      gtk_container_border_width (GTK_CONTAINER (frame), 5);
      gtk_widget_show (frame);

      est_width = 0;
      cl = first_list_element (o->choice_list);
      while (cl != NULL) {
         est_width += 4;
         est_width += strlen( ((choice *)cl->data)->desc );
         cl = cl->next;
      }
      if (est_width < 55) {
         hbox = gtk_hbox_new (FALSE, 00);              /* Don't ask... */
      } else {
         hbox = gtk_vbox_new (FALSE, 00);              /* Don't ask... */
      }
      gtk_container_add (GTK_CONTAINER(frame), hbox);
      gtk_container_border_width (GTK_CONTAINER (hbox), 10);
      gtk_widget_show (hbox);

      cl = first_list_element (o->choice_list);
      ch = (choice *)cl->data;
      compress_whitespace (&ch->help);
      widget = gtk_radio_button_new_with_label (NULL, ch->desc);
      gtk_box_pack_start (GTK_BOX(hbox), widget, FALSE, TRUE, 0);
      gtk_widget_show (widget);
      if (ch->help != NULL) {
	 tooltips = gtk_tooltips_new();
	 buf = malloc (strlen(ch->name) + 
	 	(ch->help == NULL ? 0 : strlen(ch->help)) + 30);
	 if (buf == NULL) {
	    fprintf (stderr, "Not enough memory.\n");
	    my_exit (1);
	 }
	 sprintf (buf, "%s\n(can be set with -O%s)", 
	    ch->help, ch->name);
	 gtk_tooltips_set_tip (tooltips, widget, buf, NULL);
	 free (buf);
      }
      pp = new_pointer_pair();
      pp->a = o;
      pp->b = ch;
      interface_option_state->option_widget_pairs = 
      		append_to_list (interface_option_state->option_widget_pairs, pp);
      gtk_signal_connect( GTK_OBJECT(widget), "toggled",
	     GTK_SIGNAL_FUNC(set_option_clicked), (gpointer) pp);
      cl = cl->next;
      while (cl != NULL) {
	 ch = (choice *)cl->data;
         compress_whitespace (&ch->help);
	 widget = gtk_radio_button_new_with_label(
	                  gtk_radio_button_group (GTK_RADIO_BUTTON
			  (widget)), ch->desc);
         gtk_box_pack_start (GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	 gtk_widget_show (widget);
	 if (ch->help != NULL) {
	    tooltips = gtk_tooltips_new();
	    buf = malloc (strlen(ch->name) + 
	    	(ch->help == NULL ? 0 : strlen(ch->help)) + 30);
	    if (buf == NULL) {
	       fprintf (stderr, "Not enough memory.\n");
	       my_exit (1);
	    }
	    sprintf (buf, "%s\n(can be set with -O%s)", 
	       ch->help, ch->name);
	    gtk_tooltips_set_tip (tooltips, widget, buf, NULL);
	    free (buf);
	 }
	 if (0 == strcmp (ch->name, o->default_choice)) 
	    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
	 pp = new_pointer_pair();
	 pp->a = o;
	 pp->b = ch;
	 interface_option_state->option_widget_pairs = 
	     append_to_list (interface_option_state->option_widget_pairs, pp);
	 gtk_signal_connect( GTK_OBJECT(widget), "toggled",
		GTK_SIGNAL_FUNC(set_option_clicked), (gpointer) pp);
         cl = cl->next;
      }

      l = l->prev;

   }

   hbox = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(interface_option_panel), hbox, FALSE, FALSE, 00);
   gtk_widget_show(hbox);
   button = gtk_button_new_with_label ("  Okay  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 5);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(twiddle_interface_options_okay), NULL);
   gtk_widget_show(button);
   button = gtk_button_new_with_label ("  Cancel  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 5);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(twiddle_interface_options_cancel), NULL);
   gtk_widget_show(button);
   button = gtk_button_new_with_label ("  Set as default  ");
   gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(set_interface_options_as_default), NULL);
   gtk_widget_show(button);

}

dl_list *i_list_from_opt_list (dl_list *opt_list) {

   option *opt;
   char *i;
   dl_list *l;

   l = NULL;
   opt_list = first_list_element (opt_list);
   while (opt_list != NULL) {
      opt = (option *)opt_list->data;
      str_set (&i, opt->default_choice);
      l = append_to_list (l, i);
      opt_list = opt_list->next;
   }
   return (l);

}

dl_list *ip_list_from_arg_list (dl_list *arg_list) {

   argument *arg;
   item_pair *ip;
   dl_list *l;

   l = NULL;
   arg_list = first_list_element (arg_list);
   while (arg_list != NULL) {
      arg = (argument *)arg_list->data;
      ip = new_item_pair();
      str_set (&ip->key, arg->var);
      str_set (&ip->value, arg->def_value);
      l = append_to_list (l, ip);
      arg_list = arg_list->next;
   }
   return (l);

}

void option_set_arg_defaults (dl_list *arg_list, dl_list *l) {

   dl_list *a;
   argument *arg;
   item_pair *i;

   l = first_list_element (l);
   while (l != NULL) {
      i = (item_pair *)l->data; 
      a = find_in_list_by_string (arg_list, i->key);
      if (a != NULL) {
         arg = (argument *)a->data; 
         str_reset (&arg->def_value, i->value);
      }
      l = l->next;
   }

}

void option_set_opt_defaults (dl_list *opt_list, dl_list *l, dl_list *lbak) {

   dl_list *choice_list, *sel_list;
   option *opt;
   choice *ch;
   char *selection;
   int i, j, resolved;

   for (i = 0; i < 1; i++) {
      opt_list = first_list_element (opt_list);
      while (opt_list != NULL) {
	 opt = (option *) opt_list->data;
	 opt->choice_list = first_list_element (opt->choice_list);
	 resolved = 0;

	 /* Look in default option specification sources 
	  * for one of the choices in opt->choice_list;
	  */
	 for (j = 0; j < 2; j++) {
	    if (j == 0) {
	       sel_list = last_list_element(l);
	    } else {
	       sel_list = last_list_element(lbak);
	    }
	    while ((sel_list != NULL) && (!resolved)) {
	       selection = (char *) sel_list->data;
	       choice_list = opt->choice_list;
	       while ((choice_list != NULL) && (!resolved)) {
		  ch = (choice *) choice_list->data;
		  if (0 == strcmp (ch->name, selection)) resolved = 1;
		  choice_list = choice_list->next;
	       }
	       sel_list = sel_list->prev;
	    }
	 }
	 if (!resolved) {     
	    /* Nothing was specified in option lists.
	     * let's have a look in the option definition
	     */
	    if (opt->default_choice != NULL) {  
	       choice_list = opt->choice_list;
	       while ((choice_list != NULL) && (!resolved)) {
		  ch = (choice *) choice_list->data;
		  if (0 == strcmp (ch->name, opt->default_choice)) 
		  	resolved = 1;
		  choice_list = choice_list->next;
	       }
	    }
	 } 
	 if (!resolved) {     
	    /* Still nothing.
	     * We'll give up and default to the first choice listed
	     */
	     if (opt->choice_list == NULL) {
	        fprintf (stderr, "Why does %s have no choices?", opt->var);
	     }
	     ch = (choice *) opt->choice_list->data;
	 }

         /* We have a choice, let's rock and roll */
         str_reset (&opt->default_choice, ch->name);

	 opt_list = opt_list->next;
      }
   }

}
