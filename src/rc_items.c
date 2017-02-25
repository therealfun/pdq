/* Copyright 1999, 2000 Jacob A. Langford
 *
 * rc_items.c
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

#include "rc_items.h"
#include "util.h"
#include "list.h"
#include "printer.h"
#include "driver.h"
#include "interface.h"



rc_items *new_rc_items (void) {

   rc_items *rc;

   rc = malloc (sizeof(rc_items));
   if (rc == NULL) {
      fprintf (stderr, "Not enough memory.\n");
      my_exit (1);
   }

   rc->printer_list = NULL;
   rc->driver_list = NULL;
   rc->interface_list = NULL;

   rc->default_printer = NULL;
   str_set (&rc->driver_command_path,	"bin:/usr/bin:/usr/local/bin");
   str_set (&rc->interface_command_path, "bin:/usr/bin:/usr/local/bin");
   str_set (&rc->job_dir, 		"~/.printjobs");

   rc->max_send_tries = 30;
   rc->delay_between_tries = 10;          /* 10 seconds */
   rc->job_history_duration = 3600 * 72;  /* 72 hours */

   rc->debug_rc = 0;

   return (rc);
   
}

void free_rc_items (rc_items *rc) {

   list_iterate (rc->printer_list, (void *)(void *)free_printer);
   rc->printer_list = free_list (rc->printer_list);

   list_iterate (rc->driver_list, (void *)(void *)free_driver);
   rc->driver_list = free_list (rc->driver_list);

   list_iterate (rc->interface_list, (void *)(void *)free_interface);
   rc->interface_list = free_list (rc->interface_list);

   if (rc->default_printer != NULL) free (rc->default_printer);
   if (rc->interface_command_path != NULL) free (rc->interface_command_path);
   if (rc->driver_command_path != NULL) free (rc->driver_command_path);
   if (rc->job_dir != NULL) free (rc->job_dir);

}


