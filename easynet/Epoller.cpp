// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <cstring>
#include <sstream>

#include "Epoller.h"
#include "Channel.h"
#include "EventLoop.h"
#include "utils/log.h"

using namespace easynet;

namespace
{

const int kInitEventArrayLength = 32;

}

Epoller::Epoller(EventLoop *loop)
	: loop_(loop),
	  epollFd_(::epoll_create1(EPOLL_CLOEXEC)),
	  events_(kInitEventArrayLength)
{
	if (epollFd_ < 0)
	{
		LOG_FATAL("epoll fd create failed!");
		::exit(1);
	}
}

// @timeoutMillis: -1: waiting for ever; 0: don't wait, will return immediatly; > 0: waiting for @timeoutMillis ms
// @channels: is an output parameter, to return the normal channels(not listen socket) that activated
// @listeningChannels: to return the channels for listen socket that has new connection arrived
void Epoller::poll(int timeoutMillis, ChannelArray *channels, ChannelArray *listenChannels)
{
	int numEvents = ::epoll_wait(epollFd_, 
								 &*events_.begin(),
								 static_cast<int>(events_.size()),
								 timeoutMillis);

	LOG_TRACE("returned from epoll_wait(), timeout = %d millis, epoll fd = %d, %d events triggered", 
		timeoutMillis, epollFd_, numEvents);

	if (numEvents > 0)
	{
		getActiveChannels(numEvents, channels, listenChannels);
		if (numEvents == static_cast<int>(events_.size())) // the buffer is full, expand it
		{
			events_.resize(events_.size() * 2);
		}
	}
	else if (numEvents < 0)  // error occured when epoll_wait, perhaps it is interrupted by signals.
	{
		if (errno != EINTR)
		{		
			LOG_ERROR("epoll_wait() error. epoll fd = %d, error:%d %s", epollFd_, errno, ::strerror(errno));
		}
	}
//	else  // numEvents == 0, timeout
//	{
//		LOG_DEBUG("Epoller::epoll_wait(): nothing happened!");
//	}

	return;
}

void Epoller::getActiveChannels(int numEvents, ChannelArray *channels, ChannelArray *listenChannels)
{
	// 1.EPOLLIN: readable
	// 2.EPOLLOUT: writable
	// 3.EPOLLIN + EPOLLRDHUP: peer closed, or peer shutdown(SHUT_WR), or shutdown(SHUT_RD) myself
	// 4.EPOLLIN + EPOLLRDHUP + EPOLLHUP: shutdown(SHUT_RDWR) myself, ignore this case
	// 5.EPOLLIN + EPOLLRDHUP + EPOLLHUP + EPOLLERR: error occured, probably RST recieved

	for (int i = 0; i < numEvents; i++)
	{
		Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
		int revents = events_[i].events;
		int events = 0;

		LOG_TRACE("triggered %s on fd %d, epoll fd = %d", getEventsName(revents).c_str(), channel->fd(), epollFd_);

		// error occurred, or fd closed
		if (revents & EPOLLERR)
		{		
			events |= EASYNET_EVENT_ERROR; // probably RST received
		}
		else
		{
			// EPOLLRDHUP will not occur alone, it is always occurred with EPOLLIN
			// EPOLLRDHUP: peer closed, or peer shutdown write
			if ((revents & (EPOLLIN | EPOLLRDHUP)) == (EPOLLIN | EPOLLRDHUP))
			{
				events |= EASYNET_EVENT_PEER_SHUTDOWN;
			}
			else if (revents & EPOLLIN)
			{
				events |= EASYNET_EVENT_READABLE;
			}

			if (revents & EPOLLOUT)
			{		
				events |= EASYNET_EVENT_WRITABLE;
			}
		}

		channel->setRevents(events);

		if (!channel->isListenChannel())
		{
			channels->push_back(channel);
		}
		else
		{
			listenChannels->push_back(channel);
		}
	}
}

void Epoller::updateChannel(Channel *channel)
{
	// no event to monitor, so remove channel from the epollFd_
	if (!channel->hasEvent())
	{
		update(EPOLL_CTL_DEL, channel);
		return;
	}
	
	if (channel->monistoring())
	{
		update(EPOLL_CTL_MOD, channel);
	}
	else
	{
		update(EPOLL_CTL_ADD, channel);
	}
}

int Epoller::update(int oper, Channel *channel)
{
	struct epoll_event event;
	std::memset(&event, 0, sizeof(event));
	int events = channel->events();
	if (events & EASYNET_POLL_READ)
	{
		event.events |= (EPOLLIN | EPOLLRDHUP);
	}

	if (events & EASYNET_POLL_WRITE)
	{
		event.events |= EPOLLOUT;
	}

	if (channel->isEdgeTriggerMode())
	{
		event.events |= EPOLLET;
	}

	LOG_TRACE("%s events:%s, fd = %d, epoll fd =%d", 
		getEpollCtlOperName(oper).c_str(), getEventsName(event.events).c_str(), channel->fd(), epollFd_);

	event.data.ptr = channel;
	if (::epoll_ctl(epollFd_, oper, channel->fd(), &event) < 0)
	{
		LOG_ERROR("epoll_ctl() error. epoll fd = %d, operation = %s, fd = %d, error: %d %s", epollFd_,
			 (getEpollCtlOperName(oper)).c_str(), channel->fd(), errno, ::strerror(errno));
		return -1;
	}

	return 0;
}

std::string Epoller::getEventsName(int events)
{
	std::stringstream ss;

	if (events & EPOLLIN)
		ss << "EPOLLIN";

	if (events & EPOLLPRI)
		ss << " EPOLLPRI";

	if (events & EPOLLOUT)
		ss << " EPOLLOUT";

	if (events & EPOLLHUP)
		ss << " EPOLLHUP";

	if (events & EPOLLRDHUP)
		ss << " EPOLLRDHUP";

	if (events & EPOLLERR)
		ss << " EPOLLERR";

	if (events & EPOLLET)
		ss << " EPOLLET";

	return std::move(ss.str());
}

std::string Epoller::getEpollCtlOperName(int oper)
{
	if (oper == EPOLL_CTL_ADD)
	    return "EPOLL_CTL_ADD";

	if (oper == EPOLL_CTL_MOD)
		return "EPOLL_CTL_MOD";

	if (oper == EPOLL_CTL_DEL)
		return "EPOLL_CTL_DEL";

	return "ERROR EPOLL CTL";
}
