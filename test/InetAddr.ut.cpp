#include <string>
#include <iostream>
#include <thread>
#include <future>
#include <functional>

#include "TimeWheel.h"
#include "EventLoop.h"
#include "InetAddr.h"
#include "utils/TimeUtil.h"

#include <test_harness.h>


#include "utils/log.h"

using namespace std;
using namespace easynet;

TEST(InetAddr, testInetAddr)
{
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
	LOG_INFO("-----------------------------------------------------");
    LOG_INFO("InetAddr-testInetAddr");
    LOG_INFO("-----------------------------------------------------");

	InetAddr addr;
	LOG_INFO("addr=%s", addr.toString().c_str());
	EXPECT_EQ(0, addr.port());
	EXPECT_EQ("0.0.0.0", addr.ip());
	EXPECT_EQ("0.0.0.0:0", addr.toString());
	ASSERT_TRUE(addr.isIpWildcard());
	ASSERT_TRUE(addr.isAddrNone());

	InetAddr addr1("*", 80);
	LOG_INFO("addr1=%s", addr1.toString().c_str());
	EXPECT_EQ(80, addr1.port());
	EXPECT_EQ("0.0.0.0", addr1.ip());
	EXPECT_EQ("0.0.0.0:80", addr1.toString());
	ASSERT_TRUE(addr1.isIpWildcard());
	ASSERT_FALSE(addr == addr1);

	InetAddr addr2(65535);
	LOG_INFO("addr2=%s", addr2.toString().c_str());
	EXPECT_EQ(65535, addr2.port());
	EXPECT_EQ("0.0.0.0", addr2.ip());
	EXPECT_EQ("0.0.0.0:65535", addr2.toString());
	ASSERT_TRUE(addr2.isIpWildcard());

	InetAddr addr3("127.0.0.1", 8088);
	LOG_INFO("addr3=%s", addr3.toString().c_str());
	EXPECT_EQ(8088, addr3.port());
	EXPECT_EQ("127.0.0.1", addr3.ip());
	EXPECT_EQ("127.0.0.1:8088", addr3.toString());
	ASSERT_FALSE(addr3.isIpWildcard());

	InetAddr addr4("10.94.98.190", 12345);
	LOG_INFO("addr4=%s", addr4.toString().c_str());
	EXPECT_EQ(12345, addr4.port());
	EXPECT_EQ("10.94.98.190", addr4.ip());
	EXPECT_EQ("10.94.98.190:12345", addr4.toString());
	ASSERT_FALSE(addr4.isIpWildcard());

	InetAddr addr5("255.255.255.255", 1);
	LOG_INFO("addr5=%s", addr5.toString().c_str());
	EXPECT_EQ(1, addr5.port());
	EXPECT_EQ("255.255.255.255", addr5.ip());
	EXPECT_EQ("255.255.255.255:1", addr5.toString());
	ASSERT_FALSE(addr5.isIpWildcard());

	InetAddr addr6("localhost", 9091);
	LOG_INFO("addr6=%s", addr6.toString().c_str());
	EXPECT_EQ(9091, addr6.port());
	EXPECT_EQ("127.0.0.1", addr6.ip());
	EXPECT_EQ("127.0.0.1:9091", addr6.toString());
	ASSERT_FALSE(addr6.isIpWildcard());

	addr = addr4;
	LOG_INFO("addr=%s", addr.toString().c_str());
	EXPECT_EQ(12345, addr.port());
	EXPECT_EQ("10.94.98.190", addr.ip());
	EXPECT_EQ("10.94.98.190:12345", addr.toString());
	ASSERT_FALSE(addr.isIpWildcard());
    
    ASSERT_TRUE(addr == addr4);
}
