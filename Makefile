main: server client

server: server.c
	gcc server.c -o server
client: client.c
	gcc client.c -o client
ssl-redirect: ssl-redirect.c utils.c
	gcc -pthread ssl-redirect.c -o ssl-redirect
clean:
	rm redirect ssl-redirect