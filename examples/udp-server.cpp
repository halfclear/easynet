#include <string>

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
	LOG_INFO("---udp server starting.................................");
	SignalMgr::enableSignalHandling();

    unsigned short port = 12260;

    UdpServer udpServer(port);
	udpServer.setMessageHandler([&](UdpConnection &udpConnection){
		Buffer &buffer = udpConnection.getInputBuffer(); 
		LOG_INFO("---received udp message from %s:%s",  udpConnection.getPeerAddr().toString().c_str()
			, buffer.data());

		udpConnection.sendTo(buffer.data(), buffer.size(), udpConnection.getPeerAddr());
		buffer.deleteBegin(buffer.size());
	});

	udpServer.setErrorHandler([&](UdpConnection &udpConnection, int errNo, const std::string& errMsg){
		LOG_INFO("---udp from %s at %s error:%d %s",  udpConnection.getPeerAddr().toString().c_str()
			, udpConnection.getLocalAddr().toString().c_str(), errNo, errMsg.c_str());
	});

	udpServer.start();

	EventLoop loop;
	loop.loop();

	LOG_INFO("----udp server exiting.................................");
	return 0;
}
