#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

typedef struct Client {
    int pid;
    char command[200];
    char fifoPath[20];
} Client;

int pidNumber;
//handles the SIGINT signal
void handle_sigint(int signal){
    struct Client data;
    printf("\nSIGINT handled.\n");
    strcpy(data.command,"kill");
    data.pid = pidNumber;
    int fd = open("/tmp/mainserver", O_WRONLY);//writes connection fifo that this client is killed
    write(fd, &data, sizeof(data));
    close(fd);
    usleep(200);
    exit(0);
}

int main(int argc, char *argv[]) {
    struct Client data;
    char filepath[30];
    
    snprintf(filepath, sizeof(filepath), "/tmp/%s", argv[2]);
    if (access(filepath, F_OK) != -1) {//checks the file name as serverPID
    } else {// if there is not, that means wrong server PID.
        printf("Could not connect to Server. Wrong Server PID!\n");
        exit(EXIT_FAILURE);
    }
    char pid[40];
    char temp[200];
    data.pid = getpid();
    pidNumber = data.pid;
    signal(SIGINT,handle_sigint);
    char fifoPath[50];
    sprintf(pid, "%d", data.pid);
    
    snprintf(fifoPath,sizeof(fifoPath),"/tmp/%s",pid);    
    strcpy(data.command, argv[1]);

    int fd = open("/tmp/mainserver", O_WRONLY);
    write(fd, &data, sizeof(data));
    close(fd);
    usleep(200);
    
    printf(">>Waiting for Queue..\n");
    //this loop for connection
    while(1){
        if (strcmp(data.command,"Connect") == 0 && access(fifoPath, F_OK) != -1) {
            break;
        }else if(strcmp(data.command,"tryConnect") == 0 && access(fifoPath, F_OK) != -1){
            int clientFifo = open(fifoPath, O_RDONLY);
            read(clientFifo,&temp,sizeof(temp));
            close(clientFifo);
            if(strcmp(temp,"full") == 0){
                printf(">>Queue was full!\n");//if queue is full it can not connect
                exit(EXIT_FAILURE);
            }else if(strcmp(temp,"empty") == 0){
                break;
            }
        }
    }
    memset(data.command, '\0', sizeof(data.command));
    printf(">>Connection established:\n");
    //now the connection established::
    while (1) {
            //gets comments then write them to connection fifo
            printf(">>Enter comment:");
            fgets(data.command, 100, stdin);
            strcpy(data.fifoPath,fifoPath);
            int fd = open("/tmp/mainserver", O_WRONLY);
            write(fd, &data, sizeof(data));
            close(fd);
            usleep(200);

            char data_copy[sizeof(data.command)];
            strcpy(data_copy,data.command);
            char *token = strtok(data.command," ");

            //now starts to read response and prints the outputs for client
            if(strcmp(data.command,"list\n") == 0){
                while(strcmp(temp,"filelistended") != 0){
                    int client = open(fifoPath,O_RDONLY);
                    read(client,&temp,200);
                    close(client);
                    if(strcmp(temp,"filelistended") != 0) printf("%s\n",temp);
                    usleep(100);
                    
                }
                memset(temp,'\0',sizeof(temp));
            }
            else if(strcmp(data.command,"help\n") == 0){
                
                int client = open(fifoPath,O_RDONLY);
                read(client,&temp,200);
                close(client);
                printf("%s\n",temp);
                usleep(800);                    
                memset(temp,'\0',sizeof(temp));
                usleep(300);
            }
            else if(strcmp(data.command,"quit\n") == 0){
                printf("Sending write request to server log file\n");
                printf("waiting for logfile...\n");
                printf("logfile write request granted\n");
                printf("bye\n");
                break;
            }else if(strcmp(data.command,"killServer\n") == 0){
                printf("bye..\n");
                break;
            }
            else if(strcmp(token,"readF") == 0){
                while(strcmp(temp,"end") != 0 && strcmp(temp,"error") != 0){
                    int client = open(fifoPath,O_RDONLY);
                    read(client,&temp,200);
                    close(client);
                    if(strcmp(temp,"end") != 0) printf("%s",temp);
                    if(strcmp(temp,"error") == 0) printf("File could not find!\n");
                    usleep(800);
                    
                }
                memset(temp,'\0',sizeof(temp));
            }else if(strcmp(token,"writeT") == 0){
                int client = open(fifoPath,O_RDONLY);
                read(client,&temp,200);
                close(client);
                if(strcmp(temp,"error") == 0){
                    printf("File could not find!\n");

                }else if(strcmp(temp,"written") == 0){
                    printf("Write operation is successful!\n");
                }
                memset(temp,'\0',sizeof(temp));
            }else if(strcmp(token,"upload") == 0){
                int client = open(fifoPath,O_RDONLY);
                read(client,&temp,200);
                close(client);
                if(strcmp(temp,"error") == 0){
                    printf("File could not find!\n");

                }else if(strcmp(temp,"uploaded") == 0){
                    printf("Upload completed successfully!\n");
                }
                memset(temp,'\0',sizeof(temp));
            }else if(strcmp(token,"download") == 0){
                int client = open(fifoPath,O_RDONLY);
                read(client,&temp,200);
                close(client);
                if(strcmp(temp,"error") == 0){
                    printf("File could not find!\n");

                }else if(strcmp(temp,"downloaded") == 0){
                    printf("Download completed successfully!\n");
                }
                memset(temp,'\0',sizeof(temp));
            }else if(strcmp(data.command,"killServer\n") == 0){
                printf("bye..\n");
                break;
            }else if(strcmp(token,"help") == 0){
                int client = open(fifoPath,O_RDONLY);
                read(client,&temp,200);
                close(client);
                printf("%s\n",temp);
                memset(temp,'\0',sizeof(temp));
            }
            else{
                printf("Invalid comment!!!\n");
            }
            memset(data.command, '\0', sizeof(data.command));
    }
    
    unlink(fifoPath);
    return 0;
}