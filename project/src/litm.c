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

	*conn = litm_connection_open();
	if (NULL==*conn)
		return LITM_CODE_ERROR_CONNECTION_OPEN;

	return LITM_CODE_OK;

}//


	litm_code
litm_disconnect(litm_connection *conn) {

	litm_connection_close(conn);

	return LITM_CODE_OK;
}//


	litm_code
litm_subscribe(litm_connection *conn, litm_bus bus_id) {

}//


	litm_code
litm_unsubscribe(litm_connection *conn, litm_bus bus_id) {


}//

	litm_code
litm_send(litm_connection *conn, litm_bus bus_id, void *msg) {

}//


	litm_code
litm_receive(litm_connection *conn, litm_envelope *envlp) {

}//


	litm_code
litm_receive_nb(litm_connection *conn, litm_envelope *envlp) {

}//


	litm_code
litm_release(litm_connection *conn, litm_envelope *envlp) {

}//


	void *
litm_get_message(litm_envelope *envlp) {

}//
