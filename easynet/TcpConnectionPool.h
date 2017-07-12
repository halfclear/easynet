// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_TCP_CONNECTION_POOL_H_
#define _EASYNET_TCP_CONNECTION_POOL_H_

#include <list>
#include <memory>

namespace easynet
{

class TcpConnection;
class EventLoop;
class Timer;

class TcpConnectionPool
{
public:
	TcpConnectionPool(EventLoop *loop,
		              unsigned int corePoolSize,
	                  unsigned int maxPoolSize,
	                  int livingTimeSecs);
	~TcpConnectionPool();
	
	TcpConnectionPool(const TcpConnectionPool &rhs) = delete;
	TcpConnectionPool& operator=(const TcpConnectionPool &rhs) = delete;

	TcpConnection* pop();
	void push(TcpConnection *tcpConnection);

	unsigned int getFreeConnectionsNum() const { return numFreeConnections_; }
	unsigned int getAllocatiedConnectionsNum() const { return numAllocatedConnections_; }

private:

	void onFreeConnections();

	EventLoop *loop_;

    unsigned int numAllocatedConnections_;
    unsigned int numFreeConnections_;
    int timerInterval_;

	unsigned int corePoolSize_;
	unsigned int maxPoolSize_;
	int livingTimeMillis_;

	std::list<std::unique_ptr<TcpConnection>> tcpConnections_;

	Timer *timer_;
	int closeTimerCounter_;
};

}

#endif