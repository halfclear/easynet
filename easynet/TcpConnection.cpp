// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <functional>
#include <cstring>

#include "TcpConnection.h"
#include "EventLoop.h"
#include "utils/log.h"

using namespace easynet;

TcpConnection::TcpConnection(EventLoop *loop)
                   : loop_(loop),
                     socket_(EASYNET_INVALID_SOCKET),
                     channel_(loop),
                     closed_(true),
                     establishedTimeMillis_(0),
                     data_(nullptr),
                     errNo_(0),
                     idleMillis_(0),
	                 idleTimer_(nullptr)              
{
	setChannelHandlers();
	channel_.setEdgeTriggerMode();
}

TcpConnection::TcpConnection(EventLoop *loop, Socket &&socket)
                   : loop_(loop),
                     socket_(std::move(socket)),
					 channel_(loop, socket_.fd()),
					 closed_(true),
                     establishedTimeMillis_(loop_->now()),
                     data_(nullptr),
                     errNo_(0),
                     idleMillis_(0),
	                 idleTimer_(nullptr) 
{
	socket_.getLocalAddr(&localAddr_);
	socket_.getPeerAddr(&peerAddr_);
	setChannelHandlers();
	channel_.setEdgeTriggerMode();
	channel_.enableReading();
}

void TcpConnection::setChannelHandlers()
{	
	channel_.setReadHandler(std::bind(&TcpConnection::onReadable, this));
	channel_.setWriteHandler(std::bind(&TcpConnection::onWritable, this));
	channel_.setPeerShutdownHandler(std::bind(&TcpConnection::onPeerShutdown, this));
	channel_.setErrorHandler(std::bind(&TcpConnection::onPollError, this));	
}

void TcpConnection::reset(Socket &&socket, const InetAddr &peerAddr)
{
	clear();
	closed_ = false;

	establishedTimeMillis_ = loop_->now();
	socket_ = std::move(socket);
	socket_.getLocalAddr(&localAddr_);
	peerAddr_ = peerAddr;
	channel_.resetFd(socket_.fd());
	channel_.enableReading();
}

void TcpConnection::onReadable()
{
	if (recvData() > 0)
	{
		readHandler_(*this);
	}

	// n is 0 only when peer closed or peer shutdown write, 
	// we don't need to process this case
}

void TcpConnection::onPeerShutdown()
{
	LOG_TRACE("peer shutdown, TcpConnection:%s->%s, socket fd = %d", 
		peerAddr_.toString().c_str(), localAddr_.toString().c_str(), socket_.fd());

	// peer closed socket, or peer shutdown write. we can't distinguish closing from shutingdown
	channel_.disableRdHup();
	// read data first
	onReadable();
	disableReceiving();

	if (!closed() && peerShutdownHandler_)
	{
		peerShutdownHandler_(*this);
	}
}

void TcpConnection::onWritable()
{
	LOG_TRACE("socket writable, TcpConnection:%s->%s, socket fd = %d", 
		localAddr_.toString().c_str(), peerAddr_.toString().c_str(), socket_.fd());

	// most of the case is the first time when adding this socketfd to epoll.
	if (outputBuffer_.empty() || !channel_.writing())
	{
		return;
	}

	ssize_t nWrote = sendData(outputBuffer_.data(), outputBuffer_.size());
	if (nWrote > 0)
	{	
		outputBuffer_.deleteBegin(nWrote);
		if (outputBuffer_.empty())
		{
			channel_.disableWriting();
			if (writeCompleteHandler_)
			{
			    writeCompleteHandler_(*this);
			}
		}
	}
}

void TcpConnection::onPollError()
{
	errNo_ = socket_.getSocketError();
	errMsg_ = ::strerror(errNo_);
	LOG_WARN("socket poll error(disconnected), TcpConnection:%s->%s, socket fd = %d error:%d %s", 
		peerAddr_.toString().c_str(), localAddr_.toString().c_str(), socket_.fd(), errNo_, errMsg_.c_str());

	onDisconnected();
}

ssize_t TcpConnection::recvData()
{
	size_t n = 0;
	// get how many bytes in the socket's receive buffer
	if (socket_.getReadableSize(&n) < 0)
	{
		LOG_ERROR("get socket's readable bytes error, TcpConnection:%s->%s, socket fd = %d", 
			peerAddr_.toString().c_str(), localAddr_.toString().c_str(), socket_.fd());
		return -1;
	}

	if (n <= 0)
	{
		return n;
	}

	inputBuffer_.expand(n);
	while (true)
	{
		ssize_t len = socket_.recv(inputBuffer_.end(), n);
		LOG_TRACE("to read %u bytes from socket, readed %d bytes, TcpConnection:%s->%s, socket fd = %d", 
		    n, len, peerAddr_.toString().c_str(), localAddr_.toString().c_str(), socket_.fd());

		if (len >= 0)
		{
			updateIdleTimer();
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
				errNo_ = savedErrno;
			    errMsg_ = ::strerror(savedErrno);
			    LOG_ERROR("socket read error, TcpConnection:%s->%s, socket fd = %d, error:%d %s", 
				    peerAddr_.toString().c_str(), localAddr_.toString().c_str(), 
				    socket_.fd(), errNo_, errMsg_.c_str());

			    onDisconnected();
				return len;
			}
		}
	}
}

void TcpConnection::send(const char *data, size_t len)
{
	ssize_t nWrote = 0;
	size_t remaining = len;

	if (!channel_.writing() && outputBuffer_.empty())
	{
		nWrote = sendData(data, len);
		if (nWrote >= 0)
		{
			remaining = len - nWrote;
			if (remaining == 0 && writeCompleteHandler_)
			{
				writeCompleteHandler_(*this);
			}
		}
		else
		{
			return;
		}
	}

	if (remaining > 0)
	{
		outputBuffer_.append(data + nWrote, remaining);
		if (!channel_.writing())
		{
			channel_.enableWriting();
		}
	}
}

ssize_t TcpConnection::sendData(const char *buf, size_t len)
{
	ssize_t nWrote = 0;
	while (true)
	{
		nWrote = socket_.send(buf, len);
		LOG_TRACE("to send %u bytes to socket, %d bytes sent, TcpConnection:%s->%s, socket fd = %d", 
		    len, nWrote, localAddr_.toString().c_str(), peerAddr_.toString().c_str(), socket_.fd());

		if (nWrote >= 0)
		{
			break;
		}
		else  // nWrote < 0
		{
			int savedErrno = errno;
			if (savedErrno == EINTR)
			{
				continue;
			}
			else if (savedErrno == EAGAIN)
			{
				break;
			}
			else
			{
				errNo_ = savedErrno;
				errMsg_ = ::strerror(savedErrno);
				LOG_ERROR("socket send error, TcpConnection:%s->%s, socket fd = %d error:%d %s", 
					localAddr_.toString().c_str(), peerAddr_.toString().c_str(), socket_.fd(), errNo_, errMsg_.c_str());
				onDisconnected();
				break;
			}
		}
	}

	return nWrote;
}

void TcpConnection::onDisconnected()
{
	channel_.disableAll();
	if (disconnectedHandler_)
	{
		disconnectedHandler_(*this);
	}

	close();
}

void TcpConnection::shutdown()
{
	channel_.disableAll();
	socket_.shutdownWrite();
}

void TcpConnection::shutdownRead()
{
	channel_.disableRdHup();
	// will trigger EPOLLIN+EPOLLRDHUP in the @socket_ itself
	socket_.shutdownRead();
}

void TcpConnection::shutdownWrite()
{
	channel_.disableWriting();
	socket_.shutdownWrite();
}

void TcpConnection::close()
{
	if (!closed())
	{
		clear();
		if (closeHandler_)
		{
			closeHandler_(*this);  // call framework's callback, to free this object
		}
	}
}

void TcpConnection::clear()
{
	errNo_ = 0;
    errMsg_.clear();
	channel_.clear();	
	socket_.close();
	data_ = nullptr;
	inputBuffer_.clear();
	outputBuffer_.clear();
	closed_ = true;
	idleMillis_ = 0;
	closeIdleTimer();
}

void TcpConnection::setIdleHandler(int idleSecs, TimerHandler &&handler)
{
    if (idleSecs < 0)
    {
    	return;
    }
    idleMillis_ = idleSecs * 1000;
    closeIdleTimer();
    idleTimer_ = loop_->addIdleTimer(idleMillis_, std::move(handler));
}

void TcpConnection::closeIdleTimer()
{
    if (idleTimer_)
    {
    	idleTimer_->cancel();
    	idleTimer_ = nullptr;
    }
}

void TcpConnection::updateIdleTimer()
{
    if (idleTimer_)
    {
    	idleTimer_->restart(idleMillis_);
    }
}
