#include <unistd.h>

#include <string>
#include <thread>
#include <iostream>

#include "EventLoop.h"
#include "Timer.h"
#include "utils/TimeUtil.h"
#include "utils/log.h"

#include <test_harness.h>

using namespace std;
using namespace easynet;


void testTimeResolution(int64_t interval)
{
    LOG_INFO("EventLoop-testTimeResolution, interval:%lld millis", interval);
    int64_t timeResolution = 100;

    EventLoop loop(timeResolution);

    int ticks = interval / timeResolution;
    if (interval % timeResolution) {
        ticks++;
    }

    int64_t expectIntervalMillis = ticks * timeResolution;
    
    int64_t lastTime = TimeUtil::now();
    int cnt = 0;
    loop.runAfter(0, [&]{
        cnt++;
        int64_t nowMillis = TimeUtil::now();
        int64_t realInterval = nowMillis - lastTime;
        int64_t diff = cnt == 1 ? realInterval - timeResolution : realInterval - expectIntervalMillis;
        if (diff < 0) {
            diff *= -1;
        }
        lastTime = nowMillis;
        LOG_INFO("onTimer %d, realInterval:%lld", cnt, realInterval);
        ASSERT_LE(diff, ticks*10+10);
    }, interval);

    loop.runAfter(timeResolution * 10, [&]{
        LOG_INFO("to stop test");   
        loop.quit();
    });

    loop.loop();
}

TEST(EventLoop, testTimeResolution)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
    LOG_INFO("-----------------------------------------------------");
    LOG_INFO("EventLoop-testTimeResolution");
    LOG_INFO("-----------------------------------------------------");

    testTimeResolution(20);
    testTimeResolution(120);  
}
