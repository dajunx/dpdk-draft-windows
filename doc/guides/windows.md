Windows DPDK Development

Revision 1.1

Revision 1.0	Nov 6, 2017	Initial Revision
Revision 1.1	May 3, 2018	Update of the source to DPDK release 18.02;

1.	Windows DPDK Software package

The Windows DPDK Software package contains the Windows port of essential DPDK libraries and example applications. The ported DPDK libraries and applications have been built and verified on Windows Server 2016. It has been confirmed as being as performant as the Linux version of the stack for the same applications. The Windows DPDK application stack requires a kernel mode driver to provide the user-space application with direct access to the networking hardware. This driver is also used to allocate physically contiguous memory and map the same to the user-mode application process. This document contains information on how these libraries, example applications and the kernel mode driver can be built and used.


2.	Windows DPDK support
This port of the DPDK libraries and sample applications are presently only supported on:
•	Intel® XL710 series of network adapters (40GbE class adapters)
•	Windows Server 2019 
•	Windows Server 2016 (hence, 64-bit only)

Poll Mode Driver support is expected shortly for these NICs:
•	Cavium 
•	Broadcom
•	Chelsio

This following libraries and example applications have been ported and confirmed to be working with expected performance on Windows: (Note: this is not the complete list of all DPDK libraries)
•	librte_acl
•	librte_bitratestats
•	librte_bus_pci
•	librte_cfgfile
•	librte_cmdline
•	librte_cryptodev
•	librte_distributor
•	librte_eal
•	librte_efd
•	librte_ether
•	librte_eventdev
•	librte_flowclassify
•	librte_gro
•	librte_gso
•	librte_hash
•	librte_ipfrag
•	librte_kvargs
•	librte_latencystats
•	librte_lpm
•	librte_mbuf
•	librte_member
•	librte_mempool
•	librte_mempool_ring
•	librte_meter
•	librte_metrics
•	librte_net
•	librte_pci
•	librte_pipeline
•	librte_port
•	librte_reorder
•	librte_ring
•	librte_sched
•	librte_security
•	librte_table
•	librte_timer
•	librte_pmd_i40e - the Intel i40e poll-mode driver library interface
•	l2fwd - example application
•	l3fwd - example application

Note: librte_eal will include code that is specific to Windows, similar to the way the library is built on other operating systems (like Linux and FreeBSD)


3.	Build Tools/Environment
The following tools are required to build the Windows DPDK stack:
•	Visual Studio 2017 (or 2015) Professional Edition 
•	Intel C/C++ compiler version 18. (The Intel C/C++ compiler is part of the Intel Parallel Studio XE 2018 for Windows suite). The Intel Parallel Studio XE 2018 suite can be downloaded from: https://software.intel.com/en-us/parallel-studio-xe
•	Windows Software Development Kit (SDK) version 10.0.15063
•	Windows Driver Kit (WDK) version 10.0.15063 (required to build the UIO driver)
To integrate the Intel C/C++ compiler version 18 into Visual Studio 2017 (or 2015), the latter must be installed before the Intel Parallel Studio XE 2018 software is installed. 
 
4.	Building the DPDK source
The Windows port of DPDK libraries and applications have now been updated to the latest public release v18.02, of the main DPDK source.
To build the DPDK libraries and applications for Windows, do the following:
1.	Ensure that the build tools and development environment is set up as described above in Section 3.
2.	Clone the Windows port of the DPDK source from dpdk.org by executing the following Git commands:
  i.	git clone http://dpdk.org/git/draft/dpdk-draft-windows
  ii.	git checkout windpdk-v18.02
3.	This should create a Git repo on the local system with code based on the v18.02 version of the main DPDK source.
4.	Load the Visual Studio solution (which contains the project files for all essential libraries and applications) from: mk\exec-env\windows\dpdk.sln
5.	Build the entire solution.
6.	The resultant output files can be found in the folders under: x64\Release 


5.	UIO Driver
The Windows DPDK stack requires a kernel mode driver to provide the user-space application with direct access to the hardware. This driver (called netuio.sys) is also used to allocate physically contiguous memory and map it to the user (application) process.
The driver source for netuio is included within the DPDK source tree.
The source code can be found in: -
 lib\librte_eal\windows\netuio
To build the netuio driver, load the Visual Studio solution file from:-
 mk\exec-env\windows\netuio\netuio.sln

Important: The UIO driver needs to be loaded in place of the existing built-in (inbox) driver for the Intel XL710 adapter. It can be loaded on both ports of the dual port adapter, if required. Appropriate measures have to be taken to replace a signed driver with a test-signed driver. Refer to this MSDN page for more information: How to Test-Sign a Driver Package.
The resultant output files from the driver build can be found in the x64\Release\netuio folder. The driver package requires the WdfCoinstaller01011.dll co-installer for installation. This file can be found inside the Windows WDK.

Note: The Intel C/C++ compiler version 18 is not required to build the netuio driver. 
6.	Test setup
A simple test setup consists of the following components:-
•	Two Intel® Xeon™ 2699 v4 (Broadwell-class) based servers
•	Install two Intel XL710 network adapters in each system.
•	One of the systems (system under test) should have Windows Server 2106 installed.
•	The other system can have any recent release of Fedora* installed. This is required because this system will be used to run the pktgen application which is a packet generator. This application presently only runs on Linux using DPDK 17.08. It can be downloaded from: http://dpdk.org/browse/apps/pktgen-dpdk/refs/ 
•	For optimal DPDK performance, set up the connections between the network devices as described here: http://dpdk.org/doc/guides-16.04/linux_gsg/nic_perf_intel_platform.html
•	Build and deploy the Windows port of the DPDK libraries and applications to the system under test running Windows. Ensure the UIO driver is installed for the network device(s).
•	Start the pktgen application on the packet generator system. Full instructions to setup pktgen and run it using its various command-line options can be found at: http://pktgen-dpdk.readthedocs.io/en/latest/getting_started.html
•	Run the test applications (see Section 7)


7.	Applications
The Windows port of the DPDK source can be used to compile and build the following two example applications:
1.	L2Fwd: A simple forwarding application that swaps sender and receiver MAC addresses in the packet. For instructions on how to run the L2Fwd application, see:
http://dpdk.org/doc/guides/sample_app_ug/l2_forward_real_virtual.html

2.	L3Fwd: A forwarding application that can route packets based on certain rules. For instructions on how to run the L3Fwd application, see:
http://dpdk.org/doc/guides/sample_app_ug/l3_forward.html

 
References
All things DPDK:
http://dpdk.org/

DPDK Programmer’s Guide:
http://www.dpdk.org/doc/guides/prog_guide/index.html

DPDK API Documentation:
http://www.dpdk.org/doc/api/