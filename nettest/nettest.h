/*
 *	$Id: nettest.h,v 1.9 1996/02/21 22:03:29 vwelch Exp $
 */
/* USMID @(#)tcp/usr/etc/nettest/nettest.h	61.0	09/03/90 19:11:52 */

float	version = 1.0;

#define	CHUNK	4096
#define	NCHUNKS	100

#if defined(sun) && (defined(__svr4__) || !defined(bsd))
# define SOLARIS
# define SYSV
#endif

#if	defined(CRAY) || defined(SYSV) || defined(RS6000) || defined(sgi)
# define USE_TIMES
#else
# define USE_FTIME
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/times.h>
#ifdef USE_TIMES
# include <sys/param.h>
#else /* USE_FTIMES */
# ifndef HZ
#  define	HZ	60
# endif
# include <sys/timeb.h>
#endif	/* USE_TIMES */

#define	UNIXPORT	"un_socket"
#define	PORTNUMBER	(IPPORT_RESERVED + 42)

#if !defined(CRAY) && !defined(__hpux)
# define WAIT3CODE
#endif /* CRAY */

#ifdef CRAY
# define DONT_HAVE_VALLOC
#endif

#ifdef sgi
# define SIGNAL_NEEDS_RESET
#endif

#ifdef DONT_HAVE_VALLOC
# define	valloc(size)	malloc(size)
#endif

#if defined(sun)
extern void		*valloc();
#endif

#if defined(__convex__)
extern char		*valloc();
#endif

extern float get_load();

#if defined(sgi) || defined(SOLARIS)
# define NEED_IN_SYSTM_H
#endif

#if defined(IP_TOS)
# if !defined(RS6000) && !defined(__convex__) 
#  define DO_IP_TOS
# endif
#endif

#if !defined(CRAY) || defined(s_addr)
# define USE_S_ADDR
#endif
