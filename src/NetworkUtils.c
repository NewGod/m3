#include "Log.h"
#include "NetworkUtils.h"
#include "immintrin.h"

#ifdef  __cplusplus  
extern "C" {  
#endif

sighandler signalNoRestart(int signum, sighandler handler)
{
    struct sigaction action, oldAction;
    char errbuf[256];

    action.sa_sigaction = handler; 
    // block all signal(except SIGTERM) here. We use SIGTERM instead of SIGKILL
    // as a final way of closing a program.
    sigfillset(&action.sa_mask);
    sigdelset(&action.sa_mask, SIGTERM);
    action.sa_flags = 0;

    if (sigaction(signum, &action, &oldAction) < 0)
        logFatal("sigaction() failed(%s).", strerrorV(errno, errbuf));
    return oldAction.sa_sigaction;
}

/*
 * rio_readn - robustly read n bytes (unbuffered)
 */
/* $begin rio_readn */
ssize_t rio_readn(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
	if ((nread = read(fd, bufp, nleft)) < 0) {
	    if (errno == EINTR) /* interrupted by sig handler return */
		nread = 0;      /* and call read() again */
	    else
		return -1;      /* errno set by read() */ 
	} 
	else if (nread == 0)
	    break;              /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);         /* return >= 0 */
}
/* $end rio_readn */

/*
 * rio_writen - robustly write n bytes (unbuffered)
 */
/* $begin rio_writen */
ssize_t rio_writen(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0) {
	if ((nwritten = send(fd, bufp, nleft, MSG_DONTWAIT)) <= 0) {
	    if (errno == EINTR)  /* interrupted by sig handler return */
		nwritten = 0;    /* and call write() again */
	    else if (errno == EAGAIN || errno == EWOULDBLOCK)
        return n - nleft;
        else
		return -1;       /* errorno set by write() */
	}
	nleft -= nwritten;
	bufp += nwritten;
    }
    return n;
}
/* $end rio_writen */


/* 
 * rio_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
/* $begin rio_read */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;

    while (rp->rio_cnt <= 0) {  /* refill if buf is empty */
	rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, 
			   sizeof(rp->rio_buf));
	if (rp->rio_cnt < 0) {
	    if (errno != EINTR) /* interrupted by sig handler return */
		return -1;
	}
	else if (rp->rio_cnt == 0)  /* EOF */
	    return 0;
	else 
	    rp->rio_bufptr = rp->rio_buf; /* reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;          
    if (rp->rio_cnt < n)   
	cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}
/* $end rio_read */

/*
 * rio_readinitb - Associate a descriptor with a read buffer and reset buffer
 */
/* $begin rio_readinitb */
void rio_readinitb(rio_t *rp, int fd) 
{
    rp->rio_fd = fd;  
    rp->rio_cnt = 0;  
    rp->rio_bufptr = rp->rio_buf;
}
/* $end rio_readinitb */

/*
 * rio_readnb - Robustly read n bytes (buffered)
 */
/* $begin rio_readnb */
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;
    
    while (nleft > 0) {
	if ((nread = rio_read(rp, bufp, nleft)) < 0) {
	    if (errno == EINTR) /* interrupted by sig handler return */
		nread = 0;      /* call read() again */
	    else
		return -1;      /* errno set by read() */ 
	} 
	else if (nread == 0)
	    break;              /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);         /* return >= 0 */
}
/* $end rio_readnb */

/* 
 * rio_readlineb - robustly read a text line (buffered)
 */
/* $begin rio_readlineb */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) 
{
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) { 
	if ((rc = rio_read(rp, &c, 1)) == 1) {
	    *bufp++ = c;
	    if (c == '\n')
		break;
	} else if (rc == 0) {
	    if (n == 1)
		return 0; /* EOF, no data read */
	    else
		break;    /* EOF, some data was read */
	} else
	    return -1;	  /* error */
    }
    *bufp = 0;
    return n;
}
/* $end rio_readlineb */

// rio_read/write that returns on interrupt.
ssize_t rio_readnr(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = (char*)usrbuf;
    char errbuf[256];

    while (nleft > 0)
    {
        if ((nread = read(fd, bufp, nleft)) < 0)
        {
            if (errno == EINTR)
            {
                logVerbose("Read interrupted.");
                break;
            }
            else
            {
                logError("Read error(%s).", strerrorV(errno, errbuf));
                logVerboseL(2, "Got %ld bytes.", (ssize_t)(n - nleft));
                return -1; 
            }
        } 
        else if (nread == 0)
        {
            logDebug("EOF reached.");
            break;
        }    
        nleft -= nread;
        bufp += nread;
    }

    logVerboseL(2, "Got %ld bytes.", (ssize_t)(n - nleft));
    return (n - nleft);
}

ssize_t rio_writenr(int fd, const void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nwritten;
    const char *bufp = (const char*)usrbuf;
    char errbuf[256];

    while (nleft > 0) 
    {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) 
        {
            if (errno == EINTR)
            {
                logVerbose("Write interrupted.");
                n -= nleft;
                break;
            }
            else
            {
                logError("Read error(%s).", strerrorV(errno, errbuf));
                n = -1;
                break;
            }
        }
        nleft -= nwritten;
        bufp += nwritten;
    }

    logVerboseL(2, "Sent %ld bytes.", (ssize_t)n);
    return n;
}

// open_listenfd code comes from CS:APP2e example code pack: 
// http://csapp.cs.cmu.edu/public/code.html
// Modified to support bind local IP(code from below).
typedef struct sockaddr SA;
#define LISTENQ 1024 
int open_listenfd(const char *local, int port)
{
    int listenfd, optval = 1;
    int ret = -1;
    struct sockaddr_in *serveraddr, addr;
    struct addrinfo hints, *local_res;

    if (local) {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = IPPROTO_TCP;
        if (getaddrinfo(local, NULL, &hints, &local_res) != 0)
            return -1;
    }

    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        goto open_listenfd_out;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval, sizeof(int)) < 0)
        goto open_listenfd_out;

    /* Listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    if (local)
    {
        serveraddr = (struct sockaddr_in*)local_res->ai_addr;
        serveraddr->sin_port = htons((unsigned short)port);
    }
    else
    {
        bzero((char *)&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons((unsigned short)port);
        serveraddr = &addr;
    }
    if (bind(listenfd, (SA *)serveraddr, sizeof(SA)) < 0)
        goto open_listenfd_out;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        goto open_listenfd_out;
    
    ret = listenfd;

open_listenfd_out:
    if (local)
    {
        freeaddrinfo(local_res);
    }
    return ret;
}

/* netdial and netannouce code comes from libtask: http://swtch.com/libtask/
 * Copyright: http://swtch.com/libtask/COPYRIGHT
*/

/* make connection to server */
int
netdial(int domain, int proto, const char *local, int local_port, const char *server, int port)
{
    struct addrinfo hints, *local_res, *server_res;
    int s;

    if (local) {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = domain;
        hints.ai_socktype = proto;
        if (getaddrinfo(local, NULL, &hints, &local_res) != 0)
            return -1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = domain;
    hints.ai_socktype = proto;
    if (getaddrinfo(server, NULL, &hints, &server_res) != 0)
        return -1;

    s = socket(server_res->ai_family, proto, 0);
    if (s < 0) {
	if (local)
	    freeaddrinfo(local_res);
	freeaddrinfo(server_res);
        return -1;
    }

    if (local) {
        if (local_port) {
            struct sockaddr_in *lcladdr;
            lcladdr = (struct sockaddr_in *)local_res->ai_addr;
            lcladdr->sin_port = htons(local_port);
            local_res->ai_addr = (struct sockaddr *)lcladdr;
        }

        if (bind(s, (struct sockaddr *) local_res->ai_addr, local_res->ai_addrlen) < 0) {
	    close(s);
	    freeaddrinfo(local_res);
	    freeaddrinfo(server_res);
            return -1;
	}
        freeaddrinfo(local_res);
    }

    ((struct sockaddr_in *) server_res->ai_addr)->sin_port = htons(port);
    if (connect(s, (struct sockaddr *) server_res->ai_addr, server_res->ai_addrlen) < 0 && errno != EINPROGRESS) {
	close(s);
	freeaddrinfo(server_res);
        return -1;
    }

    freeaddrinfo(server_res);
    return s;
}

/* 
* Copyright (c) 1982, 1986, 1988, 1990, 1993, 1994
*     The Regents of the University of California.  All rights reserved.
*
* Redistribution and use in source and binary form, with or without
* modifications, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. All advertising materials mentioning features or use of this software
*    must display the following acknowledgement:
*      This product includes software developed by the University of 
*      California, Berkeley and its contributors.
* 4. Neither the name of the University nor the names of its contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ''AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*/

unsigned short cksum(void *ip, int len){
    unsigned long sum = 0;  /* assume 32 bit long, 16 bit short */

#ifdef ENABLE_AVX
    __m128i psum = _mm_setzero_si128();
    __m128i foldmask = _mm_set1_epi32(0xffff);
    __m128i cmpmask = _mm_set1_epi32(0x80000000);
    int i = 0;
    for(; 2 * i + 7 < len; i += 4)
    {
        __m128i pv = _mm_loadl_epi64((void*)((short*)ip + i));
        pv = _mm_unpacklo_epi16(pv, _mm_setzero_si128());
        psum = _mm_add_epi32(psum, pv);
        __m128i psum_fold = _mm_add_epi32(_mm_srli_epi32(psum, 16), _mm_and_si128(psum, foldmask));
        __m128i cmpres = _mm_cmpeq_epi32(_mm_and_si128(psum, cmpmask), _mm_setzero_si128());
        psum = _mm_or_si128(_mm_and_si128(cmpres, psum), _mm_andnot_si128(cmpres, psum_fold));
    }
    unsigned int tmp[4];
    _mm_store_si128((void*)&tmp[0], psum);
    tmp[0] = tmp[0] + tmp[1];
    tmp[2] = tmp[2] + tmp[3];
    if(tmp[0] & 0x80000000) tmp[0] = (tmp[0] & 0xffff) + (tmp[0] >> 16);
    if(tmp[2] & 0x80000000) tmp[2] = (tmp[2] & 0xffff) + (tmp[2] >> 16);
    sum = tmp[0] + tmp[2];
    if(sum & 0x80000000) sum = (sum & 0xffff) + (sum >> 16);
    for(; 2 * i + 1 < len; i++)
    {
        sum += *((unsigned short*)ip + i);
        if(sum & 0xffff)
            sum = (sum & 0xffff) + (sum >> 16);
    }
    if(2 * i < len)
        sum += (unsigned short)*((unsigned char*)ip + len - 1);
#else
    while(len > 1){
        sum += *((unsigned short*) ip);
        ip = (struct ip*)((unsigned short*)ip + 1);
        if(sum & 0x80000000)   /* if high order bit set, fold */
        sum = (sum & 0xFFFF) + (sum >> 16);
        len -= 2;
    }

    if(len)       /* take care of left over byte */
        sum += (unsigned short) *(unsigned char *)ip;
#endif
    
    while(sum>>16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return ~sum;
}

unsigned short tcp_cksum(void* psd, void* tcph, int len){
    unsigned long sum = 0;  /* assume 32 bit long, 16 bit short */
    int i;

    for (i = 0; i < (sizeof(struct psdhdr) >> 1); ++i)
    {
        sum += *((unsigned short*) psd);
        psd = ((unsigned short*)psd + 1);
        if(sum & 0x80000000)   /* if high order bit set, fold */
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

#ifdef ENABLE_AVX
    __m128i psum = _mm_setzero_si128();
    __m128i foldmask = _mm_set1_epi32(0xffff);
    __m128i cmpmask = _mm_set1_epi32(0x80000000);
    i = 0;
    for(; 2 * i + 7 < len; i += 4)
    {
        __m128i pv = _mm_loadl_epi64((void*)((short*)tcph + i));
        pv = _mm_unpacklo_epi16(pv, _mm_setzero_si128());
        psum = _mm_add_epi32(psum, pv);
        __m128i psum_fold = _mm_add_epi32(_mm_srli_epi32(psum, 16), _mm_and_si128(psum, foldmask));
        __m128i cmpres = _mm_cmpeq_epi32(_mm_and_si128(psum, cmpmask), _mm_setzero_si128());
        psum = _mm_or_si128(_mm_and_si128(cmpres, psum), _mm_andnot_si128(cmpres, psum_fold));
    }
    unsigned int tmp[4];
    _mm_store_si128((void*)&tmp[0], psum);
    tmp[0] = tmp[0] + tmp[1];
    tmp[2] = tmp[2] + tmp[3];
    if(tmp[0] & 0x80000000) tmp[0] = (tmp[0] & 0xffff) + (tmp[0] >> 16);
    if(tmp[2] & 0x80000000) tmp[2] = (tmp[2] & 0xffff) + (tmp[2] >> 16);
    tmp[0] = tmp[0] + tmp[2];
    if(tmp[0] & 0x80000000) tmp[0] = (tmp[0] & 0xffff) + (tmp[0] >> 16);
    sum = sum + tmp[0];
    if(sum & 0x80000000) sum = (sum & 0xffff) + (sum >> 16);

    for(; 2 * i + 1 < len; i++)
    {
        sum += *((unsigned short*)tcph + i);
        if(sum & 0xffff)
            sum = (sum & 0xffff) + (sum >> 16);
    }
    if(2 * i < len)
        sum += (unsigned short)*((unsigned char*)tcph + len - 1);
#else
    while(len > 1){
        sum += *((unsigned short*) tcph);
        tcph = ((unsigned short*)tcph + 1);
        if(sum & 0x80000000)   /* if high order bit set, fold */
        sum = (sum & 0xFFFF) + (sum >> 16);
        len -= 2;
    }

    if(len)       /* take care of left over byte */
        sum += (unsigned short) *(unsigned char *)tcph;
#endif
    
    while(sum>>16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return ~sum;
}

/* taken from TCP/IP Illustrated Vol. 2(1995) by Gary R. Wright and W. Richard
Stevens. Page 236 */

// code from http://backreference.org/2010/03/26/tuntap-interface-tutorial/
int tun_alloc(char *dev, int flags) {
  struct ifreq ifr;
  int fd, err;
  char *clonedev = "/dev/net/tun";

  /* Arguments taken by the function:
   *
   * char *dev: the name of an interface (or '\0'). MUST have enough
   *   space to hold the interface name if '\0' is passed
   * int flags: interface flags (eg, IFF_TUN etc.)
   */

   /* open the clone device */
   if( (fd = open(clonedev, O_RDWR)) < 0 ) {
     return fd;
   }

   /* preparation of the struct ifr, of type "struct ifreq" */
   memset(&ifr, 0, sizeof(ifr));

   ifr.ifr_flags = flags;   /* IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI */

   if (*dev) {
     /* if a device name was specified, put it in the structure; otherwise,
      * the kernel will try to allocate the "next" device of the
      * specified type */
     strncpy(ifr.ifr_name, dev, IFNAMSIZ);
   }

   /* try to create the device */
   if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
     close(fd);
     return err;
   }

  /* if the operation was successful, write back the name of the
   * interface to the variable "dev", so the caller can know
   * it. Note that the caller MUST reserve space in *dev (see calling
   * code below) */
  strcpy(dev, ifr.ifr_name);

  /* this is the special file descriptor that the caller will use to talk
   * with the virtual interface */
  return fd;
}

// code from https://www.cnblogs.com/quicksnow/p/3299172.html, modified
int getDeviceAddress(const char *name, struct ifreq *hw,
    struct ifreq *ip)
{
    int sock;

    if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror ("socket");
        return -1;
    }

    strcpy (hw->ifr_name, name);
    if (ioctl (sock, SIOCGIFHWADDR, hw) < 0)
    {
        perror ("ioctl");
        return -2;
    }

    strcpy (ip->ifr_name, name);
    if (ioctl (sock, SIOCGIFADDR, ip) < 0)
    {
        perror ("ioctl");
        return -3;
    }

    close(sock);

    return 0;
}

char* inet_ntoa_r(struct in_addr in, char* buf)
{
    strcpy(buf, inet_ntoa(in));
    return buf;
};

char* dump_tcp_flags(struct tcphdr *tcph, char *buf)
{
    sprintf(buf, "%1x%1x", tcph->res1, tcph->doff);
    buf[2] = tcph->fin ? 'F' : '.';
    buf[3] = tcph->syn ? 'S' : '.';
    buf[4] = tcph->rst ? 'R' : '.';
    buf[5] = tcph->ack ? 'A' : '.';
    buf[6] = tcph->psh ? 'P' : '.';
    buf[7] = tcph->urg ? 'U' : '.';
    buf[8] = '0' + tcph->res2;
    buf[9] = 0;

    return buf;
}
//-----------------------------------------------------------------------------
// MurmurHash2 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

// Note - This code makes a few assumptions about how your machine behaves -

// 1. We can read a 4-byte value from any address without crashing
// 2. sizeof(int) == 4

// And it has a few limitations -

// 1. It will not work incrementally.
// 2. It will not produce the same results on little-endian and big-endian
//    machines.

//-----------------------------------------------------------------------------
// Platform-specific functions and macros

// Microsoft Visual Studio

#if defined(_MSC_VER)

#define BIG_CONSTANT(x) (x)

// Other compilers

#else	// defined(_MSC_VER)

#define BIG_CONSTANT(x) (x##LLU)

#endif // !defined(_MSC_VER)

//-----------------------------------------------------------------------------
// MurmurHash2, 64-bit versions, by Austin Appleby

// The same caveats as 32-bit MurmurHash2 apply here - beware of alignment 
// and endian-ness issues if used across multiple platforms.

// 64-bit hash for 64-bit platforms

uint64_t MurmurHash64A ( const void * key, int len, uint64_t seed )
{
  const uint64_t m = BIG_CONSTANT(0xc6a4a7935bd1e995);
  const int r = 47;

  uint64_t h = seed ^ (len * m);

  const uint64_t * data = (const uint64_t *)key;
  const uint64_t * end = data + (len/8);

  while(data != end)
  {
    uint64_t k = *data++;

    k *= m; 
    k ^= k >> r; 
    k *= m; 
    
    h ^= k;
    h *= m; 
  }

  const unsigned char * data2 = (const unsigned char*)data;

  switch(len & 7)
  {
  case 7: h ^= (uint64_t)(data2[6]) << 48;
  case 6: h ^= (uint64_t)(data2[5]) << 40;
  case 5: h ^= (uint64_t)(data2[4]) << 32;
  case 4: h ^= (uint64_t)(data2[3]) << 24;
  case 3: h ^= (uint64_t)(data2[2]) << 16;
  case 2: h ^= (uint64_t)(data2[1]) << 8;
  case 1: h ^= (uint64_t)(data2[0]);
          h *= m;
  };
 
  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
} 

//-----------------------------------------------------------------------------

#ifdef  __cplusplus  
} 
#endif  