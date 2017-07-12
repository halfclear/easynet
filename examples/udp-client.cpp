#include <string>
#include <functional>

#include <easynet/UdpServer.h>
#include <easynet/EventLoop.h>
#include <easynet/SignalMgr.h>
#include <easynet/UdpConnection.h>
#include <easynet/utils/log.h>

using namespace std;
using namespace easynet;

int main(int argc, char* argv[])
{
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
	LOG_INFO("----udp client starting.................................");

	std::string ip = "127.0.0.1";
	unsigned short port = 12260;
	std::string msg("hello easynet!");

	EventLoop loop;
    UdpConnection udpConnection(&loop);
    
    udpConnection.setMessageHandler([&](UdpConnection &udpConnection){
	    Buffer &buffer = udpConnection.getInputBuffer();
	    
		LOG_INFO("----received udp message from server %s:%s", 
			udpConnection.getPeerAddr().toString().c_str(), buffer.data());

        buffer.deleteBegin(buffer.size());
        loop.quit();
	});

	udpConnection.setErrorHandler([&](UdpConnection &udpConnection, int errNo, const std::string& errMsg){
		LOG_INFO("----udp from %s at %s error:%d %s",  udpConnection.getPeerAddr().toString().c_str()
			, udpConnection.getLocalAddr().toString().c_str(), errNo, errMsg.c_str());
		loop.quit();
	});

    LOG_INFO("----client send udp message to %s:%d:%s",  ip.c_str(), port, msg.c_str());
	udpConnection.sendTo(msg, ip, port);

    loop.loop();

	LOG_INFO("----udp client exiting.................................");
    return 0;
}
