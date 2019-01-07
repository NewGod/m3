#include "SpinQueueBuffer.hh"

#ifdef SPINQUEUEBUFFER_DEBUG

#include <cstdio>
#include <thread>

using namespace m3;

SpinQueueBuffer<long> buffer(12);

const int N = 1000000;

class ProducerThread : public std::thread
{
protected:
    static void threadmain(int n)
    {
        for (int i = 0; i != N; ++i)
        {
            long val = ((long)n << 32) | (long)i;
            //buffer.advance(&val);
            while (buffer.tryAdvance(&val));
        }
    }
public:
    ProducerThread(int i) : thread(threadmain, i) {}
};

class ConsumerThread : public std::thread
{
protected:
    static void threadmain(int n)
    {
        long val = 0;
        uint32_t cursor;
        n *= N;
        for (int i = 0; i < n; ++i)
        {
            while ((cursor = buffer.tryPurchase(&val)) != 0);
            //buffer.purchase(&val);
            //cursor = buffer.tryPurchase(&val);
            printf("PUR %d %d %u\n",
                cursor, (int)(val >> 32), (int)(unsigned)val);
        }
    }
public:
    ConsumerThread(int i) : thread(threadmain, i) {}
};

int main()
{
    ConsumerThread *ct1 = new ConsumerThread(1);
    ProducerThread *pt0 = new ProducerThread(0);
    //ProducerThread *pt1 = new ProducerThread(1);
    
    //ct1->join();
    
    pt0->join();
//    pt1->join();
    ct1->join();

    fprintf(stderr, "!\n");

    return 0;
}

#endif
