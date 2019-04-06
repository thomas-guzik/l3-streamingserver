#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/timerfd.h>

#include "../sysprog-audio-1.5/audio.h"
#include "../include/lecteur.h"

#define BUFLEN 1024


char float_to_char(float a) {
	if(a - (float)(char)a < 0.5) {
		return (char)a;
	}
	else {
		return (char)a+1;
	}
}

void newSon(Son* s,char* name){
	s->read = aud_readinit(name, &s->rate, &s->size, &s->channels);
	if(s->read < 0) {
		perror("error aud_readinit");
		exit(1);		
	}
}

void play(Son *s) {
	char buf[64];
	int n = 0;
	s->write = aud_writeinit (s->rate, s->size, s->channels);
	if(s->write < 0) {
		perror("error aud_writeinit");
	}
	
	while((n = read(s->read,buf, 64)) != 0) {
		write(s->write, buf , 64);
	}
	close(s->write);
	close(s->read);
}

void init_echo(unsigned char *a, unsigned char *b) {
	*a = 0;
	*b = 128;
}


void echo(Son *s, short* buf,short bufecho[256][BUFLEN], unsigned char* a, unsigned char* b, unsigned short bytes) {
	memcpy(&bufecho[*a], buf, bytes);
	
	bytes >>= 1;
	for(int i = 0; i < bytes; i++) {
		buf[i] += bufecho[*b][i];
	}
	(*a)++;
	(*b)++;
}

void volume(Son *s, char* buf, unsigned short size, float lvl) {
	if(s->size == 8) {
		for(int i = 0; i < size; i++) {
			//printf("%d ", buf[i]);
			char tmp = float_to_char((float)buf[i]*lvl);
			//printf("%d\n", tmp);
			
			if(tmp > 127) {
				buf[i] = 127;
			}
			else if (tmp < -128) {
				buf[i] = -128;
			}
			else {
				buf[i] = tmp;
			}
			
		}
	}
	else if(s->size == 16) {
		for(int i = 0; i < size; i++) {
			((short* )buf)[i] *= lvl;
		}
	}
}

void channel1(Son *s, void* buf, unsigned short size) {

}

void channel2(Son *s, void* buf, unsigned short size) {

}

// Mono est une fonction qui peut etre trop lente vu la vitesse de conversion
// demande
// Pour accelerer mono, on utilise i << 1 pour i*2 et 
// byte >> 1 pour byte / 2
void mono(Son *s, char* buf, unsigned short bytes) {
	bytes >>= 1;

	if(s->size == 8) {
		for(int i = 0; i < bytes; i+=2) {
				buf[i] = buf[i << 1]; //
				buf[i+1] = buf[(i << 1) +1];
		}
	}
	else if(s->size == 16) {
		bytes >>= 1;
		for(int i = 0; i < bytes; i+=2) {
			((short*)buf)[i] = ((short*)buf)[i << 1];
			((short*)buf)[i+1] = ((short*)buf)[(i << 1)+1];
		}
	}
}

size_t bytes_for_sampling(Son* s) {
	return  (size_t)(s->size/8) * s->channels;
}

long elapsed_time_between_sampling(Son* s) {
	return 1000000000L/(long)s->rate;
}

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

void read_sound(Son *s, int fdout, int multi, unsigned char filter, float lvl) {
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
			write(fdout, buf , bytes_read);
		}
	}
	close(timerfd);
}

int available_filter(Son *s, unsigned char filter) {
	printf("Filter = %d, channel = %d, size = %d", filter, s->channels, s->size);
	if(((filter & MONO) == MONO) && s->channels == 1) {
		return 0;
	}
	if(((filter & VOLUME) == VOLUME) && s->size == 8) {
		return 0;
	}
	if(((filter & ECHO) == ECHO) && s->size == 8) {
		return 0;
	}
	return 1;
}
	
