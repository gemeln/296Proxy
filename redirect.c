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


char* block_arr[100];
int block_list_size = 0;
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

    int is_blocked = 0;
    for (int i = 0; i < block_list_size; i++) {
        if (strncmp(block_arr[i], hostname, strlen(block_arr[i])) == 0) {
            // website is blocked
            is_blocked = 1;
            break;
        }
    }

    if (is_blocked) {
        printf("===WEBSITE BLOCKED===\n");
        return -1;

    }
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
    printf("read %d from server\n", received);
    response_t *ret = malloc(sizeof(response_t));
    ret->data = malloc(received);
    ret->datasize = received;
    memcpy(ret->data, response, received);
    return ret;
}

int main(int argc, char **argv)
{
    // sendReq(NULL);
    // return 0;
    FILE* block_file = fopen("blocked_list.txt", "r");
    char* block_line = NULL;
    size_t link_len = 0;
    ssize_t bytes_read;
    if (block_file != NULL) {
        while ((bytes_read = getline(&block_line, &link_len, block_file)) != -1) {

            block_line[strlen(block_line)-1] = '\0';
            char* arr_str = malloc(strlen(block_line)+1);
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
        //kchar* buff_cpy = malloc(256);
        //kstrcpy(buff_cpy, buffer);
        //kchar* sep = " ";
        //kchar* word;
        //kword = strtok(buff_cpy, sep);
        //kprintf("Word: %s\n", word);
        int target_fd = parseReq(buffer);
        if (target_fd == -1) {
            write(client_fd, "", 0);
            strcpy(buffer,"GET http://cs241.cs.illinois.edu/ \r\nHost: cs241.cs.illinois.edu\r\n\r\n");
            char* page = "http://cs241.cs.illinois.edu/";
            char* host = "cs241.cs.illinois.edu";
            char* poststr = "";
            snprintf(buffer, 256, 
                         "GET %s HTTP/1.0\r\n"  // POST or GET, both tested and works. Both HTTP 1.0 HTTP 1.1 works, but sometimes
                         "Host: %s\r\n"     // but sometimes HTTP 1.0 works better in localhost type
                         "Content-type: application/x-www-form-urlencoded\r\n"
                         "Content-length: %d\r\n\r\n"
                         "%s\r\n", page, host, (unsigned int)strlen(poststr), poststr);
            target_fd = parseReq(buffer);
            response_t *res = sendReq(target_fd, buffer, strlen(buffer));
            //puts(res->data);
            //printf("HTTP/1.0 301 Moved Permanently\r\nLocation: %s\r\n", page);
            dprintf(client_fd, "HTTP/1.0 301 Moved Permanently\r\nLocation: %s\r\n", page);
            ssize_t rv = send(client_fd, res->data, res->datasize, 0);
            if (rv == -1)
                perror("send()");
            close(client_fd);
            continue;
        }
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
