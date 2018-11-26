/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
*/

#include <rte_log.h>
#include "eal_internal_cfg.h"
#include "eal_hugepages.h"


/*
 * Need to complete hugepage support on Windows
 */
int
eal_hugepage_info_init(void)
{
	internal_config.num_hugepage_sizes = 1;
	internal_config.hugepage_info[0].hugepage_sz = (uint64_t)GetLargePageMinimum();

	return 0;
}
