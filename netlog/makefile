CLFAGS=

LIBRARY	=	libnetlog.a


OBJS	=	logger.o misc_rtns.o percent_parse.o read_rtns.o \
		support.o timer.o write_rtns.o

SRCS	=	logger.c misc_rtns.c percent_parse.c read_rtns.c \
		support.c timer.c write_rtns.c

HDRS	=	logger.h support.h timer.h

TARFILE	=	netlog.tar


all:	$(LIBRARY) loggerd

$(LIBRARY):	$(OBJS)
	ar cr $(LIBRARY) `lorder $(OBJS) | tsort`
	ranlib $(LIBRARY)

loggerd:	loggerd.o
	cc -o loggerd loggerd.o

logger.o:	logger.c logger.h timer.h
loggerd.o:	loggerd.c logger.h
misc_rtns.o:	misc_rtns.c logger.h support.h
read_rtns.o:	read_rtns.c logger.h support.h timer.h
support.o:	support.c support.h
timer.o:	timer.c logger.h timer.h
write_rtns.o:	write_rtns.c logger.h support.h timer.h


clean:
	rm -f *.o

tar:
	rm -f $(TARFILE) $(TARFILE).Z
	(cd ..;tar cfhFF $(TARFILE) netlog)
	mv ../$(TARFILE) .
	compress $(TARFILE)
