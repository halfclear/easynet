// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EVENT_FD_CHANNEL_H_
#define _EVENT_FD_CHANNEL_H_

#include "EventFd.h"
#include "Channel.h"

namespace easynet
{

class EventFdChannel
{
public:
	EventFdChannel(EventLoop *loop);
	~EventFdChannel() = default;

	EventFdChannel(const EventFdChannel &rhs) = delete;
	EventFdChannel& operator=(const EventFdChannel &rhs) = delete;

	void notify();

private:
	void onNotified();

	EventLoop *loop_;
	EventFd eventFd_;
	Channel channel_;
};

}

#endif