all : obj audioserver audioclient test

audioclient : obj/client.o obj/audio.o obj/lecteur.o obj/socketlvl2.o
	gcc obj/client.o obj/audio.o obj/lecteur.o obj/socketlvl2.o -o  audioclient

audioserver : obj/server.o obj/audio.o obj/lecteur.o obj/socketlvl2.o
	gcc obj/server.o obj/audio.o obj/lecteur.o obj/socketlvl2.o -o audioserver
	
test : obj/test.o obj/lecteur.o obj/audio.o
	gcc obj/test.o obj/lecteur.o obj/audio.o -o test

obj/client.o: src/client.c
	gcc -c src/client.c -o obj/client.o

obj/server.o: src/server.c
	gcc -c src/server.c -o obj/server.o

obj/lecteur.o : src/lecteur.c
	gcc -c src/lecteur.c -o obj/lecteur.o

obj/test.o : src/test.c
	gcc -c src/test.c -o obj/test.o

obj/audio.o : sysprog-audio-1.5/audio.c
	gcc -c sysprog-audio-1.5/audio.c -o obj/audio.o

obj/socketlvl2.o : src/socketlvl2.c
	gcc -c src/socketlvl2.c -o obj/socketlvl2.o

obj:
	mkdir -p obj

clean :
	rm -f obj/*.o
	rm -f audioclient audioserver test
