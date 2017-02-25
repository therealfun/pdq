/* Copyright 1999, 2000 Jacob A. Langford
 *
 * shepherd.h
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

#ifndef __PDQ_SHEPHERD_H_
#define __PDQ_SHEPHERD_H_

#include "job.h"
#include "environment.h"
#include "rc_items.h"

void shepherd (job_info *job, rc_items *rc);
void redirect_output (job_info *job);
void link_print_file (job_info *job, char *output);
void analyze_print_file (job_info *job, rc_items *rc, char *input); 
char *create_script (char *base, char *script, char *ext); 
void do_file_regx (job_info *job, rc_items *rc); 
void do_convert (job_info *job, rc_items *rc, char *input, char *output); 
void do_filter (job_info *job, rc_items *rc, char *input, char *output); 
int do_send_interface (job_info *job, rc_items *rc, char *input);
void do_cancel (job_info *job, rc_items *rc); 
extern void (*my_exit) (int code);

extern job_info *job;
extern rc_items *rc;
void pdq_exit (int code);

#endif 
