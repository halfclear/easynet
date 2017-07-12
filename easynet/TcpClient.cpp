// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include "Socket.h"
#include "Buffer.h"
#include "Channel.h"
#include "Connector.h"
#include "EventLoop.h"
#include "TcpClient.h"
#include "utils/log.h"

using namespace easynet;

namespace
{

const int kInitNextRetryTimeMillis = 2 * 1000;
const double kRetryTimeStep  = 1.5;

}

TcpClient::TcpClient(EventLoop *loop)
		       : loop_(loop),
		         nextRetryTimeMillis_(kInitNextRetryTimeMillis),
		         totalTriedTimeMillis_(0),
		         maxRetryTimeMillis_(0),
		         retryTimer_(nullptr),
		         connecting_(false),
		         disconnected_(false),
		         connector_(loop_),
		         tcpConnection_(loop_)   
{
	connector_.setConnectedHandler(std::bind(&TcpClient::onConnected, this, std::placeholders::_1));
	connector_.setErrorHandler(std::bind(&TcpClient::onConnectingError, 
		this, std::placeholders::_1, std::placeholders::_2));
}

void TcpClient::connect(const InetAddr &dstAddr, int timeoutSecs)
{
	if (!connecting())
	{
		connecting_ = true;
		dstAddr_ = dstAddr;
		maxRetryTimeMillis_   = timeoutSecs * 1000;
		totalTriedTimeMillis_ = 0;
		nextRetryTimeMillis_ = kInitNextRetryTimeMillis;

		loop_->wakeupAndRun(std::bind(&TcpClient::connectInLoop, this));
    }
}

// stop connecting...
void TcpClient::stop()
{
	if (connecting())
	{
	    connector_.stop();
	    loop_->wakeupAndRun(std::bind(&TcpClient::stopInLoop, this));
    }
}

void TcpClient::connectInLoop()
{
    connector_.bind(localAddr_);
    setRetryTimer();
	connector_.connect(dstAddr_);	
}

void TcpClient::stopInLoop()
{
	onConnectingFinished();
}

void TcpClient::setRetryTimer()
{
	if (maxRetryTimeMillis_ <= 0)
	{
		return;
	}

	if (nextRetryTimeMillis_  > maxRetryTimeMillis_)
	{
		nextRetryTimeMillis_ = maxRetryTimeMillis_;
	}

	retryTimer_ = loop_->runAfter(nextRetryTimeMillis_, std::bind(&TcpClient::onRetry, this));
}

void TcpClient::onConnected(Socket &&socket)
{
	tcpConnection_.reset(std::move(socket), dstAddr_);
	tcpConnection_.setReadHandler(readHandler_);
	tcpConnection_.setWriteCompleteHandler(writeCompleteHandler_);
	tcpConnection_.setPeerShutdownHandler(peerShutdownHandler_);
	tcpConnection_.setDisconnectedHandler(disconnectedHandler_);
	localAddr_ = tcpConnection_.getLocalAddr();

	LOG_TRACE("socket fd %d connected to %s on %s", socket.fd(),
		tcpConnection_.getPeerAddr().toString().c_str(), 
		tcpConnection_.getLocalAddr().toString().c_str());

	onConnectingFinished();
	disconnected_ = false;

	if (connectedHandler_)
	{
		connectedHandler_(tcpConnection_);
	}
}

void TcpClient::onRetry()
{
	totalTriedTimeMillis_ += nextRetryTimeMillis_;
	nextRetryTimeMillis_ *= kRetryTimeStep;

	if (totalTriedTimeMillis_ + 500 >= maxRetryTimeMillis_) // if will be timedout after 500ms, don't reconnect 
	{
		LOG_TRACE("connect to %s timedout, totally tried %u millis", dstAddr_.toString().c_str(), totalTriedTimeMillis_);

		onConnectingFinished();
		if (connectingTimeoutHandler_)
		{
			connectingTimeoutHandler_();
		}

		return;
	}

	reconnect();
}

void TcpClient::reconnect()
{
	LOG_TRACE("reconnect to %s, totally tried %u millis", 
		dstAddr_.toString().c_str(), totalTriedTimeMillis_);
    restartRetryTimer();
    connector_.connect(dstAddr_);
}

void TcpClient::restartRetryTimer()
{
	if (nextRetryTimeMillis_ + totalTriedTimeMillis_ > maxRetryTimeMillis_)
	{
		nextRetryTimeMillis_ = maxRetryTimeMillis_ - totalTriedTimeMillis_;
	}

	retryTimer_->restart(nextRetryTimeMillis_);
}

void TcpClient::onConnectingError(int errNo, const std::string &errMsg)
{
	onConnectingFinished();
	if (connectingErrorHandler_)
	{
		connectingErrorHandler_(errNo, errMsg);
	}
}

void TcpClient::onConnectingFinished()
{
    connector_.close();
	deleteRetryTimer();
	connecting_ = false;
}

void TcpClient::deleteRetryTimer()
{
	if (retryTimer_)
	{
		retryTimer_->cancel();
		retryTimer_ = nullptr;
	}
}

void TcpClient::disconnect()
{
	tcpConnection_.close();
	disconnected_ = true;
}

void TcpClient::close()
{
	onConnectingFinished();
	disconnect();
}
