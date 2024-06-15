//
// Created by elabyad on 6/13/24.
//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

void handler(int signum) {
    printf("I will run forever\n");
}

int main(int argc, char *argv[]) {
    printf("I am: %d\n", (int) getpid());
    signal(SIGINT, handler);
    while (1) {
        sleep(1);
    }
    return 0;
}