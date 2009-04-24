/*
 * queue.h
 *
 *  Created on: 2009-04-23
 *      Author: Jean-Lou Dupont
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>


	// Prototypes
	// ==========
	queue *queue_create(void);
	void   queue_destroy(queue *queue);

	int   queue_put(queue *q, void *msg);
	void *queue_get(queue *q);
	int   queue_peek(queue *q);


#endif /* QUEUE_H_ */
