#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include <memory>
#include <utility>
#include <functional>

#include "Acceptor.h"
#include "Connector.h"
#include "Socket.h"
#include "TcpClient.h"
#include "TcpServer.h"


#include "EventLoop.h"

#include "utils/TimeUtil.h"
#include "utils/log.h"

#include <test_harness.h>

using std::string;
using std::thread;
using std::cout;
using std::endl;

using namespace std;
using namespace easynet;

/*
void startServer(unsigned short port)
{
    TcpServer tcpServer(port);
	tcpServer.setNewTcpConnectionHandler([&](TcpConnection &tcpConnection){
		LOG_INFO("----Server: accepted new connection from %s",  tcpConnection.getPeerAddr().toString().c_str());
	});

	tcpServer.setPeerShutdownHandler([&](TcpConnection &tcpConnection){
		LOG_INFO("----Server: connection from %s shutdown",  tcpConnection.getPeerAddr().toString().c_str());
	});

	tcpServer.start();

	EventLoop loop;
	loop.loop();
}

TEST(TcpClient, testConnectSuccess)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
	LOG_INFO("-----------------------------------------------------");
	LOG_INFO("TcpClient-testConnectSuccess");
	LOG_INFO("-----------------------------------------------------");

    std::string ip = "127.0.0.1";
    unsigned short port = 12251;

	pid_t pid = ::fork();
	if (pid < 0) {
		LOG_ERROR("fork error");
	} else if (pid == 0) { // child

		LOG_INFO("child starting-----");
		startServer(port);
		LOG_INFO("child exiting-----");

		exit(0);
	} else {    // parent
		LOG_INFO("parent starting, child pid = %d", pid);

	    std::this_thread::sleep_for(std::chrono::milliseconds(500));

	    EventLoop loop;
	    TcpClient tcpClient(&loop);
	    bool connected = false;

	    tcpClient.bind(port+1);

	    tcpClient.setConnectedHandler([&](TcpConnection &tcpConnection){
	    	LOG_INFO("--connected to %s", tcpConnection.getPeerAddr().toString().c_str());
		    connected = true;
		    tcpClient.close();
		    loop.quit();
	    });

        int timeoutSecs = 10;
	    tcpClient.connect(ip, port, timeoutSecs);
	    loop.loop();
	    ASSERT_TRUE(connected);

	    std::this_thread::sleep_for(std::chrono::milliseconds(500));
        ::kill(pid, SIGKILL);
	    int status = 0;
	    pid_t ret = ::waitpid(pid, &status, 0);
	    LOG_INFO("child %d exited, exit code = %d", ret, status);
	    EXPECT_EQ(SIGKILL, status);
	}

	LOG_INFO("exiting");
}

TEST(TcpClient, testConnectError)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
	LOG_INFO("-----------------------------------------------------");
	LOG_INFO("TcpClient-testConnectError");
	LOG_INFO("-----------------------------------------------------");

    std::string ip = "127.0.0.1";
    unsigned short port = 12251;

    EventLoop loop;
    TcpClient tcpClient(&loop);
    bool connected = false;

    tcpClient.setConnectedHandler([&](TcpConnection &tcpConnection){
    	LOG_INFO("--connected to %s", tcpConnection.getPeerAddr().toString().c_str());
	    connected = true;
	    tcpClient.close();
	    loop.quit();
    });

    bool errOcurred = false;
    tcpClient.setConnectingErrorHandler([&](int errNo, const std::string &errMsg){
    	LOG_INFO("--connect to %s failed, error:%d %s", tcpClient.getDstAddr().toString().c_str(), errNo, errMsg.c_str());
        errOcurred = true;
        tcpClient.close();
	    loop.quit();
    });

    int timeoutSecs = 10;
    tcpClient.connect(ip, port, timeoutSecs);
    loop.loop();
    ASSERT_TRUE(errOcurred);
    ASSERT_FALSE(connected);

	LOG_INFO("exiting");
}
*/

/*
TEST(TcpClient, testConnectTimeout)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
	LOG_INFO("-----------------------------------------------------");
	LOG_INFO("TcpClient-testConnectTimeout");
	LOG_INFO("-----------------------------------------------------");

    std::string ip = "192.168.1.111";
    unsigned short port = 12251;

	pid_t pid = ::fork();
	if (pid < 0) {
		LOG_ERROR("fork error");
	} else if (pid == 0) { // child


		LOG_INFO("child starting-----");
		EventLoop loop;
		loop.loop();
		LOG_INFO("child exiting-----");

		exit(0);
	} else {    // parent
		LOG_INFO("parent starting, child pid = %d", pid);

	    std::this_thread::sleep_for(std::chrono::milliseconds(500));

	    EventLoop loop;
	    TcpClient tcpClient(&loop);
	    bool connected = false;

	    tcpClient.bind(port+1);

	    tcpClient.setConnectedHandler([&](TcpConnection &tcpConnection){
	    	LOG_INFO("--connected to %s", tcpConnection.getPeerAddr().toString().c_str());
		    connected = true;
		    tcpClient.close();
		    loop.quit();
	    });

        bool timedout = false;
	    tcpClient.setConnectingTimeoutHandler([&](){
	    	LOG_INFO("--connected to %s timedout", tcpClient.getDstAddr().toString().c_str());
	    	timedout = true;
		    tcpClient.close();
		    loop.quit();
	    });

	    bool errOcurred = false;
	    tcpClient.setConnectingErrorHandler([&](int errNo, const std::string &errMsg){
	    	LOG_INFO("--connect to %s failed, error:%d %s", tcpClient.getDstAddr().toString().c_str(), errNo, errMsg.c_str());
	        errOcurred = true;
	        tcpClient.close();
		    loop.quit();
	    });

        int timeoutSecs = 0;
	    tcpClient.connect(ip, port, timeoutSecs);
	    loop.loop();
	    ASSERT_TRUE(timedout);

	    std::this_thread::sleep_for(std::chrono::milliseconds(500));
        ::kill(pid, SIGKILL);
	    int status = 0;
	    pid_t ret = ::waitpid(pid, &status, 0);
	    LOG_INFO("child %d exited, exit code = %d", ret, status);
	    EXPECT_EQ(SIGKILL, status);
	}

	LOG_INFO("exiting");
}
*/
