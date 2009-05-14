/**
 * @file   switch.c
 *
 * @date   2009-04-25
 * @author Jean-Lou Dupont
 *
 * \section Overview
 *
 * 			This source implements the *switch* function whereby *envelopes*
 *			are presented, turn-wise, to each recipient(s) subscribing
 *			to a particular	*bus*.
 *
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
#include "logger.h"


#define LITM_SHUTDOWN_FLAG_TRUE  1
#define LITM_SHUTDOWN_FLAG_FALSE 0

// Input Queue
queue *_switch_queue;

// Switch Thread
pthread_t _switchThread;
int _switchThread_status=0; //not created

// Subscriptions to busses
// -----------------------
pthread_mutex_t _subscribers_mutex = PTHREAD_MUTEX_INITIALIZER;
litm_connection *_subscribers[LITM_BUSSES_MAX][LITM_CONNECTION_MAX+1]; // index 0 is not used

// "Signal"
volatile int __switch_signal_shutdown = 0; // FALSE


// PRIVATE
// -------
void *__switch_thread_function(void *params);
litm_code __switch_get_next_subscriber(	litm_connection **result,
										int *result_index,
										litm_connection *sender,
										int current,
										litm_bus bus_id);

int __switch_find_match(litm_connection *sender, int ref, litm_bus bus_id);
litm_code __switch_try_sending_to_recipient(	litm_connection *recipient, litm_envelope *env);
litm_code __switch_finalize(litm_envelope *envlp);
litm_code __switch_try_sending_or_requeue(litm_connection *conn, litm_envelope *envlp);
void __switch_init_tables(void);
void __switch_handle_pending(litm_envelope *e);
litm_code __switch_safe_send( litm_connection *conn, litm_bus bus_id, void *msg, void (*cleaner)(void *msg), int shutdown_flag );

litm_connection *__switch_index_to_connection( int bus_id, int index );

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


	int
switch_init(void) {

	if (0== _switchThread_status) {


		// init queue *before* launching thread!
		_switch_queue = queue_create(-1);
		__switch_init_tables();

		DEBUG_LOG(LOG_INFO, "switch_init: creating switch thread & queue, q[%x] ", _switch_queue);

		pthread_create(&_switchThread, NULL, &__switch_thread_function, NULL);
		_switchThread_status = 1; //created
	}
}//

	void
__switch_init_tables(void) {
	int b, c;

	for (b=0; b<LITM_BUSSES_MAX; b++)
		for (c=0;c<=LITM_CONNECTION_MAX;c++)
			_subscribers[b][c] = NULL;
}

	void
switch_shutdown(void) {

	__switch_signal_shutdown = 1;

	pthread_join( _switchThread, (void **)NULL );
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
	litm_connection *next, *sender, *current_conn;
	litm_bus bus_id;

	char *err_msg;
	int pending, action, sd_flag=LITM_SHUTDOWN_FLAG_FALSE, shutdown_flag=LITM_SHUTDOWN_FLAG_FALSE;
	int current;
	static char *thisMsg = "__switch_thread_function: conn[%x] code[%s]";

	DEBUG_LOG(LOG_INFO, "__switch_thread_function: starting");

	while(1) {

		//shutdown signaled?
		if ((1==__switch_signal_shutdown) || (LITM_SHUTDOWN_FLAG_TRUE==shutdown_flag)) {
			break;
		}

		// we block here since if we have no messages
		//  to process, we don't have anything else to do...
		e=(litm_envelope *) queue_get_nb( _switch_queue );

		if (NULL==e) {

			usleep(50*1000);
			continue;  // <===============================================
		}



		//DEBUG_LOG(LOG_INFO, "__switch_thread_function: GOT ENVELOPE");

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

		if (1==(e->routes).pending) {

			__switch_handle_pending(e);
			continue; // <===================================================

		}

		// FROM THIS POINT, the envelope was pending
		//  so we need to figured out where to send it next.
		// NOTE: anything short of LITM_CODE_OK gets the
		//       envelope finalized.  This behavior covers
		//       the normal case where all recipients have
		//       had the chance to glance over the message.

		sender       = (e->routes).sender;
		current      = (e->routes).current;
		current_conn = (e->routes).current_conn;
		bus_id       = (e->routes).bus_id;
		sd_flag      =  e->shutdown_flag;

		//int id = litm_connection_get_id( sender );
		//DEBUG_LOG(LOG_INFO, "__switch_thread_function: bus[%u] sender[%x] current[%x] envelope[%x] sd[%i] conn_id[%i]", bus_id, sender, current, e, sd_flag, id);

		int next_index;
		code = __switch_get_next_subscriber(	&next,
												&next_index,
												sender,
												current,
												bus_id);

		switch(code) {
		case LITM_CODE_OK:
			break;

		case LITM_CODE_ERROR_END_OF_SUBSCRIBERS_LIST:
			__switch_finalize(e);
			continue;


		default:
			err_msg = litm_translate_code(code);
			int sender_id  = sender->id;

			DEBUG_LOG(LOG_DEBUG, "__switch_thread_function: code[%s] sender[%x][%i] current[%x][%i] sd[%i] finalize", err_msg, sender, sender_id, current_conn, current, sd_flag);
			__switch_finalize(e);

			// last subscriber... and shutdown?
			if (LITM_SHUTDOWN_FLAG_TRUE==sd_flag) {
				DEBUG_LOG(LOG_DEBUG, "__switch_thread_function: RECEIVED SHUTDOWN FLAG !!!!!!!!!!!!");
				shutdown_flag = LITM_SHUTDOWN_FLAG_TRUE;
			}

			continue; // <=======================================
		}


		// WE ARE GOOD TO GO


		//adjust envelope
		(e->routes).current      = next_index;
		(e->routes).current_conn = next;

		code = __switch_try_sending_or_requeue( next, e );
		err_msg = litm_translate_code(code);

		switch(code) {
		case LITM_CODE_OK:
			break;

		case LITM_CODE_ERROR_END_OF_SUBSCRIBERS_LIST:
			__switch_finalize(e);
			break;

		case LITM_CODE_BUSY_OUTPUT_QUEUE:
		case LITM_CODE_BUSY_CONNECTIONS:
			DEBUG_LOG(LOG_DEBUG, thisMsg, next, err_msg);
			break;

			/*
			 * Attempting to send to an ``inactive``
			 *  connection
			 */
		case LITM_CODE_ERROR_CONNECTION_NOT_ACTIVE:
			DEBUG_LOG(LOG_DEBUG, thisMsg, next, err_msg);

			// remove connection and try sending back
			// to head of switch_queue: maybe other
			// recipients are still to be serviced.

			// Putting NULL in the field ``current``
			// will restart the process of finding
			// a ``next`` recipient for the envelope.
			(e->routes).pending = 0;
			(e->routes).current = -1;
			queue_put( _switch_queue, e );
			break;

		default:
			DEBUG_LOG(LOG_DEBUG, thisMsg, next, err_msg);
			__switch_finalize(e);
			break;
		}


	}//while

	DEBUG_LOG(LOG_INFO, "__switch_thread_function: ENDING");

	return NULL;
}//END THREAD

/**
 * Handle ``pending`` requests to sent
 */
	void
__switch_handle_pending(litm_envelope *e) {
	DEBUG_LOG(LOG_INFO, "__switch_handle_pending: BEGIN");

	litm_connection *conn;
	litm_code code;

	conn = (e->routes).current_conn;
	code = __switch_try_sending_or_requeue( conn, e );

	// if the message was sent or requeued,
	//  we are ok but anything else means something
	//  went wrong (duh!) and since we are the owner
	//  of the envelope/message, we will "recover"
	//  by getting rid of the envelope+message.
	//  REASON:  the probable cause of errors
	//           stems from connection changes
	//           and this is not a normal usage pattern.
	static char *thisMsg = "__switch_handle_pending: conn[%x] code[%s]";
	char *translated_code = litm_translate_code(code);

	switch(code) {

	case LITM_CODE_OK:
		(e->routes).pending = 0;
		DEBUG_LOG(LOG_DEBUG, ">>> PENDING DELIVERED conn[%x][%i] envlp[%x]", conn, conn->id, e);
		break;

	case LITM_CODE_BUSY_OUTPUT_QUEUE:
	case LITM_CODE_BUSY_CONNECTIONS:
	case LITM_CODE_ERROR_BAD_CONNECTION:
		DEBUG_LOG(LOG_DEBUG, thisMsg, conn, translated_code);
		break;

	default:
		DEBUG_LOG(LOG_DEBUG, thisMsg, conn, translated_code);
		__switch_finalize(e);
		break;
	}//switch

}//

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
	(envlp->routes).pending = 0;
	envlp->released_count ++;
	conn->released++;

	//{
	DEBUG_LOG(LOG_DEBUG, "~~~ RELEASE conn[%x][%i] released[%i] envelope[%x] sender[%x][%i]", conn, conn->id, conn->released, envlp, (envlp->routes).sender, (envlp->routes).sender->id);
	//}

	int result = queue_put(_switch_queue, (void *) envlp);
	if (1 != result) {
		DEBUG_LOG(LOG_DEBUG, "switch_release: RE-QUEUE ERROR, conn[%x] envelope[%x]", conn, envlp );
		__switch_finalize( envlp );
		return LITM_CODE_ERROR_MALLOC;
	}

	return LITM_CODE_OK;
}//

/**
 * Tries sending the envelope along BUT requeue if this is
 *  not possible at this juncture.
 *
 *  The call to __switch_try_sending_to_recipient is connection safe.
 *
 */
	litm_code
__switch_try_sending_or_requeue(litm_connection *conn, litm_envelope *envlp) {

	if (NULL==envlp)
		return LITM_CODE_ERROR_INVALID_ENVELOPE;

	litm_code result = __switch_try_sending_to_recipient(conn, envlp);

	// requeue in switch
	if (LITM_CODE_BUSY_OUTPUT_QUEUE==result) {

		envlp->requeued++;
		(envlp->routes).pending = 1;
		queue_put( _switch_queue, envlp );

	}

	// a connection dropped out... no big deal,
	// just requeue as it was the first go
	if (LITM_CODE_ERROR_CONNECTION_NOT_ACTIVE==result) {

		envlp->requeued++;
		(envlp->routes).pending = 0;
		queue_put( _switch_queue, envlp );

	}

	return result;
}//

/**
 * Try sending the message to the recipient.
 *
 * This function does not block waiting for the recipient.
 * This function should only be called by ''__switch_try_sending_or_requeue''.
 */
	litm_code
__switch_try_sending_to_recipient(	litm_connection *conn,
									litm_envelope *env) {

	if (NULL==conn) {
		DEBUG_LOG(LOG_ERR, "__switch_try_sending_to_recipient: NULL connection" );
		return LITM_CODE_ERROR_BAD_CONNECTION;
	}

	if (NULL==env) {
		DEBUG_LOG(LOG_ERR, "__switch_try_sending_to_recipient: NULL envelope" );
		return LITM_CODE_ERROR_INVALID_ENVELOPE;
	}


	litm_code returnCode = LITM_CODE_OK;
	int code = 0;

	//DEBUG_LOG(LOG_ERR, "__switch_try_sending_to_recipient: START conn[%x]",conn );
	//_litm_connections_lock();

		//code = _litm_connection_validate_safe(conn);
		//if (1==code) {

			//_litm_connection_lock(conn);
				//litm_connection_status status;
				//status = _litm_connection_get_status( conn );
				//if (LITM_CONNECTION_STATUS_ACTIVE!=status) {
					//code = 2;
				//} else {
					code = queue_put_nb( conn->input_queue, (void *) env);
				//}
			//_litm_connection_unlock(conn);

		//} else {
			//code = 2;
		//}

	//_litm_connections_unlock();
	//DEBUG_LOG(LOG_ERR, "__switch_try_sending_to_recipient: STOP  conn[%x]", conn );

	switch(code) {
	case 0: //error
		returnCode = LITM_CODE_ERROR_OUTPUT_QUEUING;
		break;
	case 1: //success
		DEBUG_LOG(LOG_DEBUG, "__switch_try_sending_to_recipient: DELIVERED to conn[%x][%i] envlp[%x] sender[%x][%i]", conn, conn->id, env, (env->routes).sender, (env->routes).sender->id);
		env->delivery_count++;
		returnCode = LITM_CODE_OK;
		break;
	case -1: //busy
		returnCode = LITM_CODE_BUSY_OUTPUT_QUEUE;
		DEBUG_LOG(LOG_ERR, ">>> BUSY conn[%x][%i]", conn, conn->id );
		break;
	case 2:
		// TODO this definitely needs fixing!
		returnCode = LITM_CODE_ERROR_CONNECTION_NOT_ACTIVE;
		(env->routes).current = -1;
		(env->routes).current_conn = NULL;
		break;
	}

	return returnCode;
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

		code =_litm_connections_trylock();
		if (EBUSY==code) {
			pthread_mutex_unlock( &_subscribers_mutex );
			return LITM_CODE_BUSY;
		}

			int result=LITM_CODE_ERROR_BUS_FULL;
			int index;
			for (index=1; index<=LITM_CONNECTION_MAX; index++) {
				if (NULL==_subscribers[bus_id][index]) {
					_subscribers[bus_id][index] = conn;
					result = LITM_CODE_OK;
					//DEBUG_LOG(LOG_DEBUG,"switch_add_subscriber: conn[%x] index[%i]", conn, index);
					break;
				}
			}

		_litm_connections_unlock();

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

		code =_litm_connections_trylock();
		if (EBUSY==code) {
			pthread_mutex_unlock( &_subscribers_mutex );
			return LITM_CODE_BUSY;
		}

			int result=LITM_CODE_ERROR_SUBSCRIPTION_NOT_FOUND;
			int index;
			for (index=1; index<=LITM_CONNECTION_MAX; index++) {
				if (conn==_subscribers[bus_id][index]) {
					_subscribers[bus_id][index] = NULL;
					result = LITM_CODE_OK;
					break;
				}
			}

		_litm_connections_unlock();

	pthread_mutex_unlock( &_subscribers_mutex );

	return result;
}//


/**
 * @see switch_send
 *
 */
	litm_code
switch_send_shutdown(litm_connection *conn, litm_bus bus_id, void *msg, void (*cleaner)(void *msg)) {

	if (NULL==conn) {
		return LITM_CODE_ERROR_BAD_CONNECTION;
	}

	if (( LITM_BUSSES_MAX < bus_id ) || (0>=bus_id)) {
		return LITM_CODE_ERROR_INVALID_BUS;
	}

	return __switch_safe_send( conn, bus_id, msg, cleaner, LITM_SHUTDOWN_FLAG_TRUE);
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

	return __switch_safe_send( conn, bus_id, msg, cleaner, LITM_SHUTDOWN_FLAG_FALSE);
}//


	litm_code
__switch_safe_send( litm_connection *sender,
					litm_bus bus_id,
					void *msg,
					void (*cleaner)(void *msg),
					int shutdown_flag ) {

	DEBUG_LOG(LOG_DEBUG, "__SWITCH_SAFE_SEND: sender[%x][%i] bus[%i] sent[%i]", sender, sender->id, bus_id, sender->sent);
	litm_envelope *e=__litm_pool_get();
	e->cleaner = cleaner;
	(e->routes).pending = 0; //FALSE
	(e->routes).bus_id = bus_id;
	(e->routes).sender = sender;
	(e->routes).current = -1;  // First time sent
	(e->routes).current_conn = NULL;  // First time sent
	e->msg = msg;
	e->delivery_count = 0;
	e->released_count = 0;
	e->shutdown_flag = shutdown_flag;
	e->requeued = 0;

	/*
	 *  Initial message submission: if something goes
	 *  wrong, we need to get rid of envelope.
	 */
	int result = queue_put(_switch_queue, (void *) e);
	if (1 != result) {
		__litm_pool_recycle( e );
		return LITM_CODE_ERROR_MALLOC;
	} else {
		sender->sent++;
	}

	return LITM_CODE_OK;
}//

/**
 * Finalizes a message contained
 * in an envelope & recycles the
 * envelope
 *
 */
	litm_code
__switch_finalize(litm_envelope *envlp) {

	if (NULL==envlp) {
		return LITM_CODE_ERROR_INVALID_ENVELOPE;
	}

	DEBUG_LOG(LOG_DEBUG, "__switch_finalize: envlp[%x] rel[%i] del[%i]", envlp, envlp->released_count, envlp->delivery_count );

	void (*cleaner)(void *msg) = envlp->cleaner;

	if (NULL==cleaner) {
		DEBUG_LOG(LOG_DEBUG, "__switch_finalize: envelope[%x] freeing", envlp );

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
								int *result_index,
								litm_connection *sender,
								int  current,
								litm_bus bus_id) {

	litm_code returnCode = LITM_CODE_OK; //optimistic

	*result = NULL; //precaution
	int ref;


	// FIND FIRST {{
	if (-1==current) {
		ref = 0;
	} else {
		ref = current;
	}
	// }}

	// at this point, we are looking for the recipient
	// after ``current`` but that isn't ``sender`` nor NULL, end-of-subscribers

	int foundMatch = __switch_find_match(sender, ref, bus_id);

	if (0==foundMatch) {
		*result = NULL;
		*result_index = -1;
		returnCode = LITM_CODE_ERROR_END_OF_SUBSCRIBERS_LIST;
	} else {
		// we found ``current``...
		// need the following subscriber
		// without forgetting about split-horizon!
		*result = _subscribers[bus_id][foundMatch];
		*result_index = foundMatch;
		returnCode = LITM_CODE_OK;
	}//foundMatch


	//DEBUG_LOG(LOG_DEBUG, "__switch_get_next_subscriber: END");

	return returnCode;
}//

/**
 * Find ``match``
 *
 * THIS FUNCTION IS NOT *CONNECTION SAFE* and
 *  thus must be called whilst holding the _connections lock.
 *
 */
	int
__switch_find_match(litm_connection *sender, int ref, litm_bus bus_id) {

	//DEBUG_LOG(LOG_DEBUG, "__switch_find_match: BEGIN");

	int index, result = 0;
	litm_connection *sub;

	for (index=ref+1; index<=LITM_CONNECTION_MAX; index++) {

		sub=_subscribers[bus_id][index];
		if ((sub!=sender) && (sub!=NULL)) {
			result = index;
			break;
		}
	}

	DEBUG_LOG(LOG_DEBUG, "### MATCH: sender[%x][%i] ref[%i] bus[%u] result[%u]", sender, sender->id, ref, bus_id, result);

	return result;
}//

	litm_connection *
__switch_index_to_connection( int bus_id, int index ) {

	return _subscribers[bus_id][index];
}
