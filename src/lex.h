/* Copyright 1999, 2000 Jacob A. Langford
 *
 * lex.h
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

#ifndef __LEX_H
#define __LEX_H

#include <sys/types.h> 

size_t next_word (char **buf, char *token, long *line, long *line_in) ;
size_t next_nonwhite (char **buf, char *token, long *line, long *line_in) ;
size_t next_block (char **buf, char *token, long *line, long *line_in) ;


#define IS_WHITESPACE( c ) (  \
     ((c)=='#') || \
     ((c)=='=') || \
     ((c)==';') || \
     ((c)==',') || \
     ((c)==' ') || \
     ((c)=='\t')|| \
     ((c)=='\n'))

#define IS_SIMPLE_WORD( c ) ( \
     ((c)=='_') || \
     ((c)=='.') || \
     ((c)=='/') || \
     ((c)==':') || \
     ((c)=='@') || \
     ((c)=='-') || \
     ( ((c) >= 48) && ((c) <= 57 ))  ||\
     ( ((c) >= 65) && ((c) <= 90 ))  ||\
     ( ((c) >= 97) && ((c) <= 122 )) )



#endif 
