#include <string>
#include <iostream>
#include <thread>
#include <future>
#include <functional>

#include "TimeWheel.h"
#include "EventLoop.h"
#include "utils/TimeUtil.h"
#include <test_harness.h>

#include "utils/log.h"

using namespace std;
using namespace easynet;

Timer* addTimer(TimeWheel *tw, const std::string &name, int64_t twInterval,
    int64_t after, int64_t interval, 
    int *cnt, int64_t *lastMillis)
{ 
	LOG_INFO("----add timer %s, run after %d millis, interval %d millis", name.c_str(), after, interval);
    return tw->addTimer(after, [=]{
        int64_t realDelay = tw->getLoop()->now() - *lastMillis;
        *lastMillis = tw->getLoop()->now();
        (*cnt)++;
        
        int64_t expectDelay;
        int mistake;
        if (*cnt == 1) {
            mistake = after / twInterval;
            if (after % twInterval) 
                mistake++;

            expectDelay = (after / twInterval + ((after % twInterval) ? 1 : 0)) * twInterval;    
        } else {
            mistake = interval / twInterval;
            if (interval % twInterval) 
                mistake++;
            expectDelay = (interval / twInterval + ((interval % twInterval) ? 1 : 0)) * twInterval;
        }

        int64_t diff;
        diff = realDelay - expectDelay;
        if (diff < 0)
            diff *= -1;

            LOG_INFO("--timer %s timedout %d times, [expectDelay:realDelay]=[%lld:%lld]", 
                name.c_str(), *cnt, expectDelay, realDelay);
        ASSERT_LE(diff, mistake*10 + 10);
    }, interval);
}

TEST(TimeWheel, testAddTimerInLoop)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
    LOG_INFO("----------------------------------------------");
    LOG_INFO("TimeWheel-testAddTimerInLoop");
    LOG_INFO("----------------------------------------------");
    EventLoop loop;

    int slots = 8;
    int64_t twInterval = 100;
    TimeWheel *tw = loop.addTimeWheel(slots, twInterval);

    int cnt1 = 0;
    int cnt2 = 0;
    int cnt3 = 0;
    int cnt4 = 0;
    int cnt5 = 0;
    int cnt6 = 0;
    int cnt7 = 0;
    int cnt8 = 0;
    int cnt9 = 0;
    int cnt10 = 0;
    
    int64_t lastMillis1 = 0;
    int64_t lastMillis2 = 0;
    int64_t lastMillis3 = 0;
    int64_t lastMillis4 = 0;
    int64_t lastMillis5 = 0;
    int64_t lastMillis6 = 0;
    int64_t lastMillis7 = 0;
    int64_t lastMillis8 = 0;
    int64_t lastMillis9 = 0;
    int64_t lastMillis10 = 0;

    int testTicks = 36;

    int cnt = 0;
    int64_t lastMillis = loop.now();
    tw->addTimer(0, [&]{
        cnt++;
        if (cnt == 1) {
            lastMillis1 = tw->getLoop()->now();
            lastMillis2 = tw->getLoop()->now();
            lastMillis3 = tw->getLoop()->now();
            lastMillis4 = tw->getLoop()->now();
            lastMillis5 = tw->getLoop()->now();
            lastMillis6 = tw->getLoop()->now();
            lastMillis7 = tw->getLoop()->now();
            lastMillis8 = tw->getLoop()->now();
            lastMillis9 = tw->getLoop()->now();
            lastMillis10 = tw->getLoop()->now();
        } else if (cnt == testTicks) {
            loop.quit();
        }
        LOG_INFO("------count timer timedout %d times, real interval %lld-----", cnt, loop.now() - lastMillis);
        lastMillis = loop.now();
    }, twInterval);

    loop.runAfter(twInterval-20, [&]{

        addTimer(tw, "t-0-100", twInterval, 0, 100, &cnt1, &lastMillis1);

        addTimer(tw, "t-0-0", twInterval, 0, 0, &cnt2, &lastMillis2);

        addTimer(tw, "t-100-0", twInterval, 100, 0, &cnt3, &lastMillis3);

        addTimer(tw, "t-100-120", twInterval, 100, 120, &cnt4, &lastMillis4);

        addTimer(tw, "t-20-20", twInterval, 20, 20, &cnt5, &lastMillis5);

        addTimer(tw, "t-700-700", twInterval, 700, 700, &cnt6, &lastMillis6);

        addTimer(tw, "t-800-800", twInterval, 800, 800, &cnt7, &lastMillis7);

        addTimer(tw, "t-1000-2000", twInterval, 1000, 1000, &cnt8, &lastMillis8);

        addTimer(tw, "t-1600-1600", twInterval, 1600, 1600, &cnt9, &lastMillis9);

        addTimer(tw, "t-1700-1700", twInterval, 1700, 1700, &cnt10, &lastMillis10);

    });

    loop.loop();
    EXPECT_EQ(testTicks, cnt);
    EXPECT_EQ(testTicks, cnt1);
    EXPECT_EQ(1, cnt2);
    EXPECT_EQ(1, cnt3);
    EXPECT_EQ((testTicks - 1 - 1) / 2 + 1, cnt4);
    EXPECT_EQ(testTicks - 1, cnt5);
    EXPECT_EQ((testTicks - 1) / 7, cnt6);
    EXPECT_EQ((testTicks - 1) / 8, cnt7);
    EXPECT_EQ((testTicks - 1) / 10, cnt8);
    EXPECT_EQ((testTicks - 1) / 16, cnt9);
    EXPECT_EQ((testTicks - 1) / 17, cnt10);
}

TEST(TimeWheel, testAddTimerInTimer)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
    LOG_INFO("----------------------------------------------");
    LOG_INFO("TimeWheel-testAddTimerInTimer");
    LOG_INFO("----------------------------------------------");
    EventLoop loop;

    int slots = 8;
    int64_t twInterval = 100;
    TimeWheel *tw = loop.addTimeWheel(slots, twInterval);

    int cnt1 = 0;
    int cnt2 = 0;
    int cnt3 = 0;
    int cnt4 = 0;
    int cnt5 = 0;
    int cnt6 = 0;
    int cnt7 = 0;
    int cnt8 = 0;
    int cnt9 = 0;
    int cnt10 = 0;
    
    int64_t lastMillis1 = 0;
    int64_t lastMillis2 = 0;
    int64_t lastMillis3 = 0;
    int64_t lastMillis4 = 0;
    int64_t lastMillis5 = 0;
    int64_t lastMillis6 = 0;
    int64_t lastMillis7 = 0;
    int64_t lastMillis8 = 0;
    int64_t lastMillis9 = 0;
    int64_t lastMillis10 = 0;

    int testTicks = 38;

    int cnt = 0;
    int64_t lastMillis = loop.now();
    tw->addTimer(0, [&]{
        cnt++;
        if (cnt == testTicks) {
            loop.quit();
        }
        LOG_INFO("------count timer timedout %d times, real interval %lld-----", cnt, loop.now() - lastMillis);
        lastMillis = loop.now();
    }, twInterval);

    loop.runAfter(180, [&]{

        LOG_INFO("---to add a timer---");

        tw->addTimer(100, [&]{
            LOG_INFO("---to add timers---");
            lastMillis1 = tw->getLoop()->now();
            lastMillis2 = tw->getLoop()->now();
            lastMillis3 = tw->getLoop()->now();
            lastMillis4 = tw->getLoop()->now();
            lastMillis5 = tw->getLoop()->now();
            lastMillis6 = tw->getLoop()->now();
            lastMillis7 = tw->getLoop()->now();
            lastMillis8 = tw->getLoop()->now();
            lastMillis9 = tw->getLoop()->now();
            lastMillis10 = tw->getLoop()->now();

            addTimer(tw, "t-0-100", twInterval, 0, 100, &cnt1, &lastMillis1);

            addTimer(tw, "t-0-0", twInterval, 0, 0, &cnt2, &lastMillis2);

            addTimer(tw, "t-100-0", twInterval, 100, 0, &cnt3, &lastMillis3);

            addTimer(tw, "t-100-120", twInterval, 100, 120, &cnt4, &lastMillis4);

            addTimer(tw, "t-20-20", twInterval, 20, 20, &cnt5, &lastMillis5);

            addTimer(tw, "t-700-700", twInterval, 700, 700, &cnt6, &lastMillis6);

            addTimer(tw, "t-800-800", twInterval, 800, 800, &cnt7, &lastMillis7);

            addTimer(tw, "t-1000-1000", twInterval, 1000, 1000, &cnt8, &lastMillis8);

            addTimer(tw, "t-1600-1600", twInterval, 1600, 1600, &cnt9, &lastMillis9);

            addTimer(tw, "t-1700-1700", twInterval, 1700, 1700, &cnt10, &lastMillis10);
        });
    });

    loop.loop();

    EXPECT_EQ(testTicks, cnt);
    EXPECT_EQ(testTicks-2, cnt1);
    EXPECT_EQ(1, cnt2);
    EXPECT_EQ(1, cnt3);
    EXPECT_EQ((testTicks - 4) / 2 + 1, cnt4);
    EXPECT_EQ(testTicks - 3, cnt5);
    EXPECT_EQ((testTicks - 3) / 7, cnt6);
    EXPECT_EQ((testTicks - 3) / 8, cnt7);
    EXPECT_EQ((testTicks - 3) / 10, cnt8);
    EXPECT_EQ((testTicks - 3) / 16, cnt9);
    EXPECT_EQ((testTicks - 3) / 17, cnt10);
}

void testRestartTimerInLoop(int64_t restartAfter, int64_t restartInterval)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
    LOG_INFO("----------------------------------------------");
    LOG_INFO("TimeWheel-testRestartTimerInLoop:restart timer after %lld millis, interval %lld millis", 
        restartAfter, restartInterval);
    LOG_INFO("----------------------------------------------");
    EventLoop loop;

    int slots = 8;
    int64_t twInterval = 100;
    TimeWheel *tw = loop.addTimeWheel(slots, twInterval);

    int afterTicks = restartAfter / twInterval + (restartAfter % twInterval ? 1 : 0);
    int intervalTicks = restartInterval / twInterval + (restartInterval % twInterval ? 1 : 0);
    
    int testTicks = 6 + afterTicks + intervalTicks * 2 + 2;

    int cnt = 0;
    int64_t lastMillis = loop.now();
    int64_t lastMillis1 = 0;
    tw->addTimer(0, [&]{
        cnt++;
        if (cnt == 1) {
            lastMillis1 = tw->getLoop()->now();
        } else if (cnt == testTicks) {
            loop.quit();
        }
        LOG_INFO("------count timer timedout %d times, real interval %lld-----", cnt, loop.now() - lastMillis);
        lastMillis = loop.now();
    }, twInterval);

    int cnt1 = 0;
    bool restarted = false;

    Timer *t = nullptr;
    loop.runAfter(twInterval-20, [&]{

        LOG_INFO("--to add timer after 0 millis interval 200 millis--");
        
        t = tw->addTimer(0, [&]{
            EXPECT_EQ(0, t->remainTime());
            int64_t realDelay = tw->getLoop()->now() - lastMillis1;
            lastMillis1 = tw->getLoop()->now();
            cnt1++;
            
            int64_t expectDelay;
            int mistake;
            if (cnt1 == 1) {
                mistake = 0;
                expectDelay = 0;    
            } else if (restarted) {
                restarted = false;
                mistake = restartAfter / twInterval;
                if (restartAfter % twInterval) 
                    mistake++;
                mistake++;
                expectDelay = (restartAfter / twInterval + ((restartAfter % twInterval) ? 1 : 0) + 1) * twInterval;
            } else {
                int64_t interval = t->getInterval();
                mistake = interval / twInterval;
                if (interval % twInterval) 
                    mistake++;
                expectDelay = (interval / twInterval + ((interval % twInterval) ? 1 : 0)) * twInterval;
            }

            int64_t diff;
            diff = realDelay - expectDelay;
            if (diff < 0)
                diff *= -1;

            LOG_INFO("--timer timedout %d times, [expectDelay:realDelay]=[%lld:%lld]", 
                     cnt1, expectDelay, realDelay);
            ASSERT_LE(diff, mistake*10 + 10);
        }, 200);
    });

    loop.runAfter(550,[&]{
        LOG_INFO("---to restart timer after %lld millis and interval %lld millis", restartAfter, restartInterval);
        t->restart(restartAfter, restartInterval);
        restarted = true;
    });

    loop.loop();

    int expectCnt = 3 + 1 + (testTicks - 6 - afterTicks) / intervalTicks;

    EXPECT_EQ(testTicks, cnt);
    EXPECT_EQ(expectCnt, cnt1);
}

// ------------ testRestartTimerInLoop ------------------ //
TEST(TimeWheel, testRestartTimerInLoop0_800)
{
    testRestartTimerInLoop(0, 800);
}

TEST(TimeWheel, testRestartTimerInLoop700_700)
{
    testRestartTimerInLoop(700, 700);
}

TEST(TimeWheel, testRestartTimerInLoop800_800)
{
    testRestartTimerInLoop(800, 800);
}

TEST(TimeWheel, testRestartTimerInLoop1000_1000)
{
    testRestartTimerInLoop(1000, 1000);
}

TEST(TimeWheel, testRestartTimerInLoop1600_1600)
{
    testRestartTimerInLoop(1600, 1600);
}

TEST(TimeWheel, testRestartTimerInLoop1700_1700)
{
    testRestartTimerInLoop(1700, 1700);
}
// ------------ testRestartTimerInLoop ------------------ //


void testRestartTimerInOtherTimer(int64_t restartAfter, int64_t restartInterval)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
    LOG_INFO("----------------------------------------------");
    LOG_INFO("TimeWheel-testRestartTimerInOtherTimer:restart timer after %lld millis, interval %lld millis", 
        restartAfter, restartInterval);
    LOG_INFO("----------------------------------------------");
    EventLoop loop;

    int slots = 8;
    int64_t twInterval = 100;
    TimeWheel *tw = loop.addTimeWheel(slots, twInterval);

    int64_t lastMillis1 = 0;

    int afterTicks = restartAfter / twInterval + (restartAfter % twInterval ? 1 : 0);
    int intervalTicks = restartInterval / twInterval + (restartInterval % twInterval ? 1 : 0);
    int testTicks = 6 + afterTicks + intervalTicks * 2 + 2;

    int cnt = 0;
    int64_t lastMillis = loop.now();
    tw->addTimer(0, [&]{
        cnt++;
        if (cnt == 1) {
            lastMillis1 = tw->getLoop()->now();
        } else if (cnt == testTicks) {
            loop.quit();
        }
        LOG_INFO("------count timer timedout %d times, real interval %lld-----", cnt, loop.now() - lastMillis);
        lastMillis = loop.now();
    }, twInterval);

    int cnt1 = 0;
    bool restarted = false;

    Timer *t = nullptr;
    loop.runAfter(twInterval-20, [&]{
        LOG_INFO("--to add timer after 0 millis interval 200 millis--");
        
        t = tw->addTimer(0, [&]{
            int64_t realDelay = tw->getLoop()->now() - lastMillis1;
            lastMillis1 = tw->getLoop()->now();
            cnt1++;
            
            int64_t expectDelay;
            int mistake;
            if (cnt1 == 1) {
                mistake = 0;
                expectDelay = 0;    
            } else if (restarted) {
                restarted = false;
                mistake = restartAfter / twInterval;
                if (restartAfter % twInterval) 
                    mistake++;
                mistake++;
                expectDelay = (restartAfter / twInterval + ((restartAfter % twInterval) ? 1 : 0) + 1) * twInterval;
            } else {
                int64_t interval = t->getInterval();
                mistake = interval / twInterval;
                if (interval % twInterval) 
                    mistake++;
                expectDelay = (interval / twInterval + ((interval % twInterval) ? 1 : 0)) * twInterval;
            }

            int64_t diff;
            diff = realDelay - expectDelay;
            if (diff < 0)
                diff *= -1;

            LOG_INFO("--timer timedout %d times, [expectDelay:realDelay]=[%lld:%lld]", 
                     cnt1, expectDelay, realDelay);
            ASSERT_LE(diff, mistake*10 + 10);
        }, 200);
    });

    loop.runAfter(550,[&]{
        LOG_INFO("---to set a timer to restart the timer---");

        tw->addTimer(0, [&]{
            LOG_INFO("---to restart timer after %lld millis and interval %lld millis", restartAfter, restartInterval);
            t->restart(restartAfter, restartInterval);
            restarted = true;
        });
    });

    loop.loop();

    int expectCnt = 3 + 1 + (testTicks - 6 - afterTicks) / intervalTicks;

    EXPECT_EQ(testTicks, cnt);
    EXPECT_EQ(expectCnt, cnt1);
}

// ------------ testRestartTimerInOtherTimer ------------------ //
TEST(TimeWheel, testRestartTimerInOtherTimer0_800)
{
    testRestartTimerInOtherTimer(0, 800);
}

TEST(TimeWheel, testRestartTimerInOtherTimer700_700)
{
    testRestartTimerInOtherTimer(700, 700);
}

TEST(TimeWheel, testRestartTimerInOtherTimer800_800)
{
    testRestartTimerInOtherTimer(800, 800);
}

TEST(TimeWheel, testRestartTimerInOtherTimer1000_1000)
{
    testRestartTimerInOtherTimer(1000, 1000);
}

TEST(TimeWheel, testRestartTimerInOtherTimer1600_1600)
{
    testRestartTimerInOtherTimer(1600, 1600);
}

TEST(TimeWheel, testRestartTimerInOtherTimer1700_1700)
{
    testRestartTimerInOtherTimer(1700, 1700);
}
// ------------ testRestartTimerInOtherTimer ------------------ //


void testRestartTimerInTimerHanlder(int64_t restartAfter, int64_t restartInterval)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
    LOG_INFO("----------------------------------------------");
    LOG_INFO("TimeWheel-testRestartTimerInTimerHanlder:restart timer after %lld millis, interval %lld millis", 
        restartAfter, restartInterval);
    LOG_INFO("----------------------------------------------");
    EventLoop loop;

    int slots = 8;
    int64_t twInterval = 100;
    TimeWheel *tw = loop.addTimeWheel(slots, twInterval);

    int64_t lastMillis1 = 0;
    
    int afterTicks = restartAfter / twInterval + (restartAfter % twInterval ? 1 : 0);
    int intervalTicks = restartInterval / twInterval + (restartInterval % twInterval ? 1 : 0);
    int testTicks = 5 + afterTicks + intervalTicks * 2 + 2;
    int cnt = 0;
    int64_t lastMillis = loop.now();
    tw->addTimer(0, [&]{
        cnt++;
        if (cnt == 1) {
            lastMillis1 = tw->getLoop()->now();
        } else if (cnt == testTicks) {
            loop.quit();
        }
        LOG_INFO("------count timer timedout %d times, real interval %lld-----", cnt, loop.now() - lastMillis);
        lastMillis = loop.now();
    }, twInterval);

    int cnt1 = 0;
    bool restarted = false;

    Timer *t = nullptr;
    loop.runAfter(twInterval-20, [&]{
        LOG_INFO("--to add timer after 0 millis interval 200 millis--");
        
        t = tw->addTimer(0, [&]{
            int64_t realDelay = tw->getLoop()->now() - lastMillis1;
            lastMillis1 = tw->getLoop()->now();
            cnt1++;
            
            int64_t expectDelay;
            int mistake;
            if (cnt1 == 1) {
                mistake = 0;
                expectDelay = 0;    
            } else if (restarted) {
                restarted = false;
                mistake = restartAfter / twInterval;
                if (restartAfter % twInterval) 
                    mistake++;
                expectDelay = (restartAfter / twInterval + ((restartAfter % twInterval) ? 1 : 0)) * twInterval;
            } else {
                int64_t interval = t->getInterval();
                mistake = interval / twInterval;
                if (interval % twInterval) 
                    mistake++;
                expectDelay = (interval / twInterval + ((interval % twInterval) ? 1 : 0)) * twInterval;
            }

            int64_t diff;
            diff = realDelay - expectDelay;
            if (diff < 0)
                diff *= -1;

            LOG_INFO("--timer timedout %d times, [expectDelay:realDelay]=[%lld:%lld]", 
                     cnt1, expectDelay, realDelay);
            ASSERT_LE(diff, mistake*10 + 10);
            if (cnt1 == 3) {
                LOG_INFO("---to restart timer after %lld millis and interval %lld millis", restartAfter, restartInterval);
                t->restart(restartAfter, restartInterval);
                restarted = true;
            }
        }, 200);
    });

    loop.loop();

    
    int expectCnt = 3 + 1 + (testTicks - 5 - afterTicks) / intervalTicks;

    EXPECT_EQ(testTicks, cnt);
    EXPECT_EQ(expectCnt, cnt1);
}

// ------------ testRestartTimerInTimerHanlder ------------------ //
TEST(TimeWheel, testRestartTimerInTimerHanlder0_800)
{
    testRestartTimerInTimerHanlder(0, 800);
}

TEST(TimeWheel, testRestartTimerInTimerHanlder700_700)
{
    testRestartTimerInTimerHanlder(700, 700);
}

TEST(TimeWheel, testRestartTimerInTimerHanlder800_800)
{
    testRestartTimerInTimerHanlder(800, 800);
}

TEST(TimeWheel, testRestartTimerInTimerHanlder1000_1000)
{
    testRestartTimerInTimerHanlder(1000, 1000);
}

TEST(TimeWheel, testRestartTimerInTimerHanlder1600_1600)
{
    testRestartTimerInTimerHanlder(1600, 1600);
}

TEST(TimeWheel, testRestartTimerInTimerHanlder1700_1700)
{
    testRestartTimerInTimerHanlder(1700, 1700);
}
// ------------ testRestartTimerInTimerHanlder ------------------ //


TEST(TimeWheel, testCancelTimerInLoop)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
    LOG_INFO("----------------------------------------------");
    LOG_INFO("TimeWheel-testCancelTimerInLoop");
    LOG_INFO("----------------------------------------------");
    EventLoop loop;

    int slots = 8;
    int64_t twInterval = 100;
    TimeWheel *tw = loop.addTimeWheel(slots, twInterval);

    int64_t lastMillis1 = 0;
    int testTicks = 10;
    int cnt = 0;
    int64_t lastMillis = loop.now();
    tw->addTimer(0, [&]{
        cnt++;
        if (cnt == 1) {
            lastMillis1 = tw->getLoop()->now();
        } else if (cnt == testTicks) {
            loop.quit();
        }
        LOG_INFO("------count timer timedout %d times, real interval %lld-----", cnt, loop.now() - lastMillis);
        lastMillis = loop.now();
    }, twInterval);

    int cnt1 = 0;
    Timer *t = nullptr;
    loop.runAfter(twInterval-20, [&]{
        LOG_INFO("--to add timer after 0 millis interval 200 millis--");
        
        t = tw->addTimer(0, [&]{
            int64_t realDelay = tw->getLoop()->now() - lastMillis1;
            lastMillis1 = tw->getLoop()->now();
            cnt1++;
            
            int64_t expectDelay;
            int mistake;
            if (cnt1 == 1) {
                mistake = 0;
                expectDelay = 0;    
            } else {
                int64_t interval = t->getInterval();
                mistake = interval / twInterval;
                if (interval % twInterval) 
                    mistake++;
                expectDelay = (interval / twInterval + ((interval % twInterval) ? 1 : 0)) * twInterval;
            }

            int64_t diff;
            diff = realDelay - expectDelay;
            if (diff < 0)
                diff *= -1;

            LOG_INFO("--timer timedout %d times, [expectDelay:realDelay]=[%lld:%lld]", 
                     cnt1, expectDelay, realDelay);
            ASSERT_LE(diff, mistake*10 + 10);
        }, 200);
    });

    loop.runAfter(550,[&]{
        LOG_INFO("---to cancel timer---");
        t->cancel();
    });

    loop.loop();

    EXPECT_EQ(testTicks, cnt);
    EXPECT_EQ(3, cnt1);
}

TEST(TimeWheel, testCancelTimerInOtherTimer)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
    LOG_INFO("----------------------------------------------");
    LOG_INFO("TimeWheel-testCancelTimerInOtherTimer");
    LOG_INFO("----------------------------------------------");
    EventLoop loop;

    int slots = 8;
    int64_t twInterval = 100;
    TimeWheel *tw = loop.addTimeWheel(slots, twInterval);

    int64_t lastMillis1 = 0;
    int testTicks = 10;
    int cnt = 0;
    int64_t lastMillis = loop.now();
    tw->addTimer(0, [&]{
        cnt++;
        if (cnt == 1) {
            lastMillis1 = tw->getLoop()->now();
        } else if (cnt == testTicks) {
            loop.quit();
        }
        LOG_INFO("------count timer timedout %d times, real interval %lld-----", cnt, loop.now() - lastMillis);
        lastMillis = loop.now();
    }, twInterval);

    int cnt1 = 0;
    Timer *t = nullptr;
    loop.runAfter(twInterval-20, [&]{
        LOG_INFO("--to add timer after 0 millis interval 200 millis--");
        
        t = tw->addTimer(0, [&]{
            int64_t realDelay = tw->getLoop()->now() - lastMillis1;
            lastMillis1 = tw->getLoop()->now();
            cnt1++;
            
            int64_t expectDelay;
            int mistake;
            if (cnt1 == 1) {
                mistake = 0;
                expectDelay = 0;    
            } else {
                int64_t interval = t->getInterval();
                mistake = interval / twInterval;
                if (interval % twInterval) 
                    mistake++;
                expectDelay = (interval / twInterval + ((interval % twInterval) ? 1 : 0)) * twInterval;
            }

            int64_t diff;
            diff = realDelay - expectDelay;
            if (diff < 0)
                diff *= -1;

            LOG_INFO("--timer timedout %d times, [expectDelay:realDelay]=[%lld:%lld]", 
                     cnt1, expectDelay, realDelay);
            ASSERT_LE(diff, mistake*10 + 10);
        }, 200);
    });

    loop.runAfter(550,[&]{
        LOG_INFO("---to set a timer to cancel the timer---");

        tw->addTimer(0, [&]{
            LOG_INFO("---to cancel timer");
            t->cancel();
        });
    });

    loop.loop();

    EXPECT_EQ(testTicks, cnt);
    EXPECT_EQ(3, cnt1);
}

TEST(TimeWheel, testCancelTimerInTimerHandler)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
    LOG_INFO("----------------------------------------------");
    LOG_INFO("TimeWheel-testCancelTimerInTimerHandler");
    LOG_INFO("----------------------------------------------");
    EventLoop loop;

    int slots = 8;
    int64_t twInterval = 100;
    TimeWheel *tw = loop.addTimeWheel(slots, twInterval);

    int64_t lastMillis1 = 0;
    int testTicks = 10;
    int cnt = 0;
    int64_t lastMillis = loop.now();
    tw->addTimer(0, [&]{
        cnt++;
        if (cnt == 1) {
            lastMillis1 = tw->getLoop()->now();
        } else if (cnt == testTicks) {
            loop.quit();
        }
        LOG_INFO("------count timer timedout %d times, real interval %lld-----", cnt, loop.now() - lastMillis);
        lastMillis = loop.now();
    }, twInterval);

    int cnt1 = 0;
    Timer *t = nullptr;
    loop.runAfter(twInterval-20, [&]{
        LOG_INFO("--to add timer after 0 millis interval 200 millis--");
        
        t = tw->addTimer(0, [&]{
            int64_t realDelay = tw->getLoop()->now() - lastMillis1;
            lastMillis1 = tw->getLoop()->now();
            cnt1++;
            
            int64_t expectDelay;
            int mistake;
            if (cnt1 == 1) {
                mistake = 0;
                expectDelay = 0;    
            } else {
                int64_t interval = t->getInterval();
                mistake = interval / twInterval;
                if (interval % twInterval) 
                    mistake++;
                expectDelay = (interval / twInterval + ((interval % twInterval) ? 1 : 0)) * twInterval;
            }

            int64_t diff;
            diff = realDelay - expectDelay;
            if (diff < 0)
                diff *= -1;

            LOG_INFO("--timer timedout %d times, [expectDelay:realDelay]=[%lld:%lld]", 
                     cnt1, expectDelay, realDelay);
            ASSERT_LE(diff, mistake*10 + 10);

            if (cnt1 == 3) {
                LOG_INFO("---to cancel myself");
                t->cancel();
            }
        }, 200);
    });

    loop.loop();

    EXPECT_EQ(testTicks, cnt);
    EXPECT_EQ(3, cnt1);
}

void testRemainTimeInOtherTimer(std::string name, int64_t after, int64_t interval)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
    LOG_INFO("----------------------------------------------");
    LOG_INFO("TimeWheel-testRemainTimeInOtherTimer:timer after %lld, interval %lld", after, interval);
    LOG_INFO("----------------------------------------------");
    EventLoop loop;

    int slots = 8;
    int64_t twInterval = 100;
    TimeWheel *tw = loop.addTimeWheel(slots, twInterval);

    int64_t lastMillis1 = 0;
    int afterTicks = after / twInterval + (after % twInterval ? 1 : 0);
    int intervalTicks = interval / twInterval + (interval % twInterval ? 1 : 0);
    int testTicks = 1 + afterTicks + intervalTicks * 2 + 2;
    int cnt = 0;
    int64_t lastMillis = loop.now();
    tw->addTimer(0, [&]{
        cnt++;
        if (cnt == 1) {
            lastMillis1 = tw->getLoop()->now();
        } else if (cnt == testTicks) {
            loop.quit();
        }
        LOG_INFO("------count timer timedout %d times, real interval %lld-----", cnt, loop.now() - lastMillis);
        lastMillis = loop.now();
    }, twInterval);

    Timer *t = nullptr;

    int shadowCnt = 0;
    loop.runAfter(twInterval-40, [&]{
        LOG_INFO("----to add shadow timer after %lld, interval %lld", after, interval);
        tw->addTimer(after, [&]{
            shadowCnt++;
            LOG_INFO("----shadow timer timedout %d times, the timers remainTime is %lld", shadowCnt, t->remainTime());
            EXPECT_EQ(0, t->remainTime());
        }, interval);
    });

    int cnt1 = 0;
    loop.runAfter(twInterval-30, [&]{
        t = addTimer(tw, name, twInterval, after, interval, &cnt1, &lastMillis1);
    });

    int ticks = afterTicks;
    int cnt2 = 0;
    loop.runAfter(twInterval-20, [&]{
        LOG_INFO("----to add other timer");
        tw->addTimer(0, [&]{
            cnt2++;
            LOG_INFO("----other timer timedout %d times, the timers remainTime is %lld", cnt2, t->remainTime());
            EXPECT_EQ(ticks*twInterval, t->remainTime());
            ticks--;

            if (ticks == 0) {
                if (cnt1 < 1) {
                    if (afterTicks < slots) {
                        ticks = intervalTicks;
                    }
                } else {
                    if (intervalTicks < slots) {
                        ticks = intervalTicks;
                    }
                }
            } else if (ticks < 0) {
                ticks = intervalTicks -1;
            }
        }, twInterval);
    });

    loop.loop();
    int expectCnt = (afterTicks + 1 < testTicks ? 1 : 0) + (testTicks - 1 - afterTicks) / intervalTicks;

    EXPECT_EQ(testTicks, cnt);
    EXPECT_EQ(expectCnt, cnt1);
    EXPECT_EQ(expectCnt, shadowCnt);
    EXPECT_EQ(testTicks, cnt2);
}

// ------------ testRemainTimeInOtherTimer ------------------ //
TEST(TimeWheel, testRemainTimeInOtherTimer700_700)
{
    testRemainTimeInOtherTimer("t-700-700", 700, 700);
}

TEST(TimeWheel, testRemainTimeInOtherTimer800_800)
{
    testRemainTimeInOtherTimer("t-800-800", 800, 800);
}

TEST(TimeWheel, testRemainTimeInOtherTimer1000_1000)
{
    testRemainTimeInOtherTimer("t-1000-1000", 1000, 1000);
}

TEST(TimeWheel, testRemainTimeInOtherTimer1600_1600)
{
    testRemainTimeInOtherTimer("t-1600-1600", 1600, 1600);
}

TEST(TimeWheel, testRemainTimeInOtherTimer1700_1700)
{
    testRemainTimeInOtherTimer("t-1700-1700", 1700, 1700);
}
// ------------ testRemainTimeInOtherTimer ------------------ //

void testRemainTimeInLoop(std::string name, int64_t after, int64_t interval)
{
    Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
    LOG_INFO("----------------------------------------------");
    LOG_INFO("TimeWheel-testRemainTimeInLoop:timer after %lld, interval %lld", after, interval);
    LOG_INFO("----------------------------------------------");
    EventLoop loop;

    int slots = 8;
    int64_t twInterval = 100;
    TimeWheel *tw = loop.addTimeWheel(slots, twInterval);

    int64_t lastMillis1 = 0;
    int afterTicks = after / twInterval + (after % twInterval ? 1 : 0);
    int intervalTicks = interval / twInterval + (interval % twInterval ? 1 : 0);
    int testTicks = 1 + afterTicks + intervalTicks * 2 + 2;
    int cnt = 0;
    int64_t lastMillis = loop.now();
    tw->addTimer(0, [&]{
        cnt++;
        if (cnt == 1) {
            lastMillis1 = tw->getLoop()->now();
        } else if (cnt == testTicks) {
            loop.quit();
        }
        LOG_INFO("------count timer timedout %d times, real interval %lld-----", cnt, loop.now() - lastMillis);
        lastMillis = loop.now();
    }, twInterval);

    Timer *t = nullptr;

    int cnt1 = 0;
    loop.runAfter(twInterval-40, [&]{
        t = addTimer(tw, name, twInterval, after, interval, &cnt1, &lastMillis1);
    });

    int ticks = afterTicks;
    int cnt2 = 0;
    loop.runAfter(twInterval-20, [&]{
        cnt2++;
        LOG_INFO("--timedout %d times, reamain time is %lld millis", cnt2, t->remainTime());
        EXPECT_EQ(ticks * twInterval, t->remainTime());
        ticks--;
        if (ticks < 0) {
            ticks = intervalTicks - 1;
        }
    }, twInterval);

    loop.loop();

    int expectCnt = (afterTicks + 1 < testTicks ? 1 : 0) + (testTicks - 1 - afterTicks) / intervalTicks;

    EXPECT_EQ(testTicks, cnt);
    EXPECT_EQ(expectCnt, cnt1);
    EXPECT_EQ(testTicks, cnt2);
}

// ------------ testRemainTimerInLoop ------------------ //
TEST(TimeWheel, testRemainTimeInLoop700_700)
{
    testRemainTimeInLoop("t-700-700", 700, 700);
}

TEST(TimeWheel, testRemainTimeInLoop800_800)
{
    testRemainTimeInLoop("t-800-800", 800, 800);
}

TEST(TimeWheel, testRemainTimeInLoop1000_1000)
{
    testRemainTimeInLoop("t-1000-1000", 1000, 1000);
}

TEST(TimeWheel, testRemainTimeInLoop1600_1600)
{
    testRemainTimeInLoop("t-1600-1600", 1600, 1600);
}

TEST(TimeWheel, testRemainTimeInLoop1700_1700)
{
    testRemainTimeInLoop("t-1700-1700", 1700, 1700);
}
// ------------ testRemainTimerInLoop ------------------ //
