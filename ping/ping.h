/*
 *  static char sccsid[] = "@(#)ping.h";
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/param.h>
#ifdef	_IBMR2
#include <time.h>
#else	/* _IBMR2 */
#include <sys/time.h>
#endif	/* _IBMR2 */
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#ifdef	_IBMR2
#include <sys/select.h>
#endif	/* _IBMR2 */

#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include "ip_icmp.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#include <ctype.h>

#ifndef	__STDC__
#define	const
#endif	/* __STDC__ */

#ifndef	__GNUC__
#define	inline
#endif	/* __GNUC__ */

#define	BIT_SET(f, b)	((f) |= b)
#define	BIT_RESET(f, b)	((f) &= ~(b))
#define	BIT_FLIP(f, b)	((f) ^= (b))
#define	BIT_TEST(f, b)	((f) & (b))
#define	BIT_MATCH(f, b)	(((f) & (b)) == (b))
#define	BIT_COMPARE(f, b1, b2)	(((f) & (b1)) == b2)
#define	BIT_MASK_MATCH(f, g, b)	(!(((f) ^ (g)) & (b)))

#ifdef	MORESTATS
#include <math.h>
#endif	/* MORESTATS */

#ifndef	TRUE
#define	TRUE	1
#define	FALSE	0
#endif	/* TRUE */

#define	MAXWAIT		10		/* max time to wait for response, sec. */
#define	MAXDATA		16394
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	64
#endif

#define	C_COLS	80
#define	C_PUT(s, l)	if ((column += l) > column_max) \
			{ \
			    column = l; \
			    write(fileno(stdout), "\n", 1); \
			} \
  			write(fileno(stdout), s, sizeof (s));

#define	RECV_BUF	48*1024		/* Size of receive buffer to specify */

#define OF_VERBOSE		0x01		/* verbose flag */
#define OF_QUIET		0x02		/* quiet flag */
#define OF_FLOOD		0x04		/* floodping flag */
#define	OF_RROUTE		0x08		/* record route flag */
#define	OF_CISCO		0x10		/* Cisco style pings */
#define	OF_NUMERIC		0x20		/* Numeric IP addresses only */
#define	OF_MISSING		0x40		/* Print table of missing responses */
#define	OF_FAST			0x80		/* Send an echo request whenever we get a response */
#define	OF_FILLED		0x0100		/* Fill packets with specified data */
#define	OF_LSROUTE		0x0200		/* Loose source routing */
#define	OF_SSROUTE		0x0400		/* Strict source routing */
#define	OF_DONTROUTE		0x0800		/* Don't use routing table */
#define	OF_DEBUG		0x1000		/* Turn on socket debugging */
#define	OF_PROPTS		0x2000		/* Print IP options fields */
#define	OF_INCRTTL		0x4000		/* Increment TTL until an echo is received */
#define	OF_SYMBOLIC		0x8000		/* Print all IP addresses as symbolic */
#ifdef	IFF_MULTICAST
#define	OF_MNOLOOP		0x010000	/* No multicast LOOPBACK */
#define	OF_MIF			0x020000	/* Multicast interface specified */
#define	OF_MTTL			0x040000	/* Multicast TTL specified */
#define	OF_MJOIN		0x080000	/* Join the Multicast group */
#define	OF_MULTICAST		0x100000	/* Destination is multicast */
#endif	/* IFF_MULTICAST */

/* MAX_DUP_CHK is the number of bits in received table, ie the */
/*      maximum number of received sequence numbers we can keep track */
/*      of.  Change 128 to 8192 for complete accuracy... */

#define MAX_DUP_CHK     8 * 8192
#define A(bit)          rcvd_tbl[ (bit>>3) ]	/* identify byte in array */
#define B(bit)          ( 1 << (bit & 0x07) )	/* identify bit in byte */
#define SET(bit)        A(bit) |= B(bit)
#define CLR(bit)        A(bit) &= (~B(bit))
#define TST(bit)        (A(bit) & B(bit))

/*
 *	Structure for statistics on RTT times
 */
typedef struct _stats {
    long tn;				/* Number of responses */
    long td;				/* Number of duplicates */
    long tc;				/* Number of coruppted */
    long tsn;				/* Number included in stats */
    struct timeval tmin;		/* Minimum RTT */
    struct timeval tmax;		/* Maximum RTT */
    double tsum;			/* Sum of RTTs */
#ifdef	MORESTATS
    double tsum2;			/* Sum of RTT^2 */
    double tsum3;			/* Sum of RTT^3 */
    double tsum4;			/* Sum of RTT^4 */
#endif	/* MORESTATS */
} stats;


/* Structure returned by recv_icmp */
typedef struct {
    int		recv_len;		/* Length of receive */
    struct ip *recv_ip;			/* Pointer to IP packet header */
    int		recv_ip_hlen;		/* Length of header */
    struct icmp *recv_icmp;		/* Pointer to ICMP packet header */
    int		recv_icmp_len;		/* Length of ICMP packet */
    u_char *recv_ipopt;			/* Pointer to received IP options */
    int		recv_ipopt_len;		/* Length of receive IP options */
    struct sockaddr_in recv_addr;	/* Source address */
    struct timeval recv_rtt;		/* Calculated route trip time */
    struct timeval recv_ott;		/* Outbound time */
    struct timeval recv_itt;		/* Inbound time */
    struct timeval recv_tat;		/* Turn-around time */
    int		recv_state;		/* State */
    int		recv_type;		/* Type of reply */
    int		recv_seq;		/* Sequence of reply */
} recv_info;
#define	RST_MASK	0x0f
#define	RST_REQUEST	0x01		/* Our request */
#define	RST_REPLY	0x02		/* The expected response */
#define	RST_RESPONSE	0x03		/* An unexpected response */
#define	RST_OTHER	0x04		/* Other ICMP packet */

#define	RSF_MASK	0xf0
#define	RSF_DUPLICATE	0x10		/* This packet was a duplicate */
#define	RSF_CORRUPT	0x20		/* This packet was corrupted */

#define	TV_NEG		0x80000000	/* Hi bit of recv_rtt.tv_usec is set */
					/*  when value is negative */

struct types {
    int seq;
    const char *name;
} ;

extern struct types icmp_types[];

/*
 *	Structure for storing responses for a specific route
 */
typedef struct _optstr {
    struct _optstr *next;
    stats optstats;
    u_char optval[MAX_IPOPTLEN];
} optstr;

struct opt_names {
    char op_letter;
    const char *op_arg;
    const char *op_name;
};


#ifdef	NO_RAW_IPHDR
#define	IOVEC_SIZE	2
#define	IOVEC_ICMP	0
#define	IOVEC_PATTERN	1
#else	/* NO_RAW_IPHDR */
#define	IOVEC_SIZE	3
#define	IOVEC_IP	0
#define	IOVEC_ICMP	1
#define	IOVEC_PATTERN	2
#endif	/* NO_RAW_IPHDR */

/*
 *	Shared data
 */
extern int timing;		/* Packets are long enough to time */
extern int utiming;		/* Do microsecond timing */
extern int datalen;
extern int column;		/* For keeping track of output column */
extern int column_max;		/* Screen width */
extern u_long pingflags;	/* User specified options */
extern u_char out_pattern[];
extern int mx_dup_ck;
extern char rcvd_tbl[];
extern stats tstat;
extern optstr *sumopt, *curopt;

/*
 *	Routine type declarations
 */
void record_stats();

#ifdef	__STDC__
recv_info *recv_icmp(int, u_char *, int, int, int);
int send_icmp(int, struct iovec *, int, int, struct sockaddr_in *);
u_short in_cksum(u_short *, int);
void pr_icmph(recv_info *);
void pr_iph(struct ip *);
void pr_retip(struct ip *);
void pr_rroption(u_char *);
void pr_stats(stats *);
char *pr_addr(struct in_addr);
char *pr_ntoa(struct in_addr);
void pr_ipopt(u_char *, int);
void pr_packet(recv_info *);
#else	/* __STDC__ */
recv_info *recv_icmp();
int send_icmp();
u_short in_cksum();
void pr_icmph();
void pr_iph();
void pr_retip();
void pr_rroption();
void pr_stats();
char *pr_addr();
char *pr_ntoa();
void pr_ipopt();
void pr_packet();
#endif	/* __STDC__ */
