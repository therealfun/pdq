/* Copyright 1999, 2000 Jacob A. Langford
 *
 * list.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "list.h"


dl_list *new_list (void) {

   dl_list *list;
   list = malloc (sizeof (dl_list));
   if (list == NULL) {
      fprintf (stderr, "Not enough memory");
      my_exit (1);
   }
   list->prev = NULL;
   list->next = NULL;
   list->data = NULL;
   return (list);

}

dl_list *free_list (dl_list *list) {

   while (list != NULL) {
      list = remove_list_link (list); 
   }
   return (NULL);

}

dl_list *prepend_to_list (dl_list *list, void *data) {

   dl_list *new_link;

   new_link = new_list(); 
   new_link->data = data;
   if (list == NULL) {
      new_link -> data = data; 
   } else {
      new_link->next = first_list_element (list);
      new_link->next->prev = new_link;
   }
   return (new_link);


}
dl_list *append_to_list (dl_list *list, void *data) {

   dl_list *new_link;

   new_link = new_list(); 
   new_link->data = data;
   if (list == NULL) {
      new_link -> data = data; 
   } else {
      new_link->prev = last_list_element (list);
      new_link->prev->next = new_link;
   }
   return (new_link);


}

dl_list *remove_from_list (dl_list *list, void *data) {

   dl_list *link, *out;

   link = find_in_list (list, data);
   if (link == NULL) {
      out = list;
   } else {
      out = remove_list_link (link);
   }
   return (out);

}

dl_list *remove_list_link (dl_list *link) {

   dl_list *out;

   if (link == NULL) {
      return (NULL);
   }

   if (link->prev != NULL) {
      out = link->prev;
   } else if (link->next != NULL) {
      out = link->next;
   } else {
      out = NULL;
   }

   if (link->prev != NULL)   link->prev->next = link->next;
   if (link->next != NULL)   link->next->prev = link->prev;

   free (link);

   return (out);

}

dl_list *first_list_element (dl_list *list) {

   if (list == NULL) {
      return (NULL);
   }

   while (list->prev != NULL) {
      list = list->prev;
   }

   return (list);

}

dl_list *last_list_element (dl_list *list) {

   if (list == NULL) {
      return (NULL);
   }

   while (list->next != NULL) {
      list = list->next;
   }

   return (list);

}

dl_list *find_in_list (dl_list *list, void *data) {

   if (list == NULL) {
      return (NULL);
   }

   list = first_list_element (list);
   while (  (list != NULL) && (list->data != data) ) {
      list = list->next;
   }
   return (list);

}

dl_list *replace_in_list (dl_list *list, void *data_out, void *data_in) {

   if (list == NULL) {
      return (NULL);
   }

   list = find_in_list (list, data_out);
   if (list == NULL) {
      return (NULL);
   } else {
      list->data = data_in;
   }

   return (list);

}

void list_iterate (dl_list *list, void (*func) (void *data) ) {

   list = first_list_element (list); 

   while (list != NULL) {
      func ( list->data );
      list = list->next;
   }

}

dl_list *find_in_list_by_string	(dl_list *list, char *s) {


   if (list == NULL) return (NULL);

   list = first_list_element (list);

   /* I have assumed that a pointer to a structure points to
    * its first element.  This is an ANSI standard.
    */

   while (  (list != NULL) && (strcmp( *(char **)(list->data), s)) ) {
      list = list->next;
   }

   return (list);

}

dl_list *join_lists  (dl_list *list1, dl_list *list2) {

   if (list1 == NULL) return (list2);
   if (list2 == NULL) return (list1);

   list1 = last_list_element (list1); 
   list2 = first_list_element (list2);

   list1->next = list2;
   list2->prev = list1;

   return (list1);

}

dl_list *find_in_list_by_int	(dl_list *list, int i) {


   if (list == NULL) return (NULL);

   list = first_list_element (list);

   /* I have assumed that a pointer to a structure points to
    * its first element.  This is an ANSI standard.
    */

   while (  (list != NULL) && (i != *((int *)list->data)) ) {
      list = list->next;
   }

   return (list);

}

dl_list *copy_list (dl_list *list, void *(*func) (void *data) ) {

   dl_list *l;

   l = NULL;

   list = first_list_element (list);
   while (list != NULL) {
      l = append_to_list (l, func (list->data) );
      list = list->next;
   }

   return (l);

}
