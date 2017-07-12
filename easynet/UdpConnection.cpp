// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <cstring>

#include "UdpConnection.h"
#include "utils/log.h"

using namespace easynet;

UdpConnection::UdpConnection(EventLoop *loop)
                   : loop_(loop),
	                 socket_(Socket::SOCKET_UDP),
	                 channel_(loop_, socket_.fd())
{
	socket_.setNonBlocking(true);
	socket_.setCloseOnExec(true);
	socket_.setReuseAddr(true);
	socket_.setRecvErr(true);
	channel_.setReadHandler(std::bind(&UdpConnection::onReadable, this));
	channel_.setErrorHandler(std::bind(&UdpConnection::onPollError, this));
	channel_.enableReading();
}

int UdpConnection::bind(const InetAddr &addr) 
{
	if (socket_.bind(addr) == 0)
	{
		listenAddr_ = addr;
		return 0;
	}
	else
	{
		int savedErrno = errno;
		LOG_ERROR("udp socket bind at %s failed, socket fd = %d, error:%d %s", 
			addr.toString().c_str(), socket_.fd(), savedErrno, ::strerror(savedErrno));
	}
	return -1;
}
	

int UdpConnection::connect(const InetAddr &addr)
{
	if (socket_.connect(addr) == 0)
	{
		foreignAddr_ = addr;
		return 0;
	}
	else
	{
		int savedErrno = errno;
		LOG_ERROR("udp socket connect to %s failed, socket fd = %d, error:%d %s", 
			addr.toString().c_str(), socket_.fd(), savedErrno, ::strerror(savedErrno))
	}
	return -1;
}

void UdpConnection::onReadable()
{
	socket_.getLocalAddr(&localAddr_);
	while (readData() > 0)
	{
		if (messageHandler_)
		{
			messageHandler_(*this);
		}
	}
}

ssize_t UdpConnection::readData()
{
    size_t n = 0;
	// get how many bytes in the socket's receive buffer
	if (socket_.getReadableSize(&n) < 0)
	{
		LOG_ERROR("get udp socket's readable bytes error, socket fd = %d", socket_.fd());
		return -1;
	}

	inputBuffer_.expand(n);
	while (true)
	{
		ssize_t len = socket_.recvFrom(inputBuffer_.end(), n, &peerAddr_);
		LOG_TRACE("to recvFrom() %u bytes from udp socket, socket fd = %d, received %d bytes from %s at %s", 
			n, socket_.fd(), len, peerAddr_.toString().c_str(), localAddr_.toString().c_str());
		if (len >= 0)
		{
			inputBuffer_.addSize(len);
			return len;
		}
		else if (len < 0)
		{
			int savedErrno = errno;
			if (savedErrno == EINTR)
			{
				continue;
			}
			else if (savedErrno == EAGAIN)
			{
				return len;
			}
			else
			{
				LOG_WARN("udp socket recvFrom() error, socket fd = %d, error:%d %s", 
					socket_.fd(), savedErrno, ::strerror(savedErrno));
				return len;
			}
		}
	}
}

void UdpConnection::onPollError()
{
    int errNo = socket_.getSocketError();
	std::string errMsg = ::strerror(errNo);
	LOG_WARN("udp socket %s->%s error occured, socket fd = %d, error:%d %s ", 
			localAddr_.toString().c_str(), peerAddr_.toString().c_str(), socket_.fd(), errNo, errMsg.c_str());

    if (errorHandler_)
    {
    	errorHandler_(*this, errNo, errMsg);
    }
}
