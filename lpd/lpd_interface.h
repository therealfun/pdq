/* Copyright 1999, 2000 Jacob A. Langford
 *
 * lpd_interface.h
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


#ifndef __LPD_INTERFACE_H
#define __LPD_INTERFACE_H


#include <errno.h>

/* Routines */
int lpd_connect (char *host_name);
void lpd_send_acknowledgement (int sckt); 
void lpd_receive_acknowledgement (int sckt);
void lpd_send_data (int sckt, char *buf, int len); 
void lpd_send_request_to_send_job (int sckt, char *queue); 
void lpd_send_request_to_send_cf (int sckt,
   unsigned long cf_len, char *cf_name);
void lpd_send_request_to_send_df (int sckt,
   unsigned long df_len, char *df_name);
void lpd_send_print_waiting_jobs (int sckt, char *queue); 
void lpd_send_request_long_queue (int sckt, char *queue);
void lpd_send_request_short_queue (int sckt, char *queue); 
void lpd_send_cancel (int sckt, char *queue, char *user, char *job_id);
void lpd_receive_data_to_stream (int sckt, FILE *stream); 
void lpd_send_abort (int sckt);


/* Global variable used for messages, warnings, and errors */
extern int verbosity;



/* Error codes */
#define NO_DNS_ENTRY 		1
#define NO_RESERVED_PORT        2
#define NO_SOCKET_CONNECT 	3
#define LOST_CONNECTION 	4
#define NO_ROOT_ACCESS     	5
#define ACKNOWLEDGE_FAILED     	7
#define PROGRAM_USAGE     	8
#define BUFFER_OVERRUN     	9
#define MALLOC 	    		10
#define FILE_IO_ERROR	        11
#define MISCELLANEOUS	        12

/* MESSAGE and ERROR macros */

#define ROUTINE_ID "unknown"

#define DIE_IF(EXP, CODE ) \
   if (EXP) { \
      fprintf (stderr,  ROUTINE_ID " " #CODE " die_if(" #EXP ")\n"); \
      _exit (CODE); }

#define SILENT_DIE_IF( EXP, CODE ) \
   if (EXP) { \
      if (verbosity > 0) \
         fprintf (stderr, ROUTINE_ID " " #CODE " die_if(" #EXP ")\n"); \
      _exit (CODE); }

#define MESSAGE( LEVEL, TEXT ) \
   if (verbosity >= LEVEL) \
      fprintf ( stdout, #TEXT "\n")

#endif /*  __LPD_INTERFACE_H */
