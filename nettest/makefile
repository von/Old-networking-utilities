
FILES	=	nettest nettestd
SRCS	=	nettest.c nettestd.c nettest.h atoval.c pipe_exec.c get_load.c
NETTEST_OBJS	=	nettest.o atoval.o pipe_exec.o get_load.o
NETTESTD_OBJS	=	nettestd.o atoval.o pipe_exec.o get_load.o
LIBS	=
CFLAGS	=	$(FLAGS) -g
DESTDIR	=	./

all:	$(FILES)

nettest:	$(NETTEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $(NETTEST_OBJS) $(LIBS)


nettestd:	$(NETTESTD_OBJS)
	$(CC) $(CFLAGS) -o $@ $(NETTESTD_OBJS) $(LIBS)

nettest.o nettestd.o: nettest.h

tar:	$(SRCS) clean
	rm -f nettest.tar
	(cd ..; tar cfh nettest.tar nettest)
	mv ../nettest.tar .

sources:	$(SRCS)

$(SRCS):	
	co -q $@

clean:
	-rm -f $(FILES) *.o *~

install:	$(FILES)
	cp $(FILES) $(DESTDIR)
