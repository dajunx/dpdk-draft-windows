/*-
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


#ifndef NETUIO_DRV_H
#define NETUIO_DRV_H

#define INITGUID

#include <ntddk.h>
#include <wdf.h>

#include "netuio_dev.h"
#include "netuio_queue.h"

EXTERN_C_START

// Print output constants
#define DPFLTR_NETUIO_INFO_LEVEL   35

// WDFDRIVER Events
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD       netuio_evt_device_add;
EVT_WDF_OBJECT_CONTEXT_CLEANUP  netuio_evt_driver_context_cleanup;
EVT_WDF_DEVICE_PREPARE_HARDWARE netuio_evt_prepare_hw;
EVT_WDF_DEVICE_RELEASE_HARDWARE netuio_evt_release_hw;
EVT_WDF_FILE_CLOSE              netuio_evt_file_cleanup;

EXTERN_C_END

#endif // NETUIO_DRV_H
