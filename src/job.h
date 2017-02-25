/* Copyright 1999, 2000 Jacob A. Langford
 *
 * job.h
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

#ifndef __JOB_H_
#define __JOB_H_

#include "list.h"
#include "printer.h"
#include "driver.h"
#include "interface.h"
#include "util.h"

typedef struct _job_info job_info;
struct _job_info {

   int id;
   int fd;

   int do_help;
   char *argv0;

   char *job_base;

   char *status;

   char *input_filename;
   char *invoked_by;
   char *invoke_time;

   char *file_type;

   char *printer;
   char *driver;
   char *interface;
   char *language_driver;


   printer *my_printer;
   driver *my_driver;
   interface *my_interface;
   language_driver *my_language_driver;

   dl_list *env_driver;
   dl_list *env_interface;
   
   dl_list *extra_driver_opt_list;
   dl_list *extra_interface_opt_list;
   dl_list *extra_driver_arg_list;
   dl_list *extra_interface_arg_list;
   
   long last_stat;

};

job_info *new_job_info (void);
void free_job_info (job_info *job);
void write_status (job_info *job);
void set_status (char *status, job_info *job);
void force_set_status (char *status, job_info *job);
void purge_old_jobs (char *job_dir, long job_history_duration);
dl_list *job_list (char *job_dir); 
job_info *get_status_by_file (char *file); 
job_info *get_status (int id, rc_items *rc); 
int get_status_update (job_info **job, rc_items *rc);
void delete_job (int id, char *job_dir); 
void claim_job_id (job_info *job, rc_items *rc);
char *claim_tmpfile (char *job_dir); 

int load_printer (job_info *job);

extern void (*my_exit) (int code);


#endif
