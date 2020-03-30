/*-
*
*   Copyright(c) 2017 Intel Corporation. All rights reserved.
*
*/

#ifndef NETUIO_DEV_H
#define NETUIO_DEV_H

EXTERN_C_START

#include "netuio_interface.h"

// Constants
#define USER_MEMORY_SEGMENT_SIZE (256ULL * 1024ULL * 1024ULL)   // 256MB

struct pci_bar {
    PHYSICAL_ADDRESS base_addr;
    PVOID            virt_addr;
    UINT64           size;
};

struct mem_map_region {
    PMDL               mdl;    // MDL describing the memory region
    struct mem_region  mem;    // Memory region details
};

// The device context performs the same job as a WDM device extension in the driver frameworks
typedef struct _NETUIO_CONTEXT_DATA
{
    WDFDEVICE               wdf_device;             // WDF device handle to the FDO
    BUS_INTERFACE_STANDARD  bus_interface;          // Bus interface for config space access
    struct pci_bar          bar[PCI_MAX_BAR];       // device BARs
    struct dev_addr         addr;                   // B:D:F details of device
    USHORT                  dev_numa_node;          // The NUMA node of the device
    struct mem_map_region   dpdk_hw[PCI_MAX_BAR];   // mapped region for the device's register space
    struct mem_map_region   dpdk_seg;               // mapped region allocated for DPDK process use
} NETUIO_CONTEXT_DATA, *PNETUIO_CONTEXT_DATA;


// This macro will generate an inline function called DeviceGetContext
// which will be used to get a pointer to the device context memory in a type safe manner.
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(NETUIO_CONTEXT_DATA, netuio_get_context_data)


// Function to initialize the device and its callbacks
NTSTATUS netuio_create_device(_Inout_ PWDFDEVICE_INIT DeviceInit);
NTSTATUS netuio_map_hw_resources(_In_ WDFDEVICE Device, _In_ WDFCMRESLIST Resources, _In_ WDFCMRESLIST ResourcesTranslated);
VOID netuio_free_hw_resources(_In_ WDFDEVICE Device);


// Function called for cleanup when device object is being destroyed
VOID netuio_evt_device_context_cleanup(_In_ WDFOBJECT Device);

// Local function protoyypes
static NTSTATUS get_pci_device_info(_In_ WDFOBJECT device);
static NTSTATUS create_device_specific_symbolic_link(_In_ WDFOBJECT device);
static NTSTATUS allocate_usermemory_segment(_In_ WDFOBJECT device);
static VOID free_usermemory_segment(_In_ WDFOBJECT device);


EXTERN_C_END

#endif // NETUIO_DEV_H
