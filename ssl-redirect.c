#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
int main(int argc, int **argv)
{
    // sendReq(NULL);
    // return 0;
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
    char buffer[1024 * 1024];
    // READING
    int len = read(client_fd, buffer, sizeof(buffer) - 1);
    buffer[len] = '\0';

    printf("Read %d chars\n", len);
    printf("===Received request===\n");
    printf("%s\n", buffer);
    // Get hostname
    char hostName[512];
    char *hostBegin = strstr(buffer, "Host: ") + strlen("Host: ");
    int hostLen = strstr(hostBegin, "\r\n") - hostBegin;
    hostName[hostLen] = '\0';
    strncpy(hostName, hostBegin, hostLen);
    printf("%s|\n", hostName);
    // Get openssl command
    int toSSL[2];
    pipe(toSSL);
    int fromSSL[2];
    pipe(fromSSL);
    fflush(stdout);
    if (fork() == 0)
    {
        close(client_fd);
        close(sock_fd);
        dup2(fromSSL[1], fileno(stdout));
        dup2(toSSL[0], fileno(stdin));
        close(fromSSL[1]);
        close(fromSSL[0]);
        close(toSSL[0]);
        close(toSSL[1]);
        char openSSL[1024];
        sprintf(openSSL, "openssl s_client -connect %s\n", hostName);
        puts(openSSL);
        system("openssl s_client -connect www.google.com:443");
        exit(1);
    }
    int total = 1024 * 1024;
    while (1)
    {
        int bytes = read(fromSSL[0], buffer, total);
        if (strstr(buffer, "Verify return code: 0 (ok)"))
        {
            break;
        }
    }
    char *success = "HTTP/1.0 200 Connection Established\r\n\r\n";
    write(client_fd, success, strlen(success));
    while (1)
    {
        int bytes = read(client_fd, buffer, 1024 * 1023);
        fprintf(stderr, "read %d bytes from firefox\n", bytes);
        strcpy(buffer, "GET /\r\n\r\n");
        write(toSSL[1], buffer, bytes);
        while (1)
        {
            int bytes = read(fromSSL[0], buffer, 1024 * 1023);
            fprintf(stderr, "read %d bytes from pipe\n", bytes);
            write(client_fd, buffer, bytes);
            puts(buffer);
        }
    }
    puts("DONE");
    close(client_fd);
    close(sock_fd);
    return 0;
}