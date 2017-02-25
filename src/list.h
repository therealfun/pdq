/* Copyright 1999, 2000 Jacob A. Langford
 *
 * list.h
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

#ifndef __LIST_H
#define __LIST_H

typedef struct _dl_list dl_list;

struct _dl_list {
   void *data;
   dl_list *next;
   dl_list *prev;
};

dl_list *new_list 		(void);
dl_list *first_list_element 	(dl_list *list);
dl_list *last_list_element 	(dl_list *list);
dl_list *join_lists 	     	(dl_list *list1, dl_list *list2);
dl_list *append_to_list 	(dl_list *list, void *data);
dl_list *prepend_to_list 	(dl_list *list, void *data);
dl_list *remove_from_list 	(dl_list *list, void *data);
dl_list *find_in_list 		(dl_list *list, void *data);
dl_list *find_in_list_by_string	(dl_list *list, char *s);
dl_list *find_in_list_by_int	(dl_list *list, int i);
dl_list *replace_in_list 	(dl_list *list, void *data_out, void *data_in);
dl_list *remove_list_link 	(dl_list *link);
dl_list *free_list 		(dl_list *list);
dl_list *copy_list 		(dl_list *list, void *(*copy) (void *data) );
void     list_iterate 		(dl_list *list, void (*func) (void *data) );


extern void (*my_exit) (int code);

#endif 
