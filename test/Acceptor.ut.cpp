#include <unistd.h>

#include <string>
#include <iostream>
#include <thread>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <random>

#include "Acceptor.h"
#include "TcpClient.h"
#include "Socket.h"

#include "EventLoop.h"

#include "utils/TimeUtil.h"
#include "utils/log.h"

#include <test_harness.h>

using namespace std;
using namespace easynet;

TEST(Acceptor, testMinAcceptPerCall)
{
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
	
    std::string ip = "127.0.0.1";
    unsigned short port = 12251;

    int minAcceptsPerCall = 32;
    int maxAcceptsPerCall = 64;
    int clientNum = minAcceptsPerCall + 10;

    LOG_INFO("-----------------------------------------------------");
	LOG_INFO("Acceptor-testMinAcceptPerCall:should accept %d connections per call", minAcceptsPerCall);
	LOG_INFO("-----------------------------------------------------");

	Socket socket;
    socket.setNonBlocking(true);
    socket.setReuseAddr(true);
	if (socket.bind(ip, port) != 0)
	{
		LOG_INFO("bind at %s:%d error", ip.c_str(), port);
		ASSERT_TRUE(false);
		return;
	}
	socket.listen(1024);

	pid_t pid = ::fork();
	if (pid < 0) {
		LOG_ERROR("fork error");
	} else if (pid == 0) { // child

        LOG_INFO("child starting-----");
        Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
        socket.close();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        EventLoop loop;
	    std::vector<std::unique_ptr<TcpClient>> v;
		for (int i = 0; i < clientNum; i++)
		{
			std::unique_ptr<TcpClient> tcpClient(new TcpClient(&loop));
			v.push_back(std::move(tcpClient));
		}

		for (auto &tcpClient : v)
		{
			tcpClient->connect(ip, port);
		}

		loop.loop();
		
		LOG_INFO("child exiting-----");
        
        exit(0);
	} else {    // parent
		LOG_INFO("parent starting, child pid = %d", pid);
	
		EventLoop loop;
		
		int acceptedCntPerCall = 0;
		Acceptor acceptor(&loop, &socket, minAcceptsPerCall, maxAcceptsPerCall);
		acceptor.setNewConnectionHandler([&](Socket &&socket, const InetAddr &peerAddr)->int {
            acceptedCntPerCall++;
			LOG_INFO("--accepted %d connection from %s", acceptedCntPerCall, peerAddr.toString().c_str());
			return EASYNET_SUGGEST_STOP_ACCEPT;
		});
        acceptor.enableListening();

        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        loop.runAfter(0, [&]{
        	EXPECT_EQ(minAcceptsPerCall, acceptedCntPerCall);
        });

        loop.runAfter(600, [&]{
        	loop.quit();
        });

		loop.loop();
		EXPECT_EQ(clientNum, acceptedCntPerCall);
		
        ::kill(pid, SIGKILL);
	    int status = 0;
	    pid_t ret = ::waitpid(pid, &status, 0);
	    LOG_INFO("child %d exited, exit code = %d", ret, status);
	    EXPECT_EQ(SIGKILL, status);
	}

	LOG_INFO("exiting");
}

TEST(Acceptor, testMaxAcceptPerCall)
{
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
    std::string ip = "127.0.0.1";
    unsigned short port = 12251;

    int minAcceptsPerCall = 32;
    int maxAcceptsPerCall = 64;
    int clientNum = maxAcceptsPerCall + 10;

    LOG_INFO("-----------------------------------------------------");
	LOG_INFO("Acceptor-testMaxAcceptPerCall:should accept %d connections per call", maxAcceptsPerCall);
	LOG_INFO("-----------------------------------------------------");

    Socket socket;
    socket.setNonBlocking(true);
    socket.setReuseAddr(true);
	if (socket.bind(ip, port) != 0)
	{
		LOG_INFO("bind at %s:%d error", ip.c_str(), port);
		ASSERT_TRUE(false);
		return;
	}
	socket.listen(1024);

	pid_t pid = ::fork();
	if (pid < 0) {
		LOG_ERROR("fork error");
	} else if (pid == 0) { // child

        LOG_INFO("child starting-----");
		Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
		socket.close();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

        EventLoop loop;
	    std::vector<std::unique_ptr<TcpClient>> v;
		for (int i = 0; i < clientNum; i++)
		{
			std::unique_ptr<TcpClient> tcpClient(new TcpClient(&loop));
			v.push_back(std::move(tcpClient));
		}

		for (auto &tcpClient : v)
		{
			tcpClient->connect(ip, port);
		}

		loop.loop();
		
		LOG_INFO("child exiting-----");
        
        exit(0);
	} else {    // parent
		LOG_INFO("parent starting, child pid = %d", pid);

		EventLoop loop;
        int acceptedCntPerCall = 0;
		Acceptor acceptor(&loop, &socket, minAcceptsPerCall, maxAcceptsPerCall);
		acceptor.setNewConnectionHandler([&](Socket &&socket, const InetAddr &peerAddr)->int {
			acceptedCntPerCall++;
			LOG_INFO("--accepted %d connection from %s", acceptedCntPerCall, peerAddr.toString().c_str());
			return EASYNET_ACCEPT_NEXT;
		});
        acceptor.enableListening();

        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        loop.runAfter(0, [&]{
        	EXPECT_EQ(maxAcceptsPerCall, acceptedCntPerCall);
        });

        loop.runAfter(600, [&]{
        	loop.quit();
        });

		loop.loop();
		EXPECT_EQ(clientNum, acceptedCntPerCall);
 
        ::kill(pid, SIGKILL);
	    int status = 0;
	    pid_t ret = ::waitpid(pid, &status, 0);
	    LOG_INFO("child %d exited, exit code = %d", ret, status);
	    EXPECT_EQ(SIGKILL, status);
	}

	LOG_INFO("exiting");
}

TEST(Acceptor, testStopAccept)
{
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);

    std::string ip = "127.0.0.1";
    unsigned short port = 12251;

    int minAcceptsPerCall = 32;
    int maxAcceptsPerCall = 64;
    int clientNum = 10;

    LOG_INFO("-----------------------------------------------------");
	LOG_INFO("Acceptor-testMinAcceptPerCall:should accept 1 connection per call");
	LOG_INFO("-----------------------------------------------------");

    Socket socket;
    socket.setNonBlocking(true);
    socket.setReuseAddr(true);
	if (socket.bind(ip, port) != 0)
	{
		LOG_INFO("bind at %s:%d error", ip.c_str(), port);
		ASSERT_TRUE(false);
		return;
	}
	socket.listen(1024);

	pid_t pid = ::fork();
	if (pid < 0) {
		LOG_ERROR("fork error");
	} else if (pid == 0) { // child

        LOG_INFO("child starting-----");
		Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
		socket.close();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

        EventLoop loop;
	    std::vector<std::unique_ptr<TcpClient>> v;
		for (int i = 0; i < clientNum; i++)
		{
			std::unique_ptr<TcpClient> tcpClient(new TcpClient(&loop));
			v.push_back(std::move(tcpClient));
		}

		for (auto &tcpClient : v)
		{
			tcpClient->connect(ip, port);
		}

		loop.loop();
		
		LOG_INFO("child exiting-----");
        
        exit(0);
	} else {    // parent
		LOG_INFO("parent starting, child pid = %d", pid);

		EventLoop loop;
        int acceptedCntPerCall = 0;
		Acceptor acceptor(&loop, &socket, minAcceptsPerCall, maxAcceptsPerCall);
		acceptor.setNewConnectionHandler([&](Socket &&socket, const InetAddr &peerAddr)->int {
		    acceptedCntPerCall++;
			LOG_INFO("--accepted %d connection from %s", acceptedCntPerCall, peerAddr.toString().c_str());
			return EASYNET_STOP_ACCEPT;
		});
        acceptor.enableListening();

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        int round = 0;
        Timer *t = loop.runAfter(0, [&]{
        	EXPECT_EQ(1, acceptedCntPerCall);
        	acceptedCntPerCall = 0;
        	round++;
        	if (round < clientNum) {
        		t->restart(0);
        	}
        });

        loop.runAfter(500, [&]{
        	loop.quit();
        });

		loop.loop();
		    
        ::kill(pid, SIGKILL);
	    int status = 0;
	    pid_t ret = ::waitpid(pid, &status, 0);
	    LOG_INFO("child %d exited, exit code = %d", ret, status);
	    EXPECT_EQ(SIGKILL, status);
	}

	LOG_INFO("exiting");
}
