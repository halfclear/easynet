// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include <utility>
#include "WorkerGroup.h"

using namespace easynet;

WorkerGroup::WorkerGroup(Worker::LoadBalanceStrategy strategy,
                         int workerNum,
                         int timeResolutionMillis,
                         int delayMillisRetryAcquireTocken)
                       : loadBalanceStrategy_(strategy),
                         workerNum_(workerNum),
                         timeResolutionMillis_(timeResolutionMillis),
                         delayMillisRetryAcquireTocken_(delayMillisRetryAcquireTocken)
{
	init();
}

void WorkerGroup::initWorkers()
{
	for (int i = 0; i < workerNum_; i++)
	{
		std::unique_ptr<Worker> worker(new Worker(timeResolutionMillis_,
		                                          delayMillisRetryAcquireTocken_, 
			                                      loadBalanceStrategy_,
			                                      &loadBalanceLock_));
		workers_.push_back(std::move(worker));
	}

	std::vector<Worker*> buddies = getWorkers();
	for (auto &worker : workers_)
	{
		worker->setBuddies(buddies);
	}
}

void WorkerGroup::startWorkers()
{
	for (auto &worker : workers_)
	{
		worker->start();
	}
}

void WorkerGroup::stopWorkers()
{
	for (auto &worker : workers_)
	{
		worker->quit();
		while (!(worker->exited()))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
	//		std::this_thread::yield();
		}
	}
}

std::vector<Worker*> WorkerGroup::getWorkers() const
{
	std::vector<Worker*> v;
	for (auto &worker : workers_)
	{
		v.push_back(worker.get());
	}

	return std::move(v);
}
