/*
 * connection.h
 *
 *  Created on: 2009-04-24
 *      Author: Jean-Lou Dupont
 */

#ifndef CONNECTION_H_
#define CONNECTION_H_

#	define LITM_CONNECTION_MAX 15

	/**
	 * Opens (creates) a new connection
	 */
	litm_connection *litm_connection_open(void);

	/**
	 * Closes an opened connection
	 */
	void litm_connection_close(litm_connection *conn);



#endif /* CONNECTION_H_ */
