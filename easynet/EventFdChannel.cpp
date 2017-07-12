// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <cstring>
#include "EventFdChannel.h"
#include "EventLoop.h"
#include "utils/log.h"

using namespace easynet;

EventFdChannel::EventFdChannel(EventLoop *loop)
	               : loop_(loop),
	                 channel_(loop_, eventFd_.fd())
{
	channel_.setReadHandler(std::bind(&EventFdChannel::onNotified, this));
	channel_.enableReading();
}

void EventFdChannel::notify()
{
    uint64_t one = 1;
    LOG_TRACE("write eventfd %d", eventFd_.fd());
	ssize_t n = eventFd_.write(one);
	if (n != sizeof(one))
	{
		LOG_ERROR("write eventfd error, eventfd = %d, error:%d %s", 
			eventFd_.fd(), errno, ::strerror(errno));
	}
}

void EventFdChannel::onNotified()
{
	uint64_t one = 0;
	LOG_TRACE("read eventfd %d", eventFd_.fd());
	ssize_t n = eventFd_.read(one);
	if (n != sizeof(one))
	{
		LOG_ERROR("read eventfd error, eventfd = %d, error:%d %s", 
			eventFd_.fd(), errno, ::strerror(errno));
	}
}
