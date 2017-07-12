// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <cstring>

#include "SignalHandlerMgr.h"
#include "EventLoop.h"
#include "SignalMgr.h"
#include "utils/log.h"

using namespace easynet;

SignalHandler* SignalHandlerMgr::addSignalHandler(int sig, SigHandler &&handler)
{
	LOG_TRACE("add signalHandler for signal %d %s(%s)", 
		sig, SignalMgr::getInstance().getSignalName(sig).c_str(), ::strsignal(sig));

	if (sig <= 0 || sig >= NSIG || (sig > SIGSYS && sig < SIGRTMIN))
	{
		LOG_WARN("addSignalHandler failed: invalid signal num:%d", sig);
		return nullptr;
	}

	if (sig == SIGKILL || sig == SIGSTOP)
	{
		LOG_WARN("addSignalHandler for signal %d %s(%s) failed, can't add signal handler for this signal", 
			sig, SignalMgr::getInstance().getSignalName(sig).c_str(), ::strsignal(sig));
		return nullptr;
	}

	std::unique_ptr<SignalHandler> signalHandler(new SignalHandler(this, sig, std::move(handler)));

	auto it = signalHandlers_.find(sig);
	if (it == signalHandlers_.end())
	{
		SignalMgr::getInstance().registerSignalListener(sig, loop_);
		signalHandlers_[sig] = std::list<std::unique_ptr<SignalHandler>>();
		it = signalHandlers_.find(sig);
	}
	
	SignalHandler *p = signalHandler.get();
	auto &container = it->second;
	container.push_back(std::move(signalHandler));
	p->setPos(--container.end());

	return p;
}

void SignalHandlerMgr::deleteSignalHandler(SignalHandler *signalHandler)
{
	int sig = signalHandler->getSignal();
	auto it = signalHandlers_.find(sig);
	auto &container = it->second;

    std::unique_ptr<SignalHandler> handler(std::move(*(signalHandler->getPos())));

    // if deleteSignalHandler() is called in @signalHandler's signal handler, 
    // here we can't delete handler from @signalHandlers_
    if (!signalHandling_ || sig != curSig_)
    {
    	container.erase(signalHandler->getPos());
	    if (container.empty())
	    {
		    SignalMgr::getInstance().unregisterSignalListener(sig, loop_);
		    signalHandlers_.erase(it);
        }
    }
}

void SignalHandlerMgr::signalRaised(int sig) 
{ 
	loop_->wakeupAndRun(std::bind(&SignalHandlerMgr::onSignal, this, sig));
}

void SignalHandlerMgr::onSignal(int sig)
{
	auto it = signalHandlers_.find(sig);
	if (it != signalHandlers_.end())
	{
		auto &container = it->second;
		signalHandling_ = true;
		curSig_ = sig;
		for (auto jt = container.begin(); jt != container.end();)
		{
			(*jt).get()->onSignal();
			// the signal handler called close() to delete itself from the SignalHandlerMgr
			if ((*jt).get() == nullptr)
			{
				jt = container.erase(jt);
			}
			else
			{
				++jt;
			}
		}
		signalHandling_ = false;
		curSig_ = 0;

		if (container.empty())
		{
			SignalMgr::getInstance().unregisterSignalListener(sig, loop_);
		    signalHandlers_.erase(it);
		}
	}	
}
