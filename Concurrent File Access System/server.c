#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <semaphore.h>
#include <signal.h>
#define SEM_NAME "/sem"

pthread_mutex_t mutex;
pthread_cond_t cond;
typedef struct Client {
    int pid;//keeps the client's pid number
    char command[200];//keeps the requests and response at the same time
    char fifoPath[20];//keeps the path of client fifo
    
} Client;
char my_log[500][200];//keeps the log informations
Client clientQueue[256];
int requestCount = 0,processCounter = 0, serverPID;
char dirname[30];
int clientNumber = 0,maxClient;
void getList(char fifoPath[20], char path[20]);
void helpCommands(char otherCommand[20], char fifoPath[20]);
void readF(char fifoPath[20], char *token2, char path[20]);
void writeT(char fifoPath[20], char *token2, char write_copy[200],char path[20]);
void upload(char *token2, char path[30], char fifoPath[20]);
void download(char *token2, char path[30], char fifoPath[20]);

sem_t *sem;
pid_t processPIDs[300];
int logCounter = 0;
//creates log file and fills it
void writeToLog(char log[500][200],char path[20]) {
    char filepath[30];
    snprintf(filepath,sizeof(filepath),"./%s/%d.log",path,serverPID);
    FILE* file = fopen(filepath, "a"); 
    time_t t = time(NULL);
    struct tm* currentTime = localtime(&t);
    for(int i = 0; i < logCounter; i++){
    fprintf(file, "[%02d-%02d-%04d %02d:%02d:%02d] %s\n", 
            currentTime->tm_mday, currentTime->tm_mon + 1, currentTime->tm_year + 1900,
            currentTime->tm_hour, currentTime->tm_min, currentTime->tm_sec, my_log[i]);
    }
    fclose(file); 
}

void getResponse(Client* client) {
    
    int semValue;
    char data_copy[sizeof(client->command)];
    strcpy(data_copy,client->command);
    char *token = strtok(client->command," ");

    //calls functions according to commands
    if(strcmp(client->command,"Connect") == 0){
        sem_getvalue(sem,&semValue);
        if(semValue == 0){
            printf(">>Connection request PID %d... Queue FULL!\n",client->pid);
                
        }
        sem_wait(sem);
        printf(">>Client PID %d connected.\n", client->pid);
        snprintf(my_log[logCounter],sizeof(my_log[logCounter]),">>Client PID %d connected.\n", client->pid);
        logCounter++;
        processPIDs[processCounter] = client->pid;
        processCounter++;
        char fifoFile[20];
        snprintf(fifoFile,sizeof(fifoFile),"/tmp/%d",client->pid);
        mkfifo(fifoFile,0666);
    } else if(strcmp(client->command,"tryConnect") == 0){
        sem_getvalue(sem,&semValue);
        if(semValue == 0){
            printf(">>Connection request PID %d... Queue FULL!\n",client->pid);
            char fifoFile[20];
            snprintf(fifoFile,sizeof(fifoFile),"/tmp/%d",client->pid);
            mkfifo(fifoFile,0666);
            int tempPid = open(fifoFile, O_WRONLY);
            write(tempPid, "full", 5);
            close(tempPid);
            usleep(800);
        }else{
            sem_wait(sem);
            printf(">>Client PID %d connected.\n", client->pid);
            char fifoFile[20];
            snprintf(my_log[logCounter],sizeof(my_log[logCounter]),">>Client PID %d connected.\n", client->pid);
            logCounter++;
            snprintf(fifoFile,sizeof(fifoFile),"/tmp/%d",client->pid);
            mkfifo(fifoFile,0666);
            int tempPid = open(fifoFile, O_WRONLY);
            write(tempPid, "empty", 6);
            close(tempPid);
            usleep(800);
            processPIDs[processCounter] = client->pid;
            processCounter++;   
        }
            
    }
    else if (strcmp(token, "list\n") == 0) {
        getList(client->fifoPath,dirname);

    }else if (strcmp(token, "help\n") == 0) {
        int tempPid = open(client->fifoPath, O_WRONLY);
        write(tempPid, "Available commands are:\nhelp, list, readF, writeT, upload, download, quit, killServer\n", 87);
        close(tempPid);
        usleep(800);

    }else if(strcmp(token,"help") == 0){
        char *token2;
        char otherCommand[20];
        token2 = strtok(data_copy," ");
        while(token2 != NULL){
            token2 = strtok(NULL," ");
            if(token2 != NULL){
                strcpy(otherCommand,token2);
            }
        }
        helpCommands(otherCommand,client->fifoPath);

    }else if(strcmp(token,"writeT") == 0){
        char *token2,*token3;
        char write_copy[200];
        strcpy(write_copy,data_copy);
        token2 = strtok(data_copy," ");
        writeT(client->fifoPath,token2,write_copy,dirname);

    }
    else if (strcmp(token, "readF") == 0) {
        char *token2;
        token2 = strtok(data_copy," ");
        readF(client->fifoPath,token2,dirname);

    }else if(strcmp(token,"upload") == 0){
        char *token2;
        token2 = strtok(data_copy," ");
        token2 = strtok(NULL," ");
        upload(token2,dirname,client->fifoPath);

    }else if(strcmp(token,"download") == 0){
        char *token2;
        token2 = strtok(data_copy," ");
        token2 = strtok(NULL," ");
        download(token2,dirname,client->fifoPath);

    }else if(strcmp(token,"quit\n") == 0){
        printf(">>Client PID %d disconnected..\n",client->pid);
        snprintf(my_log[logCounter],sizeof(my_log[logCounter]),">>Client PID %d disconnected.\n", client->pid);
        logCounter++;

        writeToLog(my_log, dirname);
        sem_post(sem);
    }else if(strcmp(token,"killServer\n") == 0){
        printf(">>kill signal from PID %d..terminating...\n",client->pid);
        printf(">>bye\n");
        snprintf(my_log[logCounter],sizeof(my_log[logCounter]),">>Client PID %d killed the server.\n", client->pid);
        
        logCounter++;
        sem_close(sem);
        sem_unlink(SEM_NAME);
        unlink("/tmp/mainserver");
        remove("temp.txt");
        remove("server.log");
        exit(EXIT_SUCCESS);
                    
    }else if(strcmp(token,"kill") == 0){
        printf(">>Client PID %d disconnected..\n",client->pid);
        snprintf(my_log[logCounter],sizeof(my_log[logCounter]),">>Client PID %d disconnected.\n", client->pid);
        logCounter++;
        sem_post(sem);
        unlink(client->fifoPath);
    }else{
        int tempPid = open(client->fifoPath, O_WRONLY);
        write(tempPid, "invalid", 8);
        close(tempPid);
        usleep(800);
    }
}

//keeps incoming request in order
void orderQueue(Client client) {
    pthread_mutex_lock(&mutex);
    clientQueue[requestCount] = client;
    requestCount++;
    if(requestCount != 0){
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&mutex);
    
}

//allows threads to execute request in order
void* process(void* args){
    while(1){
        Client client;
        pthread_mutex_lock(&mutex);
        while (requestCount == 0) {
            pthread_cond_wait(&cond, &mutex);
        }
        client = clientQueue[0];
            
        int i;
        for (i = 0; i < requestCount - 1; i++) {
            clientQueue[i] = clientQueue[i + 1];
        }
        requestCount--;
            
        pthread_mutex_unlock(&mutex);
        getResponse(&client);
    }
    
}

//handles SIGINT signal
void handle_sigint(int signal){
    printf("SIGINT handled.\n");
    for(int i = 0;i < processCounter; i++){
        kill(processPIDs[i], SIGTERM);
    }
    //deletes all opened files because program ends
    sem_close(sem);
    sem_unlink(SEM_NAME);
    unlink("/tmp/mainserver");
    remove("temp.txt");
    remove("server.log");
    exit(0);
}

int main(int argc, char *argv[]) {
    
    char pidStr[30],serverFifo[40];
    signal(SIGINT,handle_sigint);

    if(argc != 4){ //if usage of program is wrong
        printf("Usage: biboServer <dirname> <max.#ofClients><poolSize>\n");
        exit(EXIT_FAILURE);
    }
    
    strcpy(dirname,argv[1]);
    int poolSize = atoi(argv[3]);
    maxClient = atoi(argv[2]);
    
    sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0644, maxClient);
    pthread_t th[poolSize]; 

    int i;
    Client client;
    mkdir(argv[1],0777);
    mkfifo("/tmp/mainserver", 0666);
    int PID = getpid();
    serverPID = PID;
    printf(">>Server Started PID %d...\n", PID);
    printf(">>Waiting for clients...\n");
    
    sprintf(pidStr, "%d", PID);
    snprintf(serverFifo,sizeof(serverFifo),"/tmp/%s",pidStr);
    mkfifo(serverFifo, 0666);//creates a fifo named as server PID to check connections
    
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    for (i = 0; i < poolSize; i++) { //creates threads as poolSize
            pthread_create(&th[i], NULL, process, NULL);
    }
    //reads the requests then adds them to queue
    while (1) {
        int fd = open("/tmp/mainserver", O_RDONLY);
        read(fd, &client, sizeof(client));
        close(fd);
        usleep(500);
        orderQueue(client);
        
    }
    
    for(i = 0; i < poolSize; i++){
        pthread_join(th[i],NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}

//download function downloads a file from server path to client's path
void download(char *token2, char path[30], char fifoPath[20]){
    char fileName[30],file[20];
    strcpy(fileName,token2);
    strcpy(file,token2);
    if(file[strlen(file) - 1] == '\n') file[strlen(file) - 1] = '\0';

    FILE *readFile, *writeFile;
    char lines[500];
    
    snprintf(fileName,sizeof(fileName),"./%s/%s",path,file);
    readFile = fopen(fileName,"r");
    
    if(readFile == NULL){
        int tempPid = open(fifoPath, O_WRONLY);
        write(tempPid, "error", 6);
        close(tempPid);
        usleep(800);
    }else{
        writeFile = fopen(file,"w+");
        while(fgets(lines,500,readFile) != NULL){    
            fputs(lines,writeFile);
        }
        fclose(readFile);
        fclose(writeFile);
        unlink(fileName);
        int tempPid = open(fifoPath, O_WRONLY);
        write(tempPid, "downloaded", 11);
        close(tempPid);
        usleep(800);
    }
}

//upload files to server path
void upload(char *token2, char path[30], char fifoPath[20]){
    char fileName[30],file[20];
    strcpy(fileName,token2);
    strcpy(file,token2);
    if(file[strlen(file) - 1] == '\n') file[strlen(file) - 1] = '\0';
    FILE *readFile, *writeFile;
    char lines[500];
    readFile = fopen(file,"r");

    if(readFile == NULL){
        int tempPid = open(fifoPath, O_WRONLY);
        write(tempPid, "error", 6);
        close(tempPid);
        usleep(800);
    }else{
        snprintf(fileName,sizeof(fileName),"./%s/%s",path,file);
        writeFile = fopen(fileName,"w+");
        while(fgets(lines,500,readFile) != NULL){    
            fputs(lines,writeFile);
        }
        fclose(readFile);
        fclose(writeFile);
        unlink(file);
        int tempPid = open(fifoPath, O_WRONLY);
        write(tempPid, "uploaded", 9);
        close(tempPid);
        usleep(800);
    }
}

//writes the given string into given line of given file
void writeT(char fifoPath[20], char *token2, char write_copy[200], char path[20]){
    int count = 0;
    FILE *writeFile;
    FILE *tempFile;
    char filename[17];
    char lineString[20];
    char string[200];
    char *token3;
    char write_copy2[200];
    while(token2 != NULL && count < 4){
        token2 = strtok(NULL," ");
        count++;
    }
    
    int count2 = 0;
    char *token4;

    int lineNumber = -1;
    if(count == 4)  sscanf(write_copy, "writeT %s %d %[^\n]", filename, &lineNumber, string);
    if(count == 3)  sscanf(write_copy, "writeT %s %[^\n]", filename, string);
   
    char tempLine[500];
    char filename2[20];
    snprintf(filename2,sizeof(filename2),"./%s/%s",path,filename);
    writeFile = fopen(filename2,"r+");
    tempFile = fopen("temp.txt","w+");
    if(writeFile == NULL){
        int tempPid = open(fifoPath, O_WRONLY);
        write(tempPid, "error", 6);
        close(tempPid);
        usleep(800);
    }else{
        while(fgets(tempLine,500,writeFile) != NULL){
            fputs(tempLine,tempFile);
        }
        char lines[500];
                    
        fseek(writeFile,0,SEEK_SET);
        fseek(tempFile,0,SEEK_SET);
        int lineCounter = 1;
        while(fgets(lines,500,tempFile) != NULL){
            if(lineCounter == lineNumber){
                fputs(string,writeFile);
                fputs("\n",writeFile);
                                
            }
            fputs(lines,writeFile);
                            
            lineCounter++;
        }
        if(lineNumber == -1){
            fseek(writeFile,0,SEEK_END);
            fputs("\n",writeFile);
            fputs(string,writeFile);
        }
        fclose(writeFile);
        fclose(tempFile);
        int tempPid = open(fifoPath, O_WRONLY);
        write(tempPid, "written", 8);
        close(tempPid);
        usleep(800);
    }
}

//reads the line or all file of given file
void readF(char fifoPath[20], char *token2, char path[20]){
    int count = 0;
    char filename[17];
    char lineString[20];
    int lineNumber;
    while(token2 != NULL){
        token2 = strtok(NULL," ");
        if(token2 != NULL){
            if(count == 0) strcpy(filename,token2);
            if(count == 1) strcpy(lineString,token2);
            }
            count++;
        }
    if(count == 2) lineNumber = -1;
    else if(count == 3) lineNumber = atoi(lineString);

    FILE *readFile;
    char line[500];
    if(filename[strlen(filename) - 1] == '\n') filename[strlen(filename) - 1] = '\0';
    char filename2[20];
    snprintf(filename2,sizeof(filename2),"./%s/%s",path,filename);                
    readFile = fopen(filename2,"r");
                    
    if(readFile == NULL){
        int tempPid = open(fifoPath, O_WRONLY);
        write(tempPid, "error", 6);
        close(tempPid);
        usleep(800);
    }else{
        int currentLine = 1;
        while(fgets(line,500,readFile) != NULL){
            if(lineNumber != -1){
                if(currentLine == lineNumber){
                    int tempPid = open(fifoPath, O_WRONLY);
                    write(tempPid,&line, sizeof(line));
                    close(tempPid);
                    usleep(200);
                }
                currentLine++;
            }else{
                int tempPid = open(fifoPath, O_WRONLY);
                write(tempPid, &line, sizeof(line));
                close(tempPid);
                usleep(800);
            }
        }
        int tempPid = open(fifoPath, O_WRONLY);
        write(tempPid,"end", 4);
        close(tempPid);
        fclose(readFile);
    }
}


//helps to print "list" output
void getList(char fifoPath[20], char path[20]){
    struct dirent *de;
    char path2[20];
    snprintf(path2,sizeof(path2),"./%s",path);   
    DIR *dr = opendir(path2);
    while ((de = readdir(dr)) != NULL) {
        int tempPid = open(fifoPath, O_WRONLY);
        write(tempPid, &de->d_name, sizeof(de->d_name));
        close(tempPid);
        usleep(500);
    }
    rewinddir(dr);
    int tempPid = open(fifoPath, O_WRONLY);
    write(tempPid, "filelistended", 14);
    close(tempPid);
    usleep(500);
    closedir(dr);
}

//prints all help commands
void helpCommands(char otherCommand[20], char fifoPath[20]){
    if(otherCommand[strlen(otherCommand) - 1] == '\n') otherCommand[strlen(otherCommand) - 1] = '\0';
    if(strcmp(otherCommand,"readF") == 0){
        int tempPid = open(fifoPath, O_WRONLY);
        write(tempPid, "readF <file><line#>\n    display the #th line of the <file>, returns with an error if <file> does not exists", 108);
        close(tempPid);
        usleep(800);
    }else if(strcmp(otherCommand,"help") == 0){
        int tempPid = open(fifoPath, O_WRONLY);
        write(tempPid, "display the list of possible client requests", 45);
        close(tempPid);
        usleep(800);
    }else if(strcmp(otherCommand,"list") == 0){
        int tempPid = open(fifoPath, O_WRONLY);
        write(tempPid, "sends a request to display the list of files in Servers directory", 66);
        close(tempPid);
        usleep(800);
    }else if(strcmp(otherCommand,"writeT") == 0){
        int tempPid = open(fifoPath, O_WRONLY);
        write(tempPid, "writeT <file><line#><string>\n    request to write the string to the #th line the <file>, if the line is not given writes to end of file\n", 137);
        close(tempPid);
        usleep(800);
    }
    else if(strcmp(otherCommand,"upload") == 0){
        int tempPid = open(fifoPath, O_WRONLY);
        write(tempPid, "upload<file>\n    uploads the file from the current working directory of client to the Servers directory\n", 105);
        close(tempPid);
        usleep(800);
    }
    else if(strcmp(otherCommand,"download") == 0){
        int tempPid = open(fifoPath, O_WRONLY);
        write(tempPid, "download<file>\n    request to receive <file> from Servers directory to client side\n", 84);
        close(tempPid);
        usleep(800);
    }else if(strcmp(otherCommand,"quit") == 0){
        int tempPid = open(fifoPath, O_WRONLY);
        write(tempPid, "Send write request to Server side log file and quits\n", 54);
        close(tempPid);
        usleep(800);
    }
    else if(strcmp(otherCommand,"killServer") == 0){
        int tempPid = open(fifoPath, O_WRONLY);
        write(tempPid, "Sends a kill request to Server\n", 32);
        close(tempPid);
        usleep(800);
    }
}