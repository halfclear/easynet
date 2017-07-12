// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_EVENT_LOOP_H_
#define _EASYNET_EVENT_LOOP_H_

#include <vector>
#include <map>
#include <mutex>

#include "Channel.h"
#include "Epoller.h"
#include "EventFdChannel.h"
#include "TimerFdChannel.h"
#include "TimerHeap.h"
#include "TimeWheel.h"
#include "SignalHandlerMgr.h"
#include "utils/TimeUtil.h"

#define EASYNET_TIMER_INFINITE   -1

namespace easynet
{

class EventLoop
{
public:
	using Functor = std::function<void ()>;
	using ChannelArray = Epoller::ChannelArray;
	using SigHandler = SignalHandlerMgr::SigHandler;
	using TimerHandler = TimerInHeap::TimerHandler;

	explicit EventLoop(int timeResolutionMillis = 0);
	~EventLoop() = default;

	EventLoop(const EventLoop &rhs) = delete;
	EventLoop& operator=(const EventLoop &rhs) = delete;

	void loop(); // loop forever
	void quit(); // can be called in other threads to stop loop() immediately
	void waitAndProcessEventsAndTimers(int timeoutMillis, const Functor *functorRunAfterAccept);
	
	// runs func immediately in the loop thread.
	// it wakes up the loop, and run the func in the loop
	// can be called in other thread
	void wakeupAndRun(Functor &&func);
	void wakeupAndRun(const Functor &func) { wakeupAndRun(Functor(func)); }
	void wakeup() { notifier_.notify(); }

    // CAN'T call this function in other thread
	void updateChannel(Channel *channel) { epoller_.updateChannel(channel); } 

	int64_t now() const { return now_; } // milliseconds, since 1970-1-1 00:00:00
	bool timeResolutionEnabled() const { return timeResolutionMillis_ > 0; }

	// ---------------- functions runAt()  runAfter() can ONLY be called in event loop ------------------------//
	//  ------------------------------------------------------------------------------------------------------------------------------------
	// | when @intervalMillis > 0:                                                                                                          |
	// |------------------------------------------------------------------------------------------------------------------------------------|
	// | 1.@timeResolutionMillis_ > 0:                                                                                                      |
	// |    it will be timed out every                                                                                                      |
	// |                                                                                                                                    |
	// |  (((@intervalMillis / @timeResolutionMillis_) + (@intervalMillis % @timeResolutionMillis_ > 0) ? 1 : 0)) * @timeResolutionMillis_  |
	// |                                                                                                                                    |
	// |       milli seconds (except the first timeout).                                                                                    |
	// |    .e.g: @timeResolutionMillis_ = 100 ms, @intervalMillis = 155 ms, it will be timed out every 200ms (except the first timeout)    |
	// | 2.@timeResolutionMillis_ == 0:                                                                                                     |
	// |    it will be timed out every @intervalMillis milli seconds                                                                        |
	//  ------------------------------------------------------------------------------------------------------------------------------------

	// when using time resolution(that is @timeResolutionMillis_>0), the timer's real timeout 
	// might be delayed [0 ~ @timeResolutionMillis_) milli seconds,
	// it means that the timer may be timedout between @whenMillis ~ @whenMillis+@timeResolutionMillis_.
	// in other words, the timer's timeout is always later than @whenMillis
	// .e.g:  
	//     @timeResolutionMillis_ = 100 ms:
	//       @whenMillis=141234000, than the timer will be timed out at 141234000~141234100 ms   
	Timer* runAt(int64_t whenMillis, TimerHandler &&handler, int64_t intervalMillis = 0)      
	{ return addTimer(whenMillis, std::move(handler), intervalMillis); }

	Timer* runAt(int64_t whenMillis, const TimerHandler &handler, int64_t intervalMillis = 0) 
	{ return runAt(whenMillis, TimerHandler(handler), intervalMillis); }

	// when using time resolution(that is @timeResolutionMillis_>0):
	//      @afterMillis <= @timeResolutionMillis_: the real timeout is between 0 ~ @timeResolutionMillis_ milli-seconds;
	//      @afterMillis > @timeResolutionMillis_:  the real timeout is between
	//           (((@afterMillis / @timeResolutionMillis_) + ((@afterMillis % @timeResolutionMillis_ > 0) ? 1 : 0)) * @timeResolutionMillis_ - @timeResolutionMillis_) 
	//        ~  ((((@afterMillis / @timeResolutionMillis_) + (@afterMillis % @timeResolutionMillis_ > 0) ? 1 : 0)) * @timeResolutionMillis_)
	// that means, the timer's real timeout might be earlier as well as later than @afterMillis.
	// .e.g:
	//     @timeResolutionMillis_ = 100 ms:
	//       @afterMillis = 30 ms, than the timer will be timedout after 0 ~ 100 ms;
	//       @afterMillis = 100 ms, than the timer will be timedout after 0 ~ 100 ms;
	//       @afterMillis = 101 ms, than the timer will be timedout after 100 ~ 200 ms;
	//       @afterMillis = 200 ms, than the timer will be timedout after 100 ~ 200 ms;
	//       @afterMillis = 299 ms, than the timer will be timedout after 200 ~ 300 ms;
	//
	Timer* runAfter(int64_t afterMillis, TimerHandler &&handler, int64_t intervalMillis = 0)      
	{ return runAt(now() + afterMillis, std::move(handler), intervalMillis); }

	Timer* runAfter(int64_t afterMillis, const TimerHandler &handler, int64_t intervalMillis = 0) 
	{ return runAfter(afterMillis, TimerHandler(handler), intervalMillis); }

    // can only be called in event loop
	TimeWheel* addTimeWheel(int slots, int64_t intervalMillis);
	void deleteTimeWheel(TimeWheel *timeWheel);

    Timer* addIdleTimer(int64_t idleMillis, TimerHandler &&handler);
	Timer* addIdleTimer(int64_t idleMillis, const TimerHandler &handler)
	{ return addIdleTimer(idleMillis, TimerHandler(handler)); }

    // can only be called in event loop
	SignalHandler* addSignalHandler(int sig, SigHandler &&handler) { return signalHandlerMgr_.addSignalHandler(sig, std::move(handler)); }
	SignalHandler* addSignalHandler(int sig, const SigHandler &handler) { return addSignalHandler(sig, SigHandler(handler)); }
	
	void signalRaised(int sig) { signalHandlerMgr_.signalRaised(sig); }

private:
	using TimeWheelContainer = std::list<std::unique_ptr<TimeWheel>> ;

	void updateTime() { now_ = TimeUtil::now(); }
	void initTimeUpdater();
	void runWakeupFunctors();
	
	void expireTimers() { timerHeap_.expireTimers(); }
	int64_t getEarliestTimersTimeout() const { return timerHeap_.getEarliestTimersTimeout(); }
	Timer* addTimer(int64_t whenMillis, TimerHandler &&handler, int64_t intervalMillis = 0)
	{ return timerHeap_.addTimer(whenMillis, std::move(handler), intervalMillis); }

	bool looping_;
	bool quit_;

	int64_t now_;   // current time, milliseconds since 1970.1.1 0:0:0
	int timeResolutionMillis_; // ms

	Epoller epoller_;  // io multi-selector
	ChannelArray activeChannels_;
	ChannelArray activeListenChannels_;

	std::mutex mutex_;    // to protect wakeupFunctors_
	EventFdChannel notifier_; // for other thread to wake me up asynchronously
	std::vector<Functor> wakeupFunctors_;     // callback functions that other thread want me execute in the loop 
	std::unique_ptr<TimerFdChannel> timeUpdater_;  // to ensure the event loop will be wake up every @timeResolutionMillis_ miliseconds

	TimerHeap timerHeap_;
	TimeWheelContainer timeWheels_;
	SignalHandlerMgr signalHandlerMgr_;

	TimeWheel *idleTimeWheel_;
};

}
#endif
