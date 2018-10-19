server : server.c
	gcc -Wall server.c -o server
clean : server
	rm server