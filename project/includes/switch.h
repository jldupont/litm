/**
 * @file   switch.h
 *
 * @date   2009-04-25
 * @author Jean-Lou Dupont
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
	litm_code switch_send_shutdown(litm_connection *conn, litm_bus bus_id, void *msg, void (*cleaner)(void *msg));
	litm_code switch_release(litm_connection *conn, litm_envelope *envlp);

	void __switch_wait_shutdown(void);

#endif /* SWITCH_H_ */
