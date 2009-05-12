/*
 * test.c
 *
 *  Created on: 2009-04-27
 *      Author: Jean-Lou Dupont
 *
 *
 *
 *  Simple Test
 *
 *
 */

#include <litm.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>

typedef struct {

	int thread_id;
	litm_connection *conn;

} thread_params;

pthread_t threads[LITM_CONNECTION_MAX];
litm_connection *conns[LITM_CONNECTION_MAX];

void printMessage(litm_code code, char *header, char *message, ...);
void printMessage2(char *message, ...);
void create_threads(void);
void create_connections(void);
void *threadFunction(void *params);


int main(int argc, char **argv) {
	printf("#main: BEGIN\n");

	create_connections();
	create_threads();

	sleep(5);

	printf("#main: END\n");
	return 0;
}


void create_threads(void) {

	int i;
	thread_params tp;


	for (i=0;i<LITM_CONNECTION_MAX;i++) {

		tp.thread_id = i;
		tp.conn = conns[i];

		pthread_create( &threads[i], NULL, &threadFunction, (void *) &tp );

		printMessage2("* Thread started: %u \n", i);
	}


}//

void create_connections(void) {

	int i;
	litm_code code;

	for (i=0;i<LITM_CONNECTION_MAX;i++) {

		code = litm_connect( &conns[i] );

		printMessage(code, "* CREATED CONNECTION, code[%s]...","id[%u]\n", i );
	}

}

void *threadFunction(void *params) {

	thread_params *tp = (thread_params *) params;

	int thread_id;
	litm_connection *conn = NULL;
	litm_code code;

	thread_id = tp->thread_id;
	conn      = tp->conn;

	printMessage2("Thread [%u] started\n", thread_id);

	sleep(5);

	printMessage2("Thread [%u] ENDING\n", thread_id);
}//

void printMessage(litm_code code, char *header, char *message, ...) {

	char buffer[4096];
	char buffer2[8192];
	char *code_msg;

	va_list ap;

	code_msg = litm_translate_code(code);
	if (NULL==code_msg) {
		printf("!!! invalid code... [%u]...", code);
	}

	snprintf(buffer, sizeof(buffer), header, code_msg);

	va_start(ap, message);

		vsnprintf (buffer2, sizeof(buffer2), message, ap);

	va_end(ap);

	printf( buffer );
	printf( buffer2 );
}//

void printMessage2(char *message, ...) {

	char buffer2[8192];
	char *code_msg;

	va_list ap;

	va_start(ap, message);

		vsnprintf (buffer2, sizeof(buffer2), message , ap);

	va_end(ap);

	printf( buffer2 );
}//
