PROGS	=	test1 test2
LIB	=	../libnetlog.a
CFLAGS	=	-I..


all:	$(PROGS)

test1:	test1.c $(LIB)
	cc $(CFLAGS) -o test1 test1.c -L.. -lnetlog

test2:	test2.c $(LIB)
	cc $(CFLAGS) -o test2 test2.c -L.. -lnetlog

clean:
	rm -f $(PROGS) *.o
