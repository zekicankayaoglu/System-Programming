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
#include <signal.h>

#define BUFFER_SIZE 1024
struct Data{
    int cmdCode;
    char path[100];
    char buffer[1024];
    int flag;
    int socket;
};

void deleteFile(int serverSocket, struct Data data);
void checkDir(DIR *dir, char files[BUFFER_SIZE][BUFFER_SIZE], const char* serverPath,int clientSocket, int* counter, time_t fileTimes[BUFFER_SIZE],int* flag);
void checkDelete(char files[BUFFER_SIZE][BUFFER_SIZE], char tempFiles[BUFFER_SIZE][BUFFER_SIZE], const char* serverPath, int* counter, time_t fileTimes[BUFFER_SIZE], int* flag);
char directory[50];
void handleServer(int serverSocket, const char* serverPath);
void sendFileEvent(int serverSocket, const char* path, const char* filename, const char* event);
void getFile(int serverSocket, struct Data data);
void getDir(struct Data data);



void signalHandler(int signal) {
    if (signal == SIGINT) {
        printf("SIGINT handled...\n");
        exit(EXIT_SUCCESS);
    }
}
int main(int argc, char *argv[]) {
    struct Data data;
    int serverSocket;
    struct sockaddr_in serverAddress;
    signal(SIGINT, signalHandler);
    strcpy(directory,argv[1]);
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Socket create error");
        exit(EXIT_FAILURE);
    }
    int portNumber = atoi(argv[2]);
    char ip[20];
    serverAddress.sin_family = AF_INET;
    
    serverAddress.sin_port = htons(portNumber);
    if(argc > 3){
        strcpy(ip,argv[3]);
        printf("%s\n", ip);
    if (inet_pton(AF_INET, ip, &(serverAddress.sin_addr)) <= 0) {
        perror("Invalid IP address");
        return 1;
    }
    }
        else{
            serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
        }
    // Server adres ve port yapılandırması
    
    
    
    // Server'a bağlanma
    if (connect(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Server connection error");
        exit(EXIT_FAILURE);
    }

    printf("*** Connected to Server ***.\n");
    
    // Server'dan gelen mesajı okuma
    int flag = 0;
    int flag2 = 0;
    while(data.cmdCode != 3){
        if(flag == 0){
            ssize_t bytesRead = recv(serverSocket, &data, sizeof(data), 0);
            //printf("okuyooo\n");
            if (bytesRead == -1) {
                perror("Socket message error");
                exit(EXIT_FAILURE);
            }
        }
        //printf("code:%d\n",data.cmdCode);
        if(data.cmdCode == 1){
            //usleep(100);
            //flag = 1;
            //printf("girdi1\n");
            //printf("geldi dosya\n");
            getFile(serverSocket,data);
            //data.cmdCode = 0;
            //printf("cikti%d\n",data.cmdCode);
            //flag = 0;
        }else if(data.cmdCode == 2){
            //printf("silinme geldi\n");
            //usleep(100);
            flag = 1;
            deleteFile(serverSocket,data);
            //printf("silinme gönderildi\n");
            flag = 0;
            //data.cmdCode = 0;
        }else if(data.cmdCode == 0){
            flag = 0;
            //printf("bekliyo\n");
        }else if(data.cmdCode == 3){
            //printf("girdi3\n");
            break;
            
        }else if(data.cmdCode == 4){
            //printf("girdi3\n");
            //printf("4girdi444\n");
            getDir(data);
            
        }
        //printf("coımmand: %d\n",data.cmdCode);
    }   
    //printf("coımmand: %d\n",data.cmdCode);
    
    //int flags = fcntl(serverSocket, F_GETFL, 0);
        //fcntl(serverSocket, F_SETFL, flags | O_NONBLOCK);
    handleServer(serverSocket,directory); 
    // Server ile iletişimi işleme
    

    // Socket'i kapatma
    close(serverSocket);

    return 0;
}
void deleteDir(struct Data data){
    char dir[BUFFER_SIZE];
    snprintf(dir, BUFFER_SIZE, "%s/%s",directory, data.path);
        //printf("path:%s\n",dir);
    rmdir(dir);
        //printf("Klasör silindi: %s\n", data.path);
}
void deleteFile(int serverSocket, struct Data data){
    char message[BUFFER_SIZE];
        //snprintf(data.path, sizeof(data.path), "%s/%s", path, filename);
        snprintf(message, BUFFER_SIZE, "%s/%s",directory, data.path);
    //printf("silinecek dosya:%s\n",message);
    remove(message);
}

void getFile(int serverSocket, struct Data data){
    //printf("girdi \n");
    int read_size;
    char buffer[BUFFER_SIZE];
    if(!strcmp(data.path,"") == 0){
        //ssize_t bytesRead = recv(serverSocket, &data, sizeof(struct Data), 0);
        //printf("file:%s\n",data.path);
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
            
            ssize_t bytesRead = recv(serverSocket, &data, sizeof(struct Data), 0);
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

void checkDir(DIR *dir, char files[BUFFER_SIZE][BUFFER_SIZE], const char* serverPath,int serverSocket, int* counter, time_t fileTimes[BUFFER_SIZE], int* flag){
    struct dirent *entry;
    struct Data data;
    //printf("budir\n");
    dir = opendir(serverPath);
        if (dir == NULL) {
            perror("Dizin açma hatası");
            exit(EXIT_FAILURE);
        }
    while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, "") != 0) {
                // Dosya adını ve son değiştirilme zamanını al
                 char filePath[BUFFER_SIZE];
                char tempPath[BUFFER_SIZE];
                snprintf(filePath, BUFFER_SIZE, "%s/%s", serverPath, entry->d_name);
                //printf("filepath:%s\n",filePath);
                strcpy(tempPath,filePath);
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
                    //printf("dir::%s\n",entry->d_name);
                    int found = 0;
                    int i,index;
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
                            data.socket = 0;
                            
                            strcpy(data.path, result);
                            send(serverSocket, &data, sizeof(data), 0);
                            strcpy(files[*counter],result);
                            fileTimes[*counter] = 1;
                            (*counter)++;
                        }
                
                    checkDir(subdir, files, tempPath, serverSocket, counter, fileTimes, flag);
                } else {
                    perror("Dizin açma hatası");
                }
            } else {
                int found = 0;
                int i,index;
               size_t substring_length = strlen(directory)+1;
                char *result = strstr(filePath, directory);
                if (result != NULL) {
                    memmove(result, result + substring_length, strlen(result + substring_length) + 1);
                }
                
               
                time_t newmodifiedTime = fileStat2.st_mtime;
                for (i = 0; i < BUFFER_SIZE; i++) {
                    if (strcmp(result, files[i]) == 0) {
                        found = 1;
                        index = i;
                        //printf("file:%s index:%ld\n",files[i],fileTimes[index]);
                        break;
                    }
                }
                if(fileTimes[index] == 0){
                    //printf("girmedi\n");
                    fileTimes[index] = newmodifiedTime;
                }
                if (found && fileTimes[index] != newmodifiedTime) {
                    //printf("girdi:%ld",fileTimes[index]);

                    if(!strcmp(entry->d_name,"a.out") == 0){
                        //printf("girdi\n");
                        //printf("file:%ld,%ld\n",fileTimes[index],newmodifiedTime);
                        // Dosya değiştirildi
                        struct Data data;
                        data.cmdCode = 1;
                        data.socket = 0;
                        strcpy(data.path,result);
                        send(serverSocket, &data, sizeof(data), 0);
                        sendFileEvent(serverSocket, serverPath, entry->d_name, "Dosya değiştirildi");
                        fileTimes[index] = newmodifiedTime;
                        *flag = 1;
                        data.cmdCode = 0;

                    }
                } else if (!found) {
                    // Dosya eklendi
                   
                    if(!strcmp(entry->d_name,"a.out") == 0){
                        strcpy(files[*counter],result);
                         //printf("result---->>>>>%s\n",result);
                        fileTimes[*counter] = newmodifiedTime;
                        (*counter)++;
                        struct Data data;
                        data.cmdCode = 1;
                        data.socket = 0;
                        strcpy(data.path,result);
                        send(serverSocket, &data, sizeof(data), 0);
                        sendFileEvent(serverSocket, serverPath, entry->d_name, "Yeni dosya");
                        *flag = 1;
                        data.cmdCode = 0;
                    }
                }
            }
            }
        }
        
        closedir(dir);
}
void checkDelete(char files[BUFFER_SIZE][BUFFER_SIZE], char tempFiles[BUFFER_SIZE][BUFFER_SIZE], const char* serverPath, int* counter, time_t fileTimes[BUFFER_SIZE], int* flag){
    struct dirent *entry2;
    DIR *dir2;
    struct Data data;
    //printf("budelete\n");
    //printf("\n\ncounter:%d\n\n",*counter);
    //printf("file1:--******%s\n\n",files[0]);
  
 
        //printf("\nfile[%d]:%s\n",i,files[i]);   
                        dir2 = opendir(serverPath);
                        int find = 0;
                        while ((entry2 = readdir(dir2)) != NULL) {
                            
                            //printf("dosya gezio: %s\n",entry2->d_name);
                            if (strcmp(entry2->d_name, ".") != 0 && strcmp(entry2->d_name, "..") != 0) {
                                char filePath[BUFFER_SIZE];
                                char tempPath[BUFFER_SIZE];
                                snprintf(filePath, BUFFER_SIZE, "%s/%s", serverPath, entry2->d_name);
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
                                //printf("-->%s\n",tempFiles[*counter]);
                                (*counter)++;
                            }
                        }
                        }
                        
                        closedir(dir2);
                    
}

void checkDirFirst(DIR *dir, char files[BUFFER_SIZE][BUFFER_SIZE], const char* serverPath,int serverSocket, int* counter, time_t fileTimes[BUFFER_SIZE]){
    struct Data data;
    struct dirent *entry,*entry2;
    //printf("gedld\n");
    while ((entry = readdir(dir)) != NULL) {
        //printf("girmiyooo\n");
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                char filePath[BUFFER_SIZE];
                char tempPath[BUFFER_SIZE];
                snprintf(filePath, BUFFER_SIZE, "%s/%s", serverPath, entry->d_name);
                    //printf("first::filepath:%s\n",filePath);
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
                    strcpy(files[*counter],result);
                    fileTimes[*counter] = 1;
                    (*counter)++;
                    checkDirFirst(subdir, files, tempPath, serverSocket, counter, fileTimes);
                } else {
                    perror("Dizin açma hatası");
                }
            } else {
                //printf("file->%s\n",entry->d_name);
                // Dosya işlemleri
                if (!strcmp(entry->d_name, "a.out") == 0) {
                    size_t substring_length = strlen(directory)+1;
                                    char *result = strstr(filePath, directory);
                                    if (result != NULL) {
                                        memmove(result, result + substring_length, strlen(result + substring_length) + 1);
                                    }
                    time_t modifiedTime = fileStat.st_mtime;
                    strcpy(files[*counter],result);
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

void handleServer(int serverSocket, const char* serverPath) {
    struct dirent *entry,*entry2;
    DIR *dir,*dir2;
    int flag = 1,fileNumber;
    int counter = 0,counter2 = 0;
    dir = opendir(serverPath);
    if (dir == NULL) {
        perror("Dizin açma hatası");
        exit(EXIT_FAILURE);
    }
    //printf("girdi handle server\n");
    char files[BUFFER_SIZE][BUFFER_SIZE];
    struct Data data;
    time_t fileTimes[BUFFER_SIZE];
    checkDirFirst(dir,files,serverPath,serverSocket,&counter,fileTimes); 
    
   //printf("bekle\n");
    
    while(1){
        flag = 0;
        // Dizin içeriğini tarama
        dir = opendir(serverPath);
    if (dir == NULL) {
        perror("Dizin açma hatası");
        exit(EXIT_FAILURE);
    }
        checkDir(dir,files,serverPath,serverSocket,&counter,fileTimes,&flag);
        char tempFiles[BUFFER_SIZE][BUFFER_SIZE];
        int x = 0;
        //printf("yeniins:%s\n",files[counter-1]);
        memset(tempFiles,'\0',sizeof(tempFiles));
        checkDelete(files,tempFiles,serverPath,&x,fileTimes,&flag);  
        //printf("tempFiles::::%s\n",tempFiles[0]);     
        for (int i = 0; i < BUFFER_SIZE; i++) {
        int found = 0;  // dosyanın tempFiles içinde bulunup bulunmadığını gösteren bayrak
        //printf("filelar:%s\n",files[i]);
        // tempFiles dizisinde files[i] elemanını arama
        for (int j = 0; j < BUFFER_SIZE; j++) {
            //printf("tempa:%s\n",tempFiles[i]);
            if (strcmp(files[i], tempFiles[j]) == 0) {
                found = 1;  // dosya tempFiles içinde bulundu
                if(!strcmp(files[i],"") == 0){
                    //printf("filei:%s\n",files[i]);
                }
                break;
            }
        }
        
        // dosya tempFiles içinde bulunmadıysa işlem yapabilirsiniz
        if (!found) {
            if(!strcmp(files[i],"null") == 0){
                //printf("dosya silindi: %s\n",files[i]);
                struct Data data;
                data.cmdCode = 2;
                                        data.socket = 0;
                                        strcpy(data.path,files[i]);
                                        send(serverSocket, &data, sizeof(data), 0);
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
            data.socket = 0;
            send(serverSocket, &data, sizeof(data), 0);
        }
        //printf("gectiii\n");      
                //printf("geldi\n");
                ssize_t bytesRead = recv(serverSocket, &data, sizeof(data), 0);
                //printf("cmd:%d\n",data.cmdCode);
                if(data.socket == 1){
                //printf("okuyooo\n");
                
                if (bytesRead == -1) {
                    perror("Mesaj alma hatası");
                    exit(EXIT_FAILURE);
                }
            if(data.cmdCode == 1){
                //usleep(100);
               
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
                    //printf("File loaded: %s\n",data.path);
                    getFile(serverSocket,data);
                    
                    
                    printf("File loaded: %s\n",data.path);
                    //printf("dosyaaa:%s\n",data.path);
                    strcpy(files[counter],data.path);
                    fileTimes[counter] = 0;
                    //printf("dosyanintime:%ld",fileTimes[counter]);
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
                    deleteFile(serverSocket,data);
                    //printf("data.path:%s\n",data.path);
                    
                    
                    
                    for(int i = 0; i < BUFFER_SIZE; i++){
                        if(strcmp(files[i],data.path) == 0){
                            //printf("files[]:%s\n",files[i]);
                            strcpy(files[i],"null");

                        }
                    }
                }
                data.cmdCode = 0;
                //printf("silinme gönderildi\n");
                
            }else if(data.cmdCode == 0){
                //printf("bekliyo\n");
            }else if(data.cmdCode == 4){
                //printf("bekliyo\n");
                //printf("girdiklasör\n");
                getDir(data);
            }
                }
        
     
       
      
        sleep(1); 
    }
    
}

void sendFileEvent(int serverSocket, const char* path, const char* filename, const char* event) {
    struct Data data;
    data.cmdCode = 1;
    char message[BUFFER_SIZE];
    
    //snprintf(data.path, sizeof(data.path), "%s/%s", path, filename);
    snprintf(message, BUFFER_SIZE, "%s/%s" ,path, filename);
    //printf("alopath:%s\n",message);
    
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
        send(serverSocket, &data, sizeof(struct Data), 0);
        
    }
    //printf("\n\n\n\nbitti\n\n\n\n");
    data.flag = 0;
    data.cmdCode = 0;
    data.socket = 0;
    send(serverSocket, &data, sizeof(data), 0);
    fclose(file);
    //printf("gggg\n");
}

void getDir(struct Data data){
    //printf("dirr\n");
    char message[BUFFER_SIZE];
        //snprintf(data.path, sizeof(data.path), "%s/%s", path, filename);
        snprintf(message, BUFFER_SIZE, "%s/%s",directory, data.path);
        //printf("path:%s\n",message);
    mkdir(message, 0777);
        //printf("Klasör oluşturuldu: %s\n", data.path);
}