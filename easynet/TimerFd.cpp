// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <sys/timerfd.h>
#include <cstring>

#include "TimerFd.h"
#include "utils/log.h"

using namespace easynet;

TimerFd::TimerFd(int intervalMillis) : fd_(-1)
{
	if (intervalMillis <= 0)
	{
		return;
	}

	fd_ = create();
	setTime(intervalMillis);	
}

int TimerFd::create()
{
	int fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	if (fd < 0)
	{
		LOG_FATAL("create timer fd failed");
		::exit(1);
	}

	return fd;
}

void TimerFd::setTime(int intervalMillis)
{
	struct itimerspec newValue;
	std::memset(&newValue, 0, sizeof(newValue));

	newValue.it_value.tv_sec = intervalMillis / 1000; // seconds, note: @intervalMillis is milliseconds
	newValue.it_value.tv_nsec = (intervalMillis % 1000) * 1000000; // nanoseconds

	newValue.it_interval.tv_sec = intervalMillis / 1000; // seconds, note: intervalMillis is milliseconds
	newValue.it_interval.tv_nsec = (intervalMillis % 1000) * 1000000; // nanoseconds

	int ret = ::timerfd_settime(fd_, 0, &newValue, NULL);
	if (ret < 0)
	{
		LOG_FATAL("timerfd_settime() failed");
		::exit(1);
	}
}

void TimerFd::close() 
{ 
	if (fd_ != -1 )
	{
		::close(fd_);
		fd_ = -1;
	}		 
}