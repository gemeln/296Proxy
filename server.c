#define _GNU_SOURCE
#include <assert.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.c"
#define BUFSIZE 1024 * 1024

char* block_arr[100];
int block_list_size = 0;

int checkBlocklist(char* hostname) {
    int is_blocked = 0;
    for (int i = 0; i < block_list_size; i++) {
        if (strncmp(block_arr[i], hostname, strlen(block_arr[i])) == 0) {
            // website is blocked
            return 1;
        }
    }

    return 0;
}
void* HostToCli(void* args) {
    int* fds = (int*)args;
    int client_fd = fds[0];
    int dest_fd = fds[1];
    ssize_t amountRead, amountWrote;
    char buffer[BUFSIZE];
    while (true) {
        amountRead = read(dest_fd, buffer, BUFSIZE);
        printf("Read from HOST %ld\n", amountRead);
        if (amountRead == 0) break;
        if (amountRead < 0) {
            if (errno == EINTR) continue;
            break;
        }
        amountWrote = write(client_fd, buffer, amountRead);
        printf("Wrote to FF %ld\n\n", amountWrote);
    }
    return NULL;
}

void* HandleConnection(void* args) {
    int client_fd = *((int*)args);
    // Get request from client, and parse host from CONNECT request
    ssize_t amountRead, amountWrote;
    char buffer[BUFSIZE + 1];
    amountRead = read(client_fd, buffer, BUFSIZE);
    if (amountRead <= 0) {
        char* mesg = "403 Forbidden\r\n\r\n";
        write_all_to_socket(client_fd, mesg, strlen(mesg));
        puts("Sent 403 Forbidden to client");
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
        return 0;
    }

    buffer[amountRead] = 0;
    hostinfo inf;
    parseHost(buffer, &inf);
    printf("%s %s\n", inf.hostname, inf.port);

    int is_blocked = checkBlocklist(inf.hostname);
    if (is_blocked) {
        char* mesg = "403 Forbidden\r\n\r\n";
        write_all_to_socket(client_fd, mesg, strlen(mesg));
        puts("Sent 403 Forbidden to client");
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
        return 0;
    }

    int dest_fd = connect_to_server(inf.hostname, inf.port);
    assert(dest_fd >= 0);
    // Success, return 200
    char* msg = "200 OK\r\n\r\n";
    write_all_to_socket(client_fd, msg, strlen(msg));
    puts("Sent 200 OK to client");

    pthread_t newThread;
    int fds[] = {client_fd, dest_fd};
    pthread_create(&newThread, NULL, &HostToCli, fds);
    pthread_detach(newThread);
    while (true) {
        amountRead = read(client_fd, buffer, BUFSIZE);
        printf("Read from FF %ld\n", amountRead);
        if (amountRead == 0) break;
        if (amountRead < 0) {
            if (errno == EINTR) continue;
            break;
        }
        amountWrote = write(dest_fd, buffer, amountRead);
        printf("Wrote to HOSt %ld\n\n", amountWrote);
    }
    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
    shutdown(dest_fd, SHUT_RDWR);
    close(dest_fd);
    return NULL;
}

int main(int argc, char** argv) {
    // Creating blocklist

    FILE* block_file = fopen("blocked_list.txt", "r");
    assert(block_file);
    char* block_line = NULL;
    size_t link_len = 0;
    ssize_t bytes_read;
    if (block_file != NULL) {
        while ((bytes_read = getline(&block_line, &link_len, block_file)) !=
               -1) {
            block_line[strlen(block_line) - 1] = '\0';
            char* arr_str = malloc(strlen(block_line) + 1);
            strcpy(arr_str, block_line);
            block_arr[block_list_size] = arr_str;
            block_list_size++;
        }
    }

    fclose(block_file);

    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int s = getaddrinfo(NULL, "1358", &hints, &result);
    assert(s == 0);
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    int enable = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    // Bind and listen
    if (bind(sock_fd, result->ai_addr, result->ai_addrlen) != 0) {
        perror("bind()");
        exit(1);
    }

    if (listen(sock_fd, 10) != 0) {
        perror("listen()");
        exit(1);
    }
    while (true) {
        puts("Waiting to accept");
        int client_fd = accept(sock_fd, NULL, NULL);
        assert(client_fd > 0);
        puts("Accepted");
        pthread_t connect_thread;
        pthread_create(&connect_thread, NULL, HandleConnection, &client_fd);
        pthread_detach(connect_thread);
    }
    return 0;
}
