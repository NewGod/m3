#include "StringUtils.h"

namespace m3
{

const char* find(const char *str, const char c)
{
    while (*str)
    {
        if (*str == c)
        {
            return str;
        }
        str++;
    }
    return 0;
}

int substrcmp(const char* str, const char *pat)
{
    int j;
    for (j = 0; pat[j]; ++j)
    {
        int cmp = str[j] - pat[j];
        if (cmp != 0)
        {
            return cmp;
        }
    }
    return 0;
}

const char* find(const char* str, const char *pat)
{
    int i = 0;
    int j = 0;
    for (; str[i]; ++i)
    {
        for (j = 0; pat[j]; ++j)
        {
            if (str[i + j] != pat[j])
            {
                break;
            }
        }
        if (!pat[j])
        {
            return str + i;
        }
    }
    return 0;
}

}