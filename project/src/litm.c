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


	litm_code
litm_connect(litm_connection **conn) {

	switch_init();

	litm_code code = litm_connection_open(conn);
	if (LITM_CODE_OK!=code)
		return code;

	return LITM_CODE_OK;

}//


	litm_code
litm_disconnect(litm_connection *conn) {

	return litm_connection_close(conn);
}//


	litm_code
litm_subscribe(litm_connection *conn, litm_bus bus_id) {

	return litm_switch_add_subscriber( conn, bus_id );
}//


	litm_code
litm_unsubscribe(litm_connection *conn, litm_bus bus_id) {

	return litm_switch_remove_subscriber( conn, bus_id );
}//

	litm_code
litm_send(litm_connection *conn, litm_bus bus_id, void *msg) {

	return litm_switch_send(conn, bus_id, msg);
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


	litm_code
litm_release(litm_connection *conn, litm_envelope *envlp) {

}//


	void *
litm_get_message(litm_envelope *envlp) {

	if (NULL==envlp) {
		return NULL;
	}

	return envlp->msg;
}//
