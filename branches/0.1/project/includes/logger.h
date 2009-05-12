/**
 * @file   logger.h
 *
 * @date   2009-04-21
 * @author Jean-Lou Dupont
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <stdlib.h>
#include <sys/types.h>
#include <syslog.h>

#ifdef _DEBUG
#	define DEBUG_LOG(...) doLog( __VA_ARGS__ )
#	define DEBUG_LOG_PTR(ptr, ...) if (NULL==ptr) doLog( __VA_ARGS__ )
#else
#	define DEBUG_LOG(...)
#	define DEBUG_LOG_PTR(...)
#endif

	// Prototypes
	// ==========
	void doLog(int priority, char *message, ...);


#endif /* LOGGER_H_ */
