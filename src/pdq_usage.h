/* Copyright 1999, 2000 Jacob A. Langford
 *
 * pdq_usage.h
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

#ifndef __PDQ_USAGE_H_
#define __PDQ_USAGE_H_

#include "job.h"

void pdq_process_argv (int argc, char *argv[], job_info *job, rc_items *rc,
   char ***file, int *nfiles);
void pdq_usage (char *argv0);
void show_overview (char *argv0);
void show_printer_list (rc_items *rc);
void show_printer_options (rc_items *rc, char *p_name);

extern void (*my_exit) (int code);

#endif 
