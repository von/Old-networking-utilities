#
#
#	Makefile for ping
#
#	Defining MORESTATS will give more per-path statistics and
#	requires the math library.
#

#
#	Define MORESTATS to get standard deviation, var??, and kurtosis
#
#	Define NOH_NERR if your system has old resolver routines that
#	don't provide h_nerr.
#
#	Define NO_RAW_IPHDR if your system does not allow the
#	IP header to be specified on a raw socket.
#
#	Define PRECISECLOCK if you want microsecond timing output by default.
#	Otherwise the default will be millisecond timing output.  The -u
#	and -U flags allow either output format to be specified from the
#	command	line in either case.
#
SRCS=	main.c packet.c print.c
HDRS=	ping.h ip_icmp.h
OBJS=	main.o packet.o print.o

DEFINES = -DMORESTATS -DPRECISECLOCK
CC = gcc -W -Wunused -Wreturn-type -Wswitch -Wshadow -Wwrite-strings -Wcast-qual -Wpointer-arith -Wcomment
CC= cc
CFLAGS = -g
LDFLAGS = -lm
DESTDIR =
BINDIR = /usr/local/bin
MANDIR = /usr/local/man/man8
MANEXT = 8
TMPDIR = /tmp

all: ping

.c.o:
	${CC} ${CFLAGS} ${DEFINES} -c $*.c

ping:	${OBJS}
	${CC} -o $@ ${OBJS} ${LDFLAGS}

install:	ping
	install -c -s -m 4755 -o root ping ${DESTDIR}${BINDIR}
	install -c -m 644 ping.8 ${MANDIR}/ping.${MANEXT}

clean:
	rm -f ping core *.o linterrs

tar:	ping.tar.Z ping.tar.gz

ping.tar.Z:	${SRCS} ${HDRS} ping.8 Makefile
	@rm -rf ${TMPDIR}/ping ; mkdir ${TMPDIR}/ping
	@tar cf - ${SRCS} ${HDRS} ping.8 Makefile | ( cd ${TMPDIR}/ping ; tar xpf - )
	@chmod 444 ${TMPDIR}/ping/*
	@( cd ${TMPDIR} ; tar cf - ping ) | compress -c > ping.tar.Z
	@( cd ${TMPDIR} ; tar cf - ping ) | gzip -9 -c > ping.tar.gz
	@rm -rf ${TMPDIR}/ping

depend:

main.o:	main.c ${HDRS}
print.o: print.c ${HDRS}
packet.o: packet.c ${HDRS}
