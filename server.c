#define _GNU_SOURCE
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
#include <sys/types.h>
#include <sys/wait.h>
int makeReq(char *writeback, char *clientHeader, int client_fd, int sock_fd)
{
    char buffer[1024 * 1024];
    int toSSL[2];
    pipe(toSSL);
    int fromSSL[2];
    pipe(fromSSL);
    fflush(stdout);
    pid_t child = fork();
    if (child == 0)
    {
        // close(client_fd);
        // close(sock_fd);
        dup2(fromSSL[1], fileno(stdout));
        dup2(toSSL[0], fileno(stdin));
        close(fromSSL[1]);
        close(fromSSL[0]);
        close(toSSL[0]);
        close(toSSL[1]);
        // Search for host name
        char *host = strstr(clientHeader, "Host: ") + strlen("Host: ");
        char *hostEnd = strstr(host, "\r\n");
        int length = hostEnd - host;
        memcpy(buffer, host, length);
        buffer[length] = 0;
        char command[1024];
        sprintf(command, "openssl s_client -connect %s", buffer);
        system(command);
        exit(1);
    }
    int total = 1024 * 1024;
    size_t numRead = 0;
    while (1)
    {
        numRead += read(fromSSL[0], buffer + numRead, total);
        if (strstr(buffer, "Verify return code: 0 (ok)"))
        {
            break;
        }
    }
    // SSL OK
    // Get Request from http rheader
    char req[1024];
    char *reqEnd = strstr(clientHeader, "\r\n");
    int length = reqEnd - clientHeader+2;
    memcpy(req, clientHeader, length);
    req[length] = 0;
    puts(req);
    numRead = 0;
    write(toSSL[1], req, strlen(req));
    char *res_start;
    char *res_end;
    while (1)
    {
        numRead += read(fromSSL[0], buffer + numRead, total);
        buffer[numRead] = 0;
        char *OK = strstr(buffer, "200 OK\r\n");
        if (OK)
        {
            res_start = OK;
            char *END = strstr(OK, "---\nPost-Handshake New Session");
            if (END)
            {
                res_end = END;
                break;
            }
        }
        // puts(buffer);
    }
    res_start = strstr(res_start, "\r\n\r\n") + 4;
    assert(res_start);
    // puts("\n\n\n\nDONE\n");
    *res_end = 0;
    length = res_end - res_start;
    memcpy(writeback, res_start, length);
    // puts(res_end - 30);
    waitpid(child, NULL, 0);
    // puts("ENDED");
    close(fromSSL[1]);
    close(fromSSL[0]);
    close(toSSL[0]);
    close(toSSL[1]);
    return length;
}
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
    // Get openssl command
    char writeback[1024 * 1024];
    int size = makeReq(writeback, buffer, client_fd, sock_fd);
    write(client_fd, writeback, size);
    shutdown(sock_fd, SHUT_RDWR);
    close(sock_fd);
    return 0;
}