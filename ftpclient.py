port = 2000
from ftplib import FTP
ftp = FTP()
ftp.connect('127.0.0.1', port)
ftp.login()
ftp.set_pasv(False)
ftp.retrbinary('RETR a.txt', open("b.txt", 'wb').write)


port = 2000
from ftplib import FTP
ftp2 = FTP()
ftp2.connect('127.0.0.1', port)
ftp2.login()
ftp2.set_pasv(False)
ftp2.retrbinary('RETR a.txt', open("b.txt", 'wb').write)
ftp2.quit()

ftp.quit()