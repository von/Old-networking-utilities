/* USMID @(#)tcp/usr/etc/nettest/nettest.h	61.0	09/03/90 19:11:52 */
#define	CHUNK	4096
#define	NCHUNKS	100

#if	defined(CRAY) || defined(SYSV) || defined(RS6000) || defined(sgi)
#define USE_TIMES
#else
#define USE_FTIME
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/times.h>
#ifdef USE_TIMES
#include <sys/param.h>
#else /* USE_FTIMES */
#define	HZ	60
#include <sys/timeb.h>
#endif	/* USE_TIMES */
#define	UNIXPORT	"un_socket"
#define	PORTNUMBER	(IPPORT_RESERVED + 42)

#if !defined(CRAY) && !defined(__hpux)
#define WAIT3CODE
#endif /* CRAY */

#ifdef sun
extern malloc_t		valloc();
#endif

extern float get_load();
