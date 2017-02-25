/* Copyright 1999, 2000 Jacob A. Langford
 *
 * rc_items.h
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

#ifndef __RC_ITEMS_H_
#define __RC_ITEMS_H_

#include "list.h"

typedef struct _rc_items rc_items;
struct _rc_items {
   dl_list *printer_list;
   dl_list *driver_list;
   dl_list *interface_list;
   char *default_printer;
   char *interface_command_path;
   char *driver_command_path;
   char *job_dir;
   long max_send_tries;
   long delay_between_tries;
   long job_history_duration;
   int debug_rc;
};

extern void (*my_exit) (int code);

rc_items *new_rc_items (void);
void free_rc_items (rc_items *rc);
#endif
