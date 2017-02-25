/* Copyright 1999, 2000 Jacob A. Langford
 *
 * parse_rc.c
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

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <glob.h>
#include <string.h>
#include <stdlib.h>
#include <values.h>


#include "parse.h"
#include "lex.h"
#include "parse_rc.h"
#include "util.h"
#include "rc_items.h"
#include "printer.h"
#include "driver.h"
#include "interface.h"

void try_parse_rc_glob (char *rc_glob, rc_items *rc) {

   glob_t globbuf;
   int i, l;
   char *d;

   d = tilde_expand (rc_glob);

   i = glob (d, 0, NULL, &globbuf); 
   free (d);
   if (i != 0) {
      if (i == GLOB_NOMATCH) {
	 /* Keep on going */
      } else {
	 fprintf (stderr, "Died while globbing %s\n", d);
	 my_exit (1);
      }
   } else {
      /* Globbed something real */
      for (i = 0; i < globbuf.gl_pathc; i++) {
          if ( *globbuf.gl_pathv[i] == '.' ) continue;
	  l = strlen (globbuf.gl_pathv[i]);
	  if ( *(globbuf.gl_pathv[i] + l - 1) == '~' ) continue;
	  parse_rc_file (globbuf.gl_pathv[i], rc);  
      }
   }
   globfree (&globbuf);

}
void parse_rc_glob (char *rc_glob, rc_items *rc) {

   glob_t globbuf;
   int i;
   char *d;

   d = tilde_expand (rc_glob);

   i = glob (d, 0, NULL, &globbuf); 
   if (i != 0) {
      if (i == GLOB_NOMATCH) {
	 fprintf (stderr, "No files match %s\n", d);
      } else {
	 fprintf (stderr, "Died while globbing %s\n", d);
      }
      free (d);
      my_exit (1);
   } else {
      free (d);
      /* Globbed something real */
      for (i = 0; i < globbuf.gl_pathc; i++) {
	  parse_rc_file (globbuf.gl_pathv[i], rc);  
      }
   }
   globfree (&globbuf);

}

#define THIS rc
#define KEYWORD_PARSE(KEY) \
  (strcmp (s->token, #KEY) == 0 ) { \
     if (scan_next_block (s) == 0) { \
	fprintf (stderr, "Found no block after " #KEY ".\n"); \
	parse_error (s); \
	my_exit (1); \
     } \
  str_reset (&THIS->KEY, s->token); \
  for (i = 0; i < rc->debug_rc; i++) fprintf (stdout, "  ");\
  if (rc->debug_rc) fprintf (stdout, "setting "#KEY" = %s in %s\n",\
  		s->token, s->file_name);\
} 
#define LONG_INT_PARSE(KEY) \
  (strcmp (s->token, #KEY) == 0 ) { \
     if (scan_next_block (s) == 0) { \
	fprintf (stderr, "Found no block after " #KEY ".\n"); \
	parse_error (s); \
	my_exit (1); \
     } \
  THIS->KEY = strtol(s->token, (char **)NULL, 10); \
  for (i = 0; i < rc->debug_rc; i++) fprintf (stdout, "  ");\
  if (rc->debug_rc) fprintf (stdout, "setting "#KEY" = %s in %s\n",\
  		s->token, s->file_name);\
} 
#define RECURSE_PARSE(KEY, FUNC) \
  (strcmp (s->token, #KEY) == 0 ) { \
     if (scan_next_block (s) == 0) { \
	fprintf (stderr, "Found no block after " #KEY ".\n"); \
	parse_error (s); \
	my_exit (1); \
     } \
     FUNC (s->token, rc); \
} 
#define KEYWORD_TO_LIST_PARSE(KEY,LIST,FUNCTION) \
  (strcmp (s->token, #KEY) == 0 ) { \
     if (FUNCTION (s,&THIS->LIST, rc->debug_rc) != 0) { \
	fprintf (stdout, "Error parsing " #KEY ".\n"); \
	parse_error (s);\
	my_exit (1); \
     } \
} 

void parse_rc_file (char *rc_file, rc_items *rc) {

   parse_state *s;
   int i;

   s = new_parse_state ();
   initialize_parse_state (s, rc_file);


   for (i = 0; i < rc->debug_rc; i++) fprintf (stdout, "  ");
   if (rc->debug_rc) fprintf (stdout, "parsing %s...\n", rc_file);
   if (rc->debug_rc) rc->debug_rc++;
   
   while ( scan_next_block (s) ) {
      if      KEYWORD_PARSE (default_printer)
      else if KEYWORD_PARSE (interface_command_path)
      else if KEYWORD_PARSE (driver_command_path)
      else if KEYWORD_PARSE (interface_command_path)
      else if KEYWORD_PARSE (job_dir)
      else if KEYWORD_TO_LIST_PARSE (printer, printer_list, add_printer_by_rc)
      else if KEYWORD_TO_LIST_PARSE (driver, driver_list, add_driver_by_rc)
      else if KEYWORD_TO_LIST_PARSE (interface, interface_list, 
      	add_interface_by_rc)
      else if LONG_INT_PARSE (max_send_tries)
      else if LONG_INT_PARSE (delay_between_tries)
      else if LONG_INT_PARSE (job_history_duration)
      else if RECURSE_PARSE (include, parse_rc_glob)
      else if RECURSE_PARSE (try_include, try_parse_rc_glob)
      else {
	 parse_error (s);
	 fprintf (stderr, "Unknown keyword \"%s\"\n", s->token);
	 my_exit(1);
      }
   }

   free_parse_state (s);

   if (rc->debug_rc) rc->debug_rc--;

}

void find_rc_locations (char *type, char *name, char *rc_file, 
      dl_list **list) {

   parse_state *s;
   char *rc_start;
   char *punch_in;

   s = new_parse_state ();
   initialize_parse_state (s, rc_file);

#define SKIP_NAMED(KEY) \
  (strcmp (s->token, #KEY) == 0 ) { \
     scan_next_block (s); \
     scan_next_block (s); \
} 
#define SKIP_UNNAMED(KEY) \
  (strcmp (s->token, #KEY) == 0 ) { \
     scan_next_block (s); \
} 
#define RECURSE_ON(KEY) \
  (strcmp (s->token, #KEY) == 0 ) { \
     if (scan_next_block (s) == 0) { \
	fprintf (stderr, "Found no block after " #KEY ".\n"); \
	parse_error (s); \
	my_exit (1); \
     } \
     find_rc_glob_locations (type, name, s->token, list); \
} 

   rc_start  = s->rc_buf_in;
   punch_in = s->rc_buf_in;
   while ( scan_next_block (s) ) {
      if (strcmp (s->token, type) == 0) {
         if (name == NULL) {
	    scan_next_block (s);
	    *list = append_to_list (*list, (void *)  
	           new_rc_loc_filled (rc_file, (punch_in - rc_start),
	           (s->rc_buf_pos - rc_start) ) );
	 } else {
	    scan_next_block (s);
	    if (strcmp (s->token, name) == 0) {
	       scan_next_block (s);
	       *list = append_to_list (*list, (void *)  
		      new_rc_loc_filled (rc_file, (punch_in - rc_start),
		      (s->rc_buf_pos - rc_start) ) );
	    } else {
	       scan_next_block (s);
	    }
	 }
      }
      else if SKIP_UNNAMED (default_printer)
      else if SKIP_UNNAMED (interface_command_path)
      else if SKIP_UNNAMED (driver_command_path)
      else if SKIP_UNNAMED (interface_command_path)
      else if SKIP_UNNAMED (job_dir)
      else if SKIP_NAMED (printer)
      else if SKIP_NAMED (driver)
      else if SKIP_NAMED (interface)
      else if SKIP_UNNAMED (max_send_tries)
      else if SKIP_UNNAMED (delay_between_tries)
      else if SKIP_UNNAMED (job_history_duration)
      else if RECURSE_ON (include)
      else if RECURSE_ON (try_include)
      punch_in = s->rc_buf_pos;
   }

   free_parse_state (s);

}

void find_rc_glob_locations (char *type, char *name, char *rc_glob, 
      dl_list **list) {

   glob_t globbuf;
   int i;
   char *d;

   d = tilde_expand (rc_glob);

   i = glob (d, 0, NULL, &globbuf); 
   free (d);
   if (i != 0) {
      /* Keep on going */
   } else {
      /* Globbed something real */
      for (i = 0; i < globbuf.gl_pathc; i++) {
	  find_rc_locations (type, name, globbuf.gl_pathv[i], list);  
      }
   }
   globfree (&globbuf);

}

void rc_set (char *type, char *name, char *buf) {

   dl_list *l, *rc_locs;
   int set_flag;
   int trouble_flag;

   rc_locs = NULL;
   find_rc_glob_locations (type, name, PRINTRC_FILE,  &rc_locs);
   find_rc_glob_locations (type, name, "~/.printrc",  &rc_locs);

   set_flag = 0;
   trouble_flag = 0;
   l = first_list_element (rc_locs);
   while (l != NULL) {
      if (set_flag == 0) {
         /* Try to set the item here. */
         if (0 == replace_in_rc_file ( (rc_location *)l->data,   buf)  ) {
            set_flag = 1;
         }
      } else {
         if (0 != delete_in_rc_file ( (rc_location *)l->data)  ) {
            trouble_flag = 1;   /* This means that a later occurence
	                         * could not be deleted and will overshadow
				 * the earlier successful write 
				 */
         }
      }
      l = l->next;
   }

   if (  (set_flag == 0) || (trouble_flag == 1)  ) {
      if (0 != append_to_rc_file (PRINTRC_FILE, buf) ) {
         append_to_rc_file ("~/.printrc", buf);  /* Return code not checked.
	                                          * What else can we do?
						  */
      }
   }
   
   list_iterate (rc_locs, (void *)(void *)free_rc_location);
   free_list (rc_locs);

}
void rc_delete (char *type, char *name, char *buf) {

   dl_list *l, *rc_locs;
   int trouble_flag;

   rc_locs = NULL;
   find_rc_glob_locations (type, name, PRINTRC_FILE,  &rc_locs);
   find_rc_glob_locations (type, name, "~/.printrc",  &rc_locs);

   trouble_flag = 0;
   l = first_list_element (rc_locs);
   while (l != NULL) {
      if (0 != delete_in_rc_file ( (rc_location *)l->data)  ) {
	 trouble_flag = 1;   /* This means that at least one occurence
			      * could not be deleted.
			      */
      }
      l = l->next;
   }

   /* In this case, buf should have been set to some commands that
    * indicate to delete the object.  For example, say printer foo is
    * defined in /etc/printrc.  Ordinary  users cannot delete it, but
    * they may add the command "printer foo delete" to their own rc file.
    */
   if (trouble_flag == 1)  {
      if (0 != append_to_rc_file (PRINTRC_FILE, buf) ) {
         append_to_rc_file ("~/.printrc", buf);  /* Return code not checked.
	                                          * What else can we do?
						  */
      }
   }
   
   list_iterate (rc_locs, (void *)(void *)free_rc_location);
   free_list (rc_locs);

}
