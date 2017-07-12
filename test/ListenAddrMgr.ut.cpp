#include <string>
#include <iostream>
#include <thread>
#include <future>
#include <functional>

#include "TimeWheel.h"
#include "EventLoop.h"
#include "InetAddr.h"
#include "ListenAddrMgr.h"
#include "utils/TimeUtil.h"

#include <test_harness.h>

#include "utils/log.h"

using namespace std;
using namespace easynet;

TEST(ListenAddrMgr, testListentAddrMgr)
{
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
	LOG_INFO("-----------------------------------------------------");
    LOG_INFO("ListenAddrMgr-testListentAddrMgr");
    LOG_INFO("-----------------------------------------------------");

	ListenAddrMgr mgr;

	ASSERT_TRUE(mgr.addListenAddr("*", 1234));
	ASSERT_TRUE(mgr.addListenAddr("*", 8088));
	ASSERT_TRUE(mgr.addListenAddr("127.0.0.1", 8088));
	ASSERT_TRUE(mgr.addListenAddr("10.94.98.90", 8088));
	ASSERT_TRUE(mgr.addListenAddr("10.94.98.90", 8089));
	ASSERT_FALSE(mgr.addListenAddr("10.94.98.90", 8088));
	ASSERT_FALSE(mgr.addListenAddr("*", 8088));

	auto v = mgr.getListenAddrs();
	for (auto addr : v)
	{
		LOG_INFO("%s", addr.toString().c_str());
	}
}
