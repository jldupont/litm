/*
 * connection.c
 *
 *  Created on: 2009-04-24
 *      Author: Jean-Lou Dupont
 */
#include <pthread.h>

#include "litm.h"
#include "logger.h"
#include "connection.h"
#include "queue.h"

// PRIVATE
int litm_connection_get_free_index(void);


litm_connection *_connections[LITM_CONNECTION_MAX];


	litm_code
litm_connection_open(litm_connection **conn) {

	int target_index;

	target_index = litm_connection_get_free_index();
	if (-1 == target_index)
		return LITM_CODE_ERROR_NO_MORE_CONNECTIONS;

	*conn=malloc(sizeof(litm_connection));
	if (NULL==*conn)
		return LITM_CODE_ERROR_MALLOC;

	queue *q = queue_create();
	if (NULL==q) {
		free(*conn);
		return LITM_CODE_ERROR_MALLOC;
	}

	_connections[target_index] = *conn;
	(*conn)->input_queue = q;

	return LITM_CODE_OK;
}//


/**
 * Verifies if a `spot` is available in
 *  the connection map OR -1 if none.
 *
 */
	int
litm_connection_get_free_index(void) {

	int index, result=-1;
	for (index=1; index<LITM_CONNECTION_MAX; index++) {
		if (NULL==_connections[index]) {
			result=index;
			break;
		}
	}

	return index;
}//

	litm_code
litm_connection_close(litm_connection *conn) {
	DEBUG_LOG_PTR(conn, "litm_connection_close: NULL conn");

	if (NULL==conn) {
		return LITM_CODE_ERROR_BAD_CONNECTION;
	}
	//is it really a connection?
	int index = litm_connection_get_index(conn);

	if (-1 == index) {
		return LITM_CODE_ERROR_BAD_CONNECTION;
	}

	_connections[index] = NULL;
	queue_destroy( conn->input_queue );
	free( conn );

	return LITM_CODE_OK;
}//

	int
litm_connection_get_index(litm_connection *conn) {

	int index, result = -1; //pessimistic

	for (index=1; index<LITM_CONNECTION_MAX; index++ ) {
		if ( conn ==_connections[index] ) {
			result = index;
			break;
		}
	}//for

	return result;
}//


	litm_connection *
litm_connection_get_ptr(int connection_index) {

	if ((LITM_CONNECTION_MAX <= connection_index) || 0==connection_index)
		return NULL;

	return _connections[connection_index];
}//
