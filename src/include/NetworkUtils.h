#ifndef __NETWORKUTILS_H__
#define __NETWORKUTILS_H__

#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#ifdef  __cplusplus  
extern "C" {  
#endif  

struct psdhdr
{
	unsigned saddr;
	unsigned daddr;
	char mbz;
	char ptcl;
	unsigned short tcpl;
};

int netdial(int domain, int proto, const char *local, int local_port,
    const char *server, int port);
int open_listenfd(const char* local, int port);
ssize_t rio_readnr(int fd, void *usrbuf, size_t n);
ssize_t rio_writenr(int fd, const void *usrbuf, size_t n);

/* Persistent state for the robust I/O (Rio) package */
/* $begin rio_t */
#define RIO_BUFSIZE 8192
typedef struct {
    int rio_fd;                /* descriptor for this internal buf */
    int rio_cnt;               /* unread bytes in internal buf */
    char *rio_bufptr;          /* next unread byte in internal buf */
    char rio_buf[RIO_BUFSIZE]; /* internal buffer */
} rio_t;
/* $end rio_t */

/* Rio (Robust I/O) package */
ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_writen(int fd, void *usrbuf, size_t n);
void rio_readinitb(rio_t *rp, int fd); 
ssize_t	rio_readnb(rio_t *rp, void *usrbuf, size_t n);
ssize_t	rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

unsigned short cksum(void *ip, int len);
unsigned short tcp_cksum(void* psd, void* tcph, int len);

int tun_alloc(char *dev, int flags);

int getDeviceAddress(const char *name, struct ifreq *hw,
    struct ifreq *ip);

typedef void (*sighandler)(int, siginfo_t*, void*);
sighandler signalNoRestart(int signum, sighandler handler);

char* inet_ntoa_r(struct in_addr in, char* buf);
char* dump_tcp_flags(struct tcphdr *tcph, char *buf);

static inline unsigned short ntohs_noalign(void *ptr)
{
    unsigned char *cptr = (unsigned char*)ptr;

    return ((unsigned int)*cptr << 8) | (unsigned int)*(cptr + 1);
}

static inline void htons_noalign(void *dest, unsigned short val)
{
    unsigned char *cptr = (unsigned char*)dest;

    cptr[0] = val >> 8;
    cptr[1] = val & 0xFF;
}

static inline unsigned int ntohl_noalign(void *ptr)
{
    unsigned char *cptr = (unsigned char*)ptr;

    return ((unsigned int)*cptr << 24) | ((unsigned int)*(cptr + 1) << 16) | 
        ((unsigned int)*(cptr + 2) << 8) | (unsigned int)*(cptr + 3);
}

static inline void htonl_noalign(void *dest, unsigned short val)
{
    unsigned char *cptr = (unsigned char*)dest;

    cptr[0] = val >> 24;
    cptr[1] = (val >> 16) & 0xFF;
    cptr[2] = (val >> 8) & 0xFF;
    cptr[3] = val & 0xFF;
}

//-----------------------------------------------------------------------------
// MurmurHash2 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#ifndef _MURMURHASH2_H_
#define _MURMURHASH2_H_

//-----------------------------------------------------------------------------
// Platform-specific functions and macros

// Microsoft Visual Studio

#if defined(_MSC_VER) && (_MSC_VER < 1600)

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;

// Other compilers

#else	// defined(_MSC_VER)

#include <stdint.h>

#endif // !defined(_MSC_VER)

//-----------------------------------------------------------------------------

uint64_t MurmurHash64A      ( const void * key, int len, uint64_t seed );

//-----------------------------------------------------------------------------

#endif // _MURMURHASH2_H_

#ifdef  __cplusplus  
} 
#endif  

#endif
