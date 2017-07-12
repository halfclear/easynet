// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include "EventFd.h"
#include "utils/log.h"

using namespace easynet;

EventFd::EventFd(unsigned int initVal, int flags)
         	: fd_(create(initVal, flags))
{}

int EventFd::create(unsigned int initVal, int flags)
{
	int fd = ::eventfd(initVal, flags);
	if (fd < 0)
	{
		LOG_FATAL("eventfd fd create failed!");
		::exit(1);
	}

	return fd;
}
