/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
*/

#pragma once

#include "..\..\common\include\generic\rte_cycles.h"

static inline uint64_t
rte_rdtsc(void)
{
	return (uint64_t) __rdtsc();
}

static inline uint64_t
rte_rdtsc_precise(void)
{
	rte_mb();
	return rte_rdtsc();
}

static inline uint64_t
rte_get_tsc_cycles(void) 
{ 
	return rte_rdtsc(); 
}
