/*
 * connection.h
 *
 *  Created on: 2009-04-24
 *      Author: Jean-Lou Dupont
 */

#ifndef CONNECTION_H_
#define CONNECTION_H_


	/**
	 * Opens (creates) a new connection
	 */
	litm_code litm_connection_open(litm_connection **conn);

	/**
	 * Closes an opened connection
	 */
	litm_code litm_connection_close(litm_connection *conn);


	/**
	 * Returns the connection for a specified index
	 *  or NULL on error
	 */
	litm_connection *litm_connection_get_ptr(int connection_index);


#endif /* CONNECTION_H_ */
