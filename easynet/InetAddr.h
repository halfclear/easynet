// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_INET_ADDR_H_
#define _EASYNET_INET_ADDR_H_

#include <netinet/in.h>
#include <string>

namespace easynet{

class InetAddr
{
public:
	InetAddr();
	InetAddr(const std::string &ip, unsigned short port);
	explicit InetAddr(unsigned short port) : InetAddr("*", port) {}
	InetAddr(const struct sockaddr_in& addr) : addr_(addr) {}
	~InetAddr() = default;

	bool operator==(const InetAddr &rhs) {
		return addr_.sin_port == rhs.addr_.sin_port && 
	       addr_.sin_addr.s_addr == rhs.addr_.sin_addr.s_addr;
	}

	std::string ip() const;
	unsigned short port() const;
	std::string toString() const;
	bool isIpWildcard() const { return addr_.sin_addr.s_addr == htons(INADDR_ANY); }
	bool isAddrNone() const { return addr_.sin_port == 0 && isIpWildcard(); }

	struct sockaddr_in& getSockAddr() { return addr_; }
	const struct sockaddr_in& getSockAddr() const { return addr_; }

private:
	struct sockaddr_in addr_;

	void toIp(char *buf, size_t size) const;
};

} // namespace easynet
#endif