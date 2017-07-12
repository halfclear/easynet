// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <sys/resource.h>

#include <string.h>
#include "rlimit.h"
#include "log.h"

namespace easynet
{

int getRlimit(int resource, size_t &softLimit, size_t &hardLimit)
{
	struct rlimit rlim;

	if (::getrlimit(resource, &rlim) != 0)
	{
		int savedErrno = errno;
		LOG_ERROR("getrlimit error, errno:%d %s", savedErrno, ::strerror(savedErrno));
		return -1;
	}

	softLimit = rlim.rlim_cur;
	hardLimit = rlim.rlim_max;

	return 0;
}

int setRlimit(int resource, size_t softLimit, size_t hardLimit)
{
	struct rlimit rlim;
	rlim.rlim_cur = softLimit;
	rlim.rlim_max = hardLimit;

	if (::setrlimit(resource, &rlim) != 0)
	{
		int savedErrno = errno;
		LOG_ERROR("setrlimit error, errno:%d %s", savedErrno, ::strerror(savedErrno));
		return -1;
	}
	
	return 0;
}

}

 