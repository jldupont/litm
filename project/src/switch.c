/*
 * switch.c
 *
 *  Created on: 2009-04-25
 *      Author: Jean-Lou Dupont
 */
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
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
litm_code __switch_try_sending_to_recipient(	litm_connection *recipient, litm_envelope *env);
litm_code __switch_finalize(litm_envelope *envlp);
litm_code __switch_try_sending_or_requeue(litm_connection *conn, litm_envelope *envlp);


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

	int pending, action;
	litm_code code;
	litm_envelope *e;
	litm_connection *next, *sender, *current;
	litm_bus bus_id;

	//TODO switch shutdown signal check
	while(1) {
		// we block here since if we have no messages
		//  to process, we don't have anything else to do...
		e=(litm_envelope *) queue_get( _switch_queue );

		if (NULL==e) {
			sleep(0.001);
			continue;
		}

		// The envelope contains the sender's connection ptr
		//  as well as the ``current`` connection ptr.

		// When the message is first sent on any bus,
		//  the switch sets the ``current`` connection ptr
		//  to the subscriber's connection ptr; this pointer
		//  is obtained by scanning the ``subscription map``
		//  for the first subscriber to the target bus.

		// BUT FIRST, we need to know if the envelope
		//  was already processed and just waiting to be
		//  sent!

		// message was processed and just sits pending

		if (1==e->routes.pending) {
			current = e->routes.current;
			code = __switch_try_sending_or_requeue( current, e );

			// if the message was sent or requeued,
			//  we are ok but anything else means something
			//  went wrong (duh!) and since we are the owner
			//  of the envelope/message, we will "recover"
			//  by getting rid of the envelope+message.
			//  REASON:  the probable cause of errors
			//           stems from connection changes
			//           and this is not a normal usage pattern.
			switch(code) {

			case LITM_CODE_OK:
			case LITM_CODE_BUSY_OUTPUT_QUEUE:
				break;

			default:
				// TODO log error?
				__switch_finalize(e);
				break;
			}//switch
			continue;  // <====================================
		}//if


		// FROM THIS POINT, the envelope was pending
		//  so we need to figured out where to send it next.
		// NOTE: anything short of LITM_CODE_OK gets the
		//       envelope finalized.  This behavior covers
		//       the normal case where all recipients have
		//       had the chance to glance over the message.

		sender  = e->routes.sender;
		current = e->routes.current;
		bus_id  = e->routes.bus_id;

		code = __switch_get_next_subscriber(	&next,
												sender,
												current,
												bus_id);

		if (LITM_CODE_OK!=code) {
			// TODO log error?
			__switch_finalize(e);
			continue; // <=======================================
		}


		// WE ARE GOOD TO GO


		//adjust envelope
		e->routes.current = next;

		code = __switch_try_sending_or_requeue( next, e );
		switch(code) {
		case LITM_CODE_OK:
		case LITM_CODE_BUSY_OUTPUT_QUEUE:
			break;

		default:
			// TODO log error?
			__switch_finalize(e);
			break;
		}


	}//while


	return NULL;
}//END THREAD


/**
 * A client has finished processing a message: send it
 *  through through the ``input_queue`` and if all intended
 *  recipients have processed it already, it will be finalized.
 *
 */
	litm_code
switch_release(litm_connection *conn, litm_envelope *envlp) {

	if (NULL==conn) {
		return LITM_CODE_ERROR_BAD_CONNECTION;
	}
	if (NULL==envlp) {
		return LITM_CODE_ERROR_INVALID_ENVELOPE;
	}

	// if a message comes this way, it means a client
	// has finished processing it... get rid of the
	// "pending" state if at all present.
	envlp->routes.pending = 0;

	int result = queue_put(_switch_queue, (void *) envlp);
	if (1 != result) {
		__switch_finalize( envlp );
		return LITM_CODE_ERROR_MALLOC;
	}

	return LITM_CODE_OK;
}//

/**
 * Tries sending the envelope along BUT requeue if this is
 *  not possible at this juncture.
 */
	litm_code
__switch_try_sending_or_requeue(litm_connection *conn, litm_envelope *envlp) {

	litm_code result = __switch_try_sending_to_recipient(conn, envlp);

	if (LITM_CODE_BUSY_OUTPUT_QUEUE==result) {
		envlp->routes.pending = 1;
		queue_put( _switch_queue, envlp );
	}

	return result;
}//

/**
 * Try sending the message to the recipient.
 *
 * This function does not block waiting for the recipient.
 */
	litm_code
__switch_try_sending_to_recipient(	litm_connection *recipient,
									litm_envelope *env) {

	int returnCode = 0;

	returnCode = queue_put_nb( recipient->input_queue, (void *) env);

	if (0==returnCode)
		return LITM_CODE_ERROR_OUTPUT_QUEUING;

	if (-1==returnCode)
		return LITM_CODE_BUSY_OUTPUT_QUEUE;

	return LITM_CODE_OK;
}//


	litm_code
switch_add_subscriber(litm_connection *conn, litm_bus bus_id) {

	if (NULL==conn) {
		return LITM_CODE_ERROR_BAD_CONNECTION;
	}

	if (( LITM_BUSSES_MAX < bus_id ) || (0>=bus_id)) {
		return LITM_CODE_ERROR_INVALID_BUS;
	}

	int code = pthread_mutex_trylock( &_subscribers_mutex );
	if (EBUSY==code)
		return LITM_CODE_BUSY;

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

	int code = pthread_mutex_trylock( &_subscribers_mutex );
	if (EBUSY==code)
		return LITM_CODE_BUSY;

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

/**
 * This function just queues up the message in the
 *  switch's input_queue without actually sending it
 *  to recipients.
 *
 *  This function blocks whilst accessing the switch's
 *  ``input_queue`` but no deadlock scenario is possible
 *  since the switch's thread does not block whilst dequeuing.
 *
 */
	litm_code
switch_send(litm_connection *conn, litm_bus bus_id, void *msg,
			void (*cleaner)(void *msg)) {

	if (NULL==conn) {
		return LITM_CODE_ERROR_BAD_CONNECTION;
	}

	if (( LITM_BUSSES_MAX < bus_id ) || (0>=bus_id)) {
		return LITM_CODE_ERROR_INVALID_BUS;
	}

	// Grab an envelope & init
	litm_envelope *e=__litm_pool_get();
	e->cleaner = cleaner;
	e->routes.pending = 0; //FALSE
	e->routes.bus_id = bus_id;
	e->routes.sender = conn;
	e->routes.current = NULL;  // First time sent
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
 * Finalizes a message
 *
 */
	litm_code
__switch_finalize(litm_envelope *envlp) {

	if (NULL==envlp) {
		return LITM_CODE_ERROR_INVALID_ENVELOPE;
	}

	void (*cleaner)(void *msg) = envlp->cleaner;

	if (NULL==cleaner) {
		free( envlp->msg );
	} else {
		(*cleaner)( (void *) envlp->msg );
	}

	__litm_pool_recycle( envlp );

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

	int code = pthread_mutex_trylock( &_subscribers_mutex );
	if (EBUSY==code)
		return LITM_CODE_BUSY;

		_litm_connections_lock();

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
					// maybe the connection was pull-off from our feet
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
		_litm_connections_unlock();
	pthread_mutex_unlock( &_subscribers_mutex );

	return returnCode;
}//

/**
 * Find the next ``non-match`` and non-NULL
 *
 * THIS FUNCTION IS NOT *CONNECTION SAFE*
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
 *
 * THIS FUNCTION IS NOT *CONNECTION SAFE*
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