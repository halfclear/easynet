#include <unistd.h>

#include <string>
#include <iostream>
#include <fstream>
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
#include "utils/rlimit.h"
#include "utils/log.h"

#include <test_harness.h>

using namespace std;
using namespace easynet;

TEST(TcpServer, testIdleClose)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
	LOG_INFO("-----------------------------------------------------");
	LOG_INFO("TcpServer-testIdleClose");
	LOG_INFO("-----------------------------------------------------");

    std::string ip = "127.0.0.1";
    unsigned short port = 12258;

	pid_t pid = ::fork();
	if (pid < 0) {
		LOG_ERROR("fork error");
	} else if (pid == 0) { // child

		LOG_INFO("child starting-----");
		Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		EventLoop loop;
		std::vector<std::unique_ptr<TcpClient>> v;
	    for (int i = 0; i < 2; i++)
	    {
	        std::unique_ptr<TcpClient> client(new TcpClient(&loop));
		    TcpClient *tcpClient = client.get();

		    tcpClient->setConnectedHandler([&](TcpConnection &tcpConnection){
		    	LOG_INFO("---client: connected to %s", tcpConnection.getPeerAddr().toString().c_str());
		    });

		    tcpClient->setPeerShutdownHandler([&](TcpConnection &tcpConn){
				LOG_INFO("----client: server closed this connection just now.");
				tcpConn.close();
			});
			v.push_back(std::move(client));
	    }

        int timeoutSecs = 10;
		v[0]->connect(ip, port, timeoutSecs);
		
		loop.runAfter(1500, [&](){
			v[1]->connect(ip, port, timeoutSecs);
		});
		
	    loop.loop();
		LOG_INFO("child exiting-----");

		exit(0);
	} else {    // parent
		LOG_INFO("parent starting, child pid = %d", pid);
	
		EventLoop loop;
		bool testok = false;
		int cnt = 0;

        TcpServer tcpServer(port);
		tcpServer.setNewTcpConnectionHandler([&](TcpConnection &tcpConnection){
			LOG_INFO("---Server: accepted new connection from %s",  tcpConnection.getPeerAddr().toString().c_str());
			tcpConnection.setIdleHandler(2, [&](){
				cnt++;
				int64_t realIdleTime = (tcpConnection.getLoop())->now() - tcpConnection.getEstablishmentTime();
				int64_t diff = realIdleTime - 2000;
				if (diff < 0) {
					diff *= -1;
				}
				ASSERT_LE(diff, 1000);
				LOG_INFO("---Server: connection from %s idled for %lld millis, close it", 
					tcpConnection.getPeerAddr().toString().c_str(), realIdleTime);

				testok = true;
				if (cnt == 2) {
					loop.quit();
				}
				tcpConnection.close(); // must be the last statement in the idle handler
			});
		});

		tcpServer.setPeerShutdownHandler([&](TcpConnection &tcpConnection){
			LOG_INFO("---Server: connection from %s shutdown",  tcpConnection.getPeerAddr().toString().c_str());
		});

		tcpServer.setReadHandler([&](TcpConnection &tcpConnection){
			Buffer &buffer = tcpConnection.getInputBuffer();
			LOG_INFO("---Server: received data from client:%s",  buffer.data());
			buffer.clear();
		});

		tcpServer.start();
		loop.loop();
		tcpServer.stop();
	    ASSERT_TRUE(testok);
	    EXPECT_EQ(2, cnt);

	    std::this_thread::sleep_for(std::chrono::milliseconds(200));
        ::kill(pid, SIGKILL);
	    int status = 0;
	    pid_t ret = ::waitpid(pid, &status, 0);
	    LOG_INFO("child %d exited, exit code = %d", ret, status);
	    EXPECT_EQ(SIGKILL, status);
	}

	LOG_INFO("exiting");
}

TEST(TcpServer, testUpdateIdle)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
	LOG_INFO("-----------------------------------------------------");
	LOG_INFO("TcpServer-testUpdateIdle");
	LOG_INFO("-----------------------------------------------------");

    std::string ip = "127.0.0.1";
    unsigned short port = 12258;

	pid_t pid = ::fork();
	if (pid < 0) {
		LOG_ERROR("fork error");
	} else if (pid == 0) { // child

		LOG_INFO("child starting-----");
		Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		EventLoop loop;
		std::vector<std::unique_ptr<TcpClient>> v;
	    for (int i = 0; i < 2; i++)
	    {
	        std::unique_ptr<TcpClient> client(new TcpClient(&loop));
		    TcpClient *tcpClient = client.get();

		    tcpClient->setConnectedHandler([&](TcpConnection &tcpConnection){
		    	LOG_INFO("---client: connected to %s", tcpConnection.getPeerAddr().toString().c_str());
		    });

		    tcpClient->setPeerShutdownHandler([&](TcpConnection &tcpConn){
				LOG_INFO("---client: server closed this connection just now.");
				tcpConn.close();
			});
			v.push_back(std::move(client));
	    }

        int timeoutSecs = 10;
		v[0]->connect(ip, port, timeoutSecs);
		
		loop.runAfter(500, [&](){
			v[1]->connect(ip, port, timeoutSecs);
		});
        
		int cnt = 0;
		loop.runAfter(1000, [&](){
			cnt++;
			v[0]->getTcpConnection().send("hello easynet!");
			if (cnt == 4){
				v[0]->close();
			}
		}, 1000);
		
	    loop.loop();
		LOG_INFO("child exiting-----");

		exit(0);
	} else {    // parent
		LOG_INFO("parent starting, child pid = %d", pid);
	
		EventLoop loop;
		int cnt = 0;

        TcpServer tcpServer(port);
		tcpServer.setNewTcpConnectionHandler([&](TcpConnection &tcpConnection){
			LOG_INFO("---Server: accepted new connection from %s",  tcpConnection.getPeerAddr().toString().c_str());
			tcpConnection.setIdleHandler(2, [&](){
				cnt++;
				int64_t realIdleTime = (tcpConnection.getLoop())->now() - tcpConnection.getEstablishmentTime();
				int64_t diff = realIdleTime - 2000;
				if (diff < 0) {
					diff *= -1;
				}
				ASSERT_LE(diff, 1000);
				LOG_INFO("---Server: connection from %s idled for %lld millis, close it", 
					tcpConnection.getPeerAddr().toString().c_str(), realIdleTime);
				tcpConnection.close(); // must be the last statement in the idle handler
			});
		});

        int closeCnt = 0;
		tcpServer.setPeerShutdownHandler([&](TcpConnection &tcpConnection){
			closeCnt++;
			LOG_INFO("---Server: connection from %s shutdown",  tcpConnection.getPeerAddr().toString().c_str());
			tcpConnection.close();
			loop.quit();
		});

        int readCnt = 0;
		tcpServer.setReadHandler([&](TcpConnection &tcpConnection){
			readCnt++;
			Buffer &buffer = tcpConnection.getInputBuffer();
			LOG_INFO("---Server: received data from client%s:%s", tcpConnection.getPeerAddr().toString().c_str(), buffer.data());
			buffer.deleteBegin(buffer.size());
		});

		tcpServer.start();
		loop.loop();
		tcpServer.stop();
	    EXPECT_EQ(1, cnt);
	    EXPECT_EQ(1, closeCnt);
	    EXPECT_EQ(4, readCnt);

	    std::this_thread::sleep_for(std::chrono::milliseconds(200));
        ::kill(pid, SIGKILL);
	    int status = 0;
	    pid_t ret = ::waitpid(pid, &status, 0);
	    LOG_INFO("child %d exited, exit code = %d", ret, status);
	    EXPECT_EQ(SIGKILL, status);
	}

	LOG_INFO("exiting");
}

TEST(TcpServer, testLoadbalance)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
	LOG_INFO("-----------------------------------------------------");
	LOG_INFO("TcpServer-testLoadbalance");
	LOG_INFO("-----------------------------------------------------");

    std::string ip = "127.0.0.1";
    unsigned short port = 12260;

    
    int clientNum = 1000;

	pid_t pid = ::fork();
	if (pid < 0) {
		LOG_ERROR("fork error");
	} else if (pid == 0) { // child

		LOG_INFO("child starting-----");
		Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_WARN);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		EventLoop loop;
		int connectedCnt = 0;
		std::vector<std::unique_ptr<TcpClient>> v;
	    for (int i = 0; i < clientNum; i++)
	    {
	        std::unique_ptr<TcpClient> client(new TcpClient(&loop));
		    TcpClient *tcpClient = client.get();

		    tcpClient->setConnectedHandler([&](TcpConnection &tcpConnection){
		    	connectedCnt++;
		    	LOG_INFO("---client: connected to %s, %d", tcpConnection.getPeerAddr().toString().c_str(), connectedCnt);
		    });

		    tcpClient->setPeerShutdownHandler([&](TcpConnection &tcpConn){
				LOG_INFO("---client: server closed this connection just now.");
				tcpConn.close();
			});

			tcpClient->setConnectingErrorHandler([&, tcpClient](int errNo, const std::string &errMsg){
				LOG_INFO("--- connect to server %s:%d failed, error:%d %s", 
				    ip.c_str(), port, errNo, errMsg.c_str());
			});
			v.push_back(std::move(client));
	    }

        int timeoutSecs = 10;
        int clientCnt = 0;
        for (auto &client : v)
        {
        	clientCnt++;
        //	LOG_INFO("client %d connect to server", clientCnt);
		    client->connect(ip, port, timeoutSecs);
        }
		
	    loop.loop();
		LOG_INFO("child exiting-----");

		exit(0);
	} else {    // parent
		LOG_INFO("parent starting, child pid = %d", pid);

        EventLoop loop;
        
        int workerNum = 4;
        int tcpConnectionPoolCoreSize = 200;
        int tcpConnectionPoolMaxSize  = 256;
        int livingTime = 10;
        int minAcceptPerCall = 10;
        int maxAcceptPerCall = 100;
             
        TcpServer tcpServer(port);
        tcpServer.setWorkerNum(workerNum);
		tcpServer.setTcpWorkerConnectionPoolCoreSize(tcpConnectionPoolCoreSize);
		tcpServer.setTcpWorkerConnectionPoolMaxSize(tcpConnectionPoolMaxSize);
		tcpServer.setTcpWorkerConnectionPoolLivingTime(livingTime);
		tcpServer.setMinAcceptsPerCall(minAcceptPerCall);
		tcpServer.setMaxAcceptsPerCall(maxAcceptPerCall);

        int cnt = 0;
		tcpServer.setNewTcpConnectionHandler([&](TcpConnection &tcpConnection){
			cnt++;
			LOG_INFO("---Server: accepted new connection from %s, %d", tcpConnection.getPeerAddr().toString().c_str(), cnt);
		});

		tcpServer.setDisconnectedHandler([&](TcpConnection &tcpConnection){
			LOG_INFO("---Server: connection from %s is disconnected, error:%d %s",  
				tcpConnection.getPeerAddr().toString().c_str(), 
				tcpConnection.getErrNo(), tcpConnection.getErrMsg().c_str());
		});

		tcpServer.setPeerShutdownHandler([&](TcpConnection &tcpConnection){
			LOG_INFO("---Server: connection from %s shutdown",  tcpConnection.getPeerAddr().toString().c_str());
			tcpConnection.close();
		});

		tcpServer.setReadHandler([&](TcpConnection &tcpConnection){
			Buffer &buffer = tcpConnection.getInputBuffer();
			LOG_INFO("---Server: received data from client%s:%s", 
				tcpConnection.getPeerAddr().toString().c_str(), buffer.data());
			buffer.deleteBegin(buffer.size());
		});

		tcpServer.start();

        int total = 0;
		loop.runAfter(3000, [&]{
			auto v = tcpServer.getTcpWorkersConnectionNums();
			EXPECT_EQ(workerNum, v.size());

			for (size_t i = 0; i < v.size(); i++)
			{
				total += v[i];
				LOG_INFO("worker[%d] accepted %d connections", i, v[i]);
//				cout << "worker[" << i << "] accepted " << v[i] << " connections\n";

				int diff = v[i] - clientNum / 4;
				if (diff < 0) {
					diff *= -1;
				}
				ASSERT_LE(diff, minAcceptPerCall);
				ASSERT_GE(v[i], clientNum / 4 - minAcceptPerCall * 2);
			}

			EXPECT_EQ(clientNum, total);
			LOG_INFO("total accepted %d connections", total);
//			cout << "total = " << total << endl;
			loop.quit();
		});
			
		loop.loop();
		tcpServer.stop();
	
	    EXPECT_EQ(clientNum, total);

        ::kill(pid, SIGKILL);
	    int status = 0;
	    pid_t ret = ::waitpid(pid, &status, 0);
	    LOG_INFO("child %d exited, exit code = %d", ret, status);
	    EXPECT_EQ(SIGKILL, status);
	}

	LOG_INFO("exiting");
}
