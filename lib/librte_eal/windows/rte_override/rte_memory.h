/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
*/

#pragma once

/* If rte_common.h has already been included, then we will have issues */
#ifdef _RTE_MEMORY_H_
#error
#endif

#ifdef DPDKWIN_NO_WARNINGS
#pragma warning (disable : 66)	/*  warning #66: enumeration value is out of "int" range */
#endif

#include "../common/include/rte_memory.h"

#ifdef DPDKWIN_NO_WARNINGS
#pragma warning (enable : 66)
#endif
