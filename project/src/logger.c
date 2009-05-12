/**
 * @file logger.c
 *
 * @date   2009-04-21
 * @author Jean-Lou Dupont
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "logger.h"

char *_LOGGER_IDENTITY = "litm";

/**
 * Crude logging function
 */
void doLog(int priority, char *message, ...) {

	openlog(_LOGGER_IDENTITY, LOG_PID, LOG_LOCAL1);

	va_list ap;

	va_start(ap, message);
		vsyslog( priority, message, ap);
	va_end(ap);

	closelog();
}
