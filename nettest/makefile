#
# For RS6000s add -DRS6000 to CFLAGS
#
FILES	=	nettest nettestd
SRCS	=	nettest.c nettestd.c nettest.h atoval.c
LIBS	=
CFLAGS	=	$(FLAGS)
DESTDIR	=	./

all:	$(FILES)

nettest:	nettest.c nettest.h atoval.c
	cc $(CFLAGS) -o nettest nettest.c atoval.c $(LIBS)

nettestd:	nettestd.c nettest.h atoval.c
	cc $(CFLAGS) -o nettestd nettestd.c atoval.c $(LIBS)

tar:	$(SRCS) clean
	rm -f nettest.tar
	(cd ..; tar cfh nettest.tar nettest)
	mv ../nettest.tar .

sources:	$(SRCS)

$(SRCS):	
	co -q $@

clean:
	rm -f $(FILES) *.o

install:	$(FILES)
	cp $(FILES) $(DESTDIR)
