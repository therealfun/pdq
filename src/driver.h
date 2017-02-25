/* Copyright 1999, 2000 Jacob A. Langford
 *
 * driver.h
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

#ifndef __DRIVER_H
#define __DRIVER_H

#include "list.h"
#include "parse.h"
#include "rc_items.h"

typedef struct _driver driver;
struct _driver {
   char *name; 
   char *filetype_exec;
   char *verify_exec;
   char *filter_exec;
   char *help;
   char *incompatibility_msg;
   int compatibility;
   dl_list *default_option_list;
   dl_list *option_list;
   dl_list *required_arg_list;
   dl_list *arg_list;
   dl_list *requirement_list;
   dl_list *language_driver_list;
}; 

typedef struct _language_driver language_driver;
struct _language_driver {
   char *name; 
   char *filetype_regx;
   char *convert_exec;
}; 

driver *new_driver (void);
int add_driver_by_rc (parse_state *s, dl_list **l, int rc_debug);
driver *copy_driver ( driver *d );
void free_driver ( driver *d );

language_driver *new_language_driver (void);
int add_language_driver_by_rc (parse_state *s, dl_list **l);
void free_language_driver ( language_driver *d );
language_driver *copy_language_driver ( language_driver *d );


void show_driver (driver *d);

void check_driver_compatibility (driver *d, rc_items *rc); 


extern void (*my_exit) (int code);

#endif
