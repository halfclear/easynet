// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_TCP_WORKER_H_
#define _EASYNET_TCP_WORKER_H_

#include <sys/time.h>

#include <string>
#include <memory>
#include <mutex>
#include <list>
#include <vector>
#include <map>

#include "Acceptor.h"
#include "Channel.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include "TcpConnectionPool.h"
#include "InetAddr.h"
#include "Worker.h"

namespace easynet
{

class Socket;
class TcpConnection;
class Worker;

class TcpWorker
{
public:
	using TcpConnectionHandler = TcpConnection::TcpConnectionHandler;

	TcpWorker(Socket *listenSocket,
	       int minAcceptsPerCall, 
		   int maxAcceptsPerCall,
		   int connectionPoolCoreSize,
		   int connectionPoolMaxSize,
		   int connectionPoolLivingTimeSecs,
		   Worker *worker);

	TcpWorker(const std::vector<Socket*> &listenSockets, 
		   int minAcceptsPerCall, 
		   int maxAcceptsPerCall, 
		   int connectionPoolCoreSize,
		   int connectionPoolMaxSize,
		   int connectionPoolLivingTimeSecs,
		   Worker *worker);

	TcpWorker(const TcpWorker &rhs) = delete;
	~TcpWorker() = default;

	EventLoop* getLoop() { return loop_; }
	int getConnectionsNum() const { return numCurConnections_; }
	
	void setNewConnectionHandler(const TcpConnectionHandler &handler) { newConnectionHandler_ = handler; }
	void setReadHandler(const TcpConnectionHandler &handler)          { readHandler_ = handler; }
	void setWriteCompleteHandler(const TcpConnectionHandler &handler) { writeCompleteHandler_ = handler; }
	void setPeerShutdownHandler(const TcpConnectionHandler &handler)  { peerShutdownHandler_ = handler; }
	void setDisconnectedHandler(const TcpConnectionHandler &handler)  { disconnectedHandler_ = handler; }

private:
	void init();
	void initAcceptors();
	void initWorker();
	void setTcpConntionsHandlers(TcpConnection *tcpConnection);

	void enableListening();
	void disableListening();
    bool needTryAcquireTocken();

    bool onTryAcquireToken() { return needTryAcquireTocken(); }
    int  onGetLoadBalanceMetric() const { return getConnectionsNum(); }
	void onListenTokenAcquirered() { enableListening(); }
	void onListenTokenYileded()   { disableListening(); }
	
	int  onNewConnection(Socket &&socket, const InetAddr &peerAddr);
	void onConnectionClosed(TcpConnection &tcpConnection);

	TcpConnection* getTcpConnection() { return tcpConnectionPool_.pop(); }
	void freeTcpConnection(TcpConnection* tcpConnection) {  tcpConnectionPool_.push(tcpConnection); }

	bool isSingleTcpWorker() const { return worker_->isSingleWorker(); }

	int numCurConnections_;
	int connectionPoolMaxSize_;
	int numConnectionsLoadBalancingLine_;
	int cntDisableAquireListenToken_;
	
	Worker* worker_;
	EventLoop *loop_;
	
	std::vector<std::unique_ptr<Acceptor>> acceptors_;
	TcpConnectionPool tcpConnectionPool_;

	TcpConnectionHandler newConnectionHandler_;   // application's callback, will be called when new connection is accepted
	TcpConnectionHandler readHandler_;            // application's callback, will be called when data is read from the socket
	TcpConnectionHandler writeCompleteHandler_;   // application's callback, will be called when all data is wroted to the socket
	TcpConnectionHandler peerShutdownHandler_;    // application's callback, will be called when peer shutdown this connect
	TcpConnectionHandler disconnectedHandler_;    // application's callback, will be called when this connection is disconnected(such as connection is hup, or error occurred)
};

} // namespace easynet

#endif
