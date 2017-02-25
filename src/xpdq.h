/* Copyright 1999, 2000 Jacob A. Langford
 *
 * xpdq.h
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

#ifndef __XPDQ_H_
#define __XPDQ_H_

#include <gtk/gtk.h>

#include "printer.h"
#include "driver.h"
#include "util.h"
#include "rc_items.h"

extern char *default_file;

extern rc_items *rc;
extern char *selected_printer; 
extern char *selected_job; 
extern GtkWidget *printer_list_widget;
extern GtkWidget *job_list_widget;
extern GtkWidget *details_window;
extern GtkWidget *details_widget;
extern GtkWidget *driver_option_window;
extern GtkWidget *interface_option_window;
extern GtkWidget *driver_option_panel;
extern GtkWidget *interface_option_panel;

extern GdkFont  *fixed_font;
extern GdkColor *red_color;
extern GdkColor *error_color;
extern GdkColor *busy_color;
extern GdkColor *ok_color;

typedef struct option_state_ option_state;
struct option_state_ {
   dl_list *argument_list; 
   dl_list *option_list; 
   dl_list *option_widget_pairs;
};

extern option_state *driver_option_state;
extern option_state *interface_option_state;

typedef struct pwizard_state_ pwizard_state;
struct pwizard_state_ {
   printer *p;
   GtkWidget *main_window;
   GtkWidget *panel;
   int panel_showing;
};

extern GtkWidget *xpdq_pref_window;
extern char *xpdq_pref_ipath;
extern char *xpdq_pref_dpath;
extern long xpdq_pref_mstries;
extern long xpdq_pref_dbtries;
extern long xpdq_pref_jhduration;

option_state *new_option_state (void);
void free_option_state (option_state *s);
void twiddle_driver_options (void);
void twiddle_interface_options (void);
void twiddle_driver_options_cancel (void);
void twiddle_interface_options_cancel (void);
void twiddle_driver_options_okay (void);
void twiddle_interface_options_okay (void);
void option_set_arg_defaults (dl_list *arg_list, dl_list *l); 
void option_set_opt_defaults (dl_list *opt_list, dl_list *l, dl_list *lbak); 
void set_driver_options_as_default (void);
void set_interface_options_as_default (void);
dl_list *ip_list_from_arg_list (dl_list *arg_list); 
dl_list *i_list_from_opt_list (dl_list *opt_list); 
void set_option_clicked (GtkWidget *radio_button, pointer_pair *pp);

void xpdq_print_stdin (void);
void xpdq_print_file (void);
void xpdq_print_file_ok (GtkWidget *w, GtkWidget *fs);
void xpdq_print_something (char *filename, char *file);
void xpdq_edit_preferences (void);
void xpdq_edit_preferences_cancel (void);
void xpdq_edit_preferences_okay (void);

pwizard_state *new_pwizard (void);
void pwizard_free (pwizard_state *wizard);
void pwizard_cancel (pwizard_state *wizard);
void pwizard_next (pwizard_state *wizard);
void pwizard_prev (pwizard_state *wizard);
GtkWidget *pwizard_add_create_panel_1 (pwizard_state *wizard);
GtkWidget *pwizard_add_create_panel_2 (pwizard_state *wizard);
GtkWidget *pwizard_add_create_panel_3 (pwizard_state *wizard);
GtkWidget *pwizard_add_create_panel_4 (pwizard_state *wizard);
GtkWidget *pwizard_add_create_panel_5 (pwizard_state *wizard);
GtkWidget *pwizard_add_create_panel_6 (pwizard_state *wizard);
GtkWidget *pwizard_add_create_panel_7 (pwizard_state *wizard);
void pwizard_text_change (GtkWidget *widget, char **text);
void pwizard_driver_selected ( GtkWidget *widget, gint row, 
   gint column, GdkEventButton *event, pwizard_state *wizard ); 
void pwizard_interface_selected ( GtkWidget *widget, gint row, 
   gint column, GdkEventButton *event, pwizard_state *wizard ); 
int pwizard_check_name (pwizard_state *wizard);
int pwizard_check_driver (pwizard_state *wizard);
int pwizard_check_interface (pwizard_state *wizard);
void pwizard_driver_info (pwizard_state *wizard);
void pwizard_interface_info (pwizard_state *wizard);

void xpdq_error (char *message);
void xpdq_error_dismiss (GtkWidget *widget);

void reconcile_printer_list (void); 
void xpdq_add_printer (void);
void xpdq_delete_printer (void);
void xpdq_show_printer_status (void);
void xpdq_set_as_default_printer (void);

extern void (*my_exit) (int code);
void xpdq_exit (int code); 

void set_main_screen (void); 
void job_selection_made( GtkWidget *clist, gint row, gint column, 
   GdkEventButton *event, gpointer data );
void do_menu_popup ( GtkWidget *clist, GdkEventButton *event, 
   gpointer data );
void printer_selection_made( GtkWidget *clist, gint row, gint column, 
   GdkEventButton *event, gpointer data );

void initialize_resources (void); 

void xpdq_set_main_screen (void); 
void xpdq_cancel_print_job (void);
void xpdq_show_job_details (void);
void xpdq_redirect_job (void);
void xpdq_delete_job (void);

int update_job_info (void); 



void show_details_window (void);
void erase_old_details (void);
void add_details (char *text);
void add_red_details (char *text);
void add_bold_details (char *text);
void details_close (void);

void xpdq_about (void);

void printer_selected ( GtkWidget *widget, gint row, 
   gint column, GdkEventButton *event); 

int verify_printer (int row, printer *p, int verbose);
void show_printer_problems (void); 

void job_selected ( GtkWidget *widget, gint row, 
   gint column, GdkEventButton *event); 

void set_status_color ( char *status, GtkWidget *job_list_widget, int row ) ;
void xpdq_spin_value_changed (GtkWidget *widget, GtkSpinButton *spin); 

#endif 
