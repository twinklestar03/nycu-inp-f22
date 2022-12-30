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

// the data structure for the oracles
typedef struct {
	int stage;
	int key;
	char *secret;
}	oracle_t;

// the task to perform by the oracles in each stage
typedef struct {
	const char *fmt;
	void (*task)(oracle_t *);
}	stage_t;

static char *username = NULL;
static int maxstage = 1;
static int oseed = -1;
static unsigned magic = 0xdeadbeef;
static oracle_t oracle[2];

char *
gen_secret(int k1, int k2) {
	static int len;
	static char buf[64];
	if(oseed < 0) {
		oseed = (magic ^ k1 ^ k2) & 0x7fffffff;
		printf("oseed = %d", oseed);
		srand(oseed);
	}
	printf("** Thank you for your message, let me generate a secret for you.\n");
	len  = snprintf(buf,     10, "%x", rand());
	len += snprintf(buf+len, 10, "%x", rand());
	len += snprintf(buf+len, 10, "%x", rand());
	len += snprintf(buf+len, 10, "%x", rand());
	buf[len] = '\0';
	return buf;
}

int	/* a valid key always >= 0 */
msg2key(char *msg, int len) {
	int i, n = len/4 + ((len%4) ? 1 : 0);
	unsigned int *v, key = 0;
	if(n == 0) return -1;
	for(i = 0, v = (unsigned int*) msg; i < n; i++, v++)
		key ^= *v;
	return key & 0x7fffffff;
}

void
oracle_init(oracle_t *o) {
	int userkey;
	char *secret;
	char buf[128];

	printf("Leave me a message: ");
	fgets(buf, sizeof(buf), stdin);

	if((userkey = msg2key(buf, strlen(buf))) < 0) {
		printf("** Invalid message.\n");
		return;
	}
	int a = rand();
	printf("next_rand = %d\n", a);
	printf("userkey = %d\n", userkey);
	printf("key = %d\n", o->key);
	o->key = a ^ getpid();
	if((secret = gen_secret(o->key, userkey)) != NULL) {
		o->secret = strdup(secret);
		o->stage = (o->stage + 1) % maxstage;
	}
}

void
oracle_guess(oracle_t *o) {
	char buf[128], *guess, *saveptr;
	printf("What's your guess?\n");
	fgets(buf, sizeof(buf), stdin);
	guess = strtok_r(buf, " \t\n\r", &saveptr);
	if(strcmp(guess, o->secret) == 0) {
		printf("*** Good Job!\n");
		system("/showflag");
		o->stage = 0;
	} else {
		printf("*** No no no ...\n");
		o->stage = (o->stage + 1) % maxstage;
	}
}

void
oracle_show(oracle_t *o) {
	printf(
		"Unfortunately, you did not make it.\n"
		"The secret is ``%s`` (from seed %x)\n", o->secret, oseed);
	o->stage = (o->stage + 1) % maxstage;
	oseed = -1;
}

static stage_t stage[] = {
	{ "Talk to Oracle %d\n", oracle_init },
	{ "[Trial #1] Guess the secret of Oracle %d\n", oracle_guess },
	{ "[Trial #2] Guess the secret of Oracle %d\n", oracle_guess },
	{ "Oracle %d, show me the secret\n", oracle_show },
};

int
menu() {
	int i, n = sizeof(oracle)/sizeof(oracle[0]);
	char buf[64];
	printf("\n"
		"-------------------------\n"
		" Oracle Interaction Menu \n"
		"-------------------------\n"
		);
	for(i = 0; i < n; i++) {
		printf(" %c: ", '0'+i+1);
		printf(stage[oracle[i].stage].fmt, i+1);
	}
	printf(
		" q: Quit\n"
		"-------------------------\n"
		"> ");
	if(fgets(buf, sizeof(buf), stdin) == NULL) return 'q';
	return buf[0];
}

void
get_name() {
	char buf[128], *token, *saveptr;
	printf("What's your name? ");
	fgets(buf, sizeof(buf), stdin);
	token = strtok_r(buf, "\t\n\r", &saveptr);
	if(token != NULL) {
		username = strdup(token);
	}
	if(token == NULL || username == NULL) {
		printf("** Internal server error.\n");
		exit(-1);
	}
	printf("Welcome, %s!\n", username);
}

int
main() {
	int ch;

	alarm(300);
	setvbuf(stdin,  NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	srand(time(0) ^ getpid());
	int a;
	magic = rand();
	printf("Magic: %d", magic);
	memset(oracle, 0, sizeof(oracle));
	maxstage = sizeof(stage)/sizeof(stage[0]);
	if(maxstage < 1) return printf("** Internal server error.");

	printf(
		"Welcome to the INP Oracle System v0.0p1\n\n"
		"Talk to the oracle you preferred, and try to guess the secret!\n\n"
		);
 
	get_name();

	while((ch = menu()) != 'q') {
		switch(ch) {
		case '1':
		case '2':
			stage[oracle[ch-'1'].stage].task(&oracle[ch-'1']);
			break;
		default:
			printf("** Invalid input '%c'.\n", ch);
		}
	}

	printf("\nThank you for using our service. We hope to see you soon!\n");

	return 0;
}
