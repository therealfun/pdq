/* Copyright 1999, 2000 Jacob A. Langford
 *
 * util.c
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

#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>

#include "util.h"


char *tilde_expand (const char *s) {

   struct passwd *pw;
   char *c, *d;

   /* Only expand ~ in first slot */

   if (s == NULL) return (NULL);

   c = strchr (s, '~');
   if (c == s) { /* ~ found in first slot */
      c = strchr (s, '/');
      if (c == (s + 1)) { /* ~/ found */
	 pw = getpwuid (getuid());
	 if (pw == NULL) {
	    fprintf ( stderr, "Error expanding %s\n", s );
	    perror (NULL);
	    my_exit (1);
	 }
      } else { /* ~username found */
         *c = 0;
	 pw = getpwnam (s+1);
	 if (pw == NULL) {
	    fprintf ( stderr, "Error expanding %s\n", s );
	    perror (NULL);
	    my_exit (1);
	 }
	 *c = '/';
      }
      if (  (d = malloc ( strlen(c)+strlen(pw->pw_dir) + 1 )) == NULL ) {
         fprintf (stderr, "Not enough memory");
	 my_exit (1);
      }
      sprintf (d, "%s%s", pw->pw_dir, c);
   } else {
       d = malloc ( strlen(s) + 1 );
       if (d == NULL) {
          fprintf (stderr, "Not enough memory");
	  my_exit (1);
       }
       strcpy (d, s);
   }

   return (d);
}

char *escape_block (char *s, char b_on, char b_off) {

   int i;
   char *c, *out;

   if (s == NULL) {
      out = malloc (3);
      out[0] = b_on;
      out[1] = b_off;
      out[2] = 0;
      return (out);
   }
   i=0;
   for (c=s; (c=strchr(c,b_on)) != NULL; c++ ) i++;
   for (c=s; (c=strchr(c,b_off)) != NULL; c++ ) i++;

   out = malloc (i + 3 + strlen(s));
   if (out == NULL) {
      fprintf (stderr, "Not enough memory");
      my_exit (1);
   }
   c = out;

   *(c++) = b_on;
   while (*s != 0) {
      if ( (*s == b_on) || (*s == b_off) ) *(c++) = '\\'; 
      *(c++) = *(s++);
   }
   *(c++) = b_off;
   *c = 0;
   
   return (out);

}

void str_reset (char **dest, char *src) {
   
   if (*dest != NULL) {
      free (*dest);
   }
   str_set (dest, src);

}
void str_set (char **dest, char *src) {
   
   if (src == NULL) {
      *dest = NULL;
      return;
   }
   *dest = malloc (strlen(src) + 1);
   if (*dest == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   strcpy (*dest, src);

}

item_pair *new_item_pair (void) {
   item_pair *p;
   p = malloc (sizeof (item_pair));
   if (p == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit(1);
   }
   p->key = NULL;
   p->value = NULL;
   return (p);
}

pointer_pair *new_pointer_pair (void) {
   pointer_pair *p;
   p = malloc (sizeof (pointer_pair));
   if (p == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit(1);
   }
   p->a = NULL;
   p->b = NULL;
   return (p);
}

int add_item_pairs_by_rc (parse_state *s, dl_list **l) {

   parse_state *s2;
   item_pair *ip;
   dl_list *ltmp;

   if ( scan_next_block (s) == 0 ) {
      fprintf (stderr, "No key = value pairs found.\n");
      return (1);
   }

   s2 = sub_parse_state (s);
   while (  scan_next_block (s2) ) {
      ip = new_item_pair();
      str_reset (&ip->key, s2->token);
      if ( scan_next_block (s2) == 0 ) {
         fprintf (stderr, "Found no value for key %s\n", ip->key);
	 return (1);
      }
      str_reset (&ip->value, s2->token);

      /* Overwrite this if it already exists */
      ltmp = find_in_list_by_string (*l, ip->key);
      if (ltmp == NULL) {
	 *l = append_to_list (*l, (void *)ip);
      } else {
	 free_item_pair (ltmp->data);
	 *l = replace_in_list (*l, ltmp->data, (void *)ip);
      }
      
   }

   free_parse_state (s2);

   return (0);

}

void free_item_pair (item_pair *p) {
   if (p->key != NULL) free (p->key);
   if (p->value != NULL) free (p->value);
   free (p);
}

int add_items_by_rc (parse_state *s, dl_list **l) {

   parse_state *s2;
   char *buf;

   if ( scan_next_block (s) == 0 ) {
      fprintf (stderr, "No items found.\n");
      return (1);
   }

   s2 = sub_parse_state (s);
   while (  scan_next_block (s2) ) {
      str_set (&buf, s2->token);
      *l = append_to_list (*l, (void *)buf);
   }

   free_parse_state (s2);

   return (0);

}

int add_items_by_string (char *s, dl_list **l) {

   char *pc;
   char *c;

   while ( (pc = strrchr (s, ',')) != NULL ) {
      str_set (&c, pc + 1);
      *l = append_to_list (*l, c);
      *pc = 0;
   }
   str_set (&c, s);

   *l = append_to_list (*l, c);

   return (0);

}

int add_item_pairs_by_string (char *s, dl_list **l) {

   item_pair *ip;
   char *pc, *pc2;
   

   while ( (pc = strrchr (s, ',')) != NULL ) {
      if ( (pc2 = strchr (s, '=')) == NULL ) {
         fprintf (stderr, "Must supply arguments as KEY=VALUE.\n");
	 return (1);
      }
      ip = new_item_pair();
      str_reset (&ip->value, pc2 + 1);
      *pc2 = 0;
      str_reset (&ip->key, pc + 1);
      *pc = 0;

      *l = append_to_list (*l, ip);
   }

   if ( (pc2 = strchr (s, '=')) == NULL ) {
      fprintf (stderr, "Must supply arguments as KEY=VALUE.\n");
      return (1);
   }
   ip = new_item_pair();
   str_reset (&ip->value, pc2 + 1);
   *pc2 = 0;
   str_reset (&ip->key, s);

   *l = append_to_list (*l, ip);

   return (0);

}

int verify_executable_in_path (char *prog, char *path) {

   struct stat fd_info;
   char *c, *d, *e, *f;
   int i;
   uid_t my_uid;

   my_uid = getuid();

   str_set (&c, path);

   while (  (d = strrchr (c, ':')) != NULL ) {
      *(d++) = 0;
      if (*d == 0) continue;  /* Skip null path entries */
      f = tilde_expand (d);
      e = malloc ( strlen(prog) + strlen(f) + 2 );
      if (e == NULL) {
         fprintf (stderr, "Not enough memory\n");
	 my_exit (1);
      }
      sprintf (e, "%s/%s", f, prog);
      free (f);
      i = stat (e, &fd_info);
      if (    (i == 0) &&     /* Group permissions not checked */
              (   (   (fd_info.st_uid == my_uid) && 
	              (fd_info.st_mode & S_IXUSR) &&
	              (fd_info.st_mode & S_IRUSR)      )    ||
                  (   (fd_info.st_uid != my_uid) &&
	              (S_IXOTH & fd_info.st_mode) && 
	              (S_IROTH & fd_info.st_mode)      )       )     )  {
         free (c);
         free (e);
         return (0);
      }
      free (e);
   }

   f = tilde_expand (c);
   e = malloc ( strlen(prog) + strlen(f) + 2 );
   if (e == NULL) {
      fprintf (stderr, "Not enough memory\n");
      my_exit(1);
   }
   sprintf (e, "%s/%s", f, prog);
   free (f);
   i = stat (e, &fd_info);
   if (    (i == 0) &&     /* Group permissions not checked */
	   (   (   (fd_info.st_uid == my_uid) && 
		   (fd_info.st_mode & S_IXUSR) &&
		   (fd_info.st_mode & S_IRUSR)      )    ||
	       (   (fd_info.st_uid != my_uid) &&
		   (S_IXOTH & fd_info.st_mode) && 
		   (S_IROTH & fd_info.st_mode)      )       )     )  {
      free (c);
      free (e);
      return (0);
   }

   free (c);
   free (e);
   return (1);

}

char *item_pair_list_to_str (dl_list *list) { 

   dl_list *l;
   item_pair *i;
   char *buf, *out;
   int len;

   len = 0;
   l = first_list_element (list);
   while (l != NULL) {
      i = (item_pair *)l->data;
      buf = escape_block (i->key, '"', '"');
      len += strlen(buf);
      free (buf);
      buf = escape_block (i->value, '"', '"');
      len += strlen(buf);
      free (buf);
      len += 10;
      l = l->next;
   }

   out = malloc (len  + 2);
   if (out == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }

   l = first_list_element (list);
   if (l == NULL) {
      out[0] = ' ';
      out[1] = 0;
      return (out); 
   } else {
      i = (item_pair *)l->data;
      buf = escape_block (i->key, '"', '"');
      strcpy (out, buf);
      free (buf);
      strcat (out, " = ");
      buf = escape_block (i->value, '"', '"');
      strcat (out, buf);
      free (buf);
      l = l->next;
   }
   while (l != NULL) {
      i = (item_pair *)l->data;
      strcat (out, ", ");
      buf = escape_block (i->key, '"', '"');
      strcat (out, buf);
      free (buf);
      strcat (out, " = ");
      buf = escape_block (i->value, '"', '"');
      strcat (out, buf);
      free (buf);
      l = l->next;
   }

   return (out);

}

char *item_list_to_str (dl_list *list) { 

   dl_list *l;
   char *i, *out;
   char *buf;
   int len;

   len = 0;
   l = first_list_element (list);
   while (l != NULL) {
      i = (char *)l->data;
      buf = escape_block (i, '"', '"');
      len += strlen(buf);
      free (buf);
      len += 2;
      l = l->next;
   }

   out = malloc (len  +  2);
   if (out == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }

   l = first_list_element (list);
   if (l == NULL) {
      out[0] = ' ';
      out[1] = 0;
      return (out); 
   } else {
      i = (char *)l->data;
      buf = escape_block (i, '"', '"');
      strcpy (out, buf);
      free (buf);
      l = l->next;
   }
   while (l != NULL) {
      i = (char *)l->data;
      strcat (out, ", ");
      buf = escape_block (i, '"', '"');
      strcat (out, buf);
      free (buf);
      l = l->next;
   }
   return (out);

}

void compress_whitespace (char **buf) {

   char *wpos, *rpos;
   int space;

   if (*buf == NULL) return;

   rpos = *buf;
   wpos = *buf;

   /* Copy everything, unless it is whitespace and there
    * is already a space.  Also translate tabs and newlines into
    * spaces.  Also replace \n and \t with newlines and tabs.
    */

   space = 0;
   while (*rpos != 0) {
      if ( (*rpos == ' ') || (*rpos == '\n') || (*rpos == '\t') ) {
         if (space == 1) {
	    rpos++;
	 } else {
            *wpos = ' ';
	    space = 1;
	    rpos++;
	    wpos++;
	 }
      } else {
         if ((*rpos == '\\') && (*(rpos+1) == 'n') ) {
	    *wpos = '\n';
	    space = 0;
	    rpos+=2;
	    wpos++;
	 } else if ((*rpos == '\\') && (*(rpos+1) == 't') ) {
	    *wpos = '\t';
	    space = 0;
	    rpos+=2;
	    wpos++;
	 } else {
	    *wpos = *rpos;
	    space = 0;
	    rpos++;
	    wpos++;
         }
      }
   }
   *wpos = 0;
}

item_pair *copy_item_pair (item_pair *i) {
  
   item_pair *out;

   out = new_item_pair();
   str_reset (&out->key, i->key);
   str_reset (&out->value, i->value);

   return (out);

}

char *copy_item (char *i) {

   char *out;

   str_set (&out, i);

   return (out);

}



char *str_match (const char *haystack, const char *needle) {

   /* Like strstr(), but 0x20,  ASCII case bit is not compared */

   const char *n, *h;

   while  ( *haystack != 0) {
      h = haystack;
      n = needle;
      while (  (*n | 0x20) == (*h | 0x20)  ) {
	 if (*(++n) == 0) return haystack;  /* Matched everything */
	 if (*(++h) == 0) return 0;         /* Haystack is empty */
      }
      haystack++;
   }

   return 0; /* no match */

}
