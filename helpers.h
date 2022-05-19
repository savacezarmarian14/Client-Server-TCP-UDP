#ifndef _HELPERS_H
#define _HELPERS_H 

#include <stdio.h>
#include <stdlib.h>

#define MAX_CLIENTS 1024 // file descriptor table are 1024 intrari
#define ID_CLIENT_LEN 10
#define BUFLEN 2000

#define INITIAL_SOCKETS 5
//0-stdin 1-stdout 2-stderr 3 udp 4 tcp (3 sau 4 prbl invers)

#define TOPIC_LEN 50
#define DATA_TYPE_LEN 1
#define PAYLOAD_LEN 1501

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)


struct subscriber {
	int sockfd; // socket ul asociat clientului
	char id_subscriber[ID_CLIENT_LEN]; // ID_CLIENT
};

struct topic_t {
	char name[TOPIC_LEN]; //denumire topic
	
	unsigned int num_subs;
	struct subscriber subs[MAX_CLIENTS]; // lista de abonati
	int sf[MAX_CLIENTS]; //pentru store and forward
	
	unsigned int num_messages;
	char **buffer; // buffer pt store and forward
};

struct topic_list_t {
	unsigned int num_topics;
	struct topic_t *topics;
};



#endif
