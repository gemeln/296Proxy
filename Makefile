server: server.c server.h
	gcc -g -pthread server.c -o server
ssl-redirect: ssl-redirect.c utils.c
	gcc -pthread ssl-redirect.c -o ssl-redirect
clean:
	rm ssl-redirect server