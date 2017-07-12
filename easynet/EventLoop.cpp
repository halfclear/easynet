// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <unistd.h>
#include <cstring>
#include <utility>

#include "EventLoop.h"
#include "utils/log.h"

namespace
{

const int kMaxTimeResolution = 1000; // milli-seconds

}

using namespace easynet;

EventLoop::EventLoop(int timeResolutionMillis)
	            : looping_(false),
				  quit_(false),
				  timeResolutionMillis_(timeResolutionMillis),
				  epoller_(this),
				  notifier_(this),
				  timerHeap_(this),
				  signalHandlerMgr_(this),
				  idleTimeWheel_(nullptr)
{
	updateTime();
	initTimeUpdater();
}

void EventLoop::initTimeUpdater()
{
	if (timeResolutionMillis_ <= 0)
	{
		return;
	}

	if (timeResolutionMillis_ > kMaxTimeResolution)
	{
		timeResolutionMillis_ = kMaxTimeResolution;
	}

	timeUpdater_.reset(new TimerFdChannel(this, 
		                        timeResolutionMillis_, 
		                        std::bind(&EventLoop::updateTime, this)));
}

void EventLoop::loop()
{
	if (looping_)
	{
		LOG_WARN("the EventLoop is already in looping.");
		return;
	}

	looping_ = true;
	quit_    = false;

	while (!quit_)
	{
		waitAndProcessEventsAndTimers(EASYNET_TIMER_INFINITE, nullptr);
	}

	looping_ = false;
}

void EventLoop::waitAndProcessEventsAndTimers(int timeoutMillis, const Functor *functorRunAfterAccept)
{
	activeChannels_.clear();
	activeListenChannels_.clear();

	int earliestTimersTimeoutMillis;
	if (timeResolutionEnabled())
	{
		earliestTimersTimeoutMillis = EASYNET_TIMER_INFINITE;
	}
	else
	{
		earliestTimersTimeoutMillis = getEarliestTimersTimeout();
	}
	
	if (timeoutMillis == EASYNET_TIMER_INFINITE ||
		(earliestTimersTimeoutMillis != EASYNET_TIMER_INFINITE && timeoutMillis > earliestTimersTimeoutMillis))
	{
		timeoutMillis = earliestTimersTimeoutMillis;
	}

	int64_t delta = now();
	epoller_.poll(timeoutMillis, &activeChannels_, &activeListenChannels_);  //-1: blocking forever; 0: return immediatly

	// not set @timeResolutionMillis_
	if (!timeResolutionEnabled())
	{
		// if not set @timeResolutionMillis_, update current time(@now_) every time when returned from epoll_wait()
		updateTime();
	}

	// for each active fd that being monitored in @epoller_, call it's event handler
	for (auto channel : activeListenChannels_)
	{
		channel->handleEvent();
	}

	if (!activeListenChannels_.empty() && functorRunAfterAccept)
	{
		(*functorRunAfterAccept)();
	}

	for (auto channel : activeChannels_)
	{
		channel->handleEvent();
	}

	runWakeupFunctors(); // to run other threads' callback function

	delta = now() - delta;
	if (delta || timeoutMillis == 0)
	{
		expireTimers();
	}
}

void EventLoop::quit()
{
	quit_ = true;
	wakeup();
}

void EventLoop::wakeupAndRun(Functor &&func)
{
	if (func)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		wakeupFunctors_.push_back(std::move(func));
	}
	wakeup();
}


void EventLoop::runWakeupFunctors()
{
	std::vector<Functor> functors;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		functors.swap(wakeupFunctors_);
	}

	for (auto &functor : functors)
	{
		functor();
	}
}

TimeWheel* EventLoop::addTimeWheel(int slots, int64_t intervalMillis)
{
	std::unique_ptr<TimeWheel> timeWheel(new TimeWheel(this, slots, intervalMillis));
	TimeWheel *p = timeWheel.get();
	timeWheels_.push_back(std::move(timeWheel));
	p->setPos(--timeWheels_.end());
	return p;
}

void EventLoop::deleteTimeWheel(TimeWheel *timeWheel)
{
	timeWheels_.erase(timeWheel->getPos());
}

Timer* EventLoop::addIdleTimer(int64_t idleMillis, TimerHandler &&handler)
{
	static const int slots = 512;
	static const int64_t interval = 1000;

	if (idleTimeWheel_ == nullptr)
	{
		idleTimeWheel_ = addTimeWheel(slots, interval);
		idleMillis -= interval;
		if (idleMillis < 0)
		{
			idleMillis = 0;
		}
	}
	return idleTimeWheel_->addTimer(idleMillis, std::move(handler));
}
