/**
 * @file   pool.h
 *
 * @date   2009-04-24
 * @author Jean-Lou Dupont
 */

#ifndef POOL_H_
#define POOL_H_


#	define LITM_POOL_SIZE 16 //max pool size


	/**
	 * Recycles an ``envelope``
	 *
	 *  NOTE: We only keep up to LITM_POOL_SIZE
	 *  ----- messages in the stack.
	 */
	void 			__litm_pool_recycle( litm_envelope *envlp );

	/**
	 * Gets an ``envelope`` from the pool
	 *  or creates a new one
	 */
	litm_envelope * __litm_pool_get(void);


	/**
	 * Destroys an ``envelope``
	 *
	 *  (probably because we have no more place
	 *  on the stack)
	 */
	void			__litm_pool_destroy( litm_envelope *envlp );


	/**
	 * Clean-up an ``envelope`` for reuse
	 */
	void			__litm_pool_clean( litm_envelope *envlp );


#endif /* POOL_H_ */
