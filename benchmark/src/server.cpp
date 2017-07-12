#include <string>
#include <functional>

#include <easynet/EventLoop.h>
#include <easynet/TcpServer.h>
#include <easynet/SignalMgr.h>
#include <easynet/TcpConnection.h>
#include <easynet/utils/log.h>

using namespace easynet;
using std::placeholders::_1;

class EchoServer
{
public:
	EchoServer() = default;
	EchoServer(const EchoServer &rhs) = delete;
	~EchoServer() = default;

	void onNewConnection(TcpConnection &tcpConn)
	{
	//	LOG_INFO("----accpeted connection from %s", tcpConn.getPeerAddr().toString().c_str());
	}

	void onMessage(TcpConnection &tcpConn)
	{
		Buffer &buffer = tcpConn.getInputBuffer();
	//	LOG_INFO("received %lld  bytes data from %s, contents:%s", buffer.size(), tcpConn.getPeerAddr().toString().c_str(), buffer.data());

		tcpConn.send(buffer.data(), buffer.size());  // send back to client
		buffer.deleteBegin(buffer.size());
	}
	
	void onPeerShutdown(TcpConnection &tcpConn)
	{
	//	LOG_INFO("connection from %s is closed", tcpConn.getPeerAddr().toString().c_str());
		tcpConn.close();
	}

	void onDisconnected(TcpConnection &tcpConn)
	{
	//	LOG_WARN("connection from %s is disconnected, errno %d %s", 
	//		tcpConn.getPeerAddr().toString().c_str(), tcpConn.getErrNo(), tcpConn.getErrMsg().c_str());
	}	
};

int main(int argc, char* argv[])
{
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
//	Logger::getInstance().setLogFile("server-log.txt");
	LOG_INFO("server starting.................................");

	SignalMgr::enableSignalHandling();

    EchoServer echoServer;
    unsigned short port = 12250;

    int workerNum = 1;
    int tcpConnectionPoolCoreSize = 512;
    int tcpConnectionPoolMaxSize  = 1024;
    int livingTime = 10;
    int minAcceptPerCall = 512;
    int maxAcceptPerCall = 1024;

    TcpServer tcpServer(port);
    
    tcpServer.setWorkerNum(workerNum);
	tcpServer.setTcpWorkerConnectionPoolCoreSize(tcpConnectionPoolCoreSize);
	tcpServer.setTcpWorkerConnectionPoolMaxSize(tcpConnectionPoolMaxSize);
	tcpServer.setTcpWorkerConnectionPoolLivingTime(livingTime);
	tcpServer.setMinAcceptsPerCall(minAcceptPerCall);
	tcpServer.setMaxAcceptsPerCall(maxAcceptPerCall);

	tcpServer.setReadHandler(std::bind(&EchoServer::onMessage, &echoServer, _1));
	tcpServer.setPeerShutdownHandler(std::bind(&EchoServer::onPeerShutdown, &echoServer, _1));
	tcpServer.setDisconnectedHandler(std::bind(&EchoServer::onDisconnected, &echoServer, _1));
    tcpServer.start();

	EventLoop loop;
	loop.addSignalHandler(SIGPIPE, [&]{
		LOG_INFO("SIGPIPE caught");
	});

	loop.addSignalHandler(SIGINT, [&]{
		LOG_INFO("to stop server");
		loop.quit();
	});
	loop.loop();
	tcpServer.stop();

	LOG_INFO("server exiting.................................");
	return 0;
}
