/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2017 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <unistd.h>
#include <sys/mman.h>
#include <tchar.h>
#include <fcntl.h>

#include <SetupAPI.h>
#include <devguid.h>
#include <winioctl.h>

#include <rte_string_fns.h>
#include <rte_eal_memconfig.h>

#include "private.h"

#include "pci_private.h"

#define  MAX_DEVICENAME_SZ     255
#define  NETUIO_DEVICENAME_SZ   20

#define  MAX_STR_TOKENS   8
#define  DEC             10
#define  HEX             16

extern struct rte_pci_bus rte_pci_bus;

/* Local functions */
static int send_ioctl(HANDLE f, DWORD ioctl, LPVOID input_buf, DWORD input_buf_size, LPVOID output_buf, DWORD output_buf_size);
static int pci_config_io(const struct rte_pci_device *dev, void *buf, size_t len, off_t offset, enum pci_io operation);


 /**
 * This code is used to simulate a PCI probe by parsing information in
 * the registry hive for PCI devices. When a device is found, it will
 * attempt to open the UIO driver and send it an IOCTL if it is loaded
 * on the device. The UIO driver is a very minimal kernel driver for
 * networking devices, providing access to PCI BAR to applications.
 */

/* The functions below are not implemented on Windows,
 * but need to be defined for compilation purposes
 */

/* unbind kernel driver for this device */
int
pci_unbind_kernel_driver(struct rte_pci_device *dev __rte_unused)
{
	RTE_LOG(ERR, EAL, "RTE_PCI_DRV_FORCE_UNBIND flag is not implemented presently in Windows\n");
	return -ENOTSUP;
}

/* Map pci device */
int
rte_pci_map_device(struct rte_pci_device *dev)
{
	/* This function is not implemented on Windows.
	 * We really should short-circuit the call to these functions by clearing the
	 * RTE_PCI_DRV_NEED_MAPPING flag in the rte_pci_driver flags.
	 */
	return 0;
}

/* Unmap pci device */
void
rte_pci_unmap_device(struct rte_pci_device *dev)
{
	/* This function is not implemented on Windows.
	 * We really should short-circuit the call to these functions by clearing the
	 * RTE_PCI_DRV_NEED_MAPPING flag in the rte_pci_driver flags.
	 */
	return;
}

void
pci_uio_free_resource(struct rte_pci_device *dev,
		struct mapped_pci_resource *uio_res)
{
	/* This function is not implemented on Windows.
	 * We really should short-circuit the call to these functions by clearing the
	 * RTE_PCI_DRV_NEED_MAPPING flag in the rte_pci_driver flags.
	 */
	return;
}

int
pci_uio_alloc_resource(struct rte_pci_device *dev,
		struct mapped_pci_resource **uio_res)
{
	/* This function is not implemented on Windows.
	 * We really should short-circuit the call to these functions by clearing the
	 * RTE_PCI_DRV_NEED_MAPPING flag in the rte_pci_driver flags.
	 */
	return 0;
}

int
pci_uio_map_resource_by_index(struct rte_pci_device *dev, int res_idx,
		struct mapped_pci_resource *uio_res, int map_idx)
{
	/* This function is not implemented on Windows.
	 * We really should short-circuit the call to these functions by clearing the
	 * RTE_PCI_DRV_NEED_MAPPING flag in the rte_pci_driver flags.
	 */
	return 0;
}

int
pci_update_device(const struct rte_pci_addr *addr)
{
	/* This function is not implemented on Windows.
	 * We really should short-circuit the call to these functions by clearing the
	 * RTE_PCI_DRV_NEED_MAPPING flag in the rte_pci_driver flags.
	 */
	return 0;
}

/* Get iommu class of PCI devices on the bus. */
enum rte_iova_mode
rte_pci_get_iommu_class(void)
{
	/* Supports only RTE_KDRV_NIC_UIO */
	return RTE_IOVA_PA;
}

/* Read PCI config space. */
int rte_pci_read_config(const struct rte_pci_device *dev,
			void *buf, size_t len, off_t offset)
{
	return pci_config_io(dev, buf, len, offset, PCI_IO_READ);
}

/* Write PCI config space. */
int rte_pci_write_config(const struct rte_pci_device *dev,
			 const void *buf, size_t len, off_t offset)
{
	return pci_config_io(dev, (void *)buf, len, offset, PCI_IO_WRITE);
}


static
int send_ioctl(HANDLE f, DWORD ioctl,
	       LPVOID input_buf, DWORD input_buf_size,
	       LPVOID output_buf, DWORD output_buf_size)
{
	BOOL bResult;
	DWORD results_sz = 0;
	DWORD dwError = ERROR_SUCCESS;

	bResult = DeviceIoControl(f, ioctl, input_buf, input_buf_size,
				  output_buf, output_buf_size,
				  &results_sz, NULL);

	if (!bResult) {
		dwError = GetLastError();
		if (dwError != ERROR_NOT_SAME_DEVICE)
			RTE_LOG(ERR, EAL, "IOCTL query failed with reported error %0ld\n", dwError);

		return -1;
	}

	return ERROR_SUCCESS;
}

/* Send IOCTL to driver to read/write PCI configuration space */
static
int pci_config_io(const struct rte_pci_device *dev, void *buf,
		  size_t len, off_t offset, enum pci_io operation)
{
	int ret = -1;
	/* Open the driver */
	TCHAR   device_name[MAX_DEVICENAME_SZ];

	/* Construct the driver name using B:D:F */
	_stprintf_s(device_name, NETUIO_DEVICENAME_SZ, _T("\\\\.\\%s_%04d%02d%02d"),
		    NETUIO_DRIVER_NAME, dev->addr.bus, dev->addr.devid, dev->addr.function);

	HANDLE f = CreateFile(device_name, GENERIC_READ | GENERIC_WRITE,
					    FILE_SHARE_READ | FILE_SHARE_WRITE,
					    NULL, OPEN_EXISTING, 0, NULL);

	if (f == INVALID_HANDLE_VALUE) {
		goto error;
	}

	/* Send IOCTL */
	struct dpdk_pci_config_io pci_io = { 0 };
	pci_io.dev_addr.bus_num = dev->addr.bus;
	pci_io.dev_addr.dev_num = dev->addr.devid;
	pci_io.dev_addr.func_num = dev->addr.function;
	pci_io.offset = offset;
	pci_io.op = operation;

	if (operation == PCI_IO_WRITE)
		pci_io.buf = buf;

	uint32_t  outputbuf = 0;
	if (send_ioctl(f, IOCTL_NETUIO_PCI_CONFIG_IO, &pci_io, sizeof(pci_io),
				&outputbuf, sizeof(outputbuf)) != ERROR_SUCCESS)
		goto error;

	if (operation == PCI_IO_READ)
		memcpy(buf, &outputbuf, sizeof(outputbuf));

	ret = 0;
error:
	if (f != INVALID_HANDLE_VALUE)
		CloseHandle(f);

	return ret;
}

/* Find first free slot and store memory segment information in global configuration */
static
ULONG store_memseg_info(struct dpdk_private_info *pvt_info)
{
	unsigned ms_cnt;

	/* get pointer to global configuration */
	struct rte_mem_config *mcfg = rte_eal_get_configuration()->mem_config;
	if (mcfg == NULL)
		return ERROR_BAD_CONFIGURATION;

	for (ms_cnt = 0; ms_cnt < RTE_MAX_MEMSEG; ms_cnt++) {
		/* Find first available slot */
		struct rte_memseg *ms = &mcfg->memseg[ms_cnt];
		if ((ms->addr == NULL) && (ms->len == 0)) {
			ms->phys_addr = (phys_addr_t)(uintptr_t)pvt_info->ms.phys_addr.QuadPart;
			ms->addr = pvt_info->ms.user_mapped_virt_addr;
			ms->hugepage_sz = RTE_PGSIZE_4K;
			ms->len = pvt_info->ms.size;
			ms->socket_id = pvt_info->dev_numa_node;
			/* Clear the memory segment */
			memset(ms->addr, 0, ms->len);

			return ERROR_SUCCESS;
		}
	}

	return ERROR_OUTOFMEMORY;
}

static
int get_device_resource_info(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDeviceInfoData, struct rte_pci_device* dev)
{
	int  ret = -1;

	/* Open the driver */
	TCHAR   device_name[MAX_DEVICENAME_SZ];
	HANDLE  f = INVALID_HANDLE_VALUE;
	PSP_DEVICE_INTERFACE_DETAIL_DATA dev_interface_detail = NULL;
	DWORD   dwError = 0;

	/* Obtain the netUIO driver interface (if available) for this device */
	DWORD   required_size = 0;
	TCHAR   dev_instance_id[MAX_DEVICENAME_SZ];

	/* Retrieve the device instance ID */
	SetupDiGetDeviceInstanceId(hDevInfo, pDeviceInfoData,
				   dev_instance_id, sizeof(dev_instance_id), &required_size);
	//_tprintf(_T("Device_instance_id=%s\n"), device_instance_id);

	/* Obtain the device information set */
	HDEVINFO hd = SetupDiGetClassDevs(&GUID_DEVINTERFACE_netUIO, dev_instance_id,
					  NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hd == INVALID_HANDLE_VALUE)
		goto end;

	SP_DEVICE_INTERFACE_DATA  dev_interface_data = { 0 };
	dev_interface_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	/* Enumerate the netUIO interfaces for this device information set */
	/* If it fails, it implies that the netUIO interface is not available for this device */
	if (!SetupDiEnumDeviceInterfaces(hd, 0, &GUID_DEVINTERFACE_netUIO, 0, &dev_interface_data)) {
		dwError = GetLastError();
		goto end;
	}

	required_size = 0;
	if (!SetupDiGetDeviceInterfaceDetail(hd, &dev_interface_data, NULL, 0, &required_size, NULL)) {
		dwError = GetLastError();
		if (dwError != ERROR_INSUFFICIENT_BUFFER)  /* ERROR_INSUFFICIENT_BUFFER is expected */
			goto end;
	}

	dev_interface_detail = malloc(required_size);
	dev_interface_detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

	if (!SetupDiGetDeviceInterfaceDetail(hd, &dev_interface_data,
					     dev_interface_detail, required_size, NULL, NULL)) {
		dwError = GetLastError();
		goto end;
	}

	f = CreateFile(dev_interface_detail->DevicePath,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, NULL);

	if (f == INVALID_HANDLE_VALUE) {
		//RTE_LOG(DEBUG, EAL, "Unable to open driver file \"%s\".\n", device_name);
		goto end;
	}

	/* Send IOCTL */
	struct dpdk_private_info input_pvt_info = { 0 };
	input_pvt_info.dev_id = dev->id.device_id;
	input_pvt_info.dev_addr.bus_num = dev->addr.bus;
	input_pvt_info.dev_addr.dev_num = dev->addr.devid;
	input_pvt_info.dev_addr.func_num = dev->addr.function;

	struct dpdk_private_info output_pvt_info = { 0 };
	if (send_ioctl(f, IOCTL_NETUIO_GET_HW_DATA, &input_pvt_info, sizeof(input_pvt_info),
						&output_pvt_info, sizeof(output_pvt_info)) != ERROR_SUCCESS)
		goto end;

	/* Set relevant values into the dev structure (use only bar 0 for hw) */
	dev->device.numa_node = output_pvt_info.dev_numa_node;
	dev->mem_resource[0].phys_addr = output_pvt_info.hw.phys_addr.QuadPart;
	dev->mem_resource[0].addr = output_pvt_info.hw.user_mapped_virt_addr;
	dev->mem_resource[0].len = output_pvt_info.hw.size;

	/* store memory segment information in global configuration */
	if (store_memseg_info(&output_pvt_info) != ERROR_SUCCESS)
		goto end;

	ret = ERROR_SUCCESS;
end:
	if (f != INVALID_HANDLE_VALUE)
		CloseHandle(f);

	if (dev_interface_detail)
		free(dev_interface_detail);

	if (hd != INVALID_HANDLE_VALUE)
		SetupDiDestroyDeviceInfoList(hd);

	return ret;
}

static
unsigned int get_numerical_value(char *str, int base)
{
	unsigned int num = 0;
	uint8_t i = 0;

	while (str[i]) {
		if (base == 10) {
			if (!IsCharAlpha(str[i]) && str[i] != ' ') {
			    num = strtoul(&str[i], NULL, base);
			    break;
			}
		}
		if (base == 16) {
			if (isxdigit(str[i]) && str[i] != ' ') {
			    num = strtoul(&str[i], NULL, base);
			    break;
			}
		}

		++i;
	}

	return num;
}

/*
* split up a pci address into its constituent parts.
*/
static int
parse_pci_addr_format(const char *buf, int bufsize,
		      uint16_t *domain, uint8_t *bus, uint8_t *device, uint8_t *function)
{
	int ret = -1;
	int i = 0;

	char *str[MAX_STR_TOKENS] = { 0 };

	char *buf_copy = _strdup(buf);
	if (buf_copy == NULL)
		goto end;

	/* PCI Addr format string is delimited by ',' */
	/* eg: "PCI bus 5, device 35, function 0" */
	if (rte_strsplit(buf_copy, bufsize, str, MAX_STR_TOKENS, ',') > MAX_STR_TOKENS - 1)
		goto end;

	/* We now have the bus, device and function values in tokens in the str[] array */
	/* now convert to numerical values */
	*domain = 0;
	*bus = (uint8_t)get_numerical_value(str[0], DEC);
	*device = (uint8_t)get_numerical_value(str[1], DEC);
	*function = (uint8_t)get_numerical_value(str[2], DEC);

	ret = 0;
end:
	/* free the copy made (if any) with _tcsdup */
	if (buf_copy)
		free(buf_copy);

	return ret;
}

static int
parse_hardware_ids(char *str, unsigned strlength, uint16_t *val1, uint16_t *val2)
{
	int ret = -1;
	char *strID[MAX_STR_TOKENS] = { 0 };

	if (rte_strsplit(str, strlength, strID, MAX_STR_TOKENS, '_') > MAX_STR_TOKENS - 1)
		goto end;

	/* check if the string is a combined ID value (eg: subdeviceID+subvendorID */
	if (strlen(strID[1]) == 8) {
		char strval1[8];
		char strval2[8];
		memset(strval1, 0, sizeof(strval1));
		memset(strval2, 0, sizeof(strval2));

		memcpy_s(strval1, sizeof(strval1), strID[1], 4);
		memcpy_s(strval2, sizeof(strval2), strID[1]+4, 4);

		if (val1)
			*val1 = (uint16_t)get_numerical_value(strval1, HEX);
		if (val2)
			*val2 = (uint16_t)get_numerical_value(strval2, HEX);
	}
	else {
		if (val1)
			*val1 = (uint16_t)get_numerical_value(strID[1], HEX);
	}

	ret = 0;
end:
	return ret;
}

/*
* split up hardware ID into its constituent parts.
*/
static int
parse_pci_hardware_id_format(const char *buf, int bufsize,
			     uint16_t *vendorID, uint16_t *deviceID, uint16_t *subvendorID, uint16_t *subdeviceID)
{
	int ret = -1;
	int i = 0;

	char *str[MAX_STR_TOKENS] = { 0 };

	char *buf_copy = _strdup(buf);
	if (buf_copy == NULL)
		goto end;

	/* PCI Hardware ID format string is first delimited by '\\' */
	/* eg: "PCI\VEN_8086&DEV_153A&SUBSYS_00008086&REV_04" */
	if (rte_strsplit(buf_copy, bufsize, str, MAX_STR_TOKENS, '\\') > MAX_STR_TOKENS - 1)
		goto end;

	char *buf_copy_stripped = _strdup(str[1]);
	/* The remaining string is delimited by '&' */
	//memcpy_s(buf_copy, bufsize, str[1], strlen(str[1]));
	if (rte_strsplit(buf_copy_stripped, bufsize, str, MAX_STR_TOKENS, '&') > MAX_STR_TOKENS - 1)
		goto end;

	/* We now have the various hw IDs in tokens in the str[] array */
	/* However, we need to isolate the numerical IDs - use '_' as the delimiter */
	if (parse_hardware_ids(str[0], strlen(str[0]), vendorID, NULL))
		goto end;

	if (parse_hardware_ids(str[1], strlen(str[1]), deviceID, NULL))
		goto end;

	if (parse_hardware_ids(str[2], strlen(str[2]), subdeviceID, subvendorID))
		goto end;

	ret = 0;
end:
	/* free the copy made (if any) with _tcsdup */
	if (buf_copy)
		free(buf_copy);
	if (buf_copy_stripped)
		free(buf_copy_stripped);
	return ret;
}

static int
pci_scan_one(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDeviceInfoData)
{
	struct rte_pci_device *dev;
	unsigned i, max;
	int    ret = -1;

	dev = malloc(sizeof(struct rte_pci_device));
	if (dev == NULL) {
		ret = -1;
		goto end;
	}

	memset(dev, 0, sizeof(*dev));

	DWORD dwType = 0, dwSize = 0;
	char  strPCIDeviceInfo[PATH_MAX];
	BOOL  bResult;

	bResult = SetupDiGetDeviceRegistryPropertyA(hDevInfo, pDeviceInfoData, SPDRP_LOCATION_INFORMATION,
						    NULL, (BYTE *)&strPCIDeviceInfo, sizeof(strPCIDeviceInfo), NULL);

	/* Some devices don't have location information - in such a case, simply return 0 */
	/* Also, if we don't find 'PCI' in the description, we'll skip it */
	if (!bResult) {
		ret = (GetLastError() == ERROR_INVALID_DATA) ? ERROR_CONTINUE : -2;
		goto end;
	}
	else if (!strstr(strPCIDeviceInfo, "PCI")) {
		ret = ERROR_CONTINUE;
		goto end;
	}

	uint16_t domain;
	uint8_t bus, device, function;

	if (parse_pci_addr_format((const char *)&strPCIDeviceInfo, sizeof(strPCIDeviceInfo), &domain, &bus, &device, &function) != 0) {
		ret = -3;
		goto end;
	}

	dev->addr.domain = domain;
	dev->addr.bus = bus;
	dev->addr.devid = device;
	dev->addr.function = function;

	/* Retrieve PCI device IDs */
	bResult = SetupDiGetDeviceRegistryPropertyA(hDevInfo, pDeviceInfoData, SPDRP_HARDWAREID,
						    NULL, (BYTE *)&strPCIDeviceInfo, sizeof(strPCIDeviceInfo), NULL);

	/* parse hardware ID string */
	uint16_t vendorID, deviceID, subvendorID, subdeviceID = 0;
	if (parse_pci_hardware_id_format((const char *)&strPCIDeviceInfo, sizeof(strPCIDeviceInfo), &vendorID, &deviceID, &subvendorID, &subdeviceID) != 0) {
		ret = -4;
		goto end;
	}

	dev->id.vendor_id = vendorID;
	dev->id.device_id = deviceID;
	dev->id.subsystem_vendor_id = subvendorID;
	dev->id.subsystem_device_id = subdeviceID;

	dev->max_vfs = 0; /* TODO: get max_vfs */

	dev->device.numa_node = 0; // This will be fixed later upon the call to get_device_resource_info()

	pci_name_set(dev);

	dev->kdrv = RTE_KDRV_NIC_UIO;

	/* parse resources and retrive the BAR address using IOCTL */
	if (get_device_resource_info(hDevInfo, pDeviceInfoData, dev) != ERROR_SUCCESS) {
		ret = ERROR_CONTINUE; // We won't add this device, but we want to continue looking for supported devices
		goto end;
	}

	/* device is valid, add in list (sorted) */
	if (TAILQ_EMPTY(&rte_pci_bus.device_list)) {
		rte_pci_add_device(dev);
	}
	else {
		struct rte_pci_device *dev2 = NULL;
		int ret;

		TAILQ_FOREACH(dev2, &rte_pci_bus.device_list, next) {
			ret = rte_eal_compare_pci_addr(&dev->addr, &dev2->addr);
			if (ret > 0)
				continue;
			else if (ret < 0) {
				rte_pci_insert_device(dev2, dev);
			}
			else { /* already registered */
				dev2->kdrv = dev->kdrv;
				dev2->max_vfs = dev->max_vfs;
				memmove(dev2->mem_resource, dev->mem_resource,
						    sizeof(dev->mem_resource));
				free(dev);
			}
			return 0;
		}
		rte_pci_add_device(dev);
	}

	ret = 0;
	return ret;
end:
	if (dev)
		free(dev);
	return ret;
}

/* Scan the contents of the PCI bus, and add all network class devices into the devices list. */
int
rte_pci_scan(void)
{
	int fd;
	DWORD		    DeviceIndex = 0, FoundDevice = 0;
	HDEVINFO	    hDevInfo = NULL;
	SP_DEVINFO_DATA	    DeviceInfoData = { 0 };
	int		    ret = -1;

	hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_NET, NULL, NULL, DIGCF_PRESENT);
	if (INVALID_HANDLE_VALUE == hDevInfo) {
		RTE_LOG(ERR, EAL, "Unable to enumerate PCI devices.\n", __func__);
		goto end;
	}

	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	DeviceIndex = 0;

	while (SetupDiEnumDeviceInfo(hDevInfo, DeviceIndex, &DeviceInfoData)) {
		DeviceIndex++;
		ret = pci_scan_one(hDevInfo, &DeviceInfoData);
		if (ret == ERROR_SUCCESS)
		    FoundDevice++;
		else if (ret != ERROR_CONTINUE)
		    goto end;

		memset(&DeviceInfoData, 0, sizeof(SP_DEVINFO_DATA));
		DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	}

	RTE_LOG(ERR, EAL, "PCI scan found %u devices\n", FoundDevice);
	ret = (FoundDevice != 0) ? 0 : -1;
end:
	if (INVALID_HANDLE_VALUE != hDevInfo)
		SetupDiDestroyDeviceInfoList(hDevInfo);

	return ret;
}

