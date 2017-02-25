/* Copyright 1999, 2000 Jacob A. Langford
 *
 * parse.c
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
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "parse.h"
#include "util.h"
#include "lex.h"

parse_state *new_parse_state (void) {
   parse_state *s;
   s = malloc (sizeof (parse_state) );
   if (s == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   s->malloced = 0;
   return (s);
}

parse_state *sub_parse_state (parse_state *s) {

   parse_state *new_s;

   new_s = new_parse_state();
   new_s->file_name = s->file_name;
   new_s->line = s->line;
   new_s->line_in = s->line_in;
   *(new_s->line) = *(s->line_in);
   new_s->rc_buf_in = s->token;
   new_s->rc_buf_pos = s->token;
   new_s->work_in = s->rc_buf_in;
   new_s->token = s->rc_buf_in;
   new_s->malloced = 0;
   
   return (new_s);

}

int try_initialize_parse_state (parse_state *s, char *file) {

   FILE *fd;
   struct stat finfo;

   if ((fd = fopen (file, "r")) == NULL) { 
      fprintf (stderr, "Warning: cannot open %s.\n", file);
      return (1);
   }

   if (  0 != stat (file, &finfo)  ) {
      fprintf (stderr, "Couldn't stat %s\n", file);
      my_exit(1);
   }
   if (finfo.st_size == 0)  {
      fprintf (stderr, "Warning: %s is empty.\n", file);
      return (1);
   }

   s->rc_buf_in = malloc (finfo.st_size+1);
   if (  s->rc_buf_in == NULL  ) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit(1);
   }

   s->work_in = malloc (finfo.st_size+1);
   if (  s->work_in == NULL  ) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit(1);
   }

   s->file_name = malloc (strlen(file) + 1);
   if (  s->file_name == NULL  ) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit(1);
   }
   strcpy (s->file_name, file);

   s->rc_buf_in [finfo.st_size] = 0;
   s->work_in [finfo.st_size] = 0;
   s->rc_buf_pos = s->rc_buf_in;
   s->token = s->work_in;

   if (  finfo.st_size != fread (s->rc_buf_in, 1, finfo.st_size, fd)  ) {
      fprintf (stderr, "IO error reading %s\n", file);
      my_exit(1);
   }

   s->line = malloc (sizeof(long));
   if (  s->line == NULL  ) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit(1);
   }
   *(s->line) = 1;

   s->line_in = malloc (sizeof(long));
   if (  s->line_in == NULL  ) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit(1);
   }

   fclose (fd);
   s->malloced = 1;
   return (0);

}

void initialize_parse_state (parse_state *s, char *file) {

   FILE *fd;
   struct stat finfo;

   if ((fd = fopen (file, "r")) == NULL) { 
      fprintf (stderr, "Can't open %s\n", file);
      my_exit(1);
   }

   if (  0 != stat (file, &finfo)  ) {
      fprintf (stderr, "Couldn't stat %s\n", file);
      my_exit(1);
   }

   if (finfo.st_size == 0)  {
      fprintf (stderr, "Error: file %s is empty!\n", file);
      my_exit (1);
   }

   s->rc_buf_in = malloc (finfo.st_size+1);
   if (  s->rc_buf_in == NULL  ) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit(1);
   }

   s->work_in = malloc (finfo.st_size+1);
   if (  s->work_in == NULL  ) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit(1);
   }

   s->file_name = malloc (strlen(file) + 1);
   if (  s->file_name == NULL  ) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit(1);
   }
   strcpy (s->file_name, file);

   s->rc_buf_in [finfo.st_size] = 0;
   s->work_in [finfo.st_size] = 0;
   s->rc_buf_pos = s->rc_buf_in;
   s->token = s->work_in;

   if (  finfo.st_size != fread (s->rc_buf_in, 1, finfo.st_size, fd)  ) {
      fprintf (stderr, "IO error reading %s\n", file);
      my_exit(1);
   }

   s->line = malloc (sizeof(long));
   if (  s->line == NULL  ) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit(1);
   }
   *(s->line) = 1;

   s->line_in = malloc (sizeof(long));
   if (  s->line_in == NULL  ) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit(1);
   }

   s->malloced = 1;
   fclose (fd);

}

void free_parse_state (parse_state *s) {

    if (s->malloced) {
       free (s->rc_buf_in);
       free (s->work_in);
       free (s->file_name);
       free (s->line);
       free (s->line_in);
    }
    free (s);

}

int scan_next_block (parse_state *s) {

   size_t len;

   len = next_block ( &(s->rc_buf_pos), s->token, s->line,
   	s->line_in);
   s->token_len = len;

   if (len == -1) {
      if (*s->rc_buf_pos != 0) {
         fprintf (stderr, "Unexpected character %c\n", *s->rc_buf_pos);
	 parse_error (s);
	 my_exit (1);
      }
      return (0);
   } else {
      return (1);
   }

}

void parse_error (parse_state *s) {
   
   fprintf (stderr, "Error parsing %s, line %li.\n", s->file_name,
   	*s->line);

}

rc_location *new_rc_location (void) {
   rc_location *rc_loc;
   rc_loc = malloc (sizeof (rc_location));
   if (rc_loc == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   rc_loc->rc_filename = NULL;
   rc_loc->punch_in = 0;
   rc_loc->punch_out = 0;
   return (rc_loc);
}

void free_rc_location (rc_location *rc_loc) {
   if (rc_loc->rc_filename != NULL) free (rc_loc->rc_filename);
   free (rc_loc);
}

rc_location *new_rc_loc_filled (char *rc_filename, 
   size_t punch_in, size_t punch_out) {
   
   rc_location *rc_loc;
   rc_loc = new_rc_location ();
   str_set (&rc_loc->rc_filename, rc_filename); 
   rc_loc->punch_in = punch_in;
   rc_loc->punch_out = punch_out;

   return (rc_loc);

}

int append_to_rc_file (char *rc_file, char *buf) {

   int fd;
   char *file;

   file = tilde_expand (rc_file);

   fd = open (file, O_APPEND | O_WRONLY | O_CREAT , 0644);
   if (fd == -1) {
      free (file);
      return (1);  /* Any of several reasons... */
   }
   if (write (fd, buf, strlen(buf)) == -1) {
      close (fd);
      free (file);
      return (1);
   } 
   close (fd);
   free (file);
   return (0);

}

int replace_in_rc_file (rc_location *rc_loc, char *buf) {

   struct stat statbuf;
   char *tmpfile, copybuf[1024];
   long len, count, remaining, buflen;
   int in_fd, out_fd;


   in_fd = open (rc_loc->rc_filename, O_RDONLY);
   if (in_fd == -1) {
      return (1);
   }
   if (-1 == fstat (in_fd, &statbuf)) {
      return (1);
   }

   tmpfile = malloc (strlen (rc_loc->rc_filename) + 5);
   if (tmpfile == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   sprintf (tmpfile, "%s.tmp", rc_loc->rc_filename);
   out_fd = open (tmpfile, O_CREAT | O_WRONLY, statbuf.st_mode);
   if (out_fd == -1) {
      close (in_fd);
      free (tmpfile);
      return (1);
   }
   count = 0;
   while (count < rc_loc->punch_in) {
      remaining = rc_loc->punch_in - count;
      len = (remaining < 1024) ? remaining : 1024;
      if (len != read (in_fd, copybuf, len)) {
         close (in_fd);    
         close (out_fd);    
	 unlink (tmpfile);
	 free (tmpfile);
	 return (1);
      }
      if (len != write (out_fd, copybuf, len)) {
         close (in_fd);    
         close (out_fd);    
	 unlink (tmpfile);
	 free (tmpfile);
	 return (1);
      }
      count += len;
   }

   buflen = strlen(buf);
   if (buflen != write (out_fd, buf, buflen)) {
      close (in_fd);    
      close (out_fd);    
      unlink (tmpfile);
      free (tmpfile);
      return (1);
   }

   if (rc_loc->punch_out != lseek (in_fd, rc_loc->punch_out, SEEK_SET) ) {
      close (in_fd);    
      close (out_fd);    
      unlink (tmpfile);
      free (tmpfile);
      return (1);
   }

   while (  (len = read (in_fd, copybuf, 1024)) != 0) {
      if ( len != write (out_fd, copybuf, len) ) {
	 close (in_fd);    
	 close (out_fd);    
	 unlink (tmpfile);
	 free (tmpfile);
	 return (1);
      }
   }
   close (in_fd);    
   close (out_fd);    

   if ( rename (tmpfile, rc_loc->rc_filename) != 0 ) {
      unlink (tmpfile);
      free (tmpfile);
      return (1);
   }

   free (tmpfile);
   return (0);

}

int delete_in_rc_file (rc_location *rc_loc) {
   return (replace_in_rc_file (rc_loc, "\n"));
}
