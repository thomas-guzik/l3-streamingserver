all : audioserver audioclient lecteur

audioclient : obj/client.o obj/audio.o obj/lecteur.o obj/socketlvl2.o
	gcc obj/client.o obj/audio.o obj/lecteur.o obj/socketlvl2.o -o  audioclient

audioserver : obj/server.o obj/audio.o obj/lecteur.o obj/socketlvl2.o
	gcc obj/server.o obj/audio.o obj/lecteur.o obj/socketlvl2.o -o audioserver
	
lecteur : obj/lecteur.o obj/audio.o
	gcc -c obj/lecteur.o -o lecteur

obj/lecteur.o : src/lecteur.c
	gcc -c src/lecteur.c -o obj/lecteur.o
	
obj/socketlvl2.o : src/socketlvl2.c
	gcc -c src/socketlvl2.c -o obj/socketlvl2.o

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
