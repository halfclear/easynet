// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _TIMER_FD_CHANNEL_H_
#define _TIMER_FD_CHANNEL_H_

#include "TimerFd.h"
#include "Channel.h"
#include "Timer.h"

namespace easynet
{

class EventLoop;

class TimerFdChannel
{
public:
	using TimerHandler = Timer::TimerHandler;
	
	TimerFdChannel(EventLoop *loop, int intervalMillis, TimerHandler &&handler);
	TimerFdChannel(EventLoop *loop, int intervalMillis, const TimerHandler &handler)
	    : TimerFdChannel(loop, intervalMillis, TimerHandler(handler))
	{}
	~TimerFdChannel() = default;

	TimerFdChannel(const TimerFdChannel &rhs) = delete;
	TimerFdChannel& operator=(const TimerFdChannel &rhs) = delete;

private:
	void onTimer();

	EventLoop *loop_;
	TimerFd timerFd_;
	Channel channel_;
	TimerHandler handler_;
};

}

#endif