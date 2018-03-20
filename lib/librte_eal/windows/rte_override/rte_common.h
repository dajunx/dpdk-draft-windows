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

