/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define err_quit(m) { perror(m); exit(-1); }

#define NIPQUAD(s)	((unsigned char *) &s)[0], \
					((unsigned char *) &s)[1], \
					((unsigned char *) &s)[2], \
					((unsigned char *) &s)[3]

static int s = -1;
static struct sockaddr_in sin;
static unsigned seq;
static unsigned count = 0;

typedef struct {
	unsigned seq;
	struct timeval tv;
}	ping_t;

double tv2ms(struct timeval *tv) {
	return 1000.0*tv->tv_sec + 0.001*tv->tv_usec;
}

void do_send(int sig) {
	unsigned char buf[1024];
	if(sig == SIGALRM) {
		ping_t *p = (ping_t*) buf;
		p->seq = seq++;
		gettimeofday(&p->tv, NULL);
		if(sendto(s, p, sizeof(*p)+16, 0, (struct sockaddr*) &sin, sizeof(sin)) < 0)
			perror("sendto");
		alarm(1);
	}
	count++;
	if(count > 10) exit(0);
}

int main(int argc, char *argv[]) {
	if(argc < 3) {
		return -fprintf(stderr, "usage: %s ... <port> <ip>\n", argv[0]);
	}

	srand(time(0) ^ getpid());

	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(strtol(argv[argc-2], NULL, 0));
	if(inet_pton(AF_INET, argv[argc-1], &sin.sin_addr) != 1) {
		return -fprintf(stderr, "** cannot convert IPv4 address for %s\n", argv[1]);
	}

	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		err_quit("socket");

	signal(SIGALRM, do_send);

	seq = rand() % 0xffffff;
	fprintf(stderr, "PING %u.%u.%u.%u/%u, init seq = %d\n",
		NIPQUAD(sin.sin_addr), ntohs(sin.sin_port), seq);

	do_send(SIGALRM);

	while(1) {
		int rlen;
		struct sockaddr_in csin;
		socklen_t csinlen = sizeof(csin);
		char buf[2048];
		struct timeval tv;
		ping_t *p = (ping_t *) buf;
		
		if((rlen = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*) &csin, &csinlen)) < 0) {
			perror("recvfrom");
			continue;
		}

		gettimeofday(&tv, NULL);
		printf("%lu.%06lu %d bytes from %u.%u.%u.%u/%u: seq=%u, time=%.6f ms\n",
				p->tv.tv_sec, p->tv.tv_usec, rlen,
				NIPQUAD(csin.sin_addr), ntohs(csin.sin_port),
				p->seq, tv2ms(&tv)-tv2ms(&p->tv));
	}

	close(s);
}
