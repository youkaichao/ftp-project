server : server.c
	gcc -w -g -o server server.c -lpthread
clean : server
	rm server