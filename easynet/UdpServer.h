// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _UDP_SERVER_H_
#define _UDP_SERVER_H_

#include "WorkerGroup.h"
#include "UdpConnection.h"
#include "ListenAddrMgr.h"

namespace easynet
{

class UdpServer
{
public:
	using UdpConnectionHandler = UdpConnection::UdpConnectionHandler;
	using ErrorHandler = UdpConnection::ErrorHandler;

	UdpServer(unsigned short port) { addListenAddr(port); }
	~UdpServer() { stop(); }

	void start();
	void stop() { workerGroup_.stop(); }
	
	void setMessageHandler(UdpConnectionHandler &&handler) { messageHandler_ = std::move(handler); }
	void setMessageHandler(const UdpConnectionHandler &handler) { setMessageHandler(UdpConnectionHandler(handler)); }

	void setErrorHandler(ErrorHandler &&handler) { errorHandler_ = std::move(handler); }
	void setErrorHandler(const ErrorHandler &handler) { setErrorHandler(ErrorHandler(handler)); }

	bool addListenAddr(unsigned short port) 
    { return listenAddrMgr_.addListenAddr(InetAddr(port)); }
    bool addListenAddr(const std::string &ip, unsigned short port) 
    { return listenAddrMgr_.addListenAddr(InetAddr(ip, port)); }
    bool addListenAddr(const InetAddr &addr) { return listenAddrMgr_.addListenAddr(addr); }

private:

    WorkerGroup workerGroup_;
    ListenAddrMgr listenAddrMgr_;
    std::vector<std::unique_ptr<UdpConnection>> udpChannels_;

	UdpConnectionHandler messageHandler_;
	ErrorHandler errorHandler_;	
};

}

#endif