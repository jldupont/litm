/*
 * test.c
 *
 *  Created on: 2009-04-27
 *      Author: Jean-Lou Dupont
 */

#include <litm.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

litm_connection *conns[LITM_CONNECTION_MAX];

void printMessage(litm_code code, char *message, ...);
void create_connections(void);


int main(int argc, char *argv) {

	create_connections();


	return 0;
}


void create_connections(void) {

	litm_code code;
	int i;

	for (i=0;i<LITM_CONNECTION_MAX;i++) {
		code = litm_connect( &conns[i] );

		printMessage(code, NULL);
	}
}//

void printMessage(litm_code code, char *message, ...) {

	char buffer[2048];
	va_list ap;

	va_start(ap, message);
		vsnprintf (buffer, sizeof(buffer), message, ap);
	va_end(ap);


}//
