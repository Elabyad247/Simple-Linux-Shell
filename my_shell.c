#include  <stdio.h>
#include  <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

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


int main(int argc, char *argv[]) {
    char line[MAX_INPUT_SIZE];
    char **tokens;
    int i;

    while (1) {
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
            continue;
        }

        // Handle  commands
        if (strcmp(tokens[0], "cd") == 0) {
            if (tokens[1] != NULL) { // Check if the directory is provided
                if (chdir(tokens[1]) != 0) {
                    printf("Error: Directory not found\n");
                }
            } else {
                printf("Shell: Incorrect command\n");
            }
        } else if (strcmp(tokens[0], "exit") == 0) {
            exit(0);
        } else {
            pid_t pid = fork();
            if (pid < 0) {
                printf("Shell: Fork failed");
            } else if (pid == 0) {
                if (execvp(tokens[0], tokens) == -1) { // Execute the command
                    printf("Error: Command not found\n");
                    exit(EXIT_FAILURE);
                }
            } else {
                wait(NULL);
            }
        }
        // Freeing the allocated memory
        for (i = 0; tokens[i] != NULL; i++) {
            free(tokens[i]);
        }
        free(tokens);
    }
    return 0;
}
