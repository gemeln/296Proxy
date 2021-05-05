#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include "includes/vector.h"
#include <sys/syscall.h>

static char child_stack[1048576];

static vector* containers;

int startContainer(void* args) {
    long res = syscall(SYS_pivot_root, "./tmp1/", "./tmp2/");
    if (res == -1)
        perror("pivot");
    char* command = args;
    free(command);
    printf("PID: %ld\n", (long)getpid());
    chdir("/");
    system("echo hi > hi.txt");
    return -1;
}

int main() {
    puts("HI");
    char* cmd = NULL;
    size_t cmdSize = 0;
    containers = int_vector_create();
    while (1) {
        printf("New container: ");
        size_t linesize = getline(&cmd, &cmdSize, stdin);
        cmd[linesize - 1] = 0;
        pid_t child_pid = clone(startContainer, child_stack + 1048576, CLONE_NEWPID | SIGCHLD | CLONE_NEWNS, strdup(cmd));
        printf("clone() = %ld\n", (long)child_pid);
    }
    return 0;
}