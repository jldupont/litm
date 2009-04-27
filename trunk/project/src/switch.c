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
void *__switch_thread_function(void *params);
litm_code __switch_get_next_subscriber(	litm_connection **result,
										litm_connection *sender,
										litm_connection *current,
										litm_bus bus_id);
int __switch_find_next_non_match(litm_connection *ref, litm_bus bus_id);
int __switch_find_match(litm_connection *ref, litm_bus bus_id);

	int
switch_init(void) {

	if (NULL== _switchThread) {
		pthread_create(_switchThread, NULL, &__switch_thread_function, NULL);

		_switch_queue = queue_create();
	}
}//

/**
 * TODO implement switch shutdown
 */
	int
switch_shutdown(void) {

}//


/**
 * The switch thread dequeues messages from the
 *  ``input queue`` and dispatches them to the
 *  next connection queue.
 */
	void *
__switch_thread_function(void *params) {

	litm_code code;
	litm_envelope *e;
	litm_connection *conn, *sender, *current;
	litm_bus bus_id;

	//TODO switch shutdown signal check
	while(1) {
		// we block here since if we have no messages
		//  to process, we don't have anything else to do...
		e=(litm_envelope *) queue_get( _switch_queue );

		// The envelope contains the sender's connection ptr
		//  as well as the ``current`` connection ptr.

		// When the message is first sent on any bus,
		//  the switch sets the ``current`` connection ptr
		//  to the subscriber's connection ptr; this pointer
		//  is obtained by scanning the ``subscription map``
		//  for the first subscriber to the target bus.

		sender  = e->routes.sender;
		current = e->routes.current;
		bus_id  = e->routes.bus_id;

		code = __switch_find_next_subscriber(	&conn,
												sender,
												current,
												bus_id);


	}//while


	return NULL;
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
	e->routes.bus_id = bus_id;
	e->routes.sender = conn;
	e->routes.current = NULL;
	e->msg = msg;

	/*
	 *  Initial message submission: if something goes
	 *  wrong, we need to get rid of envelope.
	 */
	int result = queue_put(_switch_queue, (void *) e);
	if (1 != result) {
		__litm_pool_recycle( e );
		return LITM_CODE_ERROR_MALLOC;
	}

	return LITM_CODE_OK;
}//

/**
 * Scans the subscription map for the subscriber that
 * follows ``current``.  If ``current`` is NULL (thus
 * indicating the start of the scan), then pull the
 * first subscriber from the subscription to bus ``bus_id``.
 *
 * Furthermore, we are implementing ``split horizon``
 *  forwarding ie. don't send the message back to the
 *  originator.
 *
 * If end of list is reached, return NULL.
 *
 * CASES:
 * ======
 *
 * 1- empty subscription list
 * 2- single subscriber
 *    a) the sender
 *    b) other subscriber
 * 3- more than 1 subscriber
 *
 */
	litm_code
__switch_get_next_subscriber(	litm_connection **result,
								litm_connection *sender,
								litm_connection *current,
								litm_bus bus_id) {

	litm_code returnCode = LITM_CODE_OK; //optimistic

	int index=1;
	int foundNext = 0;
	int foundFirst=0, bailOut=0; //FALSE
	litm_connection *c=NULL;

	// this shouldn't happen...
	if (sender==current) {
		return LITM_CODE_ERROR_SWITCH_SENDER_EQUAL_CURRENT;
	}

	pthread_mutex_lock( &_subscribers_mutex );

		// FIND FIRST {{
		if (NULL==current) {

			int searchResultIndex = 0;
			searchResultIndex = __switch_find_next_non_match(sender, bus_id);

			if (0!=searchResultIndex) {
				// non-null && non-sender
				foundFirst = searchResultIndex;
			} else {
				bailOut = 1;
			}
		}//if
		// }}

		if (1==foundFirst) {
			// we found the first recipient
			// bail out with the result!
			*result = _subscribers[bus_id][foundFirst];
			bailOut=1;
		}

		// at this point, we are looking for the recipient
		// after ``current`` but that isn't ``sender`` nor NULL
		if (1!=bailOut) {
			int foundMatch = __switch_find_match(current, bus_id);

			if (0==foundMatch) {
				// can't find ``current``...
				returnCode = LITM_CODE_ERROR_SWITCH_NO_CURRENT;
				bailOut = 1;
			} else {
				// we found ``current``...
				// need the following subscriber
				// without forgetting about split-horizon!
				for ( index=foundMatch+1; index<LITM_CONNECTION_MAX; index++) {
					c = _subscribers[bus_id][index];
					if ((NULL!=c) && (sender!=c)) {
						foundNext = index;
						*result = c;
						returnCode = LITM_CODE_OK;
						break;
					}
				}//for

				if (0==foundNext)
					returnCode = LITM_CODE_ERROR_SWITCH_NEXT_NOT_FOUND;

			}//foundMatch
		}//bailOut

	pthread_mutex_unlock( &_subscribers_mutex );

	return returnCode;
}//

/**
 * Find the next ``non-match`` and non-NULL
 */
	int
__switch_find_next_non_match(litm_connection *ref, litm_bus bus_id) {

	int index, result = 0;

	for (index=1; index<LITM_CONNECTION_MAX; index++) {
		litm_connection *current;

		current=_subscribers[bus_id][index];
		if ((current!=ref) && (current!=NULL)) {
			result = index;
			break;
		}
	}

	return result;
}//

/**
 * Find ``match``
 */
	int
__switch_find_match(litm_connection *ref, litm_bus bus_id) {

	int index, result = 0;

	for (index=1; index<LITM_CONNECTION_MAX; index++) {
		litm_connection *current;

		current=_subscribers[bus_id][index];
		if ((current==ref) && (current!=NULL)) {
			result = index;
			break;
		}
	}

	return result;
}//
