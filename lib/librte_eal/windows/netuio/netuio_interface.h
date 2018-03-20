/*-
*
*   Copyright(c) 2017 Intel Corporation. All rights reserved.
*
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
#define IOCTL_NETUIO_GET_HW_DATA      CTL_CODE(FILE_DEVICE_NETWORK, 51, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_NETUIO_PCI_CONFIG_IO    CTL_CODE(FILE_DEVICE_NETWORK, 52, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

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

struct dpdk_private_info
{
    struct mem_region   hw;
    struct mem_region   ms;
    struct dev_addr     dev_addr;
    struct mem_region	bar1;
//  struct mem_region	bar2;
    UINT16              dev_id;
    UINT16              sub_dev_id;
    USHORT              dev_numa_node;
    USHORT              reserved;
};

struct dpdk_pci_config_io
{
    struct dev_addr     dev_addr;
    PVOID               buf;
    UINT32              offset;
    enum pci_io         op;
};

#pragma pack(pop)

#endif // NETUIO_INTERFACE_H
