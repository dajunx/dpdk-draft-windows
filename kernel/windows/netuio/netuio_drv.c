/*
;   BSD LICENSE
;
;   Copyright (c) Microsoft Corporation. All rights reserved
;
;   Redistribution and use in source and binary forms, with or without
;   modification, are permitted provided that the following conditions
;   are met:
;
;     * Redistributions of source code must retain the above copyright
;       notice, this list of conditions and the following disclaimer.
;     * Redistributions in binary form must reproduce the above copyright
;       notice, this list of conditions and the following disclaimer in
;       the documentation and/or other materials provided with the
;       distribution.
;     * Neither the name of Intel Corporation nor the names of its
;       contributors may be used to endorse or promote products derived
;       from this software without specific prior written permission.
;
;   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
;   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
;   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
;   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
;   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
;   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
;   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
;   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
;   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
;   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
;   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "netuio_drv.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, netuio_evt_device_add)
#pragma alloc_text (PAGE, netuio_evt_driver_context_cleanup)
#endif


/*
Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry specifies the other entry
    points in the function driver, such as EvtDevice and DriverUnload.

Return Value:
    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.
 */
NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;

    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = netuio_evt_driver_context_cleanup;

    WDF_DRIVER_CONFIG_INIT(&config, netuio_evt_device_add);

    status = WdfDriverCreate(DriverObject, RegistryPath,
                             &attributes, &config,
                             WDF_NO_HANDLE);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    return status;
}


/*
Routine Description:
    netuio_evt_device_add is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device.

Return Value:
    NTSTATUS
 */
NTSTATUS
netuio_evt_device_add(_In_ WDFDRIVER Driver, _Inout_ PWDFDEVICE_INIT DeviceInit)
{
    UNREFERENCED_PARAMETER(Driver);
    return netuio_create_device(DeviceInit);
}

/*
Routine Description :
    Maps HW resources and retrieves the PCI BAR address(es) of the device

Return Value :
    STATUS_SUCCESS is successful.
    STATUS_<ERROR> otherwise
-*/
NTSTATUS
netuio_evt_prepare_hw(_In_ WDFDEVICE Device, _In_ WDFCMRESLIST Resources, _In_ WDFCMRESLIST ResourcesTranslated)
{
    NTSTATUS status;

    status = netuio_map_hw_resources(Device, Resources, ResourcesTranslated);

    if (NT_SUCCESS(status)) {
        PNETUIO_CONTEXT_DATA  netuio_contextdata;
        netuio_contextdata = netuio_get_context_data(Device);
        if (netuio_contextdata) {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_NETUIO_INFO_LEVEL, "netUIO Driver loaded...on device (B:D:F) %04d:%02d:%02d\n",
                                             netuio_contextdata->addr.bus_num, netuio_contextdata->addr.dev_num, netuio_contextdata->addr.func_num);
        }
    }
    return status;
}

/*
Routine Description :
    Releases the resource mapped by netuio_evt_prepare_hw

Return Value :
    STATUS_SUCCESS always.
-*/
NTSTATUS
netuio_evt_release_hw(_In_ WDFDEVICE Device, _In_ WDFCMRESLIST ResourcesTranslated)
{
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    netuio_free_hw_resources(Device);

    return STATUS_SUCCESS;
}

/*
Routine Description:
    Free all the resources allocated in DriverEntry.

Return Value:
    None
-*/
VOID
netuio_evt_driver_context_cleanup(_In_ WDFOBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_NETUIO_INFO_LEVEL, "netUIO Driver unloaded.\n");
    PAGED_CODE();
}

/*
Routine Description :
    EVT_WDF_FILE_CLEANUP callback, called when user process handle is closed.

    Undoes IOCTL_NETUIO_MAP_HW_INTO_USERMODE.

Return value :
    None
-*/
VOID
netuio_evt_file_cleanup(_In_ WDFFILEOBJECT FileObject)
{
    WDFDEVICE device;
    PNETUIO_CONTEXT_DATA netuio_contextdata;

    device = WdfFileObjectGetDevice(FileObject);
    netuio_contextdata = netuio_get_context_data(device);

    if (netuio_contextdata)
    {
        netuio_unmap_address_from_user_process(netuio_contextdata);
    }
}
