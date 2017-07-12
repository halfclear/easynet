// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_CONNECTOR_H_
#define _EASYNET_CONNECTOR_H_

#include <string>
#include <memory>
#include <utility>

#include "Socket.h"
#include "Channel.h"
#include "InetAddr.h"

namespace easynet
{

class EventLoop;
class Timer;

class Connector
{
public:
	using ConnectedHandler = std::function<void (Socket &&socket)>;
	using ErrorHandler = std::function<void (int, const std::string&)>;

	Connector(EventLoop *loop);
	~Connector() { close(); }

	Connector(const Connector &rhs) = delete;
	Connector& operator=(const Connector &rhs) = delete;

	EventLoop* getLoop() const { return loop_; }

    // start connecting. can be called in other thread
	void start(const std::string &dstIp, unsigned short dstPort) { start(InetAddr(dstIp, dstPort));} 
	void start(const InetAddr &dstAddr);
	void stop();  // can be called in other thread
	void close();

	// start connecting. can only be called in event loop
	void connect(const std::string &dstIp, unsigned short dstPort) { connect(InetAddr(dstIp, dstPort)); }
	void connect(const InetAddr &dstAddr); 
	
    void bind(unsigned short port) { bind(InetAddr(port)); }
    void bind(const std::string &ip, unsigned short port) { bind(InetAddr(ip, port)); }
	void bind(const InetAddr &addr) { localAddr_ = addr; }

	void setConnectedHandler(ConnectedHandler &&handler) { connectedHandler_ = std::move(handler); }
	void setConnectedHandler(const ConnectedHandler &handler) { setConnectedHandler(ConnectedHandler(handler)); }

	void setErrorHandler(ErrorHandler &&handler) { errorHandler_ = std::move(handler); }
	void setErrorHandler(const ErrorHandler &handler) { setErrorHandler(ErrorHandler(handler)); }

private:	
	void setChannelHandlers();
	void connectInLoop(); 
	
	void onWritable();
	void onPollError();
	void onError(int errNo, const std::string &errMsg);

	EventLoop *loop_;
	Socket socket_;
	Channel channel_;

    InetAddr localAddr_;
	InetAddr dstAddr_;

    bool toStop_;
	bool stopped_;

	ConnectedHandler connectedHandler_; // easynet framework's callback
	ErrorHandler errorHandler_;              // easynet framework's callback	
};

} // namespace easynet

#endif
