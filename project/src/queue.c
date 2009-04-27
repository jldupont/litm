/*
 * queue.c
 *
 *  Created on: 2009-04-24
 *      Author: Jean-Lou Dupont
 */

#include <pthread.h>

#include "../includes/logger.h"
#include "litm.h"
#include "queue.h"


/**
 * Creates a queue
 */
queue *queue_create(void) {

	// if this malloc fails,
	//  there are much bigger problems that loom
	pthread_mutex_t *mutex = malloc( sizeof(pthread_mutex_t) );
	queue *q = malloc( sizeof(queue) );
	if ((NULL != q) && (NULL!=mutex)){

		q->head  = NULL;
		q->tail  = NULL;

		pthread_mutex_init( mutex, NULL );
		q->mutex = mutex;
	}

	return q;
}// init

/**
 * Safely destroys a queue
 */
void queue_destroy(queue *q) {

	if (NULL!=q)
		free(q);
}

/**
 * Queues a message in the communication queue
 *
 * Returns 1 on success, 0 on error
 */
int queue_put(queue *q, void *msg) {

	queue_node *tmp=NULL;

	//DEBUG_LOG(LOG_DEBUG,"queue_put: BEGIN");

	pthread_mutex_lock( q->mutex );

		// if this malloc fails,
		//  there are much bigger problems that loom
		tmp = (queue_node *) malloc(sizeof(queue_node));
		if (NULL==tmp) {

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
			//}
		} else {
			code = 0;
		}
	pthread_mutex_unlock( q->mutex );

	//DEBUG_LOG(LOG_DEBUG,"queue_put: END");

	return 1;
}//[/queue_put]


/**
 * Queues a message (non-blocking)
 *  in the communication queue
 *
 * Returns	1 on success,
 * 			0 on error
 * 			-1 on pending
 */
int queue_put_nb(queue *q, void *msg) {

	queue_node *tmp=NULL;
	int code = 0;

	//DEBUG_LOG(LOG_DEBUG,"queue_put: BEGIN");

	code = pthread_mutex_trylock( q->mutex );
	if (EBUSY)
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
			//}

		} else {
			code = 0;
		}

	pthread_mutex_unlock( q->mutex );

	//DEBUG_LOG(LOG_DEBUG,"queue_put: END");

	return 1;
}//[/queue_put]

/**
 * Retrieves the next message from the communication queue
 * Returns NULL if none.
 */
void *queue_get(queue *q) {

	queue_node *tmp = NULL;
	void *msg=NULL;

	//DEBUG_LOG(LOG_DEBUG,"queue_get: BEGIN");

	pthread_mutex_lock( q->mutex );

		tmp = q->head;
		if (tmp!=NULL) {
			q->head = q->head->next;
			msg = tmp->msg;
			free(tmp);
			doLog(LOG_DEBUG,"queue_get: MESSAGE PRESENT");
		}

	pthread_mutex_unlock( q->mutex );

	//DEBUG_LOG(LOG_DEBUG,"queue_get: END");

	return msg;
}//[/queue_get]

/**
 * Verifies if a message is present
 *
 * Returns 1 if at least 1 message is present
 */
int queue_peek(queue *q) {

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
