/**
***	pipe_exec.c
***
***	$Header: /Users/vwelch/develop/ncsa-cvs/networking/nettest/pipe_exec.c,v 1.1 1995/03/04 21:32:39 vwelch Exp $
***
***
***
***	int pipe_exec(	char	*command,
***			char	**argv,
***			int	*fd_stdin,
***			int	*fd_stdout,
***			int	*fd_stderr)
***			
***	Forks and executes a child process, setting up specified pipes to
***	child. If any of fd_stdin, fd_stdout, fd_stderr are non-NULL they
***	will be filled a descriptor to the stdin/stdout/stderr of the child
***	process.
***
***	The final argument in argv must be followed by a NULL pointer.
***
***	pipe_exec() returns the pid of the child or -1 on error.
***
**/

#include <stdio.h>


/**	Close any open pipes
**/
#define	CLOSE_ALL_PIPES()	\
    { if (fd_stdin != NULL) \
	{ close(in_filedes[0]); close(in_filedes[1]); } \
      if (fd_stdout != NULL) \
	{ close(out_filedes[0]); close(out_filedes[1]); } \
      if (fd_stderr != NULL) \
	{ close(err_filedes[0]); close(err_filedes[1]); } \
    }

		

int pipe_exec(name, argv, fd_stdin, fd_stdout, fd_stderr)
	char	*name;
	char	**argv;
	int	*fd_stdin;
	int	*fd_stdout;
	int	*fd_stderr;
{
	int	in_filedes[2], out_filedes[2], err_filedes[2];
	int	pid;

	if (fd_stdin != NULL)
		if (pipe(in_filedes) == -1)
			return -1;

	if (fd_stdout != NULL)
		if (pipe(out_filedes) == -1)
			return -1;

	if (fd_stderr != NULL)
		if (pipe(err_filedes) == -1)
			return -1;

	pid = fork();

	if (pid == -1) {
		CLOSE_ALL_PIPES();
		return -1;
	}
	
	if (pid == 0) {		/** CHILD **/
		
		if (fd_stdin != NULL)
			dup2(in_filedes[0], fileno(stdin));

		if (fd_stdout != NULL)
			dup2(out_filedes[1], fileno(stdout));

		if (fd_stderr != NULL)
			dup2(err_filedes[1], fileno(stderr));

		CLOSE_ALL_PIPES();

		execv(name, argv);

		exit(1);
	}

	/** PARENT **/

	if (fd_stdin != NULL)
		*fd_stdin = dup(in_filedes[1]);

	if (fd_stdout != NULL)
		*fd_stdout = dup(out_filedes[0]);

	if (fd_stderr != NULL)
		*fd_stderr = dup(err_filedes[0]);

	CLOSE_ALL_PIPES();

	return pid;
}
