#include "netlog.h"


#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>


#define BUF_SIZE	10000

main()
{
	int s, accept_sock, sv[2], count, iovnum;
	struct sockaddr_in addr;
	struct msghdr message; 
	struct iovec iov[3];
	char buffer[BUF_SIZE], buf1[BUF_SIZE], buf2[BUF_SIZE];


	s = socket(PF_INET, SOCK_STREAM, 0);

	socketpair(PF_INET, SOCK_STREAM, 0, sv);

	socketpair(PF_UNIX, SOCK_STREAM, 0, sv);
	
	write(sv[0], buffer, 100);

	count = 0;

	while(count < 100)
		count += read(sv[1], buffer, 100 - count);
	

	shutdown(sv[0], 0);

	shutdown(sv[0], 1);

	shutdown(sv[1], 2);

	shutdown(s, 4);

	close(s);

	close(sv[0]);
	close(sv[1]);

	s = socket(PF_INET, SOCK_STREAM, 0);

	netlog_comment("Comment test.");

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(6767);

	bind(s, (struct sockaddr *) &addr, sizeof(addr));

	listen(s, 5);

	accept_sock = s;

	s = accept(accept_sock, NULL, NULL);
	
	write(s, buffer, BUF_SIZE);
	
#ifndef CRAY

	iovnum = 0;

	iov[iovnum].iov_base = buffer;
	iov[iovnum].iov_len = BUF_SIZE;
	iovnum++;

	iov[iovnum].iov_base = buf1;
	iov[iovnum].iov_len = BUF_SIZE;
	iovnum++;

	iov[iovnum].iov_base = buf2;
	iov[iovnum].iov_len = BUF_SIZE;
	iovnum++;

	count = 0;
	
	while(count < 3 * BUF_SIZE)
		count += readv(s, iov, iovnum);

#endif

#if 0
	message.msg_name = NULL;
	message.msg_iov = iov;
	message.msg_iovlen = iovnum;
	message.msg_accrights = NULL;

	count = 0;

	while(count < 3 * BUF_SIZE)
		count += recvmsg(s, message, 0);
#endif

	send(s, buffer, BUF_SIZE, 0);
}
