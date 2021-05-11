all: container.c
	gcc -g container.c -o container -Llibs/ -lvector -lsstring -lcallbacks -lm
mount:
	mount -n -t tmpfs -o size=500M none ./containerRoot