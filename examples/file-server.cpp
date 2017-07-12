#include <fstream>
#include <iostream>
#include <unistd.h>

#include <string>
#include <functional>
#include <memory>
#include <utility>
#include <thread>

#include <easynet/EventLoop.h>
#include <easynet/TcpServer.h>
#include <easynet/SignalMgr.h>
#include <easynet/TcpConnection.h>
#include <easynet/utils/log.h>

using std::ifstream;
using std::ios;

using namespace std;
using namespace easynet;

std::unique_ptr<char[]> readFile(const std::string &fileName, int &fileLen)
{
	std::ifstream infile(fileName.c_str());
    if (!infile)
    {
        LOG_ERROR("open file %s error!", fileName.c_str());
        return std::unique_ptr<char[]>();
    }

    infile.seekg(0, ios::end);
	fileLen = infile.tellg();

	LOG_INFO("%s opened, file's length = %d bytes", fileName.c_str(), fileLen);

	std::unique_ptr<char[]> buf(new char[fileLen]);
	infile.clear();
	infile.seekg(0, ios::beg);
	infile.read(buf.get(), fileLen);

	return std::move(buf);
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
        cout << "usage:" << argv[0] << " <file for downloading>" << endl;
        return 0;
	}

	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
	LOG_INFO("----file download server starting.................................");
	SignalMgr::enableSignalHandling();

/*
	thread th([&](){
		while (true) {
			LOG_INFO("i'm runing....")
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	});
	th.detach();
	*/

	char filePath[1024] = {0};
	int fileLen = 0;

    sscanf(argv[1], "%s", filePath);

    LOG_INFO("----file for downloading is %s", filePath);
    std::unique_ptr<char[]> fileBuf = readFile(filePath, fileLen);
    if (fileBuf.get() == nullptr)
    {
    	LOG_INFO("----read file failed");
    	return 0;
    }
    LOG_INFO("----read file successfully, file's length is %d", fileLen);

    unsigned short port = 12258;
    TcpServer tcpServer(port);
	tcpServer.setNewTcpConnectionHandler([&](TcpConnection &tcpConn){
		LOG_INFO("----accpeted connection from %s, begin to download file...", 
			tcpConn.getPeerAddr().toString().c_str());
		tcpConn.send(fileBuf.get(), fileLen);
	});

	tcpServer.setWriteCompleteHandler([&](TcpConnection &tcpConn){
		LOG_INFO("----send file to %s finished", tcpConn.getPeerAddr().toString().c_str());
		tcpConn.shutdownWrite();
	});

	tcpServer.setReadHandler([&](TcpConnection &tcpConn){
		// discard recieved message
		Buffer &buffer = tcpConn.getInputBuffer();
		buffer.deleteBegin(buffer.size());
	});

	tcpServer.setPeerShutdownHandler([&](TcpConnection &tcpConn){
		LOG_INFO("----connection from %s is closed", tcpConn.getPeerAddr().toString().c_str());
		tcpConn.close();
	});

	tcpServer.setDisconnectedHandler([&](TcpConnection &tcpConn){
		LOG_INFO("----connection from %s is disconnected, error:%d %s", 
			tcpConn.getPeerAddr().toString().c_str(), tcpConn.getErrNo(), tcpConn.getErrMsg().c_str());
	});

    tcpServer.start();

	EventLoop loop;
	loop.addSignalHandler(SIGPIPE, [&]{
		LOG_INFO("SIGPIPE caught");
	});

	SignalHandler *handler = loop.addSignalHandler(SIGINT, [&]{
		LOG_INFO("SIGINT caught");
		loop.quit();
		handler->close();
	});

	loop.loop();

	LOG_INFO("----file download server exiting.................................");
	return 0;
}
