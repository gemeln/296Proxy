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
char *makeReq(int client_fd, int sock_fd)
{
    char buffer[1024*1024];
    int toSSL[2];
    pipe(toSSL);
    int fromSSL[2];
    pipe(fromSSL);
    fflush(stdout);
    pid_t child = fork();
    if (child == 0)
    {
        close(client_fd);
        close(sock_fd);
        dup2(fromSSL[1], fileno(stdout));
        dup2(toSSL[0], fileno(stdin));
        close(fromSSL[1]);
        close(fromSSL[0]);
        close(toSSL[0]);
        close(toSSL[1]);
        system("openssl s_client -connect www.google.com:443");
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

    numRead = 0;
    char *req = "GET /\r\n\r\n";
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
    puts("\n\n\n\nDONE\n");
    *res_end = 0;

    puts(res_end - 30);
    waitpid(child, NULL, 0);
    puts("ENDED");
    close(fromSSL[1]);
    close(fromSSL[0]);
    close(toSSL[0]);
    close(toSSL[1]);
    return NULL;
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

    printf("Read %d chars\n", len);
    printf("===Received request===\n");
    printf("%s\n", buffer);
    puts(buffer);
    // Get openssl command
    makeReq(client_fd,sock_fd);
    return 0;
}