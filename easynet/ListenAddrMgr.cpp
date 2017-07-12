// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include "ListenAddrMgr.h"

using namespace easynet;

bool ListenAddrMgr::addListenAddr(const InetAddr &addr)
{
	unsigned short port = addr.port();
	if (port == 0)
	{
		return false;
	}

	auto it = listenAddrs_.find(port);
	if (it == listenAddrs_.end())
	{
		listenAddrs_[port] = std::list<InetAddr>();
		listenAddrs_[port].push_back(addr);
		return true;
	}

	auto &container = it->second;
	for (auto jt = container.begin(); jt != container.end(); ++jt)
	{
		if (jt->ip() == addr.ip())
		{
			return false;
		}
	}

	if (addr.isIpWildcard())
	{
		container.push_front(addr);
	}
	else
	{
		container.push_back(addr);
	}

	return true;
}

std::vector<InetAddr> ListenAddrMgr::getListenAddrs() const
{
	std::vector<InetAddr> v;
    for (auto it = listenAddrs_.begin(); it != listenAddrs_.end(); ++it)
	{
		auto &container = it->second;
		for (auto &listenAddr : container)
		{
			v.push_back(listenAddr);
		//	if (listenAddr.isIpWildcard())
		//	{
		//		break;
		//	}
		}
	}
	return std::move(v);
}
