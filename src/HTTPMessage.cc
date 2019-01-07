#include <regex>

#include "HTTPMessage.hh"
#include "Log.h"
#include "StringUtils.h"

namespace m3
{

#define CRLF "\r\n"
#define SLEN(str) (sizeof(str) - 1)
#define SPLEN (sizeof(CRLF) - 1)

#define forEachHTTPHeaderLine(lite, ite, str)\
    for (\
        lite = ite = str;\
        (unsigned long)ite != SPLEN && ite - lite != SPLEN;\
        lite = ite, ite = find(ite, CRLF) + SPLEN)

#define CIND(c) ((int)(unsigned char)c)

static const int uricmap[] =
{
    3, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const int statuscmap[] =
{
    3, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 2, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const char* checkMap(const int *cmap, const char *str)
{
    while (cmap[CIND(*(str++))] == 1);
    if (cmap[CIND(*(str - 1))] == 2)
    {
        return str - 1;
    }
    else
    {
        return 0;
    }
}

static inline const char *uriSplit(const char *str)
{
    return checkMap(uricmap, str);
}

static inline const char *statusSplit(const char *str)
{
    return checkMap(statuscmap, str);
}

// Note: we only support HTTP 1.0 and 1.1 here.
static inline bool checkHTTPVersion(const char *str)
{
    if (substrcmp(str, "HTTP/1."))
    {
        return false;
    }

    return str[SLEN("HTTP/1.")] == '0' || 
        str[SLEN("HTTP/1.")] == '1';
}

bool HTTPRequest::isRequest(const char *str)
{
    /*
    static const std::regex reqRegex = std::regex(
        "^GET \\S* HTTP/[0-9]\\.[0-9]\\r\\n");
    return std::regex_search(str, reqRegex);*/
    if (substrcmp(str, "GET "))
    {
        return false;
    }
    if ((str = uriSplit(str + SLEN("GET "))) == 0)
    {
        return false;
    }
    if (!checkHTTPVersion(++str))
    {
        return false;
    }
    return !substrcmp(str + SLEN("HTTP/1.0"), CRLF);
}

static inline bool checkHTTPStatus(const char *str)
{
    static const int cmap[] =
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    return str[-1] == ' ' && cmap[CIND(str[0])] && cmap[CIND(str[1])] && 
        cmap[CIND(str[2])] && str[3] == ' ';
}

bool HTTPResponse::isResponse(const char *str)
{
    /*
    static const std::regex reqRegex = std::regex(
        "^HTTP/[0-9]\\.[0-9] [0-9][0-9][0-9] [^\\r\\n]*\\r\\n");
    return std::regex_search(str, reqRegex); */
    if (!checkHTTPVersion(str))
    {
        return false;
    }
    if (!checkHTTPStatus(str + SLEN("HTTP/1.0 ")))
    {
        return false;
    }
    if ((str = statusSplit(str + SLEN("HTTP/1.0 200 "))) == 0)
    {
        return false;
    }
    return str[1] == '\n';
}

int HTTPRequest::init(const char *str)
{
    const char *ite, *lite;

    // get uri from first line
    // because we are parsing GET only
    uri = str + SLEN("GET ");

    forEachHTTPHeaderLine(lite, ite, str)
    {
        if (!substrcmp(ite, "Host: "))
        {
            host = ite + SLEN("Host: ");
            return 0;
        }
    }

    return 1;
}

int HTTPResponse::init(const char *str)
{
    const char *ite, *lite;
    int cnt = 2;

    forEachHTTPHeaderLine(lite, ite, str)
    {
        if (!substrcmp(ite, "Content-Length: "))
        {
            sscanf(ite + SLEN("Content-Length: "), "%d", &len);
            if (len == 0)
            {
                logMessage("LEN %s", str);
            }
            --cnt;
        }
        else if (!substrcmp(ite, "Content-Type: "))
        {
            type = ite + SLEN("Content-Type: ");
            --cnt;
        }
    }

    return cnt;
}

}