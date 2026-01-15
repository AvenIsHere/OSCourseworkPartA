#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <linux/limits.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <asm-generic/errno-base.h>
#include <sys/stat.h>

#define FORK(pcode)\
    pid_t process = fork();\
    if (process == 0) {\
        pcode\
        exit(0);\
    }\
    int status;\
    wait(&status);\
    if (status == 1) {\
        printf("Process failed.");\
    }

typedef struct List {
    char** list;
    int size;
} List;

bool startsWith(char* string, char* prefix) {
    size_t slen = strlen(string);
    size_t plen = strlen(prefix);
    if (plen == 0) {
        return true;
    }
    if (plen > slen) {
        return false;
    }
    return strncmp(string, prefix, plen) == 0;
}

List addList(char* string, List list) {
    list.list = realloc(list.list, (list.size+1)*sizeof(char*) + 1);
    list.list[list.size] = string;
    list.list[list.size + 1] = NULL;
    list.size++;
    return list;
}

int main(int argc, char *argv[]){
    char currentcmd[256];

    char* currentDirectory = malloc(PATH_MAX);
    getcwd(currentDirectory, PATH_MAX);

    List splitCommand = {NULL, 0};

    char username[256];
    getlogin_r(username, sizeof(username));

    while (true){ // keep the shell running until the exit command is entered

        if (startsWith(currentDirectory, getenv("HOME"))) {
            char* temp = malloc(PATH_MAX);
            char* temp2 = malloc(PATH_MAX);
            temp[0] = '\0';
            temp2[0] = '~';
            temp2[1] = '\0';
            size_t path_length = strlen(currentDirectory);
            size_t home_length = strlen(getenv("HOME"));
            for (int i = home_length; i<path_length; i++) {
                temp[i-home_length] = currentDirectory[i];
            }
            temp[path_length-home_length] = '\0';
            strcat(temp2, temp);
            printf("%s: ", temp2);
        }
        else {
            printf("%s: ", currentDirectory);
        }
        scanf(" %[^\n]", currentcmd);
        char* splitCommandTemp = strtok(currentcmd, " ");
        while (splitCommandTemp != NULL) {
            splitCommand = addList(splitCommandTemp, splitCommand);
            splitCommandTemp = strtok(NULL, " ");
        }

        if(strcmp(splitCommand.list[0],"false")==0){
            FORK(
                printf("Task failed successfully.\n");
            )
        }

        else if(strcmp(splitCommand.list[0],"true")==0){
            FORK(
                printf("Task succeeded.\n");
            )
        }

        else if(strcmp(splitCommand.list[0], "uname")==0){
            FORK(
                printf("Linux");
            )
        }

        else if(strcmp(splitCommand.list[0], "date")==0){
            FORK(
                time_t timenow = time(NULL);
                printf("%s",asctime(localtime(&timenow)));
            )

        }

        else if (strcmp(splitCommand.list[0], "ls")==0) {
            FORK(
                struct dirent *de;
                DIR* dir = opendir(currentDirectory);
                if (dir == NULL) {
                    printf("Could not open current directory.");
                    exit(1);
                }
                while ((de = readdir(dir)) != NULL) {
                    printf("%s\n", de->d_name);
                }
                closedir(dir);
            )
        }

        // mkdir - make direct
        else if(strcmp(splitCommand.list[0],"mkdir")==0){
            FORK(
                if (splitCommand.size<2) {
                    printf("Directory name required.");
                }
                else if (splitCommand.size>2) {
                    printf("Too many arguments.");
                }
                else {
                    int status = mkdir(splitCommand.list[1], 0755);
                }
            )
        }

        else if(strcmp(splitCommand.list[0], "echo") == 0){
            FORK(
                for (int i = 1; i<splitCommand.size; i++) {
                    printf("%s ", splitCommand.list[i]);
                }
                printf("\n");
            )
        }

        else if (strcmp(splitCommand.list[0], "cd") == 0) {
            if (splitCommand.size<2) {
                chdir(getenv("HOME"));
                getcwd(currentDirectory, PATH_MAX);
            }
            else if (splitCommand.size>2) {
                printf("Too many arguments.\n");
            }
            else {
                DIR* newDIR = NULL;
                errno = 0;
                if (startsWith(splitCommand.list[1], "/")) {
                    newDIR = opendir(splitCommand.list[1]);
                }
                else {
                    char* newPath = malloc(PATH_MAX);
                    strcpy(newPath, currentDirectory);
                    strcat(newPath, "/");
                    strcat(newPath, splitCommand.list[1]);
                    newDIR = opendir(newPath);
                }
                if (newDIR) {
                    chdir(splitCommand.list[1]);
                    getcwd(currentDirectory, PATH_MAX);
                    closedir(newDIR);
                }
                else if (ENOENT == errno) {
                    printf("Directory does not exist.\n");
                }
                else {
                    printf("cd failed for unknown reason.\n");
                }
            }
        }

        else if (strcmp(splitCommand.list[0], "exit") == 0) {
            exit(EXIT_SUCCESS);
        }
        else {
            FORK(
                execvp(splitCommand.list[0], splitCommand.list);
                perror("Command execution failed");
            )
        }
        splitCommand.list = NULL;
        splitCommand.size = 0;

    }
}