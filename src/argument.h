/* Copyright 1999, 2000 Jacob A. Langford
 *
 * argument.h
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

#ifndef __ARGUMENT_H_
#define __ARGUMENT_H_

typedef struct _argument argument;
struct _argument {
   char *var;           /* Basically this stuff is all just a place        */
   char *def_value;     /* to stuff hints for programs that auto-generate  */
   char *desc;          /* config files.  The default value might have     */
   char *help;          /*                                                 */
};


argument *new_argument (void);
int add_argument_by_rc (parse_state *s, dl_list **l);
void free_argument (argument *p);

char *verify_arguments (dl_list *required, dl_list *l_one, dl_list *l_two);

dl_list *copy_argument_list (dl_list *list);
argument *copy_argument (argument *a);

extern void (*my_exit) (int code);

#endif
