/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
*/

#pragma once

/* include the original file, so that we can re-define certain macros */
#include "..\..\..\..\drivers\bus\pci\rte_bus_pci.h"

/* Need to re-define RTE_PMD_REGISTER_PCI for Windows */
#ifdef RTE_PMD_REGISTER_PCI(nm, pci_drv)
#undef RTE_PMD_REGISTER_PCI(nm, pci_drv)
#endif

/*
* Definition for registering PMDs
* (This is a workaround for Windows in lieu of a constructor-like function)
*/
#define RTE_PMD_REGISTER_PCI(nm, pci_drv) \
void pciinitfn_##nm(void); \
void pciinitfn_##nm(void) \
{\
	(pci_drv).driver.name = RTE_STR(nm);\
	rte_pci_register(&pci_drv); \
}

