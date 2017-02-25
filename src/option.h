/* Copyright 1999, 2000 Jacob A. Langford
 *
 * option.h
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

#ifndef __OPTION_H
#define __OPTION_H

#include "list.h"
#include "parse.h"

typedef struct _option option;
struct _option {
   char *var;
   char *desc; 
   char *default_choice;
   dl_list *choice_list;
}; 

typedef struct _choice choice;
struct _choice {
   char *name; 
   char *desc; 
   char *value;
   char *help;
}; 

option *new_option (void);
int add_option_by_rc (parse_state *s, dl_list **l);
void free_option ( option *o );

choice *new_choice (void);
int new_choice_by_rc (parse_state *s, dl_list **l);
void free_choice ( choice *c );

int add_option_by_rc (parse_state *s, dl_list **l);
int add_choice_by_rc (parse_state *s, dl_list **l);

extern void (*my_exit) (int code);

dl_list *copy_option_list (dl_list *list); 
dl_list *copy_choice_list (dl_list *list); 
option *copy_option (option *opt);
choice *copy_choice (choice *ch);

#endif 
