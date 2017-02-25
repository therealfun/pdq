/* Copyright 1999, 2000 Jacob A. Langford
 *
 * xpdq_preferences.c
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

void xpdq_edit_preferences (void) {
   GtkWidget *button, *hbox, *widget, *vbox, *panel, *event_box, *frame;
   GtkAdjustment *adj;
   GtkTooltips *tooltips;

   if (xpdq_pref_window != NULL) return;

   str_reset(&xpdq_pref_ipath, rc->interface_command_path);
   str_reset(&xpdq_pref_dpath, rc->driver_command_path);
   xpdq_pref_mstries = rc->max_send_tries;
   xpdq_pref_dbtries = rc->delay_between_tries;
   xpdq_pref_jhduration = rc->job_history_duration;

   xpdq_pref_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(GTK_WINDOW (xpdq_pref_window), "Xpdq preferences");
   gtk_signal_connect_object (GTK_OBJECT (xpdq_pref_window), "delete_event",
		   GTK_SIGNAL_FUNC(xpdq_edit_preferences_cancel), NULL);

   panel = gtk_vbox_new (FALSE, 0);
   gtk_container_border_width (GTK_CONTAINER (panel), 10);
   gtk_container_add (GTK_CONTAINER (xpdq_pref_window), panel);
   gtk_widget_show (panel);

      /*************************************************************/
      event_box = gtk_event_box_new();
      gtk_box_pack_start(GTK_BOX(panel), event_box, FALSE, FALSE, 5);
      gtk_widget_show(event_box);

      tooltips = gtk_tooltips_new();
      gtk_tooltips_set_tip (tooltips, event_box, 
         "This is the path for all driver scripts.", NULL);

      hbox = gtk_hbox_new(FALSE, 00);
      gtk_container_add (GTK_CONTAINER(event_box), hbox);
      gtk_widget_show(hbox);
      widget = gtk_label_new("Driver path:");
      gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
      gtk_widget_show (widget);
      widget = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(widget), xpdq_pref_dpath);
      gtk_signal_connect( GTK_OBJECT(widget), "changed",
	     GTK_SIGNAL_FUNC(pwizard_text_change), 
	     (gpointer)(&xpdq_pref_dpath)); 
      gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
      gtk_widget_show (widget);

      /*************************************************************/
      event_box = gtk_event_box_new();
      gtk_box_pack_start(GTK_BOX(panel), event_box, FALSE, FALSE, 5);
      gtk_widget_show(event_box);

      tooltips = gtk_tooltips_new();
      gtk_tooltips_set_tip (tooltips, event_box, 
         "This is the path for all interface scripts.", NULL);

      hbox = gtk_hbox_new(FALSE, 00);
      gtk_container_add (GTK_CONTAINER(event_box), hbox);
      gtk_widget_show(hbox);
      widget = gtk_label_new("Interface path:");
      gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
      gtk_widget_show (widget);
      widget = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(widget), xpdq_pref_ipath);
      gtk_signal_connect( GTK_OBJECT(widget), "changed",
	     GTK_SIGNAL_FUNC(pwizard_text_change), 
	     (gpointer)(&xpdq_pref_ipath)); 
      gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
      gtk_widget_show (widget);

      /*************************************************************/
      event_box = gtk_event_box_new();
      gtk_box_pack_start(GTK_BOX(panel), event_box, FALSE, FALSE, 5);
      gtk_widget_show(event_box);

      tooltips = gtk_tooltips_new();
      gtk_tooltips_set_tip (tooltips, event_box, 
         "Maximum times to try sending job.", NULL);

      hbox = gtk_hbox_new(FALSE, 00);
      gtk_container_add (GTK_CONTAINER(event_box), hbox);
      gtk_widget_show(hbox);
      widget = gtk_label_new("Max send tries:");
      gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
      gtk_widget_show (widget);
      adj = (GtkAdjustment *) 
       	 gtk_adjustment_new (xpdq_pref_mstries,1,1000,1,10,100);
      widget = gtk_spin_button_new (adj, 0.0, 0);
      gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
             GTK_SIGNAL_FUNC (xpdq_spin_value_changed),
	     (gpointer) widget);
      gtk_object_set_user_data (GTK_OBJECT(widget), &xpdq_pref_mstries);
      gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
      gtk_widget_show(widget);

      /*************************************************************/
      event_box = gtk_event_box_new();
      gtk_box_pack_start(GTK_BOX(panel), event_box, FALSE, FALSE, 5);
      gtk_widget_show(event_box);

      tooltips = gtk_tooltips_new();
      gtk_tooltips_set_tip (tooltips, event_box, 
         "Delay (in seconds) between unsuccesful attempts to sending a job.",
	 NULL);

      hbox = gtk_hbox_new(FALSE, 00);
      gtk_container_add (GTK_CONTAINER(event_box), hbox);
      gtk_widget_show(hbox);
      widget = gtk_label_new("Delay between tries (seconds):");
      gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
      gtk_widget_show (widget);
      adj = (GtkAdjustment *) 
      	gtk_adjustment_new (xpdq_pref_dbtries,0,10000,1,60,0);
      widget = gtk_spin_button_new (adj, 0.0, 0);
      gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
             GTK_SIGNAL_FUNC (xpdq_spin_value_changed),
	     (gpointer) widget);
      gtk_object_set_user_data (GTK_OBJECT(widget), &xpdq_pref_dbtries);
      gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
      gtk_widget_show(widget);

      /*************************************************************/
      event_box = gtk_event_box_new();
      gtk_box_pack_start(GTK_BOX(panel), event_box, FALSE, FALSE, 5);
      gtk_widget_show(event_box);

      tooltips = gtk_tooltips_new();
      gtk_tooltips_set_tip (tooltips, event_box, 
         "Age (in seconds) before old jobs are deleted.",
	 NULL);

      hbox = gtk_hbox_new(FALSE, 00);
      gtk_container_add (GTK_CONTAINER(event_box), hbox);
      gtk_widget_show(hbox);
      widget = gtk_label_new("Job history duration (seconds):");
      gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
      gtk_widget_show (widget);
      adj = (GtkAdjustment *) 
      	gtk_adjustment_new (xpdq_pref_jhduration,0,1000000,1,3600,0);
      widget = gtk_spin_button_new (adj, 0.0, 0);
      gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
             GTK_SIGNAL_FUNC (xpdq_spin_value_changed),
	     (gpointer) widget);
      gtk_object_set_user_data (GTK_OBJECT(widget), &xpdq_pref_jhduration);
      gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
      gtk_widget_show(widget);

   hbox = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_end(GTK_BOX(panel), hbox, FALSE, FALSE, 0);
   gtk_widget_show(hbox);
   button = gtk_button_new_with_label ("  Save  ");
   gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(xpdq_edit_preferences_okay), NULL);
   gtk_widget_show(button);
   button = gtk_button_new_with_label ("  Cancel  ");
   gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 5);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(xpdq_edit_preferences_cancel), NULL);
   gtk_widget_show(button);

   gtk_widget_show (xpdq_pref_window);

}

void xpdq_edit_preferences_cancel (void) {
   gtk_widget_destroy (xpdq_pref_window);
   xpdq_pref_window = NULL;
}

void xpdq_edit_preferences_okay (void) {

   char *buf, *buf2;

   if (strcmp(xpdq_pref_ipath, rc->interface_command_path)) {
      str_reset(&rc->interface_command_path, xpdq_pref_ipath);
      buf2 = escape_block (xpdq_pref_ipath, '"', '"');
      buf = malloc (strlen(buf2) + 30);
      if (buf == NULL) {
	 fprintf (stderr, "Not enough memory.\n");
	 my_exit (1);
      }
      sprintf (buf, "\n\ninterface_command_path %s", buf2);
      rc_set ("interface_command_path", NULL, buf);
      free (buf2);
      free(buf);
   }
   if (strcmp(xpdq_pref_dpath, rc->driver_command_path)) {
      str_reset(&rc->driver_command_path, xpdq_pref_dpath);
      buf2 = escape_block (xpdq_pref_dpath, '"', '"');
      buf = malloc (strlen(buf2) + 30);
      if (buf == NULL) {
	 fprintf (stderr, "Not enough memory.\n");
	 my_exit (1);
      }
      sprintf (buf, "\n\ndriver_command_path %s", buf2);
      rc_set ("driver_command_path", NULL, buf);
      free (buf2);
      free(buf);
   }
   if (xpdq_pref_mstries != rc->max_send_tries) {
      rc->max_send_tries = xpdq_pref_mstries;
      buf = malloc (50);
      if (buf == NULL) {
	 fprintf (stderr, "Not enough memory.\n");
	 my_exit (1);
      }
      sprintf (buf, "\n\nmax_send_tries %.li", rc->max_send_tries);
      rc_set ("max_send_tries", NULL, buf);
      free(buf);
   }
   if (xpdq_pref_dbtries != rc->delay_between_tries) {
      rc->delay_between_tries = xpdq_pref_dbtries;
      buf = malloc (50);
      if (buf == NULL) {
	 fprintf (stderr, "Not enough memory.\n");
	 my_exit (1);
      }
      sprintf (buf, "\n\ndelay_between_tries %.li", rc->delay_between_tries);
      rc_set ("delay_between_tries", NULL, buf);
      free(buf);
   }
   if (xpdq_pref_jhduration != rc->job_history_duration) {
      rc->job_history_duration = xpdq_pref_jhduration;
      buf = malloc (50);
      if (buf == NULL) {
	 fprintf (stderr, "Not enough memory.\n");
	 my_exit (1);
      }
      sprintf (buf, "\n\njob_history_duration %.li", rc->job_history_duration);
      rc_set ("job_history_duration", NULL, buf);
      free(buf);
   }

   xpdq_edit_preferences_cancel();

}

void xpdq_spin_value_changed (GtkWidget *widget, GtkSpinButton *spin) {

   long *num;
   num = gtk_object_get_user_data (GTK_OBJECT(spin));
   *num = gtk_spin_button_get_value_as_int (spin);

}
