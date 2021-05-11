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
static int containerId = 0;

int checkpoint[2];

typedef struct args_t_ {
    char* toRun;
    int id;
} args_t;

void handleSigint(int sig) {
    for (size_t i = 0;i < vector_size(containers);++i) {

        pid_t proc = *((int*)vector_get(containers, i));
        printf("Waiting on process %d\n", proc);
        int stat;
        waitpid(proc, &stat, 0);
        if (WIFEXITED(stat)) {
            printf("process %d exited\n", proc);
        }
        char tmp[126];
        sprintf(tmp, "rm -r busybox%lu", i);
        system(tmp);
    }
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
    sleep(1);
    char busyNew[128];
    char busyOld[128];
    args_t* argStruct = (args_t*)args;
    sprintf(busyNew, "./busybox%d", argStruct->id);
    sprintf(busyOld, "./busybox%d/.old", argStruct->id);
    pivot_root(busyNew, busyOld);
    printf("New child pid: %d\n",getpid());
    mount("tmpfs", "/dev", "tmpfs", MS_NOSUID | MS_STRICTATIME, NULL);
    mount("proc", "/proc", "proc", 0, NULL);
    umount2("/.old", MNT_DETACH);

    close(checkpoint[1]);
    char c;
    read(checkpoint[0], &c, 1);
    system("ip link set veth1 up");

    system("ip link set veth1 up");
    system("ip addr add 169.254.1.2/30 dev veth1");

    chdir("/");
    system("adduser -D newuser");
    system("adduser -D sudouser");
    system("echo ' sudouser ALL=(ALL)   ALL' >> /etc/sudoers");
    puts("List of users: ");
    system("cut -d: -f1 /etc/passwd");
    // printf("PID: %ld\n", (long)getpid());
    // system("ifconfig");
    system("echo \"nameserver 8.8.8.8\" >> /etc/resolv.conf");
    system(argStruct->toRun);

    system("deluser newuser");
    system("deluser sudouser");
    return -1;
}
#define MAX_PROCESS 10

int main() {
    pipe(checkpoint);
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
        args_t toPass;
        toPass.id = containerId;
        toPass.toRun = cmd;
        char tmp[126];
        sprintf(tmp, "cp -R busybox busybox%d", containerId++);
        system(tmp);
        pid_t child_pid = clone(startContainer, child_stack + 1048576, CLONE_NEWPID | SIGCHLD | CLONE_NEWNS | CLONE_NEWNET, &toPass);


        char* cmd;
        asprintf(&cmd, "ip link set veth1 netns %d", child_pid);
        system("ip link add veth0 type veth peer name veth1");
        system(cmd);
        system("ip link set veth0 up");
        system("ip addr add 169.254.1.1/30 dev veth0");
        free(cmd);
        close(checkpoint[1]);
        // char buf[1024];
        // sprintf(buf, "./net.sh eth0 ");
        // char pidStr[64];
        // sprintf(pidStr, "%d", child_pid);
        // strcat(buf, pidStr);
        // puts(buf);
        // system(buf);
        printf("clone() = %ld\n", (long)child_pid);
        vector_push_back(containers, &child_pid);
    }

    return 0;
}