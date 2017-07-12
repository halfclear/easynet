// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include "Channel.h"
#include "EventLoop.h"

using namespace easynet;

Channel::Channel(EventLoop *loop, int fd)
	: loop_(loop),
	  fd_(fd),
	  events_(0),
	  revents_(0),
	  isEdgeTriggerMode_(false),
	  isListenChannel_(false),
	  monitoring_(false),
	  rdHupDisabled_(false)
{}

void Channel::handleEvent()
{
	if (revents_ & EASYNET_EVENT_ERROR)
	{
		if (errorHandler_)
		{
			errorHandler_();
		}
	}
	else
	{
		if (revents_ & EASYNET_EVENT_PEER_SHUTDOWN)
		{
			if (peerShutdownHandler_ && !rdHupDisabled_)
			{
				peerShutdownHandler_();
			} 
		}
		else if (revents_ & EASYNET_EVENT_READABLE)
		{
			if (readHandler_ )
			{
				readHandler_();
			}
		}

		if (revents_ & EASYNET_EVENT_WRITABLE)
		{
			if (writeHandler_ && writing())
			{
				writeHandler_();
			}
		}
	}
}

void Channel::enableReading() 
{
	if ((events_ & EASYNET_POLL_READ) == 0)
	{
		events_ |= EASYNET_POLL_READ;
		enable();
	}
}

void Channel::enableWriting() 
{
	if ((events_ & EASYNET_POLL_WRITE) == 0)
	{
		events_ |= EASYNET_POLL_WRITE; 	
		enable();
	}
}

void Channel::enableAll()
{
	if ((events_ & (EASYNET_POLL_READ | EASYNET_POLL_WRITE))
		!= (EASYNET_POLL_READ | EASYNET_POLL_WRITE))
	{
		events_ = (EASYNET_POLL_READ | EASYNET_POLL_WRITE); 	
		enable();
	}
}

void Channel::enable()
{
	update();
	monitoring_ = true;
}

void Channel::disableReading()
{
	if (monitoring_ && (events_ & EASYNET_POLL_READ)) 
	{
		events_  &= ~EASYNET_POLL_READ;
		revents_ &= ~EASYNET_EVENT_READABLE;
		disable();
	}	
}

void Channel::disableWriting()
{
	if (monitoring_ && (events_ & EASYNET_POLL_WRITE))
	{
		events_  &= ~EASYNET_POLL_WRITE;
		revents_ &= ~EASYNET_EVENT_WRITABLE;
		disable();
	}
}

void Channel::disableAll() 
{
	if (monitoring_)
	{
		events_  = 0;
	    revents_ = 0;
		disable();
	}
}

void Channel::disable()
{
	update();	
	if (events_ == 0)
	{
		monitoring_ = false;
	}
}

void Channel::update()
{
	loop_->updateChannel(this);
}

void Channel::resetFd(int fd)
{
	clear();
	isListenChannel_ = false;
	fd_ = fd;
}
