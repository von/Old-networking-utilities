
#ifndef lint
static char sccsid[] = "@(#)packet.c	4.5 (Berkeley) 4/14/86";
#endif

#include "ping.h"

/*
 *			I N _ C K S U M
 *
 * Checksum routine for Internet Protocol family headers (C Version)
 *
 */
u_short
in_cksum(addr, len)
u_short *addr;
int len;
{
    register int nleft = len;
    register u_short *w = addr;
    register u_short answer;
    register int sum = 0;

    /*
     *  Our algorithm is simple, using a 32 bit accumulator (sum),
     *  we add sequential 16 bit words to it, and at the end, fold
     *  back all the carry bits from the top 16 bits into the lower
     *  16 bits.
     */
    while (nleft > 1) {
	sum += *w++;
	nleft -= 2;
    }

    /* mop up an odd byte, if necessary */
    if (nleft == 1) {
	sum += *(u_char *) w;
    }
    /*
     * add back carry outs from top 16 bits to low 16 bits
     */
    sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
    sum += (sum >> 16);			/* add carry */
    answer = ~sum;			/* truncate to 16 bits */
    return (answer);
}


/*
 * Checksum routine for Internet Protocol - Modified from 4.3+ networking in_chksum.c
 *
 */

#define ADDCARRY(x)  (x > 65535 ? x -= 65535 : x)
#define REDUCE {l_util.l = sum; sum = l_util.s[0] + l_util.s[1]; ADDCARRY(sum);}

u_short
in_cksumv(v, nv, len)
register struct iovec v[];		/* List of iovecs */
register int nv;			/* Number of iovecs */
register int len;			/* Length of data */
{
    register u_short *w;
    register int sum = 0;
    register int vlen = 0;
    register struct iovec *vp;
    int byte_swapped = 0;

    union {
	char c[2];
	u_short s;
    } s_util;
    union {
	u_short s[2];
	long l;
    } l_util;

    for (vp = v; nv && len; nv--, vp++) {
	if (vp->iov_len == 0) {
	    continue;
	}
	w = (u_short *) vp->iov_base;
	if (vlen == -1) {
	    /*
             * The first byte of this mbuf is the continuation
             * of a word spanning between this mbuf and the
             * last mbuf.
             *
             * s_util.c[0] is already saved when scanning previous
             * mbuf.
             */
	    s_util.c[1] = *(char *) w;
	    sum += s_util.s;
	    w = (u_short *) ((char *) w + 1);
	    vlen = vp->iov_len - 1;
	    len--;
	} else {
	    vlen = vp->iov_len;
	}
	if (len < vlen) {
	    vlen = len;
	}
	len -= vlen;
	/*
         * Force to even boundary.
         */
	if ((1 & (int) w) && (vlen > 0)) {
	    REDUCE;
	    sum <<= NBBY;
	    s_util.c[0] = *(u_char *) w;
	    w = (u_short *) ((char *) w + 1);
	    vlen--;
	    byte_swapped = 1;
	}
	/*
         * Unroll the loop to make overhead from
         * branches &c small.
         */
	while ((vlen -= 32) >= 0) {
	    sum += w[0];
	    sum += w[1];
	    sum += w[2];
	    sum += w[3];
	    sum += w[4];
	    sum += w[5];
	    sum += w[6];
	    sum += w[7];
	    sum += w[8];
	    sum += w[9];
	    sum += w[10];
	    sum += w[11];
	    sum += w[12];
	    sum += w[13];
	    sum += w[14];
	    sum += w[15];
	    w += 16;
	}
	vlen += 32;
	while ((vlen -= 8) >= 0) {
	    sum += w[0];
	    sum += w[1];
	    sum += w[2];
	    sum += w[3];
	    w += 4;
	}
	vlen += 8;
	if (vlen == 0 && byte_swapped == 0) {
	    continue;
	}
	REDUCE;
	while ((vlen -= 2) >= 0) {
	    sum += *w++;
	}
	if (byte_swapped) {
	    REDUCE;
	    sum <<= NBBY;
	    byte_swapped = 0;
	    if (vlen == -1) {
		s_util.c[1] = *(char *) w;
		sum += s_util.s;
		vlen = 0;
	    } else {
		vlen = -1;
	    }
	} else if (vlen == -1) {
	    s_util.c[0] = *(char *) w;
	}
    }
    if (len) {
	return 0;
    }
    if (vlen == -1) {
	/* The last buffer has odd # of bytes. Follow the
           standard (the odd byte may be shifted left by 8 bits
           or not as determined by endian-ness of the machine) */
	s_util.c[1] = 0;
	sum += s_util.s;
    }
    REDUCE;
    return ~sum & 0xffff;
}


#define	tvsub2(out, in, res) \
	    do { \
		register time_t Xin = (in); \
		register time_t Xout = (out); \
		register time_t Xtime; \
		register struct timeval *Xtvp = (res); \
		if (Xout > Xin) { \
		    Xtime = Xout - Xin; \
		    Xtvp->tv_usec = TV_NEG; \
		} else { \
		    Xtime = Xin - Xout; \
		    Xtvp->tv_usec = 0; \
		} \
		Xtvp->tv_sec = Xtime / 1000; \
		Xtvp->tv_usec |= (Xtime % 1000) * 1000; \
	    } while (0)
/*
 * 			T V S U B
 *
 * Subtract 2 timeval structs:  res = out - in.
 *
 * If not doing microsecond timing, rounds the result
 */
static inline void
tvsub(out, in, res)
register struct timeval *out, *in;
register struct timeval *res;
{
    register u_long usec;
    register int neg = 0;

    if ((out->tv_sec < in->tv_sec)
      || (out->tv_sec == in->tv_sec && out->tv_usec < in->tv_usec)) {
	register struct timeval *tmp;

	neg = 1;
	tmp = out;
	out = in;
	in = tmp;
    }
    if (out->tv_usec < in->tv_usec) {
	res->tv_sec = out->tv_sec - 1 - in->tv_sec;
	usec = (out->tv_usec + 1000000) - in->tv_usec;
    } else {
	res->tv_sec = out->tv_sec - in->tv_sec;
	usec = out->tv_usec - in->tv_usec;
    }

    if (!utiming) {
	register u_long tmp = (usec / 1000) * 1000;
	if ((usec - tmp) > 500) {
	    tmp += 1000;
	    if (tmp >= 1000000) {
		tmp -= 1000000;
		res->tv_sec++;
	    }
	}
	usec = tmp;
    }
    if (neg) {
	usec |= TV_NEG;
    }
    res->tv_usec = usec;
}


void
recv_ipopts(info)
recv_info *info;
{
    int ipopt_len = info->recv_ipopt_len;
    u_char *ipopt = info->recv_ipopt;
    int optlen = 1;

    /* Scan option list */
    for (; optlen && ipopt_len > 0; ipopt_len -= optlen, ipopt += optlen) {
	u_char option = ipopt[IPOPT_OPTVAL];

	switch (option) {
	case IPOPT_EOL:
	    break;

	case IPOPT_NOP:
	    optlen = 1;
	    break;

	default:
	    optlen = ipopt[IPOPT_OLEN];
	    
	    if (option == IPOPT_RR) {
		/* Save recorded route for later */
		for (curopt = sumopt; curopt; curopt = curopt->next) {
		    if (optlen != curopt->optval[IPOPT_OLEN])
			continue;
		    if (!bcmp(ipopt, curopt->optval, optlen))
			break;
		}
		if (!curopt) {
		    curopt = (optstr *) calloc(1, (sizeof(optstr)));
		    curopt->next = sumopt;
		    sumopt = curopt;
		    bcopy(ipopt, curopt->optval, optlen);
		}
		switch (info->recv_state & RSF_MASK) {
		case RSF_CORRUPT:		/* Corrupted */
		    curopt->optstats.tc++;
		    break;

		case RSF_DUPLICATE:		/* Duplicate */
		    curopt->optstats.td++;
		    break;

		default:
		    curopt->optstats.tn++;
		    if (timing) {
			record_stats(&curopt->optstats, &info->recv_rtt);
		    }
		}
	    }
	}
    }
}


/*
 *			P R _ P A C K
 *
 * Print out the packet, if it came from us.  This logic is necessary
 * because ALL readers of the ICMP socket get a copy of ALL ICMP packets
 * which arrive ('tis only fair).  This permits multiple copies of this
 * program to be run without having intermingled output (or statistics!).
 */
static int responses[ICMP_MAXTYPE+1] =
{
    0,			/* Echo Reply */
    0,
    0,
    0,			/* Unreachable */
    0,			/* Source Quench */
    0,			/* Redirect */
    0,
    0,
    ICMP_ECHOREPLY,	/* Echo */
    0,			/* Router solicitation */
    ICMP_ROUTERADV,	/* Router advertisement */
    0,			/* Time exceeded */
    0,			/* Parameter problem */
    ICMP_TSTAMPREPLY,	/* Time stamp */
    0,			/* Time stamp reply */
    ICMP_IREQREPLY,	/* Information request */
    0,			/* Information request reply */
    ICMP_MASKREPLY,	/* Mask request */
    0			/* Mask request reply */
};

recv_info *
recv_icmp(recv_socket, buf, length, id, type)
int recv_socket;
u_char *buf;
int length;
int id;
int type;
{
    struct timeval tv;
    static recv_info info;
    int addrlen = (sizeof info.recv_addr);

    info.recv_state = RST_OTHER;
    info.recv_rtt.tv_sec = info.recv_rtt.tv_usec = 0;
    info.recv_seq = 0;
    
    if ((info.recv_len = recvfrom(recv_socket,
				  buf,
				  length,
				  0,
				  (struct sockaddr *) &info.recv_addr,
				  &addrlen)) < 0) {
	perror("ping: recvfrom");
	return (recv_info *) 0;
    }

    if (gettimeofday(&tv, NULL) < 0) {
	perror("ping: gettimeofday");
    }

    info.recv_ip = (struct ip *) buf;
    info.recv_ip_hlen = info.recv_ip->ip_hl << 2;
    if (info.recv_len < info.recv_ip_hlen + ICMP_MINLEN) {
	if (BIT_TEST(pingflags, OF_VERBOSE))
	    printf("packet too short (%d bytes) from %s\n", info.recv_len,
		   pr_ntoa(info.recv_addr.sin_addr));
	return (recv_info *) 0;
    }
    info.recv_icmp = (struct icmp *) ((u_char *) info.recv_ip + info.recv_ip_hlen);
    info.recv_icmp_len = info.recv_len - info.recv_ip_hlen;

    info.recv_ipopt_len = info.recv_ip_hlen - sizeof (struct ip);
    if (info.recv_ipopt_len > 0) {
	info.recv_ipopt = (u_char *) (info.recv_ip + 1);
    } else {
	info.recv_ipopt = (u_char *) 0;
	info.recv_ipopt_len = 0;
    }

    info.recv_type = info.recv_icmp->icmp_type;
    if (icmp_types[info.recv_type].seq) {
	info.recv_seq = ntohs(info.recv_icmp->icmp_seq);
    }
    
    switch (info.recv_type) {
    case ICMP_ECHOREPLY:
    case ICMP_TSTAMPREPLY:
    case ICMP_IREQREPLY:
    case ICMP_MASKREPLY:
	/* A direct response */
        if (info.recv_icmp->icmp_type == responses[type] &&
	    info.recv_icmp->icmp_id == id) {
	    info.recv_state = RST_REPLY;
	}
	break;
	
    case ICMP_ECHO:
    case ICMP_TSTAMP:
    case ICMP_IREQ:
    case ICMP_MASKREQ:
	/* Our echo request */
	if (info.recv_icmp->icmp_type == type &&
	    info.recv_icmp->icmp_id == id) {
	    info.recv_state = RST_REQUEST;
	}
	break;

    case ICMP_UNREACH:
    case ICMP_SOURCEQUENCH:
    case ICMP_REDIRECT:
    case ICMP_TIMXCEED:
    case ICMP_PARAMPROB:
	/* An error response */
        {
	    int icmp_hl = info.recv_icmp->icmp_ip.ip_hl << 2;
	    struct icmp *icmp_icmp = (struct icmp *) ((caddr_t) &info.recv_icmp->icmp_ip + icmp_hl);

	    if (icmp_icmp->icmp_id == id) {
		info.recv_state = RST_RESPONSE;
		break;
	    }
	}
	break;

    case ICMP_ROUTERADV:
	if (info.recv_icmp->icmp_type == responses[type]) {
	    info.recv_state = RST_REPLY;
	}
	break;

    case ICMP_ROUTERSOL:
	if (info.recv_icmp->icmp_type == type) {
	    info.recv_state = RST_REQUEST;
	}
	break;
	
    default:
	break;
    }

    if (info.recv_state == RST_REPLY) {
	/* Check for duplicates */
	if (TST(info.recv_seq % mx_dup_ck)
#ifdef	OF_MULTICAST
	    && !BIT_TEST(pingflags, OF_MULTICAST)
#endif	/* OF_MULTICAST */
	    ) {
	    info.recv_state |= RSF_DUPLICATE;
	    tstat.td++;
	} else {
	    SET(info.recv_seq % mx_dup_ck);
	    tstat.tn++;
	}

	switch (info.recv_type) {
	case ICMP_ECHOREPLY:
	    if (timing) {
		/* Check for corrupted packets */
		if (bcmp((caddr_t) info.recv_icmp + ICMP_MINLEN + sizeof (struct timeval),
			 out_pattern + sizeof (struct timeval),
			 info.recv_len - info.recv_ip_hlen - ICMP_MINLEN - sizeof(struct timeval))) {
		    info.recv_state |= RSF_CORRUPT;
		    tstat.tc++;
		}

		/* Record the round trip time */
		tvsub(&tv,
		      (struct timeval *) & info.recv_icmp->icmp_data[0],
		      &info.recv_rtt);
		if (!(info.recv_state & RSF_MASK)) {
		    /* Only record statis if packets are good */
		    record_stats(&tstat, &info.recv_rtt);
		}
	    }
	    break;

	case ICMP_TSTAMPREPLY:
	    if (timing) {
		struct tm *tmp;
		time_t lt;

		tmp = gmtime(&tv.tv_sec);

		lt = tmp->tm_hour * 3600000
		    + (tmp->tm_min * 60000
		       + (tmp->tm_sec * 1000
			  + (tv.tv_usec / 1000)));
		tvsub2(ntohl(info.recv_icmp->icmp_otime),
		       lt,
		       &info.recv_rtt);
		tvsub2(ntohl(info.recv_icmp->icmp_otime),
		       ntohl(info.recv_icmp->icmp_rtime),
		       &info.recv_ott);
		tvsub2(ntohl(info.recv_icmp->icmp_ttime),
		       lt,
		       &info.recv_itt);
		tvsub2(ntohl(info.recv_icmp->icmp_rtime),
		       ntohl(info.recv_icmp->icmp_ttime),
		       &info.recv_tat);
		if (!(info.recv_state & RSF_MASK)) {
		    /* Only record statis if packets are good */
		    record_stats(&tstat, &info.recv_rtt);
		}
	    }
	    break;
	}

#ifdef	IP_OPTIONS
      /* Remember any IP route options */
      if (info.recv_ipopt_len) {
	  recv_ipopts(&info);
      }
#endif	/* IP_OPTIONS */
    }

    return &info;
}


/*
 * 			P I N G E R
 *
 * Compose and transmit an ICMP ECHO REQUEST packet.  The IP packet
 * will be added on by the kernel.  The ID field is our UNIX process ID,
 * and the sequence number is an ascending integer.  The first 8 bytes
 * of the data portion are used to hold a UNIX "timeval" struct in VAX
 * byte-order, to compute the round-trip time.
 */
int
send_icmp(s, iov, len, seq, addr)
int s;
struct iovec *iov;
int len;
int seq;
struct sockaddr_in *addr;
{
    int cc;
    struct icmp *icmp = (struct icmp *) iov[IOVEC_ICMP].iov_base;
    struct msghdr msghdr;

    bzero((caddr_t) &msghdr, sizeof (msghdr));
    msghdr.msg_name = (caddr_t) addr;
    msghdr.msg_namelen = sizeof (*addr);
    msghdr.msg_iov = iov;
    msghdr.msg_iovlen = IOVEC_SIZE;

#ifndef	NO_RAW_IPHDR
    if (BIT_TEST(pingflags, OF_INCRTTL)) {
	struct ip *ip = (struct ip *) iov[IOVEC_IP].iov_base;
	
	if (!++ip->ip_ttl) {
	    return 1;
	}
#ifdef	IFF_MULTICAST
	if (BIT_TEST(pingflags, OF_MULTICAST)
	    && setsockopt(s,
			  IPPROTO_IP,
			  IP_MULTICAST_TTL,
			  (caddr_t) &ip->ip_ttl,
			  sizeof (ip->ip_ttl)) == -1) {
	    perror ("can't set multicast time-to-live");
	    exit(93);
	}
#endif	/* IFF_MULTICAST */
    }
#endif	/* NO_RAW_IPHDR */

    icmp->icmp_cksum = 0;
    if (icmp_types[icmp->icmp_type].seq) {
	icmp->icmp_seq = htons(seq);
    }

    CLR(seq % mx_dup_ck);

    switch (icmp->icmp_type) {
	struct tm *tmp;
	struct timeval tv;

    case ICMP_ECHO:
	if (iov[IOVEC_PATTERN].iov_len >= sizeof (struct timeval)) {
	    /* Room for timestamp */
	    if (gettimeofday((struct timeval *) iov[IOVEC_PATTERN].iov_base, NULL) < 0) {
		perror("gettimeofday");
		return -1;
	    }
	}
	break;

    case ICMP_TSTAMP:
	if (gettimeofday(&tv, NULL) < 0) {
	    perror("gettimeofday");
	    return -1;
	}
	tmp = gmtime(&tv.tv_sec);
	icmp->icmp_otime = icmp->icmp_rtime = icmp->icmp_ttime
	    = htonl(tmp->tm_hour * 3600000
		    + (tmp->tm_min * 60000
		       + (tmp->tm_sec * 1000
			  + (tv.tv_usec / 1000))));
	break;
    }

    /* Compute ICMP checksum */
    icmp->icmp_cksum = in_cksumv(&iov[IOVEC_ICMP],
				 IOVEC_SIZE-IOVEC_ICMP,
				 iov[IOVEC_ICMP].iov_len + iov[IOVEC_PATTERN].iov_len);

    cc = sendmsg(s, &msghdr, 0);
    if (cc < 0 || cc != len) {
	if (cc < 0)
	    perror("sendto");
	printf("ping: wrote %s %d chars, ret=%d\n",
	       pr_ntoa(addr->sin_addr),
	       len,
	       cc);
	fflush(stdout);
    }
    return 0;
}

