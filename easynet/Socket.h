// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_SOCKET_H_
#define _EASYNET_SOCKET_H_

#include <sys/socket.h>
#include <string>
#include "InetAddr.h"

#define EASYNET_INVALID_SOCKET   -1

namespace easynet{

// just for TCP socket, for linux
class Socket
{
public:
	enum SocketType
	{
		SOCKET_TCP = 0,
        SOCKET_UDP
	};

	Socket(SocketType socketType = SOCKET_TCP);
	explicit Socket(int socketFd) : socketFd_(socketFd) {}
	Socket(Socket &&rhs) noexcept { socketFd_ = rhs.release(); }
	~Socket() { close(); }
	Socket& operator=(Socket &&rhs) noexcept;

	Socket(const Socket &rhs) = delete;
	Socket& operator=(const Socket &rhs) = delete;
	
	int fd() const { return socketFd_; }
	
	// just for tcp
	void shutdownRead();
	void shutdownWrite();
	void shutdown();

	void close();

	int bind(unsigned short port) { return bind(InetAddr(port)); }
	int bind(const std::string &ip, unsigned short port) { return bind(InetAddr(ip, port)); }
	int bind(const InetAddr &addr)
    { return ::bind(socketFd_, (struct sockaddr*)&(addr.getSockAddr()), sizeof(struct sockaddr_in)); }
	
    // just for tcp
	int listen(int backlog) { return ::listen(socketFd_, backlog); }

	int connect(const std::string &dstIp, unsigned short dstPort) { return connect(InetAddr(dstIp, dstPort)); }
	int connect(const InetAddr &dstAddr)
    { return ::connect(socketFd_, (struct sockaddr*)&(dstAddr.getSockAddr()), sizeof(struct sockaddr_in)); }
	
	// just for tcp. return the accpeted socket fd
	int accept(InetAddr *peerAddr);

	ssize_t send(const char *buf, size_t len, int flag = 0) { return ::send(socketFd_, buf, len, flag); }
	ssize_t recv(char *buf, size_t len, int flag = 0) { return ::recv(socketFd_, buf, len, flag); }

    // just for udp
	ssize_t sendTo(const char *buf, size_t len, const std::string &dstIp, unsigned short dstPort, int flag = 0)
	{ return sendTo(buf, len, InetAddr(dstIp, dstPort), flag); }
	ssize_t sendTo(const char *buf, size_t len, const InetAddr &dstAddr, int flag = 0)
    { return ::sendto(socketFd_, buf, len, flag, (struct sockaddr*)&(dstAddr.getSockAddr()), sizeof(struct sockaddr_in)); }

	// just for udp
	ssize_t recvFrom(char *buf, size_t len, InetAddr *peerAddr, int flag = 0);
	
	int getLocalAddr(InetAddr *localAddr);
	int getPeerAddr(InetAddr *peerAddr);

    // // get how many bytes in the socket's receive buffer
	int getReadableSize(size_t *n) const;

    int setCloseOnExec(bool on);
	int setReuseAddr(bool on);
	int setReusePort(bool on);
	int setNoDelay(bool on);
	int setNonBlocking(bool on);
	int setLinger(bool on, int lingerTime);
	int setRecvBuf(int size);
	int setSendBuf(int size);
	int setRecvErr(bool on);

	int getSocketError();
	bool isSelfConnect();

private:
	void reset(int socketFd);
	int release();
	void shutdown(int how);
	int socketFd_;
};

}

#endif
