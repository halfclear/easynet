// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <string>
#include <memory>
#include <cstring>

#include "Connector.h"
#include "EventLoop.h"
#include "Socket.h"
#include "Timer.h"
#include "utils/log.h"

using namespace easynet;

Connector::Connector(EventLoop *loop)
               : loop_(loop),
                 socket_(EASYNET_INVALID_SOCKET),
                 channel_(loop),
                 toStop_(false)
{}

// can be called in other thread
void Connector::start(const InetAddr &dstAddr)
{
	toStop_ = false;
	dstAddr_ = dstAddr;
	loop_->wakeupAndRun(std::bind(&Connector::connectInLoop, this));
}

// can be called in other thread
void Connector::stop()
{
	toStop_ = true;
	loop_->wakeupAndRun(std::bind(&Connector::close, this));
}

void Connector::connect(const InetAddr &dstAddr)
{
	dstAddr_ = dstAddr;
	connectInLoop();
}

void Connector::connectInLoop()
{
	close(); // close() last connecting first

	socket_ = Socket();
	socket_.setNoDelay(true);
	socket_.setNonBlocking(true);
	socket_.setReuseAddr(true);
	socket_.setCloseOnExec(true);

	if (!localAddr_.isAddrNone())
	{
		if (socket_.bind(localAddr_) < 0)
		{
			int errNo = errno;
			std::string errMsg = ::strerror(errNo);
			LOG_WARN("socket bind at %s failed, socket fd = %d, error:%d %s", 
				localAddr_.toString().c_str(), socket_.fd(), errNo, errMsg.c_str())

			onError(errNo, errMsg);
			return;
		}
	}

	int ret = socket_.connect(dstAddr_);
	int errNo = (ret == 0) ? 0 : errno;
	std::string errMsg = (errNo == 0) ? "" : ::strerror(errNo);

	LOG_TRACE("socket start connecting to %s, socket fd = %d, errno:%d %s ", 
		dstAddr_.toString().c_str(), socket_.fd(), errNo, errMsg.c_str());
	
	switch (errNo)
	{
	case 0:
	case EINPROGRESS:
	case EINTR:
	case EISCONN:

		setChannelHandlers();
		break;

	case EAGAIN:

		LOG_WARN("socket connect to %s error, socket fd = %d, try again later. error:%d %s", 
			dstAddr_.toString().c_str(), socket_.fd(), errNo, errMsg.c_str());
		break;

	case EADDRINUSE:
	case EADDRNOTAVAIL:
	case ECONNREFUSED:
	case ENETUNREACH:
	case EACCES:
	case EPERM:
	case EAFNOSUPPORT:
	case EALREADY:
	case EBADF:
	case EFAULT:
	case ENOTSOCK:
	default:

		LOG_WARN("socket connect to %s error, socket fd = %d, error:%d %s", 
			dstAddr_.toString().c_str(), socket_.fd(), errNo, errMsg.c_str());		
		onError(errNo, errMsg);
		break;
	}
}

void Connector::setChannelHandlers()
{
	channel_.resetFd(socket_.fd());
	channel_.setWriteHandler(std::bind(&Connector::onWritable, this));
	channel_.setErrorHandler(std::bind(&Connector::onPollError, this));
	channel_.enableWriting();
}

void Connector::onWritable()
{
	if (toStop_)
	{
		close();
		return;
	}

	int errNo = socket_.getSocketError();
    if (errNo)
    {
    	std::string errMsg = ::strerror(errNo);
		LOG_WARN("socket connect to %s error, socket fd = %d, error:%d %s ", 
			dstAddr_.toString().c_str(), socket_.fd(), errNo, errMsg.c_str());
		onError(errNo, errMsg);
    }
    else if (socket_.isSelfConnect())
    {
		LOG_WARN("socket connect to %s error:self connect. socket fd = %d", 
			dstAddr_.toString().c_str(), socket_.fd());
		onError(errNo, "self connect");
    }
    else
    {
    	LOG_TRACE("socket connected to %s, socket fd = %d", 
		     dstAddr_.toString().c_str(), socket_.fd());

    	channel_.disableAll();
    	if (connectedHandler_)
    	{
    		connectedHandler_(std::move(socket_));
    	}
    	else
    	{
    		socket_.close();
    	} 
    }
}

void Connector::onPollError()
{
	int errNo = socket_.getSocketError();
	std::string errMsg = ::strerror(errNo);
	LOG_WARN("socket connect to %s error, socket fd = %d, error:%d %s ", 
			dstAddr_.toString().c_str(), socket_.fd(), errNo, errMsg.c_str());

	onError(errNo, errMsg);
}

// error occurred during connecting, such as server refused
void Connector::onError(int errNo, const std::string &errMsg)
{
	close();
	if (errorHandler_)
	{
		errorHandler_(errNo, errMsg);
	}
}

void Connector::close()
{
	channel_.disableAll();
	socket_.close();
	toStop_ = false;
}
