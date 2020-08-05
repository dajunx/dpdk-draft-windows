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
#pragma alloc_text (PAGE, netuio_queue_initialize)
#endif

static void
netuio_handle_get_hw_data_request(_In_ WDFREQUEST Request, _In_ PNETUIO_CONTEXT_DATA netuio_contextdata,
                                   _In_ PVOID outputBuf, _In_ size_t outputBufSize)
{
    WDF_REQUEST_PARAMETERS params;
    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(Request, &params);

    ASSERT(outputBufSize == sizeof(struct dpdk_private_info));

    struct dpdk_private_info *dpdk_pvt_info = (struct dpdk_private_info *)outputBuf;
    RtlZeroMemory(dpdk_pvt_info, outputBufSize);

    for (ULONG idx = 0; idx < PCI_MAX_BAR; idx++) {
        dpdk_pvt_info->hw[idx].phys_addr.QuadPart = netuio_contextdata->bar[idx].base_addr.QuadPart;
        dpdk_pvt_info->hw[idx].user_mapped_virt_addr = netuio_contextdata->dpdk_hw[idx].mem.user_mapped_virt_addr;
        dpdk_pvt_info->hw[idx].size = netuio_contextdata->bar[idx].size;
    }

    dpdk_pvt_info->ms.phys_addr.QuadPart = netuio_contextdata->dpdk_seg.mem.phys_addr.QuadPart;
    dpdk_pvt_info->ms.user_mapped_virt_addr = netuio_contextdata->dpdk_seg.mem.user_mapped_virt_addr;
    dpdk_pvt_info->ms.size = netuio_contextdata->dpdk_seg.mem.size;
}

/*
Routine Description:
    Maps address ranges into the usermode process's address space.  The following
    ranges are mapped:

        * Any PCI BARs that our device was assigned
        * The scratch buffer of contiguous pages

Return Value:
    NTSTATUS
*/
static NTSTATUS
netuio_map_address_into_user_process(_In_ PNETUIO_CONTEXT_DATA netuio_contextdata)
{
    NTSTATUS status = STATUS_SUCCESS;

    // Map the scratch memory regions to the user's process context
    MmBuildMdlForNonPagedPool(netuio_contextdata->dpdk_seg.mdl);
    __try {
        netuio_contextdata->dpdk_seg.mem.user_mapped_virt_addr =
            MmMapLockedPagesSpecifyCache(netuio_contextdata->dpdk_seg.mdl, UserMode,
                                         MmCached, NULL, FALSE, NormalPagePriority);

        if (netuio_contextdata->dpdk_seg.mem.user_mapped_virt_addr == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        goto end;
    }

    // Map any device BAR(s) to the user's process context
    for (INT idx = 0; idx < PCI_MAX_BAR; idx++) {
        if (netuio_contextdata->dpdk_hw[idx].mdl == NULL) {
            continue;
        }

        MmBuildMdlForNonPagedPool(netuio_contextdata->dpdk_hw[idx].mdl);
        __try {
            netuio_contextdata->dpdk_hw[idx].mem.user_mapped_virt_addr =
                MmMapLockedPagesSpecifyCache(netuio_contextdata->dpdk_hw[idx].mdl, UserMode,
                                             MmCached, NULL, FALSE, NormalPagePriority);

            if (netuio_contextdata->dpdk_hw[idx].mem.user_mapped_virt_addr == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
            goto end;
        }
    }

end:
    if (status != STATUS_SUCCESS) {
		netuio_unmap_address_from_user_process(netuio_contextdata);
    }

    return status;
}

/*
Routine Description:
    Unmaps all address ranges from the usermode process address space.
    MUST be called in the context of the same process which created
    the mapping.

Return Value:
    None
 */
VOID
netuio_unmap_address_from_user_process(_In_ PNETUIO_CONTEXT_DATA netuio_contextdata)
{
    if (netuio_contextdata->dpdk_seg.mem.user_mapped_virt_addr != NULL) {
        MmUnmapLockedPages(
            netuio_contextdata->dpdk_seg.mem.user_mapped_virt_addr,
            netuio_contextdata->dpdk_seg.mdl);
        netuio_contextdata->dpdk_seg.mem.user_mapped_virt_addr = NULL;
    }

    for (INT idx = 0; idx < PCI_MAX_BAR; idx++) {
        if (netuio_contextdata->dpdk_hw[idx].mem.user_mapped_virt_addr != NULL) {
            MmUnmapLockedPages(
                netuio_contextdata->dpdk_hw[idx].mem.user_mapped_virt_addr,
                netuio_contextdata->dpdk_hw[idx].mdl);
            netuio_contextdata->dpdk_hw[idx].mem.user_mapped_virt_addr = NULL;
        }
    }
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
    This routine is invoked to preprocess an I/O request before being placed into a queue.
    It is guaranteed that it executes in the context of the process that generated the request.

Return Value:
    None
 */
_Use_decl_annotations_
VOID
netuio_evt_IO_in_caller_context(
    IN WDFDEVICE  Device,
    IN WDFREQUEST Request
)
{
    WDF_REQUEST_PARAMETERS params = { 0 };
    NTSTATUS status = STATUS_SUCCESS;
    PVOID    input_buf = NULL, output_buf = NULL;
    size_t   input_buf_size, output_buf_size;
    size_t  bytes_returned = 0;
    PNETUIO_CONTEXT_DATA  netuio_contextdata = NULL;

    netuio_contextdata = netuio_get_context_data(Device);

    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(Request, &params);

    // We only need to be in the context of the process that initiated the request
    //when we need to map memory to userspace. Otherwise, send the request back to framework.
    if (!((params.Type == WdfRequestTypeDeviceControl) &&
        (params.Parameters.DeviceIoControl.IoControlCode == IOCTL_NETUIO_MAP_HW_INTO_USERMODE)))
    {
        status = WdfDeviceEnqueueRequest(Device, Request);

        if (!NT_SUCCESS(status))
        {
            WdfRequestCompleteWithInformation(Request, status, bytes_returned);
        }
        return;
    }

    // First retrieve the input buffer and see if it matches our device
    status = WdfRequestRetrieveInputBuffer(Request, sizeof(struct dpdk_private_info), &input_buf, &input_buf_size);
    if (!NT_SUCCESS(status)) {
        status = STATUS_INVALID_BUFFER_SIZE;
        goto end;
    }

    struct dpdk_private_info* dpdk_pvt_info = (struct dpdk_private_info*)input_buf;
    // Ensure that the B:D:F match - otherwise, fail the IOCTL
    if ((netuio_contextdata->addr.bus_num != dpdk_pvt_info->dev_addr.bus_num) ||
        (netuio_contextdata->addr.dev_num != dpdk_pvt_info->dev_addr.dev_num) ||
        (netuio_contextdata->addr.func_num != dpdk_pvt_info->dev_addr.func_num)) {
        status = STATUS_NOT_SAME_DEVICE;
        goto end;
    }

    if (netuio_contextdata->dpdk_seg.mem.user_mapped_virt_addr != NULL) {
        status = STATUS_ALREADY_COMMITTED;
        goto end;
    }

    // Return relevant data to the caller
    status = WdfRequestRetrieveOutputBuffer(Request, sizeof(struct dpdk_private_info), &output_buf, &output_buf_size);
    if (!NT_SUCCESS(status)) {
        status = STATUS_INVALID_BUFFER_SIZE;
        goto end;
    }

    // Zero out the physically contiguous block
    RtlZeroMemory(netuio_contextdata->dpdk_seg.mem.virt_addr, netuio_contextdata->dpdk_seg.mem.size);

    status = netuio_map_address_into_user_process(netuio_contextdata);
    if (status != STATUS_SUCCESS) {
        goto end;
    }

    netuio_handle_get_hw_data_request(Request, netuio_contextdata, output_buf, output_buf_size);
    bytes_returned = output_buf_size;

end:
    WdfRequestCompleteWithInformation(Request, status, bytes_returned);

    return;
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

    if (IoControlCode != IOCTL_NETUIO_PCI_CONFIG_IO)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    // First retrieve the input buffer and see if it matches our device
    status = WdfRequestRetrieveInputBuffer(Request, sizeof(struct dpdk_pci_config_io), &input_buf, &input_buf_size);
    if (!NT_SUCCESS(status)) {
        status = STATUS_INVALID_BUFFER_SIZE;
        goto end;
    }

    struct dpdk_pci_config_io *dpdk_pci_io_input = (struct dpdk_pci_config_io *)input_buf;

    if (dpdk_pci_io_input->access_size != 1 &&
        dpdk_pci_io_input->access_size != 2 &&
        dpdk_pci_io_input->access_size != 4 &&
        dpdk_pci_io_input->access_size != 8) {
        status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    // Ensure that the B:D:F match - otherwise, fail the IOCTL
    if ((netuio_contextdata->addr.bus_num != dpdk_pci_io_input->dev_addr.bus_num) ||
        (netuio_contextdata->addr.dev_num != dpdk_pci_io_input->dev_addr.dev_num) ||
        (netuio_contextdata->addr.func_num != dpdk_pci_io_input->dev_addr.func_num)) {
        status = STATUS_NOT_SAME_DEVICE;
        goto end;
    }
    // Retrieve output buffer
    status = WdfRequestRetrieveOutputBuffer(Request, sizeof(UINT64), &output_buf, &output_buf_size);
    if (!NT_SUCCESS(status)) {
        status = STATUS_INVALID_BUFFER_SIZE;
        goto end;
    }
    ASSERT(output_buf_size == OutputBufferLength);

    if (dpdk_pci_io_input->op == PCI_IO_READ) {
        *(UINT64 *)output_buf = 0;
        bytes_returned = netuio_contextdata->bus_interface.GetBusData(
            netuio_contextdata->bus_interface.Context,
            PCI_WHICHSPACE_CONFIG,
            output_buf,
            dpdk_pci_io_input->offset,
            dpdk_pci_io_input->access_size);
    }
    else if (dpdk_pci_io_input->op == PCI_IO_WRITE) {
        // returns bytes written
        bytes_returned = netuio_contextdata->bus_interface.SetBusData(
            netuio_contextdata->bus_interface.Context,
            PCI_WHICHSPACE_CONFIG,
            (PVOID)&dpdk_pci_io_input->data,
            dpdk_pci_io_input->offset,
            dpdk_pci_io_input->access_size);
    }
    else {
        status = STATUS_INVALID_PARAMETER;
        goto end;
    }

end:
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

