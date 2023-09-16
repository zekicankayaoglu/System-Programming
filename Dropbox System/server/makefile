all: BibakBOXServer

BibakBOXServer: server.o 
	gcc -o BibakBOXServer server.o

server.o: server.c
	gcc -c server.c -o server.o

clean:
	rm -f *.o