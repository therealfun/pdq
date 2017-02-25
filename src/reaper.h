/* Copyright 1999, 2000 Jacob A. Langford
 *
 * reaper.h
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



#ifndef __REAPER_H_
#define __REAPER_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

#include "list.h"

typedef struct _reaped_item reaped_item;
struct _reaped_item {
   pid_t pid;
   int status;
};

reaped_item *new_reaped_item (void);
void free_reaped_item (reaped_item *r);
reaped_item *get_reaped_by_pid (pid_t pid);

void reaper (int sig); 

extern dl_list *reaped_list;

#endif
