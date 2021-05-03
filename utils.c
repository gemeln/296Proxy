#pragma once
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct hostinfo_ {
    char hostname[1024];
    char port[32];
} hostinfo;

ssize_t read_all_from_socket(int socket, char* buffer, size_t count) {
    size_t amountRead = 0;
    while (amountRead < count) {
        int bytesRead = read(socket, buffer + amountRead, count - amountRead);
        printf("Read partial %d bytes\n", bytesRead);
        if (bytesRead < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                return -1;
            }
        }
        if (bytesRead == 0) {
            break;
        }
        amountRead += bytesRead;
    }
    return amountRead;
}

ssize_t write_all_to_socket(int socket, const char* buffer, size_t count) {
    // Try to write one byte at a time
    size_t attempt = 0;
    while (attempt < count) {
        int returnValue = write(socket, buffer + attempt, count - attempt);
        if (returnValue == 0) {
            break;
        } else if (returnValue == -1) {
            // Something went wrong
            if (errno == EINTR) {
                continue;
            } else {
                return -1;
            }
        }
        attempt += returnValue;
    }

    return attempt;
}
int connect_to_server(const char* host, const char* port) {
    struct addrinfo current, *result;
    memset(&current, 0, sizeof(struct addrinfo));
    current.ai_family = AF_INET;
    current.ai_socktype = SOCK_STREAM;

    int s = getaddrinfo(host, port, &current, &result);
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    int c = connect(serverSocket, result->ai_addr, result->ai_addrlen);

    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return -1;
    }

    if (c == -1) {
        perror("connect():");
        freeaddrinfo(result);
        return -1;
    }
    freeaddrinfo(result);
    return serverSocket;
}
bool parseHost(char* request, hostinfo* data) {
    fprintf(stderr, "Parsing host from %s\n", request);
    memset(data, 0, sizeof(hostinfo));
    char* hostStart = strstr(request, "Host: ") + strlen("Host: ");
    char* colon = strstr(hostStart, ":");
    assert(colon);
    strncpy(data->hostname, hostStart, colon - hostStart);
    char* portStart = colon + 1;
    char* end = strstr(portStart, "\r\n");
    strncpy(data->port, portStart, end - portStart);
}
