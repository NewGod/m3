#ifndef _FINALSCHEDULER_HH_
#define _FINALSCHEDULER_HH_

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

class FinalScheduler : public Scheduler
{
public:
	FinalScheduler() : ptr(0) {}

	int schedule(Interface **dest, Message *msg) override
	{
		int len;
		Policy *policy = this->conn->getPolicy();
		Interface **avail = policy->acquireInterfaces(&len);
		int res = 0;

		std::map<unsigned long, Interface*> metricMap;
		for (int i = 0; i < len; i++)
		{
			Interface *curr = avail[i];
			if (!curr->isReady())
				continue;
			MetricProvider *provider = curr->getProvider();
			ProtocolMetric *metric;
			provider->getMetric(ProtocolMetric::TCP_BASIC, &metric);
			TCPBasicMetricValue *tcpMetric = (TCPBasicMetricValue *)(metric->getValue());

			auto thp = tcpMetric->sthp;
			auto rtt = (tcpMetric->srtt) >> 13;
			if (rtt == 0)
				rtt = i + 1;
			thp += 88;
			metricMap[thp / rtt] = curr;
		}

		dest[0] = metricMap.rbegin()->second;
		//logMessage("Sending on interface %s, var = %lu", dest[0]->name, metricMap.rbegin()->first);
		//logMessage("           interface %s, var = %lu", dest[0]->name, (++metricMap.rbegin())->first);
		//logMessage("           interface %s, var = %lu", dest[0]->name, (++(++metricMap.rbegin()))->first);
		res = 1;


		//std::map<unsigned long, Interface*> thpInterfaceMap;
		//std::map<unsigned long, Interface*> rttInterfaceMap;
		//for (int i = 0; i < len; i++)
		//{
		//	if (avail[i]->isReady())
		//	{
		//		Interface *curr = avail[i];
		//		MetricProvider *provider = curr->getProvider();
		//		ProtocolMetric *metric;
		//		provider->getMetric(ProtocolMetric::TCP_BASIC, &metric);
		//		TCPBasicMetricValue *tcpMetric = (TCPBasicMetricValue *)(metric->getValue());
		//		auto thp = tcpMetric->sthp;
		//		auto rtt = (tcpMetric->srtt) >> 13;
		//		thpInterfaceMap[thp] = curr;
		//		rttInterfaceMap[rtt] = curr;
		//		
		//	}
		//}

		///*if (thpInterfaceMap.rbegin()->second == rttInterfaceMap.begin()->second)
		//{
		//	dest[0] = thpInterfaceMap.rbegin()->second;
		//	logMessage("Sending on interface %s, rtt = %lu, thp = %lu", dest[0]->name, rttInterfaceMap.begin()->first, thpInterfaceMap.rbegin()->first);
		//	res = 1;
		//}
		//else
		//{
		//	dest[0] = rttInterfaceMap.begin()->second;
		//	logMessage("Sending on interface %s, but is not the best", dest[0]->name);
		//	res = 1;
		//}*/

		//unsigned long best_rtt, second_best_rtt;
		//unsigned long best_thp, second_best_thp;
		//best_rtt = rttInterfaceMap.begin()->first;
		//second_best_rtt = (++rttInterfaceMap.begin())->first;
		//best_thp = thpInterfaceMap.rbegin()->first;
		//second_best_thp = (++thpInterfaceMap.rbegin())->first;

		//if (best_rtt == 0)
		//{
		//	dest[0] = rttInterfaceMap.begin()->second;
		//	logMessage("Sending on interface %s based on RTT = 0", dest[0]->name);
		//}
		//else if (second_best_thp == 0)
		//{
		//	if (best_thp == 0)
		//	{
		//		dest[0] = rttInterfaceMap.begin()->second;
		//		logMessage("Sending on interface %s based on all THP = 0", dest[0]->name);
		//	}
		//	else
		//	{
		//		dest[0] = thpInterfaceMap.rbegin()->second;
		//		logMessage("Sending on interface %s based on second THP = 0", dest[0]->name);
		//	}
		//}
		//else
		//{
		//	double rtt_rate = second_best_rtt / best_rtt, thp_rate = best_thp / second_best_thp;
		//	if (rtt_rate > thp_rate)
		//	{
		//		dest[0] = rttInterfaceMap.begin()->second;
		//		logMessage("Sending on interface %s based on RTT", dest[0]->name);
		//	}
		//	else
		//	{
		//		dest[0] = thpInterfaceMap.rbegin()->second;
		//		logMessage("Sending on interface %s based on THP", dest[0]->name);
		//	}
		//}
		//res = 1;


		policy->readRelease();

		iferr(res == 0)
		{
			logError("FinalScheduler::schedule: No interface available.");
		}

		return res;
	}
	static void* newInstance() //@ReflectFianlScheduler
	{
		return snew FinalScheduler();
	}
protected:
	int ptr;
};


#endif