/* Copyright 1999, 2000 Jacob A. Langford
 *
 * lex.c
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

#include "lex.h"

size_t next_nonwhite (char **buf, char *token, long *line, long *line_in) {

   int tokenlen;


   if (**buf == 0) {
      *token = 0;
      return (-1);
   }

   tokenlen = 0;
   while ( IS_WHITESPACE (**buf) ) {
      if (**buf == '\n') (*line)++;
      if (**buf == '#') {
	 while ( **buf != '\n' ) {
	    if (**buf == 0) {
	       *token = 0;
	       return (-1);
	    }
	    (*buf)++;
	 }
         (*line)++;
      }
      (*buf)++;
   }
   *line_in = *line;
   while ( ! IS_WHITESPACE (**buf) ) {
      if (**buf == 0) {
	 *token = 0;
	 return (tokenlen==0?-1:tokenlen);
      }
      *token = **buf;
      token++; (*buf)++;
      tokenlen++;
   }

   *token = 0;
   return (tokenlen==0?-1:tokenlen);
}

size_t next_word (char **buf, char *token, long *line, long *line_in) {

   int tokenlen;


   if (**buf == 0) {
      *token = 0;
      return (-1);
   }

   tokenlen = 0;
   while ( IS_WHITESPACE (**buf) ) {
      if (**buf == '\n') (*line)++;
      if (**buf == '#') {
	 while ( **buf != '\n' ) {
	    if (**buf == 0) {
	       *token = 0;
	       return (-1);
	    }
	    (*buf)++;
	 }
         (*line)++;
      }
      (*buf)++;
   }
   *line_in = *line;
   while ( IS_SIMPLE_WORD (**buf) ) {
      if (**buf == 0) {
	 *token = 0;
	 return (tokenlen==0?-1:tokenlen);
      }
      *token = **buf;
      token++; (*buf)++;
      tokenlen++;
   }

   *token = 0;
   return (tokenlen==0?-1:tokenlen);
}

size_t next_block (char **buf, char *token, long *line, long *line_in) {

   int tokenlen, nest_level;
   char block_off, block_on;


   if (**buf == 0) {
      *token = 0;
      return (-1);
   }

   tokenlen = 0;
   while ( IS_WHITESPACE (**buf) ) {
      if (**buf == '\n') (*line)++;
      if (**buf == '#') {
	 while ( **buf != '\n' ) {
	    if (**buf == 0) {
	       *token = 0;
	       return (-1);
	    }
	    (*buf)++;
	 }
         (*line)++;
      }
      (*buf)++;
   }
   *line_in = *line;
   while ( IS_SIMPLE_WORD (**buf) ) {
      if (**buf == 0) {
	 *token = 0;
	 return (tokenlen==0?-1:tokenlen);
      }
      *token = **buf;
      token++; (*buf)++;
      tokenlen++;
   }
   if (tokenlen > 0) {
      *token = 0;
      return (tokenlen);
   }

   block_on = **buf;
   switch (block_on) {
      case '\'':
	 block_off = '\'';
	 break;
      case '"':
	 block_off = '"';
	 break;
      case '{':
	 block_off = '}';
	 break;
      case '[':
	 block_off = ']';
	 break;
      case '(':
	 block_off = ')';
	 break;
      default:
	 *token = 0;
	 return (-1);
   }
   (*buf)++;
   nest_level = 1;
   while (  nest_level > 0  ) {
      if (**buf == '\n') (*line)++;
      if (**buf == 0) {
	 fprintf (stderr, "Parse error: Died while looking for %c\n",
	    block_off);
	 return (-1);
      }
      if (**buf == block_off) {
         if (*(*buf - 1) == '\\') {
	    /* Strip the escape character */
	    token--;
	    tokenlen--;
	 } else {
	    nest_level--;
	 }
      } else if (**buf == block_on) {
         if (*(*buf - 1) == '\\') {
	    /* Strip the escape character */
	    token--;
	    tokenlen--;
	 } else {
	   nest_level++;
	 }
      } 
      *token = **buf;
      token++; (*buf)++;
      tokenlen++;
   }

   token--;
   tokenlen--;
   *token = 0;
   return (tokenlen);
}

