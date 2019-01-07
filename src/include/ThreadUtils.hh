#ifndef __THREADUTILS_HH__
#define __THREADUTILS_HH__

#include <pthread.h>
#include <sched.h>

#include "Represent.h"

namespace m3
{

int SetRealtime(int _policy = SCHED_RR, int _priority = 99);
int AssignCPU();

}

#endif