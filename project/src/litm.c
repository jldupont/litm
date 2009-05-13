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
		"LITM_CODE_ERROR_END_OF_SUBSCRIBERS_LIST"
};

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
			void (*cleaner)(void *msg) ) {

	return switch_send(conn, bus_id, msg, cleaner);
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

	// let's see first if we need to bother with the
	// conditional waiting on the queue.
	// This also takes care of the probability of
	//  missing a signal for whatever reason
	*envlp = queue_get_nb( conn->input_queue );
	if (NULL!=*envlp) {
		returnCode=LITM_CODE_OK;
	}

	// ok, we really need to wait then...
	*envlp = queue_get_wait( conn->input_queue );

	// this shouldn't happen since we waited for a message!
	if (NULL==*envlp) {
		returnCode=LITM_CODE_NO_MESSAGE;
	}

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


	void
litm_shutdown(void) {

	switch_shutdown();

}

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
