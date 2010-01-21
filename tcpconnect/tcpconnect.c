#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <netinet/in.h>

static void
usage()
{
    fprintf(stderr,
	    "Usage: tcpconnect [<option>] <remote host> <remote port>\n");
}

static struct sockaddr_in *
get_sockaddr_in(const char		*host,
		const int		port)
{
	struct sockaddr_in *addr = NULL;
	unsigned long host_addr;
	

	addr = (struct sockaddr_in *) malloc(sizeof(*addr));

	if (addr == NULL)
		goto error_return;

	bzero(addr, sizeof(*addr));

	addr->sin_port = htons(port);
	addr->sin_family = AF_INET;

	if (host == NULL) {

		addr->sin_addr.s_addr = INADDR_ANY;

		return addr;

	}

	/*
	 * Check for dot address ("xx.xx.xx.xx")
	 */
	host_addr = inet_addr(host);

	if (host_addr != -1) {

		bcopy((char *) &host_addr,
			  (char *) &(addr->sin_addr),
			  sizeof(host_addr));

	} else {
		/*
		 * Do a lookup on hostname.
		 */
		struct hostent *host_info;

		if ((host_info = gethostbyname(host)) == NULL)
			goto error_return;

		addr->sin_family = host_info->h_addrtype;

		bcopy(host_info->h_addr,
			  (char *) &(addr->sin_addr),
			  host_info->h_length);
	}

	return addr;

error_return:
	if (addr)
		free(addr);

	return NULL;
}


int
main(int		argc,
     char		*argv[])
{
    int			local_port = 0;
    int			remote_port = 0;
    char		*local_addr_string = NULL;
    char		*remote_addr_string = NULL;
    struct sockaddr_in	*remote_address = NULL;
    struct sockaddr_in	*local_address = NULL;
    int			sock;
    int			reuseaddr_opt = 1;
    
    
    int			option;
    extern char		*optarg;
    extern int		optind;


    while ((option = getopt(argc, argv, "l:L:")) != EOF)
    {
	switch(option)
	{
	  case 'l':
	    local_port = atoi(optarg);
	    break;
	    
	  case 'L':
	    local_addr_string = optarg;
	    break;
	    
	  case '?':
	  default:
	    break;
	}
    }

    if (argc - optind < 2)
    {
	usage();
	exit(1);
    }
    
    remote_addr_string = argv[optind++];
    remote_port = atoi(argv[optind++]);
    
    /* Look up remote and local host IPs */
    remote_address = get_sockaddr_in(remote_addr_string, remote_port);
    
    if (remote_address == NULL)
    {
	fprintf(stderr, "Unknown host \"%s\"\n", remote_addr_string);
	exit(1);
    }
    
    local_address = get_sockaddr_in(local_addr_string, local_port);
    
    if (local_address == NULL)
    {
	fprintf(stderr, "Unknown host \"%s\"\n",
		(local_addr_string == NULL ? "localhost" : local_addr_string));
	exit(1);
    }
    
    /* Make and bind socket on local side */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sock == -1)
    {
	perror("socket()");
	exit(1);
    }

    /* Allow us to reuse the local port */
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
	       &reuseaddr_opt, sizeof(reuseaddr_opt));
	       
    if (bind(sock, local_address, sizeof(*local_address)) == -1)
    {
	perror("bind()");
	exit(1);
    }

    /* Now connect */
    if (connect(sock, remote_address, sizeof(*remote_address)) == -1)
    {
	perror("connect()");
	exit(1);
    }
    
    printf("Successful connect from %s:%d to %s:%d\n",
	   (local_addr_string == NULL ? "localhost" : local_addr_string),
	   local_port, remote_addr_string, remote_port);
    
    exit(0);
}


	
	    
