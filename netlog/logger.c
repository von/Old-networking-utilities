/*
	logger.c

	Part of libnetlog.a

	This file contains the source code for logging.
*/

#include "logger.h"
#include "percent_parse.h"
#include "support.h"
#include "timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/param.h>


#define TIMESTAMP_FORMAT	"%6.3f"
#define TIMESTAMP_LEN		8	/* xx.xxx */

#define	LOG_FORMAT		"%s [%2d] %-15s : %s\n"
#define LOG_CONT_FORMAT		"%29s : %s\n", ""
#define LOG_COMMENT_FORMAT	"%s      COMMENT         : %s\n"



static FILE *log_file = NULL;
static struct timeval time_zero;

/* Boolean Flags */
static int flush = 0, initialized = 0;


/* External Functions */
extern long netlog_write_total(), netlog_read_total();

void exit_function();

char *function_name[] = {
	/*   0 */	"socket()",		
	/*   1 */	"bind()",
	/*   2 */	"listen()",
	/*   3 */	"connect()",
	/*   4 */	"accept()",
	/*   5 */	"close()",
	/*   6 */	"setsockopt()",
	/*   7 */	"read()",
	/*   8 */	"write()",
	/*   9 */	"writev()",
	/*  10 */	"readv()",
	/*  11 */	"send()",
	/*  12 */	"recv()",
	/*  13 */	"sendto()",
	/*  14 */	"recvfrom()",
	/*  15 */	"sendmsg()",
	/*  16 */	"recvmsg()",
	/*  17 */	"socketpair()",
	/*  18 */	"shutdown()",
			""
	};

/*
	netlog(int socket_num, int function, char *message)

	Log a message with timestamp. Returns 0 on success, on failure
	calls fatal_error() and does not return.
*/

int netlog(socket_num, function,message)
	int socket_num, function;
	char *message;
{
	int rc;
	char timestamp[TIMESTAMP_LEN];


	if ( ! initialized )	/* Are we initialized? */
		netlog_initialize();

	get_timestamp(timestamp);

	switch(function) {

	case LOG_CONTINUE:
		rc = fprintf(log_file, LOG_CONT_FORMAT, message);
		break;

	case LOG_COMMENT:
		rc = fprintf(log_file, LOG_COMMENT_FORMAT, timestamp,
			message);
		break;

	default:
		rc = fprintf(log_file, LOG_FORMAT, timestamp,
			socket_num, function_name[function], message);
		break;
	}

	if (rc == EOF)
		fatal_error("Couldn't write to log.");	/* Will not return */

	if (flush)
		fflush(log_file);

	return 0;
}



/*
	netlog_initialize()

	Initialize netlog.

	Always returns 0, on failure it calls fatal_error() does not
	return.
*/
int netlog_initialize()
{
	char *log_filename, myhostname[MAXHOSTNAMELEN];

	if (initialized++)
		fatal_error("netlog_initialize() called twice.");
		/* Will not return */

	if ( (log_filename = getenv(FLUSH_VAR)) != NULL)
		flush++;

	/*	Try to set exit function if we can	*/
#ifdef sun
	on_exit(exit_function, NULL);

#else 
	/* Verified for sgi, cray and hp */

	atexit(exit_function);
#endif

	if ( (log_filename = getenv(LOG_FILE_VAR)) == NULL)
		log_filename = strdup(DEFAULT_LOG_FILE);

	log_filename = parse_filename(log_filename);

	if ( (log_file = fopen(log_filename, "a")) == NULL) {
		char *message;

		message = (char *) malloc(30 + strlen(log_filename));
		sprintf(message, "Couldn't open log file %s.", log_file);

		fatal_error(message);	/* Will not return */
	}

	/* Get start timestamp */
	start_timer(&time_zero);


	if (gethostname(myhostname, MAXHOSTNAMELEN) == -1)
		strcpy(myhostname, "Unknown Host");

	if (fprintf(log_file, "\n***** NETLOG: VERSION %s START LOG\n",
			NETLOG_VERSION) == EOF
		|| fprintf(log_file, "***** NETLOG: %s %s\n\n",
			myhostname, ctime(&(time_zero.tv_sec)))
				== EOF)

			fatal_error("Log header write failed.");
				/* Will not return */

	return 0;
}



/*
	get_timestamp(char *timestamp)

	Fill buffer pointed to by timestamp with a asci time stamp containing
	seocnds since program start.
*/
int get_timestamp(timestamp)
	char *timestamp;
{
	struct timeval t;


	time_since(&time_zero, &t);
	
	sprintf(timestamp, TIMESTAMP_FORMAT, num_seconds(&t));

	return 0;
}



/*
	netlog_comment()

	For use by the user to write comments into the log file.
*/
void netlog_comment(comment)
	char *comment;
{
	netlog(0, LOG_COMMENT, comment);

	return;
}



/*
	exit_fuinction()

	This function is hopefully called at exit of the application to
	clean up.
*/
void exit_function()
{
	fprintf(log_file, "\n\n***** NETLOG: Wrote %ld bytes total.\n",
		netlog_write_total());
	fprintf(log_file, "***** NETLOG: Read %ld bytes total.\n",
		netlog_read_total());
	fprintf(log_file, "***** NETLOG: END LOG.\n");

	fclose(log_file);

	return;
}



/*
	parse_filename()

	Parse the log filename replacing percent macros.
*/

#define REPLACE_LEN	256


char *parse_filename(filename)
	char *filename;
{
	char *buffer, replace[REPLACE_LEN];
	int buffer_size, percent_char;

	buffer = strdup(filename);
	buffer_size = strlen(buffer) + 1;

	if (buffer == NULL)
		fatal_error("malloc() failed.");

	replace_percent = replace;

	while(percent_char = percent_parse(buffer, buffer_size))
		switch(percent_char) {

		case -1:
			buffer_size += strlen(replace);
			buffer = realloc(buffer, buffer_size);
			break;

		case 'h':
			gethostname(replace, REPLACE_LEN);
			break;

		case 'p':
			sprintf(replace, "%d", getpid());
			break;

		case 'u':
			sprintf(replace, "%d", getuid());
			break;

		default:
			*replace = NULL;
			break;
		}

	return buffer;
}
