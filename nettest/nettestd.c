char *version = "$Id: nettestd.c,v 1.4 1995/03/07 22:08:02 vwelch Exp $";

#include "nettest.h"

#include <stdlib.h>
#include <sys/errno.h>
#include <signal.h>
#ifdef	WAIT3CODE
#include <sys/wait.h>
#endif
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef	CRAY2
#include <sys/sysmacros.h>
#endif
#include <netinet/tcp.h>


#ifdef	WAIT3CODE
/*
	This is the nicest way to handle children...
*/
void dochild()
{
	while (wait3(0, WNOHANG, 0) > 0)
		;
}
#else
/*
	Otherwise...
*/
#define	dochild	SIG_IGN
#endif

int dflag;
#define	debug(x)	if(dflag>1)fprintf x

int buffer_alignment = 0;
int do_load = 0;

#define	D_PIPE	1
#define	D_UNIX	2
#define	D_INET	3
#define	D_FILE	4

main(argc, argv)
int argc;
char **argv;
{
	register int	s, s2;
#ifdef	NAMEDPIPES
	register int	mode, dev1, dev2;
	char		buf[256];
#endif
	char		*portname = UNIXPORT;
	short		port = PORTNUMBER;
	int		domain = D_INET;
	int		type = SOCK_STREAM;
	int		namesize;
	int		on = 1;
	int		c;
	char		*f1, *f2;
	union {
		struct sockaddr		d_gen;
		struct sockaddr_un	d_unix;
		struct sockaddr_in	d_inet;
	} name;
	extern int	optind;
	extern char	*optarg;
	int		kbufsize = 0;


	while ((c = getopt(argc, argv, "b:p:dv")) != EOF) {
		switch(c) {
		case 'b':
			kbufsize = atoval(optarg);
			break;
		case 'd':
			++dflag;
			break;
		case 'p':
			if (!strcmp(optarg, "unix")) {
				domain = D_UNIX;
				type = SOCK_STREAM;
			} else if (!strcmp(optarg, "tcp")) {
				domain = D_INET;
				type = SOCK_STREAM;
			} else if (!strcmp(optarg, "udp")) {
				domain = D_INET;
				type = SOCK_DGRAM;
			} else if (!strcmp(optarg, "file")) {
				domain = D_FILE;
#ifdef	NAMEDPIPES
			} else if (!strcmp(optarg, "pipe")) {
				domain = D_PIPE;
#endif	/* NAMEDPIPES */
			} else {
				printf("Unknown protocol: %s\n", optarg);
				usage();
			}
			break;

		case 'v':
			printf("Version: %s\n", version);
			exit(0);

		case '?':
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (domain == D_FILE) {
		if (argc != 2)
			usage();
		f1 = *argv++;
		f2 = *argv++;
		argc -= 2;
	} else if (argc > 1)
		usage();
	else if (argc == 1) {
		portname = *argv++;
		argc--;
		port = atoi(portname);
	}

	switch (domain) {
#ifdef	NAMEDPIPES
	case D_PIPE:
		mode = S_IFIFO|0666;
		dev1 = dev2 = 0;
		sprintf(buf, "R%s", portname);
		umask(0);
		for(;;) {
			unlink(buf);
			if (mknod(buf, mode, dev1) < 0) {
				perror("mknod");
				exit(1);
			}
			buf[0] = 'W';
			unlink(buf);
			if (mknod(buf, mode, dev2) < 0) {
				perror("mknod");
				goto err1;
			}
			buf[0] = 'W';
			if ((s2 = open(buf, O_RDONLY)) < 0) {
				perror(buf);
				goto err2;
			}
			buf[0] = 'R';
			if ((s = open(buf, O_WRONLY)) < 0) {
				perror(buf);
				close(s2);
			err2:	buf[0] = 'R';
				unlink(buf);
			err1:	buf[0] = 'W';
				unlink(buf);
				exit(1);
			}
			data_stream(s2, s);
			close(s2);
			close(s);
		}
		break;
#endif	/* NAMEDPIPES */
	case D_FILE:
		for(;;) {
			s = open(f1, 0);
			if (s < 0) {
				perror(f1);
				exit(1);
			}
			s2 = open(f2, 1);
			if (s2 < 0) {
				perror(f2);
				exit(1);
			}
			data_stream(s, s2);
			close(s2);
			close(s);
			sleep(1);
		}
		/* break; */

	case D_UNIX:
		name.d_unix.sun_family = AF_UNIX;
		strcpy(name.d_unix.sun_path, portname);
		namesize = sizeof(name.d_unix) - sizeof(name.d_unix.sun_path)
				+ strlen(name.d_unix.sun_path);
		(void) unlink(portname);
		goto dosock;
		/* break; */

	case D_INET:
		name.d_inet.sin_family = AF_INET;
		if (port <= 0) {
			fprintf(stderr, "bad port number\n");
			exit(1);
		}
		name.d_inet.sin_port = htons(port);
#if	!defined(CRAY) || defined(s_addr)
		name.d_inet.sin_addr.s_addr = 0;
#else
		name.d_inet.sin_addr = 0;
#endif
		namesize = sizeof(name.d_inet);

	dosock:
		if ((s = socket(name.d_inet.sin_family, type, 0)) < 0) {
			perror("socket");
			exit(1);
		}
		if (dflag && setsockopt(s, SOL_SOCKET, SO_DEBUG,
					(char *) &on, sizeof(on)) < 0)
			perror("setsockopt - SO_DEBUG");
		setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
			   (char *) &on, sizeof(on));

		if (kbufsize) {
#ifdef	SO_SNDBUF
			if (setsockopt(s, SOL_SOCKET, SO_SNDBUF,
				       (char *) &kbufsize,
				       sizeof(kbufsize)) < 0)
				perror("setsockopt - SO_SNDBUF");
			if (setsockopt(s, SOL_SOCKET, SO_RCVBUF,
				       (char *) &kbufsize,
				       sizeof(kbufsize)) < 0)
				perror("setsockopt - SO_RCVBUF");
#else	/* !SO_SNDBUF */
			printf("-b: cannot set local buffer sizes\n");
#endif	/* SO_SNDBUF */
		}

		if (bind(s, (struct sockaddr *) &name, namesize) < 0) {
			perror("bind");
			exit(1);
		}
		if (type == SOCK_DGRAM)
			do_dgram(s);
		else
			do_stream(s);
		/*NOTREACHED*/
		break;
	}
}

do_stream(s)
register int s;
{
	register int		i, s2;
	struct sockaddr_in	name;
	int			namesize;

	
	listen(s, 5);

	signal(SIGCHLD, dochild);
	for (;;) {
		namesize = sizeof(name);
		s2 = accept(s, (struct sockaddr *) &name, &namesize);
		if (s2 < 0) {
			extern int errno;
			if (errno == EINTR)
				continue;
			perror("accept");
		} else {
#ifdef WAIT3CODE
			/* If we have wait3() we can handle child dying */
			if ((i = fork()) == 0) {
#else
			/* Otherwise we have parent die */
			if ((i = fork()) > 0) {
#endif
				close(s);
				i = data_stream(s2, s2);
				shutdown(s2, 2);
				exit(i);
			} else if (i < 0)
				perror("fork");
			close(s2);
		}
	}
}

data_stream(in, out)
register in, out;
{
	register int	i, t, offset = 0;
	register char	*cp, *data;
	char		buf[128];
	int		chunks = 0, chunksize = 0, fullbuf = 0, kbufsize = 0;
	int		tos = 0, nodelay = 0;

	for (cp = buf; ; ) {
		i = read(in, cp, 1);
		if (i != 1) {
			if (i < 0)
				perror("nettestd: read");
			else
				fprintf(stderr, "nettestd: Read returned %d, expected 1\n", i);
			exit(1);
		}
		if (*cp++ == '\n')
			break;
	}
	*cp = '\0';
	sscanf(buf, "%d %d %d %d %d %d %d %s", &chunks, &chunksize, &fullbuf,
	       &kbufsize, &tos, &nodelay, &buffer_alignment, &do_load);
	/*
	 * If fullbuf is set, allocate a buffer twice as big.  This
	 * is so that we can always read a full size buffer, from
	 * the offset of the last read.  This keeps the data in
	 * the first chunksize consistent in case the remote side
	 * is trying to verify the contents.
	 */
	data = valloc((fullbuf ? 2*chunksize : chunksize) + buffer_alignment);
	if (data == NULL) {
		sprintf(buf, "0 malloc() failed\n");
		write(out, buf, strlen(buf));
		return(1);
	}

	data += buffer_alignment;

	sprintf(buf, "1 %8.2f\n", (do_load ? get_load() : 0));
	    
	if (kbufsize) {
#ifdef	SO_SNDBUF
		if ((setsockopt(out, SOL_SOCKET, SO_SNDBUF,
				(char *) &kbufsize,
				sizeof(kbufsize)) < 0) ||
		    (setsockopt(in, SOL_SOCKET, SO_RCVBUF,
				(char *) &kbufsize,
				sizeof(kbufsize)) < 0))
#endif
			strcat(buf, " Cannot set buffers sizes.");
	}
	if (tos) {
#ifdef	IP_TOS
		if (setsockopt(out, IPPROTO_IP, IP_TOS,
			       (char *) &tos, sizeof(tos)) < 0)
#endif
			strcat(buf, " Cannot set TOS bits.");
	}
	if (nodelay) {
#ifdef	TCP_NODELAY
		if (setsockopt(out, IPPROTO_TCP, TCP_NODELAY,
			       (char *) &nodelay,
			       sizeof(nodelay)) < 0)
#endif
			strcat(buf, " Cannot set TCP_NODELAY.");
	}
	strcat(buf, "\n");
	write(out, buf, strlen(buf));
	for (i = 0; i < chunks || offset; i++) {
		if ((t = read(in, data + offset, chunksize)) < 0) {
			sprintf(buf, "read #%d.%d\n", i+1, chunksize);
			goto bad;
		}
		if (t == 0) {
			fprintf(stderr, "server: EOF on read, block # %d\n", i);
			break;
		}
/*@*/		debug((stderr, "server: %d: read %d\n", i, t));
		if (fullbuf) {
			offset += t;
			if (offset >= chunksize)
				offset -= chunksize;
			else
				--i;
		} else while (t != chunksize) {
			register int t2;
			t2 = read(in, data+t, chunksize-t);
			if (t2 < 0) {
				sprintf(buf, "read #%d.%d\n", i+1, chunksize-t);
				goto bad;
			}
			if (t2 == 0) {
				fprintf(stderr, "server: EOF on read, block # %d\n", i);
				break;
			}
			t += t2;
/*@*/			debug((stderr, "server: %d: partial read %d (%d)\n", i, t2, t));
		}
	}
	for (i = 0; i < chunks; i++) {
		if ((t = write(out, data, chunksize)) < 0) {
			sprintf(buf, "write #%d\n", i+1);
			goto bad;
		}
		if (t != chunksize)
			fprintf(stderr, "server: write: %d vs %d\n", t, chunksize);
/*@*/		else
/*@*/			debug((stderr, "server: %d: write %d\n", i, t));
	}
	free(data);
	return(0);
bad:
	perror(buf);
	free(data);
	return(1);
}

#define	MAXSIZE	(64*1024)

do_dgram(s)
int s;
{
	register int		t;
	register char		*cp, *data;
	char			*inet_ntoa();
	struct sockaddr_in	name;
	int			namesize;

	data = valloc(MAXSIZE + buffer_alignment);
	if (data == NULL) {
		fprintf(stderr, "no malloc\n");
		shutdown(s, 2);
		exit(1);
	}
	data += buffer_alignment;

	for (;;) {
		namesize = sizeof(name);
		t = recvfrom(s, data, MAXSIZE, 0,
			     (struct sockaddr *) &name, &namesize);
		if (t < 0) {
			perror("recvfrom");
			continue;
		}

		cp = inet_ntoa(name.sin_addr);
		printf("got %d bytes from %s\n", t, cp);
	}
}

usage()
{
	fprintf(stderr,
"Usage: nettestd <options> [-p tcp] [port]\n\
       nettestd <options> -p udp [port]\n\
       nettestd <options> -p unix|pipe [filename]\n\
       nettestd <options> -p file readfile writefile\n\
\n\
       Options:\n");
	fprintf(stderr,
"           -A <alignment>	Align buffers\n");
	fprintf(stderr, 
"           -b <size>		Set socket buffer size\n");
	fprintf(stderr,
"           -d			Turn on debugging\n");
	exit(1);
}
