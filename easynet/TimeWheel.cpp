// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <unistd.h>
#include <functional>

#include "TimeWheel.h"
#include "EventLoop.h"
#include "utils/log.h"

using namespace easynet;

void TimerInWheel::cancel()
{
	timeWheel_->cancelTimer(this);
}

void TimerInWheel::restart(int64_t afterMillis, int64_t intervalMillis)
{
	timeWheel_->restartTimer(this, afterMillis, intervalMillis);
}

int64_t TimerInWheel::remainTime() const
{
	return timeWheel_->remainTime(this);
}

TimeWheel::TimeWheel(EventLoop *loop, int slots, int64_t interval) 
	               : loop_(loop),
	                 numSlots_(slots),
	                 tickIntervalMillis_(interval),
	                 curSlot_(0),
	                 expiring_(false),
	                 timers_(slots),
	                 longTimers_(slots)
{
	tickTimer_ = loop_->runAfter(tickIntervalMillis_, std::bind(&TimeWheel::onTick, this), tickIntervalMillis_);
}

// @afterMillis: ms,
// 		   < 0:    invalid, will return null;
//        == 0:    insert the timer on current slot, the timer will time out in the TimeWheel's next tick;
//         < tickIntervalMillis_: 1 tick at least;
Timer* TimeWheel::addTimer(int64_t afterMillis, TimerHandler &&handler, int64_t intervalMillis)
{
	if (afterMillis < 0 || intervalMillis < 0)
	{
		return nullptr;
	}

	int64_t realAfterMillis = timeToTicks(afterMillis) * tickIntervalMillis_;
	int64_t realIntervalMillis = timeToTicks(intervalMillis) * tickIntervalMillis_;

	std::unique_ptr<TimerInWheel> timer(new TimerInWheel(this, 
		                                        loop_->now() + realAfterMillis, 
		                                        realIntervalMillis, 
		                                        std::move(handler)));
	TimerInWheel *p = timer.get();
	initTimer(p);
	insertTimer(std::move(timer));

	return p;
}

void TimeWheel::initTimer(TimerInWheel *timer)
{
	int64_t afterMillis = timer->getWhen() - loop_->now();
	int64_t afterTicks  = timeToTicks(afterMillis);

	setTimerPosition(timer, afterTicks);
	timer->intervalTicks_ = timeToTicks(timer->getInterval());
}

void TimeWheel::setTimerPosition(TimerInWheel *timer, int ticks)
{
	timer->slot_ = computeSlot(ticks);
	timer->rotation_ = computeRotation(ticks); // the timer will time out after rotation round
	if (timer->rotation_ == 0)
	{
		timer->rotation_ = -1;
	}
	else if (timer->rotation_ > 0 && expiring_ && timer->slot_ == curSlot_)
	{
		timer->rotation_--;
	}
}

void TimeWheel::insertTimer(std::unique_ptr<TimerInWheel> &&timer)
{
	TimerInWheel *p = timer.get();
	int slot = p->slot_;
	if (timer->rotation_ < 0)
	{
		timers_[slot].push_back(std::move(timer));
		p->pos_ = timers_[slot].end();	
	}
	else
	{
		longTimers_[slot].push_back(std::move(timer));
		p->pos_ = longTimers_[slot].end();
	}

	--(p->pos_);
}

void TimeWheel::eraseTimer(TimerInWheel *timer)
{
	if (timer->rotation_ < 0)
	{
		timers_[timer->slot_].erase(timer->pos_);		
	}
	else
	{
		longTimers_[timer->slot_].erase(timer->pos_);
	}
}

// can be called in the timer's callback
void TimeWheel::cancelTimer(TimerInWheel *timer)
{
	if (!timer)
	{
		return;
	}

    if (timer->expiring())
    {
    	// called cancelTimer() in the timer's callback
    	std::unique_ptr<TimerInWheel> t(std::move(*(timer->pos_)));
    }
    else
    {
    	eraseTimer(timer);
    }
}

// @timer must be in the TimeWheel
// @when, ms
// this function will cancel @timer and reset its timing time to @when
void TimeWheel::restartTimer(TimerInWheel *timer, int64_t after, int64_t interval)
{
	if (after < 0 || interval < 0)
	{
		return;
	}

    std::unique_ptr<TimerInWheel> t(std::move(*(timer->pos_)));

    if (!(timer->expiring()))
	{		
		eraseTimer(timer);
	} 

    int64_t realAfterMillis = timeToTicks(after) * tickIntervalMillis_;
	int64_t realIntervalMillis = timeToTicks(interval) * tickIntervalMillis_;

    timer->resetWhen(realAfterMillis + loop_->now());
	timer->resetInterval(realIntervalMillis);

	initTimer(timer);
	insertTimer(std::move(t));
}

// the @timer must be in the TimeWheel
// return the remaining time of @timer, in ms
int64_t TimeWheel::remainTime(const TimerInWheel *timer) const
{
	int64_t left = (timer->slot_ >= curSlot_) ? (timer->slot_ - curSlot_)
				: (timer->slot_ + numSlots_ - curSlot_);

    if (timer->rotation_ >= 0)
    {
    	if (timer->slot_ == curSlot_ && expiring_)
		{
			left += numSlots_;
		}

		left += timer->rotation_ * numSlots_;
    }
	
	return left * tickIntervalMillis_;
}

void TimeWheel::onTick()
{
	std::list<std::unique_ptr<TimerInWheel>> repeatableTimers;

    for (auto it = longTimers_[curSlot_].begin(); it != longTimers_[curSlot_].end();)
    {
    	TimerInWheel *timer = (*it).get();
    	timer->rotation_--;
		if (timer->rotation_ < 0)
		{
			timers_[curSlot_].push_back(std::move(*it));
			auto pos = timers_[curSlot_].end();
			timer->pos_ = --pos;
			it = longTimers_[curSlot_].erase(it);
		}
		else
		{
			++it;
		}
    }

    expiring_ = true;
	for (auto it =  timers_[curSlot_].begin(); it != timers_[curSlot_].end(); )
	{
		TimerInWheel *timer = (*it).get();

		timer->setExpiring(true);
		timer->onTimeout(); // call time's callback
		timer->setExpiring(false);

		// cancelTimer() or restartTimer() may be called in the timer's callback(handler),  
		// so the object of the timer may be moved to other position
		timer = (*it).get();
		if (timer && timer->repeatable()) // restart the timer
		{
			// prepare to move the timer to the correct slot for next timeout
			timer->resetWhen(loop_->now() + timer->getInterval());
	        setTimerPosition(timer, timer->intervalTicks_);  // time pointer just updated, need -1
		    insertTimer(std::move(*it));

		//	repeatableTimers.push_back(std::move(*it));	
		}

		// when a non-repeatalbe timer timed out, just delete it
		it = timers_[curSlot_].erase(it);
	}
	expiring_ = false;

    advance();

/*
	// move the timer to the correct slot for next timeout
	for (auto &timer : repeatableTimers)
	{
		timer->resetWhen(loop_->now() + timer->getInterval());
	    setTimerPosition(timer.get(), timer->intervalTicks_ - 1);  // time pointer just updated, need -1
		insertTimer(std::move(timer));
	}
	*/
}

// @delay: in ms
int64_t TimeWheel::timeToTicks(int64_t delay)
{
	int64_t ticks = 0;;
	if (delay <= 0) // when delay==0, means that insert the timer on current slot
	{
		ticks = 0;
	}
	else if (delay < tickIntervalMillis_)
	{
		ticks = 1;
	}
	else
	{
		ticks = delay / tickIntervalMillis_;
		if (delay % tickIntervalMillis_)
		{
			ticks += 1;
		}
	}

	return ticks;
}

void TimeWheel::close()
{
	if (tickTimer_) 
	{
		tickTimer_->cancel();
		tickTimer_ = nullptr;
	}

	loop_->deleteTimeWheel(this);
}
