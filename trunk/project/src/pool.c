/*
 * pool.c
 *
 *  Created on: 2009-04-24
 *      Author: Jean-Lou Dupont
 *
 * Handles pooling / recycling of ``envelopes``
 *
 *  FOR MORE INFORMATION, CONSULT   ``pool.h``
 *
 *
 * NOTE: This module assumes it is executing in a thread-safe environment.
 * ----
 *
 */

#include <string.h>
#include <stdlib.h>

#include "litm.h"
#include "pool.h"
#include "logger.h"

	// PRIVATE //
	// ======= //
	litm_envelope * _stack[LITM_POOL_SIZE];   //LIFO stack

	int _top_stack = 0;
	int _created   = 0;
	int _recycled  = 0;
	int _returned  = 0;


	void
__litm_pool_recycle( litm_envelope *envlp ) {

	//can we recycle this one?
	if ( _top_stack + 1 == LITM_POOL_SIZE ) {
		DEBUG_LOG(LOG_DEBUG, "__litm_pool_recycle: destroying envelope [%x]", envlp );
		__litm_pool_destroy( envlp );
		return;
	}

	DEBUG_LOG(LOG_DEBUG, "__litm_pool_recycle: recycling envelope [%x]", envlp );
	_stack[_top_stack++] = envlp;

	// statistics
	_recycled ++;

}//




	litm_envelope *
__litm_pool_get(void) {

	litm_envelope *e;

	// do we have a spare?
	e = _stack[ _top_stack ];
	if (NULL!=e) {

		_stack[_top_stack] = NULL;
		__litm_pool_clean( e );

		// statistics...
		_returned ++;

		DEBUG_LOG(LOG_DEBUG, "__litm_pool_get: returning recycled envelope [%x]", e );

		if (0!=_top_stack)
			_top_stack-- ;

	} else {

		// prepare a new one
		e = malloc(sizeof(litm_envelope));
		if (NULL!=e) {
			bzero(e, sizeof(litm_envelope));

			// statistics...
			_created ++ ;

			DEBUG_LOG(LOG_DEBUG, "__litm_pool_get: returning new envelope [%x]", e );
		}

	}

	return e;
}//


	void
__litm_pool_destroy( litm_envelope *envlp ) {
	DEBUG_LOG_PTR( envlp, LOG_ERR, "__litm_pool_destroy: NULL");

	DEBUG_LOG(LOG_DEBUG, "__litm_pool_destroy: envelope [%x]", envlp );

	// First, destroy the message
	//free( envlp-> msg );
	free( envlp );

}//



	void
__litm_pool_clean( litm_envelope *envlp ) {
	DEBUG_LOG_PTR( envlp, LOG_ERR, "__litm_pool_clean: NULL");

	// clear routing information
	bzero( (char *) &envlp->routes, sizeof(__litm_routing) );

}//
