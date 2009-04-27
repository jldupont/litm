/*
 * litm.h
 *
 *  Created on: 2009-04-24
 *      Author: Jean-Lou Dupont
 */

#ifndef LITM_H_
#define LITM_H_

#	include <sys/types.h>
#	include <pthread.h>


#	define LITM_CONNECTION_MAX 15
#	define LITM_BUSSES_MAX     7

		// TYPES
		// =====
		//
		//

		typedef struct _queue_node {
			void *msg;
			struct _queue_node *next;
		} queue_node;


		typedef struct {
			pthread_mutex_t *mutex;
			queue_node *head, *tail;
		} queue;


		/**
		 * ``Bus`` identifier type
		 */
		typedef int litm_bus;

		/**
		 * ``Connection`` type
		 */
		typedef struct _litm_connection {
			queue *input_queue;
		} litm_connection;

		//typedef _litm_connection litm_connection;

		typedef struct {
			int				 pending;
			litm_bus         bus_id;
			litm_connection *sender;
			litm_connection *current;
		} __litm_routing;






		// Return Codes
		// ------------
		//
		typedef enum {
			LITM_CODE_OK,
			LITM_CODE_NO_MESSAGE,
			LITM_CODE_ERROR_MALLOC,
			LITM_CODE_ERROR_CONNECTION_OPEN,
			LITM_CODE_ERROR_BAD_CONNECTION,
			LITM_CODE_ERROR_NO_MORE_CONNECTIONS,
			LITM_CODE_ERROR_INVALID_BUS,
			LITM_CODE_ERROR_SWITCH_SENDER_EQUAL_CURRENT,
			LITM_CODE_ERROR_SWITCH_NO_CURRENT,
			LITM_CODE_ERROR_SWITCH_NEXT_NOT_FOUND,
			LITM_CODE_ERROR_OUTPUT_QUEUING,   				//can't send to the recipient
			LITM_CODE_BUSY_OUTPUT_QUEUE,
			LITM_CODE_ERROR_INVALID_ENVELOPE,

		} litm_code;


		/**
		 * ``Envelope`` structure for messages
		 *
		 * Contains the pointer to the message
		 *  as well as a ``routing`` structure
		 *  used to deliver the said message
		 *  to all bus which have at least
		 *  1 subscriber.
		 *
		 * The client (ie. receiver of the envelope)
		 *  must not tamper with the message (ie. msg)
		 *  as it is shared with other client(s).
		 *
		 */
		typedef struct _litm_envelope {

			// cleaning function callback
			void (*cleaner)(void *msg);

			// the routing information
			__litm_routing routes;

			// the sender's message
			void *msg;

		} litm_envelope;



	// API
	// ===
	//
		/**
		 * Opens a ``connection`` to the ``switch``
		 */
		litm_code litm_connect(litm_connection **conn);


		/**
		 * Disconnects from the ``switch``
		 */
		litm_code litm_disconnect(litm_connection *conn);


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
		 *
		 * If an error occurs, it is the responsibility of the sender
		 *  to dispose of the ``messages`` appropriately.  If the
		 *  operation went LITM_CODE_OK, then once all recipients
		 *  are finished with the message, the ``cleaner`` will be
		 *  called with the message pointer OR if ``cleaner`` is
		 *  NULL, the message will simply be freed with free().
		 *
		 */
		litm_code litm_send(	litm_connection *conn,
								litm_bus bus_id,
								void *msg,
								void (*cleaner)(void *msg)
								);


		/**
		 * Receives (non-blocking) from any ``bus``
		 */
		litm_code litm_receive_nb(litm_connection *conn, litm_envelope **envlp);


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





#endif /* LITM_H_ */
