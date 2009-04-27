/*
 * switch.h
 *
 *  Created on: 2009-04-25
 *      Author: Jean-Lou Dupont
 */

#ifndef SWITCH_H_
#define SWITCH_H_

#include <pthread.h>

	// TYPES
	// -----

	typedef struct _litm_bus_subscription_map {
		litm_connection *subscribers[LITM_CONNECTION_MAX];
	} litm_bus_subscription_map;


	// PROTOTYPES
	int   switch_init(void);
	void *switch_thread_function(void *params);



#endif /* SWITCH_H_ */
