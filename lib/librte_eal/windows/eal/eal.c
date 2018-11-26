/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
*/

#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <direct.h>

#include <rte_eal.h>
#include <rte_eal_memconfig.h>
#include <rte_log.h>
#include <rte_random.h>
#include <rte_cycles.h>
#include <rte_errno.h>
#include <rte_windows.h>
#include <malloc_heap.h>

#include "eal_private.h"
#include "eal_thread.h"
#include "eal_internal_cfg.h"
#include "eal_filesystem.h"
#include "eal_hugepages.h"
#include "eal_options.h"
#include "private.h"

#define MEMSIZE_IF_NO_HUGE_PAGE (64ULL * 1024ULL * 1024ULL)

/* Allow the application to print its usage message too if set */
static rte_usage_hook_t	rte_application_usage_hook = NULL;

/* define fd variable here, because file needs to be kept open for the
 * duration of the program, as we hold a write lock on it in the primary proc */

static int mem_cfg_fd = -1; // INVALID_HANDLE_VALUE;
/*
static struct flock wr_lock = {
		.l_type = F_WRLCK,
		.l_whence = SEEK_SET,
		.l_start = offsetof(struct rte_mem_config, memseg),
		.l_len = sizeof(early_mem_config.memseg),
};
*/
/* early configuration structure, when memory config is not mmapped */
static struct rte_mem_config early_mem_config;

/* Address of global and public configuration */
static struct rte_config rte_config = {
		.mem_config = &early_mem_config,
};

/* internal configuration (per-core) */
struct lcore_config lcore_config[RTE_MAX_LCORE];

/* internal configuration */
struct internal_config internal_config;

/* external function protoypes */
/* these functions are created by the RTE_REGISTER_BUS macro */
extern void businitfn_pci(void);
extern void businitfn_vdev(void);

/* these functions are created by the MEMPOOL_REGISTER_OPS macro */
extern void mp_hdlr_init_ops_mp_mc(void);
extern void mp_hdlr_init_ops_sp_sc(void);
extern void mp_hdlr_init_ops_mp_sc(void);
extern void mp_hdlr_init_ops_sp_mc(void);

/* these functions are created by the EAL_REGISTER_TAILQ macro */
extern void init_rte_mempool_tailq(void);
extern void init_rte_ring_tailq(void);
extern void init_rte_hash_tailq(void);
extern void init_rte_fbk_hash_tailq(void);
extern void init_rte_distributor_tailq(void);
extern void init_rte_dist_burst_tailq(void);
extern void init_rte_uio_tailq(void);
extern void init_rte_lpm_tailq(void);
extern void init_rte_lpm6_tailq(void);

/* these functions are created by the RTE_PMD_REGISTER_PCI macro */
extern void pciinitfn_net_i40e(void);

/* these are more constructor-like function, that we'll need to call at the start */
extern void rte_timer_init(void);
extern void rte_log_init(void);
extern void i40e_init_log(void);

/* Return a pointer to the configuration structure */
struct rte_config *
rte_eal_get_configuration(void)
{
	return &rte_config;
}

/* Return mbuf pool ops name */
const char *
rte_eal_mbuf_user_pool_ops(void)
{
	return internal_config.user_mbuf_pool_ops_name;
}

enum rte_iova_mode
rte_eal_iova_mode(void)
{
	return rte_eal_get_configuration()->iova_mode;
}

/* platform-specific runtime dir */
static char runtime_dir[PATH_MAX];

/* create memory configuration in shared/mmap memory. Take out
 * a write lock on the memsegs, so we can auto-detect primary/secondary.
 * This means we never close the file while running (auto-close on exit).
 * We also don't lock the whole file, so that in future we can use read-locks
 * on other parts, e.g. memzones, to detect if there are running secondary
 * processes. */
static void
rte_eal_config_create(void)
{
	void *rte_mem_cfg_addr;
	BOOL retval;

	const char *pathname = eal_runtime_config_path();

	if (internal_config.no_shconf)
		return;

	if (mem_cfg_fd < 0) {
	    mem_cfg_fd = _open(pathname, _O_CREAT | _O_RDWR | _O_TRUNC, _S_IREAD | _S_IWRITE);
	    if (mem_cfg_fd < 0)
		rte_panic("Cannot open '%s' for rte_mem_config...Error: %d\n", pathname, errno);
	}

	/* Lock file for exclusive access */
	OVERLAPPED sOverlapped = {0};
	sOverlapped.Offset = sizeof(*rte_config.mem_config);
	sOverlapped.OffsetHigh = 0;

	HANDLE hWinFileHandle = (HANDLE)_get_osfhandle(mem_cfg_fd);
	retval = LockFileEx(hWinFileHandle,
			    LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0,
			    sizeof(*rte_config.mem_config), 0, &sOverlapped);
	if (!retval) {
	    _close(mem_cfg_fd);
	    rte_exit(EXIT_FAILURE, "Cannot create lock on '%s'. Is another primary process running?\n", pathname);
	}

	rte_mem_cfg_addr = mmap(NULL, sizeof(*rte_config.mem_config),
				PROT_READ | PROT_WRITE, MAP_SHARED, (int)mem_cfg_fd, 0);

	if (rte_mem_cfg_addr == MAP_FAILED) {
	    _close(mem_cfg_fd);
	    rte_exit(EXIT_FAILURE, "Cannot mmap memory for rte_config\n");
	}

	memcpy(rte_mem_cfg_addr, &early_mem_config, sizeof(early_mem_config));
	rte_config.mem_config = (struct rte_mem_config *) rte_mem_cfg_addr;
}

/* attach to an existing shared memory config */
static void
rte_eal_config_attach(void)
{
	void *rte_mem_cfg_addr;
	const char *pathname = eal_runtime_config_path();

	if (internal_config.no_shconf)
		return;

	if (mem_cfg_fd < 0) {
	    mem_cfg_fd = _open(pathname, O_RDWR);
	    if (mem_cfg_fd < 0)
		rte_panic("Cannot open '%s' for rte_mem_config\n", pathname);
	}

	rte_mem_cfg_addr = mmap(NULL, sizeof(*rte_config.mem_config), PROT_READ | PROT_WRITE, MAP_SHARED, mem_cfg_fd, 0);
	_close(mem_cfg_fd);

	if (rte_mem_cfg_addr == MAP_FAILED)
		rte_panic("Cannot mmap memory for rte_config\n");

	rte_config.mem_config = (struct rte_mem_config *) rte_mem_cfg_addr;
}

/* Detect if we are a primary or a secondary process */
enum rte_proc_type_t
eal_proc_type_detect(void)
{
	enum rte_proc_type_t ptype = RTE_PROC_PRIMARY;
	const char *pathname = eal_runtime_config_path();

	/* if we can open the file but not get a write-lock we are a secondary
	 * process. NOTE: if we get a file handle back, we keep that open
	 * and don't close it to prevent a race condition between multiple opens */
	if ((mem_cfg_fd = _open(pathname, O_RDWR)) >= 0) {
	    OVERLAPPED sOverlapped = { 0 };
	    sOverlapped.Offset = sizeof(*rte_config.mem_config);
	    sOverlapped.OffsetHigh = 0;

	    HANDLE hWinFileHandle = (HANDLE)_get_osfhandle(mem_cfg_fd);

	    if (!LockFileEx(hWinFileHandle,
			    LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0,
			    sizeof(*rte_config.mem_config), 0, &sOverlapped))
		ptype = RTE_PROC_SECONDARY;
	}

	RTE_LOG(INFO, EAL, "Auto-detected process type: %s\n",
			    ptype == RTE_PROC_PRIMARY ? "PRIMARY" : "SECONDARY");

	return ptype;
}

/* Sets up rte_config structure with the pointer to shared memory config.*/
static void
rte_config_init(void)
{
	rte_config.process_type = internal_config.process_type;

	switch (rte_config.process_type){
	case RTE_PROC_PRIMARY:
		rte_eal_config_create();
		break;
	case RTE_PROC_SECONDARY:
		rte_eal_config_attach();
		rte_eal_mcfg_wait_complete(rte_config.mem_config);
		break;
	case RTE_PROC_AUTO:
	case RTE_PROC_INVALID:
		rte_panic("Invalid process type\n");
	}
}

/* display usage */
static void
eal_usage(const char *prgname)
{
	printf("\nUsage: %s ", prgname);
	eal_common_usage();
	/* Allow the application to print its usage message too if hook is set */
	if ( rte_application_usage_hook ) {
		printf("===== Application Usage =====\n\n");
		rte_application_usage_hook(prgname);
	}
}

/* Set a per-application usage message */
rte_usage_hook_t
rte_set_application_usage_hook( rte_usage_hook_t usage_func )
{
	rte_usage_hook_t	old_func;

	/* Will be NULL on the first call to denote the last usage routine. */
	old_func			= rte_application_usage_hook;
	rte_application_usage_hook	= usage_func;

	return old_func;
}

static inline size_t
eal_get_hugepage_mem_size(void)
{
	uint64_t size = 0;
	size = (uint64_t)GetLargePageMinimum();

	return (size < SIZE_MAX) ? (size_t)(size) : SIZE_MAX;
}

/* Parse the arguments for --log-level only */
static void
eal_log_level_parse(int argc, char **argv)
{
	int opt;
	char **argvopt;
	int option_index;

	argvopt = argv;

	eal_reset_internal_config(&internal_config);

	while ((opt = getopt_long(argc, argvopt, eal_short_options,
				  eal_long_options, &option_index)) != EOF) {

		int ret;

		/* getopt is not happy, stop right now */
		if (opt == '?')
			break;

		ret = (opt == OPT_LOG_LEVEL_NUM) ?
			eal_parse_common_option(opt, optarg, &internal_config) : 0;

		/* common parser is not happy */
		if (ret < 0)
			break;
	}

	optind = 0; /* reset getopt lib */
}

/* Parse the argument given in the command line of the application */
static int
eal_parse_args(int argc, char **argv)
{
	int opt, ret;
	char **argvopt;
	int option_index;
	char *prgname = argv[0];

	argvopt = argv;

	while ((opt = getopt_long(argc, argvopt, eal_short_options,
				  eal_long_options, &option_index)) != EOF) {

		int ret;

		/* getopt is not happy, stop right now */
		if (opt == '?') {
			eal_usage(prgname);
			return -1;
		}

		ret = eal_parse_common_option(opt, optarg, &internal_config);
		/* common parser is not happy */
		if (ret < 0) {
			eal_usage(prgname);
			return -1;
		}
		/* common parser handled this option */
		if (ret == 0)
			continue;

		switch (opt) {
		case 'h':
			eal_usage(prgname);
			exit(EXIT_SUCCESS);
		default:
			if (opt < OPT_LONG_MIN_NUM && isprint(opt)) {
				RTE_LOG(ERR, EAL, "Option %c is not supported "
					"on FreeBSD\n", opt);
			} else if (opt >= OPT_LONG_MIN_NUM &&
				   opt < OPT_LONG_MAX_NUM) {
				RTE_LOG(ERR, EAL, "Option %s is not supported "
					"on FreeBSD\n",
					eal_long_options[option_index].name);
			} else {
				RTE_LOG(ERR, EAL, "Option %d is not supported "
					"on FreeBSD\n", opt);
			}
			eal_usage(prgname);
			return -1;
		}
	}

	/* create runtime data directory */
	if (internal_config.no_shconf == 0 &&
		eal_create_runtime_dir() < 0) {
		RTE_LOG(ERR, EAL, "Cannot create runtime directory\n");
		return ret = -1;
	}

	if (eal_adjust_config(&internal_config) != 0)
		return -1;

	/* sanity checks */
	if (eal_check_common_options(&internal_config) != 0) {
		eal_usage(prgname);
		return -1;
	}

	if (optind >= 0)
		argv[optind-1] = prgname;
	ret = optind-1;
	optind = 0; /* reset getopt lib */
	return ret;
}

static int
check_socket(const struct rte_memseg_list *msl, void *arg)
{
	int *socket_id = arg;

	return *socket_id == msl->socket_id;
}

static void
eal_check_mem_on_local_socket(void)
{
	const struct rte_memseg *ms;
	int i, socket_id;

	socket_id = rte_lcore_to_socket_id(rte_config.master_lcore);

	if (rte_memseg_list_walk(check_socket, &socket_id) == 0)
		RTE_LOG(WARNING, EAL, "WARNING: Master core has no memory on local socket!\n");
}

static int
sync_func(__attribute__((unused)) void *arg)
{
	return 0;
}

inline static void
rte_eal_mcfg_complete(void)
{
	/* ALL shared mem_config related INIT DONE */
	if (rte_config.process_type == RTE_PROC_PRIMARY)
		rte_config.mem_config->magic = RTE_MAGIC;
}

/* return non-zero if hugepages are enabled. */
int rte_eal_has_hugepages(void)
{
	return !internal_config.no_hugetlbfs;
}

/* Abstraction for port I/0 privilege */
int
rte_eal_iopl_init(void)
{
	// Not required on modern processors?
	return -1;
}

static void rte_eal_init_alert(const char *msg)
{
	fprintf(stderr, "EAL: FATAL: %s\n", msg);
	RTE_LOG(ERR, EAL, "%s\n", msg);
}

/* Register and initialize all buses */
/* (This is a workaround for Windows in lieu of a constructor-like function) */
static void
eal_register_and_init_buses()
{
	businitfn_pci();
	/* businitfn_vdev(); Not presently supported! */
}

/* Register and initialize all mempools */
/* (This is a workaround for Windows in lieu of a constructor-like function) */
static void
eal_register_and_init_mempools()
{
	/* these functions are created by the MEMPOOL_REGISTER_OPS macro */
	mp_hdlr_init_ops_mp_mc();
	mp_hdlr_init_ops_sp_sc();
	mp_hdlr_init_ops_mp_sc();
	mp_hdlr_init_ops_sp_mc();
}

/* Register and initialize tailqs */
/* (This is a workaround for Windows in lieu of a constructor-like function) */
static void
eal_register_and_init_tailq()
{
	/* these functions are created by the EAL_REGISTER_TAILQ macro */
	init_rte_mempool_tailq();
	init_rte_ring_tailq();
	init_rte_hash_tailq();
	init_rte_fbk_hash_tailq();
	init_rte_distributor_tailq();
	init_rte_dist_burst_tailq();
	init_rte_uio_tailq();
	init_rte_lpm_tailq();
	init_rte_lpm6_tailq();
}

/* Register and initialize all supported PMDs */
/* (This is a workaround for Windows in lieu of a constructor-like function) */
static void
eal_register_and_init_pmd()
{
	/* these functions are created by the RTE_PMD_REGISTER_PCI macro */
	pciinitfn_net_i40e();  /* init the Intel 40GbE PMD */
}

/* Launch threads, called at application init(). */
int
rte_eal_init(int argc, char **argv)
{
	int i, fctret, ret;
	pthread_t thread_id;
	static rte_atomic32_t run_once = RTE_ATOMIC32_INIT(0);
	char cpuset[RTE_CPU_AFFINITY_STR_LEN];

//	/* constructor replacement */
//	extern void eal_common_timer_init(void);

	/* Register and initialize all the buses we'll be using */
	/* This has to be done at the very start! */
	eal_register_and_init_buses();

	/* Register and initialize all the mempools we'll be using */
	/* We need to do this before any other eal init calls */
	eal_register_and_init_mempools();

	/* Register and initialize all the tailqs we'll be using */
	/* We need to do this before any other eal init calls */
	eal_register_and_init_tailq();

	/* Register and initialize all PMDs that we may use */
	/* We need to do this before any other eal init calls */
	eal_register_and_init_pmd();

	rte_timer_init(); /* Initialize timer function */

	if (!rte_atomic32_test_and_set(&run_once))
		return -1;

	thread_id = pthread_self();

	/* initialize all logs */
	rte_eal_log_init(NULL, 0);
	rte_log_init();
	i40e_init_log();

	eal_log_level_parse(argc, argv);

	/* set log level as early as possible */
	rte_log_set_global_level(RTE_LOG_LEVEL);

	/* create a map of all processors in the system */
	eal_create_cpu_map();

	if (rte_eal_cpu_init() < 0)
		rte_panic("Cannot detect lcores\n");

	fctret = eal_parse_args(argc, argv);
	if (fctret < 0)
		exit(1);

	rte_config_init();

	if (internal_config.no_hugetlbfs == 0 &&
			internal_config.process_type != RTE_PROC_SECONDARY &&
			eal_hugepage_info_init() < 0)
		rte_panic("Cannot get hugepage information\n");

	if (internal_config.memory == 0 && internal_config.force_sockets == 0) {
		if (internal_config.no_hugetlbfs)
			internal_config.memory = MEMSIZE_IF_NO_HUGE_PAGE;
		else
			internal_config.memory = eal_get_hugepage_mem_size();
	}

	if (internal_config.vmware_tsc_map == 1) {
#ifdef RTE_LIBRTE_EAL_VMWARE_TSC_MAP_SUPPORT
		rte_cycles_vmware_tsc_map = 1;
		RTE_LOG (DEBUG, EAL, "Using VMWARE TSC MAP, "
				"you must have monitor_control.pseudo_perfctr = TRUE\n");
#else
		RTE_LOG (WARNING, EAL, "Ignoring --vmware-tsc-map because "
				"RTE_LIBRTE_EAL_VMWARE_TSC_MAP_SUPPORT is not set\n");
#endif
	}

	rte_srand(rte_rdtsc());

	/* Do the PCI scan before initializing memory since we need the memory region
	   mapped inside the kernel driver which we'll retrieve via an IOCTL */
	if (rte_pci_scan() < 0)
		rte_panic("Cannot init PCI\n");

	/* in secondary processes, memory init may allocate additional fbarrays
	* not present in primary processes, so to avoid any potential issues,
	* initialize memzones first.
	*/
	if (rte_eal_memzone_init() < 0)
		rte_panic("Cannot init memzone\n");

	if (rte_eal_memory_init() < 0)
		rte_panic("Cannot init memory\n");

	if (rte_eal_malloc_heap_init() < 0)
		rte_panic("Cannot init malloc heap\n");

	if (rte_eal_tailqs_init() < 0)
		rte_panic("Cannot init tail queues for objects\n");

	if (rte_eal_alarm_init() < 0)
		rte_panic("Cannot init interrupt-handling thread\n");

	if (rte_eal_intr_init() < 0)
		rte_panic("Cannot init interrupt-handling thread\n");

	if (rte_eal_timer_init() < 0)
		rte_panic("Cannot init HPET or TSC timers\n");

	eal_check_mem_on_local_socket();

	eal_thread_init_master(rte_config.master_lcore);

	ret = eal_thread_dump_affinity(cpuset, RTE_CPU_AFFINITY_STR_LEN);

	RTE_LOG(DEBUG, EAL, "Master lcore %u is ready (tid=%p;cpuset=[%s%s])\n",
		rte_config.master_lcore, thread_id, cpuset,
		ret == 0 ? "" : "...");

	RTE_LCORE_FOREACH_SLAVE(i) {

		/*
		 * create communication pipes between master thread
		 * and children
		 */
		if (pipe(lcore_config[i].pipe_master2slave) < 0)
			rte_panic("Cannot create pipe\n");
		if (pipe(lcore_config[i].pipe_slave2master) < 0)
			rte_panic("Cannot create pipe\n");

		lcore_config[i].state = WAIT;

		/* create a thread for each lcore */
		ret = pthread_create(&lcore_config[i].thread_id, NULL,
				     eal_thread_loop, NULL);
		if (ret != 0)
			rte_panic("Cannot create thread\n");
	}

	/*
	 * Launch a dummy function on all slave lcores, so that master lcore
	 * knows they are all ready when this function returns.
	 */
	rte_eal_mp_remote_launch(sync_func, NULL, SKIP_MASTER);
	rte_eal_mp_wait_lcore();

#ifdef EAL_SERVICES_SUPPORT
	/* Not supported on Windows, presently */
	/* initialize services so vdevs register service during bus_probe. */
	ret = rte_service_init();
	if (ret) {
		rte_eal_init_alert("rte_service_init() failed\n");
		rte_errno = ENOEXEC;
		return -1;
	}
#endif

	/* Probe & Initialize PCI devices */
	if (rte_bus_probe())
		rte_panic("Cannot probe PCI\n");

#ifdef EAL_SERVICES_SUPPORT
	/* initialize default service/lcore mappings and start running. Ignore
	* -ENOTSUP, as it indicates no service coremask passed to EAL.
	*/
	ret = rte_service_start_with_defaults();
	if (ret < 0 && ret != -ENOTSUP) {
		rte_errno = ENOEXEC;
		return -1;
	}
#endif
	rte_eal_mcfg_complete();

	return fctret;
}

/* get core role */
enum rte_lcore_role_t
rte_eal_lcore_role(unsigned lcore_id)
{
	return rte_config.lcore_role[lcore_id];
}

enum rte_proc_type_t
rte_eal_process_type(void)
{
	return rte_config.process_type;
}

int
eal_create_runtime_dir(void)
{
	char  Directory[PATH_MAX];

	GetTempPathA(sizeof(Directory), Directory);

	char tmp[PATH_MAX];
	int ret;


	/* create DPDK subdirectory under runtime dir */
	ret = snprintf(tmp, sizeof(tmp), "%s\\dpdk", Directory);
	if (ret < 0 || ret == sizeof(tmp)) {
		RTE_LOG(ERR, EAL, "Error creating DPDK runtime path name\n");
		return -1;
	}

	/* create prefix-specific subdirectory under DPDK runtime dir */
	ret = snprintf(runtime_dir, sizeof(runtime_dir), "%s\\%s",
		tmp, internal_config.hugefile_prefix);
	if (ret < 0 || ret == sizeof(runtime_dir)) {
		RTE_LOG(ERR, EAL, "Error creating prefix-specific runtime path name\n");
		return -1;
	}

	/* create the path if it doesn't exist. no "mkdir -p" here, so do it
	* step by step.
	*/
	ret = mkdir(tmp);
	if (ret < 0 && errno != EEXIST) {
		RTE_LOG(ERR, EAL, "Error creating '%s': %s\n",
			tmp, strerror(errno));
		return -1;
	}

	ret = mkdir(runtime_dir);
	if (ret < 0 && errno != EEXIST) {
		RTE_LOG(ERR, EAL, "Error creating '%s': %s\n",
			runtime_dir, strerror(errno));
		return -1;
	}

	return 0;
}

const char *
eal_get_runtime_dir(void)
{
	return runtime_dir;
}
