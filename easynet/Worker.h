// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_WORKER_H_
#define _EASYNET_WORKER_H_

#include <memory>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>

#include "EventLoop.h"

namespace easynet
{

class Worker
{
public:
	enum LoadBalanceStrategy
	{
		LOAD_BALANCE_STRATEGY_BY_LOCK = 0,
		LOAD_BALANCE_STRATEGY_ROUND_ROBIN,
		LOAD_BALANCE_STRATEGY_TOKEN_RING_BY_METRIC_SMALLER,
		LOAD_BALANCE_STRATEGY_TOKEN_RING_BY_METRIC_LARGER
	};

	using AcquireTokenHandler = std::function<bool ()>;
	using LoadBalanceHandler = std::function<bool ()>;
	using LoadBalanceMetricGetter = std::function<int ()>;
	using Functor = std::function<void ()>;

	Worker(int timeResolutionMillis,
		   int delayMillisRetryAcquireToken,
		   LoadBalanceStrategy strategy,
		   std::mutex *lock)
         : quit_(false),
           exited_(false),
           heldToken_(false),
           relayed_(false),
           isHeadWorker_(false),
           strategy_(strategy),
           delayMillisRetryAcquireToken_(delayMillisRetryAcquireToken),
           lock_(lock),
           loop_(timeResolutionMillis)
	{}

	~Worker() = default;
	Worker(const Worker &rhs) = delete;
	Worker& operator=(const Worker &rhs) = delete;

	EventLoop* getLoop() { return &loop_; }

	void start() { std::thread(std::bind(&Worker::run, this)).detach(); }
	void quit() { quit_ = true; loop_.wakeup();}
	bool exited() const { return exited_; }

	bool isSingleWorker() const { return buddies_.empty(); }
	bool heldToken() const { return heldToken_; }
	void setBuddies(const std::vector<Worker*> &buddies) { buddies_ = buddies; }

    void setBeforeAcquireTokenHandler(AcquireTokenHandler &&handler) 
    { beforeAcquireTokenHandler_ = std::move(handler); }
    void setBeforeAcquireTokenHandler(const AcquireTokenHandler &handler) 
    { setBeforeAcquireTokenHandler(AcquireTokenHandler(handler)); }

//  void setBeforeLoadBalanceHandler(LoadBalanceHandler &&handler) 
//  { beforeLoadBalanceHandler_ = std::move(handler); }
//	void setBeforeLoadBalanceHandler(const LoadBalanceHandler &handler) 
//	{ setBeforeLoadBalanceHandler(LoadBalanceHandler(handler)); }

	void setLoadBalanceMetricGetter(LoadBalanceMetricGetter &&handler) 
	{ loadBalanceMetricGetter_ = std::move(handler); }
	void setLoadBalanceMetricGetter(const LoadBalanceMetricGetter &handler) 
	{ setLoadBalanceMetricGetter(LoadBalanceMetricGetter(handler)); }

	void setTokenAcquiredHandler(Functor &&handler) { tokenAcquiredHandler_ = std::move(handler); }
	void setTokenAcquiredHandler(const Functor &handler) { setTokenAcquiredHandler(Functor(handler)); }

	void setTokenYieldedHandler(Functor &&handler) { tokenYieldedHandler_ = std::move(handler); }
	void setTokenYieldedHandler(const Functor &handler) { setTokenYieldedHandler(Functor(handler)); }

private:
	void run();

	void init();
	void rearrangeBuddies();
	void initLoadBalancer();
	void initTokenAcquirer();
	
    void relay() { relayed_ = true; loop_.wakeup();}
	void acquireToken();
	void acquireTokenByLock();	
	void acquireTokenByTokenRing();
	void tokenAcquired();
	void yieldToken();

	void loadBalance();
	void loadBalanceByLock();
	void loadBalanceRoundRobin();
	void loadBalanceTokenRingByMetricSmaller();
	void loadBalanceTokenRingByMetricLarger();
	int  getLoadBalanceMetric() const { return loadBalanceMetricGetter_ ? loadBalanceMetricGetter_() : 0; }
	
	bool tryLock() { return lock_->try_lock(); }
	void unLock()  { lock_->unlock(); }

	bool quit_;
	bool exited_;

	bool heldToken_;
	bool relayed_;
	bool isHeadWorker_;
	LoadBalanceStrategy strategy_;
	int delayMillisRetryAcquireToken_;
	std::mutex *lock_;

	EventLoop loop_;

    Functor loadBalancer_;
    Functor tokenAcquirer_;

	// will be called before run task,if return true, try aqure token
	AcquireTokenHandler beforeAcquireTokenHandler_;
//	LoadBalanceHandler beforeLoadBalanceHandler_;
	LoadBalanceMetricGetter loadBalanceMetricGetter_;
	Functor tokenAcquiredHandler_; // will be called after this worker acquired token
	Functor tokenYieldedHandler_; // will be called after this worker yieled token

	std::vector<Worker*> buddies_; // other workers
};

} // namespace easynet

#endif
