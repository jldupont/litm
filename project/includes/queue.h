/**
 * @file   queue.h
 *
 * @date   2009-04-23
 * @author Jean-Lou Dupont
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "litm.h"


	// Prototypes
	// ==========
	queue *queue_create(void);
	void   queue_destroy(queue *queue);

	int   queue_put_nb(queue *q, void *msg);
	int   queue_put(queue *q, void *msg);
	void *queue_get(queue *q);
	void *queue_get_nb(queue *q);
	int   queue_peek(queue *q);


#endif /* QUEUE_H_ */
