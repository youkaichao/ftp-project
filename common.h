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

#define clean_errno() (errno == 0 ? "None" : strerror(errno))
#define log_error(M, ...) fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)
#define assertf(A, M, ...) if(!(A)) {log_error(M, ##__VA_ARGS__); assert(A); }

#define BUFFER_SIZE 2048

char CONNECT_OK_MSG[] = "220 Anonymous FTP server ready.\r\n";
char UNKNOWN_COMMAND_MSG[] = "500 Syntax error, command unrecognized.\r\n";
char NOT_LOGGED_IN_MSG[] = "530 Not logged in.\r\n";
char ALREADY_LOGGED_IN_MSG[] = "530 Already Logged In.\r\n";
char USERNAMR_OK_MSG[] = "331 User name okay, need email as password.\r\n";
char PASSWORD_OK_MSG[] = "230 User logged in, proceed.\r\n";
char QUIT_MSG[] = "221 Bye bye.\r\n";
char SYST_MSG[] = "215 UNIX Type: L8\r\n";


enum UserState {
    JUST_CONNECTED, HAS_USER_NAME,
    AUTHORIZATION_LINE, // state that is less than this number can only log in 
    LOGGED_IN, RNFR_STATE};// RNFR is used for RNFR and RNTO

struct ThreadData{
	pthread_t* pthread_id;//pointer to thread id
	int* pconnfd;// pointer to connection file descriptor
	char buffer[BUFFER_SIZE]; // reading buffer
	enum UserState userState;
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

char root_dir[2000] = "/tmp";
int host_port = 21;

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

#endif