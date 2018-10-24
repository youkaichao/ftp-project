import socket
import time
sock = socket.socket()
sock.connect(("127.0.0.1", 2000))
print(sock.recv(1024))