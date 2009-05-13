/**
 * @file   queue.h
 *
 * @date   2009-04-23
 * @author Jean-Lou Dupont
 *
 * \note   The definition of the types for this module
 * 		   are located in litm.h
 *
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "litm.h"


	// Prototypes
	// ==========
	queue *queue_create(int id);
	void   queue_destroy(queue *queue);

	int   queue_put_nb(queue *q, void *msg);
	int   queue_put(queue *q, void *msg);

	int   queue_put_head_nb(queue *q, void *node);
	int   queue_put_head(queue *q, void *msg);
	int   queue_put_head_safe( queue *q, void *node );

	void *queue_get(queue *q);
	void *queue_get_nb(queue *q);
	void *queue_get_wait(queue *q);

	int   queue_peek(queue *q);

	int queue_put_safe( queue *q, void *node );

#endif /* QUEUE_H_ */
