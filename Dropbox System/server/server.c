#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>


#define BUFFER_SIZE 1024

struct Data{
    int cmdCode;
    char path[100];
    char buffer[1024];
    int flag;
    int socket;
};

typedef struct {
    int clientSocket;
    char clientPath[50];
    int count;
} clientQueue;

clientQueue clients[50];
int clientCounter = 0;
int client_out = 0;
int client_size = 5;
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_not_empty = PTHREAD_COND_INITIALIZER;
void* handleThread(void* args);
void checkDir(DIR *dir, char files[BUFFER_SIZE][BUFFER_SIZE], const char* clientPath,int clientSocket, int* counter, time_t fileTimes[BUFFER_SIZE],int* flag);
void deleteFile(int clientSocket, struct Data data);
void sendDeletedFile(int clientSocket,const char* filename);
void handleClient(int clientSocket, const char* clientPath);
void sendFileEvent(int clientSocket, const char* path, const char* filename, const char* event);
void getFile(int clientSocket, struct Data data);
void checkDelete(char files[BUFFER_SIZE][BUFFER_SIZE], char tempFiles[BUFFER_SIZE][BUFFER_SIZE], const char* clientPath, int* counter, time_t fileTimes[BUFFER_SIZE], int* flag);
char directory[50];

void signalHandler(int signal) {
    if (signal == SIGINT) {
        printf("SIGINT handled...\n");
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char *argv[]) {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientAddressLength;
    signal(SIGINT, signalHandler);
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Socket create error");
        exit(EXIT_FAILURE);
    }
    
    strcpy(directory,argv[1]);
    int portNumber = atoi(argv[3]);

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(portNumber);


    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("socket connection error");
        exit(EXIT_FAILURE);
    }
    int poolSize = atoi(argv[2]);

    if (listen(serverSocket, poolSize) == -1) {
        perror("Listen error");
        exit(EXIT_FAILURE);
    }
    printf("Server has started. Waiting for connections...\n");
    
    
    pthread_t threadPool[poolSize];
    for(int i = 0;i < poolSize; i++){
        if(pthread_create(&threadPool[i],NULL,handleThread,&clients) != 0){
            perror("thread create error\n");
            exit(EXIT_FAILURE);
        }
    }
    
    while(1){
             
        // Client bağlantısını kabul etme
        clientAddressLength = sizeof(clientAddress);
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressLength);
        if (clientSocket == -1) {
            perror("Accept error");
            exit(EXIT_FAILURE);
        }

        printf("Client connected successfuly. Client IP: %s\n", inet_ntoa(clientAddress.sin_addr));
        strcpy(clients[clientCounter].clientPath,directory);
        
        clients[clientCounter].clientSocket = clientSocket;
        clients[clientCounter].count = 1;
        clientCounter++;
        pthread_cond_signal(&buffer_not_empty);   
        
    }

    close(clientSocket);
    close(serverSocket);

    return 0;
}
void getDir(struct Data data){
    char dir[BUFFER_SIZE];
        //snprintf(data.path, sizeof(data.path), "%s/%s", path, filename);
        snprintf(dir, BUFFER_SIZE, "%s/%s",directory, data.path);
        //printf("path:%s\n",dir);
    mkdir(dir, 0777);
        //printf("Klasör oluşturuldu: %s\n", data.path);
}
void deleteDir(struct Data data){
    char dir[BUFFER_SIZE];
    snprintf(dir, BUFFER_SIZE, "%s/%s",directory, data.path);
        //printf("path:%s\n",dir);
    rmdir(dir);
        //printf("Klasör silindi: %s\n", data.path);
}
void* handleThread(void* args){
    clientQueue task;
    
    //continues to copy files until finish
    //printf("girdithread\n");
    while (1) {
        pthread_mutex_lock(&buffer_mutex);
        //printf("sa\n");

        while (clientCounter < 1) {
            pthread_cond_wait(&buffer_not_empty, &buffer_mutex);
        }
        //printf("gecti1\n");
        if (clientCounter > 0) {
            task = clients[0];
            //printf("out:%d\n",client_out);
            client_out = (client_out + 1) % client_size;
            clientCounter--;
            
        pthread_cond_signal(&buffer_not_full);
        }
        //printf("gecti2\n");
        //printf("beklio\n");
        pthread_mutex_unlock(&buffer_mutex);
        handleClient(task.clientSocket, task.clientPath);
        pthread_mutex_lock(&buffer_mutex);
        if (clientCounter == 0) {
            pthread_mutex_unlock(&buffer_mutex);
            break;
        }
        pthread_mutex_unlock(&buffer_mutex);
    }
    
    return NULL;
}

void checkDirFirstDir(DIR *dir, char files[BUFFER_SIZE][BUFFER_SIZE], const char* clientPath,int clientSocket, int* counter, time_t fileTimes[BUFFER_SIZE]){
    struct Data data;
    struct dirent *entry,*entry2;
    //printf("klasördir %s\n",clientPath);
    while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                char filePath[BUFFER_SIZE];
                char tempPath[BUFFER_SIZE];
                snprintf(filePath, BUFFER_SIZE, "%s/%s", clientPath, entry->d_name);
                //printf("filepath:%s\n",filePath);
                strcpy(tempPath,filePath);
                struct stat fileStat;
                if(stat(filePath, &fileStat) == -1){
                    perror("file stat error");
                }if (S_ISDIR(fileStat.st_mode)) { // Dosya bir klasör mü?
                // Alt klasöre gir
                DIR* subdir = opendir(filePath);
                if (subdir != NULL) {
                    size_t substring_length = strlen(directory)+1;
                char *result = strstr(filePath, directory);
                if (result != NULL) {
                    memmove(result, result + substring_length, strlen(result + substring_length) + 1);
                }
                
                //printf("result---->>>>>%s\n",result);
                    data.cmdCode = 4;
                    data.socket = 1;
                    strcpy(data.path, result);
                    send(clientSocket, &data, sizeof(data), 0);
                    strcpy(files[*counter],result);
                    fileTimes[*counter] = 1;
                    (*counter)++;
                    checkDirFirstDir(subdir, files, tempPath, clientSocket, counter, fileTimes);
                } else {
                    perror("Dizin açma hatası");
                }
            }else{}
            }
            
    }
    //printf("ciktiii\n");
    closedir(dir);
}

void checkDirFirst(DIR *dir, char files[BUFFER_SIZE][BUFFER_SIZE], const char* clientPath,int clientSocket, int* counter, time_t fileTimes[BUFFER_SIZE]){
    struct Data data;
    struct dirent *entry,*entry2;
    //printf("gedld\n");
    while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                char filePath[BUFFER_SIZE];
                char tempPath[BUFFER_SIZE];
                snprintf(filePath, BUFFER_SIZE, "%s/%s", clientPath, entry->d_name);
                //printf("filepath:%s\n",filePath);
                strcpy(tempPath,filePath);
                struct stat fileStat;
                if(stat(filePath, &fileStat) == -1){
                    perror("file stat error");
                }if (S_ISDIR(fileStat.st_mode)) { // Dosya bir klasör mü?
                // Alt klasöre gir
                DIR* subdir = opendir(filePath);
                if (subdir != NULL) {
                   
                    checkDirFirst(subdir, files, tempPath, clientSocket, counter, fileTimes);
                } else {
                    perror("Dizin açma hatası");
                }
            } else {
                //printf("file->%s\n",entry->d_name);
                // Dosya işlemleri
                 size_t substring_length = strlen(directory)+1;
                char *result = strstr(filePath, directory);
                if (result != NULL) {
                    memmove(result, result + substring_length, strlen(result + substring_length) + 1);
                }
                
                //printf("result---->>>>>%s\n",result);
                if (!strcmp(entry->d_name, "a.out") == 0) {
                    usleep(20);
                    data.cmdCode = 1;
                    strcpy(data.path, result);
                    send(clientSocket, &data, sizeof(data), 0);
                    sendFileEvent(clientSocket, clientPath, entry->d_name, "Yeni dosya");
                    time_t modifiedTime = fileStat.st_mtime;
                    strcpy(files[*counter], result);
                    fileTimes[*counter] = modifiedTime;
                    //printf("files:%s\n", files[*counter]);
                    (*counter)++;
                }
            }
            }
            
    }
    //printf("ciktiii\n");
    closedir(dir);
}

void checkDir(DIR *dir, char files[BUFFER_SIZE][BUFFER_SIZE], const char* clientPath,int clientSocket, int* counter, time_t fileTimes[BUFFER_SIZE], int* flag){
    struct dirent *entry;
    struct Data data;
    while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, "") != 0) {
                // Dosya adını ve son değiştirilme zamanını al
                char filePath[BUFFER_SIZE];
                char tempPath[BUFFER_SIZE];
                snprintf(filePath, BUFFER_SIZE, "%s/%s", clientPath, entry->d_name);
                //printf("filepath:%s\n",filePath);
                strcpy(tempPath,filePath);
                struct stat fileStat2;
                if(stat(filePath, &fileStat2) == -1){
                    perror("file stat error");
                }if (S_ISDIR(fileStat2.st_mode)) { // Dosya bir klasör mü?
                // Alt klasöre gir
                DIR* subdir = opendir(filePath);
                if (subdir != NULL) {
                    //printf("dir::%s\n",entry->d_name);
                    int found = 0;
                    int i,index;
                    size_t substring_length = strlen(directory)+1;
                char *result = strstr(filePath, directory);
                if (result != NULL) {
                    memmove(result, result + substring_length, strlen(result + substring_length) + 1);
                }
                
                //printf("result---->>>>>%s\n",result);
                    for (i = 0; i < BUFFER_SIZE; i++) {
                        if (strcmp(result, files[i]) == 0) {
                            found = 1;
                            index = i;
                        //printf("file:%s index:%d\n",files[i],index);
                            break;
                        }
                        }if(!found){
                            //printf("yeni klasörr\n");
                            data.cmdCode = 4;
                            data.socket = 1;
                            
                            strcpy(data.path, result);
                            send(clientSocket, &data, sizeof(data), 0);
                            strcpy(files[*counter],result);
                            fileTimes[*counter] = 1;
                            (*counter)++;
                        }
                
                    checkDir(subdir, files, tempPath, clientSocket, counter, fileTimes, flag);
                } else {
                    perror("Dizin açma hatası");
                }
            } else {
                int found = 0;
                int i,index;
                size_t substring_length = strlen(directory) +1;
                char *result = strstr(filePath, directory);
                if (result != NULL) {
                    memmove(result, result + substring_length, strlen(result + substring_length) + 1);
                }
                //printf("result---->>>>>%s\n",result);
                time_t newmodifiedTime = fileStat2.st_mtime;
                for (i = 0; i < BUFFER_SIZE; i++) {
                    if (strcmp(result, files[i]) == 0) {
                        found = 1;
                        index = i;
                        //printf("file:%s index:%d\n",files[i],index);
                        break;
                    }
                }
                if(fileTimes[index] == 0){
                    fileTimes[index] = newmodifiedTime;
                }
                if (found && fileTimes[index] != newmodifiedTime) {
                    //printf("girdi");
                    if(!strcmp(entry->d_name,"a.out") == 0){
                        //printf("girdi\n");
                        //printf("file:%ld,%ld\n",fileTimes[index],newmodifiedTime);
                        // Dosya değiştirildi
                        struct Data data;
                        data.cmdCode = 1;
                        data.socket = 1;
                        strcpy(data.path,result);
                        send(clientSocket, &data, sizeof(data), 0);
                        sendFileEvent(clientSocket, clientPath, entry->d_name, "Dosya değiştirildi");
                        fileTimes[index] = newmodifiedTime;
                        *flag = 1;
                        data.cmdCode = 0;

                    }
                } else if (!found) {
                    // Dosya eklendi
                    if(!strcmp(entry->d_name,"a.out") == 0){
                        strcpy(files[*counter],result);
                        fileTimes[*counter] = newmodifiedTime;
                        (*counter)++;
                        struct Data data;
                        data.cmdCode = 1;
                        data.socket = 1;
                        strcpy(data.path,result);
                        send(clientSocket, &data, sizeof(data), 0);
                        sendFileEvent(clientSocket, clientPath, entry->d_name, "Yeni dosya");
                        *flag = 1;
                        data.cmdCode = 0;
                    }
                }
            }
            }
        }
        
        closedir(dir);
}

void checkDelete(char files[BUFFER_SIZE][BUFFER_SIZE], char tempFiles[BUFFER_SIZE][BUFFER_SIZE], const char* clientPath, int* counter, time_t fileTimes[BUFFER_SIZE], int* flag){
    struct dirent *entry2;
    DIR *dir2;
    struct Data data;
    //printf("\n\ncounter:%d\n\n",*counter);
    //printf("file1:--******%s\n\n",files[0]);
  
 
        //printf("\nfile[%d]:%s\n",i,files[i]);   
                        dir2 = opendir(clientPath);
                        int find = 0;
                        while ((entry2 = readdir(dir2)) != NULL) {
                            
                            //printf("dosya gezio: %s\n",entry2->d_name);
                            if (strcmp(entry2->d_name, ".") != 0 && strcmp(entry2->d_name, "..") != 0) {
                                char filePath[BUFFER_SIZE];
                                char tempPath[BUFFER_SIZE];
                                snprintf(filePath, BUFFER_SIZE, "%s/%s", clientPath, entry2->d_name);
                                //printf("filepath:%s\n",filePath);
                                strcpy(tempPath,filePath);
                                //printf("filepath:%s\n",filePath);
                                struct stat fileStat2;
                                if(stat(filePath, &fileStat2) == -1){
                                    perror("file stat error");
                                }if (S_ISDIR(fileStat2.st_mode)) { // Dosya bir klasör mü?
                                // Alt klasöre gir
                                DIR* subdir = opendir(filePath);
                                if (subdir != NULL) {
                                    size_t substring_length = strlen(directory)+1;
                                    char *result = strstr(filePath, directory);
                                    if (result != NULL) {
                                        memmove(result, result + substring_length, strlen(result + substring_length) + 1);
                                    }
                                    
                                    //printf("result---->>>>>%s\n",result);
                                    strcpy(tempFiles[*counter],result);
                                    (*counter)++;
                                    checkDelete(files, tempFiles,tempPath, counter, fileTimes,flag);
                                } else {
                                    perror("Dizin açma hatası");
                                }
                            } else {
                                size_t substring_length = strlen(directory)+1;
                                char *result = strstr(filePath, directory);
                                if (result != NULL) {
                                    memmove(result, result + substring_length, strlen(result + substring_length) + 1);
                                }
                                
                                //printf("result---->>>>>%s\n",result);
                                strcpy(tempFiles[*counter],result);
                                //printf("temp:%s\n",tempFiles[*counter]);
                                (*counter)++;
                            }
                        }
                        }
                        
                        closedir(dir2);
                    
}

void handleClient(int clientSocket, const char* clientPath) {
    struct dirent *entry,*entry2;
    DIR *dir,*dir2;
    int flag = 1,fileNumber;
    int counter = 0,counter2 = 0;
    //printf("socket:%d, path:%s\n",clientSocket, clientPath);
    dir = opendir(clientPath);
    
    if (dir == NULL) {
        perror("Dizin açma hatası");
        exit(EXIT_FAILURE);
    }
    //printf("handle client girdi\n");
    char files[BUFFER_SIZE][BUFFER_SIZE];
    
    time_t fileTimes[BUFFER_SIZE];
    checkDirFirstDir(dir,files,clientPath,clientSocket,&counter,fileTimes);
    dir = opendir(clientPath);
    
    if (dir == NULL) {
        perror("Dizin açma hatası");
        exit(EXIT_FAILURE);
    }
    checkDirFirst(dir,files,clientPath,clientSocket,&counter,fileTimes); 
    //printf("gelliii\n");
    struct Data data;
    data.cmdCode = 3;
    data.socket = 1;
    send(clientSocket, &data, sizeof(data), 0);
    //int flags = fcntl(clientSocket, F_GETFL, 0);
      //  fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
    //printf("bekle%d\n",data.cmdCode);
    //printf("\n\nFİLE::%s\n\n",files[0]);
    flag = 0;
    while (1) {
        flag = 0;
        // Dizin içeriğini tarama
        dir = opendir(clientPath);
        if (dir == NULL) {
            perror("Dizin açma hatası");
            exit(EXIT_FAILURE);
        }
        
        checkDir(dir,files,clientPath,clientSocket,&counter,fileTimes,&flag);
        //printf("\n\nFİLE---<<>>>%s\n\n",files[0]);
        int x = 0;
        char tempFiles[BUFFER_SIZE][BUFFER_SIZE];
        memset(tempFiles,'\0',sizeof(tempFiles));
        checkDelete(files,tempFiles,clientPath,&x,fileTimes,&flag);  

        //printf("tempFiles::::%s\n",tempFiles[4]);     
        for (int i = 0; i < BUFFER_SIZE; i++) {
        int found = 0;  // dosyanın tempFiles içinde bulunup bulunmadığını gösteren bayrak
        //printf("filelar:%s\n",files[i]);
        // tempFiles dizisinde files[i] elemanını arama
        for (int j = 0; j < BUFFER_SIZE; j++) {
            if (strcmp(files[i], tempFiles[j]) == 0) {
                //printf("bulundu:%s\n",files[i]);
                found = 1;  // dosya tempFiles içinde bulundu
                break;
            }
        }
        
        // dosya tempFiles içinde bulunmadıysa işlem yapabilirsiniz
        if (!found) {
            if(!strcmp(files[i],"null") == 0){
                //printf("dosya silindi: %s\n",files[i]);
                struct Data data;
                                        data.cmdCode = 2;
                                        data.socket = 1;
                                        strcpy(data.path,files[i]);
                                        send(clientSocket, &data, sizeof(data), 0);
                                        //printf("aloalo\n");
                                        strcpy(files[i],"null");
                                        //printf("alooooo\n");
                                        flag = 1;
                                    }
        }
    }

            data.cmdCode = 0;
            //printf("flag:%d\n",flag);
            if(flag == 0){
                data.socket = 1;
                send(clientSocket, &data, sizeof(data), 0);
            }
                //printf("geldi\n");
                ssize_t bytesRead = recv(clientSocket, &data, sizeof(data), 0);
                
                //printf("cmd:%d\n",data.cmdCode);
                if(data.socket == 0){
                //printf("okuyooo\n");
                
                if (bytesRead == -1) {
                    perror("Mesaj alma hatası");
                    exit(EXIT_FAILURE);
                }
            
            if(data.cmdCode == 1){
                //usleep(100);
                //printf("1 : %s\n",data.path);
                int found = 0;
                for(int i = 0; i < BUFFER_SIZE; i++){
                    if(strcmp(data.path,files[i]) == 0){
                        found = 1;
                        break;
                    }
                }
                if(found){
                  
                    for(int i = 0; i < BUFFER_SIZE; i++){
                        if(strcmp(files[i],data.path) == 0){
                           
                            strcpy(files[i],"null");

                        }
                    }
                }
               
                   
                    //printf("data.path:%s",data.path);
                   
                    printf("File loaded: %s\n",data.path);
                    getFile(clientSocket,data);
                    strcpy(files[counter],data.path);
                    fileTimes[counter] = 0;
                    counter++;     
                    //printf("cikti\n");
                
                data.cmdCode = 0;
            }else if(data.cmdCode == 2){
                //usleep(100);
                int found = 0;
                for(int i = 0; i < BUFFER_SIZE; i++){
                    if(strcmp(data.path,files[i]) == 0){
                        found = 1;
                        break;
                    }
                }
                if(found){
                    printf("File deleted: %s\n",data.path);
                    deleteFile(clientSocket,data);
                    for(int i = 0; i < BUFFER_SIZE; i++){
                        if(strcmp(files[i],data.path) == 0){
                            strcpy(files[i],"null");
                        }
                    }
                }
                //printf("silinme gönderildi\n");
                data.cmdCode=0;
            }else if(data.cmdCode == 0){
                //printf("bekliyo\n");
            }else if(data.cmdCode == 4){
                //printf("bekliyo\n");
                //printf("girdiklasör\n");
                getDir(data);
            }
                }
            
           
        // 1 saniye bekleme
        sleep(1);
    }
}

void deleteFile(int clientSocket, struct Data data){
    
   char message[BUFFER_SIZE];
        //snprintf(data.path, sizeof(data.path), "%s/%s", path, filename);
        snprintf(message, BUFFER_SIZE, "%s/%s",directory, data.path);
    //printf("silinecek dosya:%s\n",message);
    remove(message);
}

void getFile(int clientSocket, struct Data data){
    //printf("girdi \n");
    int read_size;
    char buffer[BUFFER_SIZE];
    
    if(!strcmp(data.path,"") == 0){
        //ssize_t bytesRead = recv(serverSocket, &data, sizeof(struct Data), 0);
        char message[BUFFER_SIZE];
        //snprintf(data.path, sizeof(data.path), "%s/%s", path, filename);
        snprintf(message, BUFFER_SIZE, "%s/%s",directory, data.path);
        //printf("path:%s\n",message);
        FILE *file = fopen(message, "w");
        if (file == NULL) {
            perror("Dosya oluşturma hatası");
            exit(EXIT_FAILURE);
        }
            //printf("geldi\n");
        //printf("%d\n",data.cmdCode);
        while(1){
            //printf("wgileirid\n");
            
            ssize_t bytesRead = recv(clientSocket, &data, sizeof(struct Data), 0);
            //printf("%s\n",data.buffer);
            //printf("%d\n",data.cmdCode);
            if(data.flag == 0){
                break;
            }
            fputs(data.buffer,file);
        }
        //printf("bitti\n\n");   
        fclose(file);
    }
}

void sendDeletedFile(int clientSocket,const char* filename){
    struct Data data;
    data.cmdCode = 2;
    char message[BUFFER_SIZE];
    //printf("silinen dosya:%s",filename);
    strcpy(data.path,filename);
    data.socket = 1;
    send(clientSocket, &data, sizeof(struct Data), 0);
    
    
}

void sendFileEvent(int clientSocket, const char* path, const char* filename, const char* event) {
    struct Data data;
    data.cmdCode = 1;
    char message[BUFFER_SIZE];
    //printf("%s:\n",path);
    //snprintf(data.path, sizeof(data.path), "%s/%s", path, filename);
    snprintf(message, BUFFER_SIZE, "%s/%s",path, filename);
    //printf("path:%s\n",message);
    
    FILE *file = fopen(message, "r");
    if (file == NULL) {
        perror("Dosya okuma hatası");
        exit(EXIT_FAILURE);
    }
    size_t bytes_read;
    char buffer[BUFFER_SIZE];
    
    //  data.buffer = malloc(BUFFER_SIZE);
    while (fgets(data.buffer, sizeof(data.buffer), file) != NULL) {
        // Alınan veriyi bir dosyaya yazma
        //usleep(100);
        data.flag = 1;
        data.cmdCode = 1;
        
        send(clientSocket, &data, sizeof(struct Data), 0);
        
    }
    //printf("\n\n\n\nbitti\n\n\n\n");
    data.flag = 0;
    data.cmdCode = 0;
    data.socket = 1;
    send(clientSocket, &data, sizeof(data), 0);
    fclose(file);
}