from gui_lib import *

app = QtWidgets.QApplication(sys.argv)
MainWindow = QtWidgets.QMainWindow()
ui.setupUi(MainWindow)

ui.login_button.clicked.connect(login_dispatcher)
gui_logout()

ui.ip_input.setText("144.208.69.31")
ui.port_input.setText("21")
ui.username_input.setText("dlpuser@dlptest.com")
ui.password_input.setText("e73jzTRTNqCN9PYAAjjn")

MainWindow.show()
sys.exit(app.exec_())