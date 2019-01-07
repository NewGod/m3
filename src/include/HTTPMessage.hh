#ifndef __HTTPMESSAGE_HH__
#define __HTTPMESSAGE_HH__

// Note: this is NOT a subclass of Message! We call this header HTTPMessage 
// just because HTTPPacket is not a correct name

// An example implementation compatible with HTTP/1.1. see RFC2616.

// todo: use HTTP parser to rewrite this.
// see https://github.com/h2o/picohttpparser

namespace m3
{

struct HTTPRequest
{
    const char *host;
    const char *uri;

    HTTPRequest(): host(0), uri(0) {}

    int init(const char *str);
    static bool isRequest(const char *str);
};

struct HTTPResponse
{
    const char *type;
    int len;

    HTTPResponse(): type(0), len(0) {}

    int init(const char *str);
    static bool isResponse(const char *str);
};

}

#endif
