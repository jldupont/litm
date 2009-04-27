/*
 * switch.c
 *
 *  Created on: 2009-04-25
 *      Author: Jean-Lou Dupont
 */
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>

#include "litm.h"
#include "switch.h"
#include "queue.h"

// Input Queue
queue *_switch_queue;

// Switch Thread
pthread_t *_switchThread = NULL;

// Subscriptions to busses
litm_bus_subscription_map _subscribers[LITM_BUSSES_MAX];


// PRIVATE
// -------
void *switch_thread_function(void *params);


	int
switch_init(void) {

	if (NULL== _switchThread) {
		pthread_create(_switchThread, NULL, &switch_thread_function, NULL);

		_switch_queue = queue_create();
	}


}//


	void *
switch_thread_function(void *params) {



}//

	int
switch_send() {


}//
