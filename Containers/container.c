#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "includes/vector.h"
#include <sys/syscall.h>
#include <errno.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <assert.h>
static char child_stack[1048576];
static vector* containers;

void handleSigint(int sig) {
    for (size_t i = 0;i < vector_size(containers);++i) {
        pid_t proc = *((int*)vector_get(containers, i));
        printf("Waiting on process %d\n", proc);
        int stat;
        waitpid(proc, &stat, 0);
        if (WIFEXITED(stat)) {
            printf("process %d exited\n", proc);
        }
    }
    rmdir("./busybox/.old");
    vector_destroy(containers);
    exit(0);
}
int pivot_root(char* a, char* b) {
    if (mount(a, a, "bind", MS_BIND | MS_REC, "") < 0) {
        printf("error mount: %s\n", strerror(errno));
    }
    if (mkdir(b, 0755) < 0) {
        printf("error mkdir %s\n", strerror(errno));
    }
    printf("pivot setup ok\n");
    return syscall(SYS_pivot_root, a, b);
}
int startContainer(void* args) {
    pivot_root("./busybox", "./busybox/.old");
    mount("tmpfs", "/dev", "tmpfs", MS_NOSUID | MS_STRICTATIME, NULL);
    mount("proc", "/proc", "proc", 0, NULL);
    umount2("/.old", MNT_DETACH);

    chdir("/");
    // system("ifconfig");
    // printf("PID: %ld\n", (long)getpid());
    // system("./net.sh eth0");
    // system("echo \"nameserver 8.8.8.8\" >> /etc/resolv.conf");
    system(args);
    return -1;
}
#define MAX_PROCESS 10


int main() {
    system("mount --make-rprivate  /");
    signal(SIGINT, handleSigint);
    containers = int_vector_create();
    char* cmd = NULL;
    size_t cmdSize = 0;
    containers = int_vector_create();
    while (1) {
        printf("New container: ");
        size_t linesize = getline(&cmd, &cmdSize, stdin);
        cmd[linesize - 1] = 0;
        if (vector_size(containers) == MAX_PROCESS) {
            fprintf(stderr, "Exceeded max processes: %d\n", MAX_PROCESS);
            continue;
        }
        pid_t child_pid = clone(startContainer, child_stack + 1048576, CLONE_NEWPID | SIGCHLD | CLONE_NEWNS | CLONE_NEWNET, cmd);
        printf("clone() = %ld\n", (long)child_pid);
        vector_push_back(containers, &child_pid);
    }

    return 0;
}
