#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <linux/limits.h>
#include <sys/wait.h>
#include <errno.h>

void execute_command(char** args) {
    pid_t process = fork();
    if (process == -1) {
        perror("fork failed");
        return;
    }
    if (process == 0) {
        execvp(args[0], args);
        perror("Command execution failed");
        exit(1);
    }
    int status;
    waitpid(process, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        printf("Process failed with error code %i.", WEXITSTATUS(status));
    }
}

typedef struct List {
    char** list;
    int size;
} List;

bool startsWith(const char* string, const char* prefix) {
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

List addList(const char* string, List list) {
    char** temp = realloc(list.list, (list.size+2)*sizeof(char*));
    if (temp == NULL) {
        perror("realloc failed");
        return list;
    }
    list.list = temp;
    list.list[list.size] = strdup(string);
    if (list.list[list.size] == NULL) {
        perror("strdup failed");
        return list;
    }
    list.list[list.size + 1] = NULL;
    list.size++;
    return list;
}



int main(int argc, char *argv[]){
    char currentcmd[PATH_MAX];
    char* homePath = getenv("HOME");

    char* currentDirectory = malloc(PATH_MAX);
    if (getcwd(currentDirectory, PATH_MAX) == NULL) {
        perror("Could not find current directory.");
        exit(1);
    }

    List splitCommand = {NULL, 0};

    char* username = getenv("USER");
    if (username == NULL) {
        username = "unknown";
    }

    while (true){ // keep the shell running until the exit command is entered

        if (homePath != NULL && startsWith(currentDirectory, homePath)) {
            printf("%s:~%s: ", username, currentDirectory + strlen(homePath));
        }
        else {
            printf("%s:%s: ", username, currentDirectory);
        }
        fgets(currentcmd, sizeof(currentcmd), stdin);
        currentcmd[strcspn(currentcmd, "\n")] = 0;
        char* splitCommandTemp = strtok(currentcmd, " ");
        while (splitCommandTemp != NULL) {
            splitCommand = addList(splitCommandTemp, splitCommand);
            splitCommandTemp = strtok(NULL, " ");
        }

        if (splitCommand.size == 0) continue;

        if (strcmp(splitCommand.list[0], "cd") == 0) {
            if (splitCommand.size<2) {
                if (homePath != NULL) {
                    chdir(homePath);
                }
            }
            else if (splitCommand.size>2) {
                printf("Too many arguments.\n");
            }
            else {
                errno = 0;
                if (chdir(splitCommand.list[1]) == -1) {
                    if (ENOENT == errno) {
                        printf("Directory does not exist.\n");
                    }
                    else {
                        printf("cd failed for unknown reason.\n");
                    }
                    if (splitCommand.list != NULL) {
                        for (int i = 0; i < splitCommand.size; i++) {
                            free(splitCommand.list[i]);
                        }
                        free(splitCommand.list);
                        splitCommand.list = NULL;
                        splitCommand.size = 0;
                    }
                    continue;
                }
            }
            if (getcwd(currentDirectory, PATH_MAX) == NULL) {
                perror("Could not find current directory.");
                exit(1);
            }
        }
        else if (strcmp(splitCommand.list[0], "exit") == 0) {
            free(currentDirectory);
            exit(EXIT_SUCCESS);
        }
        else {
            execute_command(splitCommand.list);
        }

        if (splitCommand.list != NULL) {
            for (int i = 0; i < splitCommand.size; i++) {
                free(splitCommand.list[i]);
            }
            free(splitCommand.list);
            splitCommand.list = NULL;
            splitCommand.size = 0;
        }
    }
}