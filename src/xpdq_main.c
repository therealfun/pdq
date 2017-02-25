/* Copyright 1999, 2000 Jacob A. Langford
 *
 * xpdq_main.c
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
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

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
#include "shepherd.h"

void (*my_exit) (int code);

void xpdq_exit (int code) {
   exit (code);
}

char *default_file;

rc_items *rc;
job_info *job;

dl_list *job_info_list;

GtkWidget *printer_list_widget;
GtkWidget *job_list_widget;

GtkWidget *main_window;

GtkWidget *details_widget;
GtkWidget *details_window;

GtkWidget *driver_option_window;
GtkWidget *driver_option_panel;
GtkWidget *interface_option_window;
GtkWidget *interface_option_panel;

GdkFont  *fixed_font, *bold_font;
GdkColor *error_color, *busy_color, *ok_color, *red_color;

option_state *driver_option_state;
option_state *interface_option_state;

GtkWidget *xpdq_pref_window;
char *xpdq_pref_ipath;
char *xpdq_pref_dpath;
long xpdq_pref_mstries;
long xpdq_pref_dbtries;
long xpdq_pref_jhduration;

int main (int argc, char *argv[]) {


   default_file = NULL;
   if (argc > 1) {
      default_file = argv[argc - 1]; 
   }
   my_exit = xpdq_exit;

   rc = new_rc_items();
   job = NULL;

   try_parse_rc_glob (PRINTRC_FILE, rc);
   try_parse_rc_glob ("~/.printrc", rc);

   gtk_init (&argc, &argv);

   signal (SIGCHLD, reaper); 

   xpdq_pref_ipath = NULL;
   xpdq_pref_dpath = NULL;

   xpdq_pref_window = NULL;

   job_info_list = NULL;
   driver_option_window = NULL;
   interface_option_window = NULL;
   driver_option_panel = NULL;
   interface_option_panel = NULL;
   driver_option_state = NULL;
   interface_option_state = NULL;

   set_main_screen (); 
   reconcile_printer_list ();  /* Do this here so that the selected
                                * printer will be scrolled to
				*/

   gtk_timeout_add ( 1000*5, (GtkFunction) update_job_info, NULL); 

   gtk_main ();

   return 0;
}

void set_main_screen (void) {

   GtkWidget    *main_vbox;
   GtkWidget       *main_menu_bar;
   GtkWidget       *vpane;
   GtkWidget          *pframe; 
   GtkWidget             *pinfo_box;
   GtkWidget                *scrolled_window;
   GtkWidget                   *clistbox;
   GtkWidget                *button_hbox;
   GtkWidget                   *button;
   GtkWidget *printer_menu;
   GtkWidget *job_menu;
   GtkWidget *help_menu;
   GtkWidget *file_menu;
   GtkWidget *menu_item;
   gchar *ctitles[10];

   main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(GTK_WINDOW (main_window), "xpdq");
   gtk_signal_connect(GTK_OBJECT (main_window), "delete_event",
                      (GtkSignalFunc) gtk_main_quit, NULL);

   initialize_resources ();

   main_vbox = gtk_vbox_new(FALSE, 0);
   gtk_container_add(GTK_CONTAINER(main_window), main_vbox);
   gtk_widget_show(main_vbox);

   main_menu_bar = gtk_menu_bar_new();
   gtk_box_pack_start(GTK_BOX(main_vbox), main_menu_bar, FALSE, FALSE, 2);
   gtk_widget_show(main_menu_bar);

   vpane = gtk_vpaned_new();
   gtk_box_pack_start(GTK_BOX(main_vbox), vpane, TRUE, TRUE, 0);
   gtk_widget_show (vpane);

   /* Do menus */
   file_menu = gtk_menu_new();

      menu_item = gtk_menu_item_new_with_label("File");
      gtk_widget_show(menu_item);
      gtk_menu_item_set_submenu(GTK_MENU_ITEM (menu_item), file_menu);
      gtk_menu_bar_append(GTK_MENU_BAR (main_menu_bar), menu_item);

      menu_item = gtk_menu_item_new_with_label("Print file");
      gtk_menu_append(GTK_MENU (file_menu), menu_item);
      gtk_widget_show(menu_item);
      gtk_signal_connect_object( GTK_OBJECT(menu_item), "activate",
	    GTK_SIGNAL_FUNC(xpdq_print_file), NULL); 

      menu_item = gtk_menu_item_new_with_label("Print stdin");
      gtk_menu_append(GTK_MENU (file_menu), menu_item);
      gtk_widget_show(menu_item);
      gtk_signal_connect_object( GTK_OBJECT(menu_item), "activate",
	    GTK_SIGNAL_FUNC(xpdq_print_stdin), NULL); 

      menu_item = gtk_menu_item_new_with_label("Preferences");
      gtk_menu_append(GTK_MENU (file_menu), menu_item);
      gtk_widget_show(menu_item);
      gtk_signal_connect_object( GTK_OBJECT(menu_item), "activate",
	    GTK_SIGNAL_FUNC(xpdq_edit_preferences), NULL); 

      menu_item = gtk_menu_item_new_with_label("Quit");
      gtk_menu_append(GTK_MENU (file_menu), menu_item);
      gtk_widget_show(menu_item);
      gtk_signal_connect_object( GTK_OBJECT(menu_item), "activate",
	    GTK_SIGNAL_FUNC(gtk_main_quit), NULL); 

   printer_menu = gtk_menu_new();

      menu_item = gtk_menu_item_new_with_label("Printer");
      gtk_widget_show(menu_item);
      gtk_menu_item_set_submenu(GTK_MENU_ITEM (menu_item), printer_menu);
      gtk_menu_bar_append(GTK_MENU_BAR (main_menu_bar), menu_item);

      menu_item = gtk_menu_item_new_with_label("Show errors");
      gtk_menu_append(GTK_MENU (printer_menu), menu_item);
      gtk_widget_show(menu_item);
      gtk_signal_connect_object( GTK_OBJECT(menu_item), "activate",
	    GTK_SIGNAL_FUNC(show_printer_problems), NULL); 

      menu_item = gtk_menu_item_new_with_label("Add printer");
      gtk_menu_append(GTK_MENU (printer_menu), menu_item);
      gtk_widget_show(menu_item);
      gtk_signal_connect_object( GTK_OBJECT(menu_item), "activate",
	    GTK_SIGNAL_FUNC(xpdq_add_printer), NULL); 

      menu_item = gtk_menu_item_new_with_label("Delete printer");
      gtk_menu_append(GTK_MENU (printer_menu), menu_item);
      gtk_widget_show(menu_item);
      gtk_signal_connect_object( GTK_OBJECT(menu_item), "activate",
	    GTK_SIGNAL_FUNC(xpdq_delete_printer), NULL); 

      menu_item = gtk_menu_item_new_with_label("Configure driver");
      gtk_menu_append(GTK_MENU (printer_menu), menu_item);
      gtk_widget_show(menu_item);
      gtk_signal_connect_object( GTK_OBJECT(menu_item), "activate",
	    GTK_SIGNAL_FUNC(twiddle_driver_options), NULL); 

      menu_item = gtk_menu_item_new_with_label("Configure interface");
      gtk_menu_append(GTK_MENU (printer_menu), menu_item);
      gtk_widget_show(menu_item);
      gtk_signal_connect_object( GTK_OBJECT(menu_item), "activate",
	    GTK_SIGNAL_FUNC(twiddle_interface_options), NULL); 

      menu_item = gtk_menu_item_new_with_label("Show status");
      gtk_menu_append(GTK_MENU (printer_menu), menu_item);
      gtk_widget_show(menu_item);
      gtk_signal_connect_object( GTK_OBJECT(menu_item), "activate",
	    GTK_SIGNAL_FUNC(xpdq_show_printer_status), NULL); 

      menu_item = gtk_menu_item_new_with_label("Set as default printer");
      gtk_menu_append(GTK_MENU (printer_menu), menu_item);
      gtk_widget_show(menu_item);
      gtk_signal_connect_object( GTK_OBJECT(menu_item), "activate",
	    GTK_SIGNAL_FUNC(xpdq_set_as_default_printer), NULL); 

   job_menu = gtk_menu_new();

      menu_item = gtk_menu_item_new_with_label("Job");
      gtk_widget_show(menu_item);
      gtk_menu_item_set_submenu(GTK_MENU_ITEM (menu_item), job_menu);
      gtk_menu_bar_append(GTK_MENU_BAR (main_menu_bar), menu_item);

      menu_item = gtk_menu_item_new_with_label("Request job cancel");
      gtk_menu_append(GTK_MENU (job_menu), menu_item);
      gtk_widget_show(menu_item);
      gtk_signal_connect_object( GTK_OBJECT(menu_item), "activate",
	    GTK_SIGNAL_FUNC(xpdq_cancel_print_job), NULL); 

      menu_item = gtk_menu_item_new_with_label("Resend to selected printer");
      gtk_menu_append(GTK_MENU (job_menu), menu_item);
      gtk_widget_show(menu_item);
      gtk_signal_connect_object( GTK_OBJECT(menu_item), "activate",
	    GTK_SIGNAL_FUNC(xpdq_redirect_job), NULL); 

      menu_item = gtk_menu_item_new_with_label("Delete job");
      gtk_menu_append(GTK_MENU (job_menu), menu_item);
      gtk_widget_show(menu_item);
      gtk_signal_connect_object( GTK_OBJECT(menu_item), "activate",
	    GTK_SIGNAL_FUNC(xpdq_delete_job), NULL); 

      menu_item = gtk_menu_item_new_with_label("Show details");
      gtk_menu_append(GTK_MENU (job_menu), menu_item);
      gtk_widget_show(menu_item);
      gtk_signal_connect_object( GTK_OBJECT(menu_item), "activate",
	    GTK_SIGNAL_FUNC(xpdq_show_job_details), NULL); 

   
   help_menu = gtk_menu_new();

      menu_item = gtk_menu_item_new_with_label("Help");
      gtk_widget_show(menu_item);
      gtk_menu_item_set_submenu(GTK_MENU_ITEM (menu_item), help_menu);
      gtk_menu_bar_append(GTK_MENU_BAR (main_menu_bar), menu_item);
      gtk_menu_item_right_justify (GTK_MENU_ITEM (menu_item));

      menu_item = gtk_menu_item_new_with_label("About");
      gtk_menu_append(GTK_MENU (help_menu), menu_item);
      gtk_widget_show(menu_item);
      gtk_signal_connect_object( GTK_OBJECT(menu_item), "activate",
	    GTK_SIGNAL_FUNC(xpdq_about), NULL); 


   /* Printer selection box */
   pframe = gtk_frame_new ("Selected printer");
   gtk_widget_set_usize(GTK_WIDGET(pframe), 750, 200);
   gtk_paned_add1 (GTK_PANED(vpane), pframe);
   gtk_container_border_width (GTK_CONTAINER(pframe), 10);
   gtk_widget_show (pframe);

   pinfo_box = gtk_vbox_new(FALSE, 0);
   gtk_container_add(GTK_CONTAINER(pframe), pinfo_box);
   gtk_container_border_width (GTK_CONTAINER(pinfo_box), 5);
   gtk_widget_show (pinfo_box);

   clistbox = gtk_vbox_new(FALSE, 00);
   gtk_box_pack_start(GTK_BOX(pinfo_box), clistbox, TRUE, TRUE, 0);
   gtk_container_border_width (GTK_CONTAINER(clistbox), 10);
   gtk_widget_show(clistbox);

   ctitles[0] = "Printer";
   ctitles[1] = "Model";
   ctitles[2] = "Location";
   printer_list_widget = gtk_clist_new_with_titles (3, ctitles);
   gtk_signal_connect( GTK_OBJECT(printer_list_widget), 
         "button-press-event", GTK_SIGNAL_FUNC(do_menu_popup), 
	 printer_menu); 
   gtk_clist_column_titles_show ((GtkCList *)printer_list_widget);
   gtk_clist_column_titles_passive ((GtkCList *)printer_list_widget);
   gtk_clist_set_column_width ((GtkCList *)printer_list_widget, 0, 100);
   gtk_clist_set_column_width ((GtkCList *)printer_list_widget, 1, 200);
   gtk_clist_set_column_width ((GtkCList *)printer_list_widget, 2, 100);

   gtk_signal_connect( GTK_OBJECT(printer_list_widget), "select_row",
	  GTK_SIGNAL_FUNC(printer_selected), NULL);

   scrolled_window = gtk_scrolled_window_new (NULL, NULL);
   gtk_box_pack_start(GTK_BOX(clistbox), scrolled_window, TRUE, TRUE, 0);
   gtk_container_add (GTK_CONTAINER (scrolled_window), printer_list_widget);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_widget_show (scrolled_window);

   gtk_clist_set_selection_mode( (GtkCList  *)printer_list_widget,
      GTK_SELECTION_BROWSE);

   gtk_widget_show (printer_list_widget);



   button_hbox = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(pinfo_box), button_hbox, FALSE, FALSE, 00);
   gtk_widget_show(button_hbox);

   button = gtk_button_new_with_label ("  Interface options  ");
   gtk_box_pack_end(GTK_BOX(button_hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(twiddle_interface_options), NULL); 
   gtk_widget_show(button);

   button = gtk_button_new_with_label ("  Driver options  ");
   gtk_box_pack_end(GTK_BOX(button_hbox), button, FALSE, FALSE, 10);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(twiddle_driver_options), NULL); 
   gtk_widget_show(button);




   /* Printer selection box */
   pframe = gtk_frame_new ("Recent jobs");
   gtk_paned_add2 (GTK_PANED(vpane), pframe);
   gtk_container_border_width (GTK_CONTAINER(pframe), 10);
   gtk_widget_show (pframe);

   pinfo_box = gtk_vbox_new(FALSE, 0);
   gtk_container_add(GTK_CONTAINER(pframe), pinfo_box);
   gtk_container_border_width (GTK_CONTAINER(pinfo_box), 5);
   gtk_widget_show (pinfo_box);

   clistbox = gtk_vbox_new(FALSE, 00);
   gtk_box_pack_start(GTK_BOX(pinfo_box), clistbox, TRUE, TRUE, 0);
   gtk_container_border_width (GTK_CONTAINER(clistbox), 10);
   gtk_widget_show(clistbox);

   ctitles[0] = "Job ID";
   ctitles[1] = "Status";
   ctitles[2] = "Filename";
   ctitles[3] = "File_type";
   ctitles[4] = "Printer";
   ctitles[5] = "Time submitted";
   job_list_widget = gtk_clist_new_with_titles (6, ctitles);
   gtk_signal_connect( GTK_OBJECT(job_list_widget), 
         "button-press-event", GTK_SIGNAL_FUNC(do_menu_popup), 
	 job_menu); 
   gtk_clist_column_titles_show ((GtkCList *)job_list_widget);
   gtk_clist_column_titles_passive ((GtkCList *)job_list_widget);
   gtk_clist_set_column_width ((GtkCList *)job_list_widget, 0, 50);
   gtk_clist_set_column_width ((GtkCList *)job_list_widget, 1, 125);
   gtk_clist_set_column_width ((GtkCList *)job_list_widget, 2, 125);
   gtk_clist_set_column_width ((GtkCList *)job_list_widget, 3, 125);
   gtk_clist_set_column_width ((GtkCList *)job_list_widget, 4, 100);


   scrolled_window = gtk_scrolled_window_new (NULL, NULL);
   gtk_box_pack_start(GTK_BOX(clistbox), scrolled_window, TRUE, TRUE, 0);
   gtk_container_add (GTK_CONTAINER (scrolled_window), job_list_widget);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_widget_show (scrolled_window);

   update_job_info();



   gtk_clist_set_selection_mode( (GtkCList  *)job_list_widget,
      GTK_SELECTION_BROWSE);
   gtk_signal_connect( GTK_OBJECT(job_list_widget), "select_row",
	  GTK_SIGNAL_FUNC(job_selected), NULL);

   gtk_widget_show (job_list_widget);


   gtk_window_set_default_size(GTK_WINDOW(main_window), 750, 400);
   gtk_widget_show(main_window);


}

void selection_made( GtkWidget *clist, gint row, gint column, 
   GdkEventButton *event, gpointer data ) {

   if (event == NULL) {
      return;
   }
   if (event->button == 1) {
      gtk_menu_popup (GTK_MENU (data), NULL, NULL, NULL, NULL,
          event->button, event->time);
   }
   return;

}

void do_menu_popup ( GtkWidget *clist, GdkEventButton *event, 
     		gpointer data) {
   if (event == NULL) {
      return;
   }
   if (event->button == 3) {
      gtk_menu_popup (GTK_MENU (data), NULL, NULL, NULL, NULL,
          event->button, event->time);
   }
   return;
}

void initialize_resources (void) {

  fixed_font = gdk_font_load ("-misc-fixed-medium-r-*-*-*-120-*-*-*-*-*-*");

  red_color = (GdkColor *)malloc(sizeof(GdkColor));
  red_color->red = 255 * (65535/255);
  red_color->green = 0 * (65535/255);
  red_color->blue = 0 * (65535/255);
  red_color->pixel = (gulong)(255*65536 + 0*256 + 0);
  gdk_color_alloc(gdk_colormap_get_system(), red_color);

  error_color = (GdkColor *)malloc(sizeof(GdkColor));
  error_color->red = 255 * (65535/255);
  error_color->green = 150 * (65535/255);
  error_color->blue = 170 * (65535/255);
  error_color->pixel = (gulong)(255*65536 + 150*256 + 170);
  gdk_color_alloc(gdk_colormap_get_system(), error_color);

  busy_color = (GdkColor *)malloc(sizeof(GdkColor));
  busy_color->red = 180 * (65535/255);
  busy_color->green = 255 * (65535/255);
  busy_color->blue = 180 * (65535/255);
  busy_color->pixel = (gulong)(255*65536 + 255*256 + 180);
  gdk_color_alloc(gdk_colormap_get_system(), busy_color);

  ok_color = (GdkColor *)malloc(sizeof(GdkColor));
  ok_color->red = 255 * (65535/255);
  ok_color->green = 255 * (65535/255);
  ok_color->blue = 255 * (65535/255);
  ok_color->pixel = (gulong)(255*65536 + 255*256 + 255);
  gdk_color_alloc(gdk_colormap_get_system(), ok_color);

}

void show_details_window (void) {

   GtkWidget *vbox;
   GtkWidget *details_widget_container;
   GtkWidget *button;
   GtkWidget *buttonbox;

   if (details_window != NULL)  return;

   details_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(GTK_WINDOW (details_window), "xpdq details");
   gtk_signal_connect(GTK_OBJECT (details_window), "delete_event",
		      (GtkSignalFunc) details_close, NULL);

   vbox = gtk_vbox_new(FALSE, 10);
   gtk_container_add(GTK_CONTAINER(details_window), vbox);
   gtk_container_border_width (GTK_CONTAINER(vbox), 10);
   gtk_widget_show (vbox);

   details_widget_container = gtk_scrolled_window_new (NULL, NULL);
   gtk_box_pack_start(GTK_BOX(vbox), 
	  details_widget_container, TRUE, TRUE, 0);
   gtk_scrolled_window_set_policy (
      GTK_SCROLLED_WINDOW (details_widget_container),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_widget_show (details_widget_container);

   details_widget = gtk_text_new (FALSE, FALSE);
   gtk_text_set_word_wrap (GTK_TEXT (details_widget), TRUE);
   gtk_container_add (GTK_CONTAINER (details_widget_container),
   	details_widget);
   gtk_widget_show (details_widget);

   buttonbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start(GTK_BOX(vbox), buttonbox, FALSE, FALSE, 0);
   gtk_widget_show (buttonbox);
   button = gtk_button_new_with_label ("        Dismiss        ");
   gtk_box_pack_end(GTK_BOX(buttonbox), button, TRUE, FALSE, 00);
   gtk_widget_show(button);
   gtk_signal_connect_object( GTK_OBJECT(button), "clicked",
	  GTK_SIGNAL_FUNC(details_close), NULL); 

   gtk_window_set_default_size(GTK_WINDOW(details_window), 600, 200);
   gtk_widget_show(details_window);

}

void erase_old_details (void) {
   gtk_text_backward_delete (
      GTK_TEXT (details_widget),
      gtk_text_get_length ( GTK_TEXT (details_widget) ) );
}

void add_details (char *text) {
   if (text == NULL) return;
   gtk_text_insert (GTK_TEXT (details_widget), NULL,
      &details_widget->style->black, NULL, 
      text, -1);
}

void add_red_details (char *text) {
   if (text == NULL) return;
   gtk_text_insert (GTK_TEXT (details_widget), NULL,
      red_color, NULL, text, -1);
}

void add_bold_details (char *text) {
   if (text == NULL) return;
   gtk_text_insert (GTK_TEXT (details_widget), NULL,
      red_color, NULL, text, -1);
}

void add_fixed_font_details (char *text) {
   if (text == NULL) return;
   gtk_text_insert (GTK_TEXT (details_widget), fixed_font,
      &details_widget->style->black, NULL, 
      text, -1);
}

void details_close (void) {
   gtk_widget_destroy (details_window);
   details_window = NULL;
}

void xpdq_about (void) {
   
   show_details_window();
   gtk_text_freeze ( GTK_TEXT(details_widget) );
   erase_old_details();
   add_details(
   "xpdq version 2.2.1  -  Copyright 1999, 2000 Jacob A. Langford\n\n"

   "Try http://feynman.tam.uiuc.edu/pdq for documentation.  Send bug "
   "reports and/or fixes to langford@uiuc.edu.\n\n"

   "This program is free software; you can redistribute it and/or "
   "modify it under the terms of the GNU General Public License "
   "version 2.\n\n"
   );

   add_bold_details (
   "This program is distributed in the hope that it will be useful, "
   "but WITHOUT ANY WARRANTY; without even the implied warranty of "
   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
   "GNU General Public License for more details.\n\n"
   );

   add_details(
   "You should have received a copy of the GNU General Public License "
   "along with this program; if not, write to the Free Software "
   "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA "
   "02111-1307, USA.\n"
   );
   gtk_text_thaw ( GTK_TEXT(details_widget) );

}

void xpdq_print_file (void) {
    GtkWidget *filew;
    
    filew = gtk_file_selection_new ("Xpdq print file");
    
    gtk_signal_connect (GTK_OBJECT (filew), "destroy",
			(GtkSignalFunc) gtk_widget_destroy, &filew);

    /* Connect the ok_button to file_ok_sel function */
    gtk_signal_connect(
    	GTK_OBJECT (
		GTK_FILE_SELECTION (filew)->ok_button),
	  	"clicked", (GtkSignalFunc) xpdq_print_file_ok, filew );

    gtk_label_set_text (
       GTK_LABEL(
          GTK_BIN(
	     GTK_FILE_SELECTION(filew)->ok_button
	  )->child
       ), "Print");

    gtk_signal_connect_object (GTK_OBJECT
    	(GTK_FILE_SELECTION (filew)->cancel_button),
		"clicked", (GtkSignalFunc) gtk_widget_destroy,
		GTK_OBJECT (filew));
    
    gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION(filew));

    if (default_file != NULL) {
       gtk_file_selection_set_filename (GTK_FILE_SELECTION(filew), 
       		default_file);
    }

    gtk_widget_show(filew);

}

void xpdq_print_file_ok (GtkWidget *w, GtkWidget *fs) {

   char *filename;

   filename = gtk_file_selection_get_filename ( GTK_FILE_SELECTION (fs));

   xpdq_print_something (filename, filename);;

   gtk_widget_destroy (fs);

}

void xpdq_print_something (char *filename, char *file) {

   job_info *j;
   char *buf;
   time_t t;
   int row;
   printer *p;
   dl_list *ll;
   pid_t pid;

   if (GTK_CLIST(printer_list_widget)->selection == NULL) return;
   row = (int) GTK_CLIST(printer_list_widget)->selection->data;
   p = gtk_clist_get_row_data (GTK_CLIST (printer_list_widget), row);
   if (p == NULL) {
      fprintf (stderr, "Should have found a printer for this row!\n");
      return;
   }

   j = new_job_info();
   str_reset (&j->input_filename, filename);
   if (file != NULL) {
      j->fd = open(file, O_RDONLY); 
      if (j->fd == -1) {
	  buf = malloc (strlen (j->input_filename) 
	      + 50 + strlen (strerror(errno))); 
	  if (buf == NULL) {
	     fprintf (stderr, "Not enough memory.\n");
	     my_exit (1);
	  }
	  sprintf (buf, "Cannot open %s\n%s", j->input_filename,
		   strerror(errno));
	  xpdq_error (buf);
	  free (buf);
	  free_job_info (j);
	  return;
      }
   }
   str_reset (&j->invoked_by, "xpdq");
   time(&t);
   str_reset (&j->invoke_time, ctime(&t));
   j->invoke_time [strlen(j->invoke_time)-1] = 0; /* Strip \n */
   str_reset (&j->printer, p->name);

   if (verify_printer (row, p, 0) == 0) { 
       buf = malloc (strlen (p->name) + 50);
       if (buf == NULL) {
          fprintf (stderr, "Not enough memory.\n");
	  my_exit (1);
       }
       sprintf (buf, "Printer %s is unusable", p->name);
       xpdq_error (buf);
       free (buf);
       free_job_info (j);
       return;
   }
   /* Set job->my-printer, etc.
   fork etc.
   */
   j->my_printer = copy_printer (p);
   ll = find_in_list_by_string (rc->interface_list, p->interface);
   if (ll == NULL) {
      fprintf (stderr, "Should have found an interface!\n");
      my_exit (1);
   }
   j->my_interface = copy_interface((interface *)ll->data);
   ll = find_in_list_by_string (rc->driver_list, p->driver);
   if (ll == NULL) {
      fprintf (stderr, "Should have found an driver!\n");
      my_exit (1);
   }
   j->my_driver = copy_driver((driver *)ll->data);

   pid = fork(); 

   switch (pid) {   
      case -1:
         perror ("fork");
	 my_exit (1);
      case 0:
	 job = j;
	 my_exit = pdq_exit;
	 shepherd ( j, rc );
	 my_exit (0);
   }

   close (j->fd);
   free_job_info (j);

}

void xpdq_print_stdin (void) {
   
   fd_set read_set;
   struct timeval tv;
   int n;

   FD_SET (0, &read_set);
   tv.tv_sec = 0;         /* Don't block */
   tv.tv_usec = 0;
   n = select (1, &read_set, NULL, NULL, &tv);
   if (n != 1) {
      xpdq_error ("There is no data on stdin.\nPerhaps your application"
      " prints data to a tmp file,\nin which case you should use the \"print"
      " file\" menu.");
      return;
   }

   xpdq_print_something ("stdin", NULL);;

}

void xpdq_error (char *message) {

   GtkWidget *dialog, *msg, *but, *msg_container;

   dialog = gtk_dialog_new ();
   msg = gtk_label_new (message);
   gtk_label_set_justify (GTK_LABEL (msg), GTK_JUSTIFY_LEFT );
   msg_container = gtk_hbox_new(FALSE, 0);
   gtk_container_border_width (GTK_CONTAINER (msg_container), 50);
   gtk_container_add (GTK_CONTAINER (msg_container), msg);
   but = gtk_button_new_with_label ("  OK  ");
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
                            but, FALSE, FALSE, 0);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                            msg_container, FALSE, FALSE, 0);
   gtk_signal_connect_object( GTK_OBJECT(but), "clicked",
	  GTK_SIGNAL_FUNC(xpdq_error_dismiss), (gpointer)(dialog)); 
   gtk_widget_show (but);
   gtk_widget_show (msg);
   gtk_widget_show (msg_container);
   gtk_widget_show (dialog);
   gtk_grab_add (dialog);

}

void xpdq_error_dismiss (GtkWidget *widget) {
   gtk_grab_remove (widget);
   gtk_widget_destroy (widget);
}

