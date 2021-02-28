#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
int main(int argc, int **argv)
{
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int s = getaddrinfo(NULL, "1357", &hints, &result);
    assert(s == 0);
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    // Bind and listen
    if (bind(sock_fd, result->ai_addr, result->ai_addrlen) != 0)
    {
        perror("bind()");
        exit(1);
    }

    if (listen(sock_fd, 10) != 0)
    {
        perror("listen()");
        exit(1);
    }
    puts("Waiting to accept");
    int client_fd = accept(sock_fd, NULL, NULL);
    puts("Accepted");
    char buffer[1000];
    // READING
    int len = read(client_fd, buffer, sizeof(buffer) - 1);
    buffer[len] = '\0';

    printf("Read %d chars\n", len);
    printf("===\n");
    printf("%s\n", buffer);
    return 0;
}