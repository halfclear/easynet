# easynet
A multithreaded nonblocking C++11 network library in Linux

用于Linux的多线程非阻塞C++11网络库。

1.概述

easynet是一款用于Linux系统的多线程非阻塞网络库，采用C++11开发。

easynet对原始套接字进行了封装，还封装了基于epoll的事件驱动机制，在此基础上封装了多线程tcp服务器TcpServer、tcp客户端TcpClient、udp服务器UdpServer,另外easynet还提供了定时器、时间轮、信号处理器，可以方便地在easynet的基础上开发各种服务端程序、客户端程序。对于某些特别简单的网络应用，也可以直接使用easynet的Socket采用阻塞模式开发。

easynet有如下特点：
** TcpServer的多个工作线程为对等关系，每个线程都可以监听连接；
** TcpServer可以同时监听地址多个地址；
** 支持空闲连接检测；
** 支持信号处理，可以为每个事件循环单独添加信号处理器；
** 支持时间轮；
** 支持udp套接字。

对于监听套接字，easynet采用水平触发模式；对于连接套接字，采用边沿触发模式。

2.运行环境
easynet不依赖任何第三方库。需运行在Linux2.8.26以上内核版本，编译器gcc4.8.2以上版本。

3.安装
下载easynet并解压，执行make进行编译，执行make install安装到系统默认路径下。若执行make install prefix=xxx，则安装到路径xxx之下。

4.目录结构
** easynet------------源代码
** expamples----------使用示例
** benchmark----------性能测试
** test---------------单元测试

5.测试
进入unitest目录下，执行make编译，然后执行./easynet-unitest进行单元测试。

本人出于个人兴趣及学习之目的开发了easynet，有错误及不足之处欢迎批评指正。
email:fangsh1982@126.com

fangshenghua
2017.7.12
