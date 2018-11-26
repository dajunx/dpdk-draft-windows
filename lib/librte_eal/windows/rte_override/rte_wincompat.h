/*-
*   BSD LICENSE
*
*   Copyright(c) 2010-2017 Intel Corporation. All rights reserved.
*   All rights reserved.
*
*   Redistribution and use in source and binary forms, with or without
*   modification, are permitted provided that the following conditions
*   are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in
*       the documentation and/or other materials provided with the
*       distribution.
*     * Neither the name of Intel Corporation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
*   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*   OF THIS SOFTWARE, EVEN I ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _RTE_WINCOMPAT_H_
#define _RTE_WINCOMPAT_H_

#if !defined _M_IX86 && !defined _M_X64
#error Unsupported architecture
#endif

#include <stdint.h>

/* Required for definition of read(), write() */
#include <io.h>
#include <intrin.h>

/* limits.h replacement */
#include <stdlib.h>

#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif


#ifndef EDQUOT
#define EDQUOT 0xFE
#endif

/* C++ only */
#ifdef __cplusplus
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Quick generic implemetation of popcount - all architectures */
static __forceinline int __popcount(unsigned int x)
{
	static const unsigned int m1 = 0x55555555; 
	static const unsigned int m2 = 0x33333333;
	static const unsigned int m4 = 0x0f0f0f0f;
	static const unsigned int h01 = 0x01010101;

	x -= (x >> 1) & m1;
	x = (x & m2) + ((x >> 2) & m2);
	x = (x + (x >> 4)) & m4; 
	return (x * h01) >> 24;
}

static __forceinline int __builtin_popcountl(unsigned long x)
{
	return __popcount((unsigned int)x);
}

static __forceinline int __builtin_popcountll(unsigned long long x)
{
	static const unsigned long long m1 = 0x5555555555555555LL; 
	static const unsigned long long m2 = 0x3333333333333333LL;
	static const unsigned long long m4 = 0x0f0f0f0f0f0f0f0fLL;
	static const unsigned long long h01 = 0x0101010101010101LL;

	x -= (x >> 1) & m1;
	x = (x & m2) + ((x >> 2) & m2);
	x = (x + (x >> 4)) & m4; 
	return (x * h01) >> 56;
}

static __forceinline int __builtin_popcount(unsigned int x)
{
	return __popcount(x);
}

// __builtin_ctz - count of trailing zeroes
// _BitScanForward returns the bit number of first bit that is 1 starting from the LSB to MSB 
static __forceinline int __builtin_ctz(unsigned int x)
{
	unsigned long index = 0;

	if (_BitScanForward(&index, x))
		return index;

	return 32;
}

// __builtin_ctzl - count of trailing zeroes for long
static __forceinline int __builtin_ctzl(unsigned long x)
{
	return __builtin_ctz((unsigned int) x);
}

// __builtin_ctzll - count of trailing zeroes for long long (64 bits)
static __forceinline int __builtin_ctzll(unsigned long long x)
{
	unsigned long index = 0;

	if (_BitScanForward64(&index, x))
		return (int) index;

	return 64;
}


// __builtin_clz - count of leading zeroes
// _BitScanReverse returns the bit number of first bit that is 1 starting from the MSB to LSB 
static __forceinline int __builtin_clz(unsigned int x)
{
	unsigned long index = 0;

	if (_BitScanReverse(&index, x))
		return ((sizeof(x) * CHAR_BIT) -1 - index);

	return 32;
}

// __builtin_clzl - count of leading zeroes for long
static __forceinline int __builtin_clzl(unsigned long x)
{
	return __builtin_clz((unsigned int) x);
}

// __builtin_clzll - count of leading zeroes for long long (64 bits)
static __forceinline int __builtin_clzll(unsigned long long x)
{
	unsigned long index = 0;

	if (_BitScanReverse64(&index, x))
		return ((sizeof(x) * CHAR_BIT) - 1 - index);

	return 64;
}

static __forceinline uint32_t __builtin_bswap32(uint32_t val)
{
	return  ((val & 0x000000ff) << 24) |
		((val & 0x0000ff00) << 8) |
		((val & 0x00ff0000) >> 8) |
		((val & 0xff000000) >> 24);
}

static inline int getuid (void)
{
	return 0;
}

#include <string.h>
static inline char* strtok_r(char *str, const char *delim, char **nextp)
{
	char *ret;

	if (str == NULL)
		str = *nextp;

	str += strspn(str, delim);
	if (*str == '\0')
		return NULL;

	ret = str;
	str += strcspn(str, delim);

	if (*str)
		*str++ = '\0';

	*nextp = str;
	return ret;
}

#define index(a, b)     strchr(a, b)
#define rindex(a, b)    strrchr(a, b)

#define pipe(i)         _pipe(i, 8192, _O_BINARY)


#define strncasecmp(s1,s2,count)        _strnicmp(s1,s2,count)

// Replacement with safe string functions
#define strcpy(dest,src)                strcpy_s(dest,sizeof(dest),src)
#define strncpy(dest,src,count)         strncpy_s(dest,sizeof(dest),src,count)
#define strerror(errnum)                WinSafeStrError(errnum)
#define strsep(str,sep)                 WinStrSep(str,sep)
#define strdup(str)                     _strdup(str)


static inline char* WinSafeStrError(int errnum)
{
	static char buffer[256];

	ZeroMemory(buffer, sizeof(buffer));
	strerror_s(buffer, sizeof(buffer), errnum);
	return buffer;
}

static inline char* WinStrSep(char** ppString, char* pSeparator)
{
	char *pStrStart = NULL;

	if ((ppString != NULL) && (*ppString != NULL) && (**ppString != '\0')) {
		pStrStart = *ppString;
		char *pStr = pStrStart + strcspn(pStrStart, pSeparator);

		if (pStr == NULL)
			*ppString = NULL;
		else {
			*pStr = '\0';
			*ppString = pStr + 1;
		}
	}

	return pStrStart;
}

#define sleep(secs)                 Sleep((secs)*1000)   // Windows Sleep() requires milliseconds

// If use of pthread-win32, define _USE_WIN_PTHREAD_ on your external project to prevent conflict with 
// followings stuff
#ifndef _USE_WIN_PTHREAD_

// CPU set function overrides
#define CPU_ZERO(cpuset)                {*cpuset = 0;}
#define CPU_SET(cpucore, cpuset)        { *cpuset |= (1 << cpucore); }
#define CPU_ISSET(cpucore, cpuset)      ((*cpuset & (1 << cpucore)) ? 1 : 0)

// pthread function overrides
#define pthread_self()                                          ((pthread_t)GetCurrentThreadId())
#define pthread_setaffinity_np(thread,size,cpuset)              WinSetThreadAffinityMask(thread, cpuset)
#define pthread_getaffinity_np(thread,size,cpuset)              WinGetThreadAffinityMask(thread, cpuset)
#define pthread_create(threadID, threadattr, threadfunc, args)  WinCreateThreadOverride(threadID, threadattr, threadfunc, args);

static inline int WinSetThreadAffinityMask(void* threadID, unsigned long *cpuset)
{
	DWORD dwPrevAffinityMask = SetThreadAffinityMask(threadID, *cpuset);
	return 0;
}

static inline int WinGetThreadAffinityMask(void* threadID, unsigned long *cpuset)
{
	/* Workaround for the lack of a GetThreadAffinityMask() API in Windows */
	DWORD dwPrevAffinityMask = SetThreadAffinityMask(threadID, 0x1); /* obtain previous mask by setting dummy mask */
	SetThreadAffinityMask(threadID, dwPrevAffinityMask); /* set it back! */
	*cpuset = dwPrevAffinityMask;
	return 0;
}

static inline int WinCreateThreadOverride(void* threadID, void* threadattr, void* threadfunc, void* args)
{
	HANDLE hThread;
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadfunc, args, 0, (LPDWORD)threadID);
	return ((hThread != NULL) ? 0: E_FAIL);
}

#endif // _USE_WIN_PTHREAD_

/* Winsock IP protocol Numbers (not available on Windows) */
#define IPPROTO_NONE	59       /* No next header for IPv6 */
#define IPPROTO_SCTP	132      /* Stream Control Transmission Protocol */

#ifdef __cplusplus
}
#endif

#endif

