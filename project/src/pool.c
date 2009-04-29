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

#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "litm.h"
#include "pool.h"
#include "logger.h"

	// PRIVATE //
	// ======= //
	litm_envelope * _stack[LITM_POOL_SIZE];   //LIFO stack

	int _stack_initialized = 0; //FALSE
	int _top_stack = 0;
	int _created   = 0;
	int _recycled  = 0;
	int _returned  = 0;

	pthread_mutex_t  _pool_mutex = PTHREAD_MUTEX_INITIALIZER;

	void
__litm_pool_recycle( litm_envelope *envlp ) {

	pthread_mutex_lock( &_pool_mutex );

	//can we recycle this one?
	if ( _top_stack + 1 == LITM_POOL_SIZE ) {
		DEBUG_LOG(LOG_DEBUG, "__litm_pool_recycle: destroying envelope [%x]", envlp );
		__litm_pool_destroy( envlp );

	} else {
		DEBUG_LOG(LOG_DEBUG, "__litm_pool_recycle: recycling envelope [%x]", envlp );
		_top_stack ++;
		_stack[_top_stack] = envlp;

		// statistics
		_recycled ++;
	}

	pthread_mutex_unlock( &_pool_mutex );

}//




	litm_envelope *
__litm_pool_get(void) {

	pthread_mutex_lock( &_pool_mutex );


	// first time around?
	if (0==_stack_initialized) {
		_stack_initialized = 1;

		int i;
		for(i=0;i<LITM_POOL_SIZE;i++)
			_stack[i] = NULL;
	}//==============================

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

			// prepare the envelope
			__litm_pool_clean( e );

			// statistics...
			_created ++ ;

			DEBUG_LOG(LOG_DEBUG, "__litm_pool_get: returning new envelope [%x]", e );
		}

	}

	pthread_mutex_unlock( &_pool_mutex );
	return e;
}//


	void
__litm_pool_destroy( litm_envelope *envlp ) {

	if (NULL==envlp) {
		DEBUG_LOG( LOG_ERR, "__litm_pool_destroy: NULL envelope");
		return;
	}
	DEBUG_LOG(LOG_DEBUG, "__litm_pool_destroy: envelope [%x]", envlp );

	free( envlp );

}//



	void
__litm_pool_clean( litm_envelope *envlp ) {
	DEBUG_LOG_PTR( envlp, LOG_ERR, "__litm_pool_clean: NULL");

	//DEBUG_LOG(LOG_DEBUG, "__litm_pool_clean: envelope [%x]", envlp );

	(envlp->routes).bus_id  = 0;
	(envlp->routes).current = NULL;
	(envlp->routes).sender  = NULL;
	(envlp->routes).pending = 0;

	envlp->cleaner = NULL;
	envlp->msg     = NULL;

	envlp->delivery_count = 0;
	envlp->released_count = 0;
}//
