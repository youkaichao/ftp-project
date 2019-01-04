server : common.h global.c server.c lib.c handlers.c
	gcc -Wall -g -o server $^ -lpthread

clean : 
	rm ./server