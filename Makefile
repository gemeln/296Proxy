main: server client

server: server.c
	gcc server.c -o server
client: client.c
	gcc client.c -o client
ssl-redirect: ssl-redirect.c
	gcc ssl-redirect.c -o ssl-redirect -L/usr/local/ssl/lib -lssl -lcrypto
clean:
	rm redirect ssl-redirect