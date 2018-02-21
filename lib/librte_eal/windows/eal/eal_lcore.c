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

#include <rte_windows.h>

/* global data structure that contains the CPU map */
static struct _win_cpu_map {
	unsigned numTotalProcessors;
	unsigned numProcessorSockets;
	unsigned numProcessorCores;
	unsigned reserved;
	struct _win_lcore_map {
		uint8_t    socketid;
		uint8_t    coreid;
	} win_lcore_map[RTE_MAX_LCORE];
} win_cpu_map = { 0 };


void eal_create_cpu_map()
{
	win_cpu_map.numTotalProcessors = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);

	LOGICAL_PROCESSOR_RELATIONSHIP lprocRel;
	DWORD  lprocInfoSize = 0;
	BOOL bHyperThreadingEnabled = FALSE;

	/* First get the processor package information */
	lprocRel = RelationProcessorPackage;
	/* Determine the size of buffer we need (pass NULL) */
	GetLogicalProcessorInformationEx(lprocRel, NULL, &lprocInfoSize);
	win_cpu_map.numProcessorSockets = lprocInfoSize / 48;

	lprocInfoSize = 0;
	/* Next get the processor core information */
	lprocRel = RelationProcessorCore;
	GetLogicalProcessorInformationEx(lprocRel, NULL, &lprocInfoSize);
	win_cpu_map.numProcessorCores = lprocInfoSize / 48;

	if (win_cpu_map.numTotalProcessors > win_cpu_map.numProcessorCores)
	    bHyperThreadingEnabled = TRUE;

	/* Distribute the socket and core ids appropriately across the logical cores */
	/* For now, split the cores equally across the sockets - might need to revisit this later */
	unsigned lcore = 0;
	for (unsigned socket=0; socket < win_cpu_map.numProcessorSockets; ++socket) {
	    for (unsigned core = 0; core < (win_cpu_map.numProcessorCores / win_cpu_map.numProcessorSockets); ++core) {
		win_cpu_map.win_lcore_map[lcore].socketid = socket;
		win_cpu_map.win_lcore_map[lcore].coreid = core;

		lcore++;

		if (bHyperThreadingEnabled) {
		    win_cpu_map.win_lcore_map[lcore].socketid = socket;
		    win_cpu_map.win_lcore_map[lcore].coreid = core;
		    lcore++;
		}
	    }
	}

	return;
}

/* Check if a cpu is present by the presence of the cpu information for it */
int
eal_cpu_detected(unsigned lcore_id)
{
	return (lcore_id < win_cpu_map.numTotalProcessors);
}

/* Get CPU socket id (NUMA node) for a logical core */
unsigned
eal_cpu_socket_id(unsigned lcore_id)
{
	return win_cpu_map.win_lcore_map[lcore_id].socketid;
}

/* Get the cpu core id value */
unsigned
eal_cpu_core_id(unsigned lcore_id)
{
	return win_cpu_map.win_lcore_map[lcore_id].coreid;
}

