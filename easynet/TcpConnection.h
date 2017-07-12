// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_TCP_CONNECTION_H_
#define _EASYNET_TCP_CONNECTION_H_

#include <string>
#include <memory>
#include <utility>

#include "Socket.h"
#include "Buffer.h"
#include "Channel.h"
#include "InetAddr.h"
#include "Timer.h"

namespace easynet
{

class EventLoop;

class TcpConnection
{
public:
	using TcpConnectionHandler = std::function<void (TcpConnection&)>;
	using TimerHandler = Timer::TimerHandler;

	TcpConnection(EventLoop *loop); // empty tcp connection
	TcpConnection(EventLoop *loop, Socket &&socket);
	~TcpConnection() { clear(); }

	TcpConnection(const TcpConnection &rhs) = delete;
	TcpConnection& operator=(const TcpConnection &rhs) = delete;

	EventLoop* getLoop() const { return loop_; }
	int fd() const { return socket_.fd(); }

	int setNoDelay(bool on) { return socket_.setNoDelay(on); }
	int setLinger(bool on, int lingerTime) { return socket_.setLinger(on, lingerTime); }
	int setRecvBuf(int size) { return socket_.setRecvBuf(size); }
	int setSendBuf(int size) { return socket_.setSendBuf(size); }

	void setReadHandler(TcpConnectionHandler &&handler)      { readHandler_ = std::move(handler); }
	void setReadHandler(const TcpConnectionHandler &handler) { setReadHandler(TcpConnectionHandler(handler)); }

	void setWriteCompleteHandler(TcpConnectionHandler &&handler)      { writeCompleteHandler_ = std::move(handler); }
	void setWriteCompleteHandler(const TcpConnectionHandler &handler) { setWriteCompleteHandler(TcpConnectionHandler(handler)); }

	void setPeerShutdownHandler(TcpConnectionHandler &&handler)      { peerShutdownHandler_ = std::move(handler); }
	void setPeerShutdownHandler(const TcpConnectionHandler &handler) { setPeerShutdownHandler(TcpConnectionHandler(handler)); }

	void setDisconnectedHandler(TcpConnectionHandler &&handler)      { disconnectedHandler_ = std::move(handler); }
	void setDisconnectedHandler(const TcpConnectionHandler &handler) { setDisconnectedHandler(TcpConnectionHandler(handler)); }

	void setCloseHandler(TcpConnectionHandler &&handler)      { closeHandler_ = std::move(handler); }
	void setCloseHandler(const TcpConnectionHandler &handler) { setCloseHandler(TcpConnectionHandler(handler)); }

	void setIdleHandler(int idleSecs, TimerHandler &&handler);
	void setIdleHandler(int idleSecs, const TimerHandler &handler) { setIdleHandler(idleSecs, TimerHandler(handler)); }

	void  setData(void *data) { data_ = data; }
	void* getData() const { return data_; }

	const InetAddr& getLocalAddr() const { return localAddr_; }
	const InetAddr& getPeerAddr() const { return peerAddr_; }

	Buffer& getInputBuffer()  { return inputBuffer_;  }
	Buffer& getOutputBuffer() { return outputBuffer_; }

	int64_t getEstablishmentTime() const { return establishedTimeMillis_; }

	int getErrNo() const { return errNo_; }
	std::string getErrMsg() const { return errMsg_; }

    // can only be called in event loop
	void send(const char *data, size_t len); 
	void send(const std::string &data) { send(data.c_str(), data.size()); }

    // can only be called in event loop
    void disableReceiving() { channel_.disableReading(); }
    void enableReceiving()  { channel_.enableReading(); }

    // can only be called in event loop
    void shutdown();
	void shutdownRead(); // can only be called in event loop
	void shutdownWrite(); // can only be called in event loop
	void close();   // can only be called in event loop

	bool closed() const { return closed_; }
    void reset(Socket &&socket, const InetAddr &peerAddr);

private:
	void setChannelHandlers();
	
	void onReadable();
	void onWritable();
	void onPeerShutdown();
	void onPollError();
	void onDisconnected();

    ssize_t recvData();
	ssize_t sendData(const char *buf, size_t len);
	void clear();

	void closeIdleTimer();
	void updateIdleTimer();

	EventLoop *loop_;
	Socket socket_;
	Channel channel_;
	
	Buffer inputBuffer_;
	Buffer outputBuffer_;

    InetAddr localAddr_;
    InetAddr peerAddr_;

    bool closed_;
    int64_t establishedTimeMillis_; // connection establised time, milliseconds since 1970-1-1 00:00:00
	void *data_;   // application's data related to this connection

	int errNo_;
	std::string errMsg_;

	int64_t idleMillis_;
	Timer *idleTimer_;

	TcpConnectionHandler readHandler_;            // application's callback, will be called when data is read from the socket
	TcpConnectionHandler writeCompleteHandler_;   // application's callback, will be called when all data is wroted to the socket
	TcpConnectionHandler disconnectedHandler_;    // application's callback, will be called when this connection is disconnected(such as peer socket hup, or error occurred )
	TcpConnectionHandler peerShutdownHandler_;    // application's callback, will be called when peer shutdown

	TcpConnectionHandler closeHandler_;         // framework(easynet)'s callback, when the connection is closed, will call this function to free this object
};

} // namespace easynet

#endif
