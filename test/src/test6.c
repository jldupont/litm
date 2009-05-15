/*
 * test6.c
 *
 *  Created on: 2009-04-27
 *      Author: Jean-Lou Dupont
 *
 *
 *  Test for litm_connect_ex_wait, and litm_subscribe_wait
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
void *leader_threadFunction(void *params);
int my_random(void);
void void_cleaner(void *msg);


typedef struct _message {
	int code;
	char message[255];
} message;



int main(int argc, char **argv) {
	printf("#main: BEGIN\n");

	srand(time(NULL));

	create_connections();
	create_threads();

	printf("*** WAITING FOR THREAD EXIT ***\n");

	int i;
	for (i=0;i<LITM_CONNECTION_MAX;i++) {
		pthread_join( threads[i], NULL );
	}

	printf("#main: END\n");
	return 0;
}


void create_threads(void) {

	int i;
	thread_params tp;


	for (i=0;i<LITM_CONNECTION_MAX;i++) {

		tp.thread_id = i;
		tp.conn = conns[i];

		if (0==i)
			pthread_create( &threads[i], NULL, &leader_threadFunction, (void *) &tp );
		else
			pthread_create( &threads[i], NULL, &threadFunction, (void *) &tp );

		//printMessage2("* Thread started: %u \n", i);
	}


}//

void create_connections(void) {

	int i;
	litm_code code;

	for (i=0;i<LITM_CONNECTION_MAX;i++) {

		code = litm_connect_ex_wait( &conns[i], 100+i, 0 );

		//printMessage(code, "* CREATED CONNECTION, code[%s] ...","id[%u] conn[%x]\n", i, conns[i] );
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

void *leader_threadFunction(void *params) {

	thread_params *tp = (thread_params *) params;

	int thread_id, delivery=0, released=0;
	litm_connection *conn = NULL;
	litm_code code;
	int subscribed = 0;

	thread_id = tp->thread_id;
	conn      = tp->conn;

	// wait before sending shutdown message
	sleep(10);

	message msg;

	msg.code = 1;
	sprintf( msg.message, "shutdown from [%u]", thread_id );


	while(1) {
		printMessage(code, "* LEADER Sent, code[%s]...","msg[%x] thread_id[%u] conn[%x]\n", &msg.message, thread_id, conn );
		code = litm_send_shutdown( conn, 1, &msg, &void_cleaner );
		if (LITM_CODE_OK==code)
			break;
		sleep(1);
	}

	printMessage2("LEADER Thread [%u] INITIATING SHUTDOWN\n", thread_id);
	litm_wait_shutdown();
	printMessage2("LEADER Thread [%u] ENDING\n", thread_id);
}


void *threadFunction(void *params) {

	thread_params *tp = (thread_params *) params;

	int thread_id, delivery=0, released=0;
	litm_connection *conn = NULL;
	litm_code code;
	int subscribed = 0;

	thread_id = tp->thread_id;
	conn      = tp->conn;

	//printMessage2("Thread [%u] started, conn[%x]\n", thread_id, conn);

	code = litm_subscribe_wait(conn, 1, 0);
	if (LITM_CODE_OK!=code)
		printMessage(code, "* Subscribed, code[%s]...","thread_id[%u]\n", thread_id );

	if (LITM_CODE_OK==code) {
		subscribed = 1;
	}

	message _msg;

	_msg.code = 2; // no shutdown
	sprintf( &_msg.message, "message from [%u]", thread_id );

	int sd_flag;
	message *msg;
	litm_envelope *e;

	int my_received=0, my_released=0;

	int j;
	for (j=0;j<1000;j++) {
		code = litm_send_wait( conn, 1, &_msg, &void_cleaner, 0 );
		if (LITM_CODE_OK!=code)
			printMessage(code, "* Sent, code[%s]...","msg[%x] thread_id[%3u] conn[%x]\n", &_msg, thread_id, conn );
	}

	while (0==_exit_threads) {

		code = litm_receive_wait(conn, &e);
		if (LITM_CODE_OK==code) {
			my_received++;
			msg = e->msg;
			sd_flag = msg->code;

			//delivery = e->delivery_count;
			//released = e->released_count;
			//printf("* thread[%u] rx msg[%s], sd[%i] msg[%x] envlp[%x] delv[%u] rel[%u] \n", thread_id, msg->message, sd_flag, msg, e, delivery, released);

			if (1==sd_flag) {
				//printf("!!! Thread [%u] conn[%x][%i] queue num[%i]\n", thread_id, conn, conn->id, conn->input_queue->num);
			}

			//releasing is blocking
			litm_release(conn, e);
			my_released++;

			if (1==sd_flag)
				break;
		}

		//usleep(1*1000);
		//printf("Thread[%u] loop restart", thread_id);

	}//while

	if (1==subscribed) {
		code = litm_unsubscribe(conn, 1);
		//printMessage(code, "* Unsubscribed, code[%s]...","thread_id[%u]  RECEIVED[%i] \n", thread_id, received );
	} else {
		printf("Thread [%u] wasn't subscribed\n", thread_id);
	}
	queue *q = (conn->input_queue);
	int qnum  = q->num;
	printf("Thread [%3u] conn[%x][%3i] sent[%3i] received[%3i] my_received[%3i] my_released[%3i] qnum[%3i]\n", thread_id, conn, conn->id, conn->sent, conn->received,
			my_received, my_released, qnum);

	//printMessage2("Thread [%u] ENDING\n", thread_id);
}//

void void_cleaner(void *msg) {
	//printf("# void_cleaner: msg[%x]\n", msg);
}
