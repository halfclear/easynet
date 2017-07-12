// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include "Worker.h"
#include "utils/log.h"

using namespace easynet;

void Worker::rearrangeBuddies()
{
	// assume there are 4 workers, after re-arrange, worker's buddies are:
	// worker 1: 2, 3, 4
	// worker 2: 3, 4, 1
	// worker 3: 4, 1, 2
	// worker 4: 1, 2, 3
	auto it = buddies_.begin();
	if (*it == this)
	{
		isHeadWorker_ = true;
	}

    while (it != buddies_.end())
    {
    	Worker *worker = *it;
    	it = buddies_.erase(it);

    	if (worker == this)
    	{
    		break;
    	}
    	else
    	{
    		buddies_.push_back(worker); 		 		
    	}
    }
}

void Worker::initLoadBalancer()
{
	switch (strategy_)
	{
	case LOAD_BALANCE_STRATEGY_BY_LOCK:
		loadBalancer_ = std::bind(&Worker::loadBalanceByLock, this);
	    break;

	case LOAD_BALANCE_STRATEGY_ROUND_ROBIN:
	    loadBalancer_ = std::bind(&Worker::loadBalanceRoundRobin, this);
	    break;

	case LOAD_BALANCE_STRATEGY_TOKEN_RING_BY_METRIC_SMALLER:
	    loadBalancer_ = std::bind(&Worker::loadBalanceTokenRingByMetricSmaller, this);
	    break;

	case LOAD_BALANCE_STRATEGY_TOKEN_RING_BY_METRIC_LARGER:
	    loadBalancer_ = std::bind(&Worker::loadBalanceTokenRingByMetricLarger, this);
	    break;
	}
}

void Worker::initTokenAcquirer()
{
	switch (strategy_)
	{
	case LOAD_BALANCE_STRATEGY_BY_LOCK:
		tokenAcquirer_ = std::bind(&Worker::acquireTokenByLock, this);
	    break;

	case LOAD_BALANCE_STRATEGY_ROUND_ROBIN:
	case LOAD_BALANCE_STRATEGY_TOKEN_RING_BY_METRIC_SMALLER:
	case LOAD_BALANCE_STRATEGY_TOKEN_RING_BY_METRIC_LARGER:
	    tokenAcquirer_ = std::bind(&Worker::acquireTokenByTokenRing, this);
	    break;
	}
}

void Worker::init()
{
	rearrangeBuddies();
	initLoadBalancer();
	initTokenAcquirer();
	
	if (isSingleWorker() || (isHeadWorker_ && strategy_ != LOAD_BALANCE_STRATEGY_BY_LOCK))
	{
		tokenAcquired();
	}
}

void Worker::run()
{
	LOG_INFO("worker starting........................................");
	init();

	Functor loadBalancer(std::bind(&Worker::loadBalance, this));
	Functor *functorRunAfterAccept = isSingleWorker() ? nullptr : &loadBalancer;
	int timeoutMillis = EASYNET_TIMER_INFINITE;
  
	while (!quit_)
	{
		acquireToken();
		timeoutMillis = EASYNET_TIMER_INFINITE;
		if (!heldToken() && strategy_ == LOAD_BALANCE_STRATEGY_BY_LOCK)
		{
			timeoutMillis = delayMillisRetryAcquireToken_;
		}
		loop_.waitAndProcessEventsAndTimers(timeoutMillis, functorRunAfterAccept);	
	}

	yieldToken();
	exited_ = true;
	LOG_DEBUG("worker exiting.......................");	
}

void Worker::acquireToken()
{
	if (isSingleWorker() || heldToken())
	{
		return;
	}

	if (LOAD_BALANCE_STRATEGY_BY_LOCK == strategy_ &&
		beforeAcquireTokenHandler_ && 
		!(beforeAcquireTokenHandler_()))
	{
		// don't aquire token
		return;
	}

	tokenAcquirer_();	
}

void Worker::acquireTokenByLock()
{
	if (tryLock())
	{
		tokenAcquired();
	}
}

void Worker::acquireTokenByTokenRing()
{
	if (relayed_)
	{
		relayed_ = false;
		tokenAcquired();
	}
}

void Worker::loadBalance()
{
	if (!heldToken())
	{
		return;
	}

//	if (beforeLoadBalanceHandler_ && 
//		!beforeLoadBalanceHandler_())
//	{
//		return;
//	}

	loadBalancer_();
}

void Worker::loadBalanceByLock()
{
	yieldToken();
	unLock();
}

void Worker::loadBalanceRoundRobin()
{
	yieldToken();
	(buddies_[0])->relay();
}

void Worker::loadBalanceTokenRingByMetricSmaller()
{
	for (auto worker : buddies_)
	{
		if (worker->getLoadBalanceMetric() < getLoadBalanceMetric())
		{	
			yieldToken();
			worker->relay();
			return;
		}
	}
}

void Worker::loadBalanceTokenRingByMetricLarger()
{
	for (auto worker : buddies_)
	{
		if (worker->getLoadBalanceMetric() > getLoadBalanceMetric())
		{
			yieldToken();
			worker->relay();
			return;
		}
	}
}

void Worker::yieldToken()
{
	LOG_TRACE("worker[0x%x] yield token", this);
	heldToken_ = false;
	if (tokenYieldedHandler_)
	{
		tokenYieldedHandler_();
	}
}

void Worker::tokenAcquired()
{
	LOG_TRACE("---worker[0x%x] acquired token----", this);
	heldToken_ = true;
	if (tokenAcquiredHandler_)
	{
		tokenAcquiredHandler_();
	}
}