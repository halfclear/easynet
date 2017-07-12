#include <string>
#include "Buffer.h"
#include <test_harness.h>

using namespace std;
using namespace easynet;

TEST(Buffer, BasicTest)
{
    Buffer buf;

    EXPECT_EQ(buf.size(), 0);
    EXPECT_EQ(buf.capacity(), Buffer::kBufferSizeDefault);
    ASSERT_TRUE(buf.empty());
    EXPECT_EQ(buf.begin(), buf.data());
    EXPECT_EQ(buf.begin(), buf.end());

    string str1(200, 's');
    buf.append(str1.c_str(), str1.size());
    ASSERT_FALSE(buf.empty());
    EXPECT_EQ(buf.size(), str1.size());
    EXPECT_EQ(buf.begin(), buf.data());
    EXPECT_EQ(buf.end()-buf.begin(), buf.size());

    string str1Copy(buf.begin(), buf.size());
    EXPECT_EQ(str1Copy, str1);

    string str2(100, 'x');
    buf.append(str2.c_str(), str2.size());
    EXPECT_EQ(buf.size(), str1.size() + str2.size());
    EXPECT_EQ(buf.begin(), buf.data());
    EXPECT_EQ(buf.end()-buf.begin(), buf.size());

    string str1and2 = str1 + str2;
    string str1and2Copy(buf.begin(), buf.size());
    EXPECT_EQ(str1and2, str1and2Copy);

    buf.clear();
    ASSERT_TRUE(buf.empty());
    EXPECT_EQ(0, buf.size());

    EXPECT_EQ(buf.begin(), buf.data());
    EXPECT_EQ(buf.begin(), buf.end());

    buf.append(str1.c_str(), str1.size());
    EXPECT_EQ(buf.size(), str1.size());
    buf.deleteBegin(50);
    EXPECT_EQ(buf.size(), 150);
    buf.deleteBegin(150);
    EXPECT_EQ(buf.size(), 0);
    ASSERT_TRUE(buf.empty());
}

TEST(Buffer, testCopy)
{
    Buffer buf1;
    string str1(200, 's');
    buf1.append(str1.c_str(), str1.size());

    Buffer dstBuf1(buf1);
    EXPECT_EQ(buf1.size(), str1.size());
    EXPECT_EQ(dstBuf1.size(), buf1.size());
    EXPECT_EQ(dstBuf1.capacity(), buf1.capacity());
    EXPECT_EQ(dstBuf1.begin(), dstBuf1.data());
    EXPECT_EQ(dstBuf1.end()-dstBuf1.begin(), dstBuf1.size());

    string dstStr1(dstBuf1.begin(), dstBuf1.size());
    EXPECT_EQ(str1, dstStr1);

    string str2(100, 'a');
    Buffer dstBuf2;
    dstBuf2.append(str2.c_str(), str2.size());
    dstBuf2 = buf1;
    EXPECT_EQ(dstBuf2.size(), buf1.size());
    EXPECT_EQ(dstBuf2.capacity(), buf1.capacity());

    string dstStr2(dstBuf2.begin(), dstBuf2.size());
    EXPECT_EQ(dstStr2, str1);
}

TEST(Buffer, testMove)
{
    Buffer buf1;
    string str1(200, 's');
    buf1.append(str1.c_str(), str1.size());

    Buffer dstBuf1(std::move(buf1));
    ASSERT_TRUE(buf1.empty());
    EXPECT_EQ(buf1.size(), 0);
    EXPECT_EQ(buf1.begin(), buf1.data());
    EXPECT_EQ(buf1.begin(), buf1.end());

    ASSERT_FALSE(dstBuf1.empty());
    EXPECT_EQ(dstBuf1.size(), str1.size());
    EXPECT_EQ(dstBuf1.begin(), dstBuf1.data());
    EXPECT_EQ(dstBuf1.end()-dstBuf1.begin(), dstBuf1.size());

    string str1Copy(dstBuf1.begin(), dstBuf1.size());
    EXPECT_EQ(str1Copy, str1);

    string str2(100, 'x');
    Buffer dstBuf2;
    dstBuf2.append(str2.c_str(), str2.size());

    dstBuf2 = std::move(dstBuf1);
    ASSERT_TRUE(dstBuf1.empty());
    EXPECT_EQ(dstBuf1.size(), 0);
    EXPECT_EQ(dstBuf1.begin(), dstBuf1.data());
    EXPECT_EQ(dstBuf1.end(), dstBuf1.begin());

    ASSERT_FALSE(dstBuf2.empty());
    EXPECT_EQ(dstBuf2.size(), str1.size());
    EXPECT_EQ(dstBuf2.begin(), dstBuf2.data());
    EXPECT_EQ(dstBuf2.end()-dstBuf2.begin(), dstBuf2.size());

    string str2Copy(dstBuf2.begin(), dstBuf2.size());
    EXPECT_EQ(str2Copy, str1);
}

TEST(Buffer, testGrouth)
{
    Buffer buf;
    string str1(Buffer::kBufferSizeDefault, 'a');

    buf.append(str1.c_str(), str1.size());
    EXPECT_EQ(buf.capacity(), Buffer::kBufferSizeDefault);

    string str2(10, 'b');
    buf.append(str2.c_str(), str2.size());
    EXPECT_EQ(buf.capacity(), Buffer::kBufferSizeDefault*2);

    string strcopy(buf.data(), buf.size());
    string str3 = str1 + str2;
    EXPECT_EQ(strcopy, str3);
}

TEST(Buffer, testSwap)
{
    Buffer buf1;
    string str1(200, 'a');
    buf1.append(str1.c_str(), str1.size());

    Buffer buf2;
    string str2(100, 'b');
    buf2.append(str2.c_str(), str2.size());

    buf1.swap(buf2);
    EXPECT_EQ(buf1.size(), str2.size());
    EXPECT_EQ(buf2.size(), str1.size());

    string buf1str(buf1.data(), buf1.size());
    string buf2str(buf2.data(), buf2.size());

    EXPECT_EQ(buf1str, str2);
    EXPECT_EQ(buf2str, str1);
}

TEST(Buffer, testTake)
{
    Buffer buffer;
    string str1(200, 'a');
    string str2(100, 'b');
    string str3(150, 'c');

    buffer.append(str1.c_str(), str1.size());    
    buffer.append(str2.c_str(), str2.size());
    buffer.append(str3.c_str(), str3.size());

    char buf[1024];

    buffer.take(buf, str1.size());
    string takedStr1(buf, str1.size());
    EXPECT_EQ(takedStr1, str1);

    buffer.takeAll(buf);
    string takedStr2(buf, str2.size() + str3.size());
    EXPECT_EQ(takedStr2, (str2+str3));
}
