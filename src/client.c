#include <stdio.h>	//printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#include "../sysprog-audio-1.5/audio.h"
#include "../include/lecteur.h"

#define SERVER "127.0.0.1" //Adresse du serveur
#define BUFLEN 512	//Max length of buffer
#define PORT 8888	//The port on which to send data

int main(int argc, char *argv[])
{
	char name[] = "test.wav";
	char buf[BUFLEN] = {0};
	int meta[3];
	struct sockaddr_in serv;
	socklen_t servlen = sizeof(serv);
	
	struct sockaddr client;
	socklen_t clientlen = sizeof(client);
		
	int speakerfd;
	
	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sockfd == -1) {
		perror("socket failed");
		exit(1);
	}
	
	bzero((char *) &serv, sizeof(serv));
	
	serv.sin_family = AF_INET;
	serv.sin_port = htons(PORT);
	serv.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// ??
	if(sendto(sockfd, name, strlen(name), 0, (struct sockaddr *)&serv, servlen) == -1) {
		perror("sendto failed");
		exit(1);
	}
	
	if(recvfrom(sockfd, meta, 12, 0, &client, &clientlen) == -1) {
		perror("recvfrom()");
		exit(1);
	}
	
	printf("Metadata received %d %d %d\n",meta[0], meta[1], meta[2]);
	speakerfd = aud_writeinit(meta[0],meta[1],meta[2]);
	int bytes_to_read = (meta[1]/8) * meta[2];
	
	if(speakerfd < 0) {
		perror("error aud_writeinit");
	}
	// Permet de creer un timeout
	// https://stackoverflow.com/questions/13547721/udp-socket-set-timeout
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
	 	perror("Error");
	}
	

	int n = 2;
	while((n = recvfrom(sockfd, buf, bytes_to_read, 0, &client, &clientlen)) > 0) {
		write(speakerfd, buf , n);
	}
	return 0;
}
