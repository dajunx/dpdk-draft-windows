/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
*/

#pragma once

/* If rte_common.h has already been included, then we will have issues */
#ifdef _RTE_DEBUG_H_
#error
#endif

#include <stdio.h>
#include "../common/include/rte_debug.h"

#undef rte_panic
#define rte_panic(fmt, ...)	{ printf (fmt, ##__VA_ARGS__); while(1); }

#undef RTE_VERIFY
#define	RTE_VERIFY(exp)	do {											\
	if (!(exp))                                                         \
		rte_panic("line %d\tassert \"" #exp "\" failed\n", __LINE__);	\
} while (0)

