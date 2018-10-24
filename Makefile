server : server.c common.h common.c
	gcc -Wall -g -o server server.c common.c -lpthread common.h

test : test.c common.h common.c
	gcc -Wall -g -o test test.c common.c -lpthread common.h

clean : 
	rm ./server
	rm ./test