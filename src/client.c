#include <stdio.h>	//printf
#include <stdlib.h> //exit(0);
#include <string.h> //memset
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "../sysprog-audio-1.5/audio.h"
#include "../include/lecteur.h"
#include "../include/socketlvl2.h"

#define SERVER "127.0.0.1" //Adresse du serveur
#define BUFLEN 64	//Max length of buffer
#define PORT 8888	//The port on which to send data

int main(int argc, char *argv[])
{
	char name[] = "test.wav";
	char buf[BUFLEN] = {0};
	
	struct sockaddr_in client;
	socklen_t clientlen = sizeof(client);
	ssize_t recvlen;
		
	fd_set sockfds;
	struct timeval timeout;
	
	int sockfd = socket_check(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	init_timeout_sock(sockfd, &sockfds, &timeout, 2,0);
	
	struct sockaddr_in serv;
	socklen_t servlen = sizeof(serv);
	init_sockaddr_in(&serv, AF_INET, htons(PORT), htonl(INADDR_ANY));
	
	printf("Music: %s\n", name);
	
	sendto_check(sockfd, name, strlen(name), 0, &serv, servlen);
	
	recvlen = timeout_recv_check(sockfd, buf, 6, &client, &clientlen, &sockfds, &timeout);
	
	check_error((int*)buf, recvlen);
	
	printf("Metadata received %d %d %d\n",((int*)buf)[0], buf[4], buf[5]);
	
	int speakerfd = aud_writeinit(((int*)buf)[0], buf[4], buf[5]);
	
	if(speakerfd < 0) {
		perror("aud_writeinit failed");
		exit(1);
	}
	
	size_t bytes_to_read = (buf[4]/8) * buf[5];
	
	while((recvlen = recvfrom(sockfd, buf, bytes_to_read,0, (struct sockaddr *)&client, &clientlen)) > 0) {
		write(speakerfd, buf , recvlen); // pas de verif du write
	}
	printf("Fin de la transmission\n");
	
	close(speakerfd);
	close(sockfd);
	return 0;
}
