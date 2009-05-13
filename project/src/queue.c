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

void *__queue_get_safe(queue *q);


/**
 * Creates a queue
 */
queue *queue_create(void) {

	pthread_cond_t *cond = malloc( sizeof (pthread_cond_t) );
	if (NULL == cond) {
		return NULL;
	} else {
		pthread_cond_init( cond, NULL );
	}

	// if this malloc fails,
	//  there are much bigger problems that loom
	pthread_mutex_t *mutex = malloc( sizeof(pthread_mutex_t) );
	queue *q = malloc( sizeof(queue) );

	if ((NULL != q) && (NULL != mutex)){

		q->head  = NULL;
		q->tail  = NULL;

		pthread_mutex_init( mutex, NULL );
		q->mutex = mutex;
		q->cond  = cond;

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
	pthread_mutex_t *mutex = q->mutex;
	pthread_cond_t  *cond  = q->cond;

	pthread_mutex_lock( mutex );
		free(q);
		q=NULL;
	pthread_mutex_unlock( mutex );

	free(mutex);
	free(cond);

}//

/**
 * Queues a node at the HEAD (non-blocking)
 *
 * @return 1  => success
 * @return 0  => error
 * @return -1 => busy
 *
 */
	int
queue_put_head_nb(queue *q, void *node) {

	if ((NULL==q) || (NULL==node)) {
		DEBUG_LOG(LOG_DEBUG, "queue_put_head_nb: NULL queue/node ptr");
		return 0;
	}

	if (EBUSY == pthread_mutex_trylock( q->mutex ))
		return -1;

		int returnCode = queue_put_head_safe( q, node );

	pthread_mutex_unlock( q->mutex );

	return returnCode;
}//[/queue_put]

/**
 * Puts a node at the HEAD of the queue
 *
 * This function is meant to support _high priority_ messages.
 *
 * @param q      queue reference
 * @param node   message reference
 *
 * @return 1 => success
 * @return 0 => error
 *
 */
int   queue_put_head(queue *q, void *node) {

	if ((NULL==q) || (NULL==node)) {
		DEBUG_LOG(LOG_DEBUG, "queue_put_head: NULL queue/node ptr");
		return 0;
	}

	int code;

	pthread_mutex_lock( q->mutex );

		code = queue_put_head_safe( q, node );

	pthread_mutex_unlock( q->mutex );

	//DEBUG_LOG(LOG_DEBUG,"queue_put: END");

	return code;

}//

/**
 * Puts a node a the HEAD of the queue
 * without regards to thread-safety
 *
 */
	int
queue_put_head_safe( queue *q, void *msg ) {

	int code = 1;
	queue_node *tmp=NULL;

	// if this malloc fails,
	//  there are much bigger problems that loom
	tmp = (queue_node *) malloc(sizeof(queue_node));
	if (NULL!=tmp) {

		tmp->node = msg;
		tmp->next = NULL;

		// there is a head... put at the front
		if (NULL!=q->head) {
			tmp->next = q->head;
		}

		// adjust head
		q->head = tmp;

		// adjust tail
		if (NULL==q->tail)
			q->tail=tmp;

		// we were able to put an element:
		//  signal the waiting thread(s)
		pthread_cond_signal( q->cond );

	} else {

		code = 0;
	}

	return code;
}//


/**
 * Queues a node (blocking)
 *
 * @return 1 => success
 * @return 0 => error
 *
  */
int queue_put(queue *q, void *node) {

	if ((NULL==q) || (NULL==node)) {
		DEBUG_LOG(LOG_DEBUG, "queue_put: NULL queue/node ptr");
		return 0;
	}

	int code;

	//DEBUG_LOG(LOG_DEBUG,"queue_put: q[%x] node[%x] BEGIN",q, node);
	pthread_mutex_lock( q->mutex );

		code = queue_put_safe( q, node );

	pthread_mutex_unlock( q->mutex );

	//DEBUG_LOG(LOG_DEBUG,"queue_put: q[%x] node[%x] END",q,node);

	return code;
}//[/queue_put]



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

		DEBUG_LOG(LOG_DEBUG,"queue_put_nb: q[%x] node[%x] BEGIN",q, node);
		int returnCode = queue_put_safe( q, node );

	pthread_mutex_unlock( q->mutex );
	DEBUG_LOG(LOG_DEBUG,"queue_put_nb: q[%x] node[%x] END",q, node);

	return returnCode;
}//[/queue_put]


/**
 * Queue_put_safe
 *
 * Lock is not handled here - the caller must take
 * care of this.
 *
 * @return 0 => error
 * @return 1 => success
 *
 */
	int
queue_put_safe( queue *q, void *node ) {

	int code = 1;
	queue_node *new_node=NULL;

	// if this malloc fails,
	//  there are much bigger problems that loom
	new_node = (queue_node *) malloc(sizeof(queue_node));
	if (NULL!=new_node) {

		// new node...
		new_node->node = node;
		new_node->next = NULL;

		// there is a tail... put at the end
		if (NULL!=q->tail)
			(q->tail)->next=new_node;

		// point tail to the new element
		q->tail = new_node;

		// adjust head
		if (NULL==q->head)
			q->head=new_node;

		// we were able to put an element:
		//  signal the waiting thread(s)
		pthread_cond_signal( q->cond );

	} else {

		code = 0;
	}

	return code;
}//


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

	pthread_mutex_lock( q->mutex );

		void *node=NULL;
		node = __queue_get_safe(q);

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

	if (EBUSY==pthread_mutex_trylock( q->mutex )) {
		return NULL;
	}

		void *node=NULL;
		node = __queue_get_safe(q);

	pthread_mutex_unlock( q->mutex );

	return node;
}//[/queue_get]


/**
 * Waits for a node in the queue
 */
void *queue_get_wait(queue *q) {

	if (NULL==q) {
		DEBUG_LOG(LOG_DEBUG, "queue_get_wait: NULL queue ptr");
		return NULL;
	}

	pthread_mutex_lock( q->mutex );
	pthread_cond_wait( q->cond, q->mutex );

		void *node=NULL;
		node = __queue_get_safe(q);

	pthread_mutex_unlock( q->mutex );

	return node;
}//

/**
 * Waits for a node in the queue
 */
void *queue_get_wait2(queue *q) {

	if (NULL==q) {
		DEBUG_LOG(LOG_DEBUG, "queue_get_wait: NULL queue ptr");
		return NULL;
	}

	int result = pthread_mutex_trylock( q->mutex );
	if (EBUSY==result) {
		return NULL;
	}

	pthread_cond_wait( q->cond, q->mutex );

		void *node=NULL;
		node = __queue_get_safe(q);

	pthread_mutex_unlock( q->mutex );

	return node;
}//

void *__queue_get_safe(queue *q) {

	queue_node *tmp = NULL;
	void *node=NULL;

	tmp = q->head;
	if (tmp!=NULL) {

		// the queue contained at least one node
		node = tmp->node;

		// adjust tail: case if tail==head
		//  ie. only one element present
		if (q->head == q->tail) {
			q->tail = NULL;
			q->head = NULL;
		} else {
			// adjust head : next pointer is already set
			//  to NULL in queue_put
			q->head = (q->head)->next;
		}

		//DEBUG_LOG(LOG_DEBUG,"queue_get: MESSAGE PRESENT, freeing queue_node[%x]", tmp);
		free(tmp);
	}

	return node;
}//

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
