/* Copyright 1999, 2000 Jacob A. Langford
 *
 * lpd_print.c
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
#include <sys/stat.h>
#include <pwd.h>
#include <signal.h>
#include <sys/types.h>

#include "lpd_interface.h"

typedef struct _Options Options;
   struct _Options {

      char *lpd_host;        /* Name of lpd host */
      char *lpd_queue;       /* Queue name */
      char *data_file;       /* or STDIN if not provided */
      int  job_id;           /* 1 - 999, 1 by default */
      int  nasty_cancel;     /* Do nasty abort in mid-send? */

};

typedef struct _Job Job;
   struct _Job {

      char *control_data;    /* What a pain */
      char *cf_name;         /* Name of control file */
      char *df_name;         /* Name of data file */
      size_t cf_len;         /* Length of control file */
      size_t df_len;         /* Length of data file */
      long KB_total;         /* For status bar */
      long KB_sent;          /* For status bar */

};

/* Forward declarations of routines */
void usage(char *prog_name);
Options *get_options (int argc, char *argv[]);
Job *get_job_info (Options *options);
void destroy_options (Options *options);
void destroy_job (Job *job);
void catch_signal (int sig);

int verbosity;
int terminated;

int main (int argc, char *argv[]) {

#undef ROUTINE_ID
#define ROUTINE_ID "lpd_print::main()"

     /* Just for the status stuff */
     Options *options;
     Job *job;
     char buf[8192];
     int sckt, i;
     FILE *handle;

     options = get_options (argc, argv);

     job = get_job_info (options);

     /* Make sure we really have something to print */
     if (options->data_file == NULL) {
        MESSAGE (3,Reading from stdin);
        handle = tmpfile();
	SILENT_DIE_IF (handle == NULL, FILE_IO_ERROR);
	while ( (i=fread (buf, 1, 8192, stdin)) > 0) {
	   SILENT_DIE_IF ( i != fwrite (buf, 1, i, handle) , FILE_IO_ERROR );
           job->df_len += i;
	}
	SILENT_DIE_IF (0 != fseek (handle, 0, SEEK_SET) , FILE_IO_ERROR);
     } else {
        handle = fopen (options->data_file, "r");
	SILENT_DIE_IF (handle == NULL, FILE_IO_ERROR);
     }

     if ((job->df_len == 0) && (verbosity)) {
        if (options->data_file == NULL) {
           fprintf (stdout, "ABORT: Nothing was read from stdin.\n");
	} else {
           fprintf (stdout, "ABORT: %s is an empty file.\n",
	   		options->data_file);
	}
	exit (0);
     }


     terminated = 0;
     signal (SIGTERM, catch_signal); 

     setvbuf (stdout, NULL, _IONBF, 0);  /* Disable output buffering */
   
     sckt = lpd_connect (options->lpd_host);
     MESSAGE (2, pending...);

     lpd_send_request_to_send_job (sckt, options->lpd_queue);
     lpd_receive_acknowledgement (sckt);

     lpd_send_request_to_send_cf (sckt, job->cf_len, job->cf_name);
     lpd_receive_acknowledgement (sckt);
     lpd_send_data (sckt, job->control_data, job->cf_len);
     if (verbosity >= 3) write(2, job->control_data, job->cf_len); 
     lpd_send_acknowledgement (sckt);
     lpd_receive_acknowledgement (sckt);

     lpd_send_request_to_send_df (sckt, job->df_len, job->df_name);
     lpd_receive_acknowledgement (sckt);

     if (terminated) {
        lpd_send_abort (sckt);
        lpd_receive_acknowledgement (sckt);
	MESSAGE (2, terminated);
     } else {

	job->KB_total = job->df_len / 1024;
	job->KB_sent = 0;
	while ( (i=fread (buf, 1, 8192, handle)) > 0) {
	   if (verbosity >= 2) {
	      if (terminated) {
		 fprintf (stdout, "%li/%liKB (cancelling...)\n", 
			   job->KB_sent, job->KB_total);
                 if (options->nasty_cancel) {
		    if (verbosity >= 2) {
		       fprintf (stderr, "lpd_print:"
		       	" closing socket (Nasty!  Adding you to "
			"/etc/blacklist)\n");
		    }
		    goto EXIT;  /* All's fair in love and war...
				 * when it comes to RFC1179, it is war.
				 * Let the printer timeout on this
				 * baby...
				 */
                 }
	      } else {
		 fprintf (stdout, "%li/%liKB\n", job->KB_sent, job->KB_total);
	      }
	   }
	   lpd_send_data ( sckt, buf, i );
	   job->KB_sent += 8;
	}
	lpd_send_acknowledgement (sckt);
	lpd_receive_acknowledgement (sckt);
	if (terminated) {
	   lpd_send_abort (sckt);
           lpd_receive_acknowledgement (sckt);
	   MESSAGE (2, terminated);
	}
     }

     EXIT:

     /* Barney says clean up, clean up, everybody do your share... */
     close (sckt); 
     MESSAGE (3, Connection closed);
     fclose (handle);
     destroy_options (options);
     if (terminated) {
        MESSAGE (2, cancelled );
     } else {
        MESSAGE (2, finished );
     }

     return (0);

}

Options *get_options (int argc, char *argv[]) {

#undef ROUTINE_ID
#define ROUTINE_ID "get_options()"

   Options *options;
   char *prog_name, *tmp, c;

   options = (Options *)malloc (sizeof (Options));
   if (options == NULL) {
      exit (MALLOC);
   }
   options->lpd_host      = NULL;
   options->lpd_queue     = NULL;
   options->data_file     = NULL;
   options->job_id        = 1;
   options->nasty_cancel  = 0;

   /* Resolve command line arguments */
   prog_name = &argv[0][0]; argv++; argc--;
   while ((argc > 0) && (argv[0][0] == '-')) {
      argc--;
      tmp = argv[0];
      while ( (c = *(++tmp)) != '\0' ) {
	 switch (c) {
	    case 'n':
	       options->nasty_cancel = 1;
	       break;
	    case 'v':
	       verbosity++;
	       break;
	    case 'f':
	       argc--;
	       argv++;
	       options->data_file = argv[0];
	       break;
	    case 'j':
	       argc--;
	       argv++;
	       options->job_id = atoi( argv[0] );
	       if (options->job_id > 999) options->job_id = 999;
	       if (options->job_id < 1) options->job_id = 1;
	       break;
	    default:  
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

#undef ROUTINE_ID
#define ROUTINE_ID "destroy_options()"

   free (options);
}

void usage(char *prog_name) {

   fprintf (stderr, 
      "Usage: %s", prog_name);
   fprintf (stderr, 
      " [-f file] [-j id_num] [-v|-vv] queue@host | host [queue] \n"
      "  -f   specifies data file (STDIN default)\n"
      "  -j   specifies job id (1-999, 001 default)\n"
      "  -n   do a \"nasty\" mid-send socket close on SIGTERM\n"
      "  -v   to display warnings and fatal errors\n"
      "  -vv  to be show simple status messages\n"
      "  -vvv to be annoyingly verbose\n");
   exit(PROGRAM_USAGE);
}

Job * get_job_info (Options *options) {

#undef ROUTINE_ID
#define ROUTINE_ID "get_job_info()"

   /* 
    * Control file will be
    * 
    * H hostname
    * P username
    * f dfA001hostname  (or real job_id)
    * U dfA001hostname  (or real job_id)
    *
    */

   char hostname[512];
   char *username, *p;
   struct stat file_info;
   Job *job;
   struct passwd *pw;

   job = (Job *)malloc (sizeof (Job));
   SILENT_DIE_IF (job == NULL, MALLOC);

   gethostname(hostname, 512);
   if ( strlen(hostname) >= 512 ) hostname[511] = 0;
   p = strstr (hostname, ".");
   if (p != NULL) *p = 0;

   pw = getpwuid(getuid());
   SILENT_DIE_IF (pw == NULL, MISCELLANEOUS);
   username = pw->pw_name;
   SILENT_DIE_IF (username == NULL, MISCELLANEOUS);


   job->control_data = malloc (1 + strlen(hostname) + 1
                             + 1 + strlen(username) + 1 
			     + 1 + 6 + strlen(hostname) + 1 
			     + 1 + 6 + strlen(hostname) + 1 + 1);
   SILENT_DIE_IF (job->control_data == NULL, MALLOC);
   job->cf_name = malloc ( 6 + strlen(hostname) + 1 );
   SILENT_DIE_IF (job->cf_name == NULL, MALLOC);
   job->df_name = malloc ( 6 + strlen(hostname) + 1 );
   SILENT_DIE_IF (job->df_name == NULL, MALLOC);

   sprintf (job->control_data, "H%s\nP%s\nfdfA%.3i%s\nUdfA%.3i%s\n", 
      hostname, username, options->job_id, hostname, options->job_id,
      hostname);

   sprintf (job->cf_name, "cfA%.3i%s", options->job_id, hostname);
   sprintf (job->df_name, "dfA%.3i%s", options->job_id, hostname);

   job->cf_len = strlen(job->control_data);

   if (options->data_file != NULL) {
      SILENT_DIE_IF (
         0 != stat (options->data_file, &file_info),
	 FILE_IO_ERROR);
      job->df_len = file_info.st_size;
   }

   
   return (job);

}

void destroy_job (Job *job) {

#undef ROUTINE_ID
#define ROUTINE_ID "destroy_job()"


   free (job->cf_name);
   free (job->df_name);
   free (job->control_data);
   free (job);

}

void catch_signal (int sig) {
   terminated = 1;
   if (verbosity >= 2) fprintf (stderr, "lpd_print: caught SIGTERM\n");

   signal (SIGTERM, catch_signal); /* Better be sure... */
}
