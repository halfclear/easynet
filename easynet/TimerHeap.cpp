// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <utility>

#include "TimerHeap.h"
#include "EventLoop.h"

using namespace easynet;

int64_t TimerInHeap::remainTime() const
{
	return timerHeap_->remainTime(this);
}

void TimerInHeap::cancel()
{
	timerHeap_->cancelTimer(this);
}

void TimerInHeap::restart(int64_t afterMillis, int64_t intervalMillis)
{
	timerHeap_->restartTimer(this, afterMillis, intervalMillis);
}

int64_t TimerHeap::getEarliestTimersTimeout() const
{
	if (!timers_.empty())
	{
		int64_t earlistTimeoutMillis = (timers_.begin())->first - loop_->now();
		return earlistTimeoutMillis > 0 ? earlistTimeoutMillis : 0;
	}

	return EASYNET_TIMER_INFINITE;
}

void TimerHeap::expireTimers()
{
	auto it = timers_.begin();
	while (it != timers_.end())
	{
		// the earliest timer is not timedout yet, so we don't need to check other timers 
		if (it->first > loop_->now())
		{
			return;
		}

        auto jt = it;
		++it;

		TimerInHeap *timer = (jt->second).get();
		timer->setExpiring(true);
		timer->onTimeout();          // call its callback
		timer->setExpiring(false);
        timer = (jt->second).get();
		if (timer && timer->repeatable())
		{
			timer->resetWhen(loop_->now() + timer->getInterval());
		    insertTimer(std::move(jt->second));
		}
		
		timers_.erase(jt);
		//it = timers_.begin();
	}
}

Timer* TimerHeap::addTimer(int64_t when, TimerHandler &&handler, int64_t interval)
{
	std::unique_ptr<TimerInHeap> timer(new TimerInHeap(this, when, interval, std::move(handler)));
	TimerInHeap *p = timer.get();
	insertTimer(std::move(timer));

	return p;
}

void TimerHeap::insertTimer(std::unique_ptr<TimerInHeap> &&timer)
{
	TimerInHeap *p = timer.get();
	auto it = timers_.insert(std::make_pair(timer->getWhen(), std::move(timer)));
	p->setPos(it);
}

void TimerHeap::cancelTimer(TimerInHeap *timer)
{
	if (!timer)
	{
		return;
	}

    if (timer->expiring())
    {
    	// called cancelTimer() in the timer's callback
    	std::unique_ptr<TimerInHeap> t(std::move(timer->getPos()->second));
    }
	else
	{
		timers_.erase(timer->getPos());
	}
}

void TimerHeap::restartTimer(TimerInHeap *timer, int64_t after, int64_t interval)
{
	if (after < 0 || interval < 0)
	{
		return;
	}

	std::unique_ptr<TimerInHeap> t(std::move(timer->getPos()->second));
	timer->resetWhen(after + loop_->now());
	timer->resetInterval(interval);
	if (!(timer->expiring()))
	{
		timers_.erase(timer->getPos());
	}

	insertTimer(std::move(t));
}

int64_t TimerHeap::remainTime(const TimerInHeap *timer) const 
{
	return timer->getWhen() - loop_->now();
}