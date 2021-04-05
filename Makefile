redirect: redirect.c
	gcc redirect.c -o redirect
ssl-redirect: ssl-redirect.c
	gcc ssl-redirect.c -o ssl-redirect -L/usr/local/ssl/lib -lssl -lcrypto
clean:
	rm redirect ssl-redirect