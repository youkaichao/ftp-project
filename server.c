#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>

#define clean_errno() (errno == 0 ? "None" : strerror(errno))
#define log_error(M, ...) fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)
#define assertf(A, M, ...) if(!(A)) {log_error(M, ##__VA_ARGS__); assert(A); }

struct ThreadData{
	pthread_t* pthread_id;//pointer to thread id
	int* pconnfd;// pointer to connection file descriptor
};

void *connection_thread(void *vargp);
int writeNullTerminatedString(int fd, const char* str);
//void *connection_data(void *vargp);

#define CONNECT_OK 	     220

char* const_msg[] = {
	[CONNECT_OK] = "220 Anonymous FTP server ready.\r\n",
};

char root_dir[2000] = "/tmp";
int host_port = 21;

int main(int argc, char **argv) {
	// check arguments and set port and root_dir
	if(argc != 1)
	{
		assertf(argc == 5, "You have %d arguments(only 1 or 5 arguments are accepted!)", argc);
		if(!strcmp(argv[1], "-port"))
		{
			host_port = atoi(argv[2]);
			strcpy(root_dir, argv[4]);
		}else if(!strcmp(argv[1], "-root"))
		{
			host_port = atoi(argv[4]);
			strcpy(root_dir, argv[2]);
		}else{
			assertf(1 > 2, "Wrong input arguments!(must be (--port n --root /path) or (--root /path --port n))");
		}
	}

	int listenfd, connfd;
	struct sockaddr_in addr;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(host_port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	if (listen(listenfd, 10) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	while (1) {
		if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
			printf("Error accept(): %s(%d)\n", strerror(errno), errno);
			continue;
		}
		pthread_t thread_id;
		struct ThreadData threadData;
		threadData.pthread_id = &thread_id;
		threadData.pconnfd = &connfd;
		pthread_create(&thread_id, NULL, connection_thread, (void *)&threadData);
	}
	close(listenfd);
}

void *connection_thread(void *vargp)
{
	struct ThreadData* p = (struct ThreadData*)vargp;
	int thread_id = *(p->pthread_id);
	int connfd = *(p->pconnfd);
	if(!writeNullTerminatedString(connfd, const_msg[CONNECT_OK]))
	{
		printf("thread id is %d", thread_id);
		close(connfd);
		return 0;
	}
	printf("thread id is %d", thread_id);

	// p = 0;
	// while (1) {
	// 	int n = read(connfd, sentence + p, 8191 - p);
	// 	if (n < 0) {
	// 		printf("Error read(): %s(%d)\n", strerror(errno), errno);
	// 		close(connfd);
	// 		continue;
	// 	} else if (n == 0) {
	// 		break;
	// 	} else {
	// 		p += n;
	// 		if (sentence[p - 1] == '\n') {
	// 			break;
	// 		}
	// 	}
	// }
	// sentence[p - 1] = '\0';
	// len = p - 1;
	
	// for (p = 0; p < len; p++) {
	// 	sentence[p] = toupper(sentence[p]);
	// }

	close(connfd);
	return 0;
}

/*
return 1 if it writes successfully, 0 otherwise 
*/
int writeNullTerminatedString(int fd, const char* str)
{
	int n = write(fd, str, strlen(str));
	if (n < 0) {
		printf("Error write(): %s(%d)\n", strerror(errno), errno);
		return 0;
	}
	return 1;
}