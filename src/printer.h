/* Copyright 1999, 2000 Jacob A. Langford
 *
 * printer.h
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

#ifndef __PRINTER_H
#define __PRINTER_H

#include "list.h"
#include "parse.h"
#include "rc_items.h"

typedef struct _printer printer;
struct _printer {
   char *name;
   char *interface;
   char *driver;
   char *model;
   char *location;
   dl_list *driver_option_list;
   dl_list *driver_arg_list;
   dl_list *interface_arg_list;
   dl_list *interface_option_list;
};

int add_printer_by_rc (parse_state *s, dl_list **l, int rc_debug);
printer *new_printer (void);
printer *copy_printer (printer *p);
void free_printer (printer *p);
char *printer_to_str (printer *p);
void set_printer (printer *p);
void delete_printer (char *name);

char *get_printer_status (printer *p, rc_items *rc);

extern void (*my_exit) (int code);

#endif 
