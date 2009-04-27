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
#include "pool.h"
#include "connection.h"


// Input Queue
queue *_switch_queue;

// Switch Thread
pthread_t *_switchThread = NULL;

// Subscriptions to busses
// -----------------------
pthread_mutex_t _subscribers_mutex = PTHREAD_MUTEX_INITIALIZER;
litm_connection *_subscribers[LITM_BUSSES_MAX][LITM_CONNECTION_MAX];


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

/**
 * The switch thread dequeues messages from the
 *  ``input queue`` and dispatches them to the
 *  next connection queue.
 */
	void *
switch_thread_function(void *params) {

	litm_envelope *e;


	// we block here since if we have no messages
	//  to process, we don't have anything else to do...
	e=(litm_envelope *) queue_get( _switch_queue );


}//


	litm_code
switch_add_subscriber(litm_connection *conn, litm_bus bus_id) {

	if (NULL==conn) {
		return LITM_CODE_ERROR_BAD_CONNECTION;
	}

	if (( LITM_BUSSES_MAX < bus_id ) || (0>=bus_id)) {
		return LITM_CODE_ERROR_INVALID_BUS;
	}

	pthread_mutex_lock( &_subscribers_mutex );

	int result=LITM_CODE_ERROR_INVALID_BUS;
	int index;
	for (index=1; index<LITM_CONNECTION_MAX; index++) {
		if (NULL==_subscribers[bus_id][index]) {
			_subscribers[bus_id][index] = conn;
			result = LITM_CODE_OK;
			break;
		}
	}

	pthread_mutex_unlock( &_subscribers_mutex );

	return result;
}//

	litm_code
switch_remove_subscriber(litm_connection *conn, litm_bus bus_id) {

	if (NULL==conn) {
		return LITM_CODE_ERROR_BAD_CONNECTION;
	}

	if (( LITM_BUSSES_MAX < bus_id ) || (0>=bus_id)) {
		return LITM_CODE_ERROR_INVALID_BUS;
	}

	pthread_mutex_lock( &_subscribers_mutex );

	int result=LITM_CODE_ERROR_INVALID_BUS;
	int index;
	for (index=1; index<LITM_CONNECTION_MAX; index++) {
		if (conn==_subscribers[bus_id][index]) {
			_subscribers[bus_id][index] = NULL;
			result = LITM_CODE_OK;
			break;
		}
	}

	pthread_mutex_unlock( &_subscribers_mutex );

	return result;
}//


	litm_code
switch_send(litm_connection *conn, litm_bus bus_id, void *msg) {

	if (NULL==conn) {
		return LITM_CODE_ERROR_BAD_CONNECTION;
	}

	if (( LITM_BUSSES_MAX < bus_id ) || (0>=bus_id)) {
		return LITM_CODE_ERROR_INVALID_BUS;
	}

	// Grab an envelope & init
	litm_envelope *e=__litm_pool_get();
	e->routes.sender = conn;
	e->routes.current = NULL;
	e->msg = msg;

	/*
	 *  Initial message submission: if something goes
	 *  wrong, we need to get rid of envelope.
	 */
	int result = queue->put(_switch_queue, (void *) e);
	if (1 != result) {
		__litm_pool_recycle( e );
		return LITM_CODE_ERROR_MALLOC;
	}

	return LITM_CODE_OK;
}//
