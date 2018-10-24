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
char RNFR_OK_MSG[] = "350 RNFR accepted - file exists, ready for destination\r\n";
char RNTO_NO_RNFR_MSG[] = "503 Need RNFR before RNTO.\r\n";
char RNTO_ERROR_MSG[] = "451 Rename/move failure.\r\n";
char RNTO_OK_MSG[] = "250 File successfully renamed.\r\n";
char NO_DATA_CONNECTION_MSG[] = "425 No data connection.\r\n";
char OK_150_CONNECTION_MSG[] = "150 about to open data connection.\r\n";
char WRONG_425_CONNECTION_MSG[] = "425 Can't open data connection.\r\n";
char OK_226_CONNECTION_MSG[] = "226 Closing data connection,file transfer successful.\r\n";
char PORT_OK_MSG[] = "200 OK.\r\n";
char PASV_OK_MSG[] = "227 Entering PassiveMode (%s,%d,%d).\r\n";

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
char host_ip[100] = {0};

void *connection_thread(void *vargp)
{
	struct ThreadData* pThreadData = (struct ThreadData*)vargp;
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;
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
	int connfd = *(pThreadData->pconnfd);
	char* buffer = pThreadData->buffer;

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
	
	int connfd = *(pThreadData->pconnfd);
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
	
	int connfd = *(pThreadData->pconnfd);
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

char* get_host_ip()
{
	if(strlen(host_ip) == 0)
	{
		int listenfd = socket(AF_INET, SOCK_DGRAM, 0);
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(80);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		inet_pton(AF_INET, "8.8.8.8", &(addr.sin_addr.s_addr));
		connect(listenfd, (struct sockaddr*)&(addr), sizeof (addr));

		socklen_t n = sizeof addr;
		getsockname(listenfd, (struct sockaddr*)&addr, &n);
		inet_ntop(AF_INET, &(addr.sin_addr), host_ip, INET_ADDRSTRLEN);
		return host_ip;
	}else{
		return host_ip;
	}
}

// TODO:
int PASV_handler(struct ThreadData* pThreadData)
{
	int connfd = *(pThreadData->pconnfd);
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

int read_or_write_file(char* filename, struct ThreadData* pThreadData, int flag)
{
	if(!(flag == O_RDONLY || flag == O_WRONLY))
	{
		return 0;
	}
	int connfd = *(pThreadData->pconnfd);
	if(!writeNullTerminatedString(connfd, OK_150_CONNECTION_MSG))
	{// send 150 message
		return 0;
	}
	if(pThreadData->dataConnectionStatus == PORT_Data_Connection)
	{
		pThreadData->data_connfd = socket(AF_INET, SOCK_STREAM, 0);
		if(pThreadData->data_connfd == -1)
		{// can't open the connection
			return writeNullTerminatedString(connfd, WRONG_425_CONNECTION_MSG);
		}
		if(connect(pThreadData->data_connfd, (struct sockaddr *)&(pThreadData->data_addr), sizeof (pThreadData->data_addr)) == -1)
		{// can't open the connection
			close(pThreadData->data_connfd);
			return writeNullTerminatedString(connfd, WRONG_425_CONNECTION_MSG);
		}
	}
	// data connection ok, send data now!
	int fd;
	if(flag == O_RDONLY)
	{
		fd = open(filename, flag);
	}
	else{
		// write file, create if necessary
		fd = open(filename, flag | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	}
	char contentBuffer[BUFFER_SIZE];
	int readLen;
	int readfd, writefd;
	if(flag == O_RDONLY)
	{
		readfd = fd;
		writefd = pThreadData->data_connfd;
	}
	else{
		readfd = pThreadData->data_connfd;
		writefd = fd;
	}
	while((readLen = read(readfd, contentBuffer, BUFFER_SIZE)) == BUFFER_SIZE)
	{
		write(writefd, contentBuffer, readLen);
	}
	write(writefd, contentBuffer, readLen);
	writeNullTerminatedString(connfd, OK_226_CONNECTION_MSG);
	close(readfd);
	close(writefd);
	pThreadData->dataConnectionStatus = None_Data_Connection;
	return 1;
}

int file_ls(const char* filename, const char* dirname)
{
	FILE* file = fopen(filename, "w");
    DIR *mydir;
    struct dirent *myfile;
    struct stat mystat;

    char buf[MAX_DIRECTORY_SIZE];
    mydir = opendir(dirname);
    while((myfile = readdir(mydir)) != NULL)
    {
        sprintf(buf, "%s/%s", dirname, myfile->d_name);
        stat(buf, &mystat);
        fprintf(file, "%zu",mystat.st_size);
        fprintf(file, " %s\n", myfile->d_name);
    }
    closedir(mydir);
	fclose(file);
	return 1;
}

// TODO: retr in pasv mode
int RETR_handler(struct ThreadData* pThreadData)
{
	
	int connfd = *(pThreadData->pconnfd);
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

// TODO: stor in pasv mode
int STOR_handler(struct ThreadData* pThreadData)
{
	
	int connfd = *(pThreadData->pconnfd);
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

// TODO: list in pasv mode
int LIST_handler(struct ThreadData* pThreadData)
{
	
	int connfd = *(pThreadData->pconnfd);
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
		if(*left == '.' && *(left + 1) == '.' && *(left + 2) == '.')
		{// have ... which is invalid
			return 0;
		}
		*pos_a_end = '/';
		pos_a_end += 1;
		strcpy(pos_a_end, left);
		pos_a_end += slash_pos - left;
		left = slash_pos + 1;
	}
	return 1;
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

int RNFR_handler(struct ThreadData* pThreadData)
{
	int connfd = *(pThreadData->pconnfd);
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
	int connfd = *(pThreadData->pconnfd);
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
