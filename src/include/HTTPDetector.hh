#ifndef __HTTPDETECTOR_HH__
#define __HTTPDETECTOR_HH__

#include "HTTPMessage.hh"
#include "TypeDetector.hh"
#include "ConnectionType.hh"

namespace m3
{

class HTTPDetector : public TypeDetector
{
protected:
    class DumbTypeHTTP : public ConnectionType
    {
    protected:
        HTTPDetector *det;
    public:
        DumbTypeHTTP(HTTPDetector *det): det(det) {}

        TypeDetector *getDetector() override
        {
            return det;
        }
        Type getType() override
        {
            return ConnectionType::Type::HTTP_GENERIC;
        }
    };
    DumbTypeHTTP dumb;
    ConnectionType::Type setTypeForResponse(
        DataMessage *msg, HTTPResponse *resp);
    bool isRequest(char *str);
    bool isResponse(char *str);
public:
    HTTPDetector(): dumb(this) 
    {
        dumb.reference();
    }

    Type getType() override
    {
        return Type::HTTPDetector;
    }

    int getResponse(char *str, HTTPResponse *resp);
    int getRequest(char *str, HTTPRequest *req);
    ConnectionType::Type detectFrom(DataMessage *msg) override;
};

}

#endif
