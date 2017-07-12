#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <cstring>
#include <string>
#include <iostream>
#include "utils/TimeUtil.h"
#include <test_harness.h>

#include <thread>
#include <chrono>
#include <future>
#include <sys/wait.h>

#include "EventLoop.h"
#include "SignalHandler.h"
#include "SignalMgr.h"

#include "utils/log.h"

using namespace std;
using namespace easynet;

int testAddAndCloseInLoop(int sig)
{
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
	LOG_INFO("-----------------------------------------");
	LOG_INFO("SignalHandlerMgr-testAddAndCloseInLoop signal:%d %s", sig, SignalMgr::getSignalName(sig).c_str());
	LOG_INFO("----------------------------------------");

	pid_t pid = ::fork();
	if (pid < 0) {
		LOG_ERROR("fork error");
		return -1;
	} else if (pid == 0) { // child
        
        LOG_INFO("child starting----");
    
        SignalMgr::enableSignalHandling();
		EventLoop loop;

		SignalHandler *handler1 = nullptr;
		SignalHandler *handler2 = nullptr;

		int cnt1 = 0;
		int cnt2 = 0;

		loop.runAfter(0, [&]{
			int sigNum = sig;
			handler1 = loop.addSignalHandler(sigNum, [&](){
				cnt1++;
		        LOG_INFO("signal %d %s caught %d times by child handler1", 
		        	sigNum, SignalMgr::getSignalName(sigNum).c_str(), cnt1);	
	        });

			handler2 = loop.addSignalHandler(sigNum, [&](){
				cnt2++;
		        LOG_INFO("signal %d %s caught %d times by child handler2", 
		        	sigNum, SignalMgr::getSignalName(sigNum).c_str(), cnt2);	
	        });
		});

		loop.runAfter(200, [&]{
			LOG_INFO("child to close all signal %d %s handlers", 
				sig, SignalMgr::getSignalName(sig).c_str());
			handler1->close();
			handler2->close();
			EXPECT_EQ(1, cnt1);
			EXPECT_EQ(1, cnt2);
		});
		
		loop.loop();
		
		LOG_INFO("child exiting----");;
        exit(0);
        return 0;
	} else {    // parent
		
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		LOG_INFO("send signal %d %s to child %d", sig, SignalMgr::getSignalName(sig).c_str(), pid);
		kill(pid, sig);

		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		LOG_INFO("send signal %d %s again to child %d", sig, SignalMgr::getSignalName(sig).c_str(), pid);
		kill(pid, sig);

		std::this_thread::sleep_for(std::chrono::milliseconds(50));

		int status = 0;
        pid_t ret = ::waitpid(pid, &status, 0);
        LOG_INFO("child %d exited,  exit status = %d", ret, status);
        return status;
	}
}

TEST(SignalHandlerMgr, testAddAndCloseInLoop)
{
	LOG_INFO("-----------------------------------------------------");
	LOG_INFO("SignalHandlerMgr-testAddAndCloseInLoop");
	LOG_INFO("-----------------------------------------------------");

	int status = 0;

    status = testAddAndCloseInLoop(SIGHUP);
	EXPECT_EQ(SIGHUP, status);

	status = testAddAndCloseInLoop(SIGINT);
	EXPECT_EQ(SIGINT, status);

	status = testAddAndCloseInLoop(SIGQUIT);
	EXPECT_EQ(SIGQUIT+128, status);

	status = testAddAndCloseInLoop(SIGILL);
	EXPECT_EQ(SIGILL+128, status);

	status = testAddAndCloseInLoop(SIGTRAP);
	EXPECT_EQ(SIGTRAP+128, status);

	status = testAddAndCloseInLoop(SIGABRT);
	EXPECT_EQ(SIGABRT+128, status);

	status = testAddAndCloseInLoop(SIGBUS);
	EXPECT_EQ(SIGBUS+128, status);

	status = testAddAndCloseInLoop(SIGFPE);
	EXPECT_EQ(SIGFPE+128, status);

	status = testAddAndCloseInLoop(SIGKILL);
	EXPECT_EQ(SIGKILL, status);

	status = testAddAndCloseInLoop(SIGUSR1);
	EXPECT_EQ(SIGUSR1, status);

	status = testAddAndCloseInLoop(SIGSEGV);
	EXPECT_EQ(SIGSEGV+128, status);

	status = testAddAndCloseInLoop(SIGUSR2);
	EXPECT_EQ(SIGUSR2, status);

	status = testAddAndCloseInLoop(SIGPIPE);
	EXPECT_EQ(SIGPIPE, status);

	status = testAddAndCloseInLoop(SIGALRM);
	EXPECT_EQ(SIGALRM, status);

	status = testAddAndCloseInLoop(SIGTERM);
	EXPECT_EQ(SIGTERM, status);

	status = testAddAndCloseInLoop(SIGSTKFLT);
	EXPECT_EQ(SIGSTKFLT, status);

	status = testAddAndCloseInLoop(SIGXCPU);
	EXPECT_EQ(SIGXCPU+128, status);

	status = testAddAndCloseInLoop(SIGXFSZ);
	EXPECT_EQ(SIGXFSZ+128, status);

	status = testAddAndCloseInLoop(SIGVTALRM);
	EXPECT_EQ(SIGVTALRM, status);

	status = testAddAndCloseInLoop(SIGPROF);
	EXPECT_EQ(SIGPROF, status);

	status = testAddAndCloseInLoop(SIGIO);
	EXPECT_EQ(SIGIO, status);

	status = testAddAndCloseInLoop(SIGPWR);
	EXPECT_EQ(SIGPWR, status);

	status = testAddAndCloseInLoop(SIGSYS);
	EXPECT_EQ(SIGSYS+128, status);

	for (int i = SIGRTMIN; i <= SIGRTMAX; i++)
	{
		status = testAddAndCloseInLoop(i);
	    EXPECT_EQ(i, status);
	}
}

int testAddAndCloseInHandler(int sig)
{
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
	LOG_INFO("-----------------------------------------");
	LOG_INFO("SignalHandlerMgr-testAddAndCloseInHandler signal:%d %s", sig, SignalMgr::getSignalName(sig).c_str());
	LOG_INFO("----------------------------------------");

	pid_t pid = ::fork();
	if (pid < 0) {
		LOG_ERROR("fork error");
		return -1;
	} else if (pid == 0) { // child
        LOG_INFO("child starting----");
    
        SignalMgr::enableSignalHandling();
		EventLoop loop;

		SignalHandler *handler1 = nullptr;
		SignalHandler *handler2 = nullptr;
		SignalHandler *handler3 = nullptr;
		SignalHandler *handler4 = nullptr;
        
        int cnt1 = 0;
        int cnt2 = 0;
        int cnt3 = 0;
        int cnt4 = 0;

		handler1 = loop.addSignalHandler(sig, [&](){
			cnt1++;
	        LOG_INFO("signal %d %s caught %d times by child handler1", 
	        	sig, SignalMgr::getSignalName(sig).c_str(), cnt1);
	        handler1->close();
	    });
        
		handler2 = loop.addSignalHandler(sig, [&](){
			cnt2++;
	        LOG_INFO("signal %d %s caught %d times by child handler2", 
	        	sig, SignalMgr::getSignalName(sig).c_str(), cnt2);
	        handler2->close();

	        handler3 = loop.addSignalHandler(sig, [&](){
	        	cnt3++;
	            LOG_INFO("signal %d %s caught %d times by child handler3", 
	            	sig, SignalMgr::getSignalName(sig).c_str(), cnt3);
	        });

	        handler4 = loop.addSignalHandler(sig+1, [&](){
	        	cnt4++;
	            LOG_INFO("signal %d %s caught %d times by child handler4", 
	            	sig+1, SignalMgr::getSignalName(sig+1).c_str(), cnt4);
	        });
        });

		loop.runAfter(300, [&]{
			LOG_INFO("child to close all signal handlers");
			handler3->close();
			handler4->close();
			EXPECT_EQ(1, cnt1);
			EXPECT_EQ(1, cnt2);
			EXPECT_EQ(2, cnt3);
			EXPECT_EQ(1, cnt4);
		});
		
		loop.loop();
		
		LOG_INFO("child exiting----");;
        exit(0);
        return 0;
	} else {    // parent
		
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		LOG_INFO("send signal %d %s to child %d", sig, SignalMgr::getSignalName(sig).c_str(), pid);
		kill(pid, sig);

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		LOG_INFO("send signal %d %s again to child %d", sig, SignalMgr::getSignalName(sig).c_str(), pid);
		kill(pid, sig);
		LOG_INFO("send signal %d %s to child %d", sig+1, SignalMgr::getSignalName(sig+1).c_str(), pid);
		kill(pid, sig+1);

		std::this_thread::sleep_for(std::chrono::milliseconds(400));
		LOG_INFO("send signal %d %s again to child %d", sig, SignalMgr::getSignalName(sig).c_str(), pid);
		kill(pid, sig);

		int status = 0;
        pid_t ret = ::waitpid(pid, &status, 0);
        LOG_INFO("child %d exited,  exit status = %d", ret, status);
        return status;
	}
}

TEST(SignalHandlerMgr, testAddAndCloseInHandler)
{
	LOG_INFO("-----------------------------------------------------");
	LOG_INFO("SignalHandlerMgr-testAddAndCloseInHandler");
	LOG_INFO("-----------------------------------------------------");

	int status = 0;
    status = testAddAndCloseInHandler(SIGINT);
    EXPECT_EQ(2, status);
    status = testAddAndCloseInHandler(SIGABRT);
    EXPECT_EQ(134, status);
}
