/*-
*
*   Copyright(c) 2017 Intel Corporation. All rights reserved.
*
*/


#include "netuio_drv.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, netuio_queue_initialize)
#endif

VOID netuio_read_PCI_config(PNETUIO_CONTEXT_DATA netuio_contextdata, ULONG offset, UINT32 access_size, _Out_ UINT64 *output)
{
    *output = 0;
    netuio_contextdata->bus_interface.GetBusData(netuio_contextdata->bus_interface.Context,
                                                 PCI_WHICHSPACE_CONFIG,
                                                 output,
                                                 offset,
                                                 access_size);
}

VOID netuio_write_PCI_config(PNETUIO_CONTEXT_DATA netuio_contextdata, ULONG offset, UINT32 access_size, union dpdk_pci_config_io_data const *input)
{
    netuio_contextdata->bus_interface.SetBusData(netuio_contextdata->bus_interface.Context,
                                                 PCI_WHICHSPACE_CONFIG,
                                                 (PVOID)input,
                                                 offset,
                                                 access_size);
}

static NTSTATUS
netuio_handle_get_hw_data_request(_In_ WDFREQUEST Request, _In_ PNETUIO_CONTEXT_DATA netuio_contextdata,
                                   _In_ PVOID outputBuf, _In_ size_t outputBufSize)
{
    NTSTATUS status = STATUS_SUCCESS;

    WDF_REQUEST_PARAMETERS params;
    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(Request, &params);

    if (!netuio_contextdata || (outputBufSize != sizeof(struct dpdk_private_info))) {
        status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    struct dpdk_private_info *dpdk_pvt_info = (struct dpdk_private_info *)outputBuf;
    RtlZeroMemory(dpdk_pvt_info, outputBufSize);

    dpdk_pvt_info->hw.phys_addr.QuadPart = netuio_contextdata->bar[0].base_addr.QuadPart;
    dpdk_pvt_info->hw.user_mapped_virt_addr = netuio_contextdata->dpdk_hw.mem.user_mapped_virt_addr;
    dpdk_pvt_info->hw.size = netuio_contextdata->bar[0].size;

    dpdk_pvt_info->ms.phys_addr.QuadPart = netuio_contextdata->dpdk_seg.mem.phys_addr.QuadPart;
    dpdk_pvt_info->ms.user_mapped_virt_addr = netuio_contextdata->dpdk_seg.mem.user_mapped_virt_addr;
    dpdk_pvt_info->ms.size = netuio_contextdata->dpdk_seg.mem.size;
end:
    return status;
}

/*
Routine Description:
    The I/O dispatch callbacks for the frameworks device object are configured here.
    A single default I/O Queue is configured for parallel request processing, and a
    driver context memory allocation is created to hold our structure QUEUE_CONTEXT.

Return Value:
    None
 */
NTSTATUS
netuio_queue_initialize(_In_ WDFDEVICE Device)
{
    WDFQUEUE queue;
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG    queueConfig;

    PAGED_CODE();

    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);

    queueConfig.EvtIoDeviceControl = netuio_evt_IO_device_control;
    queueConfig.EvtIoStop = netuio_evt_IO_stop;

    status = WdfIoQueueCreate(Device,
                              &queueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &queue);

    if( !NT_SUCCESS(status) ) {
        return status;
    }

    return status;
}

/*
Routine Description:
    This event is invoked when the framework receives IRP_MJ_DEVICE_CONTROL request.

Return Value:
    None
 */
VOID
netuio_evt_IO_device_control(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request,
                              _In_ size_t OutputBufferLength, _In_ size_t InputBufferLength,
                              _In_ ULONG IoControlCode)
{
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    NTSTATUS status = STATUS_SUCCESS;
    PVOID    input_buf = NULL, output_buf = NULL;
    size_t   input_buf_size, output_buf_size;
    size_t  bytes_returned = 0;

    WDFDEVICE device = WdfIoQueueGetDevice(Queue);

    PNETUIO_CONTEXT_DATA  netuio_contextdata;
    netuio_contextdata = netuio_get_context_data(device);

    switch (IoControlCode) {
    case IOCTL_NETUIO_GET_HW_DATA:
        // First retrieve the input buffer and see if it matches our device
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(struct dpdk_private_info), &input_buf, &input_buf_size);
        if (!NT_SUCCESS(status)) {
            status = STATUS_INVALID_BUFFER_SIZE;
            break;
        }

        struct dpdk_private_info *dpdk_pvt_info = (struct dpdk_private_info *)input_buf;
        // Ensure that the B:D:F match - otherwise, fail the IOCTL
        if ((netuio_contextdata->addr.bus_num != dpdk_pvt_info->dev_addr.bus_num) ||
            (netuio_contextdata->addr.dev_num != dpdk_pvt_info->dev_addr.dev_num) ||
            (netuio_contextdata->addr.func_num != dpdk_pvt_info->dev_addr.func_num)) {
            status = STATUS_NOT_SAME_DEVICE;
            break;
        }

        // Map the previously allocated/defined memory regions to the user's process context
        MmBuildMdlForNonPagedPool(netuio_contextdata->dpdk_hw.mdl);
        netuio_contextdata->dpdk_hw.mem.user_mapped_virt_addr =
                            MmMapLockedPagesSpecifyCache(netuio_contextdata->dpdk_hw.mdl, UserMode, MmCached,
        NULL, FALSE, (NormalPagePriority | MdlMappingNoExecute));

        MmBuildMdlForNonPagedPool(netuio_contextdata->dpdk_seg.mdl);
        netuio_contextdata->dpdk_seg.mem.user_mapped_virt_addr =
                            MmMapLockedPagesSpecifyCache(netuio_contextdata->dpdk_seg.mdl, UserMode, MmCached,
        NULL, FALSE, (NormalPagePriority | MdlMappingNoExecute));

        if (!netuio_contextdata->dpdk_hw.mem.user_mapped_virt_addr && !netuio_contextdata->dpdk_seg.mem.user_mapped_virt_addr) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        // Zero out the physically contiguous block
        RtlZeroMemory(netuio_contextdata->dpdk_seg.mem.virt_addr, netuio_contextdata->dpdk_seg.mem.size);

        // Return relevant data to the caller
        status = WdfRequestRetrieveOutputBuffer(Request, sizeof(struct dpdk_private_info), &output_buf, &output_buf_size);
        if (!NT_SUCCESS(status)) {
            status = STATUS_INVALID_BUFFER_SIZE;
            break;
        }
        ASSERT(output_buf_size == OutputBufferLength);
        status = netuio_handle_get_hw_data_request(Request, netuio_contextdata, output_buf, output_buf_size);
        if (NT_SUCCESS(status))
            bytes_returned = output_buf_size;

        break;

    case IOCTL_NETUIO_PCI_CONFIG_IO:
        // First retrieve the input buffer and see if it matches our device
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(struct dpdk_pci_config_io), &input_buf, &input_buf_size);
        if (!NT_SUCCESS(status)) {
            status = STATUS_INVALID_BUFFER_SIZE;
            break;
        }

        struct dpdk_pci_config_io *dpdk_pci_io_input = (struct dpdk_pci_config_io *)input_buf;

        if (dpdk_pci_io_input->access_size != 1 &&
            dpdk_pci_io_input->access_size != 2 &&
            dpdk_pci_io_input->access_size != 4 &&
            dpdk_pci_io_input->access_size != 8) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        // Ensure that the B:D:F match - otherwise, fail the IOCTL
        if ((netuio_contextdata->addr.bus_num != dpdk_pci_io_input->dev_addr.bus_num) ||
            (netuio_contextdata->addr.dev_num != dpdk_pci_io_input->dev_addr.dev_num) ||
            (netuio_contextdata->addr.func_num != dpdk_pci_io_input->dev_addr.func_num)) {
            status = STATUS_NOT_SAME_DEVICE;
            break;
        }
        // Retrieve output buffer
        status = WdfRequestRetrieveOutputBuffer(Request, sizeof(UINT64), &output_buf, &output_buf_size);
        if (!NT_SUCCESS(status)) {
            status = STATUS_INVALID_BUFFER_SIZE;
            break;
        }
        ASSERT(output_buf_size == OutputBufferLength);

        if (dpdk_pci_io_input->op == PCI_IO_READ) {
            netuio_read_PCI_config(netuio_contextdata,
                                   dpdk_pci_io_input->offset,
                                   dpdk_pci_io_input->access_size,
                                   (UINT64*)output_buf);
            
            bytes_returned = sizeof(UINT64);
        }
        else if (dpdk_pci_io_input->op == PCI_IO_WRITE) {
            netuio_write_PCI_config(netuio_contextdata,
                                    dpdk_pci_io_input->offset,
                                    dpdk_pci_io_input->access_size,
                                    &dpdk_pci_io_input->data);
            bytes_returned = 0;
        }
        else {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        break;

    default:
        break;
    }

    WdfRequestCompleteWithInformation(Request, status, bytes_returned);

    return;
}

/*
Routine Description:
    This event is invoked for a power-managed queue before the device leaves the working state (D0).

Return Value:
    None
 */
VOID
netuio_evt_IO_stop(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request,_In_ ULONG ActionFlags)
{
    //
    // In most cases, the EvtIoStop callback function completes, cancels, or postpones
    // further processing of the I/O request.
    //
    // Typically, the driver uses the following rules:
    //
    // - If the driver owns the I/O request, it calls WdfRequestUnmarkCancelable
    //   (if the request is cancelable) and either calls WdfRequestStopAcknowledge
    //   with a Requeue value of TRUE, or it calls WdfRequestComplete with a
    //   completion status value of STATUS_SUCCESS or STATUS_CANCELLED.
    //
    //   Before it can call these methods safely, the driver must make sure that
    //   its implementation of EvtIoStop has exclusive access to the request.
    //
    //   In order to do that, the driver must synchronize access to the request
    //   to prevent other threads from manipulating the request concurrently.
    //   The synchronization method you choose will depend on your driver's design.
    //
    //   For example, if the request is held in a shared context, the EvtIoStop callback
    //   might acquire an internal driver lock, take the request from the shared context,
    //   and then release the lock. At this point, the EvtIoStop callback owns the request
    //   and can safely complete or requeue the request.
    //
    // - If the driver has forwarded the I/O request to an I/O target, it either calls
    //   WdfRequestCancelSentRequest to attempt to cancel the request, or it postpones
    //   further processing of the request and calls WdfRequestStopAcknowledge with
    //   a Requeue value of FALSE.
    //
    // A driver might choose to take no action in EvtIoStop for requests that are
    // guaranteed to complete in a small amount of time.
    //
    // In this case, the framework waits until the specified request is complete
    // before moving the device (or system) to a lower power state or removing the device.
    // Potentially, this inaction can prevent a system from entering its hibernation state
    // or another low system power state. In extreme cases, it can cause the system
    // to crash with bugcheck code 9F.
    //
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(ActionFlags);

    return;
}

