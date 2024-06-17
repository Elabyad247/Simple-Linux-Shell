#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

// Splits the string by space and returns the array of tokens
char **tokenize(char *line) {
    char **tokens = (char **) malloc(MAX_NUM_TOKENS * sizeof(char *));
    char *token = (char *) malloc(MAX_TOKEN_SIZE * sizeof(char));
    int i, tokenIndex = 0, tokenNo = 0;

    for (i = 0; i < strlen(line); i++) {
        char readChar = line[i];
        if (readChar == ' ' || readChar == '\n' || readChar == '\t') {
            token[tokenIndex] = '\0';
            if (tokenIndex != 0) {
                tokens[tokenNo] = (char *) malloc(MAX_TOKEN_SIZE * sizeof(char));
                strcpy(tokens[tokenNo++], token);
                tokenIndex = 0;
            }
        } else {
            token[tokenIndex++] = readChar;
        }
    }

    free(token);
    tokens[tokenNo] = NULL;
    return tokens;
}

pid_t background_pids[MAX_NUM_TOKENS];
int background_count = 0;
pid_t current_pid = -1;

void reap_background_processes() {
    pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        printf("Shell: Background process finished\n");
        // Remove the finished PID from the list
        for (int i = 0; i < background_count; i++) {
            if (background_pids[i] == pid) {
                for (int j = i; j < background_count - 1; j++) {
                    background_pids[j] = background_pids[j + 1];
                }
                background_count--;
                break;
            }
        }
    }
}

void terminate_background_processes() {
    for (int i = 0; i < background_count; i++) {
        kill(background_pids[i], SIGKILL);
    }
    background_count = 0;
}

void free_tokens(char **tokens) {
    for (int i = 0; tokens[i] != NULL; i++) {
        free(tokens[i]);
    }
    free(tokens);
}

void sigint_handler(int signum) {
    if (current_pid != -1) {
        kill(-current_pid, SIGKILL);
    }
}

int main(int argc, char *argv[]) {
    char line[MAX_INPUT_SIZE];
    char **tokens;
    int i;

    struct sigaction sa;
    sa.sa_handler = sigint_handler; // Set the signal handler
    sigemptyset(&sa.sa_mask); // Clear the signal mask
    sa.sa_flags = SA_RESTART; // Restart the system call if possible
    sigaction(SIGINT, &sa, NULL); // Register the signal handler

    while (1) {
        reap_background_processes(); // Check and reap background processes
        /* BEGIN: TAKING INPUT */
        bzero(line, sizeof(line));
        printf("$ ");
        scanf("%[^\n]", line);
        getchar();
        /* END: TAKING INPUT */

        // Handle empty input
        if (strcmp(line, "") == 0) {
            continue;
        }
        line[strlen(line)] = '\n'; //terminate with new line
        tokens = tokenize(line);
        // Handle empty input after tokenization
        if (tokens[0] == NULL) {
            free_tokens(tokens);
            continue;
        }

        // Handle commands
        if (strcmp(tokens[0], "cd") == 0) {
            if (tokens[1] != NULL) { // Check if the directory is provided
                if (chdir(tokens[1]) != 0) {
                    printf("Error: Directory not found\n");
                }
            } else {
                printf("Shell: Incorrect command\n");
            }
        } else if (strcmp(tokens[0], "exit") == 0) {
            terminate_background_processes(); // Terminate all background processes
            free_tokens(tokens);
            break;
        } else {
            int background = 0;
            int tokenCount = 0;
            while (tokens[tokenCount] != NULL) {
                tokenCount++;
            }
            if (strcmp(tokens[tokenCount - 1], "&") == 0) {
                background = 1;
                tokens[tokenCount - 1] = NULL; // Remove the '&' token
            }

            pid_t pid = fork();
            if (pid < 0) {
                printf("Shell: Fork failed\n");
            } else if (pid == 0) {
                setpgid(0, 0); // Set the process group ID to the PID of the child process
                if (strcmp(tokens[0], "sleep") == 0) { // Handle sleep command separately for testing
                    if (tokens[1] != NULL) {
                        sleep(atoi(tokens[1]));
                    } else {
                        printf("Shell: Incorrect command\n");
                    }
                    exit(0);
                } else {
                    if (execvp(tokens[0], tokens) == -1) { // Execute the command
                        printf("Error: Command not found\n");
                        exit(EXIT_FAILURE);
                    }
                }
            } else {
                if (!background) {
                    current_pid = pid;
                    setpgid(pid, pid); // Set the process group ID to the PID of the child process
                    waitpid(pid, NULL, 0); // Wait for the foreground process to finish
                    current_pid = -1;
                } else {
                    //Sleep for 0.25 second to allow the background process to start
                    usleep(250000);
                    background_pids[background_count++] = pid;
                }
            }
        }

        // Freeing the allocated memory
        free_tokens(tokens);
    }
    return 0;
}
