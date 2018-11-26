/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
*/

#include <rte_windows.h>

#include <rte_cycles.h>

#include "eal_private.h"

enum timer_source eal_timer_source;

uint64_t
get_tsc_freq_arch(void)
{
	/* This function is not supported on Windows */
	return 0;
}

uint64_t
get_tsc_freq(void)
{
	uint64_t tsc_freq;
	LARGE_INTEGER Frequency;

	QueryPerformanceFrequency(&Frequency);
	/* mulitply by 1K to obtain the true frequency of the CPU */
	tsc_freq = ((uint64_t)Frequency.QuadPart * 1024);

	return tsc_freq;
}

int
rte_eal_timer_init(void)
{
	eal_timer_source = EAL_TIMER_TSC;

	set_tsc_freq();
	return 0;
}
