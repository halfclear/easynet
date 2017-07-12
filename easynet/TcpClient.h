// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_TCP_CLIENT_H_
#define _EASYNET_TCP_CLIENT_H_

#include <string>
#include <memory>
#include <utility>

#include "TcpConnection.h"
#include "Connector.h"
#include "Timer.h"
#include "InetAddr.h"

namespace easynet
{

class EventLoop;

class TcpClient
{
public:
	using TcpConnectionHandler = TcpConnection::TcpConnectionHandler;
	using ErrorHandler = Connector::ErrorHandler;
	using TimerHandler = Timer::TimerHandler;

	TcpClient(EventLoop *loop);
	~TcpClient() { close(); }

	TcpClient(const TcpClient &rhs) = delete;
	TcpClient& operator=(const TcpClient &rhs) = delete;

	EventLoop* getLoop() const { return loop_; }
	TcpConnection& getTcpConnection() { return tcpConnection_; }

    void bind(unsigned short port) { bind(InetAddr(port)); }
    void bind(const std::string &ip, unsigned short port) { bind(InetAddr(ip, port)); }
	void bind(const InetAddr &addr) { localAddr_ = addr; }

    void connect(const InetAddr &dstAddr, int timeoutSecs = 0);
	void connect(const std::string &dstIp, unsigned short dstPort, int timeoutSecs = 0) // can be called in other thread
	{ connect(InetAddr(dstIp, dstPort), timeoutSecs); }
	
	void stop(); // to stop connecting, can be called in other thread
	void disconnect(); // to close this connection
	void close();

	bool connecting() const { return connecting_; }
	bool disconnected() const { return disconnected_; }

	const InetAddr& getLocalAddr() const { return localAddr_; }
	const InetAddr& getDstAddr() const { return dstAddr_; }
	
	void setConnectedHandler(TcpConnectionHandler &&handler)      { connectedHandler_ = std::move(handler); }
	void setConnectedHandler(const TcpConnectionHandler &handler) { setConnectedHandler(TcpConnectionHandler(handler)); }

	void setConnectingErrorHandler(ErrorHandler &&handler)      { connectingErrorHandler_ = std::move(handler); }
	void setConnectingErrorHandler(const ErrorHandler &handler) { setConnectingErrorHandler(ErrorHandler(handler)); }

	void setConnectingTimeoutHandler(TimerHandler &&handler)       { connectingTimeoutHandler_ = std::move(handler); }
	void setConnectingTimeoutHandler(const TimerHandler &&handler) { setConnectingTimeoutHandler(TimerHandler(handler)); }

	void setReadHandler(TcpConnectionHandler &&handler)      { readHandler_ = std::move(handler); }
	void setReadHandler(const TcpConnectionHandler &handler) { setReadHandler(TcpConnectionHandler(handler)); }

	void setWriteCompleteHandler(TcpConnectionHandler &&handler)      { writeCompleteHandler_ = std::move(handler); }
	void setWriteCompleteHandler(const TcpConnectionHandler &handler) { setWriteCompleteHandler(TcpConnectionHandler(handler)); }

	void setPeerShutdownHandler(TcpConnectionHandler &&handler)      { peerShutdownHandler_ = std::move(handler); }
	void setPeerShutdownHandler(const TcpConnectionHandler &handler) { setPeerShutdownHandler(TcpConnectionHandler(handler)); }

	void setDisconnectedHandler(TcpConnectionHandler &&handler)      { disconnectedHandler_ = std::move(handler); }
	void setDisconnectedHandler(const TcpConnectionHandler &handler) { setDisconnectedHandler(TcpConnectionHandler(handler)); }

private:
	void connectInLoop();
	void stopInLoop();
	void onConnected(Socket &&socket);
	void onConnectingError(int errNo, const std::string &errMsg);
	void onConnectingFinished();
	void onRetry();

    void reconnect();
	void setRetryTimer();
	void restartRetryTimer();
	void deleteRetryTimer();

	EventLoop *loop_;
	
	InetAddr localAddr_;
	InetAddr dstAddr_;

	int64_t nextRetryTimeMillis_;
	int64_t totalTriedTimeMillis_;
	int64_t maxRetryTimeMillis_;
	Timer   *retryTimer_;
	bool connecting_;
	bool disconnected_;

	Connector connector_;
	TcpConnection tcpConnection_;

	TcpConnectionHandler connectedHandler_; // application's callback
	ErrorHandler connectingErrorHandler_;     // application's callback
	TimerHandler connectingTimeoutHandler_;   // application's callback

	TcpConnectionHandler readHandler_;            // application's callback
	TcpConnectionHandler writeCompleteHandler_;   // application's callback
	TcpConnectionHandler peerShutdownHandler_;    // application's callback
	TcpConnectionHandler disconnectedHandler_;    // application's callback

};

} // namespace easynet

#endif
