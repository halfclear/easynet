#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <memory>
#include <utility>
#include <functional>
#include <random>

#include "TcpServer.h"
#include "TcpClient.h"
#include "TcpConnection.h"
#include "Acceptor.h"
#include "Connector.h"
#include "Socket.h"
#include "EventLoop.h"
#include "utils/TimeUtil.h"
#include "utils/log.h"

#include <test_harness.h>

using namespace std;
using namespace easynet;

void startFileEchoServer(unsigned short port)
{
    TcpServer tcpServer(port);
	tcpServer.setNewTcpConnectionHandler([&](TcpConnection &tcpConnection){
		LOG_INFO("---FileEchoServer:accepted new connection from %s", 
			tcpConnection.getPeerAddr().toString().c_str());
	});

	tcpServer.setReadHandler([&](TcpConnection &tcpConnection){
		Buffer &buffer = tcpConnection.getInputBuffer();
		LOG_INFO("---FileEchoServer: received %llu bytes from %s", 
			buffer.size(), tcpConnection.getPeerAddr().toString().c_str());
	});

	tcpServer.setPeerShutdownHandler([&](TcpConnection &tcpConnection){
		Buffer &buffer = tcpConnection.getInputBuffer();
		LOG_INFO("---FileEchoServer:connection from %s shutdown, totally received %llu bytes, send file back to client", 
			tcpConnection.getPeerAddr().toString().c_str(), buffer.size());

		tcpConnection.send(buffer.data(), buffer.size());
	});

	tcpServer.setWriteCompleteHandler([&](TcpConnection &tcpConnection){
		LOG_INFO("---FileEchoServer: send file to %s complete", tcpConnection.getPeerAddr().toString().c_str());
		tcpConnection.close();
	});

	tcpServer.setDisconnectedHandler([](TcpConnection &tcpConnection){
		LOG_INFO("---FileEchoServer: connection from %s is disconnected", tcpConnection.getPeerAddr().toString().c_str());
	});

	tcpServer.start();

	EventLoop loop;
	loop.loop();
}


std::unique_ptr<char[]> genContent(size_t size)
{
	std::unique_ptr<char[]> buf(new char[size]);

	std::memset(buf.get(), 0, size);

	uniform_int_distribution<unsigned> u(0, 51);
	default_random_engine e(TimeUtil::now());
	int a = 'a';
	int z = 'z';
	int A = 'A';
	int Z = 'Z';
	LOG_INFO("a=%d, z=%d, A=%d, Z=%d", a, z, A, Z);
	char *p = buf.get(); 
	for (size_t i = 0; i < size; i++)
	{
		unsigned int index = u(e);
		char c = 0;
		if (index < 26) {
			c = 'a' + index;
		} else {
			index -= 26;
			c = 'A' + index;
		}
		p[i] = c;
	}

	return std::move(buf);
}

bool checkContent(const char *src, const char *dst, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		if (src[i] != dst[i]) 
		{
			return false;
		}
	}
	return true;
}

TEST(FileEchoServer, testFileEchoServer)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
	LOG_INFO("-----------------------------------------------------");
    LOG_INFO("FileEchoServer-testFileEchoServer");
    LOG_INFO("-----------------------------------------------------");

    std::string ip = "127.0.0.1";
    unsigned short port = 12251;

	pid_t pid = ::fork();
	if (pid < 0) {
		LOG_ERROR("fork error");
	} else if (pid == 0) { // child

		LOG_INFO("child starting-----");
		Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
		startFileEchoServer(port);
		LOG_INFO("child exiting-----");

		exit(0);
	} else {    // parent
		LOG_INFO("parent starting, child pid = %d", pid);

	    std::this_thread::sleep_for(std::chrono::milliseconds(500));

        size_t len = 10 * 1024 * 1024;
        std::unique_ptr<char[]> buf = genContent(len);

		bool testok = false;

	    EventLoop loop;
	    TcpClient tcpClient(&loop);

	    tcpClient.setConnectedHandler([&](TcpConnection &tcpConnection){
	    	LOG_INFO("--connect to FileEchoServer %s successfully! begin to send file", tcpConnection.getPeerAddr().toString().c_str());
		    tcpConnection.send(buf.get(), len);
	    });

	    tcpClient.setConnectingErrorHandler([&](int errNo, const std::string &errMsg){
	    	LOG_INFO("--connect to FileEchoServer %s failed, error:%d %s", 
	    		tcpClient.getDstAddr().toString().c_str(), errNo, errMsg.c_str());
			loop.quit();
	    });

		tcpClient.setConnectingTimeoutHandler([&]{
			LOG_INFO("--connect to FileEchoServer timedout!");
			loop.quit();
		});

		tcpClient.setReadHandler([&](TcpConnection &tcpConnection){
			Buffer &buffer = tcpConnection.getInputBuffer();
		    LOG_INFO("--received %llu bytes from FileEchoServer", buffer.size()); 
		});

		tcpClient.setPeerShutdownHandler([&](TcpConnection &tcpConnection){
			Buffer &buffer = tcpConnection.getInputBuffer();
			LOG_INFO("--FileEchoServer shutdown, totally received %llu bytes from FileEchoServer", buffer.size());
			if (buffer.size() == len && checkContent(buf.get(), buffer.data(), len)) {
				testok = true;
			}

			tcpConnection.close();
			loop.quit();
		});

		tcpClient.setWriteCompleteHandler([&](TcpConnection &tcpConnection){
			LOG_INFO("--send file to FileEchoServer complete");
			tcpConnection.shutdownWrite();
		});

		tcpClient.setDisconnectedHandler([&](TcpConnection &tcpConnection){
			LOG_INFO("--connection to FileEchoServer is disconnected");
			loop.quit();
		});

        int timeoutSecs = 10;
	    tcpClient.connect(ip, port, timeoutSecs);
	    loop.runAfter(1000, [&](){
	    	loop.quit(); // to ensure test will be finished
	    });
	    loop.loop();
	    ASSERT_TRUE(testok);

	    std::this_thread::sleep_for(std::chrono::milliseconds(500));
        ::kill(pid, SIGKILL);
	    int status = 0;
	    pid_t ret = ::waitpid(pid, &status, 0);
	    LOG_INFO("child %d exited, exit code = %d", ret, status);
	    EXPECT_EQ(SIGKILL, status);
	}

	LOG_INFO("exiting");
}
