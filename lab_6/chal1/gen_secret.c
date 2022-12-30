/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>


int	/* a valid key always >= 0 */
msg2key(char *msg, int len) {
	int i, n = len/4 + ((len%4) ? 1 : 0);
	unsigned int *v, key = 0;
	if(n == 0) return -1;
	for(i = 0, v = (unsigned int*) msg; i < n; i++, v++)
		key ^= *v;
	return key & 0x7fffffff;
}

int
main() {
	char* msg = "AAA";
	int key = msg2key(msg, strlen(msg));

	printf("key = %d", key);
	
	return 0;
}