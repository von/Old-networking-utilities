/*
	timer.c

	Routines used for timing.
*/

#include "timer.h"

#include <stdio.h>



/*
	start_timer()

	Given a pointer to a timeval structure capture the current time.

	Returns 0 normally, -1 if an error occurs.
*/
int start_timer(start)
	struct timeval *start;
{
	return gettimeofday(start, NULL);
}



/*
	time_since()

	Given a starting time from a start_timer() call, fill time_elapsed
	with the time elapsed.

	Returns 0 normally, -1 if an error occurs.
*/
int time_since(start, time_elapsed)
	struct timeval *start, *time_elapsed;
{
	int rc;

	rc = gettimeofday(time_elapsed, NULL);

	if (!rc) {
		time_elapsed->tv_sec -= start->tv_sec;
		time_elapsed->tv_usec -= start->tv_usec;
	}

	return rc;
}



/*
	num_seconds()

	Given a timeval structure, return the number of seconds as a float
	it represents.
*/
float num_seconds(time)
	struct timeval *time;
{
	return (float) time->tv_sec + (float) time->tv_usec / 1000000;
}
