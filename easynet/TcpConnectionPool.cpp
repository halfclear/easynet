// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <functional>

#include "TcpConnectionPool.h"
#include "TcpConnection.h"
#include "EventLoop.h"
#include "Timer.h"

using namespace easynet;
using std::unique_ptr;

namespace {

const int kMinTimerIntervalSecs = 1;
const int kMaxTimerIntervalSecs = 60;
const int kFreeConnectionsPerCall = 100;

}

TcpConnectionPool::TcpConnectionPool(EventLoop *loop,
		              unsigned int corePoolSize,
	                  unsigned int maxPoolSize,
	                  int livingTimeSecs)
	                 : loop_(loop),
	                   numAllocatedConnections_(0),
	                   numFreeConnections_(0),
	                   corePoolSize_(corePoolSize),
	                   maxPoolSize_(maxPoolSize),
	                   livingTimeMillis_(livingTimeSecs * 1000),
	                   timer_(nullptr),
	                   closeTimerCounter_(0)
{
	timerInterval_ = livingTimeSecs / 10;
	if (timerInterval_ < kMinTimerIntervalSecs)
	{
		timerInterval_ = kMinTimerIntervalSecs;
	}

	if (timerInterval_ > kMaxTimerIntervalSecs)
	{
		timerInterval_ = kMaxTimerIntervalSecs;
	}

	timerInterval_ *= 1000;
}

TcpConnectionPool::~TcpConnectionPool()
{
	if (timer_)
	{
		timer_->cancel();
	}
}

TcpConnection* TcpConnectionPool::pop()
{
	std::unique_ptr<TcpConnection> tcpConnection;
	if (!tcpConnections_.empty())
	{
		numFreeConnections_--;
		tcpConnection.reset(tcpConnections_.front().release());
		tcpConnections_.pop_front();
	}
	else if (numAllocatedConnections_ < maxPoolSize_)
	{
		numAllocatedConnections_++;
		tcpConnection.reset(new TcpConnection(loop_));
	}

	return tcpConnection.release();
}

void TcpConnectionPool::push(TcpConnection* tcpConnection)
{
    numFreeConnections_++;
    tcpConnections_.push_front(std::unique_ptr<TcpConnection>(tcpConnection));

    if (!timer_  && numFreeConnections_ > corePoolSize_)
    {
    	timer_ = loop_->runAfter(timerInterval_, std::bind(&TcpConnectionPool::onFreeConnections, this), timerInterval_);
    }
}

void TcpConnectionPool::onFreeConnections()
{
	if (numFreeConnections_ <= corePoolSize_)
	{
		closeTimerCounter_++;
		if (closeTimerCounter_ > 3)
		{
			timer_->cancel();
			timer_ = nullptr;
		}
		return;
	}
	closeTimerCounter_ = 0;

    int deletedConnections = 0;
    int64_t now = loop_->now();

    auto it = tcpConnections_.end();
    --it;
	while (it != tcpConnections_.end() && deletedConnections < kFreeConnectionsPerCall)
	{
		if (getFreeConnectionsNum() > corePoolSize_ && 
			(*it)->getEstablishmentTime() < now - livingTimeMillis_)
		{
			it = tcpConnections_.erase(it);
			--it;
			numFreeConnections_--;
			numAllocatedConnections_--;
			deletedConnections++;
		}
		else
		{
			break;
		}
	}
}