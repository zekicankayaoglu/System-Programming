#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


int dup(int fd1){
    int fd2;
    fd2 = fcntl(fd1, F_DUPFD, 0);
    if(fd2 == -1){
        perror("File Error: ");
        return -1;
    }
    return fd2;
}

int dup2(int fd1, int fd2){

    if(fd1 == fd2){//special case where oldfd equals newfd
        if(fcntl(fd1,F_GETFL) == -1){ //checks is oldfd valid or not
            errno = EBADF; //if not gives error
            return -1;
        }
        return fd1;
    }
    if(close(fd2) == -1){//close newfd
        return -1;
    }

    int duplicated;
    duplicated = fcntl(fd1,F_DUPFD,fd2); //creating duplicated file descriptor
    if(duplicated == -1){
        return -1;
    }

    return duplicated;
}
//test function
int main(){
    char buf1[20],buf2[20];
    //test cases for dup and dup2 functions ::  
    int fd1 = open("fd1.txt",O_CREAT|O_RDWR|O_APPEND,0644);
    int fd2 = open("fd2.txt",O_CREAT|O_RDWR|O_APPEND,0644);
    int fd3 = open("fd3.txt",O_CREAT|O_RDWR,0644);
    int copy = dup2(fd1,fd2);
    int copy2 = dup(fd3);
    printf("fd1: %d fd2: %d copy: %d\n",fd1,fd2,copy);
    printf("fd3: %d copy2: %d\n", fd3,copy2);
    write(copy2,"hello",5);
    write(copy,"hello world",11);
    lseek(fd1,0,SEEK_SET);
    lseek(fd3,0,SEEK_SET);
    read(fd1,buf1,sizeof(buf1));
    read(fd3,buf2,sizeof(buf2));

    printf("buf1(fd1.txt): %s\n",buf1);
    printf("buf2(fd3.txt): %s\n",buf2);    
    return 0;
}