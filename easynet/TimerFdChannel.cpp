// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <utility>
#include <cstring>
#include "TimerFdChannel.h"
#include "EventLoop.h"
#include "utils/log.h"

using namespace easynet;

TimerFdChannel::TimerFdChannel(EventLoop *loop, 
	                 int intervalMillis, 
	                 TimerHandler &&handler)
                    : loop_(loop),
                      timerFd_(intervalMillis),
                      channel_(loop_, timerFd_.fd()),
                      handler_(std::move(handler))
{
	channel_.setReadHandler(std::bind(&TimerFdChannel::onTimer, this));
	channel_.enableReading();
}

void TimerFdChannel::onTimer()
{
	uint64_t one = 0;
	ssize_t n = timerFd_.read(one);
	if (n != sizeof(one))
	{
		LOG_ERROR("read timerfd error, timerfd = %d, error:%d %s", 
			timerFd_.fd(), errno, ::strerror(errno));
	}

	if(handler_)
	{
		handler_();
	}
}