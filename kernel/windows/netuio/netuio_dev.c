/*-
*
*   Copyright(c) 2017 Intel Corporation. All rights reserved.
*
*/


#include <stdio.h>
#include "netuio_drv.h"

#include <wdmguid.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, netuio_create_device)
#pragma alloc_text (PAGE, netuio_evt_device_context_cleanup)
#pragma alloc_text (PAGE, netuio_map_hw_resources)
#pragma alloc_text (PAGE, netuio_free_hw_resources)
#endif

/*
Routine Description:
    Worker routine called to create a device and its software resources.

Return Value:
    NTSTATUS
 */
NTSTATUS
netuio_create_device(_Inout_ PWDFDEVICE_INIT DeviceInit)
{
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDFDEVICE device;
    NTSTATUS status;

    PAGED_CODE();

	// Ensure that only administrators can access our device object.
	status = WdfDeviceInitAssignSDDLString(DeviceInit, &SDDL_DEVOBJ_SYS_ALL_ADM_ALL);

	if (NT_SUCCESS(status)) {
		WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, NETUIO_CONTEXT_DATA);

		// Set the device context cleanup callback.
		// This function will be called when the WDF Device Object associated to the current device is destroyed
		deviceAttributes.EvtCleanupCallback = netuio_evt_device_context_cleanup;

		status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
	}

	if (NT_SUCCESS(status)) {
		// Create a device interface so that applications can find and talk to us.
		status = WdfDeviceCreateDeviceInterface(device, &GUID_DEVINTERFACE_netUIO, NULL);
	}

    if (NT_SUCCESS(status)) {
        // Retrieve and store PCI information
        status = get_pci_device_info(device);
    }

    if (NT_SUCCESS(status)) {
        // Create a symbolic link name for user-space access
        status = create_device_specific_symbolic_link(device);
    }

    if (NT_SUCCESS(status)) {
        // Initialize the I/O Package and any Queues
        status = netuio_queue_initialize(device);
    }

    if (NT_SUCCESS(status)) {
        // Allocate physically contiguous memory for user process use. We'll map it later
        status = allocate_usermemory_segment(device);
    }

    return status;
}

/*
Routine Description:
    Free all the resources allocated in AdfEvtDeviceAdd.

Return Value:
    None
 */
VOID
netuio_evt_device_context_cleanup(_In_ WDFOBJECT Device)
{
    free_usermemory_segment(Device);
    return;
}

NTSTATUS
netuio_map_hw_resources(_In_ WDFDEVICE Device, _In_ WDFCMRESLIST Resources, _In_ WDFCMRESLIST ResourcesTranslated)
{
    UNREFERENCED_PARAMETER(Resources);

    NTSTATUS status = STATUS_SUCCESS;

    PNETUIO_CONTEXT_DATA  netuio_contextdata;
    netuio_contextdata = netuio_get_context_data(Device);

    if (!netuio_contextdata)
        return STATUS_UNSUCCESSFUL;

    PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;
    UINT8 bar_index = 0;

    // Collect device BAR resources from the ResourcesTranslated object
    for (ULONG idx = 0; idx < WdfCmResourceListGetCount(ResourcesTranslated); idx++) {
        descriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, idx);
        if (!descriptor) {
            status = STATUS_DEVICE_CONFIGURATION_ERROR;
            goto end;
        }

        switch (descriptor->Type) {
        case CmResourceTypeMemory:
            // Retrieve and map the BARs
            netuio_contextdata->bar[bar_index].base_addr.QuadPart = descriptor->u.Memory.Start.QuadPart;
            netuio_contextdata->bar[bar_index].size = descriptor->u.Memory.Length;
            netuio_contextdata->bar[bar_index].virt_addr =
                MmMapIoSpace(descriptor->u.Memory.Start, descriptor->u.Memory.Length, MmNonCached);

            if (netuio_contextdata->bar[bar_index].virt_addr == NULL) {
                status = STATUS_UNSUCCESSFUL;
                goto end;
            }

            bar_index++;
            break;

            // Don't handle any other resource type
            // This could be device-private type added by the PCI bus driver.
        case CmResourceTypeInterrupt:
        default:
            break;
        }
    }

    // Allocate an MDL for the device BAR, so that we can map it to the user's process context later...
    if (status == STATUS_SUCCESS) {
        // Bar 0 is typically the HW BAR
        if (netuio_contextdata->bar[0].virt_addr) {
            netuio_contextdata->dpdk_hw.mdl = IoAllocateMdl(netuio_contextdata->bar[0].virt_addr, (ULONG)netuio_contextdata->bar[0].size, FALSE, FALSE, NULL);
            if (!netuio_contextdata->dpdk_hw.mdl) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }
            netuio_contextdata->dpdk_hw.mem.size = netuio_contextdata->bar[0].size;
        }
    }

end:
    return status;
}

VOID
netuio_free_hw_resources(_In_ WDFDEVICE Device)
{
    PNETUIO_CONTEXT_DATA  netuio_contextdata;
    netuio_contextdata = netuio_get_context_data(Device);

    if (netuio_contextdata) {
        // Free the allocated MDL
        if (netuio_contextdata->dpdk_hw.mdl)
            IoFreeMdl(netuio_contextdata->dpdk_hw.mdl);

        // Unmap all the BAR regions previously mapped
        for (UINT8 bar_index = 0; bar_index < PCI_MAX_BAR; bar_index++) {
            if (netuio_contextdata->bar[bar_index].virt_addr)
                MmUnmapIoSpace(netuio_contextdata->bar[bar_index].virt_addr, netuio_contextdata->bar[bar_index].size);
        }
    }
}


static NTSTATUS
get_pci_device_info(_In_ WDFOBJECT device)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    PNETUIO_CONTEXT_DATA  netuio_contextdata;
    netuio_contextdata = netuio_get_context_data(device);

    if (!netuio_contextdata)
        return status;

    netuio_contextdata->wdf_device = device;  // Store for later use

    // Obtain the BUS_INTERFACE_STANDARD interface from the Bus Driver
    status = WdfFdoQueryForInterface(device, &GUID_BUS_INTERFACE_STANDARD,
                                    (PINTERFACE)&netuio_contextdata->bus_interface,
                                    sizeof(BUS_INTERFACE_STANDARD), 1, NULL);
    if (!NT_SUCCESS(status))
        return status;

    // Retrieve the B:D:F details of our device
    PDEVICE_OBJECT pdo = NULL;
    pdo = WdfDeviceWdmGetPhysicalDevice(device);
    if (pdo) {
        ULONG prop = 0, length = 0;
        status = IoGetDeviceProperty(pdo, DevicePropertyBusNumber, sizeof(ULONG), (PVOID)&netuio_contextdata->addr.bus_num, &length);
        status = IoGetDeviceProperty(pdo, DevicePropertyAddress, sizeof(ULONG), (PVOID)&prop, &length);

        if (NT_SUCCESS(status)) {
            netuio_contextdata->addr.func_num = prop & 0x0000FFFF;
            netuio_contextdata->addr.dev_num = ((prop >> 16) & 0x0000FFFF);
        }
        // Also, retrieve the NUMA node of the device
        USHORT numaNode;
        status = IoGetDeviceNumaNode(pdo, &numaNode);
        if (NT_SUCCESS(status)) {
            netuio_contextdata->dev_numa_node = numaNode;
        }
    }

    return status;
}

static NTSTATUS
create_device_specific_symbolic_link(_In_ WDFOBJECT device)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    UNICODE_STRING netuio_symbolic_link;

    PNETUIO_CONTEXT_DATA  netuio_contextdata;
    netuio_contextdata = netuio_get_context_data(device);

    if (!netuio_contextdata)
        return status;

    // Build symbolic link name as <netuio_symbolic_link>_BDF  (bus/device/func)
    CHAR  symbolic_link[64] = { 0 };
    sprintf_s(symbolic_link, sizeof(symbolic_link), "%s_%04d%02d%02d",
                            NETUIO_DEVICE_SYMBOLIC_LINK_ANSI, netuio_contextdata->addr.bus_num,
                            netuio_contextdata->addr.dev_num, netuio_contextdata->addr.func_num);

    ANSI_STRING ansi_symbolic_link;
    RtlInitAnsiString(&ansi_symbolic_link, symbolic_link);

    status = RtlAnsiStringToUnicodeString(&netuio_symbolic_link, &ansi_symbolic_link, TRUE);
    if (!NT_SUCCESS(status))
        return status;

    status = WdfDeviceCreateSymbolicLink(device, &netuio_symbolic_link);

    RtlFreeUnicodeString(&netuio_symbolic_link);

    return status;
}

static NTSTATUS
allocate_usermemory_segment(_In_ WDFOBJECT device)
{
    NTSTATUS status = STATUS_SUCCESS;

    PNETUIO_CONTEXT_DATA  netuio_contextdata;
    netuio_contextdata = netuio_get_context_data(device);

    if (!netuio_contextdata)
        return STATUS_UNSUCCESSFUL;

    PHYSICAL_ADDRESS lowest_acceptable_address;
    PHYSICAL_ADDRESS highest_acceptable_address;
    PHYSICAL_ADDRESS boundary_address_multiple;

    lowest_acceptable_address.QuadPart = 0x0000000000800000;
    highest_acceptable_address.QuadPart = 0xFFFFFFFFFFFFFFFF;
    boundary_address_multiple.QuadPart = 0;

    // Allocate physically contiguous memory for user process use
    netuio_contextdata->dpdk_seg.mem.virt_addr =
                MmAllocateContiguousMemorySpecifyCache(USER_MEMORY_SEGMENT_SIZE,
                                                       lowest_acceptable_address,
                                                       highest_acceptable_address,
                                                       boundary_address_multiple,
                                                       MmCached);

    if (!netuio_contextdata->dpdk_seg.mem.virt_addr) {
        status = STATUS_NO_MEMORY;
        goto end;
    }

    netuio_contextdata->dpdk_seg.mem.size = USER_MEMORY_SEGMENT_SIZE;

    // Allocate an MDL for this memory region - so that we can map it into the user's process context later
    netuio_contextdata->dpdk_seg.mdl = IoAllocateMdl((PVOID)netuio_contextdata->dpdk_seg.mem.virt_addr, USER_MEMORY_SEGMENT_SIZE, FALSE, FALSE, NULL);
    if (netuio_contextdata->dpdk_seg.mdl == NULL) {
        status = STATUS_NO_MEMORY;
        goto end;
    }

    // Store the region's physical address
    netuio_contextdata->dpdk_seg.mem.phys_addr = MmGetPhysicalAddress(netuio_contextdata->dpdk_seg.mem.virt_addr);

end:
    return status;
}

static VOID
free_usermemory_segment(_In_ WDFOBJECT device)
{
    PNETUIO_CONTEXT_DATA  netuio_contextdata;
    netuio_contextdata = netuio_get_context_data(device);

    if (netuio_contextdata) {
        if (netuio_contextdata->dpdk_seg.mdl)
            IoFreeMdl(netuio_contextdata->dpdk_seg.mdl);

        if (netuio_contextdata->dpdk_seg.mem.virt_addr)
            MmFreeContiguousMemory(netuio_contextdata->dpdk_seg.mem.virt_addr);
    }
}
