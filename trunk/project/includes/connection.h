/**
 * @file   connection.h
 *
 * @date   2009-04-24
 * @author Jean-Lou Dupont
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


	// PRIVATE
	void _litm_connections_lock(void);
	void _litm_connections_unlock(void);

#endif /* CONNECTION_H_ */
