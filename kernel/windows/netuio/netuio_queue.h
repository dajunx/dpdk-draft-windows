/*-
*
*   Copyright(c) 2017 Intel Corporation. All rights reserved.
*
*/


#ifndef NETUIO_QUEUE_H
#define NETUIO_QUEUE_H

EXTERN_C_START

// This is the context that can be placed per queue and would contain per queue information.
typedef struct _QUEUE_CONTEXT {
    ULONG PrivateDeviceData;  // just a placeholder
} QUEUE_CONTEXT, *PQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, QueueGetContext)

VOID
netuio_unmap_address_from_user_process(_In_ PNETUIO_CONTEXT_DATA netuio_contextdata);

NTSTATUS
netuio_queue_initialize(_In_ WDFDEVICE hDevice);

// Events from the IoQueue object
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL netuio_evt_IO_device_control;
EVT_WDF_IO_QUEUE_IO_STOP netuio_evt_IO_stop;

EVT_WDF_IO_IN_CALLER_CONTEXT netuio_evt_IO_in_caller_context;

EXTERN_C_END

#endif // NETUIO_QUEUE_H
