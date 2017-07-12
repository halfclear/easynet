#include <string>
#include <iostream>
#include <thread>
#include <future>
#include <functional>

#include "TimerHeap.h"
#include "EventLoop.h"
#include "utils/TimeUtil.h"
#include <test_harness.h>
#include <map>
#include "Socket.h"
#include "InetAddr.h"
#include "utils/log.h"

using namespace std;
using namespace easynet;

TEST(TimerInHeap, testAddTimerAfter0Millis)
{	
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
	LOG_INFO("----------------------------------------------");
	LOG_INFO("TimerInHeap-testAddTimerAfter0Millis");
	LOG_INFO("----------------------------------------------");

	int cnt1 = 0;
	int cnt2 = 0;
	int64_t ts = 0;

    EventLoop loop;
    ts = loop.now();
    LOG_INFO("----to add timer1, it should be timedout after 0 millis");
    loop.runAfter(0, [&]{
    	cnt1++;

    	int64_t diff = loop.now() - ts;
	    LOG_INFO("----timer1 onTimer after %lld millis", diff);
	    ASSERT_LE(diff, 10);
	    loop.quit();
	});

    LOG_INFO("----to add timer2, it should be timedout after 0 millis");
	loop.runAfter(0, [&]{
		cnt2++;

    	int64_t diff = loop.now() - ts;
	    LOG_INFO("----timer2 onTimer after %lld millis", diff);
	    ASSERT_LE(diff, 10);
	    loop.quit();
	});
	loop.loop();
	EXPECT_EQ(1, cnt1);
	EXPECT_EQ(1, cnt2);
}

TEST(TimerInHeap, testAddTimer)
{	
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
	LOG_INFO("----------------------------------------------");
	LOG_INFO("TimerInHeap-testAddTimer");
	LOG_INFO("----------------------------------------------");

	int cnt1 = 0;
	int cnt2 = 0;
	int64_t ts = 0;

    EventLoop loop;
    ts = loop.now();
    LOG_INFO("----to add timer1, it should be timedout after 200 millis");
    loop.runAfter(200, [&]{
    	cnt1++;
    	int64_t realAfter = loop.now() - ts;
    	int64_t diff = realAfter - 200;
    	if (diff < 0) {
    		diff *= -1;
    	}
    	ASSERT_LE(diff, 20);
	    LOG_INFO("----timer1 timedout, [expectDelay:realDelay]:[200:%lld]", realAfter);
	});

    LOG_INFO("----to add timer2, it should be timedout after 300 millis");
	loop.runAfter(300, [&]{
		cnt2++;
    	int64_t realAfter = loop.now() - ts;
    	int64_t diff = realAfter - 300;
    	if (diff < 0) {
    		diff *= -1;
    	}
    	ASSERT_LE(diff, 30);
	    LOG_INFO("----timer2 timedout, [expectDelay:realDelay]:[300:%lld]", realAfter);
	});

	int cnt3 = 0;
	int64_t lastTime = loop.now();
	LOG_INFO("----to add timer3, it should be timedout 5 times every 300 millis");
	loop.runAfter(300, [&]{
		cnt3++;
		int64_t realAfter = loop.now() - lastTime;
		lastTime = loop.now();
    	int64_t diff = realAfter - 300;
    	if (diff < 0) {
    		diff *= -1;
    	}
    	ASSERT_LE(diff, 30);
    	
	    LOG_INFO("----timer3 timedout %d times, [expectDelay:realDelay]:[300:%lld]", cnt3, realAfter);
	    
	    if (cnt3 == 3)
	    {
	    	loop.quit();
	    }
	}, 300);

	loop.loop();
	EXPECT_EQ(1, cnt1);
	EXPECT_EQ(1, cnt2);
	EXPECT_EQ(3, cnt3);
}

TEST(TimerInHeap, testAddTimerInTimer)
{	
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
	LOG_INFO("----------------------------------------------");
	LOG_INFO("TimerInHeap-testAddTimerInTimer");
	LOG_INFO("----------------------------------------------");

	int cnt1 = 0;
	int cnt2 = 0;
	int64_t ts = 0;

    EventLoop loop;
    ts = loop.now();
    LOG_INFO("----to add timer1, it should be timedout after 100 millis");
    loop.runAfter(100, [&]{
    	cnt1++;
    	int64_t realAfter = loop.now() - ts;
    	int64_t diff = realAfter - 100;
    	if (diff < 0) {
    		diff *= -1;
    	}
	    ASSERT_LE(diff, 10);

	    LOG_INFO("----timer1 timedout after %lld millis, to add timer2, it should be timedout after 200 millis", realAfter);

	    int64_t nowMillis = loop.now();
	    loop.runAfter(200, [&, nowMillis]{
	    	cnt2++;
	    	int64_t after = loop.now() - nowMillis;
	    	int64_t diff = after - 200;
	    	if (diff < 0) {
	    		diff *= -1;	
	    	}
		    ASSERT_LE(diff, 20);

		    LOG_INFO("----timer2 timedout %d times, [expectDelay:realDelay]:[200:%lld]", cnt2, after);
		    loop.quit();
	    });
	});

	loop.loop();
	EXPECT_EQ(1, cnt1);
	EXPECT_EQ(1, cnt2);
}

TEST(TimerInHeap, testRestart)
{	
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
	LOG_INFO("----------------------------------------------");
	LOG_INFO("TimerInHeap-testRestart");
	LOG_INFO("----------------------------------------------");
	
    EventLoop loop;

    int cnt1 = 0;
    int64_t lastTime1 = loop.now();
    LOG_INFO("----to add timer1, it should be timedout after 0 millis");
	Timer *t1 = loop.runAfter(0, [&]{
		cnt1++;
		int64_t realAfter = loop.now() - lastTime1;
		lastTime1 = loop.now();
    	int64_t diff = cnt1 == 1 ? realAfter : realAfter - 200;
    	if (diff < 0) {
    		diff *= -1;
    	}
	    ASSERT_LE(diff, 20);
	    int64_t expectDelay = cnt1 == 1 ? 0 : 200;

	    LOG_INFO("----timer1 timedout %d times, [expectDelay:realDelay]:[%lld:%lld]", cnt1, expectDelay, realAfter);
	    if (cnt1 < 3)
	    {
	    	LOG_INFO("----to restart timer1 after 200 millis");
	    	t1->restart(200);
	    }
	});

    int cnt2 = 0;
    int64_t lastTime2 = loop.now();
    LOG_INFO("----to add timer2, it should be timedout after 0 millis");
	Timer *t2 = loop.runAfter(0, [&]{
		cnt2++;
		int64_t realAfter = loop.now() - lastTime2;
		lastTime2 = loop.now();
    	int64_t diff = cnt2 == 1 ? realAfter : realAfter - 100;
    	if (diff < 0) {
    		diff *= -1;
    	}
	    ASSERT_LE(diff, 10);

	    int64_t expectDelay = cnt2 == 1 ? 0 : 100;

	    LOG_INFO("----timer2 timedout %d times, [expectDelay:realDelay]:[%lld:%lld]", cnt2, expectDelay, realAfter);
	    if (cnt2 == 1)
	    {
	    	LOG_INFO("----to restart timer2 after 100 millis, interval 100 mills");
	    	t2->restart(100, 100);
	    } 
	    else if (cnt2 == 4) 
	    {
	    	LOG_INFO("----to cancel timer2");
	    	t2->cancel();
	    }
	});

    int cnt3 = 0;
    int64_t lastTime3 = loop.now();
    LOG_INFO("----to add timer3, it should be timedout every 200 millis");
	Timer *t3 = loop.runAfter(200, [&]{
		cnt3++;
		int64_t realAfter = loop.now() - lastTime3;
		lastTime3 = loop.now();
    	int64_t diff = cnt3 < 3 ? realAfter - 200 : realAfter - 250;
    	if (diff < 0) {
    		diff *= -1;
    	}

	    ASSERT_LE(diff, 20);
	    int64_t expectDelay = cnt3 < 3 ? 200 : 250;
	    LOG_INFO("----timer3 timedout %d times, [expectDelay:realDelay]:[%lld:%lld]", cnt3, expectDelay, realAfter);
	}, 200);

    int64_t lastTime4 = loop.now();
    LOG_INFO("----to add timer4, it should be timedout after 500 millis");
	loop.runAfter(500, [&]{
		int64_t realAfter = loop.now() - lastTime4;
    	int64_t diff = realAfter - 500;
    	if (diff < 0) {
    		diff *= -1;
    	}
	    ASSERT_LE(diff, 50);

		LOG_INFO("----timer4 timedout after %lld millis, to restart timer3 after 150 millis", realAfter);
        t3->restart(150);
	});

    loop.runAfter(1000, [&]{
	    LOG_INFO("----to stop test");
	    loop.quit();
	});

	loop.loop();
	EXPECT_EQ(3, cnt1);
	EXPECT_EQ(4, cnt2);
	EXPECT_EQ(3, cnt3);
}

TEST(TimerInHeap, testCancelInTimer)
{
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
	LOG_INFO("----------------------------------------------");
	LOG_INFO("TimerInHeap-testCancelInTimer");
	LOG_INFO("----------------------------------------------");

	EventLoop loop;

	int cnt1 = 0;
    int64_t lastTime1 = loop.now();
    LOG_INFO("----to add timer1, it should be timedout 3 times every 100 millis");
	Timer *t1 = loop.runAfter(100, [&]{
		cnt1++;
		int64_t realAfter = loop.now() - lastTime1;
		lastTime1 = loop.now();
    	int64_t diff = realAfter - 100;
    	if (diff < 0) {
    		diff *= -1;
    	}
	    ASSERT_LE(diff, 10);
	    LOG_INFO("----timer1 timedout %d times, [expectDelay:realDelay]:[100:%lld]", cnt1, realAfter);
	    if (cnt1 == 3)
	    {
	    	LOG_INFO("----to cancel timer1");
	    	t1->cancel();
	    }
	}, 100);

	loop.runAfter(500, [&]{
		LOG_INFO("----to stop test");
		loop.quit();
	});

	loop.loop();
	EXPECT_EQ(3, cnt1);
}

TEST(TimerInHeap, testCancelInOtherTimer)
{
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
	LOG_INFO("----------------------------------------------");
	LOG_INFO("TimerInHeap-testCancelInOtherTimer");
	LOG_INFO("----------------------------------------------");

	EventLoop loop;

	int cnt1 = 0;
    int64_t lastTime1 = loop.now();
    LOG_INFO("----to add timer1, it should be timedout 3 times every 100 millis");
	Timer *t1 = loop.runAfter(100, [&]{
		cnt1++;
		int64_t realAfter = loop.now() - lastTime1;
		lastTime1 = loop.now();
    	int64_t diff = realAfter - 100;
    	if (diff < 0) {
    		diff *= -1;
    	}

	    ASSERT_LE(diff, 10);
	    LOG_INFO("----timer1 timedout %d times, [expectDelay:realDelay]:[100:%lld]", cnt1, realAfter);
	}, 100);

	loop.runAfter(350, [&]{
		LOG_INFO("----to cancel timer1");
		t1->cancel();
	});

	loop.runAfter(450, [&]{
		LOG_INFO("----to stop test");
		loop.quit();
	});

	loop.loop();
	EXPECT_EQ(3, cnt1);
}

TEST(TimerInHeap, testCancelInLoop)
{
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
	LOG_INFO("----------------------------------------------");
	LOG_INFO("TimerInHeap-testCancelInLoop");
	LOG_INFO("----------------------------------------------");

	EventLoop loop;

	int cnt1 = 0;
    int64_t lastTime1 = loop.now();
    LOG_INFO("----to add timer1, it should be timedout 3 times every 100 millis");
	Timer *t1 = loop.runAfter(100, [&]{
		cnt1++;
		int64_t realAfter = loop.now() - lastTime1;
		lastTime1 = loop.now();
    	int64_t diff = realAfter - 100;
    	if (diff < 0) {
    		diff *= -1;
    	}
	    ASSERT_LE(diff, 10);
	    LOG_INFO("----timer1 timedout %d times, [expectDelay:realDelay]:[100:%lld]", cnt1, realAfter);
	}, 100);

	auto f = std::async(std::launch::async, [&]{

		std::this_thread::sleep_for(std::chrono::milliseconds(350));
	    loop.wakeupAndRun([&]{
	    	LOG_INFO("----to cancel timer1");
	    	t1->cancel();
	    	loop.quit();
	    });
	    std::this_thread::sleep_for(std::chrono::milliseconds(50));
	});

	loop.loop();
	f.wait();

	EXPECT_EQ(3, cnt1);
}
