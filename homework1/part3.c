#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

//my dup functions::

int dup(int fd1){
    int fd2 = fcntl(fd1, F_DUPFD, 0);
    if(fd2 == -1){
        perror("File Error: ");
        return -1;
    }
    return fd2;
}

int dup2(int fd1, int fd2){
    if(fd1 == fd2){
        if(fcntl(fd1,F_GETFL) == -1){ //checks is oldfd valid or not
            errno = EBADF; //if not gives error
            return -1;
        }
        return fd1;
    }
    if(close(fd2) == -1){
        return -1;
    }

    int duplicated = fcntl(fd1,F_DUPFD,fd2);
    if(duplicated == -1){
        return -1;
    }

    return duplicated;
}

//Test main to verify that duplicated file descriptors share a file offset value 
int main(){
    //test for dup function:
    int fd,duplicated;
    char buf1[20],buf2[20];
    fd = open("testpart3.txt",O_CREAT|O_TRUNC|O_RDWR,0644);
    write(fd,"hello",5);

    duplicated = dup(fd);

    printf("offset:%ld\n",lseek(fd,0,SEEK_CUR));
    printf("offset:%ld\n",lseek(duplicated,0,SEEK_CUR));

    write(fd,"hello again",11);
    printf("offset:%ld\n",lseek(fd,0,SEEK_CUR));
    printf("offset:%ld\n",lseek(duplicated,0,SEEK_CUR));

    lseek(fd,0,SEEK_SET);
    read(fd,buf1,sizeof(buf1));
    lseek(duplicated,0,SEEK_SET);
    read(duplicated,buf2,sizeof(buf2));

    printf("buf1(fd1.txt): %s\n",buf1);
    printf("buf2(fd3.txt): %s\n",buf2);  

    printf("return: %d\n",fcntl(fd,F_GETFL));
    printf("return dup:%d\n",fcntl(duplicated,F_GETFL));

    close(fd);
    close(duplicated);
    unlink("testpart3.txt");

    //test for dup2 function::
    int fd2,fd3,duplicated2;
    char buf3[20],buf4[20];
    fd2 = open("testpart4.txt",O_CREAT|O_TRUNC|O_RDWR,0644);
    fd3 = open("testpart5.txt",O_CREAT|O_TRUNC|O_RDWR,0644);
    write(fd2,"hello",5);

    duplicated2 = dup2(fd2,fd3);
    //checking offsets
    printf("offset fd2:%ld\n",lseek(fd2,0,SEEK_CUR));
    printf("offset fd3:%ld\n",lseek(fd3,0,SEEK_CUR));
    printf("offset duplicated:%ld\n",lseek(duplicated2,0,SEEK_CUR));
    //checking contents of files
    lseek(fd3,0,SEEK_SET);
    read(fd3,buf3,sizeof(buf3));
    lseek(duplicated2,0,SEEK_SET);
    read(duplicated2,buf4,sizeof(buf4));

    printf("buf1(fd3): %s\n",buf3);
    printf("buf2(duplicated2): %s\n",buf4);  
    close(fd2);
    close(fd3);
    close(duplicated2);
    unlink("testpart4.txt");
    unlink("testpart5.txt");

}
