#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
int main(int arc, char **argv)
{

    struct addrinfo current, *result;
    memset(&current, 0, sizeof(struct addrinfo));
    current.ai_family = AF_INET;
    current.ai_socktype = SOCK_STREAM;

    getaddrinfo("127.0.0.1", "1358", &current, &result);
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    connect(sock_fd, result->ai_addr, result->ai_addrlen);

    write(sock_fd, "BARBAR", 6);
    char buffer[1000];
    int len = read(sock_fd, buffer, sizeof(buffer) - 1);
    buffer[len] = '\0';

    printf("Read %d chars\n", len);
    printf("===\n");
    printf("%s\n", buffer);
    close(sock_fd);
}