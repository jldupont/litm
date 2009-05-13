/**
 * @file   utils.h
 *
 * @date   2009-05-13
 * @author Jean-Lou Dupont
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <sys/time.h>

	/**
	 * Sleeps for a random period of time
	 * up to a maximum of ``max_usecs``
	 *
	 * @param max_usecs maximum sleep in microseconds
	 */
	void random_sleep(int max_usecs);


	/**
	 * Random Interval based sleeping
	 *
	 * This function ''sleeps'' for a maximum of ``max_usecs_total``
	 * in intervals of ``max_usecs_interval`` at the time.
	 *
	 * @param start the start time
	 * @param current the current time
	 * @param max_usecs_interval maximum sleep per interval
	 * @param max_usecs_total maximum total sleep
	 *
	 * @return 0 SUCCESS
	 * @return 1 End of Period
	 *
	 */
	int random_sleep_period(struct timeval *start, struct timeval *current, int max_usecs_interval, int max_usecs_total);


#endif /* UTILS */
