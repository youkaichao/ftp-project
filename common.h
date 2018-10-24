#ifndef COMMON
#define COMMON

#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <arpa/inet.h>

#define clean_errno() (errno == 0 ? "None" : strerror(errno))
#define log_error(M, ...) fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)
#define assertf(A, M, ...) if(!(A)) {log_error(M, ##__VA_ARGS__); assert(A); }

#define BUFFER_SIZE 2048
#define MAX_DIRECTORY_SIZE 200

extern char CONNECT_OK_MSG[];
extern char UNKNOWN_COMMAND_MSG[];
extern char NOT_LOGGED_IN_MSG[];
extern char ALREADY_LOGGED_IN_MSG[];
extern char USERNAMR_OK_MSG[];
extern char PASSWORD_OK_MSG[];
extern char TYPE_SET_MSG[];
extern char QUIT_MSG[];
extern char SYST_MSG[];
extern char WRONG_PATH_MSG[];
extern char CREATED_PATH_MSG[];
extern char CWD_OK_MSG[];
extern char RMD_OK_MSG[];
extern char RNFR_OK_MSG[];
extern char RNTO_NO_RNFR_MSG[];
extern char RNTO_ERROR_MSG[];
extern char RNTO_OK_MSG[];
extern char NO_DATA_CONNECTION_MSG[];
extern char OK_150_CONNECTION_MSG[];
extern char WRONG_425_CONNECTION_MSG[];
extern char OK_226_CONNECTION_MSG[];
extern char PORT_OK_MSG[];
extern char PASV_OK_MSG[];

enum UserState {
    JUST_CONNECTED, HAS_USER_NAME,
    AUTHORIZATION_LINE, // state that is less than this number can only log in 
    LOGGED_IN, RNFR_STATE};// RNFR is used for RNFR and RNTO

enum DataConnectionStatus{
    None_Data_Connection, PASV_Data_Connection, PORT_Data_Connection
};

struct ThreadData{
	int connfd;// connection file descriptor (control connection)
	char buffer[BUFFER_SIZE]; // reading buffer
    char cwd[MAX_DIRECTORY_SIZE]; // current working directory
    char RNFR_buffer[MAX_DIRECTORY_SIZE]; // buffer for saving rnfr command arguments
	enum UserState userState;
    enum DataConnectionStatus dataConnectionStatus;
    struct sockaddr_in data_addr;
    int data_listenfd;// fd for server socket (if in pasv mode)
    int data_connfd;//fd for connection socket (in pasv or port mode)
};

typedef int (*HANDLER)(struct ThreadData*);

int USER_handler(struct ThreadData* pThreadData);
int PASS_handler(struct ThreadData* pThreadData);
int RETR_handler(struct ThreadData* pThreadData);
int STOR_handler(struct ThreadData* pThreadData);
int QUIT_handler(struct ThreadData* pThreadData);
int SYST_handler(struct ThreadData* pThreadData);
int TYPE_handler(struct ThreadData* pThreadData);
int PORT_handler(struct ThreadData* pThreadData);
int PASV_handler(struct ThreadData* pThreadData);
int MKD_handler(struct ThreadData* pThreadData);
int CWD_handler(struct ThreadData* pThreadData);
int PWD_handler(struct ThreadData* pThreadData);
int LIST_handler(struct ThreadData* pThreadData);
int RMD_handler(struct ThreadData* pThreadData);
int RNFR_handler(struct ThreadData* pThreadData);
int RNTO_handler(struct ThreadData* pThreadData);
int WRONG_COMMAND_handler(struct ThreadData* pThreadData);

enum COMMANDS {
    USER, PASS, RETR, STOR, QUIT, SYST, TYPE, PORT, PASV, MKD, CWD,PWD, LIST, RMD,RNFR, RNTO,
    NUM_OF_COMMANDS
};

extern char* command_to_string[];

extern HANDLER handlers[];

extern char root_dir[MAX_DIRECTORY_SIZE];
extern int host_port;
extern char host_ip[];

/*
thread to dispose one connection
*/
void *connection_thread(void *vargp);

/*
return 1 if it writes successfully, 0 otherwise 
*/
int writeNullTerminatedString(int fd, const char* str);

/*
dispatch command (in pThreadData->buffer) and dispose it.
pThreadData->buffer is promised to be CRLF ended. (finally ended with \0)
that is to say, the last 3 characters are \r \n \0
@return : 1 if no error.
*/
int dispatchCommand(struct ThreadData* pThreadData);

/*
join two path: patha + pathb. the result is stored in patha.
return 1 in succeeding.
**warning : it assumes that patha points to a long enough buffer!**
*/
int join_path(char* patha, char* pathb);

/*
dispose path.
join ``cwd`` and ``command[proceding:]`` into ``buffer``,
check if the answer exceeds ``root_dir``
*/
int dispose_path(char* buffer, char* command, int proceding, char* cwd, char* root_dir);

/*
``flag`` can only be O_RDONLY or OWRONLY
if flag == O_RDONLY
    read content from ``filename`` and then send it to socket.
if flag == OWRONLY
    read content from the socket and then save it ito ``filename``.
*/
int read_or_write_file(char* filename, struct ThreadData* pThreadData, int flag);

/*
ls ``dirname`` and save the output into file.
file will be created but not deleted.
*/
int file_ls(const char* filename, const char* dirname);

/*
return public ip of this server
*/
char* get_host_ip();
#endif