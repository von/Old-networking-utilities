/*
	logger.h

	Header file to be included for using routines in logger.c
*/

/*	Our version
*/
#define NETLOG_VERSION			"1.1"

/*	Default log file
*/
#define	DEFAULT_LOG_FILE		"file:./netlog.%h.%p"

/*	Default log host
*/
#define	DEFAULT_LOG_HOST		"localhost"

/*	Default socket port
*/
#define	DEFAULT_SOCKET_PORT	 	5555

/*	Environment variable for changing log file
*/
#define LOG_FILE_VAR			"NETLOGFILE"

/*	Environment variable for flushing of log file after each entry
*/
#define FLUSH_VAR			"NETLOGFLUSH"

/*	Generic buffer size.

	This is the length of a buffer usedto pass values from the netlog
	library to loggerd, so don't change lightly, as it will make new
	versions incompatible with old.
*/
#define BUFFER_SIZE			256


/*
	Logged functions
*/
#define	LOG_SOCKET		0
#define	LOG_BIND		1
#define	LOG_LISTEN		2
#define	LOG_CONNECT		3
#define	LOG_ACCEPT		4
#define	LOG_CLOSE		5
#define	LOG_SETSOCKOPT		6
#define	LOG_READ		7
#define	LOG_WRITE		8
#define	LOG_WRITEV		9
#define	LOG_READV		10
#define	LOG_SEND		11
#define	LOG_RECV		12
#define	LOG_SENDTO		13
#define	LOG_RECVFROM		14
#define	LOG_SENDMSG		15
#define	LOG_RECVMSG		16
#define	LOG_SOCKETPAIR		17
#define	LOG_SHUTDOWN		18


/*
	Special flags
*/
#define	LOG_CONTINUE		256
#define	LOG_COMMENT		257

