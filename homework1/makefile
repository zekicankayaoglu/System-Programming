all: appendMeMore duplicate duplicate2

appendMeMore: part1.o 
	gcc -o appendMeMore part1.o
duplicate: part2.o 
	gcc -o duplicate part2.c
duplicate2: part3.o
	gcc -o duplicate2 part3.c
part1.o: part1.c
	gcc -c part1.c -o part1.o

part2.o: part2.c
	gcc -c part2.c -o part2.o

part3.o: part3.c
	gcc -c part3.c -o part3.o

clean:
	rm -f *.o appendMeMore duplicate duplicate2
