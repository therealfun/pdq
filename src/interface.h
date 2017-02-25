/* Copyright 1999, 2000 Jacob A. Langford
 *
 * interface.h
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

#ifndef __INTERFACE_H_
#define __INTERFACE_H_

#include "list.h"
#include "parse.h"
#include "rc_items.h"

typedef struct _interface interface;
struct _interface {
   char *name;
   char *verify_exec;
   char *send_exec;
   char *status_exec;
   char *cancel_exec;
   char *help;
   char *incompatibility_msg;
   int compatibility;
   dl_list *required_arg_list;
   dl_list *arg_list;
   dl_list *option_list;
   dl_list *default_option_list;
   dl_list *requirement_list;
};

interface *new_interface(void);
int add_interface_by_rc (parse_state *s, dl_list **l, int debug_rc);
void free_interface (interface *i);
interface *copy_interface (interface *i);

void check_interface_compatibility (interface *d, rc_items *rc); 

extern void (*my_exit) (int code);

#endif
