/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
*/

#include <unistd.h>
#include <windows.h>

int getpagesize(void)
{
    SYSTEM_INFO si;

    GetSystemInfo(&si);

    return si.dwPageSize;
}

int getdtablesize(void)
{
	// Return OPEN_MAX (256)
	return 256;
}