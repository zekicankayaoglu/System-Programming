#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>


int main(int argc, char *argv[]){
    char *fileName;
    int numBytes,checklseek;

    if(argc != 4 && argc != 3){ //checks command line
        printf("Wrong usage!!!\n");
        printf("You must use appendMeMore as: 'appendMeMore filename num-bytes [x]'\n");
        exit(EXIT_FAILURE);
    }

    fileName = argv[1]; //takes the filename
    sscanf(argv[2], "%d" ,&numBytes); //converst byte string to integer

    if(argc == 4 && argv[3][0] == 'x'){ //checks for x
        checklseek = 1;
    }

    int checks = O_WRONLY | O_CREAT;
    int fd; //file descriptor

    if(checklseek == 1){
        fd = open(fileName,checks | 0, 0644);  //x is supplied so O_APPEND omitted

    }
    if(checklseek == 0){//if x is not supplied:
        fd = open(fileName,checks | O_APPEND,0644); 
    }

    if(fd == -1){ //error for opening file
        perror("Error: ");
        exit(EXIT_FAILURE);
    }

    off_t offset = lseek(fd,0,SEEK_END); //if x is supplied should perform lseek before each write

    if(checklseek == 1){
        for(int i = 0; i < numBytes; i++){
            if(lseek(fd, offset+i, SEEK_SET) == -1){
                perror("lseek: ");
                exit(EXIT_FAILURE);
            }
            if(write(fd, "x", 1)!=1){
                perror("Write: ");
                exit(EXIT_FAILURE);
            }
        }
    }

    if(checklseek == 0){ //if x is not supplied
        for(int i = 0; i < numBytes; i++){
            if(write(fd, "x", 1)!=1){
                perror("Write: ");
                exit(EXIT_FAILURE);
            }
        }
    }
}