// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_TIMER_HEAP_H_
#define _EASYNET_TIMER_HEAP_H_

#include <map>
#include <memory>

#include "Timer.h"

namespace easynet
{

class TimerHeap;

class TimerInHeap : public Timer
{
public:
	friend class TimerHeap;
	using TimerHandler = Timer::TimerHandler;

	virtual ~TimerInHeap() = default;

	int64_t remainTime() const override;
	void cancel() override;
	void restart(int64_t afterMillis, int64_t intervalMillis = 0) override;

private:
	using TimerPos = std::multimap<int64_t, std::unique_ptr<TimerInHeap>>::iterator;

	TimerInHeap(TimerHeap *timerHeap, int64_t when, int64_t interval, const TimerHandler &handler)
		: TimerInHeap(timerHeap, when, interval, TimerHandler(handler))
	{}

	TimerInHeap(TimerHeap *timerHeap, int64_t when, int64_t interval, TimerHandler &&handler) 
		: Timer(when, interval, std::move(handler)),
		  timerHeap_(timerHeap)
	{}

	void setPos(const TimerPos &it) { pos_ = it; }
	const TimerPos& getPos() const { return pos_; }

	TimerHeap *timerHeap_;
	TimerPos pos_; // position(iterator) in the timer map
};

class EventLoop;

class TimerHeap
{
public:
	using TimerHandler = TimerInHeap::TimerHandler;
	
	TimerHeap(EventLoop *loop) : loop_(loop) {}
	~TimerHeap() = default;

	TimerHeap(const TimerHeap &rhs) = delete;
	TimerHeap& operator=(const TimerHeap &rhs) = delete;

	void cancelTimer(TimerInHeap *timer); // can be called in the timer's callback
	void restartTimer(TimerInHeap *timer, int64_t after, int64_t interval = 0); // can be called in the timer's callback
	int64_t remainTime(const TimerInHeap *timer) const; // can be called in the timer's callback

	void expireTimers();
	int64_t getEarliestTimersTimeout() const;
	Timer* addTimer(int64_t when, TimerHandler &&handler, int64_t interval = 0);
	void insertTimer(std::unique_ptr<TimerInHeap> &&timer);

private:
	// the timers' container, key: when this timer will timeout, in miliseconds, since 1970-1-1 00:00:00 
	using TimerContainer = std::multimap<int64_t, std::unique_ptr<TimerInHeap>>;

    EventLoop *loop_;
	TimerContainer timers_;
};

}

#endif