#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

#include "../include/socketlvl2.h"
#include "../include/lecteur.h"


int socket_check(int domain, int type, int protocol) {
	int sockfd = socket(domain, type, protocol);
	if(sockfd == -1) {
		perror("socket failed");
		exit(1);
	}
	return sockfd;
}

void bind_check(int sockfd, const struct sockaddr_in *addr,socklen_t addrlen) {
	if(bind(sockfd,(struct sockaddr*) addr, addrlen) < 0) {
		perror("bind");
		exit(1);
	}
}

void init_sockaddr_in(struct sockaddr_in *addr, short int sin_family, unsigned short sin_port, unsigned long s_addr) {
	bzero((char *) addr, sizeof(*addr));
	addr->sin_family = sin_family;
	addr->sin_port = sin_port;
	addr->sin_addr.s_addr = s_addr;
}

void init_timeout_sock(int sockfd, fd_set* set, struct timeval* timeout, time_t tv_sec, suseconds_t tv_usec) {
	timeout->tv_sec = tv_sec;
	timeout->tv_usec = tv_usec;
	
	FD_ZERO(set);
	FD_SET(sockfd, set);
}

ssize_t sendto_check(int sockfd, const void *buf, size_t len, int flags, struct sockaddr_in *dest_addr, socklen_t addrlen) {
	ssize_t ret = sendto(sockfd, buf, len, flags, (struct sockaddr *)dest_addr, addrlen);
	if(ret == -1) {
		perror("sendto failed");
	}
	return ret;
}

ssize_t timeout_recv_check(int sockfd, void* buf, size_t len, struct sockaddr_in *client, socklen_t *clientlen, fd_set *sockfds, struct timeval *timeout) {
	ssize_t recvlen;
	
	FD_SET(sockfd, sockfds);
	
	if(select(8, sockfds, NULL, NULL, timeout) == 0) {
		return 0;
	}
	
	if((recvlen = recvfrom(sockfd, buf, len, 0, (struct sockaddr *)client, clientlen)) == -1) {
		perror("recvfrom()");
	}
	return recvlen;
}

void check_error(int* buf, int recvlen) {
	if(recvlen == 4) {
		switch (buf[0]) {
			case 503:
				printf("Error 503 Service Unavailable, come later\n");
				break;
			case 404:
				printf("Error 404 File not found\n");
				break;
			case 405:
				printf("Error 404 Filter not available\n");
				break;
			default:
				printf("Unknown error %d\n",(int)buf[0]);
		}
		exit(1);
	}
	else if(recvlen == 0) {
		printf("Connection timeout\n");
		exit(1);
	}
}

void send_metadata(int sockfd, char *buf, struct sockaddr_in *client, size_t len, struct Son *s, unsigned char filter) {
	((int*)buf)[0] = s->rate;
	buf[4] = s->size;
	if((filter & MONO) == MONO) {
		buf[5] = 1;
	}
	else {
	buf[5] = s->channels;
}
	
	sendto(sockfd,buf,6,0,(struct sockaddr*)client,len);
	printf("Metadata sent: %d %d %d\n",((int*)buf)[0], buf[4], buf[5]);
}

void send_errno(int sockfd, int* buf, struct sockaddr_in* client, size_t len, int errno) {
	buf[0] = errno;
	sendto(sockfd,buf,4,0,(struct sockaddr*)client,len);
}

void send_sound(Son *s, int sockfd, int multi, unsigned char filter, float lvl, struct sockaddr_in *client, socklen_t clientlen) {
	size_t bytes_to_read, bytes_read;
	char buf[BUFLEN];
	
	bytes_to_read = bytes_for_sampling(&s[0]) * multi;
	long elapsed = elapsed_time_between_sampling(&s[0]) * multi;
	
	int timerfd = create_timer(0, elapsed); 
	unsigned long long overrun; 
	
	printf("Bytes to read: %ld every  %ld /ns\n", bytes_to_read, elapsed);

	unsigned char a, b;
	short bufecho[256][BUFLEN];
	init_echo(&a, &b);
	
	
	while(read(timerfd, &overrun, sizeof(overrun)) > 0) {
		if((bytes_read = read(s->read,buf, bytes_to_read)) == 0) {
			break;
		}
		else {
			if((filter & MONO) == MONO) {
				mono(s, buf, bytes_read);
				bytes_read = bytes_read/2;
			}
			if((filter & VOLUME) == VOLUME) {
				volume(s, buf, bytes_read, lvl);
			}
			if((filter & ECHO) == ECHO) {
				echo(s, (short*)buf, bufecho, &a, &b, bytes_read);
			}
			sendto(sockfd, buf, bytes_read, 0,(struct sockaddr*)client,clientlen);
		}
	}
	close(timerfd);
	close(s->read);
}
