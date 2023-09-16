all: BibakBOXClient

BibakBOXClient: client.o 
	gcc -o BibakBOXClient client.o

client.o: client.c
	gcc -c client.c -o client.o

clean:
	rm -f *.o