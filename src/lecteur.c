#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include "../sysprog-audio-1.5/audio.h"
#include "../include/lecteur.h"

Son* newSon(char* name){
	Son* s = (Son *)malloc(sizeof(Son));
	s->read = aud_readinit(name, &s->rate, &s->size, &s->channels);
	if(s->read < 0) {
		perror("error aud_readinit");
		free(s);
		exit(1);		
	}
	return s;
}


void play(Son *s) {
	char buff[64];
	int n = 0;
	s->write = aud_writeinit (s->rate, s->size, s->channels);
	if(s->write < 0) {
		perror("error aud_writeinit");
	}
	
	while((n = read(s->read,buff, 64)) != 0) {
		printf("%d %d\n",n, buff[0]);
		write(s->write, buff , 64);
	}
	close(s->write);
	close(s->read);
	free(s);
}
/*
int main(int argc, char *argv[]){
	Son* s = newSon(argv[1]);
	play(s);
	//free(s);
	return 0;
}*/
