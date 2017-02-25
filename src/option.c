/* Copyright 1999, 2000 Jacob A. Langford
 *
 * option.c
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

#include "option.h"
#include "lex.h"
#include "parse.h"
#include "util.h"


option *new_option (void) {

   option *o;
  
   o = malloc (sizeof(option));
   if (o == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   o->var = NULL;
   o->desc = NULL;
   o->default_choice = NULL;
   o->choice_list = NULL;

   return (o);

}

void free_option (option *o) {
   if (o->desc != NULL) { free (o->desc); }
   if (o->var != NULL) { free (o->var); }
   if (o->default_choice != NULL) { free (o->default_choice); }

   list_iterate (o->choice_list, (void *)(void *)free_choice);
   o->choice_list = free_list (o->choice_list);

   free (o);
}


choice *new_choice (void)  {
   choice *ch;
   ch = malloc (sizeof(choice));
   if (ch == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   ch->name = NULL;
   ch->desc = NULL;
   ch->value = NULL;
   ch->help = NULL;

   return (ch);

}

int add_option_by_rc (parse_state *s, dl_list **l) {

   option *p;
   parse_state *s2;
   dl_list *ltmp;

   p = new_option();

   if (  scan_next_block (s) == 0  ) {
       fprintf (stderr, "Block not found after option.\n");
       return (1); 
   }

   s2 = sub_parse_state (s);

  #define THIS p
  #define KEYWORD_PARSE(KEY) \
     (strcmp (s2->token, #KEY) == 0 ) { \
	if (scan_next_block (s2) == 0) { \
	   fprintf (stderr, "Found no block after " #KEY ".\n"); \
	   fprintf (stderr, "Error parsing option.\n");\
	   return (1); \
	} \
     str_reset (&THIS->KEY, s2->token); \
  } 
  #define KEYWORD_TO_LIST_PARSE(KEY,LIST,FUNCTION) \
     (strcmp (s2->token, #KEY) == 0 ) { \
	if (FUNCTION (s2,&THIS->LIST) != 0) { \
	   fprintf (stderr, "Error parsing " #KEY ".\n"); \
	   fprintf (stderr, "Error parsing option.\n");\
	   return (1); \
	} \
  } 


   while ( scan_next_block (s2)  ) {
      if      KEYWORD_PARSE (var)
      else if KEYWORD_PARSE (desc)
      else if KEYWORD_PARSE (default_choice)
      else if KEYWORD_TO_LIST_PARSE (choice, choice_list, add_choice_by_rc)
      else {
         fprintf (stderr, "Unknown keyword \"%s\" parsing option.\n",
	 	s2->token);
	 return (1); 
      }
   }

   free_parse_state (s2);

   if (  (p->choice_list == NULL) || (p->var == NULL)   ) {
      fprintf (stderr, 
         "An option must specify a var and at least one choice.\n");
      return (1);
   }
  
   /* Overwrite this if it already exists */
   ltmp = find_in_list_by_string (*l, p->var);
   if (ltmp == NULL) {
      *l = append_to_list (*l, (void *)p);
   } else {
      free_option (ltmp->data);
      *l = replace_in_list (*l, ltmp->data, (void *)p);
   }
   return (0);
  
}

int add_choice_by_rc (parse_state *s, dl_list **l) {

   choice *p;
   parse_state *s2;
   dl_list *ltmp;

   p = new_choice();

   if (  scan_next_block (s) == 0  ) {
       fprintf (stderr, "Choice name not found\n");
       return (1); 
   }
   str_reset (&p->name, s->token);

   if (  scan_next_block (s) == 0  ) {
       fprintf (stderr, "Block not found after choice %s\n", p->name);
       return (1); 
   }

   s2 = sub_parse_state (s);

  #undef THIS
  #undef KEYWORD_PARSE
  #undef KEYWORD_TO_LIST_PARSE
  #define THIS p
  #define KEYWORD_PARSE(KEY) \
     (strcmp (s2->token, #KEY) == 0 ) { \
	if (scan_next_block (s2) == 0) { \
	   fprintf (stderr, "Found no block after " #KEY ".\n"); \
	   fprintf (stderr, "Error parsing choice %s\n", THIS->name);\
	   return (1); \
	} \
     str_reset (&THIS->KEY, s2->token); \
  } 
  #define KEYWORD_TO_LIST_PARSE(KEY,LIST,FUNCTION) \
     (strcmp (s2->token, #KEY) == 0 ) { \
	if (FUNCTION (s2,&THIS->LIST) != 0) { \
	   fprintf (stderr, "Error parsing " #KEY ".\n"); \
	   fprintf (stderr, "Error parsing choice %s\n", THIS->name);\
	   return (1); \
	} \
  } 


   while ( scan_next_block (s2)  ) {
      if      KEYWORD_PARSE (desc)
      else if KEYWORD_PARSE (value)
      else if KEYWORD_PARSE (help)
      else {
         fprintf (stderr, "Unknown keyword \"%s\" parsing choice %s\n", 
	    s2->token, p->name);
	 return (1); 
      }
   }

   free_parse_state (s2);

   /* Overwrite this if it already exists */
   ltmp = find_in_list_by_string (*l, p->name);
   if (ltmp == NULL) {
      *l = append_to_list (*l, (void *)p);
   } else {
      free_choice (ltmp->data);
      *l = replace_in_list (*l, ltmp->data, (void *)p);
   }
   return (0);
  
}

void free_choice (choice *ch) {
   if (ch->name != NULL) { free (ch->name); }
   if (ch->desc != NULL) { free (ch->desc); }
   if (ch->value != NULL) { free (ch->value); }
   if (ch->help != NULL) { free (ch->help); }
   free (ch);
}

dl_list *copy_option_list (dl_list *list) {

   return (copy_list (list, (void *)(void *)copy_option));

}


option *copy_option (option *opt) {

   option *newopt;

   newopt = new_option();
   str_reset (&newopt->var, opt->var);
   str_reset (&newopt->default_choice, opt->default_choice);
   str_reset (&newopt->desc, opt->desc);
   newopt->choice_list = copy_choice_list (opt->choice_list);

   return (newopt);

}

dl_list *copy_choice_list (dl_list *list) {
  
    return (copy_list (list, (void *)(void *)copy_choice));

}

choice *copy_choice (choice *ch) {

   choice *newch;

   newch = new_choice();
   str_reset (&newch->name, ch->name);
   str_reset (&newch->value, ch->value);
   str_reset (&newch->desc, ch->desc);
   str_reset (&newch->help, ch->help);

   return (newch);

}
