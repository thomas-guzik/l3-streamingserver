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
#include <signal.h> 

#define BUFLEN 512
#define PORT 8888
#define MAX_PID 4

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

int create_timer(time_t tv_sec, long tv_nsec) {
	struct itimerspec spec;
	int timerfd = timerfd_create(CLOCK_REALTIME, 0);
	if(timerfd < 0) {
		perror("timerfd_create");
		exit(1);
	}

	spec.it_interval.tv_sec = tv_sec;
	spec.it_interval.tv_nsec = tv_nsec;
	spec.it_value = spec.it_interval;
	
	if(timerfd_settime(timerfd, 0, &spec, NULL)) {
		perror("timerfd_settime");
		exit(1);
	}
	
	return timerfd;
}

int main(void)
{
	char buf[BUFLEN] = {0};
	char name[64] = {"\0"};
	
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
	
	init_timeout_sock(sockfd, &sockfds, &timeout, 5, 0);
	
	// Fin  d'initialisations du serveur
	while(1) {
		
		lenrcv = timeout_recv_check(sockfd, name, 64, &client, &clientlen, &sockfds, &timeout);
		
		usleep(2);
		
		if(lenrcv > 0) {
			printf("Name received: %s Client: %s Port: %hu \n",name,inet_ntoa(client.sin_addr), client.sin_port);
			
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
			Son *s = newSon(name);
			
			send_metadata(sockfd, buf, &client, clientlen, s);
			
			usleep(2);
			
			// Creation d'un timer qui tick toutes les 10^9/rate_sound nanosecondes
			int timerfd = create_timer(0, (long)MULTISOCKER * (long)1000000000 /(long)s->rate); 
			unsigned long long overrun; 
			
			size_t bytes_to_read = ((s->size/8) * s->channels * (long)MULTISOCKER);
			int bytes_read;
			
			printf("Bytes to read: %ld every  %.9ld /ns\n Sending...\n", bytes_to_read, (long)MULTISOCKER * (long)1000000000 /(long)s->rate);
			
			// unsigned char sockcounter = 0;
			// L'appel de read(timerfd est bloquant jusqu'a que le temps soit ecoule
			while(read(timerfd, &overrun, sizeof(overrun)) > 0) {
				if((bytes_read = read(s->read,buf, bytes_to_read)) == 0) {
					break;
				}
				else {
					//if(sockcounter < 220) // Permet de générer des erreurs
					sendto(sockfd, buf, bytes_read, 0,(struct sockaddr*)&client,clientlen);
					//sockcounter++;
				}
			}
			// Envoi d'une socket vide pour signaler la fin
			sendto(sockfd,buf, 0, 0,(struct sockaddr*)&client,clientlen);
			printf("%s sent completly\n", name);
			
			close(s->read);
			free(s);
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
