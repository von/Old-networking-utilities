#ifndef lint
static char sccsid[] = "@(#)print.c	4.5 (Berkeley) 4/14/86";
#endif

#include "ping.h"
  
struct types icmp_types[ICMP_MAXTYPE+1] = {
  { TRUE, "EchoReply" },
  { FALSE },
  { FALSE },
  { FALSE, "UnReachable" },
  { FALSE, "SourceQuench" },
  { FALSE, "ReDirect" },
  { FALSE },
  { FALSE },
  { TRUE, "Echo" },
  { FALSE, "RouterAdvertisement" },
  { FALSE, "RouterSolicitation" },
  { FALSE, "TimeExceeded" },
  { FALSE, "ParamaterProblem" },
  { TRUE, "TimeStamp" },
  { TRUE, "TimeStampReply" },
  { TRUE, "InfoRequest" },
  { TRUE, "InfoReply" },
  { TRUE, "MaskRequest" },
  { TRUE, "MaskReply" }
};


/*
 * Print a time value
 */
static inline void
pr_usec(tvp, pref, suff)
struct timeval *tvp;
const char *pref;
const char *suff;
{
    long usec = tvp->tv_usec;
    char *neg = "";

    if (usec & TV_NEG) {
	neg = "-";
	usec &= ~TV_NEG;
    }
    if (utiming) {
	printf("%s%s%d.%03d%s",
	       pref,
	       neg,
	       (tvp->tv_sec * 1000 + usec / 1000),
	       (usec % 1000),
	       suff);
    } else {
	printf("%s%s%d%s",
	       pref,
	       neg,
	       (tvp->tv_sec * 1000 + usec / 1000),
	       suff);
    }
}


/*
 * Print a time value
 */
static inline void
pr_msec(tvp, pref, suff)
struct timeval *tvp;
const char *pref;
const char *suff;
{
    long usec = tvp->tv_usec;
    char *neg = "";

    if (usec & TV_NEG) {
	neg = "-";
	usec &= ~TV_NEG;
    }
    printf("%s%s%d%s",
	   pref,
	   neg,
	   (tvp->tv_sec * 1000 + usec / 1000),
	   suff);
}


/*
 *  Print a descriptive string about an ICMP header.
 */
void
pr_icmph(info)
recv_info *info;
{
    printf("%s from %s: len=%d ttl=%d",
	   icmp_types[info->recv_type].name,
	   BIT_TEST(pingflags, OF_SYMBOLIC) ? pr_addr(info->recv_addr.sin_addr) : pr_ntoa(info->recv_addr.sin_addr),
	   info->recv_len - info->recv_ip_hlen,
	   info->recv_ip->ip_ttl);
    if (icmp_types[info->recv_type].seq) {
	printf(" seq=%u id=%u",
	       info->recv_seq,
	       ntohs(info->recv_icmp->icmp_id));
    }

    switch (info->recv_icmp->icmp_type) {
    case ICMP_ECHOREPLY:
        if (timing) {
	    pr_usec(&info->recv_rtt, " time=", " ms.\n");
	} else {
	    printf("\n");
	}
	break;

    case ICMP_ECHO:
	printf(".\n");
	/* XXX - Data */
	break;

    case ICMP_UNREACH:
	switch (info->recv_icmp->icmp_code) {
	case ICMP_UNREACH_NET:
	    printf(" Network.\n");
	    break;
	case ICMP_UNREACH_HOST:
	    printf(" Host.\n");
	    break;
	case ICMP_UNREACH_PROTOCOL:
	    printf(" Protocol.\n");
	    break;
	case ICMP_UNREACH_PORT:
	    printf(" Port.\n");
	    break;
	case ICMP_UNREACH_NEEDFRAG:
	    printf(" Frag needed and DF set.\n");
	    break;
	case ICMP_UNREACH_SRCFAIL:
	    printf(" Source Route Failed.\n");
	    break;
	default:
	    printf(" Bad Code: %d.\n",
		   info->recv_icmp->icmp_code);
	    break;
	}
	/* Print returned IP header information */
	pr_retip((struct ip *)info->recv_icmp->icmp_data);
	break;

    case ICMP_SOURCEQUENCH:
	printf(".\n");
	pr_retip((struct ip *)info->recv_icmp->icmp_data);
	break;

    case ICMP_REDIRECT:
	switch (info->recv_icmp->icmp_code) {
	case ICMP_REDIRECT_NET:
	    printf(" Network");
	    break;
	case ICMP_REDIRECT_HOST:
	    printf(" Host");
	    break;
	case ICMP_REDIRECT_TOSNET:
	    printf(" TOS Network");
	    break;
	case ICMP_REDIRECT_TOSHOST:
	    printf(" TOS Host");
	    break;
	default:
	    printf(" Bad Code: %u",
		   info->recv_icmp->icmp_code);
	    break;
	}
	printf(" to %s.\n",
	       pr_ntoa(info->recv_icmp->icmp_gwaddr));
	pr_retip((struct ip *)info->recv_icmp->icmp_data);
	break;

    case ICMP_TIMXCEED:
	switch (info->recv_icmp->icmp_code) {
	case ICMP_TIMXCEED_INTRANS:
	    printf(" TTL exceeded.\n");
	    break;
	case ICMP_TIMXCEED_REASS:
	    printf(" Frag reassembly time exceeded.\n");
	    break;
	default:
	    printf(" Bad Code: %d.\n",
		   info->recv_icmp->icmp_code);
	    break;
	}
	pr_retip((struct ip *)info->recv_icmp->icmp_data);
	break;

    case ICMP_PARAMPROB:
	printf(" pointer=0x%02x\n",
	       info->recv_icmp->icmp_pptr);
	pr_retip((struct ip *)info->recv_icmp->icmp_data);
	break;

    case ICMP_TSTAMP:
    case ICMP_TSTAMPREPLY:
        {
	    time_t otime = ntohl(info->recv_icmp->icmp_otime);
	    time_t rtime = ntohl(info->recv_icmp->icmp_rtime);
	    time_t ttime = ntohl(info->recv_icmp->icmp_ttime);

	    printf(" otime=%02u:%02u:%02u rtime=%02u:%02u:%02u ttime=%02u:%02u:%02u ms.\n",
		   otime / 3600000,
		   otime / 60000 % 60,
		   otime / 1000 % 60,
		   rtime / 3600000,
		   rtime / 60000 % 60,
		   rtime / 1000 % 60,
		   ttime / 3600000,
		   ttime / 60000 % 60,
		   ttime / 1000 % 60);
	}
	break;

    case ICMP_IREQ:
	printf(".\n");
	break;

    case ICMP_IREQREPLY:
	printf(" to=%s.\n",
	       pr_ntoa(info->recv_ip->ip_dst));
	break;
	
    case ICMP_MASKREQ:
	printf(".\n");
	break;

    case ICMP_MASKREPLY:
        {
	    struct in_addr in;

	    in.s_addr = info->recv_icmp->icmp_mask;
		
	    printf(" mask=%s.\n",
		   pr_ntoa(in));
	}
	break;

    case ICMP_ROUTERADV:
	printf(" to=%s\n\t(%u %u byte entrie%s, lifetime %u)\n",
	       pr_ntoa(info->recv_ip->ip_dst),
	       info->recv_icmp->icmp_addrnum,
	       info->recv_icmp->icmp_addrsiz * sizeof (struct in_addr),
	       info->recv_icmp->icmp_addrnum ? "s" : "",
	       ntohs(info->recv_icmp->icmp_lifetime));
	if (info->recv_icmp->icmp_addrsiz == sizeof (struct id_rdisc) / sizeof (struct in_addr)) {
	    struct id_rdisc *rdp = info->recv_icmp->icmp_rdisc;
	    struct id_rdisc *lp = rdp + info->recv_icmp->icmp_addrnum;

	    for (; rdp < lp; rdp++) {
		if ((caddr_t) (rdp + 1) > (caddr_t) info->recv_icmp + info->recv_icmp_len) {
		    printf("\tOut of data!\n");
		    break;
		}
		printf("\t%-15s preference %d\n",
		       pr_ntoa(rdp->ird_addr),
		       ntohl(rdp->ird_pref));
	    }
	}
	break;

    case ICMP_ROUTERSOL:
	printf(" to=%s.\n",
	       pr_ntoa(info->recv_ip->ip_dst));
	break;

    default:
	printf(" Bad ICMP type: %d\n", info->recv_icmp->icmp_type);
	break;
    }
}


/*
 *  Print an IP header with options.
 */
void
pr_iph(ip)
struct ip *ip;
{
    int hlen;
    unsigned char *cp;

    hlen = ip->ip_hl << 2;
    cp = (unsigned char *) ip + 20;	/* point to options */

    printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src              Dst Data\n");
    printf(" %1d %2d  %02x %04d %04x",
	   ip->ip_v,
	   ip->ip_hl,
	   ip->ip_tos,
	   ip->ip_len,
	   ip->ip_id);
    printf("   %1x %04x",
	   ((ip->ip_off) & 0xe000) >> 13,
	   (ip->ip_off) & 0x1fff);
    printf(" %3d %3d %04x",
	   ip->ip_ttl,
	   ip->ip_p,
	   ip->ip_sum);
    printf(" %-15s",
	   pr_ntoa(ip->ip_src));
    printf(" %-15s ",
	   pr_ntoa(ip->ip_dst));
    /* dump and option bytes */
    while (hlen-- > 20) {
	printf("%02x", *cp++);
    }
    printf("\n");
}


/* Print an IP address as a dotted quad.  We could use inet_ntoa, but */
/* that breaks with GCC on a Sun4 */
char *
pr_ntoa(addr)
struct in_addr addr;
{
    static char buf[18];
    register char *bp = buf;
    register u_char *cp = (u_char *) &addr;
    register int i = sizeof (struct in_addr);
    register int c;

    while (i--) {
	if (c = *cp++) {
	    register int leading = 0;
	    register int power = 100;

	    do {
		if (c >= power) {
		    *bp++ = '0' + c/power;
		    c %= power;
		    leading = 1;
		} else if (leading || power == 1) {
		    *bp++ = '0';
		}
	    } while (power /= 10) ;
	} else {
	    *bp++ = '0';
	}
	if (i) {
	    *bp++ = '.';
	}
    }
    *bp = (char) 0;
    return buf;
}

/*
 *  Return an ascii host address
 *  as a dotted quad and optionally with a hostname
 */
char *
pr_addr(addr)
struct in_addr addr;
{
    struct hostent *hp;
    static char buf[80];

    if (BIT_TEST(pingflags, OF_NUMERIC)
	|| (hp = gethostbyaddr((char *) &addr.s_addr, 4, AF_INET)) == NULL)
	sprintf(buf, "%s",
		pr_ntoa(addr));
    else
	sprintf(buf, "%s (%s)",
		hp->h_name,
		pr_ntoa(addr));

    return (buf);
}

/*
 *  Dump some info on a returned (via ICMP) IP packet.
 */
void
pr_retip(ip)
struct ip *ip;
{
    int hlen;
    unsigned char *cp;

    pr_iph(ip);
    hlen = ip->ip_hl << 2;
    cp = (unsigned char *) ip + hlen;

    if (ip->ip_p == 6) {
	printf("TCP: from port %d, to port %d (decimal)\n",
	       (*cp * 256 + *(cp + 1)), (*(cp + 2) * 256 + *(cp + 3)));
    } else if (ip->ip_p == 17) {
	printf("UDP: from port %d, to port %d (decimal)\n",
	       (*cp * 256 + *(cp + 1)), (*(cp + 2) * 256 + *(cp + 3)));
    }
}


#ifdef	IP_OPTIONS
void
pr_rroption(optval)
u_char *optval;
{
    u_char *optptr = &optval[IPOPT_MINOFF - 1];
    int optlen = optval[IPOPT_OFFSET] - IPOPT_MINOFF;

    column = 8;
    printf("\t");

    while (optlen > 0) {
	struct in_addr addr;
	int len = sizeof(addr);
	char *cp;
	u_char *ucp = (u_char *) &addr;

	optlen -= len;
	while (len--) {
	    *ucp++ = *optptr++;
	}

	cp = pr_addr(addr);

	if (column + strlen(cp) + 1 > column_max) {
	    printf("\n\t");
	    column = 8;
	} else if (column > 8) {
	    printf(" ");
	    column++;
	}
	printf(cp);
	column += strlen(cp);
    }
    if (column > 8) {
	printf("\n");
    }
}



void
pr_ipopt(ipopt, ipopt_len)
u_char *ipopt;
int ipopt_len;
{
  int optlen;

  if (!ipopt_len)
    return;

  printf("IP options(%d):",
	 ipopt_len);

  /* XXX - need to stay within screen width */

  for (optlen = -1; optlen && ipopt_len > 0; ipopt += optlen, ipopt_len -= optlen) {
    u_char option = ipopt[IPOPT_OPTVAL];

    switch (option) {
      case IPOPT_EOL:
        optlen = 0;
        printf(" EOL");
	if (--ipopt_len) {
	  printf("-%d",
		 ipopt_len);
	}
	break;

      case IPOPT_NOP:
	optlen = 1;
	printf(" NOP");
	break;

      default:
	optlen = ipopt[IPOPT_OLEN];
	switch (option) {
	  case IPOPT_RR:
	    printf(" RR");
	    goto route;

	  case IPOPT_TS:
	    printf(" TS(%d)",
		   optlen);
	    break;

	  case IPOPT_SECURITY:
	    printf(" SECURITY(%d)",
		   optlen);
	    break;

	  case IPOPT_LSRR:
	    printf(" LSRR");
	    goto route;

	  case IPOPT_SATID:
	    printf(" SATID(%d)",
		   optlen);
	    break;

	  case IPOPT_SSRR:
	    printf(" SSRR");

	  route:
	    {
	      u_char *op = ipopt + IPOPT_MINOFF - 1;

	      printf("(%d",
		     optlen);
	      if (optlen % sizeof (struct in_addr) != IPOPT_MINOFF - 1) {
		printf("!bad|");
	      }
	      printf(") {");
	      if (ipopt[IPOPT_OFFSET] == IPOPT_MINOFF) {
		printf("#");
	      }
	      
	      while (op < ipopt + optlen) {
		struct in_addr addr;
		u_char *cp = (u_char *) &addr;
		int len = sizeof (addr);

		while (len--) {
		  *cp++ = *op++;
		}
		if (addr.s_addr) {
		  printf("%s%c",
			 pr_ntoa(addr),
			 op == ipopt + ipopt[IPOPT_OFFSET] -1 ? '#' : ' ');
		}
	      }

	      printf("}");
	    }
	    break;

	  default:
	    printf(" Unknown option %d(%d)",
		   option,
		   optlen);
	}
    }
  }
  printf("\n");
}
#endif	/* IP_OPTIONS */


/*
 *	Calculate and print RTT stats
 */
void
pr_stats(sp)
stats *sp;
{
    double mean;

#ifdef	MORESTATS
    double mean2, mean3, mean4, var, sd, sk, kt;

#endif	/* MORESTATS */

    if (sp->tsn) {
	mean = sp->tsum / ((double)(sp->tsn));
	if (utiming) {
	    printf("round-trip (ms)\tmin/avg/max = %s%d.%03d/%.3lf/%s%d.%03d\n",
	     ((sp->tmin.tv_usec & TV_NEG) ? "-" : ""),
	     (sp->tmin.tv_sec * 1000 + (sp->tmin.tv_usec & ~TV_NEG) / 1000),
	     ((sp->tmin.tv_usec & ~TV_NEG) % 1000),
	     mean * 1000.0,
	     ((sp->tmax.tv_usec & TV_NEG) ? "-" : ""),
	     (sp->tmax.tv_sec * 1000 + (sp->tmax.tv_usec & ~TV_NEG) / 1000),
	     ((sp->tmax.tv_usec & ~TV_NEG) % 1000));
	} else {
	    printf("round-trip (ms)\tmin/avg/max = %s%d/%.0lf/%s%d\n",
	     ((sp->tmin.tv_usec & TV_NEG) ? "-" : ""),
	     (sp->tmin.tv_sec * 1000 + (sp->tmin.tv_usec & ~TV_NEG) / 1000),
	     mean * 1000.0,
	     ((sp->tmax.tv_usec & TV_NEG) ? "-" : ""),
	     (sp->tmax.tv_sec * 1000 + (sp->tmax.tv_usec & ~TV_NEG) / 1000));
	}
#ifdef	MORESTATS
	if (sp->tsn > 1) {
	    mean2 = mean * mean;
	    mean3 = mean2 * mean;
	    mean4 = mean3 * mean;
	    var = (sp->tsum2 - mean * sp->tsum) / (sp->tsn - 1.0);
	    if (var > 1.0e-20) {
		sd = sqrt(var);
		/* XXX This one is screwed up.  Should have a mean3 somewhere */
#ifdef notdef
		sk = (sp->tsum3 - 3.0 * mean * sp->tsum2 + 3.0 * mean2 * sp->tsum - mean2 * sp->tsum) / (sp->tsn * var * sd);
#endif
		sk = (sp->tsum3 - 3.0 * mean * sp->tsum2 + 3.0 * mean2 * sp->tsum - mean3 * sp->tsn) / (sp->tsn * var * sd);
		kt = (sp->tsum4 - 4.0 * mean * sp->tsum3 + 6.0 * mean2 * sp->tsum2 - 4.0 * mean3 * sp->tsum + sp->tsn * mean4) /
		    (sp->tsn * var * var);
		var *= 1.0e6;		/* convert to milliseconds */
		sd *= 1.0e3;		/* same here */
		if (utiming) {
		    printf("\t\tvar/sdev/skew/kurt = %.3lf/%.3lf/%.3lf/%.3lf\n",
			   var, sd, sk, kt);
		} else {
		    printf("\t\tvar/sdev/skew/kurt = %.1lf/%.1lf/%.1lf/%.1lf\n",
			   var, sd, sk, kt);
		}
#ifdef	notdef
	    } else {
		printf("\t\tvar/sdev = 0.00/0.00\n");
#endif	/* notdef */
	    }
	}
#endif	/* MORESTATS */
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
void
pr_packet(info)
recv_info *info;
{
    const char *state;

    printf("%s from %s: len=%d ttl=%d",
	   icmp_types[info->recv_type].name,
	   BIT_TEST(pingflags, OF_SYMBOLIC) ? pr_addr(info->recv_addr.sin_addr) : pr_ntoa(info->recv_addr.sin_addr),
	   info->recv_len - info->recv_ip_hlen,
	   info->recv_ip->ip_ttl);
    if (icmp_types[info->recv_type].seq) {
	printf(" seq=%d",
	       info->recv_seq);
    }

    switch (info->recv_type) {
    case ICMP_ECHOREPLY:
        if (timing) {
	    pr_usec(&info->recv_rtt, " time=", " ms");
	}
	break;

    case ICMP_TSTAMPREPLY:
	if (timing) {
	    pr_msec(&info->recv_rtt, " time=", "");
	    pr_msec(&info->recv_ott, " out=", "");
	    pr_msec(&info->recv_tat, " turn=", "");
	    pr_msec(&info->recv_itt, " in=", " ms");
	}
	break;

    case ICMP_MASKREPLY:
	{
	    struct in_addr in;

	    in.s_addr = info->recv_icmp->icmp_mask;
	    
	    printf(" mask=%s",
		   pr_ntoa(in));
	}
	break;

    case ICMP_IREQREPLY:
    case ICMP_ROUTERADV:
    case ICMP_ROUTERSOL:
	printf(" to=%s",
	       pr_ntoa(info->recv_ip->ip_dst));
	break;

    case ICMP_UNREACH:
	switch (info->recv_icmp->icmp_code) {
	case ICMP_UNREACH_NET:
	    printf(" network");
	    break;
	    
	case ICMP_UNREACH_HOST:
	    printf(" host");
	    break;
	    
	case ICMP_UNREACH_PROTOCOL:
	    printf(" protocol %d",
		   info->recv_ip->ip_p);
	    break;
	    
	case ICMP_UNREACH_PORT:
	    printf(" port");
	    break;
	    
	case ICMP_UNREACH_NEEDFRAG:
	    printf(" need frag");
	    break;
	    
	case ICMP_UNREACH_SRCFAIL:
	    printf(" source route fail");
	    break;
	    
	default:
	    printf(" invalid code %d",
		   info->recv_icmp->icmp_code);
	}
    }

    switch (info->recv_state & RSF_MASK) {
    case RSF_DUPLICATE:
	state = " Duplicate!";
	break;

    case RSF_CORRUPT:
	state = " Corrupt!";
	break;

    default:
	state = ".";
	break;
    }

    printf("%s\n",
	   state);

#ifdef	IP_OPTIONS
	if (BIT_TEST(pingflags, OF_PROPTS)
	    && info->recv_ipopt_len) {
	  pr_ipopt(info->recv_ipopt, info->recv_ipopt_len);
	}
#endif	/* IP_OPTIONS */
	
}
