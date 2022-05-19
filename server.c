#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include "helpers.h"




int connected; //numarul de utilizatori conectati

struct topic_list_t topic_list;
struct subscriber subscribers[MAX_CLIENTS];

void print_topic_subscribers(char*);
void print_topic_buffer(char*);
int disconnected_sf_clients(int);

void create_topic(char *name)
{
	/* daca topicul exista trec peste adaugare */
	for (int i = 0; i < topic_list.num_topics; i++) {
		char *this_name = strdup(topic_list.topics[i].name);
		if (strcmp(name, this_name) == 0) {
			return;
		}
	}
	/* aloc o noua intrare pt lista mea de topicuri */
	if (topic_list.num_topics == 0) {
		topic_list.topics  = malloc(sizeof(struct topic_t));
		topic_list.num_topics ++;
	} else {
		topic_list.topics = realloc(topic_list.topics,
							(++topic_list.num_topics) * sizeof(struct topic_t));
	}
	int n = topic_list.num_topics;
	strcpy(topic_list.topics[n-1].name, name);
}

void send_new_topic_message(char *buffer, int sockfd)
{
	int rc;
	rc = send(sockfd, buffer, BUFLEN, 0);
	DIE(rc < 0, "SEND TOPIC MESSAGE");
}

void add_message_to_topic(char *topic_name, char *buffer) 
{
	int topic_index;
	for(int i = 0; i < topic_list.num_topics; i++) {
		if (strcmp(topic_name, topic_list.topics[i].name) == 0) {
			// ma aflu in topicul respectiv //
			topic_index = i;
			if (disconnected_sf_clients(i) == 1) {
				int n = topic_list.topics[i].num_messages;
				if (n == 0) {
					topic_list.topics[i].buffer = malloc(sizeof(char*));
					topic_list.topics[i].buffer[n] = strdup(buffer);
					topic_list.topics[i].num_messages += 1;
				} else {
					topic_list.topics[i].buffer = realloc(topic_list.topics[i].buffer,
							(++topic_list.topics[i].num_messages) * sizeof(char*));
					topic_list.topics[i].buffer[n] = strdup(buffer);
				}
			}
		}
	}
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(topic_list.topics[topic_index].subs[i].sockfd != 0) {
			send_new_topic_message(buffer, 
				topic_list.topics[topic_index].subs[i].sockfd);
		}
	}
	//print_topic_buffer(topic_name);
}

int disconnected_sf_clients(int index) 
{
	for(int i = 0; i < topic_list.topics[index].num_subs; i++) {
		if (topic_list.topics[index].sf[i] == 1 &&
			topic_list.topics[index].subs[i].sockfd == 0) {
				return 1;
			}
	}
	return 0;
}

char** parse_command(char *buffer, int *num)
{	
	int num_words = 0; 

	char **command_words = malloc(sizeof(char*)); // aloc doar un cuvant

	char *token = strtok(buffer, " \n");
	command_words[num_words++] = strdup(token);
	while (token != NULL) {
		token = strtok(NULL, " \n");
		if (token != NULL) {
			command_words = realloc(command_words, 
									(num_words + 1) * sizeof(char*));
			command_words[num_words++] = strdup(token);
		}
	}
	
	*num = num_words;
	return command_words;
}



void subscribe_topic(char *topic, int sockfd, char *sf) {
	int index = sockfd - INITIAL_SOCKETS;
	int topic_index;
	int sf_int = atoi(sf);	for(int i = 0; i < topic_list.num_topics; i++) {
		if(strcmp(topic_list.topics[i].name, topic) == 0) { 
			topic_index = i;
			break;
		}
	}


	for (int i = 0; i < MAX_CLIENTS ;i++) {
		if (strcmp(topic_list.topics[topic_index].subs[i].id_subscriber,
			subscribers[index].id_subscriber) == 0) {
			topic_list.topics[topic_index].sf[i] = sf_int;
			return;
		}
	}

	for (int i = 0; i < MAX_CLIENTS ;i++) {
		if (topic_list.topics[topic_index].subs[i].sockfd == 0) {
			topic_list.topics[topic_index].subs[i] = subscribers[index]; 
			topic_list.topics[topic_index].sf[i] = sf_int;
			if (topic_list.topics[topic_index].num_subs == i) {
				topic_list.topics[topic_index].num_subs = i + 1;
			}

			break;
		}
	}

}

void print_topic_subscribers(char *topic)
{
	int topic_index;
	for(int i = 0; i < topic_list.num_topics; i++) {
		if(strcmp(topic_list.topics[i].name, topic) == 0) { 
			topic_index = i;
			break;
		}
	}
	for(int i = 0; i < topic_list.topics[topic_index].num_subs; i++) {
		printf("%s -> %d\n",topic_list.topics[topic_index].subs[i].id_subscriber,
			topic_list.topics[topic_index].subs[i].sockfd);
	}
}

void print_topic_buffer(char *topic) 
{
	int topic_index;
	for(int i = 0; i < topic_list.num_topics; i++) {
		if(strcmp(topic_list.topics[i].name, topic) == 0) { 
			topic_index = i;
			break;
		}
	}
	printf("%d\n", topic_list.topics[topic_index].num_messages);
	for(int i = 0; i < topic_list.topics[topic_index].num_messages; i++) {
		printf("%s\n", topic_list.topics[topic_index].buffer[i]);
	}
}

void forward(int sockfd) {
	char *id_client = strdup(subscribers[sockfd - INITIAL_SOCKETS].id_subscriber);
	char buffer[BUFLEN];
	int num_topic = topic_list.num_topics;
	for(int i = 0; i < num_topic; i++) {
		int num_subs = topic_list.topics[i].num_subs;
		for(int j = 0; j < num_subs; j++) {
			if(strcmp(topic_list.topics[i].subs[j].id_subscriber,
				id_client) == 0 && topic_list.topics[i].sf[j] == 1) {
					for(int k = 0; k < topic_list.topics[i].num_messages; k++) {
						memset(buffer, 0, BUFLEN);

						strcpy(buffer, topic_list.topics[i].buffer[k]);
						printf("%s\n", buffer);

						int rc = send(sockfd, &buffer, strlen(buffer), 0);
						DIE(rc < 0, "forward");
					}
					break;
				}
		}
	}
}

void notify_reconnected(int sockfd) 
{
	int index = sockfd - INITIAL_SOCKETS;
	char *id_client = strdup(subscribers[index].id_subscriber);

	/* parcurc toate topicurile si anunt reconectarea indiferent 
	daca sunt abonati la acesta */
	for (int i = 0; i < topic_list.num_topics; i++) {
		/* parcurc toti abonatii topicului */
		for(int j = 0; j < topic_list.topics[i].num_subs; j++) {
			if(strcmp(topic_list.topics[i].subs[j].id_subscriber, 
				id_client) == 0) {
					//printf("socket for %s set to %d\n", id_client, sockfd);

					/* schimb sockfd in caz ca s a reconectat pe alt socket */
					topic_list.topics[i].subs[j].sockfd = sockfd;
				}
		}
	}
}

void notify_disconnected(int sockfd) 
{
	int index = sockfd - INITIAL_SOCKETS;
	char *id_client = strdup(subscribers[index].id_subscriber);

	/* parcurc toate topicurile si anunt reconectarea indiferent 
	daca sunt abonati la acesta */
	for (int i = 0; i < topic_list.num_topics; i++) {
		/* parcurc toti abonatii topicului */
		for(int j = 0; j < topic_list.topics[i].num_subs; j++) {
			if(strcmp(topic_list.topics[i].subs[j].id_subscriber, 
				id_client) == 0) {
					/* schimb sockfd in 0 pt deconectat */
					//printf("socket for %s set to 0\n", id_client);
					topic_list.topics[i].subs[j].sockfd = 0;
				}
		}
	}
}

void execute_command(char *buffer, int sockfd, fd_set *set) 
{
	
	int rc;
	int num_words;
	char *send_buffer;
	char **words = parse_command(buffer, &num_words);

	if (strcmp(words[0], "subscribe") == 0) {
		send_buffer = strdup("Subscribed to topic.");
		//TODO
		create_topic(words[1]); // words-1 topic
		subscribe_topic(words[1], sockfd, words[2]);
	} 
	else if (strcmp(words[0], "unsubscribe") == 0) {
		send_buffer = strdup("Unsubscribed from topic.");
		//TODO
	}
	else if (strcmp(words[0], "exit") == 0) {
		send_buffer = strdup("exit");
		FD_CLR(sockfd, set);
		/* trimit mesaj de inchidere */
		rc = send(sockfd, send_buffer, strlen(send_buffer), 0);///

		int index = sockfd - INITIAL_SOCKETS;
		
		printf("Client %s disconnected.\n", subscribers[index].id_subscriber);
		/* notific toate topicurile ca s a deconectat */
		notify_disconnected(sockfd); 
		/* sterg din lista de clienti conectati */
		strcpy(subscribers[index].id_subscriber, "");
		subscribers[index].sockfd = -1;
		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
	}

	if(strcmp(words[0], "exit") != 0) {
		rc = send(sockfd, send_buffer, strlen(send_buffer), 0);
		DIE(rc < 0, "send ACK");
	}
}


void send_shutdown_signal(int sockfd) {
	int rc ;
	rc = send(sockfd, "shutdown", strlen("shutdown"), 0);
		DIE(rc < 0, "Send shutdown failed");
	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);
}

void send_shutdown_signal_all() {
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if(subscribers[i].sockfd != -1) {
			printf("%d\n", subscribers[i].sockfd);
			send_shutdown_signal(subscribers[i].sockfd);
		}
	}
}

/* FUNCTIA DE ACCEPTARE A CLIENTILOR */
int accept_connection(int sockfd) 
{
	ssize_t ret_recv, ret_send;
	struct sockaddr_in client_address;
	socklen_t addres_len = sizeof(client_address);
	/* Accpet conexiunea si retin in newfd file descriptorul clientului abia conectat */
	int newfd = accept (sockfd, 
						(struct sockaddr*) &client_address,
						&addres_len);
	DIE(newfd < 0, "accept");

	char ID_CLIENT[ID_CLIENT_LEN] = {0};
	/* primesc de la client <ID_CLIENT> */
	ret_recv = recv(newfd, ID_CLIENT, ID_CLIENT_LEN, 0);
	DIE(ret_recv == -1, "recv-accpet-client");
	/* Verific daca exista un client cu acelasi ID */
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (strcmp(subscribers[i].id_subscriber, ID_CLIENT) == 0) {
			/* Exista deja un client cu ID_CLIENT */

			/* trimit mesaj clientului sa se inchida */
			send_shutdown_signal(newfd);
			printf("Client %s already connected.\n", ID_CLIENT);
			
			return -1;
		}
	}

	ret_send = send(newfd, "accepted\n", strlen("accepted\n"), 0);
	DIE(ret_send < 0, "Send accept ack failed");
	/* Adaug clientul in lista clientilor conectati */
	strcpy(subscribers[newfd - INITIAL_SOCKETS].id_subscriber, ID_CLIENT);
	subscribers[newfd - INITIAL_SOCKETS].sockfd =  newfd;
	notify_reconnected(newfd);
	printf("New client %s connected from %s:%d.\n",
			ID_CLIENT,
			inet_ntoa(client_address.sin_addr), 
			ntohs(client_address.sin_port));

	return newfd;
}

void make_topic_message(char *buffer, struct sockaddr_in client_addr)
{
	char topic[TOPIC_LEN];
	unsigned int type;
	char payload[PAYLOAD_LEN];

	memcpy(topic, buffer, TOPIC_LEN); memcpy(&type, buffer + TOPIC_LEN, 1);
	memcpy(payload, buffer + TOPIC_LEN + DATA_TYPE_LEN, PAYLOAD_LEN);

	create_topic(topic);

	memset(buffer, 0, BUFLEN);
	if (type == 0) {
		uint32_t int_num = ntohl(*((uint32_t*)(payload + 1)));
		if(payload[0] == 1) {
			int_num *= -1;
		}
		sprintf(buffer, "%s:%d - %s - INT - %d",
				inet_ntoa(client_addr.sin_addr),
				ntohs(client_addr.sin_port), topic, int_num);

	} else if (type == 1) {
		double real_num = ntohs(*(uint16_t*)(payload));
        real_num /= 100;
        sprintf(buffer, "%s:%d - %s - SHORT_REAL - %.2f",
				inet_ntoa(client_addr.sin_addr),
            	ntohs(client_addr.sin_port), topic, real_num);

	} else if (type == 2) {
		double real_num = ntohl(*(uint32_t*)(payload + 1));
		real_num /= pow(10, payload[5]);
		if (payload[0] == 1) {
			    real_num *= -1;
		}
		sprintf(buffer, "%s:%d - %s - FLOAT - %f",
				inet_ntoa(client_addr.sin_addr),
			    ntohs(client_addr.sin_port), topic, real_num);

	} else if (type == 3) {
		sprintf(buffer, "%s:%d - %s - STRING - %s",
				inet_ntoa(client_addr.sin_addr),
            	ntohs(client_addr.sin_port), topic, payload);
	}
	add_message_to_topic(topic, buffer);
}

void recive_infortmation(int sockfd)
{
	int rc;
	char buffer[BUFLEN];
	memset(buffer, 0, BUFLEN);
	struct sockaddr_in udp_client_address;
	socklen_t addres_len;
	
	/* primesc mesaj de la un client udp */
	rc = recvfrom(sockfd, buffer, BUFLEN, 0,
					(struct sockaddr*) &udp_client_address, 
					&addres_len);
	DIE(rc < 0, "Recv from udp client");

	/* parsez bufferul si creez mesajul */
	make_topic_message(buffer, udp_client_address);

}
int main(int argc, char *argv[]) 
{
	/* initializari */
	for(int i = 0; i < MAX_CLIENTS; i++)
		subscribers[i].sockfd = -1;

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int rc;
	char buffer[BUFLEN];
	/* INITIALIZARE SOCKET UDP SI TCP */
	int sock_tcp, sock_udp; // deschid doi socketi TCP SI UDP
	
	sock_tcp = socket(AF_INET, SOCK_STREAM, 0); // AF_INET -> IPv4 | SOCK_STREAM -> TCP
	DIE(sock_tcp < 0, "Open socket TCP failed");
	
	sock_udp = socket(AF_INET, SOCK_DGRAM, 0); // SOCK_DGRAM -> (datagram) UDP
	DIE(sock_udp < 0, "Open socket UDP failed");
	/* INITIALIZARE SERVER ADDRESS + BIND PE PORT */
	
	struct sockaddr_in server_addres;
	/* zeroizez structura de adresa */
	memset((void *) &server_addres, 0, sizeof(server_addres));
	/* setez familia de adrese la IPv4 */
	server_addres.sin_family = AF_INET;
	/* setez portul serverului */
	server_addres.sin_port = htons(atoi(argv[1]));
	/* setez valabilitate pentru orice IP */
	server_addres.sin_addr.s_addr = INADDR_ANY;

	/* BIND pentru socket ul TCP si UDP pe adresa serverului */
	rc = bind(sock_tcp, (const struct sockaddr*) &server_addres,
		sizeof(server_addres));
	DIE(rc < 0, "Bind socket TCP failed");
			// Acest lucru o sa mearga deoarece sunt protocoale diferite
	rc = bind(sock_udp, (const struct sockaddr*) &server_addres,
		sizeof(server_addres));
	DIE(rc < 0, "Bind socket UDP failed");

	/* Initializez doua seturi 1.Lista de socket 2.Lista temporara pt multiplexare */
	fd_set sock_set, tmp_set;
	/* Zeroizez seturile */
	FD_ZERO(&sock_set); FD_ZERO(&tmp_set);
	/* Adaug in multime sock_udp si sock_tcp */
	FD_SET(sock_tcp, &sock_set); FD_SET(sock_udp, &sock_set);
	/* Adaug STDIN */
	FD_SET(STDIN_FILENO, &sock_set);
	/* initializez fdmax cu sock_udp deoarece e ultimul deschis */
	int fdmax = sock_udp;

	/* Setez sock_tcp ca socket de listen pentru eventualele conexiuni */
	rc = listen (sock_tcp, 1);
	DIE(rc < 0 , "Listen failed");

	while (1) {
		tmp_set = sock_set;
		/* selectez doar fd urile unde s au facut modificari */
		rc = select(fdmax + 1, &tmp_set, NULL, NULL, NULL);
		DIE(rc < 0, "Select failed");

		/* iterez prin lista de fds pentru a vedea daca a fost
		 * pentru a vedea daca s au facut modificari 
		 */
		
		for (int i = 0; i < fdmax + 1; i++) {
			if (FD_ISSET(i, &tmp_set)) {
				/* verific daca s au fact cereri pe socketul TCP
				 * deschis pentru ascultarea noilor conexiuni
				 */
				if (i == sock_tcp) {
					int new_fd = accept_connection(sock_tcp);
					if (new_fd != -1) {
						FD_SET(new_fd, &sock_set);	
						fdmax += 1;
						forward(new_fd);
					}
				}
				if (i == sock_udp) {
					recive_infortmation(sock_udp);
					
				}
				if (i == STDIN_FILENO) {
					memset(buffer, 0, BUFLEN);
					rc = read(STDIN_FILENO, buffer, BUFLEN - 1);
					DIE(rc < 0, "read");

					if(strcmp(buffer, "exit\n") == 0){
						send_shutdown_signal_all();
						shutdown(sock_tcp, SHUT_RDWR);
						shutdown(sock_udp, SHUT_RDWR);
						close(sock_tcp);
						close(sock_udp);
						return 0;
					}
				} else if(i != sock_tcp && i != sock_udp && 
							i != STDIN_FILENO) {
					memset(buffer, 0 , BUFLEN);
					rc = recv(i, buffer, BUFLEN, 0);
					DIE(rc < 0, "recv command from client");
					execute_command(buffer, i, &sock_set);
				}
			}
		}
	}



	


	return 0;
}