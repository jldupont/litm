/**
 * @file litm.c
 *
 * @date   2009-04-24
 * @author Jean-Lou Dupont
 *
 * This module is the principal interface to LITM.
 * For more information as to the API of this module,
 * please consult litm.h .
 *
 */
#include <errno.h>
#include <stdlib.h>

#include "litm.h"
#include "connection.h"
#include "queue.h"
#include "pool.h"
#include "logger.h"
#include "utils.h"

char *LITM_CODE_MESSAGES[] = {
		"LITM_CODE_OK",
		"LITM_CODE_NO_MESSAGE",
		"LITM_CODE_BUSY",
		"LITM_CODE_ERROR_MALLOC",
		"LITM_CODE_ERROR_CONNECTION_OPEN",
		"LITM_CODE_ERROR_BAD_CONNECTION",
		"LITM_CODE_ERROR_NO_MORE_CONNECTIONS",
		"LITM_CODE_ERROR_INVALID_BUS",
		"LITM_CODE_ERROR_SWITCH_SENDER_EQUAL_CURRENT",
		"LITM_CODE_ERROR_SWITCH_NO_CURRENT",
		"LITM_CODE_ERROR_SWITCH_NEXT_NOT_FOUND",
		"LITM_CODE_ERROR_OUTPUT_QUEUING",
		"LITM_CODE_BUSY_OUTPUT_QUEUE",
		"LITM_CODE_ERROR_INVALID_ENVELOPE",
		"LITM_CODE_ERROR_SUBSCRIPTION_NOT_FOUND",
		"LITM_CODE_ERROR_BUS_FULL",
		"LITM_CODE_BUSY_CONNECTIONS",
		"LITM_CODE_ERROR_CONNECTION_NOT_ACTIVE",
		"LITM_CODE_ERROR_END_OF_SUBSCRIBERS_LIST",
		"LITM_CODE_ERROR_NO_SUBSCRIBERS",
		"LITM_CODE_ERROR_CONNECTION_ERROR",
		"LITM_CODE_ERROR_SUBSCRIPTION_ERROR",
		"LITM_CODE_ERROR_SEND_ERROR",
		"LITM_CODE_ERROR_RECEIVE_WAIT"
};

// PRIVATE
int __litm_compute_timeout(int timeout);
// =====================================


	litm_code
litm_connect(litm_connection **conn) {

	return litm_connect_ex( conn, 0);
}//

	litm_code
litm_connect_ex(litm_connection **conn, int id) {

	switch_init();

	litm_code code = litm_connection_open_ex(conn, id);
	if (LITM_CODE_OK!=code) {
		*conn = NULL;
		return code;
	}

	return LITM_CODE_OK;
}

	litm_code
litm_connect_ex_wait(litm_connection **conn, int id, int timeout) {

	litm_code code;
	struct timeval *start, *current;
	int computed_timeout = __litm_compute_timeout( timeout );

	while(1) {

		code = litm_connect_ex(conn, id);
		if (LITM_CODE_OK==code)
			break;

		// an error we can't handle here?
		if (LITM_CODE_BUSY!=code) {
			break;
		}

		DEBUG_LOG(LOG_DEBUG, "litm_connect_ex_wait: NEED TO RETRY id[%i]", id);

		// at this point, we just back-off an retry
		int result = random_sleep_period(	start,
											current,
											LITM_DEFAULT_MAX_BACKOFF*1000*1000,
											computed_timeout);
		// expired period?
		if (1==result) {
			code = LITM_CODE_ERROR_CONNECTION_ERROR;
			break;
		}
	}

	return code;
}

	litm_code
litm_disconnect(litm_connection *conn) {

	return litm_connection_close(conn);
}//


	litm_code
litm_subscribe(litm_connection *conn, litm_bus bus_id) {

	return switch_add_subscriber( conn, bus_id );
}//


	litm_code
litm_subscribe_wait(litm_connection *conn, litm_bus bus_id, int timeout) {

	litm_code code;
	struct timeval *start, *current;
	int computed_timeout = __litm_compute_timeout( timeout );

	while(1) {

		code = switch_add_subscriber( conn, bus_id );
		if (LITM_CODE_OK==code)
			break;

		// an error we can't handle here?
		if (LITM_CODE_BUSY!=code) {
			break;
		}

		DEBUG_LOG(LOG_DEBUG, "litm_subscribe_wait: NEED TO RETRY conn[%x] bus[%i]", conn, bus_id);

		// at this point, we just back-off an retry
		int result = random_sleep_period(	start,
											current,
											LITM_DEFAULT_MAX_BACKOFF*1000*1000,
											computed_timeout);
		// expired period?
		if (1==result) {
			code = LITM_CODE_ERROR_SUBSCRIPTION_ERROR;
			break;
		}

	}

	return code;
}//

	litm_code
litm_unsubscribe(litm_connection *conn, litm_bus bus_id) {

	return switch_remove_subscriber( conn, bus_id );
}//

	litm_code
litm_send(	litm_connection *conn,
			litm_bus bus_id,
			void *msg,
			void (*cleaner)(void *msg) ) {

	return switch_send(conn, bus_id, msg, cleaner);
}//


	litm_code
litm_send_wait(	litm_connection *conn,
				litm_bus bus_id,
				void *msg,
				void (*cleaner)(void *msg),
				int timeout ) {

	litm_code code;
	struct timeval *start, *current;
	int computed_timeout = __litm_compute_timeout( timeout );

	while(1) {

		code = switch_send(conn, bus_id, msg, cleaner);
		if (LITM_CODE_OK==code)
			break;

		// an error we can't handle here?
		if (LITM_CODE_BUSY!=code) {
			break;
		}

		DEBUG_LOG(LOG_DEBUG, "litm_send_wait: NEED TO RETRY conn[%x][%3i] bus[%3i]", conn, conn->id, bus_id);
		sched_yield();
		//usleep( 10*1000 );

		// at this point, we just back-off an retry
		int result = random_sleep_period(	start,
											current,
											LITM_DEFAULT_MAX_BACKOFF*1000*1000,
											computed_timeout);
		// expired period?
		if (1==result) {
			code = LITM_CODE_ERROR_SEND_ERROR;
			break;
		}

	}

	return code;
}//


	litm_code
litm_send_shutdown( 	litm_connection *conn,
						litm_bus bus_id,
						void *msg,
						void (*cleaner)(void *msg) ) {

	return switch_send_shutdown(conn, bus_id, msg, cleaner);
}//

/**
 * Receive (non-blocking) function for clients
 *
 * \note It is the responsibility of the client
 *       to _not_ use this function once a
 *       connection is closed.
 *
 */
	litm_code
litm_receive_nb(litm_connection *conn, litm_envelope **envlp) {

	if (NULL==conn) {
		return LITM_CODE_ERROR_BAD_CONNECTION;
	}
	int returnCode = LITM_CODE_OK; //optimistic

	*envlp = queue_get_nb( conn->input_queue );
	if (NULL==*envlp) {
		returnCode=LITM_CODE_NO_MESSAGE;
	} else {
		conn->received++;
	}

	return returnCode;
}//

/**
 * Receive with wait
 */
	litm_code
litm_receive_wait(litm_connection *conn, litm_envelope **envlp) {

	if (NULL==conn) {
		return LITM_CODE_ERROR_BAD_CONNECTION;
	}
	int returnCode = LITM_CODE_OK; //optimistic

	while(1) {

		// let's see first if we need to bother with the
		// conditional waiting on the queue.
		// This also takes care of the probability of
		//  missing a signal for whatever reason
		*envlp = queue_get_nb( conn->input_queue );
		if (NULL!=*envlp) {

			conn->received++;
			break;
		}

		//give the chance to another thread
		usleep(1);
		//sched_yield();

		int rc= queue_wait( conn->input_queue );
		if (rc) {
			returnCode = LITM_CODE_ERROR_RECEIVE_WAIT;
			break;
		}
		// try again... this time around,
		// we have been signaled that a message is
		// ready... we should get lucky!

	}//while

	//DEBUG_LOG(LOG_DEBUG,"litm_receive_wait, STOP conn[%x]",conn);
	return returnCode;
}//

/**
 * Release a message to LITM
 *
 * This function should *ONLY* be used by client's
 *  as a result of finishing the processing of an
 *  envelope/message retrieved through ``litm_receive_nb''.
 *
 */
	litm_code
litm_release(litm_connection *conn, litm_envelope *envlp) {

	return switch_release( conn, envlp );
}//


	void *
litm_get_message(litm_envelope *envlp) {

	if (NULL==envlp) {
		return NULL;
	}

	return envlp->msg;
}//

	char *
litm_translate_code(litm_code code) {

	if ((sizeof(LITM_CODE_MESSAGES)/sizeof(char *)) <= code )
		return NULL;

	return LITM_CODE_MESSAGES[code];
}//

/**
 * Translates a seconds based timeout to a
 * useconds timeout value whilst adjusting for
 * the default timeout value.
 *
 */
	int
__litm_compute_timeout(int timeout) {

	int computed_timeout;

	// adjust to default
	if (0!=timeout) {
		computed_timeout = timeout;
	} else {
		computed_timeout = LITM_DEFAULT_MAX_TIMEOUT;
	}
	computed_timeout *= 1000*1000;

	return computed_timeout;
}


	void
litm_wait_shutdown(void){

	__switch_wait_shutdown();

}
