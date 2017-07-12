// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _LISTEN_ADDR_MGR_H_
#define _LISTEN_ADDR_MGR_H_

#include <list>
#include <vector>
#include <map>

#include "InetAddr.h"

namespace easynet
{

class ListenAddrMgr
{
public:
	ListenAddrMgr() = default;
	~ListenAddrMgr() = default;

    bool addListenAddr(const InetAddr &addr);
    bool addListenAddr(unsigned short port)
    { return addListenAddr(InetAddr(port)); }
    bool addListenAddr(const std::string &ip, unsigned short port)
    { return addListenAddr(InetAddr(ip, port)); }
    
	std::vector<InetAddr> getListenAddrs() const;

private:
    // key: listen port
	std::map<unsigned short, std::list<InetAddr>> listenAddrs_;
};

}
#endif