/*
 *	get_load.c
 *
 *	Call uptime and return system load.
 *
 *
 *	Assumes uptime output format of:
 *	 11:39am  up 19:34,  4 users,  load average: 0.44, 0.38, 0.08
 *
 *	$Id: get_load.c,v 1.2 1995/03/23 02:15:27 vwelch Exp $
 */

#include <stdio.h>
#include <string.h>
#include <sys/wait.h>


#ifdef sgi
static char	*uptime_cmd = "/usr/bsd/uptime";
#else
static char	*uptime_cmd = "/usr/ucb/uptime";
#endif

static char	*uptime_args[] = { "uptime", NULL };


#define BUFFER_SIZE	256



float
get_load()
{
  int		uptime_out;
  FILE		*uptime_stream;
  char		buffer[BUFFER_SIZE];
  char		*average_string;
  float		load;

  if (pipe_exec(uptime_cmd, uptime_args, NULL, &uptime_out, NULL) < 0)
    return (float) -1;

  uptime_stream = fdopen(uptime_out, "r");

  if (uptime_stream == NULL) {
    close(uptime_out);
    return (float) -1;
  }

  while(!feof(uptime_stream))
    fgets(buffer, sizeof(buffer), uptime_stream);

  close(uptime_out);
  fclose(uptime_stream);
  wait(NULL);

  if ((average_string = strrchr(buffer, ':')) == NULL)
    return (float) -1;

  average_string++;		/* Go one past ':' */

  if (sscanf(average_string, "%f", &load) != 1)
    load = -1;

  return load;
}
