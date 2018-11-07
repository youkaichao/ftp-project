from gui_lib import *



ui.login_button.clicked.connect(login_dispatcher)
ui.portRadioButton.clicked.connect((lambda : name_to_command['SETPORT'].invoke('SETPORT')))
ui.pasvRadioButton.clicked.connect((lambda : name_to_command['SETPASV'].invoke('SETPASV')))
gui_logout()

ui.ip_input.setText("144.208.69.31")
ui.port_input.setText("21")
ui.username_input.setText("dlpuser@dlptest.com")
ui.password_input.setText("e73jzTRTNqCN9PYAAjjn")

MainWindow.show()
sys.exit(app.exec_())