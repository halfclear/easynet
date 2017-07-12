// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <unistd.h>

#include <string>
#include <memory>
#include <functional>

#include "TcpWorker.h"
#include "Worker.h"
#include "Channel.h"
#include "Socket.h"
#include "utils/log.h"

using namespace easynet;

TcpWorker::TcpWorker(Socket *listenSocket, 
	                 int minAcceptsPerCall, 
	                 int maxAcceptsPerCall,
		             int connectionPoolCoreSize,
		             int connectionPoolMaxSize,
		             int connectionPoolLivingTimeSecs,
		             Worker *worker)
                  : TcpWorker(std::vector<Socket*>(1, listenSocket),
                  	    minAcceptsPerCall,
                  	    maxAcceptsPerCall, 
		        		connectionPoolCoreSize,
		        		connectionPoolMaxSize,
		        		connectionPoolLivingTimeSecs,
		        		worker)
{}

TcpWorker::TcpWorker(const std::vector<Socket*> &listenSockets,
               int minAcceptsPerCall,  
	           int maxAcceptsPerCall,
		       int connectionPoolCoreSize,
		       int connectionPoolMaxSize,
		       int connectionPoolLivingTimeSecs,
		       Worker *worker)
                  : numCurConnections_(0),
                    connectionPoolMaxSize_(connectionPoolMaxSize),
                    worker_(worker),
                    loop_(worker_->getLoop()),
                    tcpConnectionPool_(loop_, 
                    	connectionPoolCoreSize,
                    	connectionPoolMaxSize,
                    	connectionPoolLivingTimeSecs)
{
	numConnectionsLoadBalancingLine_ = connectionPoolCoreSize * 7 / 8;
	cntDisableAquireListenToken_ = numCurConnections_ - (connectionPoolMaxSize_ * 7) / 8;

	for (auto listenSocket : listenSockets)
	{
		acceptors_.push_back(std::unique_ptr<Acceptor>(new Acceptor(loop_, listenSocket, minAcceptsPerCall, maxAcceptsPerCall)));
	}

	init();	
}

void TcpWorker::init()
{
	initAcceptors();
	initWorker();
}

void TcpWorker::initAcceptors()
{
	for (auto &acceptor : acceptors_)
	{
		acceptor->setNewConnectionHandler(std::bind(&TcpWorker::onNewConnection, this, 
			std::placeholders::_1, std::placeholders::_2));
	}
}

void TcpWorker::initWorker()
{
	worker_->setBeforeAcquireTokenHandler(std::bind(&TcpWorker::onTryAcquireToken, this));
	worker_->setLoadBalanceMetricGetter(std::bind(&TcpWorker::onGetLoadBalanceMetric, this));
	worker_->setTokenAcquiredHandler(std::bind(&TcpWorker::onListenTokenAcquirered, this));
	worker_->setTokenYieldedHandler(std::bind(&TcpWorker::onListenTokenYileded, this));
}

void TcpWorker::setTcpConntionsHandlers(TcpConnection *tcpConnection)
{
	tcpConnection->setReadHandler(readHandler_);
	tcpConnection->setWriteCompleteHandler(writeCompleteHandler_);
	tcpConnection->setPeerShutdownHandler(peerShutdownHandler_);
	tcpConnection->setDisconnectedHandler(disconnectedHandler_);

	tcpConnection->setCloseHandler(std::bind(&TcpWorker::onConnectionClosed, this, std::placeholders::_1));
}

bool TcpWorker::needTryAcquireTocken()
{
	if (cntDisableAquireListenToken_ > 0)
	{
		cntDisableAquireListenToken_--;
		return false;
	}
	return true;
}

void TcpWorker::enableListening()
{
	for (auto &acceptor : acceptors_)
	{
		acceptor->enableListening();
	}
}

void TcpWorker::disableListening()
{
	for (auto &acceptor : acceptors_)
	{
		acceptor->disableListening();
	}
}

int TcpWorker::onNewConnection(Socket &&socket, const InetAddr &peerAddr)
{
	TcpConnection *tcpConnection = getTcpConnection();
	if (tcpConnection == nullptr)
	{
		InetAddr addr;
		socket.getLocalAddr(&addr);
		LOG_WARN("tcpConnectionPool has no tcpConnection, connection from %s at server %s will be closed.", 
			peerAddr.toString().c_str(), addr.toString().c_str());
		
		// there is no free connection any more, close it
		return EASYNET_STOP_ACCEPT;
	}

	numCurConnections_++;
	cntDisableAquireListenToken_ = numCurConnections_ - (connectionPoolMaxSize_ * 7) / 8;

	LOG_TRACE("worker[0x%x] accepted connection, socket fd = %d, holds %d connections", 
		worker_, socket.fd(), numCurConnections_);

	// new TcpConnection
	if (0 == tcpConnection->getEstablishmentTime())
	{
		setTcpConntionsHandlers(tcpConnection);
	}

	int acceptNext = EASYNET_ACCEPT_NEXT;
	if (!isSingleTcpWorker()) 
	{
		if (numCurConnections_ >= connectionPoolMaxSize_)
		{
			// server has more than 1 workers and free connections is less than @freeConnectionsLowWater_(total / 8), start load balance
			acceptNext = EASYNET_STOP_ACCEPT; 
		}
		else if (numCurConnections_ >= numConnectionsLoadBalancingLine_)
		{
			acceptNext = EASYNET_SUGGEST_STOP_ACCEPT;
		}
	}

	tcpConnection->reset(std::move(socket), peerAddr);
	if (newConnectionHandler_)
	{
		newConnectionHandler_(*tcpConnection);   // call the application's callback
	}

	return acceptNext;
}

void TcpWorker::onConnectionClosed(TcpConnection &tcpConnection)
{
	freeTcpConnection(&tcpConnection);
	numCurConnections_--;
}
