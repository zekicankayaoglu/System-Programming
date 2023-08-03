all: biboServer biboClient

biboServer: server.o
	gcc -o biboServer server.o
biboClient: client.o
	gcc -o biboClient client.o
server.o: server.c
	gcc -c server.c -o server.o
client.o: client.c
	gcc -c client.c -o client.o
clean:
	rm -f *.o biboServer biboClient