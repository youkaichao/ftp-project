from lib import *
import traceback
from window import *
ui = Ui_MainWindow()


class FakeFile(object):
    @classmethod
    def write(whatever, what):
        ui.text_display.append(what)
        # ui.text_display.setText(ui.text_display.toPlainText() + what)


import sys
sys.stdout = FakeFile
sys.stderr = FakeFile
import os


def gui_logout():
    texts = [ui.ip_input, ui.port_input, ui.username_input, ui.password_input]
    radios = [ui.pasvRadioButton, ui.portRadioButton]
    for x in texts:
        x.setText("")
    for x in radios:
        x.setEnabled(False)
    ui.login_button.setText("login")
    ui.local_directory.setText("")
    ui.remote_directory.setText("")
    ui.upload_button.setEnabled(False)
    ui.download_button.setEnabled(False)
    user.is_logged_in = False
    user.close_control_socket()


def error_handler(func):
    def new_func(*args, **kwargs):
        try:
            func(*args, **kwargs)
        except Exception as e:
            traceback.print_exc()
            print(e.message)
            gui_logout()
    return new_func


@error_handler
def gui_login():
    ip = ui.ip_input.text().strip()
    port = int(ui.port_input.text().strip())
    error = connect_command.invoke(ip, port)
    if error:
        return
    error = user_command.invoke(ui.username_input.text().strip())
    if error:
        return
    error = pass_command.invoke(ui.password_input.text().strip())
    if error:
        return
    user.control_socket.sendall('TYPE I' + '\r\n')
    user.read_all_control()
    ui.login_button.setText("logout")
    radios = [ui.pasvRadioButton, ui.portRadioButton]
    for x in radios:
        x.setEnabled(True)
    ui.upload_button.setEnabled(True)
    ui.download_button.setEnabled(True)
    ui.local_directory.setText(os.getcwd())
    # get remote directory
    name_to_command['PWD'].invoke('pwd')
    response = ui.text_display.toPlainText().splitlines()[-2]
    ui.remote_directory.setText(response.split('\"')[1])

def login_dispatcher():
    if ui.login_button.text() == "login":
        gui_login()
    else:
        name_to_command['QUIT'].invoke('quit')
        gui_logout()