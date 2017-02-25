/* Copyright 1999, 2000 Jacob A. Langford
 *
 * util.h
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

#ifndef __UTIL_H
#define __UTIL_H

#include "parse.h" 
#include "list.h" 

typedef struct _item_pair item_pair;
struct _item_pair {
   char *key;
   char *value;
};

typedef struct _pointer_pair pointer_pair;
struct _pointer_pair {
   void *a;
   void *b;
};

item_pair *new_item_pair (void);
item_pair *copy_item_pair (item_pair *i);
char *copy_item (char *i);
pointer_pair *new_pointer_pair (void);
int add_item_pairs_by_rc (parse_state *s, dl_list **l);
void free_item_pair (item_pair *p);
int add_items_by_rc (parse_state *s, dl_list **l);
int add_items_by_string (char *s, dl_list **l);
int add_item_pairs_by_string (char *s, dl_list **l);

char *tilde_expand (const char *s); 
char *escape_block (char *s, char b_on, char b_off);
void str_set (char **dest, char *src);
void str_reset (char **dest, char *src);

extern void (*my_exit) (int code);
int verify_executable_in_path (char *prog, char *path);
char *item_pair_list_to_str (dl_list *list); 
char *item_list_to_str (dl_list *list); 

void compress_whitespace (char **buf);

char *str_match (const char *haystack, const char *needle); 

#endif 
