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


	litm_code
litm_connect(litm_connection **conn) {

	return litm_connect_ex( conn, 0);
}//

	litm_code
litm_connect_ex(litm_connection **conn, int id) {

	switch_init();

	DEBUG_LOG(LOG_INFO, "connect_ex BEGIN, id[%i]", id);

	litm_code code = litm_connection_open_ex(conn, id);
	if (LITM_CODE_OK!=code) {
		*conn = NULL;
		return code;
	}

	return LITM_CODE_OK;
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
litm_unsubscribe(litm_connection *conn, litm_bus bus_id) {

	return switch_remove_subscriber( conn, bus_id );
}//

	litm_code
litm_send(	litm_connection *conn,
			litm_bus bus_id,
			void *msg,
			void (*cleaner)(void *msg),
			int type) {

	return switch_send(conn, bus_id, msg, cleaner, type);
}//




#ifdef _DEBUG
	void _litm_log_timediff(litm_envelope *env) {

		struct timeval *sent, current, lapsed;

		sent=env->sent_time;

		gettimeofday( &current, NULL );

		if (sent->tv_usec > current.tv_usec) {
			current.tv_usec += 1000000;
			current.tv_sec--;
		}
		lapsed.tv_usec = current.tv_usec - sent->tv_usec;
		lapsed.tv_sec  = current.tv_sec  - sent->tv_sec;

		double elapsed = lapsed.tv_sec + (1.0/(double)lapsed.tv_usec);

		doLog(LOG_INFO,">>> enlvp[%x] elapsed[%e]", env, elapsed);
	}//
#endif

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
		DEBUG_FUNC(_litm_log_timediff(*envlp));
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
			DEBUG_FUNC(_litm_log_timediff(*envlp));
			conn->received++;
			break;
		}

		//give the chance to another thread
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

	DEBUG_LOG(LOG_DEBUG,"litm_receive_wait, STOP conn[%x]",conn);
	return returnCode;
}//

/**
 * Receive with wait and unblocked through the 'timer' message
 */
	litm_code
litm_receive_wait_timer(litm_connection *conn, litm_envelope **envlp, int usec_timer) {

	if (NULL==conn) {
		return LITM_CODE_ERROR_BAD_CONNECTION;
	}
	int returnCode = LITM_CODE_OK; //optimistic

	// let's see first if we need to bother with the
	// conditional waiting on the queue.
	// This also takes care of the probability of
	//  missing a signal for whatever reason
	*envlp = queue_get_nb( conn->input_queue );
	if (NULL!=*envlp) {
		DEBUG_FUNC(_litm_log_timediff(*envlp));
		conn->received++;
		return returnCode;  // <============================
	}

	//give the chance to another thread
	//sched_yield();

	//DEBUG_LOG(LOG_DEBUG,"litm_receive_wait_timer, BEFORE WAIT conn[%x][%i]",conn, conn->id);
	int rc= queue_wait_timer( conn->input_queue, usec_timer );
	//DEBUG_LOG(LOG_DEBUG,"litm_receive_wait_timer, AFTER WAIT conn[%x][%i]",conn, conn->id);
	if (0!=rc) {

		DEBUG_LOG(LOG_DEBUG,"!!! litm_receive_wait_timer, ERROR COND WAIT conn[%x][%i] rc[%i]",conn, conn->id, rc);
		returnCode = LITM_CODE_ERROR_RECEIVE_WAIT;

	} else {

		*envlp = queue_get_nb( conn->input_queue );
		if (NULL!=*envlp) {
			DEBUG_FUNC(_litm_log_timediff(*envlp));
			conn->received++;
			returnCode=LITM_CODE_OK;
		} else {
			returnCode=LITM_CODE_NO_MESSAGE;
		}
	}


	//DEBUG_LOG(LOG_DEBUG,"litm_receive_wait_timer, STOP conn[%x][%i]",conn, conn->id);
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
litm_get_message(litm_envelope *envlp, int *type) {

	if (NULL==envlp) {
		return NULL;
	}

	*type = envlp->type;
	return envlp->msg;
}//


	char *
litm_translate_code(litm_code code) {

	if ((sizeof(LITM_CODE_MESSAGES)/sizeof(char *)) <= code )
		return NULL;

	return LITM_CODE_MESSAGES[code];
}//


	void
litm_wait_shutdown(void){

	__switch_wait_shutdown();

}
