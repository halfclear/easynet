// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <sys/socket.h>
#include <signal.h>

#include <cstring>
#include <thread>
#include <functional>
#include <algorithm>

#include "SignalMgr.h"
#include "utils/log.h"

using namespace easynet;

namespace
{

std::unique_ptr<Socket> kSigNotifyUnixSocket;

}

std::once_flag SignalMgr::initedFlag_;

static void sigalHandler(int sig);

SignalMgr::SignalMgr(int fd)
			: sigNotifyUnixSocket_(fd),
		  	  sigNotifyChnl_(&loop_, fd)
{
	sigNotifyUnixSocket_.setNonBlocking(true);
	sigNotifyUnixSocket_.setCloseOnExec(true);
	sigNotifyChnl_.setReadHandler(std::bind(&SignalMgr::onSignal, this));
	sigNotifyChnl_.enableReading();
}

SignalMgr& SignalMgr::getInstance()
{
	static std::unique_ptr<SignalMgr> signalMgr;
	std::call_once(initedFlag_, std::bind(&SignalMgr::initSignalMgrOnce, std::ref(signalMgr)));

	return *(signalMgr.get());
}

void SignalMgr::initSignalMgrOnce(std::unique_ptr<SignalMgr> &signalMgr)
{
	LOG_TRACE("init SignalMgr");

	int fds[2];
	if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
	{
		int savedErrno = errno;
		LOG_FATAL("socketpair() error. error:%d %s", savedErrno, ::strerror(savedErrno));
		::exit(1);
	}

	kSigNotifyUnixSocket.reset(new Socket(fds[1]));
	kSigNotifyUnixSocket->setNonBlocking(true);
	kSigNotifyUnixSocket->setCloseOnExec(true);

	signalMgr.reset(new SignalMgr(fds[0]));
}

void SignalMgr::enableSignalHandling()
{
	blockAllSignals();
	getInstance().start();
	LOG_INFO("signal handling enabled");
}

void SignalMgr::start()
{
	std::thread(std::bind(&SignalMgr::run, this)).detach();
}

void SignalMgr::run()
{
	unblockAllSignals();

	loop_.loop();
}

void SignalMgr::registerSignalListenerInLoop(int sig, EventLoop *listener)
{
	auto it = signalListenerMap_.find(sig);
	if (it != signalListenerMap_.end()) // signal @sig has beed added
	{
		auto &container = it->second;
		auto jt = std::find(container.begin(), container.end(), listener);
		if (jt == container.end())
		{
			container.push_back(listener);
		}		
	}
	else
	{
		if (setSignalHandler(sig, sigalHandler) == 0)
		{
			std::list<EventLoop*> container;
		    container.push_back(listener);
		    signalListenerMap_[sig] = std::move(container);
		}
	}
}

void SignalMgr::unregisterSignalListenerInLoop(int sig, EventLoop *listener)
{
	auto it = signalListenerMap_.find(sig);
	if (it != signalListenerMap_.end())
	{
		auto &container = it->second;
		auto jt = std::find(container.begin(), container.end(), listener);
		if (jt != container.end())
		{
			container.erase(jt);
		}

		if (container.empty())
		{
			restoreSignalHandler(sig);
			signalListenerMap_.erase(it);		
		}
	}	
}

int SignalMgr::setSignalHandler(int sig, SigAcitonHandler handler)
{
	struct sigaction sa;
	struct sigaction oldsa;
	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	sa.sa_flags |= SA_RESTART;
	::sigfillset(&sa.sa_mask); // block all signals when during the calling of signal handler

	int ret = ::sigaction(sig, &sa, &oldsa);
	if (ret == 0)
	{
		oldSaMap_[sig] = oldsa;  // save the old signal handler of signal @sig
	}
	else  // failed
	{
		int savedErrno = errno;
		LOG_ERROR("set signal handler failed, signal %d %s(%s). sigaction() error. error:%d %s", 
			sig, getSignalName(sig).c_str(), ::strsignal(sig), savedErrno, ::strerror(savedErrno));
	}

	return ret;
}

void SignalMgr::restoreSignalHandler(int sig)
{
	auto it = oldSaMap_.find(sig);
	if (it != oldSaMap_.end())
	{
		if (::sigaction(sig, &(it->second), nullptr) != 0)
		{
			int savedErrno = errno;
			LOG_ERROR("restore signal handler failed, signal %d %s(%s). sigaction() error. error:%d %s", 
				sig, getSignalName(sig).c_str(), ::strsignal(sig), savedErrno, ::strerror(savedErrno));
		}

		oldSaMap_.erase(it);
	}	
}

void SignalMgr::onSignal()
{
	static char signals[1024];

	int ncaught[NSIG];
	::memset(&ncaught, 0, sizeof(ncaught));
	while (true)
	{
		ssize_t n = sigNotifyUnixSocket_.recv(signals, sizeof(signals));
		if (n == -1)
		{
			if (errno != EAGAIN)
			{
				int savedErrno = errno;
				LOG_ERROR("onSignal(): recv() from unix socket error, error:%d %s", savedErrno, ::strerror(savedErrno));				
			}
			break;
		}
		else if (n == 0)
		{
			break;
		}
		else // n > 0
		{
			for (int i = 0; i < n; ++i) 
			{
				int sig = signals[i];
				LOG_TRACE("signal %d %s(%s) caught", sig, getSignalName(sig).c_str(), ::strsignal(sig));

				if (sig < NSIG)
				{
					ncaught[sig]++;
				}
			}
		}
	}

	for (int sig = 0; sig < NSIG; sig++)
	{
		for(int cnt = 0; cnt < ncaught[sig]; cnt++)
		{
			auto it = signalListenerMap_.find(sig);
			if (it != signalListenerMap_.end())
			{
				auto &container = it->second;
				for (auto loop : container)
				{
					loop->signalRaised(sig);
				}
			}
		}
	}
}

void sigalHandler(int sig)
{
	char signum = sig;
	LOG_TRACE("signal %d %s caught", sig, SignalMgr::getSignalName(sig).c_str());
	kSigNotifyUnixSocket->send(static_cast<char*>(&signum), 1);
}

void SignalMgr::blockAllSignals()
{
	blockAllSignals(SIG_BLOCK);
}

void SignalMgr::unblockAllSignals()
{
	blockAllSignals(SIG_UNBLOCK);
}

void SignalMgr::blockAllSignals(int how)
{
	sigset_t set;
	::sigfillset(&set);

	if (::pthread_sigmask(how, &set, NULL) != 0)
	{
		int savedErrno = errno;
		LOG_ERROR("pthread_sigmask() error, error:%d %s", savedErrno, ::strerror(savedErrno));
	}
}

const std::string& SignalMgr::getSignalName(int sig)
{
	static std::string unknown = "unknown signal";
	static std::map<int, std::string> signalNameMap;
	static std::once_flag onceFlag;

	std::call_once(onceFlag, [&](){
		initSignalNameOnce(signalNameMap);
	});

	auto it = signalNameMap.find(sig);
	if (it != signalNameMap.end())
	{
		return it->second;
	}
	return unknown;
}

void SignalMgr::initSignalNameOnce(std::map<int, std::string> &signalNameMap)
{
    signalNameMap[SIGHUP] = std::string("SIGHUP");
    signalNameMap[SIGINT] = std::string("SIGINT");
    signalNameMap[SIGQUIT] = std::string("SIGQUIT");
    signalNameMap[SIGILL] = std::string("SIGILL");
    signalNameMap[SIGTRAP] = std::string("SIGTRAP");
    signalNameMap[SIGABRT] = std::string("SIGABRT");
    signalNameMap[SIGBUS] = std::string("SIGBUS");
    signalNameMap[SIGFPE] = std::string("SIGFPE");
    signalNameMap[SIGKILL] = std::string("SIGKILL");
    signalNameMap[SIGUSR1] = std::string("SIGUSR1");
    signalNameMap[SIGSEGV] = std::string("SIGSEGV");
    signalNameMap[SIGUSR2] = std::string("SIGUSR2");
    signalNameMap[SIGPIPE] = std::string("SIGPIPE");
    signalNameMap[SIGALRM] = std::string("SIGALRM");
    signalNameMap[SIGTERM] = std::string("SIGTERM");
    signalNameMap[SIGSTKFLT] = std::string("SIGSTKFLT");
    signalNameMap[SIGCHLD] = std::string("SIGCHLD");
    signalNameMap[SIGCONT] = std::string("SIGCONT");
    signalNameMap[SIGSTOP] = std::string("SIGSTOP");
    signalNameMap[SIGTSTP] = std::string("SIGTSTP");
    signalNameMap[SIGTTIN] = std::string("SIGTTIN");
    signalNameMap[SIGTTOU] = std::string("SIGTTOU");
    signalNameMap[SIGURG] = std::string("SIGURG");
    signalNameMap[SIGXCPU] = std::string("SIGXCPU");
    signalNameMap[SIGXFSZ] = std::string("SIGXFSZ");
    signalNameMap[SIGVTALRM] = std::string("SIGVTALRM");
    signalNameMap[SIGPROF] = std::string("SIGPROF");
    signalNameMap[SIGWINCH] = std::string("SIGWINCH");
    signalNameMap[SIGIO] = std::string("SIGIO");
    signalNameMap[SIGPWR] = std::string("SIGPWR");
    signalNameMap[SIGSYS] = std::string("SIGSYS");
    signalNameMap[SIGRTMIN] = std::string("SIGRTMIN");
    signalNameMap[SIGRTMIN+1] = std::string("SIGRTMIN+1");
    signalNameMap[SIGRTMIN+2] = std::string("SIGRTMIN+2");
    signalNameMap[SIGRTMIN+3] = std::string("SIGRTMIN+3");
    signalNameMap[SIGRTMIN+4] = std::string("SIGRTMIN+4");
    signalNameMap[SIGRTMIN+5] = std::string("SIGRTMIN+5");
    signalNameMap[SIGRTMIN+6] = std::string("SIGRTMIN+6");
    signalNameMap[SIGRTMIN+7] = std::string("SIGRTMIN+7");
    signalNameMap[SIGRTMIN+8] = std::string("SIGRTMIN+8");
    signalNameMap[SIGRTMIN+9] = std::string("SIGRTMIN+9");
    signalNameMap[SIGRTMIN+10] = std::string("SIGRTMIN+10");
    signalNameMap[SIGRTMIN+11] = std::string("SIGRTMIN+11");
    signalNameMap[SIGRTMIN+12] = std::string("SIGRTMIN+12");
    signalNameMap[SIGRTMIN+13] = std::string("SIGRTMIN+13");
    signalNameMap[SIGRTMIN+14] = std::string("SIGRTMIN+14");
    signalNameMap[SIGRTMIN+15] = std::string("SIGRTMIN+15");
    signalNameMap[SIGRTMAX-14] = std::string("SIGRTMAX-14");
    signalNameMap[SIGRTMAX-13] = std::string("SIGRTMAX-13");
    signalNameMap[SIGRTMAX-12] = std::string("SIGRTMAX-12");
    signalNameMap[SIGRTMAX-11] = std::string("SIGRTMAX-11");
    signalNameMap[SIGRTMAX-10] = std::string("SIGRTMAX-10");
    signalNameMap[SIGRTMAX-9] = std::string("SIGRTMAX-9");
    signalNameMap[SIGRTMAX-8] = std::string("SIGRTMAX-8");
    signalNameMap[SIGRTMAX-7] = std::string("SIGRTMAX-7");
    signalNameMap[SIGRTMAX-6] = std::string("SIGRTMAX-6");
    signalNameMap[SIGRTMAX-5] = std::string("SIGRTMAX-5");
    signalNameMap[SIGRTMAX-4] = std::string("SIGRTMAX-4");
    signalNameMap[SIGRTMAX-3] = std::string("SIGRTMAX-3");
    signalNameMap[SIGRTMAX-2] = std::string("SIGRTMAX-2");
    signalNameMap[SIGRTMAX-1] = std::string("SIGRTMAX-1");
    signalNameMap[SIGRTMAX] = std::string("SIGRTMAX");
}
