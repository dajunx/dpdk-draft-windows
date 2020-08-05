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


#ifndef NETUIO_INTERFACE_H
#define NETUIO_INTERFACE_H

// All structures in this file are packed on an 8B boundary. 
#pragma pack(push)
#pragma pack(8)

// Define an Interface Guid so that any app can find the device and talk to it.
DEFINE_GUID (GUID_DEVINTERFACE_netUIO, 0x08336f60,0x0679,0x4c6c,0x85,0xd2,0xae,0x7c,0xed,0x65,0xff,0xf7); // {08336f60-0679-4c6c-85d2-ae7ced65fff7}

// Device name definitions
#define NETUIO_DEVICE_SYMBOLIC_LINK_ANSI    "\\DosDevices\\netuio"

// netUIO driver symbolic name (prefix)
#define NETUIO_DRIVER_NAME  _T("netuio")

// IOCTL code definitions
#define IOCTL_NETUIO_MAP_HW_INTO_USERMODE CTL_CODE(FILE_DEVICE_NETWORK, 51, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_NETUIO_PCI_CONFIG_IO        CTL_CODE(FILE_DEVICE_NETWORK, 52, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

struct mem_region {
    UINT64           size;       // memory region size
    PHYSICAL_ADDRESS phys_addr;  // physical address of the memory region
    PVOID            virt_addr;  // virtual address of the memory region
    PVOID            user_mapped_virt_addr;  // virtual address of the region mapped into user process context
};

struct dev_addr {
    ULONG   bus_num;
    USHORT  dev_num;
    USHORT  func_num;
};

enum pci_io {
    PCI_IO_READ = 0,
    PCI_IO_WRITE = 1
};

#define PCI_MAX_BAR 6

struct dpdk_private_info
{
    struct mem_region   hw[PCI_MAX_BAR];
    struct mem_region   ms;
    struct dev_addr     dev_addr;
    UINT16              dev_id;
    UINT16              sub_dev_id;
    USHORT              dev_numa_node;
    USHORT              reserved;
};

struct dpdk_pci_config_io
{
    struct dev_addr     dev_addr;
    UINT32              offset;
    enum pci_io         op;
    UINT32              access_size; // 1, 2, 4, or 8 bytes

    union dpdk_pci_config_io_data {
        UINT8			u8;
        UINT16			u16;
        UINT32			u32;
        UINT64			u64;
    } data;
};

#pragma pack(pop)

#endif // NETUIO_INTERFACE_H
