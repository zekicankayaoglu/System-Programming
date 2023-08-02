#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>

void terminal();
void sigint_handler(int sig){
    printf("\nSIGINT signal occured!!! \n");
    printf("\n");
    printf("Zekican's Terminal$ ");
    
    fflush(stdout);
}

void sigtstp_handler(int sig){
    printf("\nSIGTSTP signal occured!!! \n");
    printf("\n");
    printf("Zekican's Terminal$ ");
    fflush(stdout);
}
//creates log file for each execution
void log_command(pid_t pid, char* command) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char filename[64];

    snprintf(filename, sizeof(filename), "%d-%02d-%02d_%02d-%02d-%02d.log", 
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    FILE* log_file = fopen(filename, "a");
    
    if (log_file == NULL) {
        perror("fopen error");
        exit(EXIT_FAILURE);
    }
    fprintf(log_file, "PID: %d\nCommand: %s\n", pid, command);
    fclose(log_file);
}

void run_multiple_commands(char** commands, int num_commands);

// parse function to pares command lines
void parse(char *line){
    char **commands = (char**) malloc(64 * sizeof(char*));
    int i = 0;
    
    char* token = strtok(line, "|");  
    while (token != NULL) {
        commands[i] = strdup(token);
        i++;
        token = strtok(NULL, "|");
    }
    commands[i] = NULL;
    
    run_multiple_commands(commands, i);
    
    for (int j = 0; j < i; j++) {
        free(commands[j]);
    }
    free(commands);
}

//helps to run multiple commands by pipe and redirections
void run_multiple_commands(char** commands, int num_commands) {
    int i;
    int input_fd = STDIN_FILENO;
    int output_fd = STDOUT_FILENO;
    int pipe_fd[2];
    pid_t pid;

    for (i = 0; i < num_commands; i++) {
        if (pipe(pipe_fd) < 0) {
            perror("pipe error!");
        }

        if ((pid = fork()) < 0) {
            perror("fork error");
        } else if (pid == 0) {
            if (i != 0) {
                if (dup2(input_fd, STDIN_FILENO) < 0) {
                    perror("dup2 error");
                }
            }
            if (i != num_commands - 1) {
                if (dup2(pipe_fd[1], STDOUT_FILENO) < 0) {
                    perror("dup2 error");
                }
            }
            close(pipe_fd[0]);
            close(pipe_fd[1]);

            char* args[64];
            int j = 0;
            char* command = commands[i];
            char* token = strtok(command, " ");

            while (token != NULL) {
                if (strcmp(token, ">") == 0) {
                    token = strtok(NULL, " ");
                    output_fd = open(token, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                    if (output_fd < 0) {
                        perror("open error");
                        exit(EXIT_FAILURE);
                    }
                } else if (strcmp(token, "<") == 0) {
                    token = strtok(NULL, " ");
                    input_fd = open(token, O_RDONLY);
                    if (input_fd < 0) {
                        perror("open error");
                        exit(EXIT_FAILURE);
                    }
                } else {
                    args[j++] = token;
                    
                }
                token = strtok(NULL, " ");
            }
            args[j] = NULL;

            if (dup2(input_fd, STDIN_FILENO) < 0) {
                perror("Dup2 Error inputfd: ");
                exit(EXIT_FAILURE);
                
            }

            if (dup2(output_fd, STDOUT_FILENO) < 0) {
                perror("Dup2 Error outputfd: ");
                exit(EXIT_FAILURE);
            }

            log_command(getpid(),commands[i]);//calls function that creates log file
            execvp(args[0], args);
            perror("Execvp Error!! Invalid command!");
            exit(EXIT_FAILURE);
        } else {
            if (i != 0) {
                close(input_fd);
            }
            if (i != num_commands - 1) {
                input_fd = pipe_fd[0];
            }
            close(pipe_fd[1]);
            
            int status;
            waitpid(pid, &status, 0);       
        }
    }
    
}
//printing terminal function
void terminal(){
    char input[300];
    signal(SIGINT,sigint_handler);//signal handlers
    signal(SIGTSTP,sigtstp_handler);
    while(1){//loop continues until user input ":q"
        printf("\n");
        printf("Zekican's Terminal$ ");
        fflush(stdout);
        // gets input
        
        fgets(input, sizeof(input), stdin);
        input[strlen(input)-1] = '\0';
        
        //if input is :q finalizes execution
        if(strcmp(input,":q")==0){
            exit(EXIT_FAILURE);
        }
            
        parse(input);    
    }
    
}

int main(){
    
    printf("=== WELCOME TO MY TERMINAL ===\n");
    
    terminal(); 
}
