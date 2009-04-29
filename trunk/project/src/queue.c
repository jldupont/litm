/*
 * queue.c
 *
 *  Created on: 2009-04-24
 *      Author: Jean-Lou Dupont
 */

#include <pthread.h>
#include <errno.h>

#include "../includes/logger.h"
#include "litm.h"
#include "queue.h"


/**
 * Creates a queue
 */
queue *queue_create(void) {

	//DEBUG_LOG(LOG_INFO, "queue_create: BEGIN");

	// if this malloc fails,
	//  there are much bigger problems that loom
	pthread_mutex_t *mutex = malloc( sizeof(pthread_mutex_t) );
	queue *q = malloc( sizeof(queue) );

	if ((NULL != q) && (NULL != mutex)){

		q->head  = NULL;
		q->tail  = NULL;

		pthread_mutex_init( mutex, NULL );
		q->mutex = mutex;

	} else {

		DEBUG_LOG(LOG_DEBUG, "queue_create: MALLOC ERROR");

		if (NULL!=q)
			free(q);
		if (NULL!=mutex)
			free(mutex);
	}

	//DEBUG_LOG(LOG_INFO, "queue_create: END");
	return q;
}// init

/**
 * Destroys a queue
 *  _but_ isn't aware of the potential
 *  messages inside it...
 *
 *  TODO make more robust
 *
 *  The ``cleaner`` agent should be registered
 *  along with the queue...
 *
 */
void queue_destroy(queue *q) {

	if (NULL==q) {

		DEBUG_LOG(LOG_DEBUG, "queue_destroy: NULL queue ptr");
		return;
	}

	pthread_mutex_lock( q->mutex );
		free(q);
		q=NULL;
	pthread_mutex_unlock( q->mutex );
}

/**
 * Queues a message in the communication queue
 *
 * Returns 1 on success, 0 on error
 */
int queue_put(queue *q, void *message) {

	if ((NULL==q) || (NULL==message)) {
		DEBUG_LOG(LOG_DEBUG, "queue_destroy: NULL queue/msg ptr");
		return 0;
	}

	int code = 1;
	queue_node *tmp=NULL;

	//DEBUG_LOG(LOG_DEBUG,"queue_put: BEGIN");

	pthread_mutex_lock( q->mutex );

		// if this malloc fails,
		//  there are much bigger problems that loom
		tmp = (queue_node *) malloc(sizeof(queue_node));
		if (NULL!=tmp) {

			DEBUG_LOG(LOG_DEBUG, "queue_put: q[%x] message[%x]", q, message);

			tmp->msg  = message;
			tmp->next = NULL;

			// there is a tail... put at the end
			if (NULL!=q->tail)
				(q->tail)->next=tmp;

			// point tail to the new element
			q->tail = tmp;

			// adjust head
			if (NULL==q->head)
				q->head=tmp;

		} else {
			code = 0;
		}
	pthread_mutex_unlock( q->mutex );

	//DEBUG_LOG(LOG_DEBUG,"queue_put: END");

	return code;
}//[/queue_put]


/**
 * Queues a message (non-blocking)
 *  in the communication queue
 *
 * Returns	1 on success,
 * 			0 on error
 * 			-1 on busy
 */
int queue_put_nb(queue *q, void *msg) {

	if ((NULL==q) || (NULL==msg)) {
		DEBUG_LOG(LOG_DEBUG, "queue_put_nb: NULL queue/msg ptr");
		return 0;
	}

	queue_node *tmp=NULL;

	//DEBUG_LOG(LOG_DEBUG,"queue_put_nb: BEGIN");

	int code = pthread_mutex_trylock( q->mutex );
	if (EBUSY==code)
		return -1;

		// if this malloc fails,
		//  there are much bigger problems that loom
		tmp = (queue_node *) malloc(sizeof(queue_node));
		if (NULL!=tmp) {

			tmp->msg = msg;

			//perform the ''put'' operation {
				//no next yet...
				tmp->next=NULL;

				//insert pointer to element first
				if (NULL!=q->tail)
					(q->tail)->next=tmp;

				// insert element
				q->tail = tmp;

				//was the queue empty?
				if (NULL==q->head)
					q->head=tmp;

			// success
			code = 1;
			DEBUG_LOG(LOG_DEBUG,"queue_put_nb: QUEUED q[%x] msg[%x] queue_node[%x]", q, msg, tmp);
			//}

		} else {
			code = 0; // ERROR
		}

	pthread_mutex_unlock( q->mutex );

	return code;
}//[/queue_put]

/**
 * Retrieves the next message from the communication queue
 * Returns NULL if none.
 */
void *queue_get(queue *q) {

	if (NULL==q) {
		DEBUG_LOG(LOG_DEBUG, "queue_get: NULL queue ptr");
		return NULL;
	}

	queue_node *tmp = NULL;
	void *msg=NULL;

	//DEBUG_LOG(LOG_DEBUG,"queue_get: BEGIN");

	pthread_mutex_lock( q->mutex );

		tmp = q->head;
		if (tmp!=NULL) {

			// adjust tail: case if tail==head
			//  ie. only one element present
			if (q->head == q->tail) {
				q->tail = NULL;
			}

			// adjust head : next pointer is already set
			//  to NULL in queue_put
			q->head = (q->head)->next;
			msg = tmp->msg;



			DEBUG_LOG(LOG_DEBUG,"queue_get: MESSAGE PRESENT, freeing queue_node[%x]", tmp);

			free(tmp);

			tmp = NULL;
		}

	pthread_mutex_unlock( q->mutex );

	//DEBUG_LOG(LOG_DEBUG,"queue_get: END");

	return msg;
}//[/queue_get]

/**
 * Returns NULL if NONE / BUSY
 */
void *queue_get_nb(queue *q) {

	if (NULL==q) {
		DEBUG_LOG(LOG_DEBUG, "queue_get_nb: NULL queue ptr");
		return NULL;
	}

	//DEBUG_LOG_PTR(q, LOG_ERR, "queue_get_nb: NULL");

	queue_node *tmp = NULL;
	void *msg=NULL;

	//DEBUG_LOG(LOG_DEBUG,"queue_get_nb: BEGIN");

	int code = pthread_mutex_trylock( q->mutex );
	if (EBUSY==code) {
		//DEBUG_LOG(LOG_DEBUG,"queue_get_nb: BUSY");
		return NULL;
	}

		tmp = q->head;
		if (NULL!=tmp) {

			// adjust tail: case if tail==head
			//  ie. only one element present
			if (q->head == q->tail) {
				q->tail = NULL;
			}


			q->head = (q->head)->next;
			msg = tmp->msg;

			DEBUG_LOG(LOG_DEBUG,"queue_get_nb: MESSAGE PRESENT, q[%x] envlp[%x], freeing queue_node[%x]", q, msg, tmp);

			free(tmp);
			tmp=NULL;
		}

	pthread_mutex_unlock( q->mutex );

	//DEBUG_LOG(LOG_DEBUG,"queue_get_nb: END");

	return msg;
}//[/queue_get]

/**
 * Verifies if a message is present
 *
 * Returns 1 if at least 1 message is present,
 *  0 if NONE
 *  -1 on ERROR
 */
int queue_peek(queue *q) {

	if (NULL==q) {
		DEBUG_LOG(LOG_DEBUG, "queue_peek: NULL queue ptr");
		return -1;
	}

	queue_node *tmp = NULL;
	int result = 0;

	//DEBUG_LOG(LOG_DEBUG,"queue_peek: BEGIN");

	pthread_mutex_lock( q->mutex );

		tmp = q->head;
		result = (tmp!=NULL);

	pthread_mutex_unlock( q->mutex );

	//DEBUG_LOG(LOG_DEBUG,"queue_peek: END");

	return result;
} // queue_peek
