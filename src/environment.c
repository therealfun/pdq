/* Copyright 1999, 2000 Jacob A. Langford
 *
 * environment.c
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>

#include "option.h"
#include "argument.h"
#include "util.h"
#include "job.h"
#include "list.h"


dl_list *environment_set_item (dl_list *env,
	char *var, char *value) {

   item_pair *i;

   if (value == NULL) return (env);

   i = new_item_pair();
   str_set (&i->key, var);
   str_set (&i->value, value);
   env = append_to_list (env, i);

   return (env);

}


dl_list *environment_set_basic (dl_list *env, job_info *job) {
   
   struct passwd *pw;
   char buf[128], *d; 


   if (job != NULL) {
      sprintf (buf, "%.3i", job->id);
      env = environment_set_item (env, "JOB_ID", buf);
      env = environment_set_item (env, "FILENAME", job->input_filename);
      if (job->job_base != NULL) {
	 d = malloc (strlen (job->job_base) + 25);
	 if (d == NULL) {
	    fprintf (stderr, "Not enough memory.\n");
	    my_exit (1);
	 }
	 sprintf (d, "%s.extra_status", job->job_base);
	 env = environment_set_item (env, "STATUS", d);
	 free (d);
      }
   }


   pw = getpwuid (geteuid());
   d = getenv ("LOGNAME");
   if (d == NULL) {
      if (pw != NULL)  d = pw->pw_name;
      if (d == NULL) d = "unknown";
   }
   env = environment_set_item (env, "LOGNAME", d);

   if (pw != NULL) d = pw->pw_name;
   env = environment_set_item (env, "USER", d);
   
   return (env); 

}

dl_list *environment_from_options (dl_list *env,
	dl_list **option_lists, int opt_list_count,
	dl_list **selection_lists, int sel_list_count) {

   dl_list *op_list, *choice_list, *sel_list;
   option *opt;
   choice *ch;
   char *selection;
   int i, j, resolved;

   for (i = 0; i < opt_list_count; i++) {
      op_list = first_list_element (option_lists[i]);
      while (op_list != NULL) {
	 opt = (option *) op_list->data;
	 opt->choice_list = first_list_element (opt->choice_list);
	 resolved = 0;

	 /* Look in default option specification sources 
	  * for one of the choices in opt->choice_list;
	  */
	 for (j = sel_list_count - 1; (j >= 0) && (!resolved); j--) {
	    sel_list = last_list_element(selection_lists[j]);
	    while ((sel_list != NULL) && (!resolved)) {
	       selection = (char *) sel_list->data;
	       choice_list = opt->choice_list;
	       while ((choice_list != NULL) && (!resolved)) {
		  ch = (choice *) choice_list->data;
		  if (0 == strcmp (ch->name, selection)) resolved = 1;
		  choice_list = choice_list->next;
	       }
	       sel_list = sel_list->prev;
	    }
	 }
	 if (!resolved) {     
	    /* Nothing was specified in option lists.
	     * let's have a look in the option definition
	     */
	    if (opt->default_choice != NULL) {  
	       choice_list = opt->choice_list;
	       while ((choice_list != NULL) && (!resolved)) {
		  ch = (choice *) choice_list->data;
		  if (0 == strcmp (ch->name, opt->default_choice)) 
		  	resolved = 1;
		  choice_list = choice_list->next;
	       }
	    }
	 } 
	 if (!resolved) {     
	    /* Still nothing.
	     * We'll give up and default to the first choice listed
	     */
	    ch = (choice *) opt->choice_list->data;
	 }
         /* We have a choice, let's set the environment variable */

         env = environment_set_item (env, opt->var, ch->value);

	 op_list = op_list->next;
      }
   }

   return (env);

}

dl_list *environment_from_arguments (dl_list *env,
	dl_list **arg_lists, int arg_list_count,
	dl_list **selection_lists, int sel_list_count) {

   dl_list *arg_list, *sel_list, *tmp_env, *tmp_list;
   argument *arg;
   item_pair *it;
   int i;

   tmp_env = NULL;

   for (i = 0; i < sel_list_count; i++) {
      sel_list = first_list_element (selection_lists[i]);
      while (sel_list != NULL) {
         it = (item_pair *) sel_list->data;
         tmp_env = environment_set_item (tmp_env, it->key, it->value);
         sel_list = sel_list->next;
      }
   }
   for (i = 0; i < arg_list_count; i++) {
      arg_list = first_list_element (arg_lists[i]);
      while (arg_list != NULL) {
         arg = (argument *) arg_list->data;
	 tmp_list = find_in_list_by_string (tmp_env, arg->var);
	 if (tmp_list == NULL) {  /* Revert to default */
            tmp_env = environment_set_item (tmp_env, 
	       arg->var, arg->def_value);
	 }
         arg_list = arg_list->next;
      }
   }

   return ( join_lists (env, tmp_env) );

}

void setup_job_environments (job_info *job, rc_items *rc) {

   dl_list *env, *arg_lists[1], *option_lists[2], *selection_lists[3];
   char *buf;

   env = environment_set_basic (NULL, job);

   env = environment_set_item (env, "PATH", rc->driver_command_path);

   option_lists[0] = job->my_driver->option_list;
   selection_lists[0] = job->my_driver->default_option_list;
   selection_lists[1] = job->my_printer->driver_option_list;
   selection_lists[2] = job->extra_driver_opt_list;
   env = environment_from_options (env, option_lists, 1,
	selection_lists, 3);

   arg_lists[0] = job->my_driver->arg_list;
   selection_lists[0] = job->my_printer->driver_arg_list;
   selection_lists[1] = job->extra_driver_arg_list;
   env = environment_from_arguments (env, arg_lists, 1,
	selection_lists, 2);
   
   if (job->env_driver != NULL) {
      list_iterate (job->env_driver, (void *)(void *)free_item_pair);
      free_list (job->env_driver);
   }
   job->env_driver = env;


   env = environment_set_basic (NULL, job);

   env = environment_set_item (env, "PATH", rc->interface_command_path);

   option_lists[0] = job->my_interface->option_list;
   selection_lists[0] = job->my_interface->default_option_list;
   selection_lists[1] = job->my_printer->interface_option_list;
   selection_lists[2] = job->extra_interface_opt_list;
   env = environment_from_options (env, option_lists, 1,
	selection_lists, 3);

   arg_lists[0] = job->my_interface->arg_list;
   selection_lists[0] = job->my_printer->interface_arg_list;
   selection_lists[1] = job->extra_interface_arg_list;
   env = environment_from_arguments (env, arg_lists, 1,
	selection_lists, 2);

   if (job->env_interface != NULL) {
      list_iterate (job->env_interface, (void *)(void *)free_item_pair);
      free_list (job->env_interface);
   }
   job->env_interface = env;

}

char **env_list_to_env (dl_list *l) {
   
   dl_list *m;
   item_pair *ip;
   int count;
   char **env;

   count = 0;
   m = first_list_element (l);
   while (m != NULL) {
      count++;
      m = m->next;
   }

   env = malloc (sizeof (char *) * (count + 1));
   if (env == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }

   m = first_list_element (l);
   count = 0;
   while (m != NULL) {
      ip = (item_pair *)m->data;
      if (ip->value != NULL) {
	 env[count] = malloc (strlen (ip->key) + strlen(ip->value) + 2);
	 if (env[count] == NULL) {
	    fprintf (stderr, "Not enough memory.\n");
	    my_exit (1);
	 }
	 sprintf (env[count], "%s=%s", ip->key, ip->value);
      } else {
	 env[count] = malloc (strlen (ip->key) + 2);
	 if (env[count] == NULL) {
	    fprintf (stderr, "Not enough memory.\n");
	    my_exit (1);
	 }
	 sprintf (env[count], "%s=", ip->key);
      }
      count++;
      m = m->next;
   }
   env[count] = 0;

   return (env);

}

void free_env (char **env) {
  
   int i;

   i = 0;
   while (env[i] != 0)  
   	free ( env[i++] );

   free (env);

}

void free_environment (dl_list *l) {
   list_iterate (l, (void *)(void *)free_item_pair);
   free_list (l);
}
