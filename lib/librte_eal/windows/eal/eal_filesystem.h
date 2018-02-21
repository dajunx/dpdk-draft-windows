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

/**
 * @file
 * Stores functions and path defines for files and directories
 * on the filesystem for Windows, that are used by the Windows EAL.
 */

#ifndef EAL_FILESYSTEM_H
#define EAL_FILESYSTEM_H

#include <rte_windows.h>
#include "eal_internal_cfg.h"

/* define the default filename prefix for the %s values below */
#define HUGEFILE_PREFIX_DEFAULT "rte"

/* Path of rte config file */
#define RUNTIME_CONFIG_FMT "%s\\%s.config"

static inline const char *
eal_runtime_config_path(void)
{
	static char buffer[PATH_MAX];  /* static so auto-zeroed */
	char  Directory[PATH_MAX];

	GetTempPathA(sizeof(Directory), Directory);
	snprintf(buffer, sizeof(buffer)-1, RUNTIME_CONFIG_FMT, Directory, internal_config.hugefile_prefix);

	return buffer;
}

/* Path of hugepage info file */
#define HUGEPAGE_INFO_FMT "%s\\.%s_hugepage_info"

static inline const char *
eal_hugepage_info_path(void)
{
	static char buffer[PATH_MAX];  /* static so auto-zeroed */
	TCHAR  Directory[PATH_MAX];

	GetSystemDirectory(Directory, sizeof(Directory));
	snprintf(buffer, sizeof(buffer)-1, HUGEPAGE_INFO_FMT, Directory, internal_config.hugefile_prefix);
	return buffer;
}

/* String format for hugepage map files */
#define HUGEFILE_FMT "%s/%smap_%d"
#define TEMP_HUGEFILE_FMT "%s/%smap_temp_%d"

static inline const char *
eal_get_hugefile_path(char *buffer, size_t buflen, const char *hugedir, int f_id)
{
	snprintf(buffer, buflen, HUGEFILE_FMT, hugedir, internal_config.hugefile_prefix, f_id);
	buffer[buflen - 1] = '\0';
	return buffer;
}


/** Function to read a single numeric value from a file on the filesystem.
 * Used to read information from files on /sys */
int eal_parse_sysfs_value(const char *filename, unsigned long *val);

#endif /* EAL_FILESYSTEM_H */
