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
#include <sys/types.h>

litm_connection *conns[LITM_CONNECTION_MAX];

void printMessage(litm_code code, char *message, ...);
void create_connections(void);


int main(int argc, char **argv) {
	printf("#main: BEGIN\n");

	create_connections();

	printf("#main: END\n");
	return 0;
}


void create_connections(void) {

	printf("#create_connections: BEGIN\n");
	litm_code code=LITM_CODE_OK;

	int i;

	for (i=0;i<LITM_CONNECTION_MAX;i++) {
		code = litm_connect( &conns[i] );

		printMessage(code, "* Connection result: %s\n");
	}

	printf("#create_connections: END\n");
}//

void printMessage(litm_code code, char *message, ...) {

	char buffer[4096];
	char buffer2[8192];
	char *code_msg;

	va_list ap;

	code_msg = litm_translate_code(code);
	if (NULL==code_msg) {
		printf("!!! invalid code\n");
	}

	snprintf(buffer, sizeof(buffer), message, code_msg);

	va_start(ap, message);

		vsnprintf (buffer2, sizeof(buffer), buffer, ap);

	va_end(ap);

	printf( buffer2 );
}//
