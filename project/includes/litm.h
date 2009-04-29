/**
 * @file   litm.h
 *
 * @date   2009-04-24
 * @author Jean-Lou Dupont
 *
 * @mainpage Welcome to the documentation for the library LITM
 *
 *           The library serves as a <b>Lightweight Inter-Thread Messaging</b> under Linux/Unix.
 *
 * \section Dependencies
 *
 *			<b>Mandatory</b>

 *          - libpthread
 *
 *          <b>Optional</b>
 *          The following components are only required if building from source:
 *
 *          - scons
 *
 *          - python package "pyjld.os" (available through <i>easy_install</i>)
 *
 *
 * \section	Example_Usage Example Usage:
 *
 *			\subsection Example_Opening Opening a connection
 *
 *			\code
 *
 *				litm_connection *conn;
 *				litm_code code = litm_connection_open( &conn );
 *				switch(code) {
 *				 case LITM_CODE_OK:
 *				 		// can start using the connection
 *				 		break;
 *				 case LITM_CODE_BUSY:
 *				 		// better try again later
 *				 		break;
 *				 default:
 *				 		//something is wrong...
 *				 		break;
 *				}
 *
 *			\endcode
 *
 *
 *			\subsection Example_Subscribing Subscribing to a _bus_
 *
 *				\code
 *				// subscribing to bus #1
 *				litm_code code = litm_subscribe(conn, 1);
 *				  ... check return code ...
 *
 *				\endcode
 *
 *
 *			\subsection Example_Sending Sending a message on a _bus_
 *
 *				\code
 *					void *message; // whatever structure
 *					litm_code code = litm_send(conn, bus_id, message, NULL);
 *					 ... check return code ...
 *				\endcode
 *
 *
 *			\subsection Example_Receiving Receiving a message
 *
 *				\code
 *					litm_envelope *envelope;
 *					litm_code code = litm_receive_nb(conn, &envelope);
 *					 ... check return code ...
 *
 *					void *msg;
 *					msg = litm_get_message( envelope );
 *
 *					 ... process message WITHOUT MODIFYING IT ...
 *
 *					// return the envelope to LITM
 *					code = litm_release(conn, envelope);
 *
 *				\endcode
 *
 * \todo Better shutdown system
 * \todo Better connection close
 *
 */

#ifndef LITM_H_
#define LITM_H_

#	include <sys/types.h>
#	include <pthread.h>


#	define LITM_CONNECTION_MAX 15
#	define LITM_BUSSES_MAX     7

		/**
		 * Queue node - entry in a queue
		 *
		 * @param msg:  pointer to the message
		 * @param next: pointer to the ``next`` queue_node entry
		 */
		typedef struct _queue_node {
			void *node;
			struct _queue_node *next;
		} queue_node;

		/**
		 * Queue - thread-safe
		 *
		 * @param mutex: mutex
		 * @param head:  pointer to ``head``
		 * @param tail:  pointer to ``tail``
		 */
		typedef struct {
			pthread_mutex_t *mutex;
			struct _queue_node *head, *tail;
		} queue;


		/**
		 * ``Bus`` identifier type
		 */
		typedef int litm_bus;

		/**
		 * ``Connection Status`` type
		 */
		typedef enum _litm_connection_status_codes {
			LITM_CONNECTION_STATUS_INVALID = 0,
			LITM_CONNECTION_STATUS_ACTIVE,
			LITM_CONNECTION_STATUS_PENDING_DELETION
		} litm_connection_status;

		/**
		 * ``Connection`` type
		 *
		 * @param input_queue the connection's input queue
		 */
		typedef struct _litm_connection {
			litm_connection_status status;
			queue *input_queue;
		} litm_connection;

		//typedef _litm_connection litm_connection;

		/**
		 * Envelope Routing
		 *
		 * @param pending Pending Status Flag
		 * @param bus_id  Destination ``bus``
		 * @param sender  The sender's connection pointer
		 * @param current The current recipient's connection pointer
		 */
		typedef struct {
			int				 pending;
			litm_bus         bus_id;
			litm_connection *sender;
			litm_connection *current;
		} __litm_routing;




		// Return Codes
		// ------------
		//
		typedef enum _litm_codes {
			LITM_CODE_OK = 0,
			LITM_CODE_NO_MESSAGE,
			LITM_CODE_BUSY,
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
			LITM_CODE_ERROR_SUBSCRIPTION_NOT_FOUND,
			LITM_CODE_ERROR_BUS_FULL,
			LITM_CODE_BUSY_CONNECTIONS,
			LITM_CODE_ERROR_CONNECTION_NOT_ACTIVE

		} litm_code;


		/**
		 * ``Envelope`` structure for messages
		 *
		 * @param cleaner The ``cleaner`` function to use
		 * @param routes  The ``routing`` structure
		 * @param msg     The pointer to the message
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

			int released_count;
			int delivery_count;
			void (*cleaner)(void *msg);
			__litm_routing routes;
			void *msg;

		} litm_envelope;


		/**
		 * Opens a ``connection`` to the ``switch``
		 *  and returns a pointer to the connection reference
		 *
		 * @param **conn pointer to connection reference
		 */
		litm_code litm_connect(litm_connection **conn);


		/**
		 * Disconnects from the ``switch``
		 *
		 * Once a client calls this function, no more access
		 * with the connection is permitted to any of the
		 * following functions:
		 *
		 *  - litm_subscribe
		 *  - litm_unsubscribe
		 *  - litm_receive_nb
		 *  - litm_send
		 *  - litm_release
		 *
		 * Be sure to release any message in the client's
		 * care before disconnecting.
		 *
		 * @param *conn connection reference
		 */
		litm_code litm_disconnect(litm_connection *conn);


		/**
		 * Subscribe to a ``bus``
		 *
		 * @see litm_disconnect
		 *
		 * @param *conn connection reference
		 * @param bus_id the ``bus`` identifier to subscribe to
		 *
		 */
		litm_code litm_subscribe(litm_connection *conn, litm_bus bus_id);


		/**
		 * Unsubscribe from a ``bus``
		 *
		 * @see litm_disconnect
		 *
		 * @param *conn connection reference
		 * @param bus_id the ``bus`` identifier to unsubscribe from
		 */
		litm_code litm_unsubscribe(litm_connection *conn, litm_bus bus_id);


		/**
		 * Send message on a ``bus``
		 *
		 * @see litm_disconnect
		 *
		 * @param *conn connection reference
		 * @param bus_id the ``bus`` to send the message onto
		 * @param *msg the pointer to the message
		 * @param *cleaner the pointer to the cleaner function
		 *
		 * If omitted, the default ``cleaner`` function will be
		 * <b>free()</b>.
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
		 *
		 * @see litm_disconnect
		 *
		 * @param *conn connection reference
		 * @param **envlp the pointer to received envelope
		 *
		 */
		litm_code litm_receive_nb(litm_connection *conn, litm_envelope **envlp);


		/**
		 * Releases an ``envelope``
		 *
		 * @see litm_disconnect
		 *
		 * @param *conn connection reference
		 * @param *envlp the pointer to received envelope to release
		 *
		 *  Once the client has consumed the message
		 *  ``contained`` in the envelope, the client
		 *  **MUST** release it back to litm.
		 */
		litm_code litm_release(litm_connection *conn, litm_envelope *envlp);


		/**
		 * Shutdown
		 *
		 * This function attempts to shutdown all the connection
		 * threads in a graceful manner.  This function is blocking.
		 */
		void litm_shutdown(void);


		/**
		 * Returns a copy of the pointer to the message.
		 *
		 * @param *envlp the envelope from which to extract the pointer to the message
		 *
		 * This function should be used instead
		 *  of relying on the ``litm_envelope``
		 *  data structure.
		 *
		 *  <b>** DO NOT** modify the message!</b>
		 *
		 */
		void *litm_get_message(litm_envelope *envlp);


		/**
		 * Translates a code to a message pointer
		 *
		 * @param code a <i>litm_code</i> to translate to a string version
		 *
		 */
		char *litm_translate_code(litm_code code);


#endif /* LITM_H_ */
