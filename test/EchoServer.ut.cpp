#include <unistd.h>

#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include <memory>
#include <utility>
#include <functional>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

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


void startEchoServer(unsigned short port)
{
    TcpServer tcpServer(port);
	tcpServer.setNewTcpConnectionHandler([&](TcpConnection &tcpConnection){
		LOG_INFO("---EchoServer: accepted new connection from %s",  tcpConnection.getPeerAddr().toString().c_str());
	});

	tcpServer.setPeerShutdownHandler([&](TcpConnection &tcpConnection){
		LOG_INFO("---EchoServer: connection from %s shutdown",  tcpConnection.getPeerAddr().toString().c_str());
	});

	tcpServer.setReadHandler([&](TcpConnection &tcpConnection){
		Buffer &buffer = tcpConnection.getInputBuffer();
		LOG_INFO("---EchoServer: received data from client:%s",  buffer.data());
		tcpConnection.send(buffer.data(), buffer.size());
		buffer.clear();
	});

	tcpServer.start();

	EventLoop loop;
	loop.loop();
}

TEST(EchoServer, testEchoServer)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
	LOG_INFO("-----------------------------------------------------");
	LOG_INFO("EchoServer-testEchoServer");
	LOG_INFO("-----------------------------------------------------");

    std::string ip = "127.0.0.1";
    unsigned short port = 12251;

	pid_t pid = ::fork();
	if (pid < 0) {
		LOG_ERROR("fork error");
	} else if (pid == 0) { // child

		LOG_INFO("child starting-----");
		Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
		startEchoServer(port);
		LOG_INFO("child exiting-----");

		exit(0);
	} else {    // parent
		LOG_INFO("parent starting, child pid = %d", pid);

	    std::this_thread::sleep_for(std::chrono::milliseconds(100));

        vector<string> stringsVec;
		stringsVec.push_back(string("hello world!"));
		stringsVec.push_back(string("this is easynet."));
		stringsVec.push_back(string("thank you for using."));

		bool testok = false;
		size_t sendIndex = 0;

	    EventLoop loop;
	    TcpClient tcpClient(&loop);

	    tcpClient.setConnectedHandler([&](TcpConnection &tcpConnection){
	    	LOG_INFO("connected to %s", tcpConnection.getPeerAddr().toString().c_str());
		    tcpConnection.send(stringsVec[sendIndex]);
	    });

	    tcpClient.setConnectingErrorHandler([&](int errNo, const std::string &errMsg){
	    	LOG_INFO("connect to %s failed, error:%d %s", tcpClient.getDstAddr().toString().c_str(), errNo, errMsg.c_str());
			loop.quit();
	    });

		tcpClient.setConnectingTimeoutHandler([&]{
			LOG_INFO("connect to echo server timedout!");
			loop.quit();
		});

		tcpClient.setReadHandler([&](TcpConnection &tcpConnection){
			Buffer &buffer = tcpConnection.getInputBuffer();
			LOG_INFO("received from echo server:%s", buffer.data());
			if (string(buffer.data(), buffer.size()) == stringsVec[sendIndex])
			{				
				buffer.deleteBegin(buffer.size());			
			}
			else
			{
				LOG_INFO("string received from echo server is not what i send!");
				ASSERT_TRUE(false);
				testok = false;
				loop.quit();
				return;
			}
			
			sendIndex++;
			if (sendIndex == stringsVec.size())
			{
				testok = true;
				tcpConnection.close();
				loop.quit();
				return;		
			}

			tcpConnection.send(stringsVec[sendIndex]);
		});

        int timeoutSecs = 10;
	    tcpClient.connect(ip, port, timeoutSecs);
	    loop.runAfter(500, [&](){
	    	loop.quit();
	    });
	    loop.loop();
	    ASSERT_TRUE(testok);

	    std::this_thread::sleep_for(std::chrono::milliseconds(200));
        ::kill(pid, SIGKILL);
	    int status = 0;
	    pid_t ret = ::waitpid(pid, &status, 0);
	    LOG_INFO("child %d exited, exit code = %d", ret, status);
	    EXPECT_EQ(SIGKILL, status);
	}

	LOG_INFO("exiting");
}
