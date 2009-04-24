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

#include "../includes/litm.h"
#include "../includes/pool.h"

#ifndef DEBUG_LOG
#	define DEBUG_LOG(...)
#	define DEBUG_LOG_PTR(ptr, ...)
#endif


	// PRIVATE //
	// ======= //
	litm_envelope _stack[LITM_POOL_SIZE];   //LIFO stack

	int _top_stack = 0;
	int _created   = 0;
	int _recycled  = 0;
	int _returned  = 0;


	void
__litm_pool_recycle( litm_envelope *envlp ) {

	//can we recycle this one?
	if ( _top_stack + 1 == LITM_POOL_SIZE ) {
		return;
	}

	_stack[_top_stack++] = envlp;

	// statistics
	_recycled ++;

}//




	litm_envelope *
__litm_pool_get(void) {

	litm_envelope *e;

	// do we have a spare?
	e = _stack[ _top_of_stack ];
	if (NULL!=e) {

		_stack[_top_of_stack] = NULL;
		__litm_pool_clean( e );

		// statistics...
		_returned ++;

		if (0!=_top_of_stack)
			_top_of_stack-- ;

	} else {

		// prepare a new one
		e = malloc(sizeof(litm_envelope));
		if (NULL!=e)
			bzero(e, sizeof(litm_envelope));

		// statistics...
		_created ++ ;
	}

	return e;
}//


	void
__litm_pool_destroy( litm_envelope *envlp ) {

	DEBUG_LOG_PTR(envlp, "ERROR: __litm_pool_destroy: NULL pointer");

	// First, destroy the message
	free( envlp-> msg );
	free( envlp );

}//



	void
__litm_pool_clean( litm_envelope *envlp ) {

	DEBUG_LOG_PTR(envlp, "ERROR: __litm_pool_clean: NULL pointer");

	// clear routing information
	bzero( envlp->routes, sizeof(__litm_routing) );

}//