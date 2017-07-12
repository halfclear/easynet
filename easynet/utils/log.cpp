#include "log.h"

#include <limits.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include <string>
#include <iostream>
#include <sstream>
#include <thread>

#include "TimeUtil.h"

using namespace easynet;

namespace 
{

const long kRotateIntervalDefault = 24 * 60 * 60;

}

const char* Logger::logLevelStrings[LOG_LEVEL_FATAL + 1] = {
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

Logger::Logger() 
            : logLevel_(LOG_LEVEL_DEBUG),
              fd_(-1),
              realRotate_(time(nullptr)),
              lastRotate_(realRotate_),
              rotateInterval_(kRotateIntervalDefault)
{}

Logger::~Logger() 
{
    if (fd_ != -1) 
    {
        ::close(fd_);
        fd_ = -1;
    }
}

Logger& Logger::getInstance() 
{
    static Logger logger;
    return logger;
}

void Logger::setLogFile(const std::string &filename) 
{
    int fd = ::open(filename.c_str(), O_APPEND|O_CREAT|O_WRONLY|O_CLOEXEC, DEFFILEMODE);
    if (fd < 0) 
    {
        fprintf(stderr, "open log file %s failed. msg: %s ignored\n",
                filename.c_str(), strerror(errno));
        return;
    }

    fileName_ = filename;
    if (fd_ == -1)
    {
        fd_ = fd;
    } 
    else 
    {
        int r = ::dup2(fd, fd_);
        if (r < 0)
        {
            fprintf(stderr, "dup2 error");
        }
        ::close(fd);
    }
}

void Logger::rotateLogFile() 
{
    time_t now = time(NULL);

    if (fd_ == -1 || now / rotateInterval_ == lastRotate_ / rotateInterval_) {
        return;
    }
    lastRotate_ = now;
    long old = realRotate_.exchange(now);
    if (old / rotateInterval_ == lastRotate_ / rotateInterval_) {
        return;
    }
    struct tm ntm;
    ::localtime_r(&now, &ntm);
    char newname[4096];
    ::snprintf(newname, sizeof(newname), "%s_%d%02d%02d%02d%02d",
        fileName_.c_str(), 
        ntm.tm_year + 1900, 
        ntm.tm_mon + 1, 
        ntm.tm_mday,
        ntm.tm_hour, 
        ntm.tm_min);

    const char* oldname = fileName_.c_str();
    int err = ::rename(oldname, newname);
    if (err != 0) {
        ::fprintf(stderr, "rename logfile %s -> %s failed msg: %s\n",
            oldname, newname, strerror(errno));
        return;
    }

    int fd = ::open(fileName_.c_str(), O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC, DEFFILEMODE);
    if (fd < 0) 
    {
        ::fprintf(stderr, "open log file %s failed. msg: %s ignored\n",
            newname, strerror(errno));
        return;
    }
    ::dup2(fd, fd_);
    ::close(fd);
}

void Logger::log(int level, const char* file, int line, const char* func, const char* fmt ...)
{
    static thread_local std::thread::id threadIdNone = std::thread::id();
    static thread_local std::thread::id tid = threadIdNone;
    if (tid == threadIdNone) 
    {
        tid = std::this_thread::get_id();;
    }

    if (level < logLevel_) 
    {
        return;
    }

    rotateLogFile();

    char buffer[4*1024];
    char* p = buffer;
    char* limit = buffer + sizeof(buffer);

    struct timeval now_tv;
    gettimeofday(&now_tv, NULL);
    const time_t seconds = now_tv.tv_sec;
    struct tm t;
    localtime_r(&seconds, &t);
    std::stringstream ss;
    ss << tid;
    p += snprintf(p, limit - p,
        "%04d-%02d-%02d %02d:%02d:%02d %06d %s %s %s:%d [%s]",
        t.tm_year + 1900,
        t.tm_mon + 1,
        t.tm_mday,
        t.tm_hour,
        t.tm_min,
        t.tm_sec,
        static_cast<int>(now_tv.tv_usec),
        ss.str().c_str(),
        logLevelStrings[level],
        file,
        line,
        func);
    va_list args;
    va_start(args, fmt);
    p += vsnprintf(p, limit-p, fmt, args);
    va_end(args);
    p = std::min(p, limit - 3);
    *++p = '\r';
    *++p = '\n';
    *++p = '\0';
    int fd = fd_ == -1 ? 1 : fd_;
    int err = ::write(fd, buffer, p - buffer);
    if (err != p-buffer) 
    {
        fprintf(stderr, "write log file %s failed. written %d errmsg: %s\n",
            fileName_.c_str(), err, strerror(errno));
    }
}

