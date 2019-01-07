#ifndef __STRINGUTILS_H__
#define __STRINGUTILS_H__

#include <cstring>
#include <map>
#include <unordered_map>

#include "Represent.h"

namespace m3
{

struct StringHash
{
    // from https://www.byvoid.com/zhs/blog/string-hash-compare
    // BKDR Hash Function
    std::size_t operator()(const char* const& str) const
    {
        unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
        unsigned int hash = 0;
        const char *p = str;

        while (*p)
        {
            hash = hash * seed + (*p++);
        }

        return (hash & 0x7FFFFFFF);
    }
};

class StringCmp
{
public:
    int operator()(const char* const& lv, const char* const& rv)
    {
        return strcmp(lv, rv);
    }
};

struct StringEqual
{
    bool operator()(const char* const& str1, const char* const& str2) const
    {
        return strcmp(str1, str2) == 0;
    }
};

template <typename T>
using StringKeyHashMap = 
    std::unordered_map<const char*, T, StringHash, StringEqual>;

template <typename T>
using StringKeyMap = 
    std::map<const char*, T, StringCmp>;

const char* find(const char* str, const char c);
const char* find(const char* str, const char *pat);

static inline char *memnclone(const void *buf, int n)
{
    char *res = snew char[n];
    if (res != 0)
    {
        memcpy(res, buf, n);
    }
    return res;
}

static inline char *strnclone(const char *str, int n)
{
    char *res = snew char[n + 1];
    if (res != 0)
    {
        memcpy(res, str, n);
        res[n] = 0;
    }
    return res;
}

int substrcmp(const char* str, const char *pat);

#define forEachPartOfString(ite, str, split, splen)\
    for (ite = str; ite != 0; ite = find(ite, split) + splen)

}

#endif
