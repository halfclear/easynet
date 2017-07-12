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

TEST(SignalMgr, testGetSignalMgr)
{
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_TRACE);
	LOG_INFO("-----------------------------------------------------");
	LOG_INFO("SignalMgr-testGetSignalMgr");
	LOG_INFO("-----------------------------------------------------");
   
    for (int i = 0; i < 10; i++)
    {
    	std::thread([=]{
    		SignalMgr &mgr = SignalMgr::getInstance();
    		LOG_INFO("mgr:%x", &mgr);
    	}).detach();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}


TEST(SignalMgr, testSignalName)
{
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
	LOG_INFO("-----------------------------------------------------");
	LOG_INFO("SignalMgr-testSignalName");
	LOG_INFO("-----------------------------------------------------");

	LOG_INFO("NSIG=%d, SIGRTMIN=%d", NSIG, SIGRTMIN);

	for (int i = 0; i < NSIG; ++i)
	{
		LOG_INFO("%d %s (%s)", i, SignalMgr::getSignalName(i).c_str(), ::strsignal(i));
	}
}

int testExitCodeKilledBySignal(int sig)
{
	pid_t pid = ::fork();
	if (pid < 0) {
		LOG_ERROR("fork error");
		return -1;
	} else if (pid == 0) { // child
        
        LOG_INFO("child starting");
		EventLoop loop;
		loop.loop();
		
		LOG_INFO("child exiting");
        exit(0);
        return -1;
	} else {    // parent
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		LOG_INFO("send signal %d %s to child %d", sig, SignalMgr::getSignalName(sig).c_str(), pid);
		kill(pid, sig);

		int status = 0;
        pid_t ret = ::waitpid(pid, &status, 0);
        LOG_INFO("child %d exited, exit code = %d", ret, status);
        return status;
	}
}

TEST(SignalMgr, testProcessExitCodeAfterKilledBySignal)
{
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
	LOG_INFO("-----------------------------------------------------");
	LOG_INFO("SignalMgr-testExitCodeKilledBySignal");
	LOG_INFO("-----------------------------------------------------");

	int status = 0;

	status = testExitCodeKilledBySignal(SIGHUP);
	EXPECT_EQ(SIGHUP, status);

	status = testExitCodeKilledBySignal(SIGINT);
	EXPECT_EQ(SIGINT, status);

	status = testExitCodeKilledBySignal(SIGQUIT);
	EXPECT_EQ(SIGQUIT+128, status);

	status = testExitCodeKilledBySignal(SIGILL);
	EXPECT_EQ(SIGILL+128, status);

	status = testExitCodeKilledBySignal(SIGTRAP);
	EXPECT_EQ(SIGTRAP+128, status);

	status = testExitCodeKilledBySignal(SIGABRT);
	EXPECT_EQ(SIGABRT+128, status);

	status = testExitCodeKilledBySignal(SIGBUS);
	EXPECT_EQ(SIGBUS+128, status);

	status = testExitCodeKilledBySignal(SIGFPE);
	EXPECT_EQ(SIGFPE+128, status);

	status = testExitCodeKilledBySignal(SIGKILL);
	EXPECT_EQ(SIGKILL, status);

	status = testExitCodeKilledBySignal(SIGUSR1);
	EXPECT_EQ(SIGUSR1, status);

	status = testExitCodeKilledBySignal(SIGSEGV);
	EXPECT_EQ(SIGSEGV+128, status);

	status = testExitCodeKilledBySignal(SIGUSR2);
	EXPECT_EQ(SIGUSR2, status);

	status = testExitCodeKilledBySignal(SIGPIPE);
	EXPECT_EQ(SIGPIPE, status);

	status = testExitCodeKilledBySignal(SIGALRM);
	EXPECT_EQ(SIGALRM, status);

	status = testExitCodeKilledBySignal(SIGTERM);
	EXPECT_EQ(SIGTERM, status);

	status = testExitCodeKilledBySignal(SIGSTKFLT);
	EXPECT_EQ(SIGSTKFLT, status);

	status = testExitCodeKilledBySignal(SIGXCPU);
	EXPECT_EQ(SIGXCPU+128, status);

	status = testExitCodeKilledBySignal(SIGXFSZ);
	EXPECT_EQ(SIGXFSZ+128, status);

	status = testExitCodeKilledBySignal(SIGVTALRM);
	EXPECT_EQ(SIGVTALRM, status);

	status = testExitCodeKilledBySignal(SIGPROF);
	EXPECT_EQ(SIGPROF, status);

	status = testExitCodeKilledBySignal(SIGIO);
	EXPECT_EQ(SIGIO, status);

	status = testExitCodeKilledBySignal(SIGPWR);
	EXPECT_EQ(SIGPWR, status);

	status = testExitCodeKilledBySignal(SIGSYS);
	EXPECT_EQ(SIGSYS+128, status);

	for (int i = SIGRTMIN; i <= SIGRTMAX; i++)
	{
		status = testExitCodeKilledBySignal(i);
	    EXPECT_EQ(i, status);
	}
}

void runInThread(int threadNum)
{
    EventLoop loop;
    int caughtCnt[NSIG] = {0};
    int cnt = 0;

    for (int i = 0; i < NSIG; i++)
    {
    	if (loop.addSignalHandler(i, [=, &loop, &caughtCnt, &cnt]{
    		caughtCnt[i]++;
    		if (i == SIGRTMAX) {
    			cnt++;
    		}
    		if (cnt == 3) {
    			loop.quit();
    		}
    		LOG_INFO("thread %d caught signal %d %s", threadNum, i, SignalMgr::getSignalName(i).c_str());
    	})) {
    		LOG_INFO("thread %d add signal handler for signal %d %s", threadNum, i, SignalMgr::getSignalName(i).c_str());
    	} else {
    		LOG_INFO("thread %d add signal handler for signal %d %s failed", threadNum, i, SignalMgr::getSignalName(i).c_str());
    	}
    }

    loop.loop();
    for (int i = SIGHUP; i <= SIGSYS; i++) {
    	LOG_INFO("signal %s caught %d times by thread %d ", SignalMgr::getSignalName(i).c_str(), caughtCnt[i], threadNum);
        if (i != SIGKILL && i != SIGSTOP) {
        	EXPECT_EQ(1, caughtCnt[i]);
        }
    }
    for (int i = SIGRTMIN; i < SIGRTMAX; i++) {
    	LOG_INFO("signal %s caught %d times by thread %d ", SignalMgr::getSignalName(i).c_str(), caughtCnt[i], threadNum);
    	EXPECT_EQ(3, caughtCnt[i]);
    }
    loop.loop();
}

void child()
{
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
    LOG_INFO("child starting...");
    SignalMgr::enableSignalHandling();

    auto f1 = std::async(std::launch::async, runInThread, 1);
    auto f2 = std::async(std::launch::async, runInThread, 2);

    f1.wait();
    f2.wait();

    LOG_INFO("child exit...");
}


TEST(SignalMgr, testSignalHandling)
{
	Logger::getInstance().setLogLevel(Logger::LOG_LEVEL_INFO);
	LOG_INFO("-----------------------------------------------------");
	LOG_INFO("SignalMgr-testSignalHandling");
	LOG_INFO("-----------------------------------------------------");

	pid_t pid = ::fork();
	if (pid < 0) {
		LOG_ERROR("fork error");
	} else if (pid == 0) { // child
         child();
         exit(0);
	} else {    // parent
		LOG_INFO("parent starting, child pid = %d", pid);

	    std::this_thread::sleep_for(std::chrono::milliseconds(200));

	    for (int i = 0; i < NSIG; i++)
	    {
	    	if (i != SIGKILL && i != SIGSTOP)
	    	{
	    		LOG_INFO("send signal %d %s to child %d", i, SignalMgr::getSignalName(i).c_str(), pid);
	    	    ::kill(pid, i);
	    	    ::kill(pid, i);
	    	    ::kill(pid, i);
	    	    std::this_thread::sleep_for(std::chrono::milliseconds(200));
	    	}
	    }

        ::kill(pid, SIGKILL);
	    int status = 0;
	    pid_t ret = ::waitpid(pid, &status, 0);
	    LOG_INFO("child %d exited, exit code = %d", ret, status);
	    EXPECT_EQ(SIGKILL, status);
	}

	LOG_INFO("exiting");
}
