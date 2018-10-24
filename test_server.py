import socket
import time
sock = socket.socket()
sock.bind(("127.0.0.1", 2000))
sock.listen(1)
data, address = sock.accept()
print(data.sendall("hahaha\r\n"))
data.close()
sock.close()
