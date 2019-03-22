#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "../sysprog-audio-1.5/audio.h"
#include "../include/lecteur.h"

#include <sys/timerfd.h> 
#include <sys/wait.h>


#define BUFLEN 512
#define PORT 8888

// Concernant les timers
// http://man7.org/linux/man-pages/man7/time.7.html

// Various system calls and functions allow a program to sleep (suspend
// execution) for a specified period of time; see nanosleep(2),
// clock_nanosleep(2), and sleep(3).
// 
// Various system calls allow a process to set a timer that expires at
// some point in the future, and optionally at repeated intervals; see
// alarm(2), getitimer(2), timerfd_create(2), and timer_create(2).

// On utilise un timerfd, les autres timers demandent une gestion des signaux
// trop compliqué et nanosleep presentait des bugs sur mon ordi(il fallait 
// fixer un flag pour que les fonctions sleep fonctionne)

int main(void)
{
	char buf[BUFLEN] = {0};
	int meta[3] = {0};
	char name[64] = {"\0"};
	
	struct sockaddr_in serv;
	struct sockaddr_in client;
	socklen_t clientlen = sizeof(client);
	size_t lenrcv;
	
	pid_t pid;
	int status;

	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sockfd < 0) {
		perror("socket failed");
		exit(1);
	}
	
	bzero((char *) &serv, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_port = htons(PORT);
	serv.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if(bind(sockfd,(struct sockaddr*)&serv, sizeof(serv)) < 0) {
		perror("bind");
		exit(1);
	}
	
	while(1) {
		printf("Waiting for client request.. \n");
		lenrcv = recvfrom(sockfd,name,64,0,(struct sockaddr*)&client,&clientlen);
		if(lenrcv < 0) {
			perror("recvfrom failed");
		}
		
		printf("Name received: %s Client: %s\n",name,inet_ntoa(client.sin_addr));
		
		if(access(name, F_OK ) == -1) {
			printf("Error 404 File doesn't exist, refusing request...\n");
			meta[0] = 404;
			sendto(sockfd,meta,4,0,(struct sockaddr*)&client,clientlen);
			
		}
		else {
			printf("File exists\n");
			pid = fork();
		}
		
		if(pid == 0) {
			
			Son *s = newSon(name);
			
			meta[0] = s->rate;
			meta[1] = s->size;
			meta[2] = s->channels;
			
			sendto(sockfd,meta,12,0,(struct sockaddr*)&client,clientlen);
			printf("Metadata sent: %d %d %d\n",meta[0], meta[1], meta[2]);
			usleep(2);
			
			int bytes_to_read = ((s->size/8) * s->channels);
			int bytes_read;
			
			
			// Creation d'un timer qui tick toutes les 10^9/rate_sound nanosecondes
			unsigned long long overrun; 
			struct itimerspec spec; 
			int timerfd = timerfd_create(CLOCK_REALTIME, 0); 
			if(timerfd < 0) { 
				perror("timerfd_create"); 
				exit(1); 
			}
			
			spec.it_interval.tv_sec = 0; 
			spec.it_interval.tv_nsec = (long)1000000000 /(long)s->rate;
			spec.it_value = spec.it_interval;
			
			if(timerfd_settime(timerfd, 0, &spec, NULL)) { 
				perror("timerfd_settime"); 
				exit(1); 
			}
			
			// Fin de la creation du timer
			
			printf("Bytes to read: %d every  %.9ld /ns\n", bytes_to_read, spec.it_interval.tv_nsec);
			
			unsigned char sockcounter = 0;
			// L'appel de read(timerfd est bloquant jusqu'a que le temps soit ecoule
			while(read(timerfd, &overrun, sizeof(overrun)) > 0) {
				bytes_read = read(s->read,buf, bytes_to_read);
				if(bytes_read == 0) {
					break;
				}
				else {
					// Permet de générer des erreurs
					//if(sockcounter < 220)
					sendto(sockfd, buf, bytes_read, 0,(struct sockaddr*)&client,clientlen);
				sockcounter++;
				}
			}
			
			// Envoi d'une socket vide pour signaler la fin
			sendto(sockfd,buf, 0, 0,(struct sockaddr*)&client,clientlen);
			
			close(s->read);
			free(s);
		}
		else {
			while (wait(&status) != pid) {
				printf("Refusing any request... \n");
				lenrcv = recvfrom(sockfd,name,1,0,(struct sockaddr*)&client,&clientlen);
				if(lenrcv < 0) {
					perror("recvfrom failed");
				}
				meta[0] = 503;
				sendto(sockfd,meta,4,0,(struct sockaddr*)&client,clientlen);
			}
			printf("Let's start again\n");
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
	
	return 0;
}
