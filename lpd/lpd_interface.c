/* Copyright 1999, 2000 Jacob A. Langford
 *
 * lpd_interface.c
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "lpd_interface.h"

int lpd_connect (char *host_name) {

#undef ROUTINE_ID
#define ROUTINE_ID "lpd_connect()"

	struct hostent *dns_stuff;
	struct sockaddr_in my_addr, serv_addr;

	int sckt;
	int local_port;

        /* Set destination address */
	MESSAGE (3, Getting IP number);
	dns_stuff = gethostbyname(host_name);
	if (dns_stuff == NULL) {
	   if (verbosity) {
	      fprintf (stdout, "ABORT: unknown host %s?", host_name);
	   }
	   exit (1);
	}
        bzero (&serv_addr, sizeof(serv_addr));
        bzero (&my_addr, sizeof(serv_addr));
	bcopy (dns_stuff->h_addr_list[0], &serv_addr.sin_addr, 
	   dns_stuff->h_length);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(515);  /* Is this bad?  I am seeking
	                                   * RFC compliance.
					   */

	if (verbosity >= 3) fprintf (stdout, "trying %s...\n", 
		inet_ntoa (serv_addr.sin_addr) );

        MESSAGE (3, Trying to get reserved port);
	local_port = IPPORT_RESERVED - 1;
	sckt = rresvport (&local_port);
        if (sckt == -1) {
	   MESSAGE (3, Failed. Trying to get ordinary port.  This program);
	   MESSAGE (3, should be suid root to be RFC 1179 compliant);
	   sckt = socket(AF_INET, SOCK_STREAM, 0); 
	   if (sckt == -1) {
	      perror ("bind");
	      exit (1);
	   }
	   my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	   my_addr.sin_family = AF_INET;
	   my_addr.sin_port = htons(0);
	   if (-1 ==
	      bind (sckt, (struct sockaddr *)&my_addr, sizeof(my_addr)) ) {
	      perror ("bind");
	      exit (1);
	   }
	}

	MESSAGE (2, connecting...);

	if ( connect(sckt, &serv_addr, sizeof(serv_addr)) != 0 ) { 
	   perror ("connect");
	   exit (1);
	} 

	return (sckt);

}

void lpd_send_data (int sckt, char *buf, int len) {

#undef ROUTINE_ID
#define ROUTINE_ID "lpd_send_data()"

     int wcount;
     MESSAGE (3, Sending data);
     while (len > 0) {
	wcount = write (sckt, buf, len);
	SILENT_DIE_IF (wcount == -1, LOST_CONNECTION);
	len -= wcount;
	buf += wcount;
     }

}

void lpd_send_acknowledgement (int sckt) {

#undef ROUTINE_ID
#define ROUTINE_ID "lpd_send_acknowledgement()"

   char s;
   
   s = 0;
   SILENT_DIE_IF( write(sckt, &s, 1) != 1, LOST_CONNECTION );
   MESSAGE (3, Acknowledgement sent);

}

void lpd_receive_acknowledgement (int sckt) {

#undef ROUTINE_ID
#define ROUTINE_ID "lpd_receive_acknowledgement()"

     char buf;

     SILENT_DIE_IF (1 != read(sckt, &buf, 1), LOST_CONNECTION);
     SILENT_DIE_IF (buf != 0, ACKNOWLEDGE_FAILED);
     MESSAGE (3, Acknowledgement received);

}

void lpd_send_request_to_send_job (int sckt, char *queue) {

#undef ROUTINE_ID
#define ROUTINE_ID "lpd_send_request_to_send_job()"

     char *buf;
     int len;

     buf = malloc ( strlen(queue) + 2 + 1 );
     SILENT_DIE_IF (buf == NULL, MALLOC); 
     sprintf(buf, "%c%s\n", '\2',queue);
     len = strlen (buf);
     MESSAGE (3, Requesting lpd to receive a print job);
     SILENT_DIE_IF (write(sckt, buf, len) != len, LOST_CONNECTION);
     free (buf);

}

void lpd_send_request_to_send_cf (int sckt, unsigned long cf_len, 
		char *cf_name) {

#undef ROUTINE_ID
#define ROUTINE_ID "lpd_send_request_to_send_cf()"

     char *buf;
     int len;

     /* Guess 50 is enough for the sprintf of unsigned long... if not we'll
        suffer the consequences of buffer overrun... Too bad snprintf
	isn't standard */
     len = strlen(cf_name) + 2 + 50 + 1;
     buf = malloc ( len );
     SILENT_DIE_IF (buf == NULL, MALLOC); 
     sprintf(buf, "%c%lu %s\n", '\2',cf_len, cf_name);
     len = strlen (buf);
     MESSAGE (3, Requesting lpd to receive a job control file);
     SILENT_DIE_IF (write(sckt, buf, len) != len, LOST_CONNECTION);
     free (buf);

}

void lpd_send_request_to_send_df (int sckt, unsigned long df_len, 
		char *df_name) {

#undef ROUTINE_ID
#define ROUTINE_ID "lpd_send_request_to_send_df()"

     char *buf;
     int len;

     /* Guess 50 is enough for the sprintf of unsigned long... if not we'll
        suffer the consequences of buffer overrun... Too bad snprintf
	isn't standard */
     len = strlen(df_name) + 2 + 50 + 1;
     buf = malloc ( len );
     SILENT_DIE_IF (buf == NULL, MALLOC); 
     sprintf(buf, "%c%lu %s\n", '\3',df_len, df_name);
     len = strlen (buf);
     if (verbosity >= 3) {
        fprintf (stdout, "Requesting lpd to receive a job data file, "
		"%lu bytes\n", df_len);
     }
     SILENT_DIE_IF (write(sckt, buf, len) != len, LOST_CONNECTION);
     free (buf);

}

void lpd_send_print_waiting_jobs (int sckt, char *queue) {

#undef ROUTINE_ID
#define ROUTINE_ID "lpd_send_print_waiting_jobs()"

     char *buf;
     int len;

     buf = malloc ( strlen(queue) + 2 + 1 );
     SILENT_DIE_IF (buf == NULL, MALLOC); 
     sprintf(buf, "%c%s\n", '\1',queue);
     len = strlen (buf);
     MESSAGE (3, Requesting lpd to print waiting jobs);
     SILENT_DIE_IF (write(sckt, buf, len) != len, LOST_CONNECTION);
     free (buf);

}

void lpd_send_request_short_queue (int sckt, char *queue) {

#undef ROUTINE_ID
#define ROUTINE_ID "lpd_send_request_short_queue()"

     char *buf;
     int len;

     buf = malloc ( strlen(queue) + 2 + 1 );
     SILENT_DIE_IF (buf == NULL, MALLOC); 
     sprintf(buf, "%c%s\n", '\3',queue);
     len = strlen (buf);
     MESSAGE (3, Requesting lpd to send the short form of the queue);
     SILENT_DIE_IF (write(sckt, buf, len) != len, LOST_CONNECTION);
     free (buf);

}

void lpd_send_request_long_queue (int sckt, char *queue) {

#undef ROUTINE_ID
#define ROUTINE_ID "lpd_send_request_long_queue()"

     char *buf;
     int len;

     buf = malloc ( strlen(queue) + 2 + 1 );
     SILENT_DIE_IF (buf == NULL, MALLOC); 
     sprintf(buf, "%c%s\n", '\4',queue);
     len = strlen (buf);
     MESSAGE (3, Requesting lpd to send the long form of the queue);
     SILENT_DIE_IF (write(sckt, buf, len) != len, LOST_CONNECTION);
     free (buf);

}

void lpd_receive_data_to_stream (int sckt, FILE *stream) {

#undef ROUTINE_ID
#define ROUTINE_ID "lpd_receive_data_to_stream()"

     int i;
     char buf[1024];

     MESSAGE (3, Reading from lpd sckt);
     while ((i = read(sckt, buf, sizeof(buf))) > 0) {
        SILENT_DIE_IF (i != fwrite(buf, 1, i, stdout), FILE_IO_ERROR);
     }

}

void lpd_send_cancel (int sckt, char *queue, char *user,
	char *job_id) {

#undef ROUTINE_ID
#define ROUTINE_ID "lpd_send_cancel()"

     char *buf;
     int len;

     buf = malloc ( strlen(queue) + strlen(user) + strlen(job_id) + 10);
     SILENT_DIE_IF (buf == NULL, MALLOC); 
     sprintf(buf, "%c%s %s %s\n", '\5', queue, user, job_id);
     len = strlen (buf);
     if (verbosity >= 3) fprintf (stdout, "Requesting to cancel %s %s at %s\n", 
	     job_id, user, queue );
     SILENT_DIE_IF (write(sckt, buf, len) != len, LOST_CONNECTION);
     free (buf);

}

void lpd_send_abort (int sckt) {
#undef ROUTINE_ID
#define ROUTINE_ID "lpd_send_abort()"

     char buf[2];

     buf[0] = '\5';
     buf[1] = '\n';
     if (verbosity >= 3) fprintf (stdout, "Requesting abort\n");
     SILENT_DIE_IF (write(sckt, buf, 2) != 2, LOST_CONNECTION);

}

