// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_ACCEPTOR_H_
#define _EASYNET_ACCEPTOR_H_

#include <functional>
#include <utility>
#include "Channel.h"
#include "InetAddr.h"

#define EASYNET_ACCEPT_NEXT           0
#define EASYNET_SUGGEST_STOP_ACCEPT   1
#define EASYNET_STOP_ACCEPT           2

namespace easynet
{

class EventLoop;
class Socket;

class Acceptor
{
public:
	using NewConnectionHandler = std::function<int (Socket &&socket, const InetAddr &peerAddr)>;

	Acceptor(EventLoop *loop, Socket *listenSocket, int minAcceptsPerCall, int maxAcceptsPerCall);
	Acceptor(const Acceptor &rhs) = delete;
	~Acceptor() = default;
	Acceptor& operator=(const Acceptor &rhs) = delete;

	void setNewConnectionHandler(NewConnectionHandler &&hanlder)      { newConnectionHandler_ = std::move(hanlder); }
	void setNewConnectionHandler(const NewConnectionHandler &hanlder) { setNewConnectionHandler(NewConnectionHandler(hanlder)); }

	void enableListening();
	void disableListening();

private:
	void onConnectionArrived();

	EventLoop *loop_;
	Socket  *listenSocket_;
	InetAddr listenAddr_;
	Channel  channel_;
	bool     listening_;
	int minAcceptsPerCall_;
	int maxAcceptsPerCall_;

	NewConnectionHandler newConnectionHandler_;
};

}

#endif
