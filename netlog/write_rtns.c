/*
	write_rtns.c

	This file contains routines that do logging for socket functions
	that do writing.
*/

#include "logger.h"
#include "support.h"
#include "timer.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>


static char message[256];

static struct timeval start, elapsed;

static long write_total = 0;
long netlog_write_total();


/*
	netlog_write_total()

	Return total bytes written.
*/
long netlog_write_total()
{
	return write_total;
}



/*
	log_write(s, buf, nbyte)

	Log a write() call.
*/
int log_write(s, buf, nbyte)
	int s;
	char *buf;
	int nbyte;
{
	int rc;

	
	if (! is_socket(s)) return write(s, buf,nbyte);

	start_timer(&start);

	rc = write(s, buf, nbyte);

	time_since(&start, &elapsed);

	if (rc == -1)
		sprintf(message, "Attempted %d bytes. %s", nbyte,
				desc_error());
	else {
		sprintf(message, "Wrote %d of %d bytes at %s", rc, nbyte,
				thoughput(rc, num_seconds(&elapsed)));

		write_total += rc;
	}

	netlog(s, LOG_WRITE, message);

	return rc;
}



#ifndef cray
/*
	int log_writev()

	Log a writev() call.
*/
int log_writev(s, iov, iovcnt)
	int s;
	struct iovec *iov;
	int iovcnt;
{
	int iovnum, total, rc;

	if ( ! is_socket(s) )
		return log_writev(s, iov, iovcnt);

	
	start_timer(&start);

	rc = writev(s, iov, iovcnt);

	time_since(&start, &elapsed);

	for (iovnum = 0, total = 0; iovnum < iovcnt; iovnum++)
		total += iov[iovnum].iov_len;

	if (rc == -1)
		sprintf(message, "Attempted %d bytes. %s", total,
				desc_error());
	else {
		sprintf(message, "Wrote %d of %d bytes at %s", rc, total,
				thoughput(rc, num_seconds(&elapsed)));

		write_total += rc;
	}

	netlog(s, LOG_WRITEV, message);
	
	return rc;
}
#endif



/*
	log_send()

	Log a send() call.
*/
int log_send(s, msg, len, flags)
	int s;
	char *msg;
	int len, flags;
{
	int rc;


	if (! is_socket(s)) return send(s, msg, len, flags);

	start_timer(&start);

	rc = send(s, msg, len, flags);

	time_since(&start, &elapsed);

	if (rc == -1)
		sprintf(message, "Attempted %d bytes. %s", len, desc_error());
	else {
		sprintf(message, "Wrote %d of %d bytes at %s", rc, len,
				thoughput(rc, num_seconds(&elapsed)));

		write_total += rc;
	}

	netlog(s, LOG_SEND, message);

	if (flags) {
		sprintf(message, "Flags: %s", desc_send_flags(flags));
		netlog(s, LOG_CONTINUE, message);
	}

	return rc;
}



/*
	log_sendto()

	Log a sendto() call.
*/
int log_sendto(s, msg, len, flags, to, tolen)
	int s;
	char *msg;
	int len, flags;
	struct sockaddr *to;
	int tolen;
{
	int rc;


	if (! is_socket(s)) return sendto(s, msg, len, flags, to, tolen);

	start_timer(&start);

	rc = sendto(s, msg, len, flags, to, tolen);

	time_since(&start, &elapsed);

	if (rc == -1)
		sprintf(message, "Attempted %d bytes to %s. %s", len,
			desc_sockaddr(to, tolen), desc_error());
	else {
		sprintf(message, "Wrote %d of %d bytes to %s at %s", rc, len,
			desc_sockaddr(to, tolen),
			thoughput(rc, num_seconds(&elapsed)));

		write_total += rc;
	}

	netlog(s, LOG_SENDTO, message);

	if (flags) {
		sprintf(message, "Flags: %s", desc_send_flags(flags));
		netlog(s, LOG_CONTINUE, message);
	}

	return rc;
}



/*
	log_sendmsg()

	Log a sendmsg() call.
*/
int log_sendmsg(s, msg, flags)
	int s;
	struct msghdr *msg;
	int flags;
{
	int rc, iovnum, total;


	if (! is_socket(s)) return sendmsg(s, msg, flags);

	start_timer(&start);

	rc = sendmsg(s, msg, flags);

	time_since(&start, &elapsed);

	for (iovnum = 0, total = 0; iovnum < msg->msg_iovlen; iovnum++)
		total += msg->msg_iov[iovnum].iov_len;

	if (rc == -1)
		sprintf(message, "Attemptd %s bytes to %s. %s", total,
			desc_sockaddr(msg->msg_name, msg->msg_namelen),
			desc_error());
	else {
		sprintf(message, "Wrote %d of %d bytes to %s at %s", rc, total,
			desc_sockaddr(msg->msg_name, msg->msg_namelen),
			thoughput(rc, num_seconds(&elapsed)));

		write_total += rc;
	}

	netlog(s, LOG_SENDMSG, message);

	if (flags) {
		sprintf(message, "Flags: %s", desc_send_flags(flags));
		netlog(s, LOG_CONTINUE, message);
	}

	return rc;
}
