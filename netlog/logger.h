/*
	logger.h

	Header file to be included for using routines in logger.c
*/

/*	Our version
*/
#define NETLOG_VERSION		"1.0"

/*	Default log file
*/
#define	DEFAULT_LOG_FILE	"./netlog.%h.%p"

/*	Environment varibale for changing log file
*/
#define LOG_FILE_VAR		"NETLOGFILE"	/* env var to change file */

/*	Environment variabel for flushing of log file after each entry
*/
#define FLUSH_VAR		"NETLOGFLUSH"	/* env to set flush flag */
 


/*
	Logged functions
*/
#define	LOG_SOCKET		0
#define	LOG_BIND		1
#define LOG_LISTEN		2
#define LOG_CONNECT		3
#define LOG_ACCEPT		4
#define LOG_CLOSE		5
#define LOG_SETSOCKOPT		6
#define	LOG_READ		7
#define	LOG_WRITE		8
#define LOG_WRITEV		9
#define	LOG_READV		10
#define LOG_SEND		11
#define	LOG_RECV		12
#define	LOG_SENDTO		13
#define	LOG_RECVFROM		14
#define	LOG_SENDMSG		15
#define	LOG_RECVMSG		16
#define LOG_SOCKETPAIR		17
#define LOG_SHUTDOWN		18


/*
	Special flags
*/
#define LOG_CONTINUE		256
#define	LOG_COMMENT		257





void netlog_comment();
void exit_function();
char *parse_filename();
