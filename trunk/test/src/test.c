/*
 * test.c
 *
 *  Created on: 2009-04-27
 *      Author: Jean-Lou Dupont
 */

#include <litm.h>

litm_connection *conns[LITM_CONNECTION_MAX];

int main(int argc, char *argv) {

	create_connections();


	return 0;
}


void create_connections(void) {
	int i;
	for (i=0;i<LITM_CONNECTION_MAX;i++) {
		litm_connect( &conns[i] );
	}
}
