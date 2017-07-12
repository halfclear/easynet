进行性能测试时，需要在/etc/sysctl.conf文件下加入如下语句：
net.ipv4.ip_local_port_range=1024 65535
net.ipv4.tcp_timestamps=1
net.ipv4.tcp_tw_recycle=1
net.ipv4.tcp_tw_reuse=1

然后保存，命令行下输入
sysctl -p
使参数生效。

首先执行./server运行服务程序，然后执行./client <ip> <port> <并发数量> <测试时间>进行测试，比如：
./client localhost 12250 100 10