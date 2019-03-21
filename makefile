all : audioserver audioclient

audioclient : obj/client.o obj/audio.o obj/lecteur.o
	gcc obj/client.o obj/audio.o obj/lecteur.o -o  audioclient

audioserver : obj/server.o obj/audio.o obj/lecteur.o
	gcc obj/server.o obj/audio.o obj/lecteur.o -o audioserver


obj/lecteur.o : src/lecteur.c
	gcc -c src/lecteur.c -o obj/lecteur.o

obj/audio.o : sysprog-audio-1.5/audio.c
	gcc -c sysprog-audio-1.5/audio.c -o obj/audio.o

obj/server.o: src/server.c
	gcc -c src/server.c -o obj/server.o

obj/client.o: src/client.c
	gcc -c src/client.c -o obj/client.o

clean :
	rm -f obj/*.o
	rm -f lecteur
	rm -f audio*
