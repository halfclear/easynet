// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _SIGNAL_HANDLER_MGR_H_
#define _SIGNAL_HANDLER_MGR_H_

#include <list>
#include <map>
#include <memory>

#include "SignalHandler.h"

namespace easynet
{

class EventLoop;

class SignalHandlerMgr
{
public:
	using SigHandler = SignalHandler::SigHandler;
	 
	SignalHandlerMgr(EventLoop *loop) 
	               : loop_(loop), 
	                 signalHandling_(false),
	                 curSig_(0)
	{}
	
	~SignalHandlerMgr() = default;

	SignalHandlerMgr(const SignalHandlerMgr &rhs) = delete;
	SignalHandlerMgr& operator=(const SignalHandlerMgr &rhs) = delete;

	SignalHandler* addSignalHandler(int sig, SigHandler &&handler);
	SignalHandler* addSignalHandler(int sig, const SigHandler &handler) { return addSignalHandler(sig, SigHandler(handler)); }
	void deleteSignalHandler(SignalHandler *signalHandler);
	void signalRaised(int sig);

private:
    // key: signal number
	using SignalHandlerContainer = std::map<int, std::list<std::unique_ptr<SignalHandler>>>;

	void onSignal(int sig);

	EventLoop *loop_;
	SignalHandlerContainer signalHandlers_;
	bool signalHandling_;
	int  curSig_;
};

}

#endif