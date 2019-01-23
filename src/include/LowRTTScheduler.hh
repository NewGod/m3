#ifndef _LOWRTTSCHEDULER_HH_
#define _LOWRTTSCHEDULER_HH_

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

class LowRTTScheduler : public Scheduler
{
public:
	LowRTTScheduler() : ptr(0) {}

	int schedule(Interface **dest, Message *msg) override
	{
		int len;
		Policy *policy = this->conn->getPolicy();
		Interface **avail = policy->acquireInterfaces(&len);
		int res = 0;

		std::map<unsigned, Interface*> rttInterfaceMap;
		for (int i = 0; i < len; i++)
		{
			if (avail[i]->isReady())
			{
				Interface *curr = avail[i];
				MetricProvider *provider = curr->getProvider();
				ProtocolMetric *metric;
				provider->getMetric(ProtocolMetric::TCP_BASIC, &metric);
				TCPBasicMetricValue *tcpMetric = (TCPBasicMetricValue *)(metric->getValue());
				auto rtt = (tcpMetric->srtt) >> 13;
				rttInterfaceMap[rtt] = curr;
			}
		}
		dest[0] = rttInterfaceMap.begin()->second;
		
		//logMessage("Sending on interface %s, rtt = %u", dest[0]->name, rttInterfaceMap.begin()->first);
		//logMessage("           interface %s, rtt = %u", dest[0]->name, (++rttInterfaceMap.begin())->first);
		//logMessage("           interface %s, rtt = %u", dest[0]->name, (++(++rttInterfaceMap.begin()))->first);
		res = 1;

		policy->readRelease();

		iferr(res == 0)
		{
			logError("LowRTTScheduler::schedule: No interface available.");
		}

		return res;
	}
	static void* newInstance() //@ReflectLowRTTScheduler
	{
		return snew LowRTTScheduler();
	}
protected:
	int ptr;
};


#endif