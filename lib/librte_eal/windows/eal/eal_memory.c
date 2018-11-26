/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
*/

#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <inttypes.h>
#include <fcntl.h>

#include <rte_eal.h>
#include <rte_eal_memconfig.h>
#include <rte_log.h>
#include <rte_string_fns.h>
#include <rte_fbarray.h>
#include <rte_errno.h>
#include "eal_private.h"
#include "eal_internal_cfg.h"
#include "eal_filesystem.h"

#define EAL_PAGE_SIZE	    (getpagesize())

/*
 * Get physical address of any mapped virtual address in the current process.
 */
phys_addr_t
rte_mem_virt2iova(const void *virtaddr)
{
	/* This function is only used by rte_mempool_virt2phy() when hugepages are disabled. */
	/* Get pointer to global configuration and calculate physical address offset */
	phys_addr_t physaddr;
	struct rte_memseg *ms;
	struct rte_mem_config *mcfg = rte_eal_get_configuration()->mem_config;
	if (mcfg) {
		ms = rte_fbarray_get(&mcfg->memsegs[0].memseg_arr, 0);
		if(ms == NULL)
			return RTE_BAD_PHYS_ADDR;
		physaddr = (phys_addr_t)((uintptr_t)ms->phys_addr + RTE_PTR_DIFF(virtaddr, mcfg->memsegs[0].base_va));
		return physaddr;
	}
	else
		return RTE_BAD_PHYS_ADDR;
}

int
rte_eal_hugepage_init(void)
{
	/* We have already initialized our memory whilst in the
	 * rte_pci_scan() call. Simply return here.
	*/
	return 0;
}

int
rte_eal_hugepage_attach(void)
{
	/* This function is called if our process is a secondary process
	 * and we need to attach to existing memory that has already
	 * been allocated.
	 * It has not been implemented on Windows
	 */
	return 0;
}

static int
alloc_va_space(struct rte_memseg_list *msl)
{
	uint64_t page_sz;
	size_t mem_sz;
	void *addr;
	int flags = 0;

#ifdef RTE_ARCH_PPC_64
	flags |= MAP_HUGETLB;
#endif

	page_sz = msl->page_sz;
	mem_sz = page_sz * msl->memseg_arr.len;

	addr = eal_get_virtual_area(msl->base_va, &mem_sz, page_sz, 0, flags);
	if (addr == NULL) {
		if (rte_errno == EADDRNOTAVAIL)
			RTE_LOG(ERR, EAL, "Could not mmap %llu bytes at [%p] - please use '--base-virtaddr' option\n",
			(unsigned long long)mem_sz, msl->base_va);
		else
			RTE_LOG(ERR, EAL, "Cannot reserve memory\n");
		return -1;
	}
	msl->base_va = addr;

	return 0;
}

static int __rte_unused
memseg_primary_init(void)
{
	/*
	*  Primary memory has already been initialized in store_memseg_info()
	*  Keeping the stub function for integration with common code.
	*/

	return 0;
}

static int
memseg_secondary_init(void)
{
	struct rte_mem_config *mcfg = rte_eal_get_configuration()->mem_config;
	int msl_idx = 0;
	struct rte_memseg_list *msl;

	for (msl_idx = 0; msl_idx < RTE_MAX_MEMSEG_LISTS; msl_idx++) {

		msl = &mcfg->memsegs[msl_idx];

		/* skip empty memseg lists */
		if (msl->memseg_arr.len == 0)
			continue;

		if (rte_fbarray_attach(&msl->memseg_arr)) {
			RTE_LOG(ERR, EAL, "Cannot attach to primary process memseg lists\n");
			return -1;
		}

		/* preallocate VA space */
		if (alloc_va_space(msl)) {
			RTE_LOG(ERR, EAL, "Cannot preallocate VA space for hugepage memory\n");
			return -1;
		}
	}

	return 0;
}


int
rte_eal_memseg_init(void)
{
	return rte_eal_process_type() == RTE_PROC_PRIMARY ? memseg_primary_init() : memseg_secondary_init();
}
