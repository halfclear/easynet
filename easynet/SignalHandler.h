// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_SIGNAL_HANDLER_H_
#define _EASYNET_SIGNAL_HANDLER_H_

#include <list>
#include <memory>
#include <functional>
#include <utility>

namespace easynet
{

class SignalHandlerMgr;

class SignalHandler
{
public:
	friend class SignalHandlerMgr;
	using SigHandler = std::function<void ()>;

    ~SignalHandler() = default;
	SignalHandler(const SignalHandler &rhs) = delete;
	SignalHandler& operator=(SignalHandler &rhs) = delete;

	int getSignal() const { return sig_; }

    // can only be called in event loop
	void close();

private:
	using SignalHandlerPos = std::list<std::unique_ptr<SignalHandler>>::iterator;

	SignalHandler(SignalHandlerMgr *signalHandlerMgr, int sig, const SigHandler &handler)
		: SignalHandler(signalHandlerMgr, sig, SigHandler(handler))
	{}

	SignalHandler(SignalHandlerMgr *signalHandlerMgr, int sig, SigHandler &&handler) 
		: signalHandlerMgr_(signalHandlerMgr),
		  sig_(sig),
		  handler_(std::move(handler))
	{}
	
	void setPos(const SignalHandlerPos &pos) { pos_ = pos; }
	const SignalHandlerPos& getPos() const { return pos_; }
	void onSignal() { if(handler_) handler_(); }

	SignalHandlerMgr *signalHandlerMgr_;
	int sig_;
	SignalHandlerPos pos_; // position(iterator) in the signal handler map
	SigHandler handler_;
};

} // namespace easynet

#endif
