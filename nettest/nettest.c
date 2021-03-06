/*
static char USMID[] = "@(#)tcp/usr/etc/nettest/nettest.c	61.1	09/13/90 09:04:50";
*/

char *version_str = "$Id: nettest.c,v 1.15 1997/02/17 20:28:03 vwelch Exp $";

#include "nettest.h"
#include <stdlib.h>
#include <string.h>

#ifdef BSD44
# include <machine/endian.h>
# include <netinet/in_systm.h>
#endif

#include <sys/un.h>
#include <netinet/tcp.h>

#ifdef	DO_IP_TOS
# ifdef NEED_IN_SYSTM_H
#  include <netinet/in_systm.h>
# endif
# include <netinet/ip.h>
#endif

#ifdef NEED_SYS_RESOURCE_H
# include <sys/resource.h>
#endif

#ifdef NEED_TIME_H
# include <time.h>
#endif

#ifdef NEED_UNISTD_H
# include <unistd.h>
#endif


union {
	struct sockaddr		d_gen;
	struct sockaddr_in	d_inet;
	struct sockaddr_un	d_unix;
} name;

int	nchunks = NCHUNKS;
int	chunksize = CHUNK;
int	dflag = 0;
int	checkdata = 0;
int	hash = 0;
int	fullbuf = 0;
int	kbufsize = 0;
int	nodelay = 0;
int	buffer_alignment = 0;
int	do_load = 0;
int do_histogram = 0;

#define	D_PIPE	1
#define	D_UNIX	2
#define	D_INET	3
#define	D_FILE	4

#if !defined(DONT_PROTO_TIMES)
clock_t	times();
#endif

#ifdef USE_FTIME
# define	GETTIMES(ts)	ftime(&((ts).ellapsed)); times(&((ts).tms));

typedef struct {
	struct timeb	ellapsed;
	struct tms		tms;
} time_struct;

#else /* !USE_FTIME */
# define	GETTIMES(ts)	((ts).ellapsed) = times(&((ts).tms));

typedef struct {
	long			ellapsed;
	struct tms		tms;
} time_struct;

#endif

#ifdef	TCP_WINSHIFT
int	winshift		= 0;
int	usewinshift		= 0;
#endif

int	tos				= 0;
int	maxchildren		= 0;


struct in_addr hisaddr;


static void do_children();
static void do_stream();
static void usage();
static void do_dgram();
static void prtimes();


main(argc, argv)
int argc;
char **argv;
{
	register int	s, s2, port = PORTNUMBER;
	struct hostent	*gethostbyname(), *hp;
	char		*hisname, _myname[256], *myname = _myname;
	register char	*portname = UNIXPORT;
	int		type = SOCK_STREAM;
	int		domain = D_INET;
	int		i;
	int		namesize;
	int		on = 1;
	int		nconnections = 1;
	extern char	*optarg;
	extern int	optind;

#ifdef TCP_MAXSEG
	int		maxseg;
	int		maxseglen = sizeof(maxseg);
#endif

#ifndef DONT_HAVE_GETDTABLESIZE
	maxchildren = getdtablesize() - 4;

#else
	{
		struct rlimit rlp;

		getrlimit(RLIMIT_NOFILE, &rlp);
		
		maxchildren = rlp.rlim_cur - 4;
	}
#endif /* DONT_HAVE_GETDTABLESIZE */

	while ((i = getopt(argc, argv, "A:b:cdfFhHln:p:s:t:v?")) != EOF) {
		switch(i) {
		case 'A':
		  	buffer_alignment = atoi(optarg);
			if (buffer_alignment < 0) {
			  fprintf(stderr,
				  "Buffer alignment of %d illegal. Ignoring.\n");
			  buffer_alignment = 0;
			}
			break;
		case 'b':
			kbufsize = atoval(optarg);
			break;
		case 'c':
			++checkdata;
			break;
		case 'd':
			dflag++;
			break;
		case 'f':
			fullbuf++;
			break;
		case 'F':
#ifdef	TCP_NODELAY
			nodelay++;
#else
			fprintf(stderr, "TCP nodelay option not supported\n");
			usage();
#endif
			break;
		case 'h':
			++hash;
			break;
		case 'H':
			do_histogram++;
			break;
		case 'l':
			do_load++;
			break;
		case 'n':
			nconnections = atoi(optarg);
			if (nconnections < 1 || nconnections > maxchildren) {
				fprintf(stderr,
					"-n: value must be between 1 and %d\n",
					maxchildren);
				usage();
			}
			break;
		case 'p':
			if (!strcmp(optarg, "tcp")) {
				domain = D_INET;
				type = SOCK_STREAM;
			} else if (!strcmp(optarg, "udp")) {
				domain = D_INET;
				type = SOCK_DGRAM;
				nchunks = 1;
			} else if (!strcmp(optarg, "unix")) {
				domain = D_UNIX;
				type = SOCK_STREAM;
			} else if (!strcmp(optarg, "pipe")) {
				domain = D_PIPE;
			} else if (!strcmp(optarg, "file")) {
				domain = D_FILE;
			} else {
				printf("Unknown protocol: %s\n", optarg);
				usage();
			}
			break;
		case 's':
#ifdef	TCP_WINSHIFT
			usewinshift++;
			winshift = atoval(optarg);
			if (winshift < -1 || winshift > 14) {
				fprintf(stderr, "window shift (-s) must be beteen -1 and 14\n");
				usage();
			}
#else
			fprintf(stderr, "window shift option not supported\n");
			usage();
#endif
			break;
		case 't':
#ifdef	DO_IP_TOS
			if (strcmp(optarg, "lowdelay") == 0) {
#ifdef IPTOS_LOWDELAY
			  tos |= IPTOS_LOWDELAY;
#else
			  fprintf(stderr, "IPTOS_LOWDELAY not supported.\n");
#endif
			} else if (strcmp(optarg, "throughput") == 0) {
#ifdef IPTOS_THROUGHPUT
			  tos |= IPTOS_THROUGHPUT;
#else
			  fprintf(stderr, "IPTOS_THROUGHPUT not supported.\n");
#endif
			} else if (strcmp(optarg, "reliability") == 0) {
#ifdef IPTOS_RELIABILITY
			  tos |= IPTOS_RELIABILITY;
#else
			  fprintf(stderr, "IPTOS_THROUGHPUT not supported.\n");
#endif
			} else {
			  tos = strtol(optarg, NULL, 0);
			}
#else
			fprintf(stderr, "TOS (-s) option not supported\n");
			usage();
#endif
			break;
		case 'v':
			printf("Version: %s\n", version_str);
			exit(0);

		case '?':
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (domain == D_INET) {
		hisname = myname;
		if (argc) {
			if (strcmp(*argv, "-"))
				hisname = *argv;
			--argc; ++argv;
		}
	} else if (domain == D_FILE) {
		if (argc < 2)
			usage();
		myname = *argv++;
		hisname = *argv++;
		argc -= 2;
	}
	if (argc) {
		if (strcmp(*argv, "-"))
			nchunks = atoval(*argv);
		--argc; ++argv;
		if (argc) {
			if (strcmp(*argv, "-"))
				chunksize = atoval(*argv);
			--argc; ++argv;
			if (argc) {
				if (strcmp(*argv, "-")) {
					port = atoi(*argv);
					portname = *argv;
				}
				if (argc > 1)
					usage();
			}
		}
	}
	

	switch(domain) {
	case D_PIPE:
		if (nconnections > 1) {
			fprintf(stderr, "-n flag not supported for pipe\n");
			usage();
		}
		sprintf(myname, "W%s", portname);
		
		if (((s2 = open(myname, 1)) < 0) ||
		    ((myname[0] = 'R'),((s = open(myname, 0)) < 0))) {
			perror(myname);
			close(s2);
			exit(1);
		}
		break;
	case D_FILE:
		if (nconnections > 1) {
			fprintf(stderr, "-n flag not supported for file\n");
			usage();
		}
		s2 = open(myname, 1);
		if (s2 < 0) {
			perror(myname);
			exit(1);
		}
		s = open(hisname, 0);
		if (s < 0) {
			perror(hisname);
			exit(1);
		}
		break;
	case D_UNIX:
		name.d_unix.sun_family = AF_UNIX;
		strcpy(name.d_unix.sun_path, portname);
		namesize = sizeof(name.d_unix) - sizeof(name.d_unix.sun_path)
			+ strlen(name.d_unix.sun_path);
		goto dosock;
		/* break; */
	case D_INET:
		if (nconnections > 1 && type != SOCK_STREAM) {
			fprintf(stderr, "-n flag not supported for udp\n");
			usage();
		}
		namesize = sizeof(name.d_inet);
		name.d_inet.sin_family = AF_INET;
		gethostname(_myname, sizeof(_myname));
		if ((hp = gethostbyname(hisname)) == NULL) {
			INADDR_TYPE tmp;

			tmp = inet_addr(hisname);
			if (tmp == INADDR_NONE) {
				fprintf(stderr, "no host entry for %s\n",
					hisname);
				exit(1);
			}
#if	defined(USE_S_ADDR)
			name.d_inet.sin_addr.s_addr = tmp;
#else
			name.d_inet.sin_addr = tmp;
#endif
		} else {
#if	defined(USE_S_ADDR)
			bcopy(hp->h_addr, (char *)&name.d_inet.sin_addr,
							hp->h_length);
#else
			INADDR_TYPE	tmp;
			bcopy(hp->h_addr, (char *)&tmp, hp->h_length);
			name.d_inet.sin_addr = tmp;
#endif
		}
		name.d_inet.sin_port = htons(port);
	dosock:
		if (nconnections > 1)
			do_children(nconnections);
		s = socket(name.d_gen.sa_family, type, 0);
		if (s < 0) {
			perror("socket");
			exit(1);
		}
		if (dflag)
		   if (setsockopt(s, SOL_SOCKET, SO_DEBUG,
				  (char *) &on, sizeof(on)) < 0)
			perror("setsockopt - SO_DEBUG");
#ifdef	DO_IP_TOS
		if (tos)
		   if (setsockopt(s, IPPROTO_IP, IP_TOS,
				  (char *) &tos, sizeof(tos)) < 0)
			perror("setsockopt - IP_TOS");
#endif
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
#ifdef	TCP_WINSHIFT
		if (usewinshift) {
			int size;

			if (setsockopt(s, IPPROTO_TCP, TCP_WINSHIFT, 
				       (char *) &winshift,
				       sizeof(winshift)) < 0)
				perror("setsockopt - TCP_WINSHIFT");

			if (getsockopt(s, IPPROTO_TCP, TCP_WINSHIFT, 
				       (char *) &winshift, &size) < 0)
				perror("getsockopt - TCP_WINSHIFT");
			else
				printf("nettest: TCP_WINSHIFT = %d\n", winshift);
		}
#endif
#ifdef	TCP_RFC1323
		if (setsockopt(s, IPPROTO_TCP, TCP_RFC1323,
					   (char *) &on, sizeof(on)) < 0) {
			perror("secsockopt - TCPRFC1323");
		} else {
			printf("nettest: TCP_RFC1323 option set.\n");
		}
#endif	/* TCP_RFC1323 */
#ifdef	TCP_NODELAY
		if (nodelay) {
			if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, 
				       (char *) &nodelay,
				       sizeof(nodelay)) < 0)
				perror("setsockopt - TCP_NODELAY");
		}
#endif
		if (type == SOCK_DGRAM) {
			do_dgram(s, &name);
			shutdown(s, 2);
			exit(0);
		}
		if (connect(s, (struct sockaddr *) &name, namesize) < 0) {
			perror("connect");
			exit(1);
		}
#ifdef	TCP_MAXSEG
		if (getsockopt(s, IPPROTO_TCP, TCP_MAXSEG, 
			       (char *) &maxseg, &maxseglen) < 0)
		  	perror("getsockopt - TCPMAXSEG");
		else
		  	printf("nettest: MSS = %d bytes\n",
			       maxseg);
#endif

		if (buffer_alignment)
		  printf("Buffer alignment: %d\n", buffer_alignment);

		s2 = s;
		break;
	}

	printf("Transfer: %d*%d bytes", nchunks, chunksize);
	if (domain == D_INET)
		printf(" from %9s to %9s\n", myname, hisname);
	else
		printf("\n");


	if (buffer_alignment)
	  printf("Buffer alignment: %d\n", buffer_alignment);
	

	if (do_load) {
	  printf("Local system load:  %8.2f\n", get_load());
	}

	do_stream(s, s2);
	if (s == s2)
		shutdown(s, 2);
	else {
		close(s);
		close(s2);
	}
}

/*
 * When called, this routine will fork off "nconnections" children, and
 * return in each of the children.  The parent program will hang around
 * for the children to die, printing out their statistics (read off of
 * a pipe set up beforehand).
 */
static void
do_children(nconnections)
int nconnections;
{
	int *children;
	register int i, n;
	int status;

	children = (int *)malloc(nconnections * sizeof(int) * 2);
	if (children == NULL) {
		fprintf(stderr,
			"malloc() for data to keep track of children failed\n");
		exit(1);
	}

	for (i = 0; i < nconnections; i++) {
		if (pipe(&children[i*2+0]) < 0) {
			fprintf(stderr, "Cannot create pipe for child %d\n", i);
			continue;
		}
		switch(n = fork()) {
		case 0:
			/*
			 * In the child.  Close down all the pipes, dup
			 * our pipe to stdout and stderr, and return.
			 */
			for (n = 0; n <= i; n++)
				close(children[n*2+0]);
			dup2(children[i*2+1], 1);
			dup2(1, 2);
			close(children[i*2+1]);
			return;

		case -1:
			fprintf(stderr, "Child %d not started\n", i);
			break;

		default:
			close(children[i*2+1]);
			children[i*2+1] = n;
			break;
		}
	}
	while ((n = wait(&status)) >= 0) {
		char buf[512];

		for (i = 0; i < nconnections; i++) {
			if (children[i*2+1] == n)
				break;
		}
		if (i == nconnections) {
			fprintf(stderr, "Unknown child [pid %d] died.\n", n);
			continue;
		}
		printf("Child %d statistics:\n", i);
		fflush(stdout);
		while ((n = read(children[i*2+0], buf, sizeof(buf))) > 0)
			write(1, buf, n);
		close(children[i*2+0]);
		children[i*2+1] = -1;
	}
	exit(0);
}


static void
do_stream(in, out)
register int in, out;
{
	register int	i, t, j, offset = 0, t2;
	register char	*cp;
	char		buf[128], *data;
	long		*cnts;
	time_struct	start, turnaround, end;
	int			daemon_status;
	float		daemon_load;
	float		daemon_version;
	char		*daemon_message;


	/*
	 *	The v on the end indicates to the daemon that we are
	 *	a customized version of the client.
	 */
	sprintf(buf, "%d %d %d %d %d %d %d %d %fv\n",
		nchunks, chunksize, fullbuf, kbufsize, tos,
		nodelay, buffer_alignment, do_load, version);

	if (write(out, buf, strlen(buf))  != strlen(buf)) {
		perror("write() of configuration to deamon");
		exit(1);
	}
	if ((i = read(in, buf, sizeof(buf))) < 0) {
		perror("read (waiting to verify setup)");
		exit(1);
	}
	buf[i] = '\0';
	if (buf[i-1] == '\n')
		buf[--i] = '\0';

	if (buf[0] == 'v') {	/* Customized daemon */
		sscanf(buf, "v%f %d %f",
			&daemon_version, &daemon_status, &daemon_load);

		daemon_message = strchr(buf, '\n');

		if (daemon_message)
			daemon_message++;	/* Skip over CR */

	} else {		/* Non-customized version */
		sscanf(buf, "%d", &daemon_status);

		daemon_message = strchr(buf, ' ');

		if (daemon_message)
			daemon_message++;	/* Skip over Space */
	}

	
	if (daemon_message) {
	  if (strlen(daemon_message))
	    printf("remote server: %s\n", daemon_message + 1);
	}

	if (daemon_status == 0) {
	  fprintf(stderr, "Dying due to error on daemon.\n");
	  exit(1);
	}

	if (do_load) {
		if (daemon_version < 1.0)
			printf("Remote system load: -1 (Not supported)\n");
		else
			printf("Remote system load: %8.2f\n", daemon_load);
	}

	data = valloc(chunksize + buffer_alignment);
	if (data == NULL) {
		fprintf(stderr, "cannot malloc enough space\n");
		exit(1);
	}
	data += buffer_alignment;

	cnts = (long *)malloc((chunksize+1)*sizeof(long));
	bzero((char *) cnts, (chunksize+1)*sizeof(long));

	if (checkdata)
		for (cp = data, i = 0; i < chunksize; )
			*cp++ = *("abcdefghijklmnopqrstuvwxyzABCDEF" + (i++/8)%32);
	if (hash)
		write(0, "\r\nWrite: ", 9);

	GETTIMES(start);

	for (i = 0; i < nchunks; i++) {
		if ((t = write(out, data, chunksize)) < 0) {
			sprintf(buf, "write #%d:", i+1);
			goto bad;
		}
		if (t != chunksize)
			fprintf(stderr, "write: %d got back %d\n", chunksize, t);
		if (hash)
			write(1, "#", 1);
	}

	GETTIMES(turnaround);

	if (hash)
		write(0, "\r\nRead:  ", 9);

	for (i = 0; i < nchunks || offset; i++) {
		if ((t = read(in, data, chunksize)) < 0) {
			sprintf(buf, "read #%d.%d", i+1, chunksize);
			goto bad;
		}
		if (t == 0) {
			fprintf(stderr, "EOF on file, block # %d\n", i);
			break;
		}
		cnts[t]++;
		if (fullbuf) {
			t2 = offset;
			offset += t;
			if (offset >= chunksize) {
				offset -= chunksize;
				if (hash)
					write(1, "#", 1);
			} else {
				--i;
				if (hash)
					write(1, ".", 1);
			}
		} else {
			while (t != chunksize) {

				if (hash)
					write(1, ".", 1);
				if ((t2 = read(in, data+t, chunksize-t)) < 0) {
					sprintf(buf, "read #%d.%d", i,
								chunksize-t);
					goto bad;
				}
				if (t2 == 0) {
					fprintf(stderr,
						"EOF on file, block # %d\n", i);
					break;
				}
				cnts[t2]++;
				t += t2;
			}
			if (hash)
				write(1, "#", 1);
			t2 = 0;
		}
		if (!checkdata)
			continue;
		for (cp = data, j = 0; j < t; cp++, j++) {
		    if (*cp != *("abcdefghijklmnopqrstuvwxyzABCDEF" +
							((j+t2)/8)%32)) {
			fprintf(stderr, "%d/%d(%d): got %d, expected %d\n",
			    i, (j+t2), i*chunksize + (j+t2), *cp,
		    	    *("abcdefghijklmnopqrstuvwxyzABCDEF" +
							((j+t2)/8)%32));
		    }
		}
	}

	GETTIMES(end);

	prtimes(&start, &turnaround, &end);

	if (do_histogram) {
		j = 0;	
		for (i = 0; i <= chunksize; i++)
			if (cnts[i]) {
				printf("%6d: %5d ", i, cnts[i]);
				if (++j == 4) {
					printf("\n");
					j=0;
				}
			}
		if (j)
			printf("\n");
	}

	return;
bad:
	perror(buf);
	exit(1);
}

static void
usage()
{
	fprintf(stderr, 
"Usage: nettest <options> [-p tcp] [host [count [size [port]]]]\n\
       nettest <options> -p udp [host [count [size [port]]]]\n\
       nettest <options> -p unix|pipes [count [size [filename]]]\n\
       nettest <options> -p file file1 file2 [count [size]]\n\
\n\
       Options:\n");
	fprintf(stderr,
"           -A <alignment>	Set buffer alignment\n");
	fprintf(stderr, 
"           -b <size>		Set socket buffer size\n");
	fprintf(stderr,
"           -c			Check data\n");
	fprintf(stderr,
"           -d			Turn on debugging\n");
	fprintf(stderr,
"           -f			Use full buffers for read()s\n");
#ifdef TCP_NODELAY
	fprintf(stderr,
"           -F			Set TCP nodelay option\n");
#endif
	fprintf(stderr,
"           -h			Print hash marks\n");
	fprintf(stderr,
"           -H          Print histogram of read sizes\n");					   
	fprintf(stderr,
"           -n			Number of connections\n");
	fprintf(stderr,
"           -p <protocol>	Set protocol (default tcp)\n");
#ifdef TCP_WINSHIFT
	fprintf(stderr,
"           -s <shift>		Set TCP window shift option\n");
#endif
#ifdef DO_IP_TOS
	fprintf(stderr,
"           -t <service>	Use IP type of servis\n");
#endif
	exit(1);
}

static void
do_dgram(s, name)
register int s;
struct sockaddr_in *name;
{
	register int	ret, i;
	register char	*data;
	time_struct	start, turnaround, end;
	
	data = valloc(chunksize + buffer_alignment);

	if (data == NULL) {
		fprintf(stderr, "cannot malloc enough space\n");
		exit(1);
	}

	data += buffer_alignment;

	GETTIMES(start);

	*data = 0;
	for (i = 0; i < nchunks; i++) {
		if ((ret = sendto(s, data, chunksize, 0,
				  (struct sockaddr *) name,
				  sizeof(*name))) < 0) {
			perror("sendto");
			printf("%d out of %d sent\n", i, nchunks);
			exit(1);
		}
		if (ret != chunksize)
			printf("sendto returned %d, expected %d\n", ret, chunksize);
		(*data)++;
	}

	GETTIMES(turnaround);

	GETTIMES(end);

	prtimes(&start, &turnaround, &end);

}

static void
prtimes(start, turnaround, end)
time_struct	*start, *turnaround, *end;
{
	long	write_ms, write_ums, write_sms;
	long	read_ms, read_ums, read_sms;

	long	total_data = chunksize*nchunks;
   
	int		hertz;


	/*
	 * Figure out value for clock ticks per second.
	 */
#ifdef HZ_USE_SYSCONF

	hertz = (int) sysconf(_SC_CLK_TCK);

#elif defined(HZ_USE_CLK_TCK)

	hertz = CLK_TCK;

#elif defined(HZ)
	hertz = HZ;

#else
	HZ undefined;
#endif
	

#ifdef USE_FTIME
	write_ms =
		((turnaround->ellapsed).time - (start->ellapsed).time)*1000L
		+ (turnaround->ellapsed).millitm - (start->ellapsed).millitm;

	read_ms =
		((end->ellapsed).time - (turnaround->ellapsed).time)*1000L
		+ (end->ellapsed).millitm - (turnaround->ellapsed).millitm;
#else
	write_ms = (turnaround->ellapsed - start->ellapsed)*1000/hertz;
	read_ms = (end->ellapsed - turnaround->ellapsed)*1000/hertz;
#endif

	write_ums =
		((turnaround->tms).tms_utime - (start->tms).tms_utime) * 1000 / hertz;

	write_sms =
		((turnaround->tms).tms_stime - (start->tms).tms_stime) * 1000 / hertz;

	read_ums = 
		((end->tms).tms_utime - (turnaround->tms).tms_utime) * 1000 / hertz;

	read_sms =
		((end->tms).tms_stime - (turnaround->tms).tms_stime) * 1000 / hertz;


	printf("           Real  System            User          Kbyte   Mbit(K^2) mbit(1+E6)\n");

#define FORMAT "%7s %7.4f %7.4f (%4.1f%%) %7.4f (%4.1f%%) %7.2f %7.3f   %7.3f\n"

    if (write_ms)
		printf(FORMAT, "write",
			   write_ms/1000.0,
			   write_sms/1000.0,
			   100.0 * write_sms/write_ms,
			   write_ums/1000.0,
			   100.0 * write_ums/write_ms,
			   total_data / (1024.0*(write_ms/1000.0)),
			   total_data / (128.0*1024.0*(write_ms/1000.0)),
			   total_data / (125.0*write_ms));
	else
		printf("Write time was 0\n");

    if (read_ms)
		printf(FORMAT, "read",
			   read_ms/1000.0,
			   read_sms/1000.0,
			   100.0 * read_sms/read_ms,
			   read_ums/1000.0,
			   100.0 * read_ums/read_ms,
			   total_data / (1024.0*(read_ms/1000.0)),
			   total_data / (128.0*1024.0*(read_ms/1000.0)),
			   total_data / (125.0*read_ms));
	else
		printf("Read time was 0\n");

	if (write_ms && read_ms) {
		long total_ms = write_ms + read_ms;
		long total_ums = write_ums + read_ums;
		long total_sms = write_sms + read_sms;

		printf(FORMAT, "r/w",
			   total_ms/1000.0,
			   total_sms/1000.0,
			   100.0 * total_sms/total_ms,
			   total_ums/1000.0,
			   100.0 * total_ums/total_ms,
			   2 * total_data / (1024.0*(total_ms/1000.0)),
			   2 * total_data / (128.0*1024.0*(total_ms/1000.0)),
			   2 * total_data / (125.0*total_ms));

	}
		
}
