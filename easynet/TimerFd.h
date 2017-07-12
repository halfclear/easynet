// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_TIMERFD_H_
#define _EASYNET_TIMERFD_H_

#include <unistd.h>
#include <sys/eventfd.h>
#include <stdint.h>

namespace easynet {

class TimerFd
{
public:
	explicit TimerFd(int intervalMillis);
	TimerFd(const TimerFd &rhs) = delete;
	~TimerFd() { close(); }

	int fd() const { return fd_; }
	ssize_t read(uint64_t &val) { return ::read(fd_, &val, sizeof(uint64_t)); }

private:
	int  create();
	void setTime(int intervalMillis);
	void close();

	int fd_;
};

}
#endif
