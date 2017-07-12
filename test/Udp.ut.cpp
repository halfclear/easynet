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

#include "UdpConnection.h"
#include "UdpServer.h"
#include "Buffer.h"

#include "EventLoop.h"

#include "utils/TimeUtil.h"
#include "utils/log.h"

#include <test_harness.h>

using namespace std;
using namespace easynet;

void startUdpServer(unsigned short port)
{
    UdpServer udpServer(port);
	udpServer.setMessageHandler([&](UdpConnection &udpConnection){
		Buffer &buffer = udpConnection.getInputBuffer(); 
		LOG_INFO("---udp server:received udp message from %s:%s",  udpConnection.getPeerAddr().toString().c_str()
			, buffer.data());

		udpConnection.sendTo(buffer.data(), buffer.size(), udpConnection.getPeerAddr());
		buffer.deleteBegin(buffer.size());
	});

	udpServer.setErrorHandler([&](UdpConnection &udpConnection, int errNo, const std::string& errMsg){
		LOG_INFO("---udp server:udp from %s at %s error:%d %s",  udpConnection.getPeerAddr().toString().c_str()
			, udpConnection.getLocalAddr().toString().c_str(), errNo, errMsg.c_str());
	});

	udpServer.start();

	EventLoop loop;
	loop.loop();
}

TEST(UdpSocket, testBlocking)
{
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
	LOG_INFO("----------------------------------------------");
	LOG_INFO("UdpSocket-testBlocking");
	LOG_INFO("----------------------------------------------");

    std::string ip = "127.0.0.1";
    unsigned short port = 12251;

	pid_t pid = ::fork();
	if (pid < 0) {
		LOG_ERROR("fork error");
	} else if (pid == 0) { // child

		LOG_INFO("child starting-----");
		Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
		startUdpServer(port);
		LOG_INFO("child exiting-----");

		exit(0);
	} else {    // parent
		LOG_INFO("parent starting, child pid = %d", pid);

	    std::this_thread::sleep_for(std::chrono::milliseconds(500));

	    Socket udpSocket(Socket::SocketType::SOCKET_UDP);
	    std::string msg("hello world!");

	    ssize_t bytesSent = udpSocket.sendTo(msg.data(), msg.size(), ip, port);
	    EXPECT_EQ(msg.size(), bytesSent);
	    char buf[1024];
	    std::memset(buf, 0, 1024);
	    InetAddr peerAddr;
	    ssize_t readLen = udpSocket.recvFrom(buf, 1024, &peerAddr);
	    LOG_INFO("--client:received %d bytes from %s:%s", readLen, peerAddr.toString().c_str(), buf);
	    EXPECT_EQ(msg.size(), readLen);
	    EXPECT_EQ(msg, string(buf, readLen));

	    std::this_thread::sleep_for(std::chrono::milliseconds(500));
        ::kill(pid, SIGKILL);
	    int status = 0;
	    pid_t ret = ::waitpid(pid, &status, 0);
	    LOG_INFO("child %d exited, exit code = %d", ret, status);
	    EXPECT_EQ(SIGKILL, status);
	    
	}

	LOG_INFO("exiting");
}

TEST(UdpServer, testUdpServer)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
	LOG_INFO("----------------------------------------------");
	LOG_INFO("UdpServer-testUdpServer");
	LOG_INFO("----------------------------------------------");

    std::string ip = "127.0.0.1";
    unsigned short port = 12251;

	pid_t pid = ::fork();
	if (pid < 0) {
		LOG_ERROR("fork error");
	} else if (pid == 0) { // child

		LOG_INFO("child starting-----");
		Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
		startUdpServer(port);
		LOG_INFO("child exiting-----");

		exit(0);
	} else {    // parent
		LOG_INFO("parent starting, child pid = %d", pid);

	    std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::string msg("hello world!");
        vector<string> stringsVec;
		stringsVec.push_back(string("hello world!"));
		stringsVec.push_back(string("this is easynet."));
		stringsVec.push_back(string("thank you for using."));

		bool testok = false;
		int sendIndex = 0;

	    EventLoop loop;
	    UdpConnection udpConnection(&loop);

	    udpConnection.setMessageHandler([&](UdpConnection &udpConnection){
		    Buffer &buffer = udpConnection.getInputBuffer();
		    
			LOG_INFO("----client: received udp message from %s:%s",  udpConnection.getPeerAddr().toString().c_str()
				, buffer.data());

            EXPECT_EQ(stringsVec[sendIndex], string(buffer.data(), buffer.size()));
            buffer.deleteBegin(buffer.size());

			sendIndex++;
            if (sendIndex == 3) {
            	loop.quit();
            	testok = true;
            }
		});

		udpConnection.setErrorHandler([&](UdpConnection &udpConnection, int errNo, const std::string& errMsg){
			LOG_INFO("----client: udp from %s at %s error:%d %s",  udpConnection.getPeerAddr().toString().c_str()
				, udpConnection.getLocalAddr().toString().c_str(), errNo, errMsg.c_str());
		});

        for (size_t i = 0; i < stringsVec.size(); i++) {
            udpConnection.sendTo(stringsVec[i].c_str(), stringsVec[i].size(), "127.0.0.1", port);
        }
		
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

TEST(UdpConnection, testBind)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
	LOG_INFO("----------------------------------------------");
	LOG_INFO("UdpConnection-testBind");
	LOG_INFO("----------------------------------------------");

    std::string ip = "127.0.0.1";
    unsigned short port = 12251;

	pid_t pid = ::fork();
	if (pid < 0) {
		LOG_ERROR("fork error");
	} else if (pid == 0) { // child

		LOG_INFO("child starting-----");
		Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
		startUdpServer(port);
		LOG_INFO("child exiting-----");

		exit(0);
	} else {    // parent
		LOG_INFO("parent starting, child pid = %d", pid);

	    std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::string msg("hello world!");

		bool testok = false;

	    EventLoop loop;
	    UdpConnection udpConnection(&loop);

	    if (udpConnection.bind(ip, port + 1) < 0)
	    {
	    	LOG_ERROR("bind at %d failed", port+1);
	    	ASSERT_TRUE(false);
	    	goto end;
	    }

	    udpConnection.setMessageHandler([&](UdpConnection &udpConnection){
		    Buffer &buffer = udpConnection.getInputBuffer();
		    
			LOG_INFO("----client: received udp message from %s:%s",  udpConnection.getPeerAddr().toString().c_str()
				, buffer.data());

            EXPECT_EQ(msg, string(buffer.data(), buffer.size()));
            if (msg == string(buffer.data(), buffer.size()))
            {
            	loop.quit();
            	testok = true;
            }

            buffer.deleteBegin(buffer.size());
		});

		udpConnection.setErrorHandler([&](UdpConnection &udpConnection, int errNo, const std::string& errMsg){
			LOG_INFO("----client: udp from %s at %s error:%d %s",  udpConnection.getPeerAddr().toString().c_str()
				, udpConnection.getLocalAddr().toString().c_str(), errNo, errMsg.c_str());
		});

        udpConnection.sendTo(msg, ip, port);
		
	    loop.loop();
	    ASSERT_TRUE(testok);

end:
	    std::this_thread::sleep_for(std::chrono::milliseconds(500));
        ::kill(pid, SIGKILL);
	    int status = 0;
	    pid_t ret = ::waitpid(pid, &status, 0);
	    LOG_INFO("child %d exited, exit code = %d", ret, status);
	    EXPECT_EQ(SIGKILL, status);
	}

	LOG_INFO("exiting");
}

TEST(UdpConnection, testConnect)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
	LOG_INFO("----------------------------------------------");
	LOG_INFO("UdpConnection-testConnect");
	LOG_INFO("----------------------------------------------");

    std::string ip = "127.0.0.1";
    unsigned short port = 12251;

	pid_t pid = ::fork();
	if (pid < 0) {
		LOG_ERROR("fork error");
	} else if (pid == 0) { // child

		LOG_INFO("child starting-----");
		Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
		startUdpServer(port);
		LOG_INFO("child exiting-----");

		exit(0);
	} else {    // parent
		LOG_INFO("parent starting, child pid = %d", pid);

	    std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::string msg("hello world!");

		bool testok = false;

	    EventLoop loop;
	    UdpConnection udpConnection(&loop);

	    if (udpConnection.connect(ip, port) < 0)
	    {
	    	LOG_ERROR("connect to %s:%d failed", ip.c_str(), port);
	    	ASSERT_TRUE(false);
	    	goto end;
	    }

	    udpConnection.setMessageHandler([&](UdpConnection &udpConnection){
		    Buffer &buffer = udpConnection.getInputBuffer();
		    
			LOG_INFO("----client: received udp message from %s:%s",  udpConnection.getPeerAddr().toString().c_str()
				, buffer.data());

            EXPECT_EQ(msg, string(buffer.data(), buffer.size()));
            if (msg == string(buffer.data(), buffer.size()))
            {
            	loop.quit();
            	testok = true;
            }

            buffer.deleteBegin(buffer.size());
		});

		udpConnection.setErrorHandler([&](UdpConnection &udpConnection, int errNo, const std::string& errMsg){
			LOG_INFO("----client: udp from %s at %s error:%d %s",  udpConnection.getPeerAddr().toString().c_str()
				, udpConnection.getLocalAddr().toString().c_str(), errNo, errMsg.c_str());
		});

        udpConnection.send(msg);
		
	    loop.loop();
	    ASSERT_TRUE(testok);

end:
	    std::this_thread::sleep_for(std::chrono::milliseconds(500));
        ::kill(pid, SIGKILL);
	    int status = 0;
	    pid_t ret = ::waitpid(pid, &status, 0);
	    LOG_INFO("child %d exited, exit code = %d", ret, status);
	    EXPECT_EQ(SIGKILL, status);
	}

	LOG_INFO("exiting");
}

TEST(UdpConnection, testErrorHandler)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
	LOG_INFO("----------------------------------------------");
	LOG_INFO("UdpConnection-testErrorHandler");
	LOG_INFO("----------------------------------------------");

    std::string ip = "127.0.0.1";
    unsigned short port = 12252;
    std::string msg("hello easynet!");
    bool errorOccurred = false;

    EventLoop loop;
    UdpConnection udpConnection(&loop);
    
    udpConnection.setMessageHandler([&](UdpConnection &udpConnection){
	    Buffer &buffer = udpConnection.getInputBuffer();
	    
		LOG_INFO("----received udp message from server %s:%s", 
			udpConnection.getPeerAddr().toString().c_str(), buffer.data());

        buffer.deleteBegin(buffer.size());
	});

	udpConnection.setErrorHandler([&](UdpConnection &udpConnection, int errNo, const std::string& errMsg){
		LOG_INFO("----udp from %s at %s error:%d %s",  udpConnection.getPeerAddr().toString().c_str()
			, udpConnection.getLocalAddr().toString().c_str(), errNo, errMsg.c_str());
		errorOccurred = true;
		loop.quit();
	});

	udpConnection.sendTo(msg, ip, port);

    loop.runAfter(2000, [&]{
    	loop.quit();
    });

    loop.loop();
    ASSERT_TRUE(errorOccurred);
}
