/*
 *	nettest.h,v 1.9 1996/02/21 22:03:29 vwelch Exp
 */
/* USMID @(#)tcp/usr/etc/nettest/nettest.h	61.0	09/03/90 19:11:52 */

float	version = 1.0;

/*
 * Default Arguments
 */
#define	CHUNK	4096
#define	NCHUNKS	100

/*
 * Default port and socket names
 */
#define	UNIXPORT	"un_socket"
#define	PORTNUMBER	(IPPORT_RESERVED + 42)




#ifdef CRAY								/* CRAY */
# define DONT_HAVE_VALLOC
/* # define DONT_USE_S_ADDR */
# define HZ_USE_CLK_TCK
# define NEED_TIME_H
# ifdef CRAY2							/* CRAY2 */
#  define NEED_SYS_SYSMACROS_H
# endif
#endif

#ifdef RS6000							/* RS6000 */
/* Nothing */
#endif

#ifdef sgi								/* SGI */
# define SIGNAL_NEEDS_RESET
# define NEED_IN_SYSTM_H
#endif

#ifdef __alpha							/* Dec Alpha */
# define NEED_IN_SYSTM_H
# define HZ_USE_SYSCONF
# define NEED_UNISTD_H
# define INADDR_TYPE	u_int
#endif

#ifdef __hpux							/* HP */
# define SIGNAL_NEEDS_RESET
# define DONT_PROTO_TIMES
# define DONT_HAVE_GETDTABLESIZE
# define DONT_HAVE_VALLOC
# define NEED_SYS_RESOURCE_H
#endif

#ifdef sun								/* Sun */
# if defined(__svr4__) || !defined(bsd)	/* Solaris */
#  define NEED_IN_SYSTM_H
# else									/* SunOS */
/* Nothing */
# endif
/* SunOS or Solaris */
# define VALLOC_TYPE	void
# define USE_FTIME
#endif

#ifdef __convex__						/* Convex */
# define VALLOC_TYPE	char
# define DONT_DO_IP_TOS
# define DONT_PROTO_TIMES
# define HZ_USE_CLK_TCK
#endif

#ifdef RS6000							/* RS6000 */
# define DONT_DO_IP_TOS
#endif



#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/times.h>
#include <sys/param.h>

#ifdef USE_FTIME
# include <sys/timeb.h>
#endif



#ifdef DONT_HAVE_VALLOC
# define	valloc(size)	malloc(size)
#endif

#ifdef VALLOC_TYPE
extern VALLOC_TYPE		*valloc();
#endif


extern float get_load();


#if defined(IP_TOS) && !defined(DONT_DO_IP_TOS)
# define DO_IP_TOS
#endif

#if !defined(DONT_USE_S_ADDR)
# define USE_S_ADDR
#endif

#if !defined(INADDR_TYPE)
# define INADDR_TYPE		long
#endif

#if !defined(INADDR_NONE)
# define INADDR_NONE		-1
#endif
