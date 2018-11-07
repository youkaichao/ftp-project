from lib import *
import traceback
from window import *
from functools import partial
import sys
ui = Ui_MainWindow()
app = QtWidgets.QApplication(sys.argv)
MainWindow = QtWidgets.QMainWindow()
ui.setupUi(MainWindow)


# redirect print to the text_display widget
class FakeFile(object):
    @classmethod
    def write(whatever, what):
        ui.text_display.append(what)


sys.stdout = FakeFile
sys.stderr = FakeFile
import os


class DirectoryView:
    local_directory_view = None
    remote_directory_view = None

    def __init__(self, listView, directory_label, is_local=True):
        # store the reference
        self.listView = listView
        self.directory_label = directory_label
        self.is_local = is_local
        self.buttons = []

    def _is_dir(self, path_name):
        if self.is_local:
            return os.path.isdir(path_name)
        control, data = name_to_command['CWD'].invoke('cwd ' + path_name)
        return control.startswith('250')

    def _add_button(self, text):

        def on_click(btn):
            if btn.text() == '..':
                # change to upper directory
                self.directory_label.setText(os.path.dirname(self.directory_label.text()))
                self.update()
                return
            new_path = os.path.join(self.directory_label.text(), btn.text())
            if self._is_dir(new_path):
                self.directory_label.setText(new_path)
                self.update()
                return
            # dispose file click
            user.local_cwd = DirectoryView.local_directory_view.directory_label.text()
            user.remote_cwd = DirectoryView.remote_directory_view.directory_label.text()
            if self.is_local:
                name_to_command['STOR'].invoke('stor ' + btn.text())
                DirectoryView.remote_directory_view.update()
            else:
                name_to_command['RETR'].invoke('retr ' + btn.text())
                DirectoryView.local_directory_view.update()

        button = QtWidgets.QPushButton()
        button.setText(text)
        itemN = QtWidgets.QListWidgetItem()
        self.listView.addItem(itemN)
        self.listView.setItemWidget(itemN, button)
        self.buttons.append(button)
        button.clicked.connect(partial(on_click, button))

    def _list_directory(self):
        cwd = self.directory_label.text()
        if self.is_local:
            return os.listdir(cwd)
        else:
            control, data = name_to_command['LIST'].invoke('list ' + cwd)
            lines = [line.strip() for line in data.splitlines()]
            i = [i for (i, line) in enumerate(lines) if line.endswith('..')][0]
            return [line.split()[-1] for line in lines[i + 1 :]]

    def clear(self):
        for btn in self.buttons:
            self.listView.takeItem(0)
        for btn in self.buttons:
            btn.deleteLater()
        self.buttons = []

    def update(self):
        # list the directory and create each button
        # clear previous buttons
        self.clear()
        self._add_button("..")
        files = self._list_directory()
        for file in files:
            self._add_button(file)


DirectoryView.local_directory_view = DirectoryView(ui.local_files, ui.local_directory, is_local=True)
DirectoryView.remote_directory_view = DirectoryView(ui.remote_files, ui.remote_directory, is_local=False)


def gui_logout():
    texts = [ui.ip_input, ui.port_input, ui.username_input, ui.password_input]
    radios = [ui.pasvRadioButton, ui.portRadioButton]
    for x in texts:
        x.setText("")
    for x in radios:
        x.setChecked(False)
        x.setEnabled(False)
    ui.new_local_folder.setEnabled(False)
    ui.new_remote_folder.setEnabled(False)

    ui.login_button.setText("login")
    ui.local_directory.setText("")
    ui.remote_directory.setText("")
    DirectoryView.local_directory_view.clear()
    DirectoryView.remote_directory_view.clear()
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
    output = connect_command.invoke(ip, port)
    if isinstance(output, Exception):
        return
    output = user_command.invoke(ui.username_input.text().strip())
    if isinstance(output, Exception):
        return
    output = pass_command.invoke(ui.password_input.text().strip())
    if isinstance(output, Exception):
        return
    user.control_socket.sendall('TYPE I' + '\r\n')
    user.read_all_control()
    ui.login_button.setText("logout")
    radios = [ui.pasvRadioButton, ui.portRadioButton]
    for x in radios:
        x.setEnabled(True)
    ui.new_local_folder.setEnabled(True)
    ui.new_remote_folder.setEnabled(True)
    ui.pasvRadioButton.setChecked(True)
    ui.local_directory.setText(os.getcwd())
    # get remote directory
    output = name_to_command['PWD'].invoke('pwd')
    control = output[0]
    ui.remote_directory.setText(control.split('\"')[1])
    DirectoryView.local_directory_view.update()
    DirectoryView.remote_directory_view.update()


def login_dispatcher():
    if ui.login_button.text() == "login":
        gui_login()
    else:
        name_to_command['QUIT'].invoke('quit')
        gui_logout()


def create_local_folder():
    new_local_folder_name = ui.new_local_folder_name.text().strip()
    if not new_local_folder_name:
        QtWidgets.QMessageBox.warning(None, 'warning', 'empty directory name')
        return
    cwd = DirectoryView.local_directory_view.directory_label.text()
    path_name = os.path.join(cwd, new_local_folder_name)
    os.mkdir(path_name)
    DirectoryView.local_directory_view.update()


def create_remote_folder():
    new_remote_folder_name = ui.new_remote_folder_name.text().strip()
    if not new_remote_folder_name:
        QtWidgets.QMessageBox.warning(None, 'warning', 'empty directory name')
        return
    cwd = DirectoryView.remote_directory_view.directory_label.text()
    path_name = os.path.join(cwd, new_remote_folder_name)
    output = name_to_command['MKD'].invoke('mkd ' + path_name)
    DirectoryView.remote_directory_view.update()