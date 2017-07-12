// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_TCP_SERVER_H_
#define _EASYNET_TCP_SERVER_H_

#include <string>
#include <vector>
#include <memory>

#include "Socket.h"
#include "WorkerGroup.h"
#include "TcpWorker.h"
#include "InetAddr.h"
#include "ListenAddrMgr.h"
#include "TcpConnection.h"

namespace easynet
{

class EventLoop;

class TcpServer
{
public:
	using TcpConnectionHandler = TcpConnection::TcpConnectionHandler;

	TcpServer();
	explicit TcpServer(unsigned short listenPort)
		        : TcpServer(std::string("*"), listenPort)
	{}

	TcpServer(const std::string &listenIp, unsigned short listenPort);
	~TcpServer() { stop(); }

	TcpServer(const TcpServer &rhs) = delete;
	TcpServer& operator=(const TcpServer &rhs) = delete;

    bool addListenAddr(unsigned short port) 
    { return listenAddrMgr_.addListenAddr(InetAddr(port)); }
    bool addListenAddr(const std::string &ip, unsigned short port) 
    { return listenAddrMgr_.addListenAddr(InetAddr(ip, port)); }
    bool addListenAddr(const InetAddr &addr) { return listenAddrMgr_.addListenAddr(addr); } 
    
    void setWorkerLoadBalanceStrategy(Worker::LoadBalanceStrategy strategy) { workerLoadBalanceStrategy_ = strategy; }
	void setWorkerNum(int num) { workerNum_ = num; }
	void setWorkerTimeResultion(int millis)  { workerTimeResolutionMillis_ = millis; }
	void setWorkerDelayMillisRetryAquireToken(int millis) { workerDelayMillisRetryAquireToken_ = millis;}
	
	void setTcpWorkerConnectionPoolCoreSize(int size)    { tcpWorkerConnectionPoolCoreSize_ = size; }
	void setTcpWorkerConnectionPoolMaxSize(int size)     { tcpWorkerConnectionPoolMaxSize_ = size; }
	void setTcpWorkerConnectionPoolLivingTime(int secs)  { tcpWorkerConnectionPoolLivingTimeSecs_ = secs;}
	void setMinAcceptsPerCall(int minAcceptsPerCall)  { minAcceptsPerCall_ = minAcceptsPerCall; }
	void setMaxAcceptsPerCall(int maxAcceptsPerCall)  { maxAcceptsPerCall_ = maxAcceptsPerCall; }

	void start();
	void stop()  { workerGroup_->stop(); }

	std::vector<EventLoop*> getWorkersLoops() const;
	std::vector<int> getTcpWorkersConnectionNums() const;

    void setNewTcpConnectionHandler(TcpConnectionHandler &&handler) { newConnectionHandler_ = std::move(handler); }
	void setNewTcpConnectionHandler(const TcpConnectionHandler &handler)
	{ setNewTcpConnectionHandler(TcpConnectionHandler(handler)); }

	void setReadHandler(TcpConnectionHandler &&handler) { readHandler_ = std::move(handler); }
	void setReadHandler(const TcpConnectionHandler &handler)
	{ setReadHandler(TcpConnectionHandler(handler)); }

	void setWriteCompleteHandler(TcpConnectionHandler &&handler) { writeCompleteHandler_ = std::move(handler); }
	void setWriteCompleteHandler(const TcpConnectionHandler &handler)
	{ setWriteCompleteHandler(TcpConnectionHandler(handler)); }

	void setPeerShutdownHandler(TcpConnectionHandler &&handler) { peerShutdownHandler_ = std::move(handler); }
	void setPeerShutdownHandler(const TcpConnectionHandler &handler)
	{ setPeerShutdownHandler(TcpConnectionHandler(handler)); }

	void setDisconnectedHandler(TcpConnectionHandler &&handler) { disconnectedHandler_ = std::move(handler); }
	void setDisconnectedHandler(const TcpConnectionHandler &handler)
	{ setDisconnectedHandler(TcpConnectionHandler(handler)); }

private:
	void initListenSockets();
	void setNofileLimit();
	
	void createWorkerGroup();
    void createTcpWorkers();
    void startWorkers() { workerGroup_->start(); }

    Worker::LoadBalanceStrategy workerLoadBalanceStrategy_;
	int workerNum_;
	int workerTimeResolutionMillis_;
	int workerDelayMillisRetryAquireToken_;
	int tcpWorkerConnectionPoolCoreSize_;
	int tcpWorkerConnectionPoolMaxSize_;
	int tcpWorkerConnectionPoolLivingTimeSecs_;
	int minAcceptsPerCall_;
	int maxAcceptsPerCall_;
	
	// key: listen port
	ListenAddrMgr listenAddrMgr_;
	std::vector<std::unique_ptr<Socket>> listenSockets_;
    
	std::unique_ptr<WorkerGroup> workerGroup_;
	std::vector<std::unique_ptr<TcpWorker>> tcpWorkers_;

	TcpConnectionHandler newConnectionHandler_;   // application's callback, will be called when new connection is accepted
	TcpConnectionHandler readHandler_;            // application's callback, will be called when data have been read from socket
	TcpConnectionHandler writeCompleteHandler_;   // application's callback, will be called when all data have been written to socket
	TcpConnectionHandler peerShutdownHandler_;    // application's callback, will be called when peer shutdown this connection
	TcpConnectionHandler disconnectedHandler_;    // application's callback, will be called when this connection is disconnected(such as connection is hup, or error occurred)
};

} // namespace easynet

#endif
