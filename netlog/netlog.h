/*
	netlog.h

	This file should be included in the users application. It will
	redirect various calls so that they are logged by the netlog
	library.
*/

#ifndef NETLOG
#define NETLOG

#ifdef __hpux		/* Need to make sure these are included first */
#include <sys/socket.h>
#include <sys/uio.h>
#endif /* __hpux */

/*
	On the Cray sendmsg and recvmsg get redefined, so we check and
	make sure that's the case, then undefine them so the precompiler
	doesn't complain when we redefine them.
*/
#ifdef CRAY
# include <sys/types.h>
# include <sys/socket.h>
 
# ifdef recvmsg
#  undef recvmsg
# endif

# ifdef sendmsg
#  undef sendmsg
# endif

#endif


/*
	Used to insert comments into log
*/
void netlog_comment();


/*
	Macros to redirect user's libc calls to libnetlog
*/

#define accept(s, a, al)		log_accept(s, a, al)
int log_accept();

#define bind(s, n, nl)			log_bind(s, n, nl)
int log_bind();

#define close(s)			log_close(s)
int log_close();

#define connect(s, n, nl)		log_connect(s, n, nl)
int log_connect();

#define listen(s, bl)			log_listen(s, bl)
int log_listen();

#define recv(s, b, l, f)		log_recv(s, b, l, f)
int log_recv();

#define recvfrom(s, b, l, f, fr, frl)	log_recvfrom(s, b, l, f, fr, frl)
int log_recvfrom();

#define recvmsg(s, m, f)		log_recvmsg(s, m, f)
int log_recvmsg();

#define	read(s, b, nb)			log_read(s, b, nb)
int log_read();

#ifndef CRAY		/* Doesn't exist on the Cray */
#define readv(f, i, ic)			log_readv(f, i, ic)
int log_readv();
#endif

#define send(s, m, l, f)		log_send(s, m, l, f)
int log_send();

#define sendmsg(s, m, f)		log_sendmsg(s, m, f)
int log_sendmsg();

#define sendto(s, m, l, f, t, tl)	log_sendto(s, m, l, f, t, tl)
int log_sendto();

#define setsockopt(s, l, on, ov, ol)	log_setsockopt(s, l, on, ov, ol)
int log_setsockopt();

#define shutdown(s,h)			log_shutdown(s,h)
int log_shutdown();

#define	socket(d, t, p)			log_socket(d, t, p)
int log_socket();

#define socketpair(d, t, p, s)		log_socketpair(d, t, p, s)
int log_socketpair();

#define	write(s, b, nb)			log_write(s, b, nb)
int log_write();

#ifndef CRAY		/* Doesn't exit on the Cray */
#define writev(f, i, ic)		log_writev(f, i, ic)
int log_writev();
#endif


#endif /* NETLOG */
