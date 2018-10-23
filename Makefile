server : server.c common.h common.c
	gcc -w -g -o server server.c common.c -lpthread common.h

test : test.c common.h common.c
	gcc -w -g -o test test.c common.c -lpthread common.h

clean : 
	rm ./server
	rm ./test