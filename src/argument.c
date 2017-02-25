/* Copyright 1999, 2000 Jacob A. Langford
 *
 * argument.c
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
#include "lex.h"
#include "parse.h"
#include "argument.h"
#include "util.h"

argument *new_argument (void) {
   argument *a;
   a = malloc (sizeof (argument));
   if (a == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   a->var = NULL;
   a->def_value = NULL;
   a->desc = NULL;
   a->help = NULL;

   return (a);

}

int add_argument_by_rc (parse_state *s, dl_list **l) {

   argument *p;
   parse_state *s2;
   dl_list *ltmp;

   p = new_argument();

   if (  scan_next_block (s) == 0  ) {
       fprintf (stderr, "Block not found after argument\n");
       return (1); 
   }

   s2 = sub_parse_state (s);

  #define THIS p
  #define KEYWORD_PARSE(KEY) \
     (strcmp (s2->token, #KEY) == 0 ) { \
	if (scan_next_block (s2) == 0) { \
	   fprintf (stderr, "Found no block after " #KEY ".\n"); \
	   fprintf (stderr, "Error parsing argument.");\
	   return (1); \
	} \
     str_reset (&THIS->KEY, s2->token); \
  } 

   while ( scan_next_block (s2)  ) {
      if      KEYWORD_PARSE (var)
      else if KEYWORD_PARSE (def_value)
      else if KEYWORD_PARSE (desc)
      else if KEYWORD_PARSE (help)
      else {
         fprintf (stderr, "Unknown keyword \"%s\" parsing argument.\n",
	    s->token);
	 return (1); 
      }
   }

   free_parse_state (s2);

   /* Overwrite this if it already exists */
   ltmp = find_in_list_by_string (*l, p->var);
   if (ltmp == NULL) {
      *l = append_to_list (*l, (void *)p);
   } else {
      free_argument (ltmp->data);
      *l = replace_in_list (*l, ltmp->data, (void *)p);
   }
   return (0);
  
}

void free_argument (argument *p) {
   if (p->var != NULL) free (p->var);
   if (p->def_value != NULL) free (p->def_value);
   if (p->desc != NULL) free (p->desc);
   if (p->help != NULL) free (p->help);
   free (p);
}

char *verify_arguments (dl_list *required, dl_list *l_one, dl_list *l_two) {
   
   dl_list *l, *msg_list;
   int missing_count, have_argument;
   long msg_len;
   item_pair *s;
   char *msg;

   msg_list = NULL;
   msg_len = 0;
   missing_count = 0;
   required = first_list_element (required);
   while (required != NULL) {
      have_argument = 0;
      l = first_list_element (l_one);
      while ( (have_argument == 0) && (l != NULL) ) {
         s = (item_pair *)l->data;
         if (strcmp ( (char *)required->data, s->key) == 0) have_argument = 1;
         l = l->next;
      }
      l = first_list_element (l_two);
      while ( (have_argument == 0) && (l != NULL) ) {
         s = (item_pair *)l->data;
         if (strcmp ( (char *)required->data, s->key) == 0) have_argument = 1;
         l = l->next;
      }
      if (have_argument == 0) {
         msg = malloc (strlen ((char *)required->data) + 50);
	 if (msg == NULL) {
	    fprintf (stderr, "Not enough memory.\n");
	    my_exit (1);
	 }
         sprintf (msg, "Argument %s is required, but not supplied.\n",
	    (char *)required->data);
	 msg_list = append_to_list (msg_list, msg);
	 msg_len += strlen(msg);
         missing_count ++;
      }
      required = required->next;
   }

   if (missing_count) {
      if (msg_len > 0) {
         msg = malloc (msg_len + 1);
	 *msg = 0;
	 if (msg == NULL) {
	    fprintf (stderr, "Not enough memory.\n");
	    my_exit (1);
	 }
	 l = first_list_element (msg_list);
	 while (l != NULL) {
	    strcat (msg, (char *)l->data);
	    free (l->data);
	    l = l->next;
	 }
	 free_list (msg_list);
      }
      return (msg);
   }
   return (NULL);

}

dl_list *copy_argument_list (dl_list *list) {

   return (copy_list (list, (void *)(void *)copy_argument));

}

argument *copy_argument (argument *arg) {

   argument *newarg;

   newarg = new_argument();
   str_reset (&newarg->var, arg->var);
   str_reset (&newarg->def_value, arg->def_value);
   str_reset (&newarg->desc, arg->desc);
   str_reset (&newarg->help, arg->help);

   return (newarg);

}
