// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <sys/resource.h>

#include <thread>
#include <functional>

#include "TcpServer.h"
#include "EventLoop.h"
#include "Acceptor.h"
#include "utils/rlimit.h"
#include "utils/log.h"

using namespace easynet;

namespace
{

const int kWorkerNumDefault                         = 1;
const int kWorkerTimeResolutionMillisDefault        = 0;   // without time resoluion in default
const int kWorkerDelayMillisRetryAquireTokenDefault = 100;

const int kTcpWorkerConnectionPoolCoreSizeDefault      = 256;
const int kTcpWorkerConnectionPoolMaxSizeDeault        = 1024;
const int kTcpWorkerConnectionPoolLivingTimeSecsDeault = 20;

const int kMinAcceptsPerCallDefault = 32;
const int kMaxAcceptsPerCallDefault = 1024;
const int kListenBacklog            = 1024;

const Worker::LoadBalanceStrategy kLoadBalanceStrategyDefault = Worker::LOAD_BALANCE_STRATEGY_TOKEN_RING_BY_METRIC_SMALLER;

}

TcpServer::TcpServer()
	           : workerLoadBalanceStrategy_(kLoadBalanceStrategyDefault),
	             workerNum_(kWorkerNumDefault),
                 workerTimeResolutionMillis_(kWorkerTimeResolutionMillisDefault),
                 workerDelayMillisRetryAquireToken_(kWorkerDelayMillisRetryAquireTokenDefault),
                 tcpWorkerConnectionPoolCoreSize_(kTcpWorkerConnectionPoolCoreSizeDefault),
				 tcpWorkerConnectionPoolMaxSize_(kTcpWorkerConnectionPoolMaxSizeDeault),
	     		 tcpWorkerConnectionPoolLivingTimeSecs_(kTcpWorkerConnectionPoolLivingTimeSecsDeault),
	     		 minAcceptsPerCall_(kMinAcceptsPerCallDefault),
				 maxAcceptsPerCall_(kMaxAcceptsPerCallDefault)			 
{}

TcpServer::TcpServer(const std::string &listenIp, unsigned short listenPort)
               : TcpServer()            
{ 
	addListenAddr(listenIp, listenPort);
}

void TcpServer::start()
{
	setNofileLimit();
	initListenSockets();

	createWorkerGroup();
	createTcpWorkers();

	startWorkers();
}

void TcpServer::setNofileLimit()
{
	int resource = RLIMIT_NOFILE;
	size_t softLimit = 0;
	size_t hardLimit = 0;

	if (getRlimit(resource, softLimit, hardLimit) != 0)
	{
		LOG_FATAL("TcpServer: can't get resource(RLIMIT_NOFILE)'s limit");
		::exit(1);
	}

	LOG_INFO("current process's soft limit:%ld, hard limit:%ld", softLimit, hardLimit);

	size_t required = workerNum_ * tcpWorkerConnectionPoolMaxSize_;
	
	if (required > softLimit)
	{
		softLimit = hardLimit = required;
		if (setRlimit(resource, softLimit, hardLimit) != 0)
		{
			LOG_FATAL("TcpServer: set resource(RLIMIT_NOFILE)'s limit to %u error", required);
			::exit(1);
		}
	}

	if (getRlimit(resource, softLimit, hardLimit) != 0)
	{
		LOG_FATAL("TcpServer: can't get resource(RLIMIT_NOFILE)'s limit");
		::exit(1);
	}

	LOG_INFO("current process's soft limit set to %ld, hard limit set to %ld", softLimit, hardLimit);

	return;
}

void TcpServer::initListenSockets()
{
	std::vector<InetAddr> listenAddrs = listenAddrMgr_.getListenAddrs();
	for (auto &addr : listenAddrs)
	{
		std::unique_ptr<Socket> socket(new Socket());
		socket->setNonBlocking(true);
		socket->setReuseAddr(true);
		socket->setCloseOnExec(true);
		if (socket->bind(addr) != 0)
		{
			LOG_FATAL("socket bind() at %s failed", addr.toString().c_str());
			::exit(1);
		}

		if (socket->listen(kListenBacklog) != 0)
		{
			LOG_FATAL("socket listen() error!");
			::exit(1);
		}
		listenSockets_.push_back(std::move(socket));
		LOG_INFO("tcp server bind at %s", addr.toString().c_str());
	}
}

void TcpServer::createWorkerGroup()
{
    workerGroup_.reset(new WorkerGroup(workerLoadBalanceStrategy_,
    	                               workerNum_,
    	                               workerTimeResolutionMillis_,
    	                               workerDelayMillisRetryAquireToken_));
}

void TcpServer::createTcpWorkers()
{
	std::vector<Socket*> listenSockets;
	for (auto &socket : listenSockets_)
	{
		listenSockets.push_back(socket.get());
	}
	
	std::vector<Worker*> workers = workerGroup_->getWorkers();
	for (auto worker : workers)
	{
		std::unique_ptr<TcpWorker> tcpWorker(new TcpWorker(listenSockets,
		                                          minAcceptsPerCall_, 
												  maxAcceptsPerCall_, 
												  tcpWorkerConnectionPoolCoreSize_,
												  tcpWorkerConnectionPoolMaxSize_,
												  tcpWorkerConnectionPoolLivingTimeSecs_,
												  worker));

		tcpWorker->setNewConnectionHandler(newConnectionHandler_);
		tcpWorker->setReadHandler(readHandler_);
		tcpWorker->setWriteCompleteHandler(writeCompleteHandler_);
		tcpWorker->setPeerShutdownHandler(peerShutdownHandler_);
		tcpWorker->setDisconnectedHandler(disconnectedHandler_);

		tcpWorkers_.push_back(std::move(tcpWorker));
	}
}

std::vector<EventLoop*> TcpServer::getWorkersLoops() const
{
	std::vector<EventLoop*> v;
	std::vector<Worker*> workers = workerGroup_->getWorkers();
	for (auto worker : workers)
	{
		v.push_back(worker->getLoop());
	}

	return std::move(v);
}

std::vector<int> TcpServer::getTcpWorkersConnectionNums() const
{
	std::vector<int> v;
	for (auto &tcpWorker : tcpWorkers_)
	{
		v.push_back(tcpWorker->getConnectionsNum());		
	}
	return std::move(v);
}
