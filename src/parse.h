/* Copyright 1999, 2000 Jacob A. Langford
 *
 * parse.h
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

#ifndef __PARSE_H_
#define __PARSE_H_

#include <sys/types.h>

typedef struct parse_state_ parse_state;
struct parse_state_ {
   char *rc_buf_in;
   char *work_in;
   char *rc_buf_pos;
   char *token;
   size_t token_len;
   long *line;
   long *line_in;
   char *file_name;
   int malloced;
};

typedef struct rc_location_ rc_location;
struct rc_location_ {
   char *rc_filename;
   size_t punch_in;
   size_t punch_out;
};

parse_state *new_parse_state (void);
parse_state *sub_parse_state (parse_state *s);

int scan_next_block (parse_state *s); 

void free_parse_state (parse_state *s);
void initialize_parse_state (parse_state *s, char *file); 
int try_initialize_parse_state (parse_state *s, char *file); 
void parse_error (parse_state *s);

rc_location *new_rc_location (void);
rc_location *new_rc_loc_filled (char *rc_filename, 
   size_t punch_in, size_t punch_out);
void free_rc_location (rc_location *rc_loc);

int append_to_rc_file (char *rc_file, char *buf); 
int replace_in_rc_file (rc_location *rc_loc, char *buf); 
int delete_in_rc_file (rc_location *rc_loc); 

extern void (*my_exit) (int code);

#endif

