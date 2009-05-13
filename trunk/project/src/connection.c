/**
 * @file connection.c
 *
 * @date   2009-04-24
 * @author Jean-Lou Dupont
 *
 * \section Overview
 * 			This module implements the ``connection`` aspect:
 *
 * 			- opening
 * 			- closing
 *
 *			A limited number of connections are supported at any point
 *			in time. When a connection is closed, it is queued for
 *			later deletion: the connection can not be used for any
 *			access from this point onwards.
 *
 * \section Connections
 *
 * 			From a client operation point of view, the connection table
 * 			is only locked whilst performing:
 *
 * 			- ``open`` and ``close`` connection operations
 * 			- ``add`` and ``remove`` subscription operations
 *
 * 			at which times the return code LITM_CODE_BUSY might be used
 * 			to signal to the client to try again later. This behavior is
 * 			necessary in order for the ``switch`` to be in control
 * 			position, thus simplifying the architecture and API.
 *
 *			Since the ``switch`` is in control, the connection table
 *			is also held in locked state whilst dispatching envelopes
 *			to clients.
 *
 *
 * \section Deletion
 *
 * 			Connection deletion is performed in the following manner:
 *
 *			- the connection's status is changed to "PENDING_DELETION"
 * 			- the connection identifier is placed in a stack (pending deletion)
 * 			- the ``switch`` inspects the said stack at regular interval
 *
 *
 */
#include <pthread.h>
#include <errno.h>

#include "litm.h"
#include "connection.h"
#include "queue.h"
#include "logger.h"

// PRIVATE
int __litm_connection_get_index(litm_connection *(table)[], litm_connection *conn);
int __litm_connection_get_free_index(litm_connection *(table)[]);
litm_connection *__litm_connection_get_ptr(int connection_index);


// PRIVATE VARIABLES
// -----------------

	// ACTIVE CONNECTIONS
	// ------------------
pthread_mutex_t  _connections_mutex = PTHREAD_MUTEX_INITIALIZER;
litm_connection *_connections[LITM_CONNECTION_MAX];

	// CONNECTIONS PENDING DELETION
	// ----------------------------
pthread_mutex_t  _connections_pending_deletion_mutex = PTHREAD_MUTEX_INITIALIZER;
litm_connection *_connections_pending_deletion[LITM_CONNECTION_MAX];


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Returns the connection id
 *
 * @return -1 on error
 */
	int
litm_connection_get_id(litm_connection *conn) {

	if (NULL==conn) {
		return -1;
	}

	return conn->id;
}//

/**
 * Opens a ``connection``
 *
 */
	litm_code
litm_connection_open(litm_connection **conn) {

	return litm_connection_open_ex( conn, 0);

}//


	litm_code
litm_connection_open_ex(litm_connection **conn, int id) {

	//DEBUG_LOG(LOG_INFO, "litm_connection_open: BEGIN");

	int target_index, code;

	code = pthread_mutex_trylock( &_connections_mutex );
	if (EBUSY==code) {
		return LITM_CODE_BUSY;
	}

	target_index = __litm_connection_get_free_index(_connections);
	if (-1 == target_index) {
		pthread_mutex_unlock( &_connections_mutex );
		return LITM_CODE_ERROR_NO_MORE_CONNECTIONS;
	}

	*conn=malloc(sizeof(litm_connection));
	if (NULL==*conn) {
		pthread_mutex_unlock( &_connections_mutex );
		return LITM_CODE_ERROR_MALLOC;
	}

	queue *q = queue_create(id);
	if (NULL==q) {
		free(*conn);
		pthread_mutex_unlock( &_connections_mutex );
		return LITM_CODE_ERROR_MALLOC;
	}


	_connections[target_index] = *conn;
	(*conn)->id = id;
	(*conn)->input_queue = q;
	(*conn)->status = LITM_CONNECTION_STATUS_ACTIVE;

	//DEBUG_LOG(LOG_INFO, "litm_connection_open: OPENED, index[%u] ref[%x], q[%x]", target_index, *conn, q);


	pthread_mutex_unlock( &_connections_mutex );

	//DEBUG_LOG(LOG_INFO, "litm_connection_open: END");
	return LITM_CODE_OK;

}//

	litm_code
litm_connection_close(litm_connection *conn) {

	if (NULL==conn) {
		return LITM_CODE_ERROR_BAD_CONNECTION;
	}

	// FLAG the connection has been about to be deleted
	//  Hopefully, this doesn't cause too much blockage...
	pthread_mutex_lock( &_connections_mutex );
		conn->status = LITM_CONNECTION_STATUS_PENDING_DELETION;
	pthread_mutex_unlock( &_connections_mutex );


	pthread_mutex_lock( &_connections_pending_deletion_mutex );

		int target_index = __litm_connection_get_free_index(_connections_pending_deletion);

		if (-1 == target_index) {
			pthread_mutex_unlock( &_connections_pending_deletion_mutex );
			return LITM_CODE_ERROR_NO_MORE_CONNECTIONS;
		}

		_connections_pending_deletion[target_index] = conn;

		DEBUG_LOG(LOG_INFO, "litm_connection_close: PENDING conn[%x]", conn);

	pthread_mutex_unlock( &_connections_pending_deletion_mutex );

	return LITM_CODE_OK;
}//

	litm_connection *
__litm_connection_get_ptr(int connection_index) {

	if ((LITM_CONNECTION_MAX <= connection_index) || 0==connection_index)
		return NULL;

	return _connections[connection_index];
}//

	int
__litm_connection_get_index(litm_connection *(table)[], litm_connection *conn) {

	int index, result = -1; //pessimistic

	for (index=1; index<LITM_CONNECTION_MAX; index++ ) {
		if ( conn == table[index] ) {
			result = index;
			break;
		}
	}//for

	return result;
}//



/**
 * Verifies if a `spot` is available in
 *  the connection map OR -1 if none.
 *
 */
	int
__litm_connection_get_free_index(litm_connection *(table)[]) {

	int index, result=-1;
	for (index=1; index<LITM_CONNECTION_MAX; index++) {
		if (NULL==table[index]) {
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

	int
_litm_connections_trylock(void) {

	return pthread_mutex_trylock( &_connections_mutex );
}//


	void
_litm_connections_unlock(void) {

	pthread_mutex_unlock( &_connections_mutex );
}//

/**
 * @see _litm_connection_validate_safe
 *
 */
	int
_litm_connection_validate(litm_connection *conn) {

	if (EBUSY==_litm_connections_trylock())
		return -1;

		int returnCode = _litm_connection_validate_safe( conn );

	_litm_connections_unlock();

	return returnCode;
}

/**
 * Connection validation
 *
 * @return 0 => not active
 * @return 1 => active
 *
 */
	int
_litm_connection_validate_safe(litm_connection *conn) {

	int i, returnCode = 0;
	for (i=1;i<=LITM_CONNECTION_MAX;i++) {
		if ( conn == _connections[i] ) {
			returnCode = 1;
			break;
		}
	}

	return returnCode;
}//

	void
_litm_connection_lock(litm_connection *conn) {

	pthread_mutex_lock( conn->input_queue->mutex );

}//

	void
_litm_connection_unlock(litm_connection *conn) {

	pthread_mutex_unlock( conn->input_queue->mutex );
}

	litm_connection_status
_litm_connection_get_status(litm_connection *conn) {

	return conn->status;

}
