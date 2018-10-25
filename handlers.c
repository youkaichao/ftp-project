#include "common.h"

/*
====================== miscellaneous commands ====================================
*/
int WRONG_COMMAND_handler(struct ThreadData* pThreadData)
{
	int connfd = pThreadData->connfd;
	return writeNullTerminatedString(connfd, UNKNOWN_COMMAND_MSG);
}


int QUIT_handler(struct ThreadData* pThreadData)
{
	int connfd = pThreadData->connfd;
	writeNullTerminatedString(connfd, QUIT_MSG);
	return 0;
}


int SYST_handler(struct ThreadData* pThreadData)
{
	int connfd = pThreadData->connfd;
	return writeNullTerminatedString(connfd, SYST_MSG);
}

int TYPE_handler(struct ThreadData* pThreadData)
{
	int connfd = pThreadData->connfd;
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
	int connfd = pThreadData->connfd;
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
	
	int connfd = pThreadData->connfd;
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

int PORT_handler(struct ThreadData* pThreadData)
{
	
	int connfd = pThreadData->connfd;
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
	// check if the command is legal
	if(buffer[4] != ' ')
	{
		return writeNullTerminatedString(connfd, UNKNOWN_COMMAND_MSG);
	}
	// assume the port command is legal
	
	if(pThreadData->dataConnectionStatus == PASV_Data_Connection)
	{
		// destroy the listen socket
		// close(pThreadData->data_connfd);
		close(pThreadData->data_listenfd);
	}
	pThreadData->dataConnectionStatus = PORT_Data_Connection;

	char* commas_pos[5];
	char* start_pos = buffer + 5;
	for(int i = 0; i < 5; ++i)
	{
		while(*start_pos != ',')
		{
			start_pos += 1;
		}
		commas_pos[i] = start_pos;
		start_pos += 1;
	}
	start_pos = buffer + 5;// reset the start position
	*(commas_pos[3]) = '\0';
	*(commas_pos[4]) = '\0';
	int p1 = atoi(commas_pos[3] + 1);
	int p2 = atoi(commas_pos[4] + 1);
	int port = p1 * 256 + p2;
	for(int i = 0; i < 3; ++i)
	{
		*(commas_pos[i]) = '.'; // replace ',' with '.'
	}
	memset(&(pThreadData->data_addr), 0, sizeof(pThreadData->data_addr));
	pThreadData->data_addr.sin_family = AF_INET;
	pThreadData->data_addr.sin_port = htons(port);
	inet_pton(AF_INET, start_pos, &((pThreadData->data_addr).sin_addr.s_addr));
	// now pThreadData->data_addr is set to be an legal address
	return writeNullTerminatedString(connfd, PORT_OK_MSG);
}

int PASV_handler(struct ThreadData* pThreadData)
{
	int connfd = pThreadData->connfd;
	enum UserState userState = pThreadData->userState;
	if(userState < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
	// just overwrite the previous data mode
	pThreadData->dataConnectionStatus = PASV_Data_Connection;

	// create the server socket
	pThreadData->data_listenfd = socket(AF_INET, SOCK_STREAM, 0);
	
	memset(&(pThreadData->data_addr), 0, sizeof(pThreadData->data_addr));
	(pThreadData->data_addr).sin_family = AF_INET;
	(pThreadData->data_addr).sin_port = htons(0);
	(pThreadData->data_addr).sin_addr.s_addr = htonl(INADDR_ANY);

	bind(pThreadData->data_listenfd, (struct sockaddr*)&(pThreadData->data_addr), sizeof(pThreadData->data_addr));

	listen(pThreadData->data_listenfd, 1);

	// parse ip and port to construct the reply msg
    socklen_t n = sizeof (pThreadData->data_addr);
    getsockname(pThreadData->data_listenfd, (struct sockaddr*)&(pThreadData->data_addr), &n);
	int port = (int)(ntohs((pThreadData->data_addr).sin_port));
	//connfd = accept(listenfd, NULL, NULL)
	int p1 = port / 256;
	int p2 = port % 256;
	char ip_buffer[MAX_DIRECTORY_SIZE];
	strcpy(ip_buffer, get_host_ip());
	char* p = ip_buffer;
	while(*p)
	{
		if(*p == '.')
		{
			*p = ',';
		}
		p += 1;
	}

	char msg_buffer[MAX_DIRECTORY_SIZE];
	sprintf(msg_buffer, PASV_OK_MSG, ip_buffer, p1, p2);
	return writeNullTerminatedString(connfd, msg_buffer);
}


int RETR_handler(struct ThreadData* pThreadData)
{
	
	int connfd = pThreadData->connfd;
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
	char tmpDir[MAX_DIRECTORY_SIZE];
	if(!dispose_path(tmpDir, buffer, 5, pThreadData->cwd, root_dir))
	{
		return writeNullTerminatedString(connfd, WRONG_PATH_MSG);
	}
	struct stat s = {0};
	stat(tmpDir, &s);
	if(!(s.st_mode & S_IFREG))
	{
		return writeNullTerminatedString(connfd, WRONG_PATH_MSG);
	}
	if(pThreadData->dataConnectionStatus == None_Data_Connection)
	{
		return writeNullTerminatedString(connfd, NO_DATA_CONNECTION_MSG);
	}

	if(pThreadData->dataConnectionStatus == PASV_Data_Connection)
	{
		pThreadData->data_connfd = accept(pThreadData->data_listenfd, NULL, NULL);
	}
	read_or_write_file(tmpDir, pThreadData, O_RDONLY);
	if(pThreadData->dataConnectionStatus == PASV_Data_Connection)
	{
		close(pThreadData->data_listenfd);
	}
	return 1;
}

int STOR_handler(struct ThreadData* pThreadData)
{
	
	int connfd = pThreadData->connfd;
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
	char tmpDir[MAX_DIRECTORY_SIZE];
	if(!dispose_path(tmpDir, buffer, 5, pThreadData->cwd, root_dir))
	{
		return writeNullTerminatedString(connfd, WRONG_PATH_MSG);
	}
	if(pThreadData->dataConnectionStatus == None_Data_Connection)
	{
		return writeNullTerminatedString(connfd, NO_DATA_CONNECTION_MSG);
	}
	if(pThreadData->dataConnectionStatus == PASV_Data_Connection)
	{
		pThreadData->data_connfd = accept(pThreadData->data_listenfd, NULL, NULL);
	}
	read_or_write_file(tmpDir, pThreadData, O_WRONLY);
	if(pThreadData->dataConnectionStatus == PASV_Data_Connection)
	{
		close(pThreadData->data_listenfd);
	}
	return 1;
}

int LIST_handler(struct ThreadData* pThreadData)
{
	
	int connfd = pThreadData->connfd;
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState  < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
	if(pThreadData->dataConnectionStatus == None_Data_Connection)
	{
		return writeNullTerminatedString(connfd, NO_DATA_CONNECTION_MSG);
	}
	// ``tmpDir`` is the dir to be listed
	char tmpDir[MAX_DIRECTORY_SIZE];
	if(buffer[4] == ' ')
	{// has pathname argument
		if(!dispose_path(tmpDir, buffer, 5, pThreadData->cwd, root_dir))
		{
			return writeNullTerminatedString(connfd, WRONG_PATH_MSG);
		}
	}else{
		strcpy(tmpDir, pThreadData->cwd);
	}

	// what if dir does not exist
	struct stat tmps = {0};
	stat(tmpDir, &tmps);
	if(!(tmps.st_mode & S_IFDIR))
	{
		return writeNullTerminatedString(connfd, WRONG_PATH_MSG);
	}

	char tmpFilename[MAX_DIRECTORY_SIZE];
	strcpy(tmpFilename, pThreadData->cwd);
	// invoke ls and save its output to a file, then send to the client
	// ============ append / to the last
	char* pos = tmpFilename + strlen(tmpFilename);
	if(*(pos - 1) != '/')
	{
		*pos = '/';
		pos += 1;
		*pos = '\0';
	}
	while(1)
	{
		// =============== try tmp file named cwd/aaaaaa until it is not a file
		*pos = 'a';
		pos += 1;
		*pos = '\0';
		struct stat s = {0};
		stat(tmpFilename, &s);
		if((s.st_mode & S_IFREG) || (s.st_mode & S_IFDIR))
		{
			continue;
		}else{
			break;
		}
	}
	// invoke ls and save the output into the temp file
	file_ls(tmpFilename, tmpDir);


	if(pThreadData->dataConnectionStatus == PASV_Data_Connection)
	{
		pThreadData->data_connfd = accept(pThreadData->data_listenfd, NULL, NULL);
	}
	read_or_write_file(tmpFilename, pThreadData, O_RDONLY);
	if(pThreadData->dataConnectionStatus == PASV_Data_Connection)
	{
		close(pThreadData->data_listenfd);
	}
	remove(tmpFilename);
	return 1;
}


/*
====================== directory related commands ====================================
*/

int MKD_handler(struct ThreadData* pThreadData)
{
	int connfd = pThreadData->connfd;
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
	int connfd = pThreadData->connfd;
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
	int connfd = pThreadData->connfd;
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
	
	int connfd = pThreadData->connfd;
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

int RNFR_handler(struct ThreadData* pThreadData)
{
	int connfd = pThreadData->connfd;
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState  < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
	char tmpDir[MAX_DIRECTORY_SIZE];
	if(!dispose_path(tmpDir, buffer, 5, pThreadData->cwd, root_dir))
	{
		return writeNullTerminatedString(connfd, WRONG_PATH_MSG);
	}
	struct stat s = {0};
	stat(tmpDir, &s);
	// it must be directory or file
	if(!((s.st_mode & S_IFREG) || (s.st_mode & S_IFDIR)))
	{
		return writeNullTerminatedString(connfd, WRONG_PATH_MSG);
	}
	pThreadData->userState = RNFR_STATE;
	strcpy(pThreadData->RNFR_buffer, tmpDir);
	return writeNullTerminatedString(connfd, RNFR_OK_MSG);
}

int RNTO_handler(struct ThreadData* pThreadData)
{
	int connfd = pThreadData->connfd;
	char* buffer = pThreadData->buffer;
	enum UserState userState = pThreadData->userState;
	if(userState < AUTHORIZATION_LINE)
	{
		return writeNullTerminatedString(connfd, NOT_LOGGED_IN_MSG);
	}
	if(userState != RNFR_STATE)
	{
		return writeNullTerminatedString(connfd, RNTO_NO_RNFR_MSG);
	}
	char tmpDir[MAX_DIRECTORY_SIZE];
	if(!dispose_path(tmpDir, buffer, 5, pThreadData->cwd, root_dir))
	{
		return writeNullTerminatedString(connfd, WRONG_PATH_MSG);
	}
	pThreadData->userState = LOGGED_IN;
	if(rename(pThreadData->RNFR_buffer, tmpDir))
	{// something wrong
		return writeNullTerminatedString(connfd, RNTO_ERROR_MSG);
	}else{
		return writeNullTerminatedString(connfd, RNTO_OK_MSG);
	}
}
