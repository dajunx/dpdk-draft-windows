/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
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
	return (uint32_t)_byteswap_ulong((unsigned long)val);
}

static __forceinline uint64_t __builtin_bswap64(uint64_t val)
{
	return (uint64_t) _byteswap_uint64((unsigned long long)val);
}

typedef int useconds_t;
static void usleep(useconds_t us)
{
	LARGE_INTEGER cntr, start, current; 
	useconds_t curr_time; 

	QueryPerformanceFrequency(&cntr);
	QueryPerformanceCounter(&start); 

	do {
		QueryPerformanceCounter(&current);

		// Compute current time. 
		curr_time = ((current.QuadPart - start.QuadPart) / (float)cntr.QuadPart * 1000 * 1000); 
	} while (curr_time < us);

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

#define siglongjmp(a, err)  /* NO-OP */

#define strncasecmp(s1,s2,count)        _strnicmp(s1,s2,count)

// Replacement with safe string functions
#define strcpy(dest,src)                strcpy_s(dest,sizeof(dest),src)
#define strncpy(dest,src,count)         strncpy_s(dest,sizeof(dest),src,count)
#define strlcpy(dest,src,count)			strncpy_s(dest,sizeof(dest),src,count)
#define strerror(errnum)                WinSafeStrError(errnum)
#define strsep(str,sep)                 WinStrSep(str,sep)
#define strdup(str)                     _strdup(str)
#define strcat(dest,src)                strcat_s(dest,sizeof(dest),src)
#define sscanf(source,pattern, ...)		sscanf_s(source,pattern, __VA_ARGS__)


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
#define ftruncate(fd,len)			_chsize_s(fd,len)

// CPU set function overrides
#define CPU_ZERO(cpuset)                {*cpuset = 0;}
#define CPU_SET(cpucore, cpuset)        { *cpuset |= (1 << cpucore); }
#define CPU_ISSET(cpucore, cpuset)      ((*cpuset & (1 << cpucore)) ? 1 : 0)

/* Winsock IP protocol Numbers (not available on Windows) */
#define IPPROTO_NONE	59       /* No next header for IPv6 */
#define IPPROTO_SCTP	132      /* Stream Control Transmission Protocol */

/* signal definitions - defined in signal.h */
#define SIGUSR1		30
#define SIGUSR2		31

/* Definitions for access() */
#define F_OK	0	/* Check for existence */
#define W_OK	2	/* Write permission */
#define R_OK	4	/* Read permission */
#define X_OK	8	/* DO NOT USE */

#ifndef AF_INET6
#define AF_INET6	28
#endif

/* stdlib extensions that aren't defined in windows */
int setenv(const char *name, const char *value, int overwrite);

// Returns a handle to an mutex object that is created only once
static inline HANDLE OpenMutexHandleAsync(INIT_ONCE *g_InitOnce)
{
	PVOID  lpContext;
	BOOL   fStatus;
	BOOL   fPending;
	HANDLE hMutex;

	// Begin one-time initialization
	fStatus = InitOnceBeginInitialize(g_InitOnce,       // Pointer to one-time initialization structure
		INIT_ONCE_ASYNC,   // Asynchronous one-time initialization
		&fPending,         // Receives initialization status
		&lpContext);       // Receives pointer to data in g_InitOnce  

						   // InitOnceBeginInitialize function failed.
	if (!fStatus)
	{
		return (INVALID_HANDLE_VALUE);
	}

	// Initialization has already completed and lpContext contains mutex object.
	if (!fPending)
	{
		return (HANDLE)lpContext;
	}

	// Create Mutex object for one-time initialization.
	hMutex = CreateMutex(NULL,    // Default security descriptor
		FALSE,    // Manual-reset mutex object
		NULL);   // Object is unnamed

				 // mutex object creation failed.
	if (NULL == hMutex)
	{
		return (INVALID_HANDLE_VALUE);
	}

	// Complete one-time initialization.
	fStatus = InitOnceComplete(g_InitOnce,             // Pointer to one-time initialization structure
		INIT_ONCE_ASYNC,         // Asynchronous initialization
		(PVOID)hMutex);          // Pointer to mutex object to be stored in g_InitOnce

								 // InitOnceComplete function succeeded. Return mutex object.
	if (fStatus)
	{
		return hMutex;
	}

	// Initialization has already completed. Free the local mutex.
	CloseHandle(hMutex);


	// Retrieve the final context data.
	fStatus = InitOnceBeginInitialize(g_InitOnce,            // Pointer to one-time initialization structure
		INIT_ONCE_CHECK_ONLY,   // Check whether initialization is complete
		&fPending,              // Receives initialization status
		&lpContext);            // Receives pointer to mutex object in g_InitOnce

								// Initialization is complete. Return mutex.
	if (fStatus && !fPending)
	{
		return (HANDLE)lpContext;
	}
	else
	{
		return INVALID_HANDLE_VALUE;
	}
}

/*
 * Used to statically create and lock a mutex
*/
static inline HANDLE WinCreateAndLockStaticMutex(HANDLE mutex, INIT_ONCE *g_InitOnce) {
	mutex = OpenMutexHandleAsync(g_InitOnce);
	WaitForSingleObject(mutex, INFINITE);
	return mutex;
}

//#include <rte_gcc_builtins.h>


#ifdef __cplusplus
}
#endif

#endif

