/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
*/

#pragma once

/* If rte_common.h has already been included, then we will have issues */
#ifdef _RTE_COMMON_H_
#error
#endif

#ifdef DPDKWIN_NO_WARNINGS
#pragma warning (disable : 42)
#endif

#include <rte_wincompat.h>

#include "../common/include/rte_common.h"

#ifdef DPDKWIN_NO_WARNINGS
#pragma warning (enable : 42)
#endif

#ifdef container_of
/* undefine the existing definition, so that we can use the Windows-compliant version */
#undef container_of
#endif

#define container_of(ptr, type, member)		CONTAINING_RECORD(ptr, type, member)


/* Override RTE_MIN() / RTE_MAX() as defined, since the one in rte_common uses typeof....TODO: Diagnose this later */
#undef RTE_MIN
#define RTE_MIN(a, b)	(((a) < (b)) ? (a) : (b))

#undef RTE_MAX
#define RTE_MAX(a, b)	max(a, b)

/* Redefine these macros with appropriate typecasting */
#undef RTE_ALIGN_FLOOR
#define RTE_ALIGN_FLOOR(val, align)		((uintptr_t)(val) & (~((uintptr_t)((align) - 1))))

#undef RTE_ALIGN_CEIL
#define RTE_ALIGN_CEIL(val, align)		RTE_ALIGN_FLOOR((val + ((uintptr_t)(align) - 1)), align)

#undef RTE_ALIGN
#define RTE_ALIGN(val, align)			RTE_ALIGN_CEIL(val, align)

#undef RTE_PTR_ALIGN_FLOOR
#define RTE_PTR_ALIGN_FLOOR(ptr, align)		(void *)(RTE_ALIGN_FLOOR((uintptr_t)ptr, align))

#undef RTE_PTR_ALIGN_CEIL
#define RTE_PTR_ALIGN_CEIL(ptr, align)		(void *)RTE_PTR_ALIGN_FLOOR((uintptr_t)RTE_PTR_ADD(ptr, (align) - 1), align)

#undef RTE_LEN2MASK
#define	RTE_LEN2MASK(ln, tp)			((uint64_t)-1 >> (sizeof(uint64_t) * CHAR_BIT - (ln)))

