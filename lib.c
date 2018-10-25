#include "common.h"

void *connection_thread(void *vargp)
{
	struct ThreadData* pThreadData = (struct ThreadData*)vargp;
	int connfd = pThreadData->connfd;
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
	int connfd = pThreadData->connfd;
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

int read_or_write_file(char* filename, struct ThreadData* pThreadData, int flag)
{
	if(!(flag == O_RDONLY || flag == O_WRONLY))
	{
		return 0;
	}
	int connfd = pThreadData->connfd;
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