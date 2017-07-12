// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <cstring>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#include "InetAddr.h"

using namespace easynet;

InetAddr::InetAddr()
{
	std::memset(&addr_, 0, sizeof(addr_));
}

InetAddr::InetAddr(const std::string &ip, unsigned short port)
{
	std::memset(&addr_, 0, sizeof(addr_));
	addr_.sin_family = AF_INET;
	addr_.sin_addr.s_addr = ip == "*" ? htons(INADDR_ANY) 
	                                : (ip == "localhost" ? ::inet_addr("127.0.0.1")
	                                	: ::inet_addr(ip.c_str()));
	addr_.sin_port = htons(port);
}

std::string InetAddr::ip() const
{
	char buf[32];
	toIp(buf, sizeof(buf));
	return buf;
}

unsigned short InetAddr::port() const
{
	return ntohs(addr_.sin_port);
}

std::string InetAddr::toString() const
{
	char ip[32];
	toIp(ip, sizeof(ip));

	char buf[32];
	unsigned short p = port();
	::snprintf(buf, sizeof(buf), "%s:%u", ip, p);

	return buf;
}

void InetAddr::toIp(char *buf, size_t size) const
{
	::inet_ntop(AF_INET, &addr_.sin_addr, buf, static_cast<socklen_t>(size));
}
