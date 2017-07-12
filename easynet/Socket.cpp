// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cstring>

#include "Socket.h"
#include "utils/log.h"

using std::string;
using namespace easynet;

Socket::Socket(SocketType socketType)
{

	socketFd_ = (socketType == SOCKET_TCP) ? ::socket(AF_INET, SOCK_STREAM, 0) 
	                : ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socketFd_ == EASYNET_INVALID_SOCKET)
	{
		LOG_FATAL("socket create failed!");
	}
}

Socket& Socket::operator=(Socket &&rhs) noexcept
{
	if (this != &rhs)
	{
		reset(rhs.release());
	}

	return *this;
}

void Socket::reset(int socketFd)
{
	close();
	socketFd_ = socketFd;
}

int Socket::release()
{
	int socketFd = socketFd_;
	socketFd_ = EASYNET_INVALID_SOCKET;
	return socketFd;
}

void Socket::shutdownRead()
{
	shutdown(SHUT_RD);
}

void Socket::shutdownWrite()
{
	shutdown(SHUT_WR);
}

void Socket::shutdown()
{
	shutdown(SHUT_RDWR);
}

void Socket::shutdown(int how)
{
	if (socketFd_ != EASYNET_INVALID_SOCKET)
	{
		::shutdown(socketFd_, how);
	}
}

void Socket::close()
{
	if (socketFd_ != EASYNET_INVALID_SOCKET)
	{
		::close(socketFd_);
		socketFd_ = EASYNET_INVALID_SOCKET;
	}
}

int Socket::accept(InetAddr *peerAddr)
{
	socklen_t len = sizeof(struct sockaddr_in);

	int sockfd = ::accept(socketFd_, (struct sockaddr*)&(peerAddr->getSockAddr()), &len);
	if (sockfd < 0)
	{
		int savedErrno = errno;
		switch (savedErrno)
		{
			case EAGAIN:
				break;

			case EMFILE:
			case ECONNABORTED:
			case EINTR:
			case EPROTO:
			case EPERM:
			case EBADF:
			case EFAULT:
			case EINVAL:
			case ENFILE:
			case ENOBUFS:
			case ENOMEM:
			case ENOTSOCK:
			case EOPNOTSUPP:
			default:
				LOG_FATAL("socket accept() error, errno:%d %s", savedErrno, ::strerror(savedErrno));
				break;
		}
	}
	
	return sockfd;
}

ssize_t Socket::recvFrom(char *buf, size_t len, InetAddr *peerAddr, int flag)
{
	socklen_t n = sizeof(struct sockaddr_in);
	return ::recvfrom(socketFd_, buf, len, flag, (struct sockaddr*)&(peerAddr->getSockAddr()), &n);
}

int Socket::getLocalAddr(InetAddr *localAddr)
{
	socklen_t len = sizeof(struct sockaddr_in);
	return ::getsockname(socketFd_, (struct sockaddr*)&(localAddr->getSockAddr()), &len);
}

int Socket::getPeerAddr(InetAddr *peerAddr)
{
	socklen_t len = sizeof(struct sockaddr_in);
	return ::getpeername(socketFd_, (struct sockaddr*)&(peerAddr->getSockAddr()), &len);
}

// get how many bytes in the socket's receive buffer
int Socket::getReadableSize(size_t *n) const
{
	if (::ioctl(socketFd_, FIONREAD, n) < 0)
	{
		int savedErrno = errno;
		LOG_ERROR("ioctl(socketFd_, FIONREAD, n) error, error:%d %s", savedErrno, ::strerror(savedErrno));
		return -1;
	}
	return 0;
}

int Socket::setCloseOnExec(bool on)
{
	int flags = ::fcntl(socketFd_, F_GETFD, 0);
	if (on)
		flags |= FD_CLOEXEC;
	else
		flags &= ~FD_CLOEXEC;

	return ::fcntl(socketFd_, F_SETFD, flags);
}

int Socket::setReuseAddr(bool on)
{
	int optval = on ? 1 : 0;
	return ::setsockopt(socketFd_, SOL_SOCKET, SO_REUSEADDR,
	             &optval, static_cast<socklen_t>(sizeof optval));
}

int Socket::setReusePort(bool on)
{
#ifdef SO_REUSEPORT
	int optval = on ? 1 : 0;
	return ::setsockopt(socketFd_, SOL_SOCKET, SO_REUSEPORT,
	             &optval, static_cast<socklen_t>(sizeof optval));
#else
	LOG_ERROR("SO_REUSEPORT not supported");
    return -1;
#endif
}

int Socket::setNoDelay(bool on)
{
	int optval = on ? 1 : 0;
	return ::setsockopt(socketFd_, IPPROTO_TCP, TCP_NODELAY,
				   &optval, static_cast<socklen_t>(sizeof optval));
}

int Socket::setNonBlocking(bool on)
{
	int flags = ::fcntl(socketFd_, F_GETFL, 0);
	if (on)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;

	return ::fcntl(socketFd_, F_SETFL, flags);
}

int Socket::setLinger(bool on, int lingerTime)
{
	struct linger ml = {on ? 1 : 0, lingerTime};
	return ::setsockopt(socketFd_, SOL_SOCKET, SO_LINGER, (const char*)&ml, sizeof(struct linger));
}

int Socket::setRecvBuf(int size)
{
	return ::setsockopt(socketFd_, SOL_SOCKET, SO_RCVBUF, (const char*)&size, sizeof(int));
}

int Socket::setSendBuf(int size)
{
	return ::setsockopt(socketFd_, SOL_SOCKET, SO_SNDBUF, (const char*)&size, sizeof(int));
}

int Socket::setRecvErr(bool on)
{
	int val = on ? 1 : 0;
	return ::setsockopt(socketFd_, IPPROTO_IP, IP_RECVERR,
				   &val, static_cast<socklen_t>(sizeof val));
}

int Socket::getSocketError()
{
	int optval;
	socklen_t optlen = static_cast<socklen_t>(sizeof optval);

	if (::getsockopt(socketFd_, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
	{
		return errno;
	}

	return optval;
}

bool Socket::isSelfConnect()
{
	InetAddr localAddr;
	InetAddr peerAddr;

	getLocalAddr(&localAddr);
	getPeerAddr(&peerAddr);
	
	return localAddr == peerAddr;
}
