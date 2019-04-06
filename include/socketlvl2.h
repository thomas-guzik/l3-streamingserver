#ifndef SOCKETLVL2_FILE
#define SOCKETLVL2_FILE

#define MULTISOCKER 64
#include <sys/socket.h>
#include "../include/lecteur.h"


int socket_check(int domain, int type, int protocol);

void bind_check(int sockfd, const struct sockaddr_in *addr,socklen_t addrlen);

void init_sockaddr_in(struct sockaddr_in *addr, short int sin_family, unsigned short sin_port, unsigned long s_addr);

void init_timeout_sock(int sockfd, fd_set* set, struct timeval* timeout, time_t tv_sec, suseconds_t tv_usec);

ssize_t sendto_check(int sockfd, const void *buf, size_t len, int flags, struct sockaddr_in *dest_addr, socklen_t addrlen);

ssize_t timeout_recv_check(int sockfd, void* buf, size_t len, struct sockaddr_in *client, socklen_t *clientlen, fd_set *sockfds, struct timeval *timeout);

void check_error(int* buf, int recvlen);

void send_metadata(int sockfd, char *buf, struct sockaddr_in *client, size_t len, struct Son *s, unsigned char filter);

void send_errno(int sockfd, int* buf, struct sockaddr_in* client, size_t len, int errno);

void send_sound(Son *s, int sockfd, int multi, unsigned char filter, float lvl, struct sockaddr_in *client, socklen_t clientlen);

#endif
