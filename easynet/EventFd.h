// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_EVENTFD_H_
#define _EASYNET_EVENTFD_H_

#include <unistd.h>
#include <sys/eventfd.h>
#include <stdint.h>

namespace easynet {

class EventFd
{
public:
	explicit EventFd(unsigned int initVal = 0, int flags = EFD_NONBLOCK | EFD_CLOEXEC);
	~EventFd() { close(); }

	EventFd(const EventFd &rhs) = delete;
	EventFd& operator=(const EventFd &rhs) = delete;

	int fd() const { return fd_; }

	ssize_t write(uint64_t val) { return ::write(fd_, &val, sizeof(uint64_t)); }
	ssize_t read(uint64_t &val) { return ::read(fd_, &val, sizeof(uint64_t)); }

private:
	int create(unsigned int initVal, int flags);
	void close() { ::close(fd_); }

	int fd_;
};

}
#endif
