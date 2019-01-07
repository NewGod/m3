#include "RWLock.hh"

#include <thread>

using m3::RWLock;

#ifdef RWLOCK_DEBUG

RWLock lock;
int x = 0;

void thread1()
{
    for (int i = 0; i < 1000000; ++i)
    {
        lock.writeLock();
        x++;
        lock.writeRelease();
    }
}

void thread2()
{
    for (int i = 0; i < 1000000; ++i)
    {
        lock.writeLock();
        x++;
        lock.writeRelease();
    }
}

void thread3()
{
    for (int i = 0; i < 1000000; ++i)
    {
        lock.writeLock();
        x++;
        lock.writeRelease();
    }
}

void thread4()
{
    for (int i = 0; i < 50; ++i)
    {
        lock.readLock();
        lock.readRelease();
    }
}

int main()
{
    std::thread th1(thread1), th2(thread2), th3(thread3), th4(thread4);

    printf("read %d\n", lock.readerCount.load());

    th1.join();
    th2.join();
    th3.join();
    th4.join();

    printf("x %d\n", x);

    return 0;
}

#endif