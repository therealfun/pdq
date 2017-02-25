/* Copyright 1999, 2000 Jacob A. Langford
 *
 * parse_rc.h
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

#ifndef __PARSE_RC_h
#define __PARSE_RC_h

#include "rc_items.h"

void parse_rc_file (char *rc_file, rc_items *rc); 
void parse_rc_glob (char *rc_glob, rc_items *rc); 
void try_parse_rc_glob (char *rc_glob, rc_items *rc); 

void find_rc_glob_locations (char *type, char *name, char *rc_file, 
      dl_list **list); 
void find_rc_locations (char *type, char *name, char *rc_glob, 
      dl_list **list); 

void rc_set (char *type, char *name, char *buf); 
void rc_delete (char *type, char *name, char *buf); 

extern void (*my_exit) (int code);

#endif 
