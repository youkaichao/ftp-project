import re
import socket
import traceback
BUFFER_SIZE = 1024
import time

HOST_IP = None


def get_host_ip():
    global HOST_IP
    if not HOST_IP:
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.connect(('8.8.8.8', 80))
            ip = s.getsockname()[0]
            HOST_IP = ip
            return HOST_IP
        finally:
            s.close()
    else:
        return HOST_IP


def rec_all(sock):
    allData = ''
    while True:
        data = sock.recv(BUFFER_SIZE)
        allData += data
        if len(data) < BUFFER_SIZE:
            return allData


class User(object):
    PASSIVE = 0
    PORT_MODE = 1

    def __init__(self):
        self.username = ''
        self.password = ''
        self.mode = User.PASSIVE
        self.is_logged_in = False
        self.control_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.data_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def close_control_socket(self):
        self.control_socket.close()
        self.control_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def close_data_socket(self):
        self.data_socket.close()
        self.data_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def read_all_control(self):
        # control connection read, the reply from the server should always be printed out to the user
        data = rec_all(self.control_socket)
        print(data)
        return data

    def read_all_data(self):
        if self.mode == User.PORT_MODE:
            conn, addres = self.data_socket.accept()
            self.data_socket = conn
        data = rec_all(self.data_socket)
        self.close_data_socket()
        return data

    def send_all_data(self, data):
        self.data_socket.sendall(data)
        self.close_data_socket()

    def connect_data_socket(self):
        if user.mode == User.PASSIVE:
            user.control_socket.sendall("PASV" + '\r\n')
            data = user.read_all_control()
            matched = re.compile(r'\(.*\)').search(data).group()
            matched = matched[1:-1]
            numbers = matched.split(',')
            ip = '.'.join(numbers[:4])
            port = int(numbers[4]) * 256 + int(numbers[5])
            user.data_socket.connect((ip, port))
        elif user.mode == User.PORT_MODE:
            # find a port and then listen
            self.data_socket.bind(('', 0))
            ip, port = self.data_socket.getsockname()
            self.data_socket.listen(1)
            p1 = port // 256
            p2 = port % 256
            params = get_host_ip().replace('.', ',') + ',%s,%s'%(p1, p2)
            user.control_socket.sendall("PORT " + params + '\r\n')
            data = user.read_all_control()


user = User() # single instance


class Command(object):

    def __init__(self, name, help_string, handler):
        self.name = name
        self.help_string = help_string
        self.handler = handler

    def invoke(self, *args):
        try:
            self.handler(*args)
            return None
        except Exception as e:
            traceback.print_exc()
            print(e.message)
            user.close_data_socket()
            return e


def connect_handler(ip, port):
    user.control_socket.connect((ip, port))
    data = user.read_all_control()
    if data[:3] != '220':
        raise Exception('connection failed!')
    return


connect_command = Command('connect', '', connect_handler)


def user_handler(username):
    user.username = username
    user.control_socket.sendall('USER ' + username + '\r\n')
    data = user.read_all_control()
    if data[:3] != '331':
        raise Exception('username not OK')
    return


user_command = Command('user', '', user_handler)


def password_handler(password):
    user.password = password
    user.control_socket.sendall('PASS ' + password + '\r\n')
    data = user.read_all_control()
    if data[:3] == '230':
        user.is_logged_in = True
    else:
        raise Exception('wrong password!')
    return


pass_command = Command('pass', '', password_handler)


def retr_handler(command):
    user.connect_data_socket()
    user.control_socket.sendall(command + '\r\n')
    data = user.read_all_control()
    if not data.startswith('150'):
        raise Exception('wrong return code!')
    data = user.read_all_data()
    filename = command[command.index(' ') + 1:].strip()
    with open(filename, 'wb') as f:
        f.write(data)
    user.read_all_control()


def list_handler(command):
    user.connect_data_socket()
    user.control_socket.sendall(command + '\r\n')
    data = user.read_all_control()
    if not data.startswith('150'):
        raise Exception('wrong return code!')
    data = user.read_all_data()
    user.read_all_control()
    print(data)


def stor_handler(command):
    filename = command[command.index(' ') + 1:].strip()
    import os
    if not (os.path.exists(filename) and os.path.isfile(filename)):
        print("'%s' not exists!" % (filename))
        return
    user.connect_data_socket()
    user.control_socket.sendall(command + '\r\n')
    data = user.read_all_control()
    if not data.startswith('150'):
        raise Exception('wrong return code!')
    with open(filename, 'rb') as f:
        data = f.read()
    user.send_all_data(data)
    user.read_all_control()


def default_handler(command):
    # just send the command and then print the response from the server
    user.control_socket.sendall(command + '\r\n')
    data = user.read_all_control()


def setPortHandler(command):
    user.mode = User.PORT_MODE
    print('in port mode now')


def setPASVHandler(command):
    user.mode = User.PASSIVE
    print('in passive mode now')


def showMode(command):
    if user.mode == User.PASSIVE:
        print('in passive mode now')
    else:
        print('in port mode now')


def quit_handler(command):
    user.control_socket.sendall(command + '\r\n')
    data = user.read_all_control()
    user.is_logged_in = False
    user.close_control_socket()
    return


commands = [
    Command('QUIT', 'quit the current session. \r\n usage: QUIT ', quit_handler),
    Command('SYST', 'get the system information about the server \r\n usage: SYST ', default_handler),
    Command('MKD', 'make directory. \r\n usage: MKD  <SP> <pathname>', default_handler),
    Command('CWD', 'change working directory. \r\n usage: CWD  <SP> <pathname>', default_handler),
    Command('PWD', 'print current working directory. \r\n usage: PWD', default_handler),
    Command('LIST', 'list path. \r\n  usage: LIST [<SP> <pathname>]', list_handler),
    Command('RMD', 'remove directory. \r\n  usage: RMD  <SP> <pathname>', default_handler),
    Command('RNFR', 'rename from. (combined with RNTO to rename remote file) \r\n  usage: RNFR <SP> <pathname>', default_handler),
    Command('RNTO', 'rename to. (combined with RNFR to rename remote file) \r\n  usage: RNTO <SP> <pathname>', default_handler),
    Command('SETPORT', 'set the mode to port mode \r\n  usage: SETPORT', setPortHandler),
    Command('SETPASV', 'set the mode to passive mode \r\n  usage: SETPASV', setPASVHandler),
    Command('SHOWMODE', 'show the current mode \r\n  usage: SHOWMODE', showMode),
    Command('RETR', 'retrieve file from remote server \r\n  usage: RETR <SP> <pathname>', retr_handler),
    Command('STOR', 'store file to remote server \r\n  usage: STOR <SP> <pathname>', stor_handler),
]

name_to_command = {
    x.name : x for x in commands
}