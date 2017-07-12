#include <string>
#include <functional>
#include <memory>
#include <utility>
#include <iostream>
#include <cstring>

#include <easynet/EventLoop.h>
#include <easynet/TcpClient.h>
#include <easynet/TcpConnection.h>
#include <easynet/utils/log.h>

using namespace std;
using namespace easynet;

int main(int argc, char* argv[])
{
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
	LOG_INFO("client starting, to test echo server.................................");

	std::string ip = "127.0.0.1";
	unsigned short port = 12256;
	std::string msg("hello easynet!");

	EventLoop loop;
    TcpClient tcpClient(&loop);

	tcpClient.setConnectedHandler([&](TcpConnection &tcpConn){
		LOG_INFO("--- connected to echo server %s:%d, send:%s", 
			ip.c_str(), port, msg.c_str());
		tcpConn.send(msg);
	});

	tcpClient.setReadHandler([&](TcpConnection &tcpConn){
		Buffer &buffer = tcpConn.getInputBuffer();
		LOG_INFO("--- received message from echo server %s:%d:%s", ip.c_str(), port, buffer.data());

		buffer.deleteBegin(buffer.size());
		tcpConn.close();
		loop.quit();	
	});

	tcpClient.setPeerShutdownHandler([&](TcpConnection &tcpConn){
		LOG_INFO("--- echo server closed this connection just now.");
		tcpConn.close();
		loop.quit();
	});

	tcpClient.setDisconnectedHandler([&](TcpConnection &tcpConn){
		LOG_INFO("--- connection to echo server %s:%d is disconnected, may be error occurred, error %d %s", 
			ip.c_str(), port, tcpConn.getErrNo(), tcpConn.getErrMsg().c_str());
		loop.quit();
	});

	tcpClient.setConnectingErrorHandler([&](int errNo, const std::string &errMsg){
		LOG_INFO("--- connect to echo server %s:%d failed, error:%d %s", ip.c_str(), port, errNo, errMsg.c_str());
		loop.quit();
	});

	tcpClient.setConnectingTimeoutHandler([&]{
		LOG_INFO("--- connect to echo server %s:%d timed out", ip.c_str(), port);
		loop.quit();
	});

    tcpClient.connect(ip, port, 10);
    loop.loop();

	LOG_INFO("client exiting.................................");
    return 0;
}
