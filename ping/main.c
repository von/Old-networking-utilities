#ifndef lint
static char sccsid[] = "@(#)main.c	4.5 (Berkeley) 4/14/86";
#endif

/*
 *			P I N G . C
 *
 * Using the InterNet Control Message Protocol (ICMP) "ECHO" facility,
 * measure round-trip-delays and packet loss across network paths.
 *
 * Author -
 *	Mike Muuss
 *	U. S. Army Ballistic Research Laboratory
 *	December, 1983
 * Modified at Uc Berkeley
 * Modified at Rutgers: Ron Natalie, David Paul Zimmerman
 * Modified at DECWRL: Jeffrey Mogul
 * Record Route and verbose headers - Phil Dykstra, BRL, March 1988.
 * Modified at Cornell: Jeffrey C Honig, April 1989
 *
 * Status -
 *	Public Domain.  Distribution Unlimited.
 *
 * Bugs -
 *	More statistics could always be gathered.
 *	This program has to run SUID to ROOT to access the ICMP socket.
 */

#include "ping.h"

u_long pingflags;				/* User specified options */
struct timeval timeout = { 1, 0 };		/* Default timeout between packets */
char hostname[MAXHOSTNAMELEN];
struct sockaddr_in destaddr;			/* Who to ping */
struct sockaddr_in srcaddr;			/* Who response is from */
#ifdef	IFF_MULTICAST
char m_interface[MAXHOSTNAMELEN];
struct sockaddr_in m_if;			/* Default interface */
#endif	/* IFF_MULTICAST */

/* Statistics */
static int n_packets;
static int preload = 0;				/* number of packets to "preload" */
static int n_transmitted = 0;			/* sequence # for outbound packets = #sent */
int column = 0;					/* Column number for cisco pings */
int column_max = C_COLS;			/* Maximum width */
int timing = 0;
#ifdef	PRECISECLOCK
int utiming = 1;				/* usec timing */
#else	/* PRECISECLOCK */
int utiming = 0;				/* usec timing */
#endif	/* PRECISECLOCK */
stats tstat = {0};

static int icmp_socket;			/* Socket file descriptor */
static int raw_socket;
optstr *sumopt = NULL, *curopt;
static int intrflag;

char *my_name;
int datalen = -1;			/* How much data */
size_t total_length = 0;		/* Total data being sent */

/* Incoming and outgoing data packets */
static u_char packet[MAXDATA];		/* Incoming packet */

static int out_length;
static struct icmp out_icmp;
u_char out_pattern[MAXDATA];

static struct iovec out_iovec[IOVEC_SIZE] = {
#ifndef	NO_RAW_IPHDR
    { NULL,		0 },
#endif	/* NO_RAW_IPHDR */
    { (caddr_t) &out_icmp,	ICMP_MINLEN },
    { (caddr_t) out_pattern,	sizeof (out_pattern) }
} ;

/* Bit table for detecting duplicates and missing responses */
int mx_dup_ck = MAX_DUP_CHK;
char rcvd_tbl[MAX_DUP_CHK / 8];

static struct opt_names options[] = {
  { 'c',	"count",	"packet count" },
  { 'd',	NULL,		"enable socket debugging" },
  { 'f',	NULL,		"flood" },
#ifdef	IP_OPTIONS
  { 'g',	"gateway",	"loose source route" },
#endif	/* IP_OPTIONS */
#ifdef	IFF_MULTICAST
  { 'j',	NULL,		"Join multicast group" },
#endif	/* MULTICAST */
  { 'l',	"preload",	"preload count" },
  { 'm',	NULL,		"report missing summary" },
  { 'n',	NULL,		"numeric address reporting" },
  { 'o',	"type",		"specify ICMP packet type" },
  { ' ',	"echo",		"echo request" },
  { ' ',	"info",		"information request" },
  { ' ',	"mask",		"mask request" },
  { ' ',	"router",	"router solicitation" },
  { ' ',	"timestamp",	"timestamp" },
  { 'p',	"pattern",	"specify fill pattern" },
  { 'q',	NULL,		"quiet" },
  { 'r',	NULL,		"don't use routing table" },
  { 's',	"packetsize",	"packet size" },
  { 't',	"timeout",	"timeout" },
  { 'u',	NULL,		"microsecond timing" },
  { 'v',	NULL,		"verbose" },
  { 'C',	NULL,		"cisco-style" },
  { 'F',	NULL,		"fast" },
#ifdef	IP_OPTIONS
  { 'G',	"gateway",	"strict source route" },
#endif	/* IP_OPTIONS */
#if	defined(IP_HDRINCL) || !defined(NO_RAW_IPHDR)
  { 'I',	NULL,		"increment TTL" },
#endif	/* defined(IP_HDRINCL) || !defined(NO_RAW_IPHDR) */
#ifdef	IP_OPTIONS
  { 'O',	NULL,		"print options" },
#ifdef	IFF_MULTICAST
  { 'L',	NULL,		"disable multicast loopback" },
#endif	/* MULTICAST */
#endif	/* IP_OPTIONS */
  { 'N',	NULL,		"symbolic" },
#ifdef	IP_OPTIONS
  { 'R',	NULL,		"record route" },
#endif	/* IP_OPTIONS */
#if	defined(IP_HDRINCL) || !defined(NO_RAW_IPHDR)
  { 'S',	"tos",		"type of service" },
  { 'T',	"ttl",		"time to live" },
#endif	/* defined(IP_HDRINCL) || !defined(NO_RAW_IPHDR) */
  { 'U',	NULL,		"millisecond timing" },
  { 0,		NULL,		NULL }
} ;


int
is_root()
{
  int ngroups;
  int groups[NGROUPS];

  if (!getuid()) {
      return 1;
  }
  
  if ((ngroups = getgroups(NGROUPS, groups)) < 0) {
    perror("getgroups");
    return 0;
  }

  while (ngroups--) {
    if (!groups[ngroups]) {
      return 1;
    }
  }

  return 0;
}


/* On SIGINT, increment count */
static void
intr()
{
    intrflag++;
}


/* On a SIGWINCH update or idea of the terminal width */
static void
window_size()
{
    struct winsize winsize;

    if (ioctl(fileno(stdout), TIOCGWINSZ, &winsize) < 0) {
	perror("ioctl TIOCGWINSZ");
	exit (1);
    }

    if (winsize.ws_col) {
	column_max = winsize.ws_col;
    }
}


static int
ping(iovec, length)
struct iovec *iovec;
int length;
{
    int i = preload + 1;
    int n_fds = getdtablesize();
    struct timeval waittime;
    int finished = FALSE;
    int finishing = FALSE;
    struct timeval start_time, end_time;

    if (gettimeofday(&start_time, NULL) < 0) {
	perror("gettimeofday");
	return -1;
    }

    printf("PING %.*s %s: %d data bytes\n",
	   24, asctime(localtime(&start_time.tv_sec)),
	   pr_addr(destaddr.sin_addr),
	   iovec[IOVEC_ICMP].iov_len + iovec[IOVEC_PATTERN].iov_len);

    fflush(stdout);

    /* Fire off one to get things started, and preload if specified */
    i = preload+1;
    while (i--) {
	send_icmp(raw_socket, iovec, length, n_transmitted++, &destaddr);
    }

    if (BIT_TEST(pingflags, OF_FLOOD)) {
	/* Set for 10 packets per second */
	waittime.tv_sec = 0;
	waittime.tv_usec = 10000;
    } else {
	/* Set specified timeout */
	waittime = timeout;	/* struct copy */
    }

    do {
	int cc;
	struct timeval tv;
	fd_set fdset;

	tv = waittime;	/* struct copy */

	FD_ZERO(&fdset);
	FD_SET(icmp_socket, &fdset);

	cc = select(n_fds, &fdset, 0, 0, &tv);
	if (cc < 0) {
	    /* Select error */
	    
	    if (errno != EINTR) {
		perror("select");
		return 1;
	    }
	} else if (!cc) {
	    /* Timeout */
	    
	    if (finishing) {
		/* Final timeout - lets get out of here */
		
		finished = TRUE;
	    } else {
		if (BIT_TEST(pingflags, OF_CISCO|OF_FLOOD)
		    && !BIT_TEST(pingflags, OF_QUIET)) {
		    C_PUT(".", 1);
		}
		
		if (send_icmp(raw_socket, iovec, length, n_transmitted++, &destaddr)) {
		    finished = TRUE;
		}
	    }
	} else {
	    /* Packet ready */
	    recv_info *info;
	    
	    info = recv_icmp(icmp_socket,
			     packet,
			     sizeof (packet),
			     ((struct icmp *)iovec[IOVEC_ICMP].iov_base)->icmp_id,
			     out_icmp.icmp_type);
	    switch (info->recv_state & RST_MASK) {
	    case RST_REPLY:
#ifdef	OF_MULTICAST
		if (BIT_COMPARE(pingflags, OF_INCRTTL|OF_MULTICAST, OF_INCRTTL)) {
#else	/* OF_MULTICAST */
		if (BIT_TEST(pingflags, OF_INCRTTL)) {
#endif	/* OF_MULTICAST */
		    finished = TRUE;
		}
		if (BIT_TEST(pingflags, OF_CISCO|OF_FLOOD)
		    && !BIT_TEST(pingflags, OF_QUIET)) {
		    switch (info->recv_state & RSF_MASK) {
		    case RSF_CORRUPT:
			C_PUT("#", 1);
			break;
			
		    case RSF_DUPLICATE:
			C_PUT("\b*", 0);
			break;
			
		    default:
			if (BIT_TEST(pingflags, OF_CISCO)) {
			    C_PUT("!", 1);
			} else {
			    C_PUT("\b \b", -1);
			}
			break;
		    }
		}
		/* Fall Thru */
		
	    case RST_RESPONSE:	/* Other message for us */
		if (!BIT_TEST(pingflags, OF_QUIET|OF_CISCO|OF_FLOOD)) {
		    if (BIT_TEST(pingflags, OF_VERBOSE)) {
			pr_icmph(info);
		    } else {
			pr_packet(info);
		    }
		}
		
		if (n_packets && tstat.tn >= n_packets) {
		    finished = TRUE;
		} else if (BIT_TEST(pingflags, OF_FLOOD|OF_FAST|OF_CISCO)
			   && !finishing) {
		    if (send_icmp(raw_socket, iovec, length, n_transmitted++, &destaddr)) {
			finished = TRUE;
		    } else if (BIT_TEST(pingflags, OF_FLOOD)
			       && !BIT_TEST(pingflags, OF_QUIET)) {
			C_PUT(".", 1);
		    }
		}
		break;

	    case RST_REQUEST:
	    case RST_OTHER:	/* Other message not for us */
		if (BIT_TEST(pingflags, OF_VERBOSE)) {
		    pr_icmph(info);
		}
		break;
	    }
	    
	}
	
	/* Check for an interrupt */
	if (intrflag) {
	    /* Interrupted by ^C */
	    
	    if (!finishing) {
		
		/* First time */
		if ((tstat.tn >= n_transmitted) || !tstat.tn) {
		    
		    /* Quit now if all packets received or none received */
		    finished = TRUE;
		} else {
		    /* Indicate we are finishing */
		    
		    finishing = TRUE;
		    intrflag = 0;
		    
		    if (BIT_TEST(pingflags, OF_CISCO|OF_FLOOD)) {
			column += 2;
		    }
		    
		    if (!BIT_TEST(pingflags, OF_CISCO)) {
			if (tstat.tn) {
			    /* If we have a round trip time, use it */
			    waittime.tv_sec = tstat.tmax.tv_sec * 2;
			    waittime.tv_usec = (tstat.tmax.tv_usec & ~TV_NEG) * 2;
			    if (waittime.tv_usec >= 1000000) {
				waittime.tv_usec -= 1000000;
				waittime.tv_sec += 1;
			    }

			    if ((waittime.tv_sec < timeout.tv_sec)
			      || (waittime.tv_sec == timeout.tv_sec
				&& waittime.tv_usec < timeout.tv_sec)) {
				waittime = timeout;	/* struct copy */
			    }
			} else {
			    waittime.tv_sec = MAXWAIT;
			    waittime.tv_usec = 0;
			}
			if (!BIT_TEST(pingflags, OF_FLOOD)) {
			    (void) fprintf(stderr, "  waiting for outstanding packets\n");
			}
		    }
		    
		    n_packets = n_transmitted + 1;	/* let the normal limit work */
		}
	    } else {
		/* Second time - bug out */
		
		finished = TRUE;
	    }
	}
    } while (!finished) ;

    if (gettimeofday(&end_time, NULL) < 0) {
	perror("gettimeofday");
	return -1;
    }

    /* Print statistics and stuff */
    signal(SIGINT, SIG_DFL);
    
    printf("\n----%.*s %s PING Statistics----\n",
	   24, asctime(localtime(&end_time.tv_sec)),
	   pr_addr(destaddr.sin_addr));
    printf("%d transmitted, ",
	   n_transmitted);
    printf("%d received, ",
	   tstat.tn);
    if (tstat.td) {
	printf("%d duplicates, ",
	       tstat.td);
    }
    if (tstat.tc) {
        printf("%d corrupted, ",
	       tstat.tc);
    }
    if (n_transmitted) {
	if (tstat.tn > n_transmitted) {
	    printf("-- somebody's printing up packets!");
	} else {
	    double received = tstat.tn;
	    double transmitted = n_transmitted;
	    double delta_t = (end_time.tv_sec + end_time.tv_usec / 1000000.0)
		- (start_time.tv_sec + start_time.tv_usec / 1000000.0);
	    double packetthru = (double) (received / delta_t);
	    double bitsthru = total_length * 8.0 * packetthru;
	    const char *mul;

	    if (bitsthru > 1000000000.0) {
		bitsthru /= 1000000000.0;
		mul = "G";
	    } else if (bitsthru > 1000000.0) {
		bitsthru /= 1000000.0;
		mul = "M";
	    } else if (bitsthru > 1000.0) {
		bitsthru /= 1000.0;
		mul = "k";
	    } else {
		mul = "";
	    }

	    printf("%.2f%% packet loss.\n",
		   (transmitted - received) * 100.0 / transmitted);
	    printf("%.3f seconds elapsed, throughput = %.2f packets/sec; %.3f %sbps",
		   delta_t,
		   packetthru,
		   bitsthru,
		   mul);
	}
    }
    printf(".\n");
    if (timing) {
	pr_stats(&tstat);
    }

#ifdef	IP_OPTIONS
    if (sumopt) {
      for (curopt = sumopt; curopt; curopt = curopt->next) {
	printf("\n%d packets (%d%%) ",
	       curopt->optstats.tn,
	       (int) (100 - ((curopt->optstats.tn - tstat.tn) / tstat.tn) * 100));
	if (curopt->optstats.td) {
	  printf("%d duplicates ", curopt->optstats.td);
	}
	if (curopt->optstats.tc) {
	  printf("%d duplicates ", curopt->optstats.tc);
	}
	printf("via:\n");
	pr_rroption(curopt->optval);
	pr_stats(&curopt->optstats);
      }
    }
#endif	/* IP_OPTIONS */

    if (BIT_TEST(pingflags, OF_MISSING)) {
	/* Print missing response numbers */
        char line[BUFSIZ];
	int missing = 0;
	int response, lresponse;
	int max_missed = (n_transmitted > mx_dup_ck) ? mx_dup_ck : n_transmitted;

	column = 1;
	for (response = 0; response < max_missed; response++) {
	    if (!TST(response % mx_dup_ck)) {
		if (!missing) {
		    printf("missed responses:\n");
		}
		for (lresponse = response + 1; lresponse < max_missed; lresponse++) {
		    if (TST(lresponse % mx_dup_ck)) {
		        lresponse--;
		        break;
		    }
		}
		if (lresponse > response) {
		    sprintf(line, "%d-%d",
			   response,
			   lresponse);
		    response = lresponse;
		} else {
		    sprintf(line, "%d",
			   response);
		}
		if ((column += strlen(line)) >= column_max) {
		    printf("\n");
		    column = strlen(line);
		  } else if (missing) {
		    column++;
		    printf(" ");
		}
		printf(line);
		missing = 1;
	    }
	}
	if (column > 1) {
	    printf("\n");
	}
    }
    fflush(stdout);

    return 0;
}


/*
 *	Print usage information
 */
void
usage(op)
struct opt_names *op;
{
#ifdef	IFF_MULTICAST
    fprintf(stderr,
	    "Usage:\t%s options address [multicast interface]\n",
	    my_name);
#else	/* IFF_MULTICAST */
    fprintf(stderr,
	    "Usage:\t%s options address\n",
	    my_name);
#endif	/* IFF_MULTICAST */
    for (op = options; op->op_letter; op++) {
	if (isspace(op->op_letter)) {
	    fprintf(stderr, "\t\t%s\t\t%s\n",
		    op->op_arg,
		    op->op_name);
	} else if (op->op_arg) {
	    fprintf(stderr, "\t-%c %-13s%s\n",
		    op->op_letter,
		    op->op_arg,
		    op->op_name);
	} else {
	    fprintf(stderr, "\t-%c\t\t%s\n",
		    op->op_letter,
		    op->op_name);
	}
    }
}


/*
 * 			M A I N
 */
int
main(argc, argv)
int argc;
char *argv[];
{
    int on = 1, i;
    char c, *cp;
    int errflg = 0;
    int fp = 0;
    char getoptions[sizeof (options)/sizeof (struct opt_names) * 2 + 1];
    struct opt_names *op;
    char *a2sockaddr();
#ifdef	SO_RCVBUF
    int recv_buf = RECV_BUF;		/* Size of receive buffer */
#endif	/* SO_RCVBUF */
#ifndef	NO_RAW_IPHDR
    struct ip *out_ip;
    u_char *ip_options;
    u_char *ip_opt;
#else	/* NO_RAW_IPHDR */
#ifdef	IP_OPTIONS
    static u_char ip_options[MAX_IPOPTLEN + sizeof (struct in_addr)];	/* Static so it is cleared */
    u_char *ip_opt = ip_options;
#endif	/* IP_OPTIONS */
#endif	/* NO_RAW_IPHDR */
    extern int optind;
    extern char *optarg;
	

#define	CONFLICT(flag)	if (BIT_TEST(pingflags, flag)) fprintf(stderr, "%s: conflicting option: -%c\n", my_name, c), exit (1)
#ifndef	NO_ROOT_CHECK
#define	ROOT(c)	if (!is_root()) fprintf(stderr, \
					"%s: you must be root or in group wheel to use the -%c option\n", my_name, c), exit (1)
#else	/* NO_ROOT_CHECK */
#define	ROOT(c)
#endif	/* NO_ROOT_CHECK */
#define	ISATTY(c)	if (!isatty(fileno(stdout))) fprintf(stderr, "%s: need a tty for option: -%c\n", my_name, c), exit (1)

    if (my_name = (char *) rindex(*argv, '/')) {
	my_name++;
    } else {
	my_name = *argv;
    }
    
    setbuf(stdout, 0);			/*  Override any buffering  */

#ifndef	NO_RAW_IPHDR
    out_ip = (struct ip *) calloc(1, sizeof (struct ip) + MAX_IPOPTLEN);
    if (!out_ip) {
	fprintf(stderr, "%s: calloc: Out of Memory\n",
		my_name);
	exit (ENOMEM);
    }
    out_ip->ip_ttl = MAXTTL;

    out_iovec[IOVEC_IP].iov_base = (caddr_t) out_ip;
    out_iovec[IOVEC_IP].iov_len = sizeof (struct ip);

    ip_opt = ip_options = (u_char *) out_iovec[IOVEC_IP].iov_base + out_iovec[IOVEC_IP].iov_len;
#endif	/* NO_RAW_IPHDR */

    /* Default query type */
    out_icmp.icmp_type = ICMP_ECHO;
    out_icmp.icmp_code = 0;
    
    /* Build option list for getopt() */
    cp = getoptions;
    for (op = options; op->op_letter; op++) {
	if (!isspace(op->op_letter)) {
	    *cp++ = op->op_letter;
	    if (op->op_arg)
		*cp++ = ':';
	}
    }
    
    while ((c = getopt(argc, argv, getoptions)) != (char) -1) {
	switch (c) {
	    case 'd':			/* Socket debugging */
		BIT_SET(pingflags, OF_DEBUG);
		break;

	    case 'p':			/* Specify data */
		BIT_SET(pingflags, OF_FILLED);
		for (cp = optarg; *cp; cp += 2) {
		    if (!isxdigit(*cp) || !isxdigit(*(cp + 1))) {
			fprintf(stderr, "%c%c is not a hex digit pair\n",
				*cp, *(cp + 1));
			exit(1);
		    }
#define	atox(c)	(isdigit(c) ? (c - '0') : ( isupper(c) ? (c - 'A' + 10) : (c - 'a' + 10) ))
		    if (fp >= MAXDATA) {
			fprintf(stderr, "too much fill data specified\n");
			exit(2);
		    }
		    out_pattern[fp++] = (atox(*cp) << 4) | atox(*(cp + 1));
		}
		break;

	    case 'r':			/* Don't use routing table */
		BIT_SET(pingflags, OF_DONTROUTE);
		break;

	    case 'v':			/* Verbose messages */
		BIT_SET(pingflags, OF_VERBOSE);
		break;

	    case 'C':			/* Cisco style pings */
		CONFLICT(OF_FLOOD|OF_FAST|OF_INCRTTL);
		ISATTY(c);
		BIT_SET(pingflags, OF_CISCO);
		break;

	    case 't':			/* Specify timeout */
		if (cp = (char *) index(optarg, '.')) {
		  *cp++ = (char) 0;
		  if (timeout.tv_usec = atoi(cp)) {
		    while (timeout.tv_usec < 100000) {
		      timeout.tv_usec *= 10;
		    }
		  }
		}
		timeout.tv_sec = atoi(optarg);
		if (!timeout.tv_sec && !timeout.tv_usec) {
		  fprintf(stderr, "%s: invalid timeout specified: %d\n",
			  my_name,
			  timeout.tv_sec);
		  exit (1);
		}
		if (!timeout.tv_sec) {
		    ROOT(c);
		}
		break;

	    case 'f':			/* Flood pings */
		CONFLICT(OF_CISCO|OF_FAST|OF_INCRTTL);
		ROOT(c);
		ISATTY(c);
		BIT_SET(pingflags, OF_FLOOD);
		break;

	    case 'F':
		CONFLICT(OF_CISCO|OF_FLOOD|OF_INCRTTL);
		BIT_SET(pingflags, OF_FAST);
		break;

#ifdef	IP_OPTIONS
	    case 'O':			/* Print options fields */
		BIT_SET(pingflags, OF_PROPTS);
		break;
		
	    case 'R':			/* Record route */
		BIT_SET(pingflags, OF_RROUTE);
		break;

	    case 'g':			/* Loose source routing */
		CONFLICT(OF_SSROUTE);
		BIT_SET(pingflags, OF_LSROUTE);

		/* Set the option type */
		ip_opt[IPOPT_OPTVAL] = (char) IPOPT_LSRR;

		goto source_route;

	    case 'G':			/* Strict source routing */
		CONFLICT(OF_LSROUTE);
		BIT_SET(pingflags, OF_SSROUTE);

		/* Set the option type */
		ip_opt[IPOPT_OPTVAL] = (char) IPOPT_SSRR;

	    source_route:
		if (ip_opt - ip_options + 1 > MAX_IPOPTLEN + 2 * sizeof (struct in_addr)) {
		    fprintf(stderr, "%s: Options field full\n",
			    my_name);
		    exit(1);
		} else {
		  u_char *oix = ip_opt + ip_opt[IPOPT_OLEN];
		  struct sockaddr_in gateway;

		  /* Get the gateway address */
		  if (cp = a2sockaddr(&gateway, optarg, (char *) 0)) {
		    fprintf(stderr, "%s: %s\n",
			    my_name,
			    cp);
		    exit(1);
		  }

		  if (!ip_opt[IPOPT_OLEN]) {
		    /* Set the option length and fill pointer */
		    ip_opt[IPOPT_OFFSET] = (char) IPOPT_MINOFF;
		    ip_opt[IPOPT_OLEN] = IPOPT_MINOFF - 1;
		    oix += ip_opt[IPOPT_OLEN];

		  }	

		  /* Copy in this gateway's IP address */
		  {
		    register int len = sizeof(gateway.sin_addr);
		    u_char *cpp = (u_char *) &gateway.sin_addr;

		    ip_opt[IPOPT_OLEN] += len;
		    while (len--) {
		      *oix++ = *cpp++;
		    }
		  }
		
		}
		break;
#endif	/* IP_OPTIONS */

#if	defined(IP_HDRINCL) || !defined(NO_RAW_IPHDR)
	    case 'I':
		CONFLICT(OF_CISCO|OF_FLOOD|OF_FAST);
		if (out_ip->ip_ttl != MAXTTL || n_packets) {
		  fprintf(stderr, "%s: conflicting option: -%c\n",
			  my_name,
			  c);
		}
		out_ip->ip_ttl = 0;
		BIT_SET(pingflags, OF_INCRTTL);
		break;
		
	    case 'T':			/* TTL */
		CONFLICT(OF_INCRTTL);
		out_ip->ip_ttl = atoi(optarg);
		if (out_ip->ip_ttl < 0 || out_ip->ip_ttl > MAXTTL) {
		  fprintf(stderr, "%s: invalid ttl: %d\n",
			  my_name,
			  out_ip->ip_ttl);
		  exit (1);
		}
		break;

	    case 'S':			/* TOS */
		out_ip->ip_tos = atoi(optarg);
		break;
#endif	/* defined(IP_HDRINCL) || !defined(NO_RAW_IPHDR) */

	    case 'l':			/* Preload */
		preload = atoi(optarg);
		break;

	    case 'n':			/* Numeric IP addresses only */
		CONFLICT(OF_SYMBOLIC);
		BIT_SET(pingflags, OF_NUMERIC);
		break;

	    case 'N':			/* All symbolic IP addresses */
		CONFLICT(OF_NUMERIC);
		BIT_SET(pingflags, OF_SYMBOLIC);
		break;
		
	    case 'q':
		BIT_SET(pingflags, OF_QUIET);
		break;

	    case 'm':
		BIT_SET(pingflags, OF_MISSING);
		break;

	    case 's':			/* Data length */
		datalen = atoi(optarg);
		if (datalen < 0 || datalen > MAXDATA) {
		    fprintf(stderr, "ping: invalid length\n");
		    exit(1);
		}
		break;

	    case 'c':			/* Packet count */
		CONFLICT(OF_INCRTTL);
		n_packets = atoi(optarg);
		break;

#ifdef	IFF_MULTICAST
	    case 'j':			/* Join multicast group */
		BIT_SET(pingflags, OF_MJOIN);
		break;

	    case 'L':			/* Disable multicast loopback */
		BIT_SET(pingflags, OF_MNOLOOP);
		break;
#endif	/* MULTICAST */

	    /* Packet types */
	    case 'o':
		i = strlen(optarg);
		if (i < 1) {
		    errflg++;
		}
		if (!strncasecmp(optarg, "echo", i)) {
		    out_icmp.icmp_type = ICMP_ECHO;
		} else if (!strncasecmp(optarg, "info", i)) {
		    out_icmp.icmp_type = ICMP_IREQ;
		} else if (!strncasecmp(optarg, "mask", i)) {
		    out_icmp.icmp_type = ICMP_MASKREQ;
		} else if (!strncasecmp(optarg, "router", i)) {
		    out_icmp.icmp_type = ICMP_ROUTERSOL;
		} else if (!strncasecmp(optarg, "timestamp", i)) {
		    out_icmp.icmp_type = ICMP_TSTAMP;
		} else {
		    errflg++;
		    break;
		}
		break;
		
	    case 'u':
		utiming = 1;
		break;

	    case 'U':
		utiming = 0;
		break;


	    case '?':
		errflg++;
		break;
	}
    }

    if (errflg || (optind == argc)) {
        usage(options);
	exit(1);
    }
    for (i = 0; optind + i < argc; i++) {
	switch (i) {
	case 0:			/* Hostname */
	    if (cp = a2sockaddr(&destaddr, argv[optind + i], hostname)) {
		fprintf(stderr, "%s: %s\n",
			my_name,
			cp);
		exit(1);
	    }
#ifdef	IFF_MULTICAST
	    if (IN_MULTICAST(ntohl(destaddr.sin_addr.s_addr))) {
		BIT_SET(pingflags, OF_MULTICAST);
	    }
#endif	/* IFF_MULTICAST */
	    break;

#ifdef	IFF_MULTICAST
	case 1:			/* Multicast default interface */
	    if (BIT_TEST(pingflags, OF_MULTICAST)) {
		if (cp = a2sockaddr(&m_if, argv[optind + i], m_interface)) {
		    fprintf(stderr, "%s: %s\n",
			    my_name,
			    cp);
		    exit(1);
		}
		BIT_SET(pingflags, OF_MIF);
		break;
	    }
#endif	/* IFF_MULTICAST */

	default:
	    usage(options);
	    exit(1);
	}
    }

    switch (out_icmp.icmp_type) {
    case ICMP_ECHO:
	out_icmp.icmp_id = htons(getpid() & 0xFFFF);
	out_iovec[IOVEC_ICMP].iov_len = ICMP_MINLEN;
	if (datalen == -1
	    || datalen >= sizeof (struct timeval)) {
	    timing = 1;
	} else {
	    timing = 0;
	}
	break;

    case ICMP_TSTAMP:
	out_icmp.icmp_id = htons(getpid() & 0xFFFF);
	out_iovec[IOVEC_ICMP].iov_len = ICMP_TSLEN;
	timing = 1;
	break;

    case ICMP_IREQ:
	out_icmp.icmp_id = htons(getpid() & 0xFFFF);
	out_iovec[IOVEC_ICMP].iov_len = ICMP_MINLEN;
	datalen = timing = 0;
	break;

    case ICMP_MASKREQ:
	out_icmp.icmp_id = htons(getpid() & 0xFFFF);
	out_iovec[IOVEC_ICMP].iov_len = ICMP_MINLEN;
	datalen = timing = 0;
	break;

    case ICMP_ROUTERSOL:
	out_icmp.icmp_id = 0;
	out_iovec[IOVEC_ICMP].iov_len = ICMP_MINLEN;
	datalen = timing = 0;
	break;

    default:
	exit(1);
    }

    if (datalen == -1) {
	datalen = 64 - out_iovec[IOVEC_ICMP].iov_len;
    }

    if (fp) {
	/* Pattern specified - replicate to fill buffer */
	if (!BIT_TEST(pingflags, OF_QUIET)) {
	    fprintf(stderr, "PATTERN: ");
	    for (i = 0; i < fp; i++) {
		fprintf(stderr, "%02x", out_pattern[i] & 0xff);
	    }
	    fprintf(stderr, "\n");
	}
	if (fp < MAXDATA) {
	    for (i = 0; i < MAXDATA - fp; i++) {
		out_pattern[fp + i] = out_pattern[i];
	    }
	}
    } else {
	/* No Pattern specified - fill with position info */
	for (fp = 0; fp < MAXDATA; fp++) {
	    out_pattern[fp] = fp + sizeof(struct timeval);
	}
    }

    if ((icmp_socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
	perror("ping: icmp socket");
	exit(5);
    }

    if (BIT_TEST(pingflags, OF_DEBUG)) {
	if (setsockopt(icmp_socket, SOL_SOCKET, SO_DEBUG, &on, sizeof(on)) < 0) {
	    perror("setsockopt: SO_DEBUG");
	    exit(42);
	}
    }

#ifdef	SO_RCVBUF
    if (setsockopt(icmp_socket, SOL_SOCKET, SO_RCVBUF, (char *) &recv_buf, sizeof(recv_buf)) < 0) {
	perror("setsockopt: SO_RCVBUF");
	exit(42);
    }
#endif	/* SO_RCVBUF */

#if	defined(IP_HDRINCL) || defined(NO_RAW_IPHDR)
    raw_socket = icmp_socket;
#else	/* defined(IP_HDRINCL) || defined(NO_RAW_IPHDR) */
    if ((raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
      perror("ping: raw socket");
      exit (5);
    }

    if (BIT_TEST(pingflags, OF_DEBUG)) {
	if (setsockopt(raw_socket, SOL_SOCKET, SO_DEBUG, &on, sizeof(on)) < 0) {
	    perror("setsockopt: SO_DEBUG");
	    exit(42);
	}
    }
#endif	/* defined(IP_HDRINCL) || defined(NO_RAW_IPHDR) */

    if (BIT_TEST(pingflags, OF_DONTROUTE)) {
	if (setsockopt(raw_socket, SOL_SOCKET, SO_DONTROUTE, &on, sizeof(on)) < 0) {
	    perror("setsockopt: SO_DONTROUTE");
	    exit(42);
	}
    }

#ifdef	IP_OPTIONS
    if (BIT_TEST(pingflags, OF_RROUTE | OF_LSROUTE | OF_SSROUTE)) {

      if (ip_opt[IPOPT_OLEN]) {
	/* Complete Source route option by putting the destination's */
	/* address at the end of the list */
	register u_char *oix = ip_opt + ip_opt[IPOPT_OLEN];
	register int len = sizeof(destaddr.sin_addr);
	u_char *cpp = (u_char *) &destaddr.sin_addr;

	ip_opt[IPOPT_OLEN] += len;
	while (len--) {
	  *oix++ = *cpp++;
	}
	ip_opt += ip_opt[IPOPT_OLEN];
      }

      if (BIT_TEST(pingflags, OF_RROUTE)) {
	int len;
	
	/* Remaining length minus room for end-of-list */
	len = MAX_IPOPTLEN - (ip_opt - ip_options) - 1;
	/* We have a bit of extra room if LSRR or SSRR are specified */
	if (BIT_TEST(pingflags, OF_SSROUTE|OF_LSROUTE)) {
	  len += sizeof (struct in_addr);
	}
	/* Round out to the size of an IP address */
	len -= (len - IPOPT_MINOFF + 1) % sizeof (struct in_addr);

	if (len < IPOPT_MINOFF - 1 + sizeof (struct in_addr)) {
	  fprintf(stderr, "%s: no room for RR option\n",
		  my_name);
	  exit(1);
	}

	/* Set the option type */
	ip_opt[IPOPT_OPTVAL] = (u_char) IPOPT_RR;
	ip_opt[IPOPT_OFFSET] = (u_char) IPOPT_MINOFF;
	ip_opt[IPOPT_OLEN] = (u_char) len;

	/* Skip to end of this option */
	ip_opt += ip_opt[IPOPT_OLEN];
      }

      /* Append the end-of-list */
      *ip_opt++ = (u_char) IPOPT_EOL;

      /* Pad the list out to a fullword in length */
      while ((ip_opt - ip_options) % sizeof (struct in_addr)) {
	*ip_opt++ = (u_char) IPOPT_EOL;
      }

#ifdef	IP_HDRINCL
      /* Set the IP options length */
      out_iovec[IOVEC_IP].iov_len += ip_opt - ip_options;

      if (BIT_TEST(pingflags, OF_PROPTS)) {
	  pr_ipopt(ip_options, ip_opt - ip_options);
      }
#else	/* IP_HDRINCL */
      if (setsockopt(raw_socket,
		     IPPROTO_IP,
		     IP_OPTIONS,
		     ip_options,
		     ip_opt - ip_options) < 0) {
	perror("setsockopt: IP_OPTIONS");
	exit(42);
      }
      total_length += ip_opt - ip_options;

      if (BIT_TEST(pingflags, OF_PROPTS)) {
	struct ipoption ipoption;
	int ret = sizeof (ipoption);

	/* Clear the options field */
	bzero((caddr_t) &ipoption, sizeof(ipoption));

	if (getsockopt(raw_socket,
		       IPPROTO_IP,
		       IP_OPTIONS,
		       &ipoption,
		       &ret) < 0) {
	  perror("getsockopt: IP_OPTIONS");
	  exit(42);
	}

	ret -= sizeof (struct in_addr);
	if (ipoption.ipopt_dst.s_addr) {
	  printf("Next hop: %s ",
		 pr_ntoa(ipoption.ipopt_dst));
	}
	pr_ipopt((u_char *)ipoption.ipopt_list, ret);
      }
#endif	/* IP_HDRINCL */
    }
#endif	/* IP_OPTIONS */

#ifdef	IP_HDRINCL
    if (setsockopt(raw_socket,
		   IPPROTO_IP,
		   IP_HDRINCL,
		   (char *) &on,
		   sizeof(on)) < 0) {
	perror("traceroute: IP_HDRINCL");
	exit(6);
    }
#endif /* IP_HDRINCL */

#ifdef	IFF_MULTICAST
    if (BIT_TEST(pingflags, OF_MULTICAST)) {
	struct ip_mreq m_req;

	if (BIT_TEST(pingflags, OF_MJOIN)) {
	    m_req.imr_interface = m_if.sin_addr;	/* struct copy */
	    m_req.imr_multiaddr = destaddr.sin_addr;
	    if (!BIT_TEST(pingflags, OF_MIF)) {
		m_req.imr_interface.s_addr = INADDR_ANY;
	    }
	    if (setsockopt(raw_socket,
			   IPPROTO_IP,
			   IP_ADD_MEMBERSHIP,
			   (caddr_t) &m_req,
			   sizeof(m_req)) == -1) {
		perror ("can't join multicast group");
		exit(92);
	    }
	}

	if (BIT_TEST(pingflags, OF_MNOLOOP)) {
	    u_char off = 0;

	    if (setsockopt(raw_socket,
			   IPPROTO_IP,
			   IP_MULTICAST_LOOP,
			   (caddr_t) &off,
			   sizeof (off)) == -1) {
		perror ("can't disable multicast loopback");
		exit(92);
	    }
	}

	if (setsockopt(raw_socket,
		       IPPROTO_IP,
		       IP_MULTICAST_TTL,
		       (caddr_t) &out_ip->ip_ttl,
		       sizeof (out_ip->ip_ttl)) == -1) {
	    perror ("can't set multicast time-to-live");
	    exit(93);
	}

	if (BIT_COMPARE(pingflags, OF_MIF|OF_MJOIN, OF_MIF)) {
	    if (setsockopt(raw_socket,
			   IPPROTO_IP,
			   IP_MULTICAST_IF,
			   &m_if.sin_addr,
			   sizeof(m_if.sin_addr)) == -1) {
		perror ("can't set multicast source interface");
		exit(94);
	    }
	}
    }
#endif	/* IFF_MULTICAST */

    /* Init packet fields */
#ifndef	NO_RAW_IPHDR
    out_ip->ip_off = 0;
    out_ip->ip_p = IPPROTO_ICMP;
    out_ip->ip_dst = destaddr.sin_addr;
    out_ip->ip_v = IPVERSION;

    /* Calculate IP header length */
    out_ip->ip_hl = out_iovec[IOVEC_IP].iov_len >> 2;
#endif	/* NO_RAW_IPHDR */

    /* Calculate packet length */
    out_iovec[IOVEC_PATTERN].iov_len = datalen;
	
    out_length = 0;
    for (i = 0; i < IOVEC_SIZE; i++) {
	out_length += out_iovec[i].iov_len;
    }
    total_length += out_length;
#ifdef	NO_RAW_IPHDR
    total_lengh += sizeof (struct ip);
#else	/* NO_RAW_IPHDR */
    out_ip->ip_len = out_length;
#endif	/* NO_RAW_IPHDR */

#ifdef	SO_SNDBUF
    do {
	int send_buf = 0;
	int send_buf_len = sizeof send_buf;
	
	if (getsockopt(raw_socket, SOL_SOCKET, SO_SNDBUF, (char *) &send_buf, &send_buf_len) < 0) {
	    perror("getsockopt: SO_SNDBUF");
	    break;
	}

	if (send_buf < total_length) {
	    send_buf = total_length;
	    if (setsockopt(raw_socket, SOL_SOCKET, SO_SNDBUF, (char *) &send_buf, sizeof send_buf) < 0) {
		perror("setsockopt: SO_SNDBUF");
	    }
	}
    } while (0) ;
#endif	/* SO_SNDBUF */

    /* Determine if output is to a tty and set line width if so */
    if (isatty(fileno(stdout))) {
	signal(SIGWINCH, window_size);
	window_size();
    }
    
    setlinebuf(stdout);

    signal(SIGINT, intr);

    /* Let user own process once the options have been set */
    setuid(getuid());
    
    return (ping(out_iovec, out_length));

}


void
record_stats(sp, rtt)
stats *sp;
struct timeval *rtt;
{

    register u_long sec, usec;
    register double t;
#ifdef	MORESTATS
    register double tn;
#endif

    /*
     * Move the values into registers for play
     */
    sec = rtt->tv_sec;
    usec = rtt->tv_usec & (~TV_NEG);
    t = (double)sec + ((double)usec) * 0.000001;

    /*
     * Lot's of garbage to deal correctly with negative values (if you
     * get any something is broken).
     */
    if (sp->tsn == 0) {
	sp->tmin.tv_sec = sp->tmax.tv_sec = sec;
	if (rtt->tv_usec & TV_NEG) {
	    usec |= TV_NEG;
	    t = -t;
	}
	sp->tmin.tv_usec = sp->tmax.tv_usec = usec;
    } else if (rtt->tv_usec & TV_NEG) {
	if (!(sp->tmin.tv_usec & TV_NEG)
	  || (sec > sp->tmin.tv_sec)
	  || ((sec == sp->tmin.tv_sec)
	    && (usec > (sp->tmin.tv_usec & ~TV_NEG)))) {
	    sp->tmin.tv_sec = sec;
	    sp->tmin.tv_usec = usec | TV_NEG;
	} else if ((sp->tmax.tv_usec & TV_NEG)
	  && ((sec < sp->tmax.tv_sec)
	    || ((sec == sp->tmax.tv_sec)
	      && (usec < (sp->tmax.tv_usec & ~TV_NEG))))) {
	    sp->tmax.tv_sec = sec;
	    sp->tmax.tv_usec = usec | TV_NEG;
	}
	t = -t;
    } else {
	if (!(sp->tmin.tv_usec & TV_NEG)
	  && ((sec < sp->tmin.tv_sec)
	  || ((sec == sp->tmin.tv_sec) && (usec < sp->tmin.tv_usec)))) {
	    sp->tmin.tv_sec = sec;
	    sp->tmin.tv_usec = usec;
	} else if ((sp->tmax.tv_usec & TV_NEG)
	  || (sec > sp->tmax.tv_sec)
	    || ((sec == sp->tmax.tv_sec) && (usec > sp->tmax.tv_usec))) {
	    sp->tmax.tv_sec = sec;
	    sp->tmax.tv_usec = usec;
	}
    }

    sp->tsn++;
    sp->tsum += t;
#ifdef	MORESTATS
    tn = t;
    sp->tsum2 += (tn *= t);
    sp->tsum3 += (tn *= t);
    sp->tsum4 += (tn *= t);
#endif	/* MORESTATS */
}


/* Parse a numeric IP address or symbolic host name and return a sockaddr_in */
char *
a2sockaddr(addr, cp, name)
struct sockaddr_in *addr;
char *cp;
char *name;
{
    struct hostent *hp;
    static char buf[80];

#if	defined(HOST_NOT_FOUND) && !defined(NOH_NERR)
    extern int h_errno;
    extern int h_nerr;
    extern char *h_errlist[];

#else				/* HOST_NOT_FOUND */
#undef	HOST_NOT_FOUND
#define	HOST_NOT_FOUND	0
#define	h_errno	0
#define	h_nerr	1
    static const char *h_errlist[h_nerr] =
    {
	"unknown host"
    };

#endif				/* HOST_NOT_FOUND */

    bzero((caddr_t) addr, sizeof(*addr));
    addr->sin_family = AF_INET;
#ifndef	BSD4_4
#define	inet_aton(cp, saddr)	(((saddr)->s_addr = inet_addr((char *) cp)) != -1)
#endif	/* BSD4_4 */
    if (!inet_aton(cp, &addr->sin_addr)) {
	hp = gethostbyname(cp);
	if (hp) {
	    addr->sin_family = hp->h_addrtype;
	    bcopy(hp->h_addr, (caddr_t) & addr->sin_addr, hp->h_length);
	    cp = hp->h_name;
	} else {
	    sprintf(buf, "%s: %s",
		  h_errlist[h_errno < h_nerr ? h_errno : HOST_NOT_FOUND],
		    cp);
	    return buf;
	}
    }
    if (name) {
	strncpy(name, cp, MAXHOSTNAMELEN);
    }
    return (char *) 0;
}
