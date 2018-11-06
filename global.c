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

char *command_to_string[] = {
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
    [NUM_OF_COMMANDS] = WRONG_COMMAND_handler};

char root_dir[MAX_DIRECTORY_SIZE] = "/tmp";
int host_port = 21;
char host_ip[100] = {0};