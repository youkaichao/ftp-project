
#include "common.h"

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
		struct ThreadData * pthreadData = (struct ThreadData *) malloc(sizeof (struct ThreadData));
		pthreadData->pthread_id = &thread_id;
		pthreadData->pconnfd = &connfd;
		pthreadData->userState = JUST_CONNECTED;
		pthread_create(&thread_id, NULL, connection_thread, (void *)pthreadData);
	}
	close(listenfd);
}

void *connection_thread(void *vargp)
{
	struct ThreadData* pThreadData = (struct ThreadData*)vargp;
	int thread_id = *(pThreadData->pthread_id);
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(!writeNullTerminatedString(connfd, CONNECT_OK_MSG))
	{
		goto endConnection;
	}

	while(1)
	{
		// read one command that ends with <CRLF> and dispath it
		int p = 0;
		int n;
		while(1)
		{
			n = read(connfd, buffer + p, BUFFER_SIZE - 1 - p);
			if (n < 0) {
				printf("Error read(): %s(%d)\n", strerror(errno), errno);
				goto endConnection;
			}
			if(n == 0)
			{// the client closed the connection
				goto endConnection;
			}
			p += n;
			if(p < 2)
			{// each command has CRLF and is at lest 2 characters long
				continue;
			}
			if(!strcmp(buffer + p - 2, "\r\n"))
			{// command end with CRLF
				buffer[p] = '\0';
				if(!dispatchCommand(pThreadData))
				{
					goto endConnection;
				}
				break;
			}else{
				if(p == BUFFER_SIZE - 1)
				{// buffer is full
					if(!writeNullTerminatedString(connfd, UNKNOWN_COMMAND_MSG))
					{
						goto endConnection;
					}
					break;
				}
				else{
					continue;
				}
			}
		}
	}

endConnection:
	free(vargp);
	close(connfd);
	return 0;
}

int writeNullTerminatedString(int fd, const char* str)
{
	int n = write(fd, str, strlen(str));
	if (n < 0) {
		printf("Error write(): %s(%d)\n", strerror(errno), errno);
		return 0;
	}
	return 1;
}

int dispatchCommand(struct ThreadData* pThreadData)
{
	int thread_id = *(pThreadData->pthread_id);
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;

	/* parse the command*/
	char command[5];
	int len = strlen(buffer) - 2;// len of command, \r\n not included
	char* space_pos = strchr(buffer, (int)' ');
	if(space_pos)
	{// space found
		len = space_pos - buffer;
	}
	if(len >= 5)
	{// len >= 5, wrong command
		return writeNullTerminatedString(connfd, UNKNOWN_COMMAND_MSG);
	}
	int i = 0;
	for (; i < len; ++i)
	{
		command[i] = toupper(buffer[i]);
	}
	command[i] = '\0';

	// find the command id
	i = 0;
	for(; i < NUM_OF_COMMANDS; ++i)
	{
		if(!strcmp(command, command_to_string[i]))
		{
			break;
		}
	}
	return handlers[i](pThreadData);
}

/*
====================== miscellaneous commands ====================================
*/
int WRONG_COMMAND_handler(struct ThreadData* pThreadData)
{
	int connfd = *(pThreadData->pconnfd);
	return writeNullTerminatedString(connfd, UNKNOWN_COMMAND_MSG);
}


int QUIT_handler(struct ThreadData* pThreadData)
{
	int connfd = *(pThreadData->pconnfd);
	writeNullTerminatedString(connfd, QUIT_MSG);
	return 0;
}


int SYST_handler(struct ThreadData* pThreadData)
{
	int connfd = *(pThreadData->pconnfd);
	return writeNullTerminatedString(connfd, SYST_MSG);
}

// TODO:
int TYPE_handler(struct ThreadData* pThreadData)
{
	int thread_id = *(pThreadData->pthread_id);
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
}

/*
====================== login commands ====================================
*/
int USER_handler(struct ThreadData* pThreadData)
{
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState > AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, ALREADY_LOGGED_IN_MSG);
	}
	if(strcmp(buffer + 4, " anonymous"))
	{// user name not anonymous, unknwon command
		pThreadData->userState = JUST_CONNECTED;
		return WRONG_COMMAND_handler(pThreadData);
	}
	// user name OK
	pThreadData->userState = HAS_USER_NAME;
	return writeNullTerminatedString(connfd, USERNAMR_OK_MSG);
}


int PASS_handler(struct ThreadData* pThreadData)
{
	int thread_id = *(pThreadData->pthread_id);
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	
	switch (userState)
	{
		case JUST_CONNECTED:
			return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
			break;
		case HAS_USER_NAME:
			pThreadData->userState = LOGGED_IN;
			return writeNullTerminatedString(connfd, PASSWORD_OK_MSG);
			break;
		case LOGGED_IN:
		case RNFR_STATE:
		case AUTHORIZATION_LINE:
		default:
			return writeNullTerminatedString(connfd, ALREADY_LOGGED_IN_MSG);
			break;
	}
}

/*
====================== commands with data connection====================================
*/

// TODO:
int PORT_handler(struct ThreadData* pThreadData)
{
	int thread_id = *(pThreadData->pthread_id);
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
}

// TODO:
int PASV_handler(struct ThreadData* pThreadData)
{
	int thread_id = *(pThreadData->pthread_id);
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
}

// TODO:
int RETR_handler(struct ThreadData* pThreadData)
{
	int thread_id = *(pThreadData->pthread_id);
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
}

// TODO:
int STOR_handler(struct ThreadData* pThreadData)
{
	int thread_id = *(pThreadData->pthread_id);
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
}

// TODO:
int LIST_handler(struct ThreadData* pThreadData)
{
	int thread_id = *(pThreadData->pthread_id);
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState  < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
}


/*
====================== directory related commands ====================================
*/
// TODO:
int MKD_handler(struct ThreadData* pThreadData)
{
	int thread_id = *(pThreadData->pthread_id);
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
}

// TODO:
int CWD_handler(struct ThreadData* pThreadData)
{
	int thread_id = *(pThreadData->pthread_id);
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState  < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
}

// TODO:
int PWD_handler(struct ThreadData* pThreadData)
{
	int thread_id = *(pThreadData->pthread_id);
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState  < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
}


// TODO:
int RMD_handler(struct ThreadData* pThreadData)
{
	int thread_id = *(pThreadData->pthread_id);
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState  < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
}

// TODO:
int RNFR_handler(struct ThreadData* pThreadData)
{
	int thread_id = *(pThreadData->pthread_id);
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState  < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
}

// TODO:
int RNTO_handler(struct ThreadData* pThreadData)
{
	int thread_id = *(pThreadData->pthread_id);
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
}
