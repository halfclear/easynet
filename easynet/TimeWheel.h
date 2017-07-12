// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _TIME_WHEEL_H_
#define _TIME_WHEEL_H_

#include <vector>
#include <list>
#include <memory>
#include <utility>

#include "Channel.h"
#include "Timer.h"

namespace easynet
{

class TimeWheel;

class TimerInWheel : public Timer
{
public:
	friend class TimeWheel;
	using TimerHandler = Timer::TimerHandler;

	virtual ~TimerInWheel() = default;
	int64_t remainTime() const override;
	void cancel() override;
	void restart(int64_t afterMillis, int64_t intervalMillis = 0) override;

private:
	using TimerPos = std::list<std::unique_ptr<TimerInWheel>>::iterator;

	TimerInWheel(TimeWheel *timeWheel, int64_t when, int64_t interval, TimerHandler &&handler) 
		: Timer(when, interval, std::move(handler)),
		  timeWheel_(timeWheel)
    {}

	TimerInWheel(TimeWheel *timeWheel, int64_t when, int64_t interval, const TimerHandler &handler)
		: TimerInWheel(timeWheel, when, interval, TimerHandler(handler))
	{}

	TimeWheel *timeWheel_;
	int rotation_;      // the timer will be timed out after TimeWheel runned rotation_ round
	int slot_;          // the slot the timer is mounted on
	int64_t intervalTicks_; // for repeatable timer. The timer will be timedout every @intervalTicks_ ticks

	TimerPos pos_;
};

class TimeWheel
{
public:
	friend class EventLoop;
	using TimerHandler = TimerInWheel::TimerHandler;

	TimeWheel(const TimeWheel &rhs) = delete;
	~TimeWheel() = default;
	TimeWheel& operator=(const TimeWheel &rhs) = delete;

	EventLoop* getLoop() const { return loop_; }

	Timer* addTimer(int64_t afterMillis, TimerHandler &&handler, int64_t intervalMillis = 0);
	Timer* addTimer(int64_t afterMillis, const TimerHandler &handler, int64_t intervalMillis = 0)
	{ return addTimer(afterMillis, TimerHandler(handler), intervalMillis); }

	int64_t remainTime(const TimerInWheel *timer) const; // the @timer must be in the TimeWheel
	void cancelTimer(TimerInWheel *timer);
	void restartTimer(TimerInWheel *timer, int64_t after, int64_t interval = 0);

	int slots() const { return numSlots_; }
	int curSlot() const { return curSlot_; }
	int timerCount(int slot) const { return timers_[slot].size(); }
	void close();  // can't call close() in ther TimerInWheel's timer handler

private:
	using TimerContainer = std::vector<std::list<std::unique_ptr<TimerInWheel>>>;
	using TimeWheelPos = std::list<std::unique_ptr<TimeWheel>>::iterator;

	TimeWheel(EventLoop *loop, int slots, int64_t interval);
	
	void onTick();
	void advance() { curSlot_ = (curSlot_ + 1) % numSlots_; }

	void initTimer(TimerInWheel *timer);
	void setTimerPosition(TimerInWheel *timer, int ticks);
	void insertTimer(std::unique_ptr<TimerInWheel> &&timer);
	void eraseTimer(TimerInWheel *timer);
	
	int64_t timeToTicks(int64_t delay);
	int computeRotation(int64_t ticks) { return ticks / numSlots_; }
	int computeSlot(int64_t ticks) { return (curSlot_ + (ticks % numSlots_)) % numSlots_; }

	void setPos(const TimeWheelPos &pos) { pos_ = pos; }
	const TimeWheelPos& getPos() const { return pos_; }

	EventLoop *loop_;
	Timer *tickTimer_;

	const int numSlots_; // slots in timewheel
	const int64_t tickIntervalMillis_;  // time interval, ms
	int curSlot_;
	bool expiring_;

	TimeWheelPos pos_;
	TimerContainer timers_;
	TimerContainer longTimers_;
};

}

#endif