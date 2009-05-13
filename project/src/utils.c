/**
 * @file   utils.c
 *
 * @date   2009-05-13
 * @author Jean-Lou Dupont
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "utils.h"


void random_sleep(int max_usecs) {

	unsigned int t;

	t = 1 + (int) (max_usecs * (rand() / (RAND_MAX + 1.0)));

	usleep( t );
}//


int random_sleep_period(struct timeval *start, struct timeval *current, int max_usecs_interval, int max_usecs_total) {

	// if the malloc fails here, there are much bigger problems anyways!
	if (NULL==start) {
		start = (struct timeval *) malloc( sizeof(struct timeval) );
		gettimeofday( start, NULL );

		if (NULL==current) {
			current = (struct timeval *) malloc( sizeof(struct timeval) );
		}
	}

	gettimeofday( current, NULL );

	// should we even sleep? ie. is the period over?
	suseconds_t diff_usecs = current->tv_usec - start->tv_usec;
	if (diff_usecs > max_usecs_total)
		return 1;

	random_sleep( max_usecs_interval );

	return 0;
}//
