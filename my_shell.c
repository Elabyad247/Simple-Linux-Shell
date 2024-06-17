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
int in_parallel_mode = 0;
int interrupted = 0;
pid_t parallel_pids[MAX_NUM_TOKENS];
int parallel_count = 0;

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

void terminate_parallel_processes() {
    for (int i = 0; i < parallel_count; i++) {
        kill(parallel_pids[i], SIGKILL);
    }
    parallel_count = 0;
}

void free_tokens(char **tokens) {
    for (int i = 0; tokens[i] != NULL; i++) {
        free(tokens[i]);
    }
    free(tokens);
}

void sigint_handler(int signum) {
    interrupted = 1;
    if (current_pid != -1) {
        kill(-current_pid, SIGKILL);
        current_pid = -1;
    }
    if (in_parallel_mode) {
        terminate_parallel_processes();
    }
}

void execute_command(char **tokens, int background) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Shell: Fork failed");
    } else if (pid == 0) {
        setpgid(0, 0); // Set the process group ID to the PID of the child process
        if (execvp(tokens[0], tokens) == -1) {
            perror("Shell: Command not found");
            exit(EXIT_FAILURE);
        }
    } else {
        if (!background) {
            current_pid = pid;
            setpgid(pid, pid); // Set the process group ID to the PID of the child process
            waitpid(pid, NULL, 0); // Wait for the foreground process to finish
            current_pid = -1;
        } else {
            background_pids[background_count++] = pid;
        }
    }
}

char ***parse_commands(char *line, const char *delimiter) {
    char ***commands = (char ***) malloc(MAX_NUM_TOKENS * sizeof(char **));
    char **tokens = tokenize(line); // Tokenize the input line
    int commandIndex = 0;
    int tokenIndex = 0;
    commands[commandIndex] = (char **) malloc(MAX_NUM_TOKENS * sizeof(char *)); // Allocate memory for the command
    for (int i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], delimiter) == 0) { // Check if the token is the delimiter
            commands[commandIndex][tokenIndex] = NULL; // Terminate the command with NULL
            commands[++commandIndex] = (char **) malloc(MAX_NUM_TOKENS * sizeof(char *));
            tokenIndex = 0;
        } else {
            commands[commandIndex][tokenIndex++] = tokens[i]; // Add the token to the current command
        }
    }
    commands[commandIndex][tokenIndex] = NULL;
    commands[commandIndex + 1] = NULL;
    free(tokens);
    return commands;
}

void execute_serial_commands(char ***commands) {
    for (int i = 0; commands[i] != NULL; i++) {
        if (interrupted) {
            break;
        }
        execute_command(commands[i], 0);
    }
}

void execute_parallel_commands(char ***commands) {
    in_parallel_mode = 1;
    parallel_count = 0; // Reset parallel count
    for (int i = 0; commands[i] != NULL; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("Shell: Fork failed");
        } else if (pid == 0) {
            setpgid(0, 0); // Put the child in a new process group
            if (execvp(commands[i][0], commands[i]) == -1) { // Execute the command
                perror("Shell: Command not found");
                exit(EXIT_FAILURE);
            }
        } else {
            parallel_pids[parallel_count++] = pid;
        }
    }
    for (int i = 0; i < parallel_count; i++) {
        waitpid(parallel_pids[i], NULL, 0); // Wait for each parallel process to finish
    }
    parallel_count = 0;
    in_parallel_mode = 0;
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
        interrupted = 0;
        if (background_count > 0) { // Check if there are any background processes
            reap_background_processes();
        }
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

        if (strstr(line, "&&&") != NULL) {
            char ***commands = parse_commands(line, "&&&");
            execute_parallel_commands(commands);
            free(commands);
        } else if (strstr(line, "&&") != NULL) {
            char ***commands = parse_commands(line, "&&");
            execute_serial_commands(commands);
            free(commands);
        } else {
            if (strcmp(tokens[0], "cd") == 0) {
                if (tokens[1] != NULL) { // Check if the directory is provided
                    if (chdir(tokens[1]) != 0) {
                        perror("Shell: Directory not found");
                    }
                } else {
                    perror("Shell: Incorrect command");
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
                execute_command(tokens, background);
            }
        }

        // Freeing the allocated memory
        free_tokens(tokens);
    }
    return 0;
}
