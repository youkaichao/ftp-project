import re
import os
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


class User(object):
    PASSIVE = 0
    PORT_MODE = 1

    def __init__(self):
        self.local_cwd = os.getcwd()
        self.remote_cwd = ''
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
        def rec_all_control(sock):
            lines = []
            remains = ''
            while True:
                data = sock.recv(BUFFER_SIZE)
                remains += data
                # server response ends with CRLF
                if remains.endswith('\r\n'):
                    # the last element returned by split is empty string
                    lines += remains.split('\r\n')[:-1]
                    remains = ''
                    # control data ends with "xxx <space> whatever"
                    if lines[-1][3] == ' ':
                        break
            lines = [x + '\r\n' for x in lines]
            return ''.join(lines)

        data = rec_all_control(self.control_socket)
        print(data)
        return data

    def read_all_data(self):
        if self.mode == User.PORT_MODE:
            conn, addres = self.data_socket.accept()
            self.data_socket = conn

        def rec_all_data(sock):
            allData = ''
            while True:
                data = sock.recv(BUFFER_SIZE)
                allData += data
                if not data:
                    return allData

        data = rec_all_data(self.data_socket)
        self.close_data_socket()
        return data

    def send_all_data(self, data):
        if user.mode == User.PORT_MODE:
            self.data_socket, address = self.data_socket.accept()
        indexes = list(range(len(data)))
        if len(data):
            for each in indexes[::BUFFER_SIZE]:
                each = data[each:min(each + BUFFER_SIZE, len(data))]
                self.data_socket.sendall(each)
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
            return self.handler(*args)
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
    return data, ''


connect_command = Command('connect', '', connect_handler)


def user_handler(username):
    user.username = username
    user.control_socket.sendall('USER ' + username + '\r\n')
    data = user.read_all_control()
    if data[:3] != '331':
        raise Exception('username not OK')
    return data, ''


user_command = Command('user', '', user_handler)


def password_handler(password):
    user.password = password
    user.control_socket.sendall('PASS ' + password + '\r\n')
    data = user.read_all_control()
    if data[:3] == '230':
        user.is_logged_in = True
        output = name_to_command['PWD'].invoke('pwd')
        control = output[0]
        user.remote_cwd = control.split('\"')[1]
    else:
        raise Exception('wrong password!')
    return data, ''


pass_command = Command('pass', '', password_handler)


def retr_handler(command):
    control_out = ''
    user.connect_data_socket()
    user.control_socket.sendall(command + '\r\n')
    data = user.read_all_control()
    control_out += data
    if not data.startswith('150') and not data.startswith('125'):
        raise Exception('wrong return code!')
    data = user.read_all_data()
    filename = command[command.index(' ') + 1:].strip()
    with open(os.path.join(user.local_cwd, filename), 'wb') as f:
        f.write(data)
    control_out += user.read_all_control()
    return control_out, ''

def list_handler(command):
    control_out = ''
    data_out = ''
    user.connect_data_socket()
    user.control_socket.sendall(command + '\r\n')
    data = user.read_all_control()
    control_out += data
    if not data.startswith('150') and not data.startswith('125'):
        raise Exception('wrong return code!')
    data_out = user.read_all_data()
    control_out += user.read_all_control()
    print(data_out)
    return control_out, data_out


def stor_handler(command):
    control_out = ''
    filename = command[command.index(' ') + 1:].strip()
    abs_path = os.path.join(user.local_cwd, filename)
    if not (os.path.exists(abs_path) and os.path.isfile(abs_path)):
        print("'%s' not exists!" % (filename))
        return
    user.connect_data_socket()
    user.control_socket.sendall(command + '\r\n')
    data = user.read_all_control()
    control_out += data
    if not data.startswith('150') and not data.startswith('125'):
        raise Exception('wrong return code!')
    with open(abs_path, 'rb') as f:
        data = f.read()
    user.send_all_data(data)
    control_out += user.read_all_control()
    return control_out, ''


def default_handler(command):
    # just send the command and then print the response from the server
    user.control_socket.sendall(command + '\r\n')
    data = user.read_all_control()
    return data, ''

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
    return data, ''

def cwd_handler(command):
    user.control_socket.sendall(command + '\r\n')
    directory = command[command.index(' ') + 1:].strip()
    data = user.read_all_control()
    if data.startswith('200'):
        user.remote_cwd = os.path.join(user.remote_cwd, directory)
    return data, ''

def exit_handler(command):
    exit(0)

commands = [
    Command('QUIT', 'quit the current session. \r\n usage: QUIT ', quit_handler),
    Command('SYST', 'get the system information about the server \r\n usage: SYST ', default_handler),
    Command('MKD', 'make directory. \r\n usage: MKD  <SP> <pathname>', default_handler),
    Command('CWD', 'change working directory. \r\n usage: CWD  <SP> <pathname>', cwd_handler),
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
    Command('EXIT', 'exit the client program \r\n  usage: EXIT', exit_handler),
]

name_to_command = {
    x.name : x for x in commands
}