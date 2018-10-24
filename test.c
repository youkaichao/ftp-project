#include "common.h"

char patha[MAX_DIRECTORY_SIZE], pathb[MAX_DIRECTORY_SIZE];

#define test_path_join(A, B)     strcpy(patha, A); \
    strcpy(pathb, B); \
    join_path(patha, pathb); \
    printf(patha); \
    printf("\r\n"); \

int main()
{
    // strcpy(patha, "/a/a");
    // strcpy(pathb, "a/d");
    // join_path(patha, pathb);
    // printf(patha);
    // printf("\r\n"); 
    // test_path_join("/a/a", "b/g");
    // test_path_join("/a/a/", "b/g");
    // test_path_join("/a/a", "b/g/");
    // test_path_join("/a/a/", "b/g/");
    // file_ls("a.txt", ".");


	// int listenfd, connfd;
	// struct sockaddr_in addr;

	// if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	// 	printf("Error socket(): %s(%d)\n", strerror(errno), errno);
	// 	return 1;
	// }
	// memset(&addr, 0, sizeof(addr));
	// addr.sin_family = AF_INET;
	// addr.sin_port = htons(0);
	// addr.sin_addr.s_addr = htonl(INADDR_ANY);
	// if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
	// 	printf("Error bind(): %s(%d)\n", strerror(errno), errno);
	// 	return 1;
	// }
    // int n = sizeof addr;
    // getsockname(listenfd, (struct sockaddr*)&addr, &n);
    // printf("I'm using %d port.\n", (int)(ntohs(addr.sin_port)));
	// char ip[] = "this is just placeholder";
	// inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN);
    // printf("my ip is %s\n", ip);
	// if (listen(listenfd, 10) == -1) {
	// 	printf("Error listen(): %s(%d)\n", strerror(errno), errno);
	// 	return 1;
	// }
    // getchar();
    // if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
    //     printf("Error accept(): %s(%d)\n", strerror(errno), errno);
    //     return 1;
    // }
    // char msg[] = "hell0\r\n";
    // write(connfd, msg, sizeof(msg));
    // close(connfd);
    // close(listenfd);
    int fd = open("a.txt", O_WRONLY | O_CREAT);
    write(fd, "123", 3);
    close(fd);
}