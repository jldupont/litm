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


litm_connection *_connections[LITM_CONNECTION_MAX];


	litm_connection *
litm_connection_open(void) {

	litm_connection *c;

	c=malloc(sizeof(litm_connection));
	if (NULL==c)
		return NULL;

	queue *q = queue_create();
	if (NULL==q) {
		free(c);
		return NULL;
	}

	c->input_queue = q;

	return c;
}//



	void
litm_connection_close(litm_connection *conn) {
	DEBUG_LOG_PTR(conn, "litm_connection_close: NULL conn");

	queue_destroy( conn->input_queue );
	free( conn );

}//
