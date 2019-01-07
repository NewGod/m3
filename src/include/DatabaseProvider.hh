#ifndef __DATABASEPROVIDER_HH__
#define __DATABASEPROVIDER_HH__

#include <thread>
#include "RWLock.hh"
#include <cstdio>
#include <hdf5/serial/hdf5.h>
#include <cmath>

namespace m3
{

class DatabaseMetadata
{
public:
    int ninterface;
    struct GPSValue
    {
        double longitude, latitude;
        timespec timeStamp;
    } currentGPS;
    struct DBValue
    {
        double longitude, latitude;
        struct Value
        {
            double thp, rtt, loss;
        } value[];
    };
    void *DB;//3 * ( 2 * sizeof(double) + ninterface * sizeof(Value) )
    struct PositionValue
    {
        double longitude, latitude;
    } velocity;
    inline DBValue& DBRef(int idx)
    {
        return *(DBValue*)((char*)DB + (idx * ( 2 * sizeof(double) + ninterface * sizeof(DBValue::Value) ) ) );
    }
    inline const DBValue& DBRef(int idx) const
    {
        return *(const DBValue*)((const char*)DB + (idx * ( 2 * sizeof(double) + ninterface * sizeof(DBValue::Value) ) ) );
    }
    template<typename T>
    static inline auto sqr(const T& x) -> decltype(x * x)
    {
        return x * x;
    }
    template<typename T, typename U>
    static inline double gps_diff(const T& lhs, const U& rhs)
    {
        return sqrt(sqr(60. * (lhs.longitude - rhs.longitude)) + sqr(60. * (lhs.latitude - rhs.latitude)));
    }
};

class DatabaseProvider: protected DatabaseMetadata
{
protected:
    class DatabaseTracker : public std::thread
    {
    protected:
        static void tmain(
            DatabaseTracker *dbt,
            DatabaseProvider *dbpvd,
            timespec unit);
    public:
        DatabaseTracker(DatabaseProvider *dbpvd, timespec unit):
            std::thread(tmain, this, dbpvd, unit) {}
    };
    RWLock lock;
    hid_t db_id;
    hid_t ds_id;
    const char *gps;
    int gps_fptr;
    int line;//next reading line
    DatabaseTracker *dt;
    timespec recvTime;
public:
    DatabaseProvider(): lock() {}
    ~DatabaseProvider();
    int UpdateDatabase();
    int init(const char *database_name, int ninterface, const char *gps, timespec unit);
    //gps = nullptr if on proxy;
    const DatabaseMetadata* acquireMeta()
    {
        lock.readLock();
        return (DatabaseMetadata*)this;
    }
    inline void releaseMeta()
    {
        lock.readRelease();
    }
    friend class DatabaseTracker;
};

extern DatabaseProvider *dbprovider;

}//m3

#endif