/*
	read_rtns.c

	This file contains routines that do logging for socket functions
	that do reading.
*/

#include "logger.h"
#include "support.h"
#include "timer.h"

#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>


static char message[256];

static struct timeval start, elapsed;

static long read_total = 0;
long get_read_total();


/*
	netlog_read_total()

	Return total bytes read.
*/
long netlog_read_total()
{
	return read_total;
}



/*
	log_read(s, buf, nbyte)

	Log a read() call.
*/
int log_read(s, buf, nbyte)
	int s;
	char *buf;
	int nbyte;
{
	int rc;

	
	if (! is_socket(s))
		return read(s, buf, nbyte);

	start_timer(&start);

	rc = read(s, buf, nbyte);

	time_since(&start, &elapsed);

	if (rc == -1)
		sprintf(message, "Attempted %d bytes. %s", nbyte,
				desc_error());
	else {
		sprintf(message, "Read %d of %d bytes at %s", rc, nbyte,
				thoughput(rc, num_seconds(&elapsed)));

		read_total += rc;
	}

	netlog(s, LOG_READ, message);

	return rc;
}



#ifndef cray
/*
	int log_readv()

	Log a readv() call.
*/
int log_readv(s, iov, iovcnt)
	int s;
	struct iovec *iov;
	int iovcnt;
{
	int iovnum, total, rc;
	

	if (! is_socket(s))
		return readv(s, iov, iovcnt);

	start_timer(&start);

	rc = readv(s, iov, iovcnt);

	time_since(&start, &elapsed);

	for(iovnum = 0, total = 0; iovnum < iovcnt; iovnum++)
		total += iov[iovnum].iov_len;

	if (rc == -1)
		sprintf(message, "Attempted %d bytes. %s", total,
				desc_error());
	else {
		sprintf(message, "Read %d of %d bytes at %s", rc, total,
				thoughput(rc, num_seconds(&elapsed)));

		read_total += rc;
	}

	netlog(s, LOG_READV, message);

	return rc;
}
#endif



/*
	log_recv()

	Log a recv() call.
*/
int log_recv(s, buf, len, flags)
	int s;
	char *buf;
	int len, flags;
{
	int rc;

	if (! is_socket(s)) return recv(s, buf, len, flags);

	start_timer(&start);

	rc = recv(s, buf, len, flags);

	time_since(&start, &elapsed);

	if (rc == -1)
		sprintf(message, "Attempted %s bytes. %s", len, desc_error());
	else {
		sprintf(message, "Read %d of %d bytes at %s", rc, len,
				thoughput(rc, num_seconds(&elapsed)));

		read_total += rc;
	}

	netlog(s, LOG_RECV, message);

	if (flags) {
		sprintf(message, "Flags: %s", desc_send_flags(flags));
		netlog(s, LOG_CONTINUE, message);
	}

	return rc;
}



/*
	int log_recvfrom()

	Log a recvfrom() call.
*/
int log_recvfrom(s, buf, len, flags, from, fromlen)
	int s;
	char *buf;
	int len, flags;
	struct sockaddr *from;
	int *fromlen;
{
	int rc;

	struct sockaddr from_addr;
	int from_addr_len;

	if (! is_socket(s)) return recvfrom(s, buf, len, flags, from, fromlen);

	/* We want the info in any case */
	if (from == NULL)	from = &from_addr;
	if (fromlen == NULL)	fromlen = &from_addr_len;
	
	start_timer(&start);

	rc = recvfrom(s, buf, len, flags, from, fromlen);

	time_since(&start, &elapsed);

	if (rc == -1)
		sprintf(message, "Attempted %s bytes. %s", len, desc_error());
	else {
		sprintf(message, "Read %d of %d bytes at %s", rc, len,
				thoughput(rc, num_seconds(&elapsed)));

		read_total += rc;
	}

	netlog(s, LOG_RECVFROM, message);

	sprintf(message, "From: %s", desc_sockaddr(from, fromlen));
	netlog(s, LOG_CONTINUE, message);

	if (flags) {
		sprintf(message, "Flags: %s", desc_send_flags(flags));
		netlog(s, LOG_CONTINUE, message);
	}

	return rc;
}



/*
	int log_recvmsg(s, msg, flags)

	Log a recvmsg() call.
*/
int log_recvmsg(s, msg, flags)
	int s;
	struct msghdr *msg;
	int flags;
{
	int rc, iovnum, total;

	struct sockaddr from_addr;
	int addr_null = 0;	/* Flags so we know to cleanup */

	if (! is_socket(s)) return recvmsg(s, msg, flags);

	/* We want the info in any case */
	if (msg->msg_name == NULL) {
		msg->msg_name = (caddr_t) &from_addr;
		msg->msg_namelen = sizeof(from_addr);
		addr_null++;
	}

	start_timer(&start);

	rc = recvmsg(s, msg, flags);

	time_since(&start, &elapsed);

	for (iovnum = 0, total = 0; iovnum < msg->msg_iovlen; iovnum++)
		total += msg->msg_iov[iovnum].iov_len;

	if (rc == -1)
		sprintf(message, "Attempted %s bytes. %s", total,
					desc_error());
	else {
		sprintf(message, "Read %d of %d bytes at %s", rc, total,
				thoughput(rc, num_seconds(&elapsed)));

		read_total += rc;
	}

	netlog(s, LOG_RECVMSG, message);

	sprintf(message, "From: %s", desc_sockaddr(msg->msg_name,
			msg->msg_namelen));
	netlog(s, LOG_CONTINUE, message);

	if (flags) {
		sprintf(message, "Flags: %s", desc_send_flags(flags));
		netlog(s, LOG_CONTINUE, message);
	}

	return rc;
}
