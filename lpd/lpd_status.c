/* Copyright 1999, 2000 Jacob A. Langford
 *
 * lpd_status.c
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
#include <string.h>
#include "lpd_interface.h"

#define BUFSIZE 10

#define QUEUE_SHORT 1
#define QUEUE_LONG  2

typedef struct _Options Options;
   struct _Options {

      char *lpd_host;        /* Name of lpd host */
      char *lpd_queue;       /* Queue name */
      int  status_type;      /* Prefix used for all files */

};

/* Forward declarations of routines */
void usage(char *prog_name);
Options *get_options (int argc, char *argv[]);
void destroy_options (Options *options);

#undef ROUTINE_ID
#define ROUTINE_ID "lpd_status.c - main()"

int verbosity;

int main (int argc, char *argv[]) {

        /* Just for the status stuff */
	Options *options;
        char buf[1024];
	int i;
	int sckt;

	options = get_options (argc, argv);

        sckt = lpd_connect (options->lpd_host);

	if (options->status_type == QUEUE_LONG) {
	   lpd_send_request_long_queue (sckt, options->lpd_queue);
	} else {
	   lpd_send_request_short_queue (sckt, options->lpd_queue);
	}

	lpd_receive_data_to_stream (sckt, stdout);

	close (sckt);
	MESSAGE (3, Connection closed);

	destroy_options (options);

	return (0);

}

Options *get_options (int argc, char *argv[]) {

   Options *options;
   char *prog_name, *tmp, c;

   options = (Options *)malloc (sizeof (Options));
   if (options == NULL) {
      exit (MALLOC);
   }
   options->lpd_host    = NULL;
   options->lpd_queue   = NULL;
   options->status_type = QUEUE_SHORT;

   /* Resolve command line arguments */
   prog_name = &argv[0][0]; argv++; argc--;
   while ((argc > 0) && (argv[0][0] == '-')) {
      argc--;
      tmp = argv[0];
      while ( (c = *(++tmp)) != '\0' ) {
	 switch (c) {
	    case 'v':
	       verbosity++;
	       break;
	    case 'l':
	       options->status_type = QUEUE_LONG;
	       break;
	    default:  /* Options taking arguments */
	       usage (prog_name);
	 }
      }
      argv++;
   }

   if (argc == 2) {
      options->lpd_host  = argv[0]; 
      options->lpd_queue = argv[1]; 
   } else if (argc == 1) {
      /* Check for the form queue@host */
      options->lpd_queue = argv[0]; 
      options->lpd_host = strchr (options->lpd_queue, '@');
      if ( options->lpd_host == NULL ) {
         options->lpd_host = options->lpd_queue;
         options->lpd_queue = "lp";
	 MESSAGE(1,Warning... queue name not supplied... defaulting to lp);
      } else {
         /* Replace @ with a null terminator and advance by one */
         *options->lpd_host = '\0';
         options->lpd_host++;
      }
   } else {
      usage(prog_name);
   }


   return (options);
}

void destroy_options (Options *options) {
   
   free (options);
}


void usage(char *prog_name) {

   printf ("Usage: %s", prog_name);
   printf ("  [-l] [-v|-vv]  queue@host | host [queue]  \n");
   printf ("  -l  to request a long listing\n");
   printf ("  -v  to display warnings and fatal errors\n");
   printf ("  -vv to be annoyingly verbose\n");
   exit(PROGRAM_USAGE);
}
