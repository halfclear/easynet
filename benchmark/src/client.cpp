#include <easynet/EventLoop.h>
#include <easynet/TcpClient.h>
#include <easynet/TcpConnection.h>
#include <easynet/utils/log.h>
#include <easynet/SignalMgr.h>
#include <memory>
#include <cstring>
#include <utility>
#include <string>
#include <vector>
#include <random>
#include <iostream>

using namespace std;
using namespace easynet;

std::unique_ptr<char[]> genContent(size_t size)
{
	std::unique_ptr<char[]> buf(new char[size]);
	std::memset(buf.get(), 0, size);

	std::uniform_int_distribution<unsigned> u(0, 51);
	default_random_engine e(TimeUtil::now());
	//int a = 'a';
	//int z = 'z';
	//int A = 'A';
	//int Z = 'Z';
	//LOG_INFO("a=%d, z=%d, A=%d, Z=%d", a, z, A, Z);
	char *p = buf.get(); 
	for (size_t i = 0; i < size; i++)
	{
		unsigned int index = u(e);
		char c = 0;
		if (index < 26) {
			c = 'a' + index;
		} else {
			index -= 26;
			c = 'A' + index;
		}
		p[i] = c;
	}

	return std::move(buf);
}

int main(int argc, const char* argv[])
{
	if (argc < 5)
	{
        cout << "usage:" << argv[0] << " <host_ip> <port> <concurrency> <test_time(seconds)>" << endl;
        return 0;
	}

	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
	Logger::getInstance().setLogFile("client-log.txt");
	LOG_INFO("client starting, to test echo server.................................");

	char serverIp[32] = {0};
	unsigned int serverPort = 0;
	int concurrency = 0;
	int testTimeSecs = 0;
	int maxTryTimeSec = 10;

    sscanf(argv[1], "%s", serverIp);
    sscanf(argv[2], "%d", &serverPort);
	sscanf(argv[3], "%d", &concurrency);
	sscanf(argv[4], "%d", &testTimeSecs);

    std::string hostIp(serverIp);
	unsigned short port = static_cast<unsigned short>(serverPort);

	LOG_INFO("to test server %s:%d, concurrency %d, test time %d seconds", hostIp.c_str(), port, concurrency, testTimeSecs);

	int maxMsgLen = 512;
    std::unique_ptr<char[]> buf(genContent(maxMsgLen));

    uniform_int_distribution<unsigned> u(10, maxMsgLen-1);
	default_random_engine e(TimeUtil::now());

	EventLoop loop;
    int64_t begin = loop.now();

    int totalRequest = 0;
	int totalResponse = 0;
	int connectFailedCnt = 0;
	int64_t totalLatency = 0;
	std::string msg("hello world");

	std::vector<std::unique_ptr<TcpClient>> v;
	for (int i = 0; i < concurrency; i++)
	{
		std::unique_ptr<TcpClient> client(new TcpClient(&loop));
		TcpClient *tcpClient = client.get();

		tcpClient->setConnectedHandler([&](TcpConnection &tcpConn){
			int msgLen = u(e);
		//	LOG_INFO("--- connected to echo server %s:%d , msgLen:%d", 
		//	    hostIp.c_str(), port, msgLen);
			tcpConn.send(buf.get(), msgLen);
		//	tcpConn.send(msg);
			totalRequest++;
		});

		tcpClient->setConnectingErrorHandler([&, tcpClient](int errNo, const std::string &errMsg){
			LOG_INFO("--- connect to echo server %s:%d failed, error:%d %s", 
			    hostIp.c_str(), port, errNo, errMsg.c_str());
		    connectFailedCnt++;
		    LOG_INFO("connectFailedCnt %d, tcpClient=0x%x", connectFailedCnt, tcpClient);
	        tcpClient->connect(hostIp, port, maxTryTimeSec);
		});

		tcpClient->setConnectingTimeoutHandler([&, tcpClient](){
			LOG_INFO("--- connect to echo server %s:%d timed out", hostIp.c_str(), port);
		    connectFailedCnt++;
	        tcpClient->connect(hostIp, port, maxTryTimeSec);
		});

		tcpClient->setReadHandler([&, tcpClient](TcpConnection &tcpConn){
			Buffer &buffer = tcpConn.getInputBuffer();
			buffer.deleteBegin(buffer.size());

			totalResponse++;
			totalLatency += (loop.now() - tcpConn.getEstablishmentTime());
			tcpConn.close();

			tcpClient->close();
		    tcpClient->connect(hostIp, port, maxTryTimeSec);
		});

		tcpClient->setPeerShutdownHandler([&, tcpClient](TcpConnection &tcpConn){
			LOG_INFO("--- echo server %s:%d closed connection.", hostIp.c_str(), port);
			tcpConn.close();

			tcpClient->connect(hostIp, port, maxTryTimeSec);
		});

		tcpClient->setDisconnectedHandler([&, tcpClient](TcpConnection &tcpConn){
			LOG_INFO("---connection to server %s:%d is disconnected, may be error occurred, error %d %s", 
			    hostIp.c_str(), port, tcpConn.getErrNo(), tcpConn.getErrMsg().c_str());
			tcpConn.close();
			tcpClient->connect(hostIp, port, maxTryTimeSec);
		});

		v.push_back(std::move(client));
	}

	for (auto &client : v)
	{
		client->connect(hostIp, port, maxTryTimeSec);
	}

    loop.runAfter(1000 * testTimeSecs, [&]{
		int64_t end = loop.now();
		for (auto &client : v)
		{
			client->close();
		}

		int64_t testTime = (end - begin) / 1000;
		double avrLantency = totalLatency;
		if (totalResponse > 0) {
			avrLantency = avrLantency / totalResponse;
		} else {
			avrLantency = 0;
		}
		
		LOG_INFO("totalRequest=%d, totalResponse=%d, connectFailedCnt=%d, avrLantency=%2.f millis, qps=%d, testTime=%d secs", 
			totalRequest, totalResponse, connectFailedCnt, avrLantency, totalResponse / testTimeSecs,
			testTime);

		cout << "totalRequest=" << totalRequest << ", totalResponse=" << totalResponse 
		     << ", failed count=" << connectFailedCnt << ", totaltime="
		     << testTime << " seconds, qps=" << totalResponse / testTime << ", avrLantency=" << avrLantency
		      << " millis" << endl;

		loop.quit();
	});

	loop.loop();
}
