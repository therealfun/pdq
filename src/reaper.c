/* Copyright 1999, 2000 Jacob A. Langford
 *
 * reaper.c
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
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>

#include "reaper.h"

dl_list *reaped_list;

reaped_item *new_reaped_item (void) {
   reaped_item *r;
   r = malloc (sizeof (reaped_item));
   if (r == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }
   return (r);
}

void free_reaped_item (reaped_item *r) {
   free (r);
}

void reaper (int sig) {

   reaped_item *r;

   r = new_reaped_item();

   r->pid = wait ( &r->status );
   if (r->pid == -1) {
      perror ("Error waiting on child");
      my_exit (1);
   }

   reaped_list = append_to_list (reaped_list, r);
   
   signal (SIGCHLD, reaper); /* Better be sure...
                              */                
}

reaped_item *get_reaped_by_pid (pid_t pid) {
   
   dl_list *l;
   reaped_item *r;

   l = first_list_element (reaped_list);
   while (l != NULL) {
      r = (reaped_item *)l->data;
      if (r->pid == pid)  {
	 reaped_list = remove_list_link (l);
         return (r);
      }
      l = l->next;
   }
   return (NULL);
}


