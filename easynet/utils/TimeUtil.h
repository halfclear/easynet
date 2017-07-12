// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_TIME_UTIL_H_
#define _EASYNET_TIME_UTIL_H_

#include <sys/time.h>
#include <unistd.h>
#include <utility>
#include <chrono>
#include <ctime>

namespace easynet
{

class TimeUtil
{
public:
	// using c++11 chrono instead of gettimeofday()

    static int64_t now() { return currentSystemTimeMillis(); }
   // static int64_t now() { return currentMonoTimeMillis(); }

    static int64_t currentSystemTimeMillis()
    {
    	std::chrono::time_point<std::chrono::system_clock> p = std::chrono::system_clock::now();
 	    return std::chrono::duration_cast<std::chrono::milliseconds>(p.time_since_epoch()).count(); 
    }

    static int64_t currentSystemTimeMicros()
    {
        std::chrono::time_point<std::chrono::system_clock> p = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(p.time_since_epoch()).count(); 
    }

    static int64_t currentMonoTimeMillis()
    {
        std::chrono::time_point<std::chrono::steady_clock> p = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(p.time_since_epoch()).count(); 
    }

    static int64_t currentMonoTimeMicros()
    {
        std::chrono::time_point<std::chrono::steady_clock> p = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(p.time_since_epoch()).count(); 
    }

    static void getCurrentSystemTime(char *buf, size_t len)
    {
        std::time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
     //   std::time_t tt = std::time(nullptr);
        std::strftime(buf, len, "%Y-%m-%d %H:%M:%S", std::localtime(&tt));
    }

    static std::string getCurrentSystemTime()
    {
        char buf[64];
        getCurrentSystemTime(buf, sizeof(buf));
        return std::move(std::string(buf));
    }

    static void getCurrentSystemTimeWithMicros(char *buf, size_t len)
    {
        struct timeval now_tv;
        ::gettimeofday(&now_tv, NULL);
        const time_t seconds = now_tv.tv_sec;
        struct tm t;
        ::localtime_r(&seconds, &t);

        snprintf(buf, len, "%04d-%02d-%02d %02d:%02d:%02d %06d",
            t.tm_year + 1900,
            t.tm_mon + 1,
            t.tm_mday,
            t.tm_hour,
            t.tm_min,
            t.tm_sec,
            static_cast<int>(now_tv.tv_usec));
    }

    static void getCurrentSystemTimeWithMillis(char *buf, size_t len)
    {
        struct timeval now_tv;
        ::gettimeofday(&now_tv, NULL);
        const time_t seconds = now_tv.tv_sec;
        struct tm t;
        ::localtime_r(&seconds, &t);

        snprintf(buf, len, "%04d-%02d-%02d %02d:%02d:%02d %03d",
            t.tm_year + 1900,
            t.tm_mon + 1,
            t.tm_mday,
            t.tm_hour,
            t.tm_min,
            t.tm_sec,
            static_cast<int>(now_tv.tv_usec / 1000));
    }

    static std::string getCurrentSystemTimeWithMillis()
    {
        char buf[100];
        getCurrentSystemTimeWithMillis(buf, sizeof(buf));
        return std::move(std::string(buf));
    }
    
    /*
    static int64_t now()
    {
    	struct timeval tv;
	    ::gettimeofday(&tv, NULL);

	    int64_t sec = tv.tv_sec;
	    int64_t msec = tv.tv_usec / 1000;
	    return sec * 1000 + msec;
    }
    */
};

} // namespace easynet


#endif
