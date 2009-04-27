/*
 * connection.c
 *
 *  Created on: 2009-04-24
 *      Author: Jean-Lou Dupont
 */
#include <pthread.h>
#include <errno.h>

#include "litm.h"
#include "logger.h"
#include "connection.h"
#include "queue.h"

// PRIVATE
int __litm_connection_get_free_index(void);
/**
 * Returns the index for a specified connection
 *  or -1 on Error
 */
int	__litm_connection_get_index(litm_connection *conn);
/**
 * Returns the connection for a specified index
 *  or NULL on error
 */
litm_connection *__litm_connection_get_ptr(int connection_index);

void _litm_connections_lock(void);
void _litm_connections_unlock(void);


// PRIVATE VARIABLES
// -----------------
pthread_mutex_t  _connections_mutex = PTHREAD_MUTEX_INITIALIZER;
litm_connection *_connections[LITM_CONNECTION_MAX];


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


	litm_code
litm_connection_open(litm_connection **conn) {

	int target_index, code;

	code = pthread_mutex_trylock( &_connections_mutex );
	if (EBUSY==code) {
		return LITM_CODE_BUSY;
	}

	target_index = __litm_connection_get_free_index();
	if (-1 == target_index) {
		pthread_mutex_unlock( &_connections_mutex );
		return LITM_CODE_ERROR_NO_MORE_CONNECTIONS;
	}

	*conn=malloc(sizeof(litm_connection));
	if (NULL==*conn) {
		pthread_mutex_unlock( &_connections_mutex );
		return LITM_CODE_ERROR_MALLOC;
	}

	queue *q = queue_create();
	if (NULL==q) {
		free(*conn);
		pthread_mutex_unlock( &_connections_mutex );
		return LITM_CODE_ERROR_MALLOC;
	}

	_connections[target_index] = *conn;
	(*conn)->input_queue = q;

	pthread_mutex_unlock( &_connections_mutex );
	return LITM_CODE_OK;
}//



	litm_code
litm_connection_close(litm_connection *conn) {
	DEBUG_LOG_PTR(conn, "litm_connection_close: NULL conn");

	if (NULL==conn) {
		return LITM_CODE_ERROR_BAD_CONNECTION;
	}

	int code = pthread_mutex_trylock( &_connections_mutex );
	if (EBUSY==code) {
		return LITM_CODE_BUSY;
	}

	//is it really a connection?
	int index = __litm_connection_get_index(conn);

	if (-1 == index) {
		pthread_mutex_unlock( &_connections_mutex );
		return LITM_CODE_ERROR_BAD_CONNECTION;
	}

	_connections[index] = NULL;

	// don't need to hold the mutex for the last part
	// of the operation: the connection isn't part of
	// the protected table anymore AND we stand less
	// chances for a livelock since we would be holding
	// another mutex when calling ``queue_destroy``
	pthread_mutex_unlock( &_connections_mutex );

	queue_destroy( conn->input_queue );
	free( conn );


	return LITM_CODE_OK;
}//

	int
__litm_connection_get_index(litm_connection *conn) {

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
__litm_connection_get_ptr(int connection_index) {

	if ((LITM_CONNECTION_MAX <= connection_index) || 0==connection_index)
		return NULL;

	return _connections[connection_index];
}//

/**
 * Verifies if a `spot` is available in
 *  the connection map OR -1 if none.
 *
 */
	int
__litm_connection_get_free_index(void) {

	int index, result=-1;
	for (index=1; index<LITM_CONNECTION_MAX; index++) {
		if (NULL==_connections[index]) {
			result=index;
			break;
		}
	}

	return index;
}//


	void
_litm_connections_lock(void) {

	pthread_mutex_lock( &_connections_mutex );
}//

	void
_litm_connections_unlock(void) {

	pthread_mutex_unlock( &_connections_mutex );
}//
