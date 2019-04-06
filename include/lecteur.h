#ifndef LECTEUR_FILE
#define LECTEUR_FILE

#include <unistd.h>
#include <sys/timerfd.h>

#define MONO 0b01
#define VOLUME 0b010
#define ECHO 0b100
#define BUFLEN 1024

typedef struct Son Son;
struct Son {
	int rate;
	int size;
	int channels;
	char* name;
	int read;
	int write;
};

void newSon(Son *s, char* name);
void play(Son *s);
void init_echo(unsigned char* a, unsigned char* b);
void echo(Son *s, short* buf,short bufecho[256][BUFLEN], unsigned char* a, unsigned char* b, unsigned short bytes);
void volume(Son *s, char* buf, unsigned short size, float lvl);
void channel1(Son *s, void* buf, unsigned short size);
void channel2(Son *s, void* buf, unsigned short size);
void mono(Son *s, char* buf, unsigned short size);
size_t bytes_for_sampling(Son* s);
long elapsed_time_between_sampling(Son* s);
int create_timer(time_t tv_sec, long tv_nsec);
void read_sound(Son *s, int fdout, int multi, unsigned char filter, float lvl);
int available_filter(Son *s, unsigned char filter);

#endif
