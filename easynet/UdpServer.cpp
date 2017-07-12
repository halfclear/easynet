// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <cstring>
#include "UdpServer.h"
#include "utils/log.h"

using namespace easynet;

void UdpServer::start()
{
    std::vector<InetAddr> listenAddrs = listenAddrMgr_.getListenAddrs();
    std::vector<Worker*> workers = workerGroup_.getWorkers();

    for (auto worker : workers)
    {
    	for (auto &addr : listenAddrs)
	    {
	    	std::unique_ptr<UdpConnection> udpChannel(new UdpConnection(worker->getLoop()));
	    	if (udpChannel->bind(addr) == 0)
	    	{
	    		LOG_INFO("udp server bind at %s", addr.toString().c_str());
	    	}
	    	else
	    	{
	    		int savedErrno = errno;
	    		LOG_INFO("udp server bind at %s failed, error:%d %s", addr.toString().c_str(), savedErrno, ::strerror(savedErrno));
	    		::exit(1);
	    	}
	    	udpChannel->setMessageHandler(messageHandler_);
	    	udpChannel->setErrorHandler(errorHandler_);
	    	udpChannels_.push_back(std::move(udpChannel));
	    }
    }
    
    workerGroup_.start();
}