/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
*/


#pragma once

#ifndef _RTE_WINDOWS_H_
#define _RTE_WINDOWS_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _MSC_VER
#error
#endif

#ifndef _WINDOWS
#define _WINDOWS
#endif

// If we define WIN32_LEAN_AND_MEAN, winsock isn't included by default. We can then include it in specific header files as we need later.
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

// This isn't a complete replacement for typeof in GCC. For example, it doesn't work in cases where you have typeof(x) _val = 0. However, 
// it does allow us to remove some of the windows specific changes that need to be put into a lot of files to allow compilation. 
#define typeof(x)	_Generic((x),			\
	void *			: void *,		\
	char			: char,			\
	unsigned char		: unsigned char		\
	char *			: char *,		\
	unsigned char *		: unsigned char *,	\
	short			: short,		\
	unsigned short		: unsigned short,	\
	short *			: short *,		\
	unsigned short *	: unsigned short *,	\
	int			: int,			\
	int *			: int *,		\
	unsigned int		: unsigned int		\
	unsigned int *		: unsigned int *,	\
	long 			: long,			\
	long *			: long *,		\
	unsigned long		: unsigned long,	\
	unsigned long *		: unsigned long *,	\
	long long		: long long,		\
	unsigned long long	: unsigned long long,	\
	unsigned long long *	: unsigned long long *, \
	default			: void			\
)

/*
* Globally driven over-rides.
*/
#define __attribute__(x) 

#define __func__ __FUNCTION__

#define NORETURN __declspec(noreturn)
#define ATTR_UNUSED
#define __AVX__    1

#define E_RTE_NO_TAILQ	(-1)

/* Include this header here, so that we can re-define EAL_REGISTER_TAILQ */
#include <rte_tailq.h>
#ifdef EAL_REGISTER_TAILQ(t)
#undef EAL_REGISTER_TAILQ(t)
#endif

/*
* Definition for registering TAILQs
* (This is a workaround for Windows in lieu of a constructor-like function)
*/
#define EAL_REGISTER_TAILQ(t) \
void init_##t(void); \
void init_##t(void) \
{ \
	if (rte_eal_tailq_register(&t) < 0) \
		rte_panic("Cannot initialize tailq: %s\n", t.name); \
}

/* Include this header here, so that we can re-define RTE_REGISTER_BUS */
#include <rte_bus.h>
#ifdef RTE_REGISTER_BUS(nm, bus)
#undef RTE_REGISTER_BUS(nm, bus)
#endif

/*
* Definition for registering a bus
* (This is a workaround for Windows in lieu of a constructor-like function)
*/
#define RTE_REGISTER_BUS(nm, bus) \
void businitfn_ ##nm(void) \
{\
	(bus).name = RTE_STR(nm);\
	rte_bus_register(&bus); \
}


/* 
* Global warnings control. Disable this to see warnings in the 
* include/rte_override files 
*/
#define DPDKWIN_NO_WARNINGS

#ifdef DPDKWIN_NO_WARNINGS
#pragma warning (disable : 94)	/* warning #94: the size of an array must be greater than zero */
#pragma warning (disable : 169)	/* warning #169: expected a declaration */
#endif


/*
* These definitions are to force a specific version of the defined function.
* For Windows, we'll always stick with the latest defined version.
*/
#define rte_lpm_create			rte_lpm_create_v1604
#define rte_lpm_add			rte_lpm_add_v1604
#define rte_lpm6_add			rte_lpm6_add_v1705
#define rte_lpm6_lookup			rte_lpm6_lookup_v1705
#define rte_lpm6_lookup_bulk_func	rte_lpm6_lookup_bulk_func_v1705
#define rte_lpm6_is_rule_present	rte_lpm6_is_rule_present_v1705
#define rte_lpm_find_existing		rte_lpm_find_existing_v1604

#define rte_distributor_request_pkt	rte_distributor_request_pkt_v1705
#define rte_distributor_poll_pkt	rte_distributor_poll_pkt_v1705
#define rte_distributor_get_pkt		rte_distributor_get_pkt_v1705
#define rte_distributor_return_pkt	rte_distributor_return_pkt_v1705
#define rte_distributor_returned_pkts	rte_distributor_returned_pkts_v1705
#define rte_distributor_clear_returns	rte_distributor_clear_returns_v1705
#define rte_distributor_process		rte_distributor_process_v1705
#define rte_distributor_flush		rte_distributor_flush_v1705
#define rte_distributor_create		rte_distributor_create_v1705

/* 
* Definitions and overrides for ethernet.h 
*/
#define u_char uint8_t
#define u_short uint16_t

#define __packed

#define __BEGIN_DECLS
#define __END_DECLS

/*
* sys/_cdefs.h 
*/
#define __extension__

/*
* sys/_iovec.h
*/
#define ssize_t size_t
#define SSIZE_T_DECLARED
#define _SSIZE_T_DECLARED
#define _SIZE_T_DECLARED

/*
* Linux to BSD termios differences
*/
#define TCSANOW 0

/* Support X86 architecture */
#define RTE_ARCH_X86

/*
* We can safely remove __attribute__((__packed__)). We will replace it with all structures
* being packed
*/
#pragma pack(1)

#include <rte_wincompat.h>

/* Include rte_common.h first to get this out of the way controlled */
#include "./rte_common.h"


#include <sys/types.h>
/* rte_pci.h must be included before we define typeof() to be nothing */
//#include "./rte_pci.h"


#define __attribute__(x)

#define RTE_FORCE_INTRINSICS

#include "rte_config.h"

#define RTE_CACHE_ALIGN		__declspec(align(RTE_CACHE_LINE_SIZE))
#define RTE_CACHE_MIN_ALIGN	__declspec(align(RTE_CACHE_LINE_MIN_SIZE))

/* The windows port does not currently support dymamic loading of libraries, so fail these calls */
#define dlopen(lib, flag)   (0)
#define dlerror()           ("Not supported!")

/* Include time.h for struct timespec */
#include <time.h>
#ifndef _TIMESPEC_DEFINED
#define _TIMESPEC_DEFINED
#endif

typedef jmp_buf sigjmp_buf;
#define sigsetjmp(env, savemask) _setjmp((env))

/* function prototypes for those used exclusively by Windows */
void eal_create_cpu_map();

#define uint uint32_t

#define RTE_APP_TEST_RESOURCE_TAR 1

#if 0
/* rte_config.h defines all the libraries that we have to include. For all the libraries that are enabled,
generate a comment that will include it */
#ifdef _RTE_WIN_DPDK_APP

#ifdef RTE_LIBRTE_EAL
#pragma comment (lib, "librte_eal.lib")
#endif
#ifdef RTE_LIBRTE_PCI
#pragma comment (lib, "librte_pci.lib")
#endif
#ifdef RTE_LIBRTE_ETHDEV
#pragma comment (lib, "librte_ethdev.lib")
#endif
#ifdef RTE_LIBRTE_KVARGS
#pragma comment (lib, "librte_kvargs.lib")
#endif
#ifdef RTE_LIBRTE_IEEE1588
#pragma comment (lib, "librte_ieee1588.lib")
#endif
#ifdef RTE_LIBRTE_PCI_BUS
#pragma comment (lib, "librte_bus_pci.lib")
#endif
#ifdef RTE_LIBRTE_EM_PMD || RTE_LIBRTE_IGB_PMD
#pragma comment (lib, "librte_pmd_e1000.lib")
#endif
#ifdef RTE_LIBRTE_IXGBE_PMD
#pragma comment (lib, "librte_pmd_ixgbe.lib")
#endif
#ifdef RTE_LIBRTE_I40E_PMD
#pragma comment (lib, "librte_pmd_i40e.lib")
#endif
#ifdef RTE_LIBRTE_FM10K_PMD
#pragma comment (lib, "librte_pmd_fm10k.lib")
#endif
#ifdef RTE_LIBRTE_MLX4_PMD
#pragma comment (lib, "librte_pmd_mlx4.lib")
#endif
#ifdef RTE_LIBRTE_MLX5_PMD
#pragma comment (lib, "librte_pmd_mlx5.lib")
#endif
#ifdef RTE_LIBRTE_BNX2X_PMD
#pragma comment (lib, "librte_pmd_bnx2x.lib")
#endif
#ifdef RTE_LIBRTE_CXGBE_PMD
#pragma comment (lib, "librte_pmd_cxgbe.lib")
#endif
#ifdef RTE_LIBRTE_ENIC_PMD
#pragma comment (lib, "librte_pmd_enic.lib")
#endif
#ifdef RTE_LIBRTE_NFP_PMD
#pragma comment (lib, "librte_pmd_nfp.lib")
#endif
#ifdef RTE_LIBRTE_SFC_EFX_PMD
#pragma comment (lib, "librte_pmd_sfc_efx.lib")
#endif
#ifdef RTE_LIBRTE_PMD_SOFTNIC
//#pragma comment (lib, "librte_pmd_softnic.lib")
#endif
#ifdef RTE_LIBRTE_PMD_SZEDATA2
#pragma comment (lib, "librte_pmd_szedata2.lib")
#endif
#ifdef RTE_LIBRTE_THUNDERX_NICVF_PMD
#pragma comment (lib, "librte_pmd_thunderx_nicvf.lib")
#endif
#ifdef RTE_LIBRTE_LIO_PMD
#pragma comment (lib, "librte_pmd_lio.lib")
#endif
#ifdef RTE_LIBRTE_DPAA_BUS
#pragma comment (lib, "librte_dpaa_bus.lib")
#endif
#ifdef RTE_LIBRTE_DPAA_PMD
#pragma comment (lib, "librte_pmd_dpaa.lib")
#endif
#ifdef RTE_LIBRTE_OCTEONTX_PMD
#pragma comment (lib, "librte_pmd_octeontx.lib")
#endif
#ifdef RTE_LIBRTE_FSLMC_BUS
#pragma comment (lib, "librte_fslmc_bus.lib")
#endif
#ifdef RTE_LIBRTE_DPAA2_PMD
#pragma comment (lib, "librte_pmd_dpaa2.lib")
#endif
#ifdef RTE_LIBRTE_VIRTIO_PMD
#pragma comment (lib, "librte_pmd_virtio.lib")
#endif
#ifdef RTE_VIRTIO_USER
#pragma comment (lib, "librte_virtio_user.lib")
#endif
#ifdef RTE_LIBRTE_VMXNET3_PMD
#pragma comment (lib, "librte_pmd_vmxnet3.lib")
#endif
#ifdef RTE_LIBRTE_PMD_RING
//#pragma comment (lib, "librte_pmd_ring.lib")
#endif
#ifdef RTE_LIBRTE_PMD_BOND
#pragma comment (lib, "librte_pmd_bond.lib")
#endif
#ifdef RTE_LIBRTE_QEDE_PMD
#pragma comment (lib, "librte_pmd_qede.lib")
#endif
#ifdef RTE_LIBRTE_PMD_AF_PACKET
#pragma comment (lib, "librte_pmd_af_packet.lib")
#endif
#ifdef RTE_LIBRTE_ARK_PMD
#pragma comment (lib, "librte_pmd_ark.lib")
#endif
#ifdef RTE_LIBRTE_AVP_PMD
#pragma comment (lib, "librte_pmd_avp.lib")
#endif
#ifdef RTE_LIBRTE_PMD_TAP
#pragma comment (lib, "librte_pmd_tap.lib")
#endif
#ifdef RTE_LIBRTE_PMD_NULL
#pragma comment (lib, "librte_pmd_null.lib")
#endif
#ifdef RTE_LIBRTE_PMD_FAILSAFE
#pragma comment (lib, "librte_pmd_failsafe.lib")
#endif
#ifdef RTE_LIBRTE_CRYPTODEV
#pragma comment (lib, "librte_cryptodev.lib")
#endif
#ifdef RTE_LIBRTE_PMD_ARMV8_CRYPTO
#pragma comment (lib, "librte_pmd_armv8_crypto.lib")
#endif
#ifdef RTE_LIBRTE_PMD_DPAA2_SEC
#pragma comment (lib, "librte_pmd_dpaa2_sec.lib")
#endif
#ifdef RTE_LIBRTE_PMD_QAT
#pragma comment (lib, "librte_pmd_qat.lib")
#endif
#ifdef RTE_LIBRTE_PMD_AESNI_MB
#pragma comment (lib, "librte_pmd_aesni_mb.lib")
#endif
#ifdef RTE_LIBRTE_PMD_OPENSSL
#pragma comment (lib, "librte_pmd_openssl.lib")
#endif
#ifdef RTE_LIBRTE_PMD_AESNI_GCM
#pragma comment (lib, "librte_pmd_aesni_gcm.lib")
#endif
#ifdef RTE_LIBRTE_PMD_SNOW3G
#pragma comment (lib, "librte_pmd_snow3g.lib")
#endif
#ifdef RTE_LIBRTE_PMD_KASUMI
#pragma comment (lib, "librte_pmd_kasumi.lib")
#endif
#ifdef RTE_LIBRTE_PMD_ZUC
#pragma comment (lib, "librte_pmd_zuc.lib")
#endif
#ifdef RTE_LIBRTE_PMD_CRYPTO_SCHEDULER
#pragma comment (lib, "librte_pmd_crypto_scheduler.lib")
#endif
#ifdef RTE_LIBRTE_PMD_NULL_CRYPTO
#pragma comment (lib, "librte_pmd_null_crypto.lib")
#endif
#ifdef RTE_LIBRTE_PMD_MRVL_CRYPTO
#pragma comment (lib, "librte_pmd_mrvl_crypto.lib")
#endif
#ifdef RTE_LIBRTE_SECURITY
#pragma comment (lib, "librte_security.lib")
#endif
#ifdef RTE_LIBRTE_EVENTDEV
#pragma comment (lib, "librte_eventdev.lib")
#endif
#ifdef RTE_LIBRTE_PMD_SKELETON_EVENTDEV
//#pragma comment (lib, "librte_pmd_skeleton_eventdev.lib")
#endif
#ifdef RTE_LIBRTE_PMD_SW_EVENTDEV
//#pragma comment (lib, "librte_pmd_sw_eventdev.lib")
#endif
#ifdef RTE_LIBRTE_RING
#pragma comment (lib, "librte_ring.lib")
#endif
#ifdef RTE_LIBRTE_MEMPOOL
#pragma comment (lib, "librte_mempool.lib")
#pragma comment (lib, "librte_mempool_ring.lib")
#endif
#ifdef RTE_LIBRTE_MBUF
#pragma comment (lib, "librte_mbuf.lib")
#endif
#ifdef RTE_LIBRTE_TIMER
#pragma comment (lib, "librte_timer.lib")
#endif
#ifdef RTE_LIBRTE_CFGFILE
#pragma comment (lib, "librte_cfgfile.lib")
#endif
#ifdef RTE_LIBRTE_CMDLINE
#pragma comment (lib, "librte_cmdline.lib")
#endif
#ifdef RTE_LIBRTE_HASH
#pragma comment (lib, "librte_hash.lib")
#endif
#ifdef RTE_LIBRTE_EFD
#pragma comment (lib, "librte_efd.lib")
#endif
#ifdef RTE_LIBRTE_EFD
#pragma comment (lib, "librte_efd.lib")
#endif
#ifdef RTE_LIBRTE_MEMBER
#pragma comment (lib, "librte_member.lib")
#endif
#ifdef RTE_LIBRTE_JOBSTATS
//#pragma comment (lib, "librte_jobstats.lib")
#endif
#ifdef RTE_LIBRTE_METRICS
#pragma comment (lib, "librte_metrics.lib")
#endif
#ifdef RTE_LIBRTE_BITRATE
#pragma comment (lib, "librte_bitratestats.lib")
#endif
#ifdef RTE_LIBRTE_LATENCY_STATS
#pragma comment (lib, "librte_latencystats.lib")
#endif
#ifdef RTE_LIBRTE_LPM
#pragma comment (lib, "librte_lpm.lib")
#endif
#ifdef RTE_LIBRTE_ACL
#pragma comment (lib, "librte_acl.lib")
#endif
#ifdef RTE_LIBRTE_POWER
#pragma comment (lib, "librte_power.lib")
#endif
#ifdef RTE_LIBRTE_NET
#pragma comment (lib, "librte_net.lib")
#endif
#ifdef RTE_LIBRTE_IP_FRAG
#pragma comment (lib, "librte_ipfrag.lib")
#endif
#ifdef RTE_LIBRTE_GRO
#pragma comment (lib, "librte_gro.lib")
#endif
#ifdef RTE_LIBRTE_GSO
#pragma comment (lib, "librte_gso.lib")
#endif
#ifdef RTE_LIBRTE_METER
#pragma comment (lib, "librte_meter.lib")
#endif
#ifdef RTE_LIBRTE_FLOW_CLASSIFY
#pragma comment (lib, "librte_flowclassify.lib")
#endif
#ifdef RTE_LIBRTE_SCHED
#pragma comment (lib, "librte_sched.lib")
#endif
#ifdef RTE_LIBRTE_DISTRIBUTOR
#pragma comment (lib, "librte_distributor.lib")
#endif
#ifdef RTE_LIBRTE_REORDER
#pragma comment (lib, "librte_reorder.lib")
#endif
#ifdef RTE_LIBRTE_PORT
#pragma comment (lib, "librte_port.lib")
#endif
#ifdef RTE_LIBRTE_TABLE
#pragma comment (lib, "librte_table.lib")
#endif
#ifdef RTE_LIBRTE_PIPELINE
#pragma comment (lib, "librte_pipeline.lib")
#endif
#ifdef RTE_LIBRTE_KNI
#pragma comment (lib, "librte_kni.lib")
#endif
#ifdef RTE_LIBRTE_PMD_KNI
#pragma comment (lib, "librte_pmd_kni.lib")
#endif
#ifdef RTE_LIBRTE_PDUMP
#pragma comment (lib, "librte_pdump.lib")
#endif
#ifdef RTE_LIBRTE_VHOST
#pragma comment (lib, "librte_vhost.lib")
#endif
#ifdef RTE_LIBRTE_PMD_VHOST
#pragma comment (lib, "librte_pmd_vhost.lib")
#endif


#endif /* _RTE_WIN_DPDK_APP */
#endif

#ifdef __cplusplus
}
#endif

#endif /* _RTE_WINDOWS_H_ */
