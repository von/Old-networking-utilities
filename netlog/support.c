/*
	support.c

	This file contains support routines for the netlog library.
*/


#include "support.h"

#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


/*
 It seems like these should be in a system include file somewhere, but
 I can't find 'em.
*/
extern int sys_nerr;
extern char *sys_errlist[];



/*
	fatal_error(char *message)

	Spit out a error message to standard error and die.

	DOES NOT RETURN.
*/
int fatal_error(message)
	char *message;
{
	fprintf(stderr, "\n\nNETLOG FATAL ERROR -- ABORTING.\n");
	fprintf(stderr, "\t%s\n\t%s\n\n\n", message, desc_error());

	exit(1);
}



/*
	desc_error()

	Return a string describing the current error as indicated by errno.
*/
char *desc_error()
{
	static char description[80];

	if (errno > sys_nerr) 
		sprintf(description, "ERROR: %d", errno);
	else
		sprintf(description, "ERROR: (%d) %s", errno,
				sys_errlist[errno]);

	return description;
}



/*
	char *desc_sockaddr(struct sockaddr *name, int namelen)

	Return a ascii description of a sockaddr.
*/
char *desc_sockaddr(name, namelen)
	struct sockaddr *name;
	int namelen;
{
	static char description[60];
	

	switch(name->sa_family) {

	case AF_UNSPEC:
		sprintf(description, "Family unspecified");
		break;

	case AF_UNIX:
		sprintf(description, "UNIX family");
		break;
	
	case AF_INET:
		{
		
		struct sockaddr_in *in_name;

		in_name = (struct sockaddr_in *) name;

		sprintf(description, "%s:%d", inet_ntoa(in_name->sin_addr),
				in_name->sin_port);

		}
		break;

	default:
		sprintf(description, "Unknown Family (%d)", name->sa_family);
		break;
	}

	return description;
}



/*
	desc_socket()

	Given a domain, type and protocol this function resturns an ascii
	string describing them.
*/
char *desc_socket(domain, type, protocol)
	int domain, type, protocol;
{
	static char description[80];
	char domain_name[25], type_name[25], protocol_name[25];
	struct protoent *proto;

	switch(domain) {

#ifdef PF_UNIX
	case PF_UNIX:		sprintf(domain_name, "UNIX");	break;
#endif
#ifdef PF_INET
	case PF_INET:		sprintf(domain_name, "INET");	break;
#endif
#ifdef PF_IMPLINK
	case PF_IMPLINK:	sprintf(domain_name, "IMP");	break;
#endif
	default:
		sprintf(domain_name, "Unknown Domain #%d", domain);
	}

	switch(type) {

#ifdef SOCK_STREAM
	case SOCK_STREAM:	sprintf(type_name, "STREAM");	break;
#endif
#ifdef SOCK_DGRAM
	case SOCK_DGRAM:	sprintf(type_name, "DGRAM");	break;
#endif
#ifdef SOCK_RAW
	case SOCK_RAW:		sprintf(type_name, "RAW");	break;
#endif
#ifdef SOCK_SEQPACKET
	case SOCK_SEQPACKET:	sprintf(type_name, "SEQPACKET");	break;
#endif
#ifdef SOCK_RDM
	case SOCK_RDM:		sprintf(type_name, "RDM");	break;
#endif
	default:
		sprintf(type_name, "Unknown Type #%d", type);
	}

	if ( (proto = getprotobynumber(protocol)) == NULL)
		sprintf(protocol_name, "Protocol #%d", protocol);
	else
		sprintf(protocol_name, "%s(%d)", proto->p_name, protocol);

	sprintf(description, "%s, %s, %s", domain_name, type_name,
				protocol_name);

	return description;
}



/*
	desc_sockopt()

	Return a buffer containing a string with an ascii description of
	a given socket option.
*/
char *desc_sockopt(level, optname, optval, optlen)
	int level, optname;
	char *optval;
	int optlen;
{
	static char description[60];


	switch(level) {

#ifdef SOL_SOCKET
	case SOL_SOCKET:	desc_socket_level_opt(description, optname,
						optval, optlen);
				break;
#endif
#ifdef IPPROTO_IP
	case IPPROTO_IP:	desc_ip_level_opt(description, optname,
						optval, optlen);
				break;
#endif
#ifdef IPPROTO_TCP
	case IPPROTO_TCP:	desc_tcp_level_opt(description, optname,
						optval, optlen);
				break;
#endif
	default:		sprintf(description,
					"Unknown option (level %d # %d)",
					level, optname);
				break;
	}

	return description;
}
			


/*
	desc_socket_level_opt()

	Fill a buffer with an ascii description of a socket-level option.

	Always returns 0;
*/
int desc_socket_level_opt(description, optname, optval, optlen)
	char *description;
	int optname;
	char *optval;
	int optlen;
{

	switch(optname) {

#ifdef SO_DEBUG
	case SO_DEBUG:		sprintf(description, "SO_DEBUG"); break;
#endif
#ifdef SO_REUSEADDR
	case SO_REUSEADDR:	sprintf(description, "SO_REUSEADDR"); break;
#endif
#ifdef SO_KEEPALIVE
	case SO_KEEPALIVE:	sprintf(description, "SO_KEEPALIVE"); break;
#endif
#ifdef SO_DONTROUTE
	case SO_DONTROUTE:	sprintf(description, "SO_DONTROUTE"); break;
#endif
#ifdef SO_LINGER
	case SO_LINGER:		sprintf(description, "SO_LINGER"); break;
#endif
#ifdef SO_BROADCAST
	case SO_BROADCAST:	sprintf(description, "SO_BROADCAST"); break;
#endif
#ifdef SO_OOBINLINE
	case SO_OOBINLINE:	sprintf(description, "SO_OOBINLINE"); break;
#endif
#ifdef SO_SNDBUF
	case SO_SNDBUF:		sprintf(description, "SO_SNDBUF - %d",
					*((int *) optval));
				break;
#endif
#ifdef SO_RCVBUF
	case SO_RCVBUF:		sprintf(description, "SO_RCVBUF - %d",
					*((int *) optval));
				break;
#endif
#ifdef SO_TYPE
	case SO_TYPE:		sprintf(description, "SO_TYPE"); break;
#endif
#ifdef SO_ERROR
	case SO_ERROR:		sprintf(description, "SO_ERROR"); break;
#endif
	default:		sprintf(description, "Unknown socket-level #%d",
					optname);
				break;
	}

	return 0;
}
	


/*
	desc_ip_level_opt()

	Fill a buffer with an ascii description of a ip-level option.

	Always returns 0;
*/
int desc_ip_level_opt(description, optname, optval, optlen)
	char *description;
	int optname;
	char *optval;
	int optlen;
{

	switch(optname) {
#ifdef IP_TOS
	case IP_TOS:		sprintf(description, "IP_TOS - %d",
					*((int *) optval));
				break;
#endif
	default:		sprintf(description, "Unknown ip-level #%d",
					optname);
				break;
	}

	return 0;
}



/*
	desc_tcp_level_opt()

	Fill a buffer with an ascii description of a tcp-level option.

	Always returns 0;
*/
int desc_tcp_level_opt(description, optname, optval, optlen)
	char *description;
	int optname;
	char *optval;
	int optlen;
{

	switch(optname) {
#ifdef TCP_WINSHIFT
	case TCP_WINSHIFT:	sprintf(description, "TCP_WINSHIFT - %d",
					*((int *) optval));
				break;
#endif
#ifdef TCP_NODELAY
	case TCP_NODELAY:	sprintf(description, "TCP_NODELAY - %d",
					*((int *) optval));
				break;
#endif
	default:		sprintf(description, "Unknown tcp-level #%d",
					optname);
				break;
	}

	return 0;
}



/*
	desc_send_flags()

	Return an ascii description of flags for a send() call.
*/
char *desc_send_flags(flags)
	int flags;
{
	static char description[80];

	strcpy(description, "");

	while(flags) {
#ifdef MSG_OOB
		if (flags | MSG_OOB) {
			strcat(description, "MSG_OOB");
			flags ^= MSG_OOB;
		} else
#endif
#ifdef MSG_DONTROUTE
		if (flags | MSG_DONTROUTE) {
			strcat(description, "MSG_DONTROUTE");
			flags ^= MSG_DONTROUTE;
		} else
#endif
		{
			char tmp[10];
			
			sprintf(tmp, "%d", flags);
			flags = 0;
			strcat(description, tmp);
		}

		if (flags)
			strcat(description, ",");
	}

	return description;
}



/*
	desc_recv_flags()

	Return an ascii description of flags for a recv() call.
*/
char *desc_recv_flags(flags)
	int flags;
{
	static char description[80];

	strcpy(description, "");

	while(flags) {
#ifdef MSG_OOB
		if (flags | MSG_OOB) {
			strcat(description, "MSG_OOB");
			flags ^= MSG_OOB;
		} else
#endif
#ifdef MSG_PEEK
		if (flags | MSG_PEEK) {
			strcat(description, "MSG_PEEK");
			flags ^= MSG_PEEK;
		} else
#endif
		{
			char tmp[10];
			
			sprintf(tmp, "%d", flags);
			flags = 0;
			strcat(description, tmp);
		}

		if (flags)
			strcat(description, ",");
	}

	return description;
}



/*
	is_socket(s)

	Return 1 if s is a socket, 0 otherwise.
*/
int is_socket(s)
	int s;
{
	struct sockaddr name;
	int namelen = sizeof(name);

	if (getsockname(s, &name, &namelen) == -1)
		if (errno == ENOTSOCK)
			return 0;

	return 1;
}



/*
	thoughput(buf_size, time)

	Given an amount of data and time, this function returns a string
	containing the throughput.

	Currently handles up to 10 Gbits/s.
*/
char *thoughput(data_size, time)
	int data_size;		/* In bytes */
	float time;		/* In seconds */
{
	float tput;
	static char buffer[25];

	tput = ((float) data_size / time) * ( (float) EIGHT_BITS / MEGA);

	sprintf(buffer, "%8.3f MBits/s", tput);

	return buffer;
}
