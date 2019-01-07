#include "DatabaseProvider.hh"
#include "Log.h"
#include "InternalMessage.hh"
#include "ContextManager.hh"
#include <sys/mman.h>
#include <cstring>

namespace m3
{

DatabaseProvider *dbprovider = nullptr;

void DatabaseProvider::DatabaseTracker::tmain(
    DatabaseTracker *dbt,
    DatabaseProvider *dbpvd,
    timespec unit)
{
    logDebug("DatabaseProvider::DatabaseTracker::tmain: Thread identify");

    const int server = dbpvd->gps == nullptr;
    //0: client; 1: server
    const int NSEC_PER_SEC = 1000000000;
    while(true)
    {
        if(server)
        {
            Context *ctx;
            InternalMessageContext *ictx;
            InternalMessage *recvMsg;
            if(ContextManager::getInstance()->getContext(Context::Type::INTERNAL, &ctx) != 0)
            {
                logError("DatabaseProvider::DatabaseTracker::tmain: server getContext fail");
                goto out;
            }
            ictx = (InternalMessageContext*) ctx;
            while(ictx->tryPurchase(&recvMsg) == 0)
            {
                void *msg = (void*)(((DataMessageMetaHeader*)recvMsg->
                    getHeader()) + 1);
                GPSValue recvGPS;
                memcpy(&recvGPS, msg, sizeof(DatabaseMetadata::GPSValue));
                long dt = (recvGPS.timeStamp.tv_sec - dbpvd->recvTime.tv_sec) * NSEC_PER_SEC
                        + (recvGPS.timeStamp.tv_nsec - dbpvd->recvTime.tv_nsec);
                if(dt < 0) goto out;
                dbpvd->lock.writeLock();
                dbpvd->velocity.longitude = (recvGPS.longitude - dbpvd->currentGPS.longitude) * NSEC_PER_SEC / dt;
                dbpvd->velocity.latitude = (recvGPS.latitude - dbpvd->currentGPS.latitude) * NSEC_PER_SEC / dt;
                dbpvd->currentGPS = recvGPS;
                clock_gettime(CLOCK_MONOTONIC_COARSE, &dbpvd->currentGPS.timeStamp);
                dbpvd->recvTime = recvGPS.timeStamp;
                dbpvd->lock.writeRelease();
                delete recvMsg;
                dbpvd->UpdateDatabase();
            }
        }
        else
        {
            switch(dbpvd->UpdateDatabase())
            {
                case 1://fail
                case -1://no update
                    goto out;
                case 0:
                    break;
                default:
                    logError("DatabaseProvider::DatabaseTracker::tmain: unexpected updatedatabase return value");
                    goto out;
            }
            
            void *buf = InternalMessage::getMessageBuf();
            if(!buf)
            {
                logError("DatabaseProvider::DatabaseTracker::tmain: client getMessageBuf fail");
                goto out;
            }
            //assert sizeof(GPSValue) <= mss;
            dbpvd->lock.readLock();
            memcpy(buf, &dbpvd->currentGPS, sizeof(DatabaseMetadata::GPSValue));
            dbpvd->lock.readRelease();
            InternalMessage *Msg = snew InternalMessage(buf, sizeof(DatabaseMetadata::GPSValue));
            if(InternalMessage::sendInternalMessage(Msg) != 0)
            {
                logError("DatabaseProvider::DatabaseTracker::tmain: client sendInternalMessage fail");
                InternalMessage::releaseMessageBuf(buf);
                goto out;
            }
        }
out:
        nanosleep(&unit, 0);
    }
}

DatabaseProvider::~DatabaseProvider()
{
    delete[] (char*)this->DB;
    H5Fclose(this->db_id);
}
int DatabaseProvider::UpdateDatabase()
{
    hsize_t count[2] = {1, 1}, offset[2] = {0, 0}, stride[2] = {1, 1}, block[2] = {3, 0};
    const long NSEC_PER_SEC = 1000000000;
    if(gps != nullptr)
    {
        //update gps-> update db value
        FILE *fgps = fopen(gps, "r");
        if(!fgps)
        {
            logError("DatabaseProvider::UpadateDatabase: cannot open GPS file %s", gps);
            return 1;
        }
        bool newline = true;
        fseek(fgps, gps_fptr, SEEK_SET);
        char buf[128];
        if(!fgets(buf, 128, fgps))
        {
            newline = false;
        }
        gps_fptr = ftell(fgps);
        fclose(fgps);
        char dummy;
        double longi, lati;
        if(!newline) return -1;//no newline

        sscanf(buf + 15, "%lf,%c,%lf", &lati, &dummy, &longi);//not portable
        lock.writeLock();
        GPSValue newGPS;
        double flongi = floor(longi/100), flati = floor(lati/100);
        newGPS.longitude = flongi + (longi - flongi * 100) / 60;
        newGPS.latitude = flati + (lati - flati * 100) / 60;
        clock_gettime(CLOCK_MONOTONIC_COARSE, &newGPS.timeStamp);
        long dt = (newGPS.timeStamp.tv_sec - currentGPS.timeStamp.tv_sec) * NSEC_PER_SEC
                + (newGPS.timeStamp.tv_nsec - currentGPS.timeStamp.tv_nsec);
        velocity.longitude = (newGPS.longitude - currentGPS.longitude) * NSEC_PER_SEC / dt;
        velocity.latitude = (newGPS.latitude - currentGPS.latitude) * NSEC_PER_SEC / dt;
        currentGPS = newGPS;

        while(gps_diff(currentGPS, DBRef(0)) > gps_diff(currentGPS, DBRef(2)) || DBRef(0).longitude < 0)
        {
            block[1] = 2 + 3 * ninterface;
            offset[0] = line++;

            hid_t dataspace_id = H5Dget_space(this->ds_id);
            hid_t memspace_id = H5Screate_simple(2, block, NULL);
            H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, offset, stride, count, block);
            H5Dread(this->ds_id, H5T_NATIVE_DOUBLE, memspace_id, dataspace_id, H5P_DEFAULT, &DBRef(0));
        }
        lock.writeRelease();
    }
    else
    {
        //update db value
        lock.writeLock();
        while(gps_diff(currentGPS, DBRef(0)) > gps_diff(currentGPS, DBRef(2)) || DBRef(0).longitude < 0)
        {
            block[1] = 2 + 3 * ninterface;
            offset[0] = line++;

            hid_t dataspace_id = H5Dget_space(this->ds_id);
            hid_t memspace_id = H5Screate_simple(2, block, NULL);
            H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, offset, stride, count, block);
            H5Dread(this->ds_id, H5T_NATIVE_DOUBLE, memspace_id, dataspace_id, H5P_DEFAULT, &DBRef(0));
        }
        lock.writeRelease();
    }
    return 0;
}
int DatabaseProvider::init(const char *database_name, int ninterface, const char *gps, timespec unit)
{
    this->ninterface = ninterface;
    gps_fptr = 0;
    this->gps = gps;

    this->DB = snew char[3 * ( 2 * sizeof(double) + ninterface * sizeof(DBValue::Value) )];
    iferr(!this->DB)
    {
        logError("DatabaseProvider::init: Allocate DB failed");
        return 1;
    }
    memset(this->DB, 0, 3 * ( 2 * sizeof(double) + ninterface * sizeof(DBValue::Value) ));
    DBRef(0).longitude = DBRef(0).latitude = -1;
    currentGPS.timeStamp.tv_sec = currentGPS.timeStamp.tv_nsec = 0;
    velocity.longitude = velocity.latitude = 0;
    recvTime.tv_sec = recvTime.tv_nsec = 0;

    this->db_id = H5Fopen(database_name, H5F_ACC_RDONLY, H5P_DEFAULT);
    iferr(this->db_id < 0)
    {
        logError("DatabaseProvider::init: Unable to open database file %s", database_name);
        return 2;
    }
    this->ds_id = H5Dopen(this->db_id, "/dataset", H5P_DEFAULT);
    iferr(this->ds_id < 0)
    {
        logError("DatabaseProvider::init: No dataset '/dataset'");
        return 3;
    }
    line = 0;

    dt = snew DatabaseTracker(this, unit);
    iferr(!dt)
    {
        logError("DatabaseProvider::init: Create DatabaseTracker failed");
        H5Fclose(this->db_id);
        delete[] (char*)this->DB;
        return 4;
    }
    return 0;
}

}//m3