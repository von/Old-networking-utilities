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
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>



#define TIMESTAMP_FORMAT	"%6.3f"
#define TIMESTAMP_LEN		8	/* xx.xxx */

#define	LOG_FORMAT		"%s [%2d] %-15s : %s\n"
#define LOG_CONT_FORMAT		"%29s : %s\n", ""
#define LOG_COMMENT_FORMAT	"%s      COMMENT         : %s\n"
#define LOGFILE_NAME_FORMAT	"%s:%s"



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


/* Logfile Types */
#define	TYPE_FILE			0
#define	TYPE_SOCKET		1


struct socket_description {
	char hostname[BUFFER_SIZE];
	int port;
};

struct log_description {
	int log_type;
	struct socket_description sock;
	char filename[BUFFER_SIZE];
	
};


void netlog_comment();
void exit_function();
char *percent_expand();
FILE *open_log_stream();
struct log_description *parse_log_name();
FILE *open_log_file();
FILE *open_log_socket();


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

	if (rc < 0)
		fatal_error("Couldn't output to log.");	/* Will not return */

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



	if ( (log_filename = getenv(LOG_FILE_VAR)) == NULL)
		log_filename = strdup(DEFAULT_LOG_FILE);

	/* Expand all the run-time percent codes */
	log_filename = percent_expand(log_filename);


	log_file = open_log_stream(log_filename);


	/*	Try to set exit function if we can	*/

#if !defined(USE_ON_EXIT)
	atexit(exit_function);

#else	/* SunOS 4.x doesn't have atexit() */

	on_exit(exit_function, NULL);
#endif

	/* Get start timestamp */
	start_timer(&time_zero);


	if (gethostname(myhostname, MAXHOSTNAMELEN) == -1)
		strcpy(myhostname, "Unknown Host");

	if ((fprintf(log_file, "\n***** NETLOG: VERSION %s START LOG\n",
			NETLOG_VERSION) < 0) ||
	    (fprintf(log_file, "***** NETLOG: %s %s\n\n",
			myhostname, ctime(&(time_zero.tv_sec)))	< 0))

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
	if (log_file == NULL)
		return;

	fprintf(log_file, "\n\n***** NETLOG: Wrote %ld bytes total.\n",
		netlog_write_total());
	fprintf(log_file, "***** NETLOG: Read %ld bytes total.\n",
		netlog_read_total());
	fprintf(log_file, "***** NETLOG: END LOG.\n");

	fclose(log_file);

	return;
}



/*
	percent_expand()

	Parse the log filename replacing percent macros.
*/

#define REPLACE_LEN	256


char *percent_expand(filename)
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


/*
	open_log_stream()

	Open the stream to be used for logging.
*/

FILE *open_log_stream(logfile_name)
	char	*logfile_name;
{
	char	type_string[BUFFER_SIZE];
	char	name[BUFFER_SIZE];

	char	error_message[BUFFER_SIZE];

	int	type;

	FILE	*stream;

	struct log_description	*desc;


	desc = parse_log_name(logfile_name);

	switch(desc->log_type) {

  	case TYPE_FILE:
		stream = open_log_file(desc);
		break;

	case TYPE_SOCKET:
		stream = open_log_socket(desc);
		break;
	}

	return stream;
}



/*
        parse_log_name()

	Parse the log file string and return a pointer to a description.
*/
struct log_description *parse_log_name(name)
	char *name;
{
	char *type_string;
	char *token;

	struct log_description *desc;

	char *tmp_ptr;

	char	error_message[BUFFER_SIZE];


	desc = malloc(sizeof(*desc));

	if (desc == NULL) {
		fatal_error("malloc() failed.\n");
			/* Will not return */
	}

	type_string = strtok(name, ":");

	if (type_string == NULL) {
			/* Assume FILE */
		desc->log_type = TYPE_FILE;
		strcpy(desc->filename, DEFAULT_LOG_FILE);

	} else if (strcmp(type_string, "file") == 0) {
			/* FILE */

		desc->log_type = TYPE_FILE;

		token = strtok(NULL, ":");

		if (token == NULL)
			strcpy(desc->filename, DEFAULT_LOG_FILE);
		else
			strcpy(desc->filename, token);

	} else if (strcmp(type_string, "socket") == 0) {
			/* SOCKET */
			/* Format is: "socket:<filename>:<hostname>:<port>" */

		desc->log_type = TYPE_SOCKET;

		token = strtok(NULL, ":");	/* Parse Filename */

		if (token == NULL)
			strcpy(desc->filename, DEFAULT_LOG_FILE);
		else
			strcpy(desc->filename, token);

		token = strtok(NULL, ":");	/* Parse Hostname */

		if (token == NULL)
			strcpy(desc->sock.hostname, DEFAULT_LOG_HOST);
		else
			strcpy(desc->sock.hostname, token);

		token = strtok(NULL, ":");	/* Parse port number */

		if (token == NULL)
			desc->sock.port = DEFAULT_SOCKET_PORT;
		else
			desc->sock.port = atoi(token);

	}	else {

		sprintf(error_message, "Unknown type field \"%s\".",
			type_string);
		fatal_error(error_message);	/* Will not return */
	}

		/* Parse filename */
	tmp_ptr = percent_expand(desc->filename);
	strcpy(desc->filename, tmp_ptr);
	free(tmp_ptr);

	return desc;
}
	
		


/*
	open_log_file()

	Return a stream to a log file on the local filesystem.

	Format of the filename string is: <filename>
*/

FILE *open_log_file(desc)
	struct log_description *desc;
{
	FILE *stream;

	char	error_message[BUFFER_SIZE];


	if ( (stream = fopen(desc->filename, "a")) == NULL) {
		sprintf(error_message,
			"Couldn't open log file \"%s\".", desc->filename);

		fatal_error(error_message);	/* Will not return */
	}

	return stream;
}

	
/*
	open_log_socket()

	Open a socket connection to a loggerd server.

	Format of the name string is: :[<host>]:[<port>]:[<filename>]
*/
FILE *open_log_socket(desc)
	struct log_description *desc;
{
	char	error_message[BUFFER_SIZE];

	int sock;

	FILE *stream;

	struct sockaddr_in addr;

	struct hostent *host_info;


	bzero(addr, sizeof(addr));

	host_info = gethostbyname(desc->sock.hostname);

	if (host_info != NULL) {
		addr.sin_family = host_info->h_addrtype;
		bcopy(host_info->h_addr,
		      (char *) &(addr.sin_addr),
		      host_info->h_length);
		addr.sin_port = htons(desc->sock.port);

	} else {

		sprintf(error_message,
			"Could resolve hostname \"%s\".\n",
			desc->sock.hostname);

		fatal_error(error_message);	/* Will not return */
	}

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sock == -1)
		fatal_error("socket() failed.\n");	/* Will not return */


	if (connect(sock, &addr, sizeof(addr)) < 0) {
		sprintf(error_message,
			"Couldn't connect to \"%s\".\n",
			desc->sock.hostname);

		fatal_error(error_message);
	}

	if (sock == -1) {
		sprintf(error_message,
			"Couldn't connect to \"%s\" port %d.\n",
			desc->sock.hostname, desc->sock.port);

		fatal_error(error_message);	/* Will not return */
	}

	write(sock, desc->filename, BUFFER_SIZE);

	stream = fdopen(sock, "w");

	if (stream == NULL) {
		fatal_error("fdopen() failed.\n"); /* Will not return */
	}

	return stream;
}


