/*
  loggerd.c

  Remote logging daemon for the netlog library.
*/

#include "logger.h"

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>



main()
{
	int accept_sock;
	int client_sock;

	struct sockaddr_in addr;

	int reuseaddr = 1;

	
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = DEFAULT_SOCKET_PORT;

	accept_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (accept_sock < 0) {
		perror("socket()");
		exit(1);
	}

	setsockopt(accept_sock, SOL_SOCKET, SO_REUSEADDR,
		   &reuseaddr, sizeof(reuseaddr));

	if (bind(accept_sock, &addr, sizeof(addr)) < 0) {
		perror("bind()");
		exit(1);
	}

	if (listen(accept_sock, 5) < 0) {	/* 5 is arbitrarily choosen */
		perror("listen()");
		exit(1);
	}


	while (1) {
		client_sock = accept(accept_sock, NULL, NULL);

		if (client_sock == -1) {
			perror("accept()");
			continue;
		}

		switch(fork()) {
		case 0:		/* Child */
			close(accept_sock);
			handle_client(client_sock);
			exit(0);

		case -1:	/* Error */
			perror("fork()");
			continue;

		default:		/* Parent */
			close(client_sock);
			continue;
		}
		/* Not reached */
	}
	/* Not Reached */
}
			

/*
  handle_client()

  Handle a connection from a client from start to finish.
*/

handle_client(sock)		
        int sock;
{
	char filename[BUFFER_SIZE];

	FILE *log_file;
	FILE *client_stream;
	
	int character;


	/*
	  First we read the name of the file from the client.
	  */
	if (read(sock, filename, BUFFER_SIZE) == -1) {
		return -1;
	}

	/*
	  Security stuff-
	  -Check and remove everything up to last '/' so that we don't create
	   any files outside the current directory.
	  */
	if (strchr(filename, '/')) {
		char tmp_buffer[BUFFER_SIZE];
		char *slash_ptr;
		
		slash_ptr = strrchr(filename, '/') + 1;
		
		strcpy(tmp_buffer, slash_ptr);
		strcpy(filename, tmp_buffer);
	}

	log_file = fopen(filename, "a");

	if (log_file == NULL) {
		fprintf(stderr, "Couldn't open log file \"%s\".\n",
			filename);
		return -1;
	}

	client_stream = fdopen(sock, "r");

	if (client_stream == NULL) {
		perror("fdopen()");
		return -1;
	}

	while((character = fgetc(client_stream)) != EOF)
		fputc(character, log_file);

	fclose(log_file);
	fclose(client_stream);

	return 0;
}
	
	
	
			
