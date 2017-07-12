// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _UDP_CONNECTION_H_
#define _UDP_CONNECTION_H_

#include <string>
#include <functional>
#include <utility>

#include "InetAddr.h"
#include "Socket.h"
#include "Channel.h"
#include "Buffer.h"

namespace easynet
{

class EventLoop;

class UdpConnection
{
public:
	using UdpConnectionHandler = std::function<void (UdpConnection &udpConnection)>;
	using ErrorHandler = std::function<void (UdpConnection &udpConnection, int errNo, const std::string& errMsg)>;

	UdpConnection(EventLoop *loop);
	~UdpConnection() { channel_.disableAll(); }

    int bind(unsigned short port) { return bind(InetAddr(port)); }
    int bind(const std::string &ip, unsigned short port) { return bind(InetAddr(ip, port)); }
	int bind(const InetAddr &addr);

	int connect(const InetAddr &addr);
	int connect(const std::string &ip, unsigned short port) { return connect(InetAddr(ip, port)); }


    ssize_t sendTo(const std::string &msg, const std::string &dstIp, unsigned short dstPort)
    { return sendTo(msg, InetAddr(dstIp, dstPort)); }
    ssize_t sendTo(const std::string &msg, const InetAddr &dstAddr) { return sendTo(msg.c_str(), msg.size(), dstAddr); }
    ssize_t sendTo(const char *buf, size_t len, const InetAddr &dstAddr) { return socket_.sendTo(buf, len, dstAddr); }
	ssize_t sendTo(const char *buf, size_t len, const std::string &dstIp, unsigned short dstPort)
	{ return sendTo(buf, len, InetAddr(dstIp, dstPort)); }

	ssize_t send(const char *buf, size_t len) { return sendTo(buf, len, foreignAddr_); }
	ssize_t send(const std::string &msg) { return send(msg.c_str(), msg.size()); }

	const InetAddr& getListenAddr() const { return listenAddr_; }
	const InetAddr& getPeerAddr() const { return peerAddr_; }
	const InetAddr& getLocalAddr() const { return localAddr_; }
	Buffer& getInputBuffer() { return inputBuffer_; }

	void setMessageHandler(UdpConnectionHandler &&handler) { messageHandler_ = std::move(handler); }
	void setMessageHandler(const UdpConnectionHandler &handler) { setMessageHandler(UdpConnectionHandler(handler)); }

	void setErrorHandler(ErrorHandler &&handler) { errorHandler_ = std::move(handler); }
	void setErrorHandler(const ErrorHandler &handler) { setErrorHandler(ErrorHandler(handler)); }

private:
	void onReadable();
	void onPollError();
	ssize_t readData();

	EventLoop *loop_;
	Socket socket_;
	Channel channel_;

    InetAddr listenAddr_;  // if bind()
    InetAddr foreignAddr_; // if connet()

	InetAddr localAddr_;
	InetAddr peerAddr_;

	Buffer inputBuffer_;

	UdpConnectionHandler messageHandler_;
	ErrorHandler errorHandler_;
};

}

#endif