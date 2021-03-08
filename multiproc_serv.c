/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490"  // the port users will be connecting to
#define PORT2 "443"
#define MAXDATASIZE 100
#define BACKLOG 10	 // how many pending connections queue will hold

void sigchld_handler(int s)
{
	(void)s; // quiet unused variable warning

	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
	int sockfd, new_fd, numbytes;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
    char buf[MAXDATASIZE];
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			//if (send(new_fd, "Hello, world!", 13, 0) == -1)
			//	perror("send");
            if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
                perror("recv");
                exit(1);
            }

            buf[numbytes] = '\0';

            printf("server: received '%s'\n",buf);


            //Open new connection and act as a client. Get connection and send() back to original client

            struct addrinfo hints2, *servinfo2, *p2;
            int sockfd2, numbytes;
            memset(&hints2, 0, sizeof(hints2));
            hints2.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;

            int rv2 = getaddrinfo(buf, PORT2, &hints2, &servinfo2);

            for (p2 = servinfo2; p2 != NULL; p2 = p2->ai_next) {
                if ((sockfd2 = socket(p2->ai_family, p2->ai_socktype, p2->ai_protocol)) == -1) {
                    perror("client: socket");
                    continue;
                }

                if (connect(sockfd2, p2->ai_addr, p2->ai_addrlen) == -1) {
                    perror("client: connect");
                    close(sockfd2);
                    continue;
                }

                break;
            }

            if (p2 == NULL) {
                fprintf(stderr, "client: failed to connect :(\n");
                return 2;
            }

            char s[INET6_ADDRSTRLEN];
            inet_ntop(p2->ai_family, get_in_addr((struct sockaddr *)p2->ai_addr),
                    s, sizeof s);
            printf("client: connecting to %s\n", s);

            freeaddrinfo(servinfo2); // all done with this structure

            char *buffer;
            asprintf(&buffer, "GET %s HTTP/1.0\r\n"
                            "Connection: close\r\n" 
                            "Accept: */*\r\n\r\n", 
                            buf);
            //if (send(sockfd2, "Hello, world!", 13, 0) == -1)
                //perror("send");
            write(sockfd2, buffer, strlen(buffer));
            free(buffer);
            
            int numbytes2 = recv(sockfd2, buf, MAXDATASIZE-1, 0);

            if (numbytes2 == -1) {
                perror("recv");
                exit(1);
            }

            buf[numbytes] = '\0';
            printf("client: received '%s'\n",buf);
            if (send(sockfd, buf, sizeof(buf), 0) == -1)
                perror("send");



			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

