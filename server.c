
#include "common.h"

int main(int argc, char **argv)
{
	// check arguments and set port and root_dir
	if (argc != 1)
	{
		assertf(argc == 5, "You have %d arguments(only 1 or 5 arguments are accepted!)", argc);
		if (!strcmp(argv[1], "-port"))
		{
			host_port = atoi(argv[2]);
			strcpy(root_dir, argv[4]);
		}
		else if (!strcmp(argv[1], "-root"))
		{
			host_port = atoi(argv[4]);
			strcpy(root_dir, argv[2]);
		}
		else
		{
			assertf(1 > 2, "Wrong input arguments!(must be (--port n --root /path) or (--root /path --port n))");
		}
		// make sure the root dir doesn't have the slash as its end.
		char *p = root_dir;
		p += strlen(root_dir);
		if (*(p - 1) == '/')
		{
			p -= 1;
			*p = '\0';
		}
	}

	int listenfd, connfd;
	struct sockaddr_in addr;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(host_port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	if (listen(listenfd, 10) == -1)
	{
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	while (1)
	{
		if ((connfd = accept(listenfd, NULL, NULL)) == -1)
		{
			printf("Error accept(): %s(%d)\n", strerror(errno), errno);
			continue;
		}
		pthread_t thread_id;
		struct ThreadData *pthreadData = (struct ThreadData *)malloc(sizeof(struct ThreadData));
		pthreadData->connfd = connfd;
		pthreadData->userState = JUST_CONNECTED;
		pthreadData->dataConnectionStatus = None_Data_Connection;
		strcpy(pthreadData->cwd, root_dir);
		pthread_create(&thread_id, NULL, connection_thread, (void *)pthreadData);
	}
	close(listenfd);
}