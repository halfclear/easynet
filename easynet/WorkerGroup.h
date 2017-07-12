// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_WORKER_GROUP_H_
#define _EASYNET_WORKER_GROUP_H_

#include <vector>
#include <memory>
#include <atomic>
#include <mutex>

#include "Worker.h"

namespace easynet
{

class WorkerGroup
{
public:
	WorkerGroup()
	     : WorkerGroup(Worker::LOAD_BALANCE_STRATEGY_ROUND_ROBIN, 1, 0, 0)
	{}

	WorkerGroup(Worker::LoadBalanceStrategy strategy,
                int workerNum,
                int timeResolutionMillis,
                int delayMillisRetryAcquireTocken);
	~WorkerGroup() { stop(); }
	WorkerGroup(const WorkerGroup &rhs) = delete;
	WorkerGroup& operator=(const WorkerGroup &rhs) = delete;

	void start() { startWorkers(); }
	void stop()  { stopWorkers(); }

	std::vector<Worker*> getWorkers() const;

private:
	void init() { initWorkers(); }
	void initWorkers();
	void startWorkers();
	void stopWorkers();

	Worker::LoadBalanceStrategy loadBalanceStrategy_;
	int workerNum_;
	int timeResolutionMillis_;
	int delayMillisRetryAcquireTocken_;
    std::mutex loadBalanceLock_;
	std::vector<std::unique_ptr<Worker>> workers_;
};

}

#endif