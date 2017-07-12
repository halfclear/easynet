// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_SIGNAL_MGR_H_
#define _EASYNET_SIGNAL_MGR_H_

#include <signal.h>

#include <list>
#include <map>
#include <atomic>
#include <functional>
#include <memory>
#include <utility>
#include <thread>

#include "EventLoop.h"
#include "Channel.h"
#include "Socket.h"

namespace easynet
{

class SignalMgr
{
public:
	~SignalMgr() = default;

	static SignalMgr& getInstance();
	static void enableSignalHandling();
	static const std::string& getSignalName(int sig);

	void registerSignalListener(int sig, EventLoop *listener) 
	{ loop_.wakeupAndRun(std::bind(&SignalMgr::registerSignalListenerInLoop, this, sig, listener)); }

	void unregisterSignalListener(int sig, EventLoop *listener)
	{ loop_.wakeupAndRun(std::bind(&SignalMgr::unregisterSignalListenerInLoop, this, sig, listener)); }
  
private:
	using SignalListenerMap = std::map<int, std::list<EventLoop*>>;
	using SigAcitonHandler  = void (*)(int);

	void start();

    static void initSignalMgrOnce(std::unique_ptr<SignalMgr> &signalMgr);
    static void initSignalNameOnce(std::map<int, std::string> &signalNameMap);
	static void blockAllSignals();
	static void unblockAllSignals();
	static void blockAllSignals(int how);

	SignalMgr(int fd);
	SignalMgr(const SignalMgr &rhs) = delete;
	SignalMgr& operator=(const SignalMgr &rhs) = delete;
    
	void run();
	void onSignal();
	void registerSignalListenerInLoop(int sig, EventLoop *listener);
	void unregisterSignalListenerInLoop(int sig, EventLoop *listener);

	int setSignalHandler(int sig, SigAcitonHandler handler);
	void restoreSignalHandler(int sig);
	
	static std::once_flag initedFlag_;
	
	EventLoop loop_;

	Socket 	sigNotifyUnixSocket_;
	Channel sigNotifyChnl_;

	SignalListenerMap signalListenerMap_;
	std::map<int, struct sigaction> oldSaMap_;
};

} // namespace easynet

#endif
