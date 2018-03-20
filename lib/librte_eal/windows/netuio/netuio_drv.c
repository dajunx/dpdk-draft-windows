/*-
*
*   Copyright(c) 2017 Intel Corporation. All rights reserved.
*
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
    NTSTATUS status;
    WDF_PNPPOWER_EVENT_CALLBACKS    pnpPowerCallbacks;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    // Zero out the PnpPowerCallbacks structure
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

    // Register Plug-aNd-Play and power management callbacks
    pnpPowerCallbacks.EvtDevicePrepareHardware = netuio_evt_prepare_hw;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = netuio_evt_release_hw;

    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    status = netuio_create_device(DeviceInit);

    return status;
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
