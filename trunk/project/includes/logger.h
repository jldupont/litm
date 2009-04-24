/*
 * logger.h
 *
 *  Created on: 2009-04-21
 *      Author: Jean-Lou Dupont
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <syslog.h>


#ifdef _DEBUG
# define DEBUG_MSG(...) printf(__VA_ARGS__)
# define DEBUG_LOG(...) doLog(__VA_ARGS__)
# define DEBUG_LOG_NULL_PTR(ptr, ...) if (NULL==ptr) doLog(__VA_ARGS__)
#else
# define DEBUG_MSG(...)
# define DEBUG_LOG(...)
# define DEBUG_LOG_NULL_PTR(ptr, ...)
#endif

	// Prototypes
	// ==========
	void doLog(int priority, char *message, ...);



#endif /* LOGGER_H_ */
