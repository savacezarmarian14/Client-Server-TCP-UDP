#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

#define BUFLEN 2000

int main(int argc, char *argv[])
{
	/* initializari */
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int rc;
	char buffer[BUFLEN];
	DIE(argc < 2, "./subscriber <ID CLIENT> <IP> <PORT>");

	/* setez socket client */
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "Open client socket failed");

	/* setez IP si PORT unde sa se conecteze clientu; */
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(atoi(argv[3]));
	rc = inet_aton(argv[2], &server_address.sin_addr); /*IP CLIENT*/
	DIE(rc == 0, "inet_aton failed");

	/* Creez un set de fd in care adaug socketCLIENT si STDIN_FILENO */
	fd_set client_set, tmp_set;
	FD_ZERO(&client_set);
	FD_SET(STDIN_FILENO, &client_set); FD_SET(sockfd, &client_set);
	int fdmax = sockfd; // Ultimul deschis //

	/* Incerc sa ma contectez la server */	
	rc = connect(sockfd, (struct sockaddr*) &server_address,
		sizeof(server_address));
	DIE(rc < 0, "Connect failed");
	
	/* Dupa ce m am conectat trimit ID_CLIENT catre server */
	rc = send(sockfd, argv[1], strlen(argv[1]), 0);
	DIE(rc < 0, "Send ID_CLIENT failed");

	/* Primesc mesaj de acceptare conexiune de la server */
	rc = recv(sockfd, buffer, BUFLEN, 0);
	DIE(rc < 0, "Recv accept/shutdown failed");

	if(strcmp(buffer, "shutdown") == 0) {
		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
		return 0;
	}

	while(1)
	{
		tmp_set = client_set;
		rc = select(fdmax + 1, &tmp_set, NULL, NULL, NULL);
		DIE(rc < 0, "Select failed");

		/* iterez prin lista de fds */
		for (int i = 0; i < fdmax + 1; i++) {
			/* verific pentru modificari */
			if (FD_ISSET(i, &tmp_set)) {
				/* Verific daca trimit mesaj de la tastatura */
				if (i == STDIN_FILENO) {
					/* Setez buffer ul cu 0 */
					memset(buffer, 0, BUFLEN);
					/* Citesc din stdin */
					rc = read(STDIN_FILENO, buffer, BUFLEN - 1);
					DIE(rc < 0, "read");
					/* tratez cazul in care clientul se deconecteaza*/
					
					rc = send(sockfd, buffer, strlen(buffer), 0);
					DIE(rc < 0, "Send failed");

					memset(buffer, 0, BUFLEN);
					
					rc = recv(sockfd, buffer, BUFLEN, 0);
					DIE(rc < 0, "Recv ACK");

					if(strcmp(buffer, "exit") == 0){
						shutdown(sockfd, SHUT_RDWR);
						close(sockfd);
						return 0;
					} else {
						printf("%s\n", buffer);
					}
					memset(buffer, 0, BUFLEN);
				} 
				if (i == sockfd) {
					memset(buffer, 0, BUFLEN);
					
					rc = recv(sockfd, &buffer, BUFLEN, 0);
					DIE(rc < 0, "ERROR RECV FROM SOCKET");

					if (strcmp(buffer, "shutdown") == 0) {
						shutdown(sockfd, SHUT_RDWR);
						close(sockfd);
						return 0;
					}
					printf("%s\n", buffer);
				}
			}
		}
	}
	return 0;
}