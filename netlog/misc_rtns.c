/*
	misc_rtns.c

	This file contains routines that do logging for miscellaneous
	socket calls (socket, bind, accept, connect, close, listen,
	and setsockopt).
*/

#include "logger.h"
#include "support.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>


static char message[256];

/*
	log_socket(domain, type, protocol)

	Log a socket() call.
*/
int log_socket(domain, type, protocol)
	int domain, type, protocol;
{
	int rc;


	rc = socket(domain, type, protocol);

	if (rc  == -1)
		sprintf(message, "%s %s", desc_socket(domain, type, protocol),
			desc_error());
	else
		strcpy(message, desc_socket(domain, type, protocol));

	netlog(rc, LOG_SOCKET, message);

	return rc;
}



/*
	log_bind(int s, sockaddr *name,  int namelen);

	Log a bind() call.
*/
int log_bind(s, name, namelen)
	int s;
	struct sockaddr *name;
	int namelen;
{
	int rc;


	rc = bind(s, name, namelen);

	if (rc == -1)
		sprintf(message, "%s  %s", desc_sockaddr(name, namelen),
						desc_error());
	else
		strcpy(message, desc_sockaddr(name, namelen));

	netlog(s, LOG_BIND, message);

	return rc;
}



/*
	log_listen(s, backlog)

	Log a listen() call.
*/
int log_listen(s, backlog)
	int s, backlog;
{
	int rc;

	rc = listen(s, backlog);

	if (rc == -1)
		sprintf(message, "Backlog = %d, %s", backlog,
					desc_error());
	else
		sprintf(message, "Backlog = %d", backlog);

	netlog(s, LOG_LISTEN, message);

	return rc;
}



/*
	log_connect()

	Log a connect() call.
*/
int log_connect(s, name, namelen)
	int s;
	struct sockaddr *name;
	int namelen;
{
	int rc;


	rc = connect(s, name, namelen);

	if (rc == -1)
		sprintf(message, "%s %s", desc_sockaddr(name, namelen),
			desc_error());
	else
		strcpy(message, desc_sockaddr(name, namelen));

	netlog(s, LOG_CONNECT, message);

	return rc;
}



/*
	log_accept(s, addr, addrlen)

	Log a accept() call.
*/
int log_accept(s, addr, addrlen)
	int s;
	struct sockaddr *addr;
	int *addrlen;
{
	int rc;

	struct sockaddr address;
	int address_len = sizeof(address);

	/* We want the info in any case */
	if (addr == NULL) {
		addr = &address;
		addrlen = &address_len;
	}

	rc = accept(s, addr, addrlen);

	if (rc == -1) {
		strcpy(message, desc_error());
		netlog(s, LOG_ACCEPT, message);

	} else {
		char *from;

		from = desc_sockaddr(addr, *addrlen);

		sprintf(message, "Socket #%d from %s", rc, from);
		netlog(s, LOG_ACCEPT, message);

		sprintf(message, "From %s via socket #%d", from, s);
		netlog(rc, LOG_ACCEPT, message);
	}

	return rc;
}



/*
	log_close(s)

	Log a close() call.
*/
int log_close(s)
	int s;
{
	int rc;

	rc = close(s);

	if (is_socket(s)) {

		if (rc == -1)
			strcpy(message, desc_error());
		else
			sprintf(message, "");

		netlog(s, LOG_CLOSE, message);
	}

	return rc;
}	



/*
	log_setsockopt()

	Log a setsockopt() call.
*/
int log_setsockopt(s, level, optname, optval, optlen)
	int s, level, optname;
	char *optval;
	int optlen;
{
	int rc;

	rc = setsockopt(s, level, optname, optval, optlen);

	if (rc == -1)
		sprintf(message, "%s %s",
			desc_sockopt(level, optname, optval, optlen),
			desc_error());
	else
		strcpy(message, desc_sockopt(level, optname, optval, optlen));

	netlog(s, LOG_SETSOCKOPT, message);

	return rc;
}



/*
	int log_socketpair()

	Log a socketpair() call.
*/
int log_socketpair(domain, type, protocol, sv)
	int domain, type, protocol, *sv;
{
	int rc;

	rc = socketpair(domain, type, protocol, sv);

	if (rc == -1) {
		sprintf(message, "%s %s", desc_socket(domain, type, protocol),
			desc_error());
		netlog(rc, LOG_SOCKETPAIR, message);

	} else {
		sprintf(message, "%s Connected to %d",
			desc_socket(domain, type, protocol), sv[1]);
		netlog(sv[0], LOG_SOCKETPAIR, message);
		sprintf(message, "%s Connected to %d",
			desc_socket(domain, type, protocol), sv[0]);
		netlog(sv[1], LOG_SOCKETPAIR, message);
	}

	return rc;
}



/*
	log_shutdown()

	Log a shutdown() call.
*/
int log_shutdown(s, how)
	int s, how;
{
	int rc;
	char how_desc[30];

	rc = shutdown(s, how);

	switch(how) {

	case 0:	strcpy(how_desc, "Receive shutdown");		break;
	case 1:	strcpy(how_desc, "Send shutdown");		break;
	case 2:	strcpy(how_desc, "Send and Receive shutdown");	break;
	default:
		sprintf(how_desc, "Unknown how #%d", how);	break;
	}

	if (rc == -1) 
		sprintf(message, "%s %s", how_desc, desc_error());
	else
		strcpy(message, how_desc);

	netlog(s, LOG_SHUTDOWN, message);

	return rc;
}
