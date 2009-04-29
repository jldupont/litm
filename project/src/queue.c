/**
 * @file queue.c
 *
 * @date 2009-04-24
 * @author: Jean-Lou Dupont
 *
 *
 * The _queue_ module isn't aware of its elements
 * and thus it is the responsibility of the clients
 * to properly dispose of the _nodes_ once retrieved.
 *
 * The term ``node`` is used generically to refer
 * to a node element inside a queue.
 *
 */

#include <pthread.h>
#include <errno.h>

#include "logger.h"
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

	return q;
}// init

/**
 * Destroys a queue
 *
 * This function is **not** aware of the
 *  message(s) potentially inside the queue,
 *  thus, the queue must be drained **before**
 *  using this function.
 *
 * The queue can be drained by:
 * - stopping the thread that ``puts``
 * - ``getting`` until NULL is returned
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
 * Queues a node (blocking)
 *
 * @return 1 => success
 * @return 0 => error
 *
  */
int queue_put(queue *q, void *node) {

	if ((NULL==q) || (NULL==node)) {
		DEBUG_LOG(LOG_DEBUG, "queue_destroy: NULL queue/node ptr");
		return 0;
	}

	pthread_mutex_lock( q->mutex );

		int code = queue_put_safe( q, node );

	pthread_mutex_unlock( q->mutex );

	//DEBUG_LOG(LOG_DEBUG,"queue_put: END");

	return code;
}//[/queue_put]

/**
 * Queue_put_safe
 *
 * Lock is not handled
 *
 * @return 0 => error
 * @return 1 => success
 *
 */
	int
queue_put_safe( queue *q, void *node ) {

	int code = 1;
	queue_node *tmp=NULL;

	// if this malloc fails,
	//  there are much bigger problems that loom
	tmp = (queue_node *) malloc(sizeof(queue_node));
	if (NULL!=tmp) {

		tmp->node = node;
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

	return code;
}//



/**
 * Queues a node (non-blocking)
 *
 * @return 1  => success
 * @return 0  => error
 * @return -1 => busy
 *
 */
int queue_put_nb(queue *q, void *node) {

	if ((NULL==q) || (NULL==node)) {
		DEBUG_LOG(LOG_DEBUG, "queue_put_nb: NULL queue/node ptr");
		return 0;
	}

	if (EBUSY == pthread_mutex_trylock( q->mutex ))
		return -1;

		int returnCode = queue_put_safe( q, node );

	pthread_mutex_unlock( q->mutex );

	return returnCode;
}//[/queue_put]


/**
 * Retrieves the next node from a queue
 *
 * @return NULL if none.
 *
 */
void *queue_get(queue *q) {

	if (NULL==q) {
		DEBUG_LOG(LOG_DEBUG, "queue_get: NULL queue ptr");
		return NULL;
	}

	queue_node *tmp = NULL;
	void *node=NULL;

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

			node = tmp->node;

			//DEBUG_LOG(LOG_DEBUG,"queue_get: MESSAGE PRESENT, freeing queue_node[%x]", tmp);
			free(tmp);
		}

	pthread_mutex_unlock( q->mutex );

	return node;
}//[/queue_get]

/**
 * Retrieves the next node from a queue (non-blocking)
 *
 * @return NULL => No node OR BUSY
 *
 */
void *queue_get_nb(queue *q) {

	if (NULL==q) {
		DEBUG_LOG(LOG_DEBUG, "queue_get_nb: NULL queue ptr");
		return NULL;
	}

	queue_node *tmp = NULL;
	void *node=NULL;

	if (EBUSY==pthread_mutex_trylock( q->mutex )) {
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
			node = tmp->node;

			//DEBUG_LOG(LOG_DEBUG,"queue_get_nb: NODE PRESENT, q[%x] envlp[%x], freeing queue_node[%x]", q, node, tmp);
			free(tmp);

		}

	pthread_mutex_unlock( q->mutex );

	return node;
}//[/queue_get]

/**
 * Verifies if a message is present
 *
 * @return 1 if at least 1 message is present,
 * @return 0  if NONE
 * @return -1 on ERROR
 *
 */
int queue_peek(queue *q) {

	if (NULL==q) {
		DEBUG_LOG(LOG_DEBUG, "queue_peek: NULL queue ptr");
		return -1;
	}

	queue_node *tmp = NULL;
	int result = 0;

	pthread_mutex_lock( q->mutex );

		tmp = q->head;
		result = (tmp!=NULL);

	pthread_mutex_unlock( q->mutex );

	return result;
} // queue_peek
