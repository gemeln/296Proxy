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
int parseReq(char *req)
{
    char *hostBegin = strstr(req, "Host: ") + strlen("Host: ");
    size_t hostlen = 0;
    while (1)
    {
        if (hostBegin[hostlen] == ':')
            break;
        if (isspace(hostBegin[hostlen]))
            break;
        hostlen++;
    }
    char hostname[1024];
    hostname[hostlen] = 0;
    strncpy(hostname, hostBegin, hostlen);
    hostname[hostlen] = '\0';
    printf("Parsed hostname: %s\\\n", hostname);

    struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        perror("ERROR opening socket");

    server = gethostbyname(hostname);
    if (server == NULL)
        perror("ERROR, no such host");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(80);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        perror("ERROR connecting");

    return sockfd;
}
typedef struct response_t_
{
    int datasize;
    void *data;
} response_t;

response_t *sendReq(int sockfd, char *req, size_t reqlen)
{
    int bytes, sent, received, total;
    total = reqlen;
    sent = 0;
    do
    {
        bytes = write(sockfd, req + sent, total - sent);
        printf("WROTE %d\n", bytes);
        if (bytes < 0)
            perror("ERROR writing message to socket");
        if (bytes == 0)
            break;
        sent += bytes;
    } while (sent < total);
    char response[1024 * 1024];
    memset(response, 0, sizeof(response));
    total = sizeof(response) - 1;
    received = 0;
    do
    {
        bytes = read(sockfd, response + received, total - received);
        if (bytes < 0)
            perror("ERROR reading response from socket");
        if (bytes == 0)
            break;
        received += bytes;
    } while (received < total);

    if (received == total)
        perror("ERROR storing complete response from socket");
    printf("read %d form server\n", received);
    response_t *ret = malloc(sizeof(response_t));
    ret->data = malloc(received);
    ret->datasize = received;
    memcpy(ret->data, response, received);
    return ret;
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
    while (1)
    {
        puts("Waiting to accept");
        int client_fd = accept(sock_fd, NULL, NULL);
        puts("Accepted");
        char buffer[2048];
        // READING
        int len = read(client_fd, buffer, sizeof(buffer) - 1);
        buffer[len] = '\0';

        printf("Read %d chars\n", len);
        printf("===Received request===\n");
        printf("%s\n", buffer);
        int target_fd = parseReq(buffer);
        strcpy(buffer,"GET google.com \r\nHost: google.com\r\n\r\n");
        response_t *res = sendReq(target_fd, buffer, strlen(buffer));
        puts(res->data);
        dprintf(client_fd, "HTTP/1.0 200 Connection established");
        return 0;
        do
        {
            len = read(client_fd, buffer, sizeof(buffer) - 1);
            buffer[len] = '\0';
            printf("===Received request 2==%d chars\n", len);
            response_t *res = sendReq(target_fd, buffer, len);
            write(client_fd, res->data, res->datasize);
        } while (len > 0);
        close(client_fd);
        free(res);
    }
    close(sock_fd);
    return 0;
}