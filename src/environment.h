/* Copyright 1999, 2000 Jacob A. Langford
 *
 * environment.h
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

#ifndef __ENVIRONMENT_H_
#define __ENVIRONMENT_H_

dl_list *environment_set_basic (dl_list *env, job_info *job);

dl_list *environment_set_item (dl_list *env,
	char *var, char *value);

dl_list *environment_from_options (dl_list *env,
	dl_list **option_lists, int opt_list_count,
	dl_list **selection_lists, int sel_list_count); 

dl_list *environment_from_arguments (dl_list *env,
	dl_list **arg_lists, int arg_list_count,
	dl_list **selection_lists, int sel_list_count); 

void setup_job_environments (job_info *job, rc_items *rc);

char **env_list_to_env (dl_list *l); 
void free_environment (dl_list *l);
void free_env (char **env); 

extern void (*my_exit) (int code);

#endif
