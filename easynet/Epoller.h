// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_EPOLLER_H_
#define _EASYNET_EPOLLER_H_

#include <unistd.h>
#include <sys/epoll.h>

#include <string>
#include <vector>

namespace easynet{

class EventLoop;
class Channel;

class Epoller
{
public:
	using ChannelArray = std::vector<Channel*>;

	Epoller(EventLoop *loop);
	~Epoller() { close(); }

	Epoller(const Epoller &rhs) = delete;
	Epoller& operator=(const Epoller &rhs) = delete;

	void poll(int timeoutMillis, ChannelArray *channels, ChannelArray *listenChannels);
	void updateChannel(Channel *channel);

private:
	void getActiveChannels(int numEvents, ChannelArray *channels, ChannelArray *listenChannels);
	int  update(int oper, Channel *channel);
	void close() { ::close(epollFd_); }

	std::string getEventsName(int events);
	std::string getEpollCtlOperName(int oper);

	EventLoop *loop_;  // the EventLoop of the epoller belongs to
	int epollFd_;

	std::vector<epoll_event> events_;  // buffers for epoller to return triggered events
};

}
#endif
