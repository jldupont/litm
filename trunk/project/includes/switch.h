/*
 * switch.h
 *
 *  Created on: 2009-04-25
 *      Author: Jean-Lou Dupont
 */

#ifndef SWITCH_H_
#define SWITCH_H_

#include <pthread.h>


	// PROTOTYPES
	int   switch_init(void);
	void *switch_thread_function(void *params);

	litm_code switch_add_subscriber(litm_connection *conn, litm_bus bus_id);
	litm_code switch_remove_subscriber(litm_connection *conn, litm_bus bus_id);
	litm_code switch_send(litm_connection *conn, litm_bus bus_id, void *msg, void (*cleaner)(void *msg));
	litm_code switch_release(litm_connection *conn, litm_envelope *envlp);

#endif /* SWITCH_H_ */
