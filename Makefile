server: server.c server.h utils.c
	gcc -g -pthread server.c set.c -o server
ssl-redirect: ssl-redirect.c utils.c
	gcc -pthread ssl-redirect.c set.c -o ssl-redirect
clean:
	rm ssl-redirect server
