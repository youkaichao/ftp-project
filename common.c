#include "common.h"

char CONNECT_OK_MSG[] = "220 Anonymous FTP server ready.\r\n";
char UNKNOWN_COMMAND_MSG[] = "500 Syntax error, command unrecognized.\r\n";
char NOT_LOGGED_IN_MSG[] = "530 Not logged in.\r\n";
char ALREADY_LOGGED_IN_MSG[] = "530 Already Logged In.\r\n";
char USERNAMR_OK_MSG[] = "331 User name okay, need email as password.\r\n";
char PASSWORD_OK_MSG[] = "230 User logged in, proceed.\r\n";
char TYPE_SET_MSG[] = "200 Type set to I.\r\n";
char QUIT_MSG[] = "221 Bye bye.\r\n";
char SYST_MSG[] = "215 UNIX Type: L8\r\n";
char WRONG_PATH_MSG[] = "530 Wrong Path!\r\n";
char CREATED_PATH_MSG[] = "257 \"%s\" created!\r\n";
char CWD_OK_MSG[] = "250 Requested file action okay, completed.\r\n";
char PWD_OK_MSG[] = "257 \"%s\" is your current location.\r\n";
char RMD_OK_MSG[] = "250 Requested file action okay, completed.\r\n";

char* command_to_string[] = {
[USER] = "USER", 
[PASS] = "PASS", 
[RETR] = "RETR", 
[STOR] = "STOR", 
[QUIT] = "QUIT", 
[SYST] = "SYST", 
[TYPE] = "TYPE", 
[PORT] = "PORT", 
[PASV] = "PASV", 
[MKD] = "MKD", 
[CWD] = "CWD", 
[PWD] = "PWD", 
[LIST] = "LIST", 
[RMD] = "RMD", 
[RNFR] = "RNFR", 
[RNTO] = "RNTO",
};

HANDLER handlers[] = {
[USER] = USER_handler,
[PASS] = PASS_handler,
[RETR] = RETR_handler,
[STOR] = STOR_handler,
[QUIT] = QUIT_handler,
[SYST] = SYST_handler,
[TYPE] = TYPE_handler,
[PORT] = PORT_handler,
[PASV] = PASV_handler,
[MKD] = MKD_handler,
[CWD] = CWD_handler,
[PWD] = PWD_handler,
[LIST] = LIST_handler,
[RMD] = RMD_handler,
[RNFR] = RNFR_handler,
[RNTO] = RNTO_handler,
[NUM_OF_COMMANDS] = WRONG_COMMAND_handler
};

char root_dir[MAX_DIRECTORY_SIZE] = "/tmp";
int host_port = 21;


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
			// you don't know if this is one command or only part of the command. so buffer is not a zero-terminated string
			if((buffer[p - 2] == '\r') && (buffer[p - 1] == '\n'))
			{// command end with CRLF, delete \r\n and then dispatch this command
				buffer[p - 2] = '\0';
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
	int len = strlen(buffer);// len of command
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

int TYPE_handler(struct ThreadData* pThreadData)
{
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
	if(strcmp(buffer + 4, " I"))
	{// command is not "TYPE I"
		return WRONG_COMMAND_handler(pThreadData);
	}
	return writeNullTerminatedString(connfd, TYPE_SET_MSG);
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

int join_path(char* patha, char* pathb)
{
	// remove the last / 
	char* pos_a_end = patha + strlen(patha);
	if(*(pos_a_end - 1) == '/')
	{
		pos_a_end -= 1;
		*(pos_a_end) = '\0';
	}
	char* pos_b_end = pathb + strlen(pathb);
	char* left = pathb;
	while(left < pos_b_end)
	{
		char* slash_pos = strchr(left, (int)'/');
		if(!slash_pos)
		{
			slash_pos = pos_b_end;
		}
		if(slash_pos == left && *slash_pos == '/')
		{// start with /, wrong path name
			return 0;
		}
		*slash_pos = '\0';
		// [left, slash_pos) now is a dirname or filename
		if(!strcmp(left, "."))
		{
			left = slash_pos + 1;
			continue;
		}
		if(*left == '.' && *(left + 1) == '.')
		{// have .. , remove the last basename in patha
			while(pos_a_end > patha && *(pos_a_end - 1) != '/')
			{
				pos_a_end -= 1;
			}
			if(pos_a_end == patha)
			{
				return 0;
			}
			pos_a_end -= 1;
			*pos_a_end = '\0';
			left = slash_pos + 1;
			continue;
		}
		if(*left == '.' && *(left + 1) == '.' & *(left + 2) == '.')
		{// have ... which is invalid
			return 0;
		}
		*pos_a_end = '/';
		pos_a_end += 1;
		strcpy(pos_a_end, left);
		pos_a_end += slash_pos - left;
		left = slash_pos + 1;
	}
}

int dispose_path(char* buffer, char* command, int proceding, char* cwd, char* root_dir)
{
	strcpy(buffer, cwd);
	if(!join_path(buffer, command + proceding))
	{
		return 0;
	}
	if(strlen(buffer) < strlen(root_dir))
	{// can not exceed ``root_dir``
		return 0;
	}
	int len_root = strlen(root_dir);
	char tmp = buffer[len_root];
	buffer[len_root] = '\0';
	if(strcmp(buffer, root_dir))
	{// path should start with ``root_dir``
		return 0;
	}
	buffer[len_root] = tmp;
	return 1;
}

int MKD_handler(struct ThreadData* pThreadData)
{
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
	char tmpDir[MAX_DIRECTORY_SIZE];
	if(!dispose_path(tmpDir, buffer, 4, pThreadData->cwd, root_dir))
	{
		return writeNullTerminatedString(connfd, WRONG_PATH_MSG);
	}
	if(mkdir(tmpDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
	{
		return writeNullTerminatedString(connfd, WRONG_PATH_MSG);
	}
	char tmpMsg[MAX_DIRECTORY_SIZE];
	sprintf(tmpMsg, CREATED_PATH_MSG, tmpDir);
	return writeNullTerminatedString(connfd, tmpMsg);
}

int CWD_handler(struct ThreadData* pThreadData)
{
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState  < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
	char tmpDir[MAX_DIRECTORY_SIZE];
	if(!dispose_path(tmpDir, buffer, 4, pThreadData->cwd, root_dir))
	{
		return writeNullTerminatedString(connfd, WRONG_PATH_MSG);
	}
	struct stat s = {0};
	stat(tmpDir, &s);
	if(!(s.st_mode & S_IFDIR))
	{
		return writeNullTerminatedString(connfd, WRONG_PATH_MSG);
	}
	strcpy(pThreadData->cwd, tmpDir);
	return writeNullTerminatedString(connfd, CWD_OK_MSG);
}

int PWD_handler(struct ThreadData* pThreadData)
{
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState  < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
	char tmpMsg[MAX_DIRECTORY_SIZE];
	sprintf(tmpMsg, PWD_OK_MSG, pThreadData->cwd);
	return writeNullTerminatedString(connfd, tmpMsg);
}

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
	char tmpDir[MAX_DIRECTORY_SIZE];
	if(!dispose_path(tmpDir, buffer, 4, pThreadData->cwd, root_dir))
	{
		return writeNullTerminatedString(connfd, WRONG_PATH_MSG);
	}
	struct stat s = {0};
	stat(tmpDir, &s);
	if(!(s.st_mode & S_IFDIR))
	{
		return writeNullTerminatedString(connfd, WRONG_PATH_MSG);
	}
	// rmdir, what if it removes its cwd?
	if(!strcmp(pThreadData->cwd, tmpDir))
	{// it tries to remove its current working directory!
		return writeNullTerminatedString(connfd, WRONG_PATH_MSG);
	}
	if(!rmdir(tmpDir))
	{// success
		return writeNullTerminatedString(connfd, RMD_OK_MSG);
	}
	else{
		return writeNullTerminatedString(connfd, WRONG_PATH_MSG);
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
