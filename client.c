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
    if (connect(sock_fd, result->ai_addr, result->ai_addrlen) != 0)
    {
        puts("failed");
        exit(1);
    }
    puts("Connected to server");
    char *res = argv[1];
    puts(res);
    write(sock_fd, res, strlen(res));
    char buffer[1024*1024];
    read(sock_fd,buffer,1024*1024-1);
    puts(buffer);
    close(sock_fd);
}