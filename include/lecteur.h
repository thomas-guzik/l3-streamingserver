#ifndef LECTEUR_FILE
#define LECTEUR_FILE

typedef struct Son Son;
struct Son {
	int rate;
	int size;
	int channels;
	char* name;
	int read;
	int write;
};

Son* newSon(char* name);
void play(Son *s);

#endif

