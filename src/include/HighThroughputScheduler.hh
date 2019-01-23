#ifndef _HIGHTHROUGHPUTSCHEDULER_HH_
#define _HIGHTHROUGHPUTSCHEDULER_HH_

#include "Policy.hh"
#include "Scheduler.hh"
#include "Connection.hh"
#include "MetricProvider.hh"
#include "TCPBasicMetric.hh"
#include <iostream>

using m3::Scheduler;
using m3::Interface;
using m3::Message;
using m3::Policy;
using m3::Connection;
using m3::ContextManager;
using m3::ProtocolMetric;
using m3::MetricProvider;
using m3::TCPBasicMetric;
using m3::TCPBasicMetricValue;

class HighThroughputScheduler : public Scheduler
{
public:
	HighThroughputScheduler() : ptr(0) {}

	int schedule(Interface **dest, Message *msg) override
	{
		int len;
		Policy *policy = this->conn->getPolicy();
		Interface **avail = policy->acquireInterfaces(&len);
		int res = 0;

		std::map<unsigned, Interface*> thpInterfaceMap;
		for (int i = 0; i < len; i++)
		{
			if (avail[i]->isReady())
			{
				Interface *curr = avail[i];
				MetricProvider *provider = curr->getProvider();
				ProtocolMetric *metric;
				provider->getMetric(ProtocolMetric::TCP_BASIC, &metric);
				TCPBasicMetricValue *tcpMetric = (TCPBasicMetricValue *)(metric->getValue());
				auto thp = tcpMetric->sthp;
				thpInterfaceMap[thp] = curr;
			}
		}
		dest[0] = thpInterfaceMap.rbegin()->second;

		//logMessage("Sending on interface %s, thp = %u", dest[0]->name, thpInterfaceMap.rbegin()->first);
		//logMessage("           interface %s, thp = %u", dest[0]->name, (++thpInterfaceMap.rbegin())->first);
		//logMessage("           interface %s, thp = %u", dest[0]->name, (++(++thpInterfaceMap.rbegin()))->first);
		res = 1;

		policy->readRelease();

		iferr(res == 0)
		{
			logError("HighThroughputScheduler::schedule: No interface available.");
		}

		return res;
	}
	static void* newInstance() //@ReflectHighThroughputScheduler
	{
		return snew HighThroughputScheduler();
	}
protected:
	int ptr;
};


#endif