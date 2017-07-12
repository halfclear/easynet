#ifndef _EASYNET_LOG_H_
#define _EASYNET_LOG_H_

#include <cstdlib>
#include <string>
#include <atomic>

#define KLOG(level, ...) \
	do {  \
		if (level >= easynet::Logger::getInstance().getLogLevel()) {  \
			easynet::Logger::getInstance().log(level, __FILE__, __LINE__, __func__, __VA_ARGS__);  \
		}  \
	} while(0);

#define LOG_TRACE(...) KLOG(easynet::Logger::LOG_LEVEL_TRACE, __VA_ARGS__)
#define LOG_DEBUG(...) KLOG(easynet::Logger::LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_INFO(...) KLOG(easynet::Logger::LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_WARN(...) KLOG(easynet::Logger::LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_ERROR(...) KLOG(easynet::Logger::LOG_LEVEL_ERROR, __VA_ARGS__)
#define LOG_FATAL(...) KLOG(easynet::Logger::LOG_LEVEL_FATAL, __VA_ARGS__)

namespace easynet {

class Logger
{
public:
	enum LogLeveL
	{
		LOG_LEVEL_TRACE = 0,
		LOG_LEVEL_DEBUG,
		LOG_LEVEL_INFO,
		LOG_LEVEL_WARN,
		LOG_LEVEL_ERROR,
		LOG_LEVEL_FATAL
	};

	static Logger& getInstance();	

	Logger(const Logger &rhs) = delete;
	Logger(Logger &&rhs) = delete;
	~Logger();
	Logger& operator=(const Logger &rhs) = delete;

	LogLeveL getLogLevel() const { return logLevel_; }

	void setRotateInterval(long interval) { rotateInterval_ = interval; }
	void setLogFile(const std::string &fileName);
	void setLogLevel(LogLeveL logLevel) { logLevel_ = logLevel; }

	void log(int level, const char* file, int line, const char* func, const char* fmt ...);

private:
	Logger();
	void rotateLogFile();

	static const char* logLevelStrings[LOG_LEVEL_FATAL + 1];

	LogLeveL logLevel_;
    int fd_;
    std::string fileName_;
    std::atomic<int64_t> realRotate_;
    long lastRotate_;
    long rotateInterval_;
};

}

#endif
