/*
 * litm.c
 *
 *  Created on: 2009-04-24
 *      Author: Jean-Lou Dupont
 */


#include "litm.h"
#include "connection.h"
#include "queue.h"
#include "pool.h"

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
		"LITM_CODE_ERROR_INVALID_ENVELOPE"
};

	litm_code
litm_connect(litm_connection **conn) {

	switch_init();

	litm_code code = litm_connection_open(conn);
	if (LITM_CODE_OK!=code) {
		*conn = NULL;
		return code;
	}

	return LITM_CODE_OK;

}//


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
litm_receive_nb(litm_connection *conn, litm_envelope **envlp) {

	if (NULL==conn) {
		return LITM_CODE_ERROR_BAD_CONNECTION;
	}

	*envlp = queue_get_nb( conn->input_queue );
	if (NULL==*envlp) {
		return LITM_CODE_NO_MESSAGE;
	}

	return LITM_CODE_OK;
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

	if (sizeof(LITM_CODE_MESSAGES) <= code )
		return NULL;

	return LITM_CODE_MESSAGES[code];
}//
