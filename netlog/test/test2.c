#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>

#include "netlog.h"

#define BUF_SIZE        10000


main()
{
	int s, iovnum, count;
	struct sockaddr_in addr;
	struct iovec iov[3];
	struct msghdr message;

	char buffer[BUF_SIZE], buf1[BUF_SIZE], buf2[BUF_SIZE];


	s = socket(PF_INET, SOCK_STREAM, 0);

	addr.sin_family = AF_INET;
#ifdef INADDR_LOOPBACK
	addr.sin_addr.s_addr = INADDR_LOOPBACK;
#else
	addr.sin_addr.s_addr = 0x7F000001;
#endif
	addr.sin_port = htons(6767);

	if (connect(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
	  perror("connect()");
	  exit(1);
	}

	count = 0;

	while(count < BUF_SIZE)
		count += read(s, buffer, BUF_SIZE - count);

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
 
	writev(s, iov, iovnum);
#endif

#if 0
	message.msg_name = NULL;
	message.msg_namelen = 0;
	message.msg_iov = iov;
	message.msg_iovlen = iovnum;
	message.msg_accrights = NULL;

	sendmsg(s, &message, 0);
#endif

	count = 0;
	
	while(count < BUF_SIZE)
		count += recv(s, buffer, BUF_SIZE - count, 0);
}
