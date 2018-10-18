from lib import *
import traceback


def execute_one_session():
    socket_connected = False
    while not socket_connected:
        ip = raw_input('please input server ip:').strip() or '144.208.69.31'
        port = raw_input('please input server port:').strip() or '21'
        port = int(port)
        error = connect_command.invoke(ip, port)
        socket_connected = not error

    username_accepted = False
    while not username_accepted:
        username = raw_input('please input username:').strip() or 'dlpuser@dlptest.com'
        error = user_command.invoke(username)
        username_accepted = not error

    password_accepted = False
    while not password_accepted:
        password = raw_input('please input password:').strip() or 'e73jzTRTNqCN9PYAAjjn'
        error = pass_command.invoke(password)
        password_accepted = not error

    user.control_socket.sendall('TYPE I' + '\r\n')
    user.read_all_control()

    print('Type "help" to see all commands available. Type "help xxx" to see the usage of command "xxx". '
          'Commands are case insensitive')
    while user.is_logged_in:
        command = raw_input('ftp>').strip()
        if not command:
            continue
        if command.upper() == 'HELP':
            print(' '.join(name_to_command.keys()))
        elif command.upper().startswith('HELP'):
            command = command[4:].strip().upper()
            if command in name_to_command:
                print(name_to_command[command].help_string)
            else:
                print('command "%s" not recognized!' % (command))
        else:
            line = command
            command = command.upper().split()[0]
            if command in name_to_command:
                name_to_command[command].invoke(line)
            else:
                print('command "%s" not recognized!' % (command))


while True:
    try:
        execute_one_session()
    except Exception as e:
        user.is_logged_in = False
        traceback.print_exc()
        user.close_control_socket()