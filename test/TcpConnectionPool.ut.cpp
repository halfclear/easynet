#include <string>
#include <iostream>
#include <thread>
#include <future>
#include <functional>

#include "TimeWheel.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include "TcpConnectionPool.h"
#include "utils/TimeUtil.h"
#include <test_harness.h>

#include "utils/log.h"

using namespace std;
using namespace easynet;

TEST(TcpConnectionPool, testTcpConnectionPool)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
    LOG_INFO("-----------------------------------------------------");
    LOG_INFO("TcpConnectionPool-testTcpConnectionPool");
    LOG_INFO("-----------------------------------------------------");

	EventLoop loop;

	int corePoolSize = 128;
	int maxPoolSize = 512;
	int livingTimeSecs = 10;

	TcpConnectionPool pool(&loop, 
        static_cast<unsigned int>(corePoolSize), 
        static_cast<unsigned int>(maxPoolSize), 
        livingTimeSecs);

	EXPECT_EQ(pool.getFreeConnectionsNum(), 0);
    EXPECT_EQ(pool.getAllocatiedConnectionsNum(), 0);

    std::vector<TcpConnection*> v;
    for (int i = 0; i < maxPoolSize; i++)
    {
    	TcpConnection *p = pool.pop();
        ASSERT_TRUE(p != nullptr);
        v.push_back(p);
    }

    TcpConnection *p = pool.pop();
    ASSERT_TRUE(p == nullptr);

    EXPECT_EQ(pool.getFreeConnectionsNum(), 0);
    EXPECT_EQ(pool.getAllocatiedConnectionsNum(), maxPoolSize);

    loop.runAfter(100, [&]{
    	for (auto tcpConn : v)
    	{
    		pool.push(tcpConn);
    	}

//    	cout << "after return back, free connections:" << pool.getFreeConnectionsNum() 
//    	     << ", allocatiedConnectionsNum:" << pool.getAllocatiedConnectionsNum() << endl;

    	EXPECT_EQ(pool.getFreeConnectionsNum(), maxPoolSize);
        EXPECT_EQ(pool.getAllocatiedConnectionsNum(), maxPoolSize);
    });

    int round = 0;
    loop.runAfter(1500, [&]{
    	round++;
 //       cout << "after 1500 ms, free connections:" << pool.getFreeConnectionsNum() 
 //   	     << ", allocatiedConnectionsNum:" << pool.getAllocatiedConnectionsNum() << endl;

        int leftFreeConnections = maxPoolSize - 100 * round;
        if (leftFreeConnections < 0  ||
        	leftFreeConnections < corePoolSize)
        {
        	leftFreeConnections = corePoolSize;
        }
    	EXPECT_EQ(pool.getFreeConnectionsNum(), leftFreeConnections);
        EXPECT_EQ(pool.getAllocatiedConnectionsNum(), leftFreeConnections);
    }, 1000);

    loop.runAfter(13000, [&]{

 //       cout << "after 13s, free connections:" << pool.getFreeConnectionsNum() 
 //   	     << ", allocatiedConnectionsNum:" << pool.getAllocatiedConnectionsNum() << endl;

    	EXPECT_EQ(pool.getFreeConnectionsNum(), corePoolSize);
        EXPECT_EQ(pool.getAllocatiedConnectionsNum(), corePoolSize);

        loop.quit();
    });

    loop.loop();
}
