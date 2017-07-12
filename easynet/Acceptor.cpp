// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <cstring>
#include "Acceptor.h"
#include "EventLoop.h"
#include "Socket.h"
#include "utils/log.h"

using namespace easynet;

Acceptor::Acceptor(EventLoop *loop, 
	               Socket *listenSocket, 
	               int minAcceptsPerCall, 
	               int maxAcceptsPerCall)
                 : loop_(loop),
                   listenSocket_(listenSocket),
                   channel_(loop, listenSocket_->fd()),
                   listening_(false),
                   minAcceptsPerCall_(minAcceptsPerCall),
                   maxAcceptsPerCall_(maxAcceptsPerCall)
{
	listenSocket->getLocalAddr(&listenAddr_);
	channel_.markListenChannel();
	channel_.setReadHandler(std::bind(&Acceptor::onConnectionArrived, this));
}

void Acceptor::enableListening()
{
	if (!listening_)
	{
		channel_.enableReading();
		listening_ = true;
	}	
}

void Acceptor::disableListening()
{
	if (listening_)
	{
		channel_.disableAll();
		listening_ = false;
	}
}

void Acceptor::onConnectionArrived()
{
	InetAddr peerAddr;
	int acceptedConnections = 0;

	while(acceptedConnections < maxAcceptsPerCall_)
	{	
		int socketFd = listenSocket_->accept(&peerAddr);
		if (socketFd >= 0)
		{
			acceptedConnections++;
			LOG_TRACE("accept connection from %s on server %s, listen socket fd = %d, accepted socket fd = %d, accepted %d connections this round", 
				peerAddr.toString().c_str(), listenAddr_.toString().c_str(), listenSocket_->fd(), socketFd, acceptedConnections);

			Socket socket(socketFd);
			socket.setNoDelay(true);
			socket.setNonBlocking(true);
			socket.setReuseAddr(true);
			socket.setCloseOnExec(true);
			// accept @minAcceptsPerCall_ ~ @maxAcceptsPerCall_ connections per call
			int acceptAgain = newConnectionHandler_(std::move(socket), peerAddr);
			if (acceptAgain == EASYNET_STOP_ACCEPT ||
				((acceptAgain == EASYNET_SUGGEST_STOP_ACCEPT) && acceptedConnections >= minAcceptsPerCall_))
			{
				break;
			}
		}
		else
		{
			// most of the case are all of the connections have been accepted
			int savedErrno = errno;
			if (savedErrno != EAGAIN && savedErrno != EINTR)
			{
				LOG_FATAL("accept error at %s, accept sockef fd = %d, error:%d %s", 
					listenAddr_.toString().c_str(), listenSocket_->fd(), savedErrno, ::strerror(savedErrno)); 
			}
			break;
		}
	}	
}
