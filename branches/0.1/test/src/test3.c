/*
 * test3.c
 *
 *  Created on: 2009-04-27
 *      Author: Jean-Lou Dupont
 *
 *
 *  Subscription / Unsubscription Test
 *
 */

#include <litm.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <time.h>

typedef struct {

	int thread_id;
	litm_connection *conn;

} thread_params;

pthread_t threads[LITM_CONNECTION_MAX];
litm_connection *conns[LITM_CONNECTION_MAX];

volatile int _exit_threads = 0;


void printMessage(litm_code code, char *header, char *message, ...);
void printMessage2(char *message, ...);
void create_threads(void);
void create_connections(void);
void *threadFunction(void *params);
int my_random(void);
void void_cleaner(void *msg);


int main(int argc, char **argv) {
	printf("#main: BEGIN\n");

	srand(time(NULL));

	create_connections();
	create_threads();

	sleep(20);

	printf("*** ASKING FOR THREAD EXIT ***\n");
	_exit_threads = 1;

	sleep(15);

	printf("*** ASKING FOR LITM SHUTDOWN ***\n");
	litm_shutdown();
	sleep(2);

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

		//printMessage2("* Thread started: %u \n", i);
	}


}//

void create_connections(void) {

	int i;
	litm_code code;

	for (i=0;i<LITM_CONNECTION_MAX;i++) {

		code = litm_connect( &conns[i] );

		printMessage(code, "* CREATED CONNECTION, code[%s] ...","id[%u] conn[%x]\n", i, conns[i] );
	}

}

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


	int
my_random(void) {

	double r;

	r = ((double)rand()/(double)RAND_MAX);

	return (int) r;
}

// ===================================================================
// ===================================================================



void *threadFunction(void *params) {

	thread_params *tp = (thread_params *) params;

	int thread_id, delivery=0, released=0;
	litm_connection *conn = NULL;
	litm_code code;
	int subscribed = 0;

	thread_id = tp->thread_id;
	conn      = tp->conn;

	//printMessage2("Thread [%u] started, conn[%x]\n", thread_id, conn);

	code = litm_subscribe(conn, 1);
	printMessage(code, "* Subscribed, code[%s]...","thread_id[%u]\n", thread_id );
	if (LITM_CODE_OK==code)
		subscribed = 1;

	char message[255];
	sprintf( message, "message from [%u]", thread_id );

	char *msg;
	litm_envelope *e;

	while(1) {
		code = litm_send( conn, 1, message, &void_cleaner );
		printMessage(code, "* Sent, code[%s]...","msg[%x] thread_id[%u] conn[%x]\n", message, thread_id, conn );
		if (LITM_CODE_OK==code)
			break;
		sleep(1);
	}

	while (0==_exit_threads) {

		code = litm_receive_nb(conn, &e);
		if (LITM_CODE_OK==code) {
			msg = e->msg;
			delivery = e->delivery_count;
			released = e->released_count;
			printf("* thread[%u] received message[%s], msg[%x] envelope[%x] delivery[%u] released[%u] \n", thread_id, msg, msg, e, delivery, released);
			litm_release(conn, e);
		}


		//sleep(0.1);
		//printf("Thread[%u] loop restart", thread_id);

	}//while

	if (1==subscribed) {
		code = litm_unsubscribe(conn, 1);
		printMessage(code, "* Unsubscribed, code[%s]...","thread_id[%u]\n", thread_id );
	} else {
		printf("Thread [%u] wasn't subscribed\n", thread_id);
	}


	printMessage2("Thread [%u] ENDING\n", thread_id);
}//

void void_cleaner(void *msg) {
	//printf("# void_cleaner: msg[%x]\n", msg);
}
