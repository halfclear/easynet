// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_CHANNEL_H_
#define _EASYNET_CHANNEL_H_

#include <string>
#include <vector>
#include <functional>
#include <utility>

#define EASYNET_POLL_READ          0x01
#define EASYNET_POLL_WRITE         0x02

#define EASYNET_EVENT_READABLE       0x01
#define EASYNET_EVENT_WRITABLE       0x02
#define EASYNET_EVENT_PEER_SHUTDOWN  0x04
#define EASYNET_EVENT_ERROR          0x08

namespace easynet
{

class EventLoop;

class Channel
{
public:
	using EventHandler = std::function<void ()>;

	Channel(EventLoop *loop) : Channel(loop, -1) {}
	Channel(EventLoop *loop, int fd);
	~Channel() { clear(); }

	Channel(const Channel &rhs) = delete;
	Channel& operator=(const Channel &rhs) = delete;

	EventLoop* getLoop() const { return loop_; }
	int fd() const { return fd_; }

	void handleEvent();

	void setReadHandler(EventHandler &&handler)      { readHandler_ = std::move(handler); }
	void setReadHandler(const EventHandler &handler) { setReadHandler(EventHandler(handler)); }

	void setWriteHandler(EventHandler &&handler)      { writeHandler_ = std::move(handler); }
	void setWriteHandler(const EventHandler &handler) { setWriteHandler(EventHandler(handler)); }

	void setPeerShutdownHandler(EventHandler &&handler)      { peerShutdownHandler_ = std::move(handler); }
	void setPeerShutdownHandler(const EventHandler &handler) { setPeerShutdownHandler(EventHandler(handler)); }

	void setErrorHandler(EventHandler &&handler)      { errorHandler_ = std::move(handler); }
	void setErrorHandler(const EventHandler &handler) { setErrorHandler(EventHandler(handler)); }

	bool isEdgeTriggerMode() const  {return isEdgeTriggerMode_; }
	void setLevelTriggerMode() { isEdgeTriggerMode_ = false; }
	void setEdgeTriggerMode()  { isEdgeTriggerMode_ = true; }

	void enableReading();
	void enableWriting();
	void enableAll();

	void disableRdHup() { rdHupDisabled_ = true; }
	void disableReading();
	void disableWriting();
	void disableAll();
	void clear() { rdHupDisabled_ = false; disableAll(); }

	void resetFd(int fd);
	
	int  events() const { return events_; }
	bool hasEvent() const { return events_ != 0; }
	void setRevents(int revents) { revents_ = revents; }  // epoller will tell us what events have been triggered

	bool monistoring() const { return monitoring_; }
	bool writing() const { return monitoring_ && (events_ & EASYNET_POLL_WRITE); }
	bool isListenChannel() const { return isListenChannel_; }
	void markListenChannel() { isListenChannel_ = true; }

private:
	void enable();
	void disable();
	void update();

	EventLoop *loop_;

	int fd_;          // file descriptor to be monitored
	int events_;      // events that interested
	int revents_;     // events has been triggerd on the @fd_
	
	bool isEdgeTriggerMode_; // edge trigger, or level trigger
	bool isListenChannel_; // this channel's @fd_ is a listen socket

	bool monitoring_;
	bool rdHupDisabled_;

	EventHandler readHandler_;           // read event callback function
	EventHandler writeHandler_;          // write event callback function
	EventHandler peerShutdownHandler_;   // peer closed or peer shutdown Writing, callback function
	EventHandler errorHandler_;          // callback function when error occured on fd (recieve RST packet)
};

}
#endif
