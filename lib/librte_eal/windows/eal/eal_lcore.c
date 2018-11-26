/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
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

