/*
 * litm.h
 *
 *  Created on: 2009-04-24
 *      Author: Jean-Lou Dupont
 */

#ifndef LITM_H_
#define LITM_H_


#	include <pthread.h>


	// API
	// ===
	//
		/**
		 * Opens a ``connection`` to the ``switch``
		 */
		litm_code litm_connect(litm_connection *conn);


		/**
		 * Disconnects from the ``switch``
		 */
		litm_code litm_disconnect(void);


		/**
		 * Subscribe to a ``bus``
		 */
		litm_code litm_subscribe(litm_connection *conn, litm_bus bus_id);


		/**
		 * Unsubscribe from a ``bus``
		 */
		litm_code litm_unsubscribe(litm_connection *conn, litm_bus bus_id);


		/**
		 * Send message on a ``bus``
		 */
		litm_code litm_send(litm_connection *conn, litm_bus bus_id, void *msg);


		/**
		 * Receives (blocking) from any ``bus``
		 */
		litm_code litm_receive(litm_connection *conn, litm_envelope *envlp);


		/**
		 * Receives (non-blocking) from any ``bus``
		 */
		litm_code litm_receive_nb(litm_connection *conn, litm_envelope *envlp);


		/**
		 * Releases an ``envelope``
		 *
		 *  Once the client has consumed the message
		 *  ``contained`` in the envelope, the client
		 *  **MUST** release it back to litm.
		 */
		litm_code litm_release(litm_connection *conn, litm_envelope *envlp);


		/**
		 * Returns a copy of the pointer to the
		 *  message.
		 *
		 * This function should be used instead
		 *  of relying on the ``litm_envelope``
		 *  data structure.
		 *
		 *  ** DO NOT** modify the message!
		 */
		void *litm_get_message(litm_envelope *envlp);


	// TYPES
	// =====
	//
		/**
		 * Return Code type
		 */
		typedef int litm_code;

		/**
		 * ``Bus`` identifier type
		 */
		typedef int litm_bus;

		/**
		 * ``Connection`` type
		 */
		typedef struct {


		} litm_connection;


		/**
		 * The client (ie. receiver of the envelope)
		 *  must not tamper with the message (ie. msg)
		 *  as it is shared with other client(s).
		 */
		struct __litm_routing; //forward declaration

		/**
		 * ``Envelope`` structure for messages
		 *
		 * Contains the pointer to the message
		 *  as well as a ``routing`` structure
		 *  used to deliver the said message
		 *  to all bus which have at least
		 *  1 subscriber.
		 */
		typedef struct _litm_envelope {

			// the routing information
			__litm_routing routes;

			// the sender's message
			void *msg;

		} litm_envelope;


	//
	// PRIVATE
	// =======
		typedef struct {

		} __litm_routing;


#endif /* LITM_H_ */