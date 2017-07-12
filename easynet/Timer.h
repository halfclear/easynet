// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_TIMER_H_
#define _EASYNET_TIMER_H_

#include <utility>

namespace easynet
{

class Timer
{
public:
	using TimerHandler = std::function<void ()>;

	Timer(int64_t when, int64_t interval, const TimerHandler &handler)
		: Timer(when, interval, TimerHandler(handler))
	{}

	Timer(int64_t when, int64_t interval, TimerHandler &&handler) 
		: expiring_(false),
		  when_(when),
		  interval_(interval),
		  handler_(std::move(handler))
	{}

	Timer(const Timer &rhs) = delete;	
	Timer& operator=(const Timer &rhs) = delete;

	int64_t getWhen() const { return when_; }
	int64_t getInterval() const { return interval_; }
	bool repeatable() const { return interval_ > 0; }

	virtual int64_t remainTime() const = 0;
	virtual void cancel() = 0;
	virtual void restart(int64_t afterMillis, int64_t intervalMillis = 0) = 0;

	virtual ~Timer() = default;

protected:
	void onTimeout() { if (handler_) handler_(); }
	void resetWhen(int64_t when) { when_ = when; }
	void resetInterval(int64_t interval) { interval_ = interval; }
	void setExpiring(bool on) { expiring_ = on; }
	bool expiring() const { return expiring_; }
	
	bool expiring_;
	int64_t when_;     // this timer will be timedout in @when, ms, since 1970.1.1 0:0:0
	int64_t interval_;
	TimerHandler handler_;
};



/*
class Timer
{
public:
	friend class EventLoop;

	using TimerPos = std::multimap<int64_t, std::unique_ptr<Timer>>::iterator;

	Timer(int64_t when, int64_t interval, const TimerHandler &handler)
		: Timer(when, interval, TimerHandler(handler))
	{}

	Timer(int64_t when, int64_t interval, TimerHandler &&handler) 
		: when_(when),
		  interval_(interval),
		  handler_(std::move(handler))
	{}

	Timer(const Timer &rhs) = delete;
	~Timer() = default;

	int64_t getWhen() const { return when_; }
	int64_t getInterval() const { return interval_; }
	bool repeatable() const { return interval_ > 0; }

private:
	void onTimedout() { handler_(); }
	void resetWhen(int64_t when) { when_ = when; }
	void clearInterval() { interval_ = 0; }

	void setPos(const TimerPos &it) { pos_ = it; }
	const TimerPos& getPos() const { return pos_; }

	int64_t when_;     // this timer will be timedout in @when, ms, since 1970.1.1 0:0:0
	int64_t interval_; // 
	TimerPos pos_; // position(iterator) in the timer map
	TimerHandler handler_;
};
*/

} // namespace easynet

#endif
