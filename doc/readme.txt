断点续传，断点继续下载是rest length; retr filename; 两条命令
断点继续上传是appe filename; 其实在服务器上就是append模式即可。过程是client发送一个命令查看服务器上同名文件的大小，然后自己发appe 命令，从某一位置开始送，client只要append到文件后面即可。