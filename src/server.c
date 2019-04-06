#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "../sysprog-audio-1.5/audio.h"
#include "../include/lecteur.h"
#include "../include/socketlvl2.h"

#include <sys/timerfd.h>
#include <sys/wait.h>
#include <sys/select.h>

#define BUFLEN 1024
#define PORT 8888
#define MAX_PID 3


int main(void)
{
	// initialisations
	char buf[BUFLEN] = {0};
	char name[64] = {"\0"};
	char full[65] = {"\0"};
	unsigned filter = 0;
	
	struct sockaddr_in client;
	socklen_t clientlen = sizeof(client);
	size_t lenrcv;
	
	int ipid = 0;
	pid_t pid = 1;
	int status;
	
	fd_set sockfds;
	struct timeval timeout;
	
	int sockfd = socket_check(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	struct sockaddr_in serv;
	init_sockaddr_in(&serv, AF_INET, htons(PORT), htonl(INADDR_ANY));
	
	bind_check(sockfd, &serv, sizeof(serv));
	
	init_timeout_sock(sockfd, &sockfds, &timeout, 2, 0);
	
	printf("Server open with ip: %s Port: %d\n", inet_ntoa(serv.sin_addr),ntohs(serv.sin_port));
	// Fin  d'initialisations du serveur
	
	while(1) {
		
		lenrcv = timeout_recv_check(sockfd, full, 64, &client, &clientlen, &sockfds, &timeout);
		
		filter = full[0];
		strcpy(name, (full+1));
		
		usleep(2);
		
		if(lenrcv > 0) {
			printf("Name received: %s Client: %s Port: %hu \n",name,inet_ntoa(client.sin_addr), ntohs(client.sin_port));
			
			// Gestion des erreurs
			if(ipid == MAX_PID) {
				printf("Max fork achieved, refusing request...\n");
				send_errno(sockfd, (int*)buf, &client, clientlen, 503);
			}
			else if(access(name, F_OK ) == -1) {
				printf("Error 404 File not found, refusing request...\n");
				send_errno(sockfd, (int*)buf, &client, clientlen, 404);
			}
			else {
				printf("File exists - PID ok\n");
				ipid++;
				if((pid = fork()) < 0) {
					perror("fork failed");
				}
				
				if(pid > 0) {
					bzero((char *) &name, sizeof(name));
				}
			}
		}
		
		if(pid == 0) {
			Son s;
			newSon(&s, name);
			
			if(available_filter(&s,filter) == 0) {
				send_errno(sockfd, (int*)buf, &client, clientlen, 405);
				exit(0);
			}
			
			send_metadata(sockfd, buf, &client, clientlen, &s, filter);
			
			usleep(2);
			send_sound(&s, sockfd, MULTISOCKER, filter, 0.8, &client, clientlen);
			sendto(sockfd,buf, 0, 0,(struct sockaddr*)&client,clientlen);
			exit(0);
		}
		
		if(waitpid(0, &status, WNOHANG) > 0) {
			printf("One zombie died\n");
			ipid--;
		}
	}
	
	
	/* TENTATIVE AVEC NANOSLEEP 
	- il faut compiler avec -D_GNU_SOURCE
	int c = 8;
	int bytes_to_read = ((s->size/8) * s->channels) * c;
	struct timespec req = {0,(long)1000000000/ (long)s->rate * (long)c};
	printf("%d %.9ld\n", bytes_to_read,req.tv_nsec);

	while(read(s->read,buf, bytes_to_read) != 0) {
		sendto(sockfd, buf, bytes_to_read, 0,&client,clientlen);
		if(nanosleep(&req, NULL) == -1) {
			printf("nano interupt\n");
			break;
		}
	}
	
	EXPERIENCE
	Comportement des read/write
	read (...32)
	write (...40)
	-> produit un bruit aigu

	read (...32)
	write(...16)
	-> joue 2 fois plus rapidement
	-> /!\ Mais si on met channels a 1, alors joue que le channel 1

	*/
	close(sockfd);
	return 0;
}
