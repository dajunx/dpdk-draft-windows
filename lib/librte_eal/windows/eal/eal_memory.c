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
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
	struct rte_mem_config *mcfg = rte_eal_get_configuration()->mem_config;
	if (mcfg) {
		physaddr = (phys_addr_t)((uintptr_t)mcfg->memseg[0].phys_addr + RTE_PTR_DIFF(virtaddr, mcfg->memseg[0].addr));
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
