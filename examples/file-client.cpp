#include <unistd.h>

#include <string>
#include <fstream>
#include <functional>
#include <memory>
#include <utility>

#include <easynet/EventLoop.h>
#include <easynet/TcpServer.h>
#include <easynet/TcpClient.h>
#include <easynet/SignalMgr.h>
#include <easynet/TcpConnection.h>
#include <easynet/utils/log.h>
#include <easynet/utils/TimeUtil.h>

using std::ifstream;
using std::ios;

using namespace std;
using namespace easynet;

void saveFile(char *buf, int len)
{
	std::string fileName = "download-file-" + TimeUtil::getCurrentSystemTime();

	std::ofstream out;  
	out.open(fileName.c_str(), ios::out|ios::binary);
	if (out.is_open())
	{
		out.write(buf, len);
		out.close();
		LOG_INFO("file save to %s", fileName.c_str());
	}
	else
	{
		LOG_INFO("open file to write error!");
	}
}


int main(int argc, char* argv[])
{
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
	LOG_INFO("client starting, to test echo server.................................");

	std::string ip = "127.0.0.1";
	unsigned short port = 12258;
	std::string msg("hello easynet!");

	EventLoop loop;
    TcpClient tcpClient(&loop);

	tcpClient.setConnectedHandler([&](TcpConnection &tcpConn){
		LOG_INFO("--- connected to file download server %s, begin to download file...", 
			tcpConn.getPeerAddr().toString().c_str());
	});

	tcpClient.setReadHandler([&](TcpConnection &tcpConn){
		Buffer &buffer = tcpConn.getInputBuffer();
		LOG_INFO("--- received %lld bytes from file download server", buffer.size());	
	});

	tcpClient.setPeerShutdownHandler([&](TcpConnection &tcpConn){
		Buffer &buffer = tcpConn.getInputBuffer();
		LOG_INFO("--- file download server closed this connection, totally downloaded %lld bytes", buffer.size());
		saveFile(buffer.data(), buffer.size());
		tcpConn.close();
		loop.quit();
	});

	tcpClient.setDisconnectedHandler([&](TcpConnection &tcpConn){
		LOG_INFO("--- connection to file download server %s:%d is disconnected, may be error occurred, error %d %s", 
			ip.c_str(), port, tcpConn.getErrNo(), tcpConn.getErrMsg().c_str());
		loop.quit();
	});

	tcpClient.setConnectingErrorHandler([&](int errNo, const std::string &errMsg){
		LOG_INFO("--- connect to file download server %s:%d failed, error:%d %s", 
			ip.c_str(), port, errNo, errMsg.c_str());
		loop.quit();
	});

	tcpClient.setConnectingTimeoutHandler([&]{
		LOG_INFO("--- connect to file download server %s:%d timed out", ip.c_str(), port);
		loop.quit();
	});

    tcpClient.connect(ip, port, 10);
    loop.loop();

	LOG_INFO("client exiting.................................");
    return 0;
}
