/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
*/

#ifndef _UNISTD_H
#define _UNISTD_H

/* This header file is required to build other rte* libraries and applications */

//#include <io.h>
#include <getopt.h> /* getopt at: https://gist.github.com/bikerm16/1b75e2dd20d839dcea58 */

/* Types used by Unix-y systems */
typedef __int16           int16_t;
typedef __int32           int32_t;
typedef __int64           int64_t;
typedef unsigned __int8   uint8_t;
typedef unsigned __int16  uint16_t;
typedef unsigned __int32  uint32_t;
typedef unsigned __int64  uint64_t;

#define srandom srand
#define random rand
#define _SC_PAGESIZE
#define sysconf getpagesize
/* function prototypes */
int getpagesize(void);

#endif /* unistd.h  */

