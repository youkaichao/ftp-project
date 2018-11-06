from gui_lib import *

app = QtWidgets.QApplication(sys.argv)
MainWindow = QtWidgets.QMainWindow()
ui.setupUi(MainWindow)

ui.login_button.clicked.connect(login_dispatcher)
gui_logout()

MainWindow.show()
sys.exit(app.exec_())