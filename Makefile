server : server.c
	gcc -Wall -g -o server server.c -lpthread
clean : server
	rm server