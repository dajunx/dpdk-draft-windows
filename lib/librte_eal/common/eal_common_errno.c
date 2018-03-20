/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2014 Intel Corporation
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include <rte_per_lcore.h>
#include <rte_errno.h>
#include <rte_string_fns.h>

RTE_DEFINE_PER_LCORE(int, _rte_errno);

const char *
rte_strerror(int errnum)
{
	/* BSD puts a colon in the "unknown error" messages, Linux doesn't */
#ifdef RTE_EXEC_ENV_BSDAPP
	static const char *sep = ":";
#else
	static const char *sep = "";
#endif
#define RETVAL_SZ 256
#ifdef _WIN64
	typedef char c_retval[RETVAL_SZ];
	RTE_DEFINE_PER_LCORE(static c_retval, retval);
#else
	static RTE_DEFINE_PER_LCORE(char[RETVAL_SZ], retval);
#endif
	char *ret = RTE_PER_LCORE(retval);

	/* since some implementations of strerror_r throw an error
	 * themselves if errnum is too big, we handle that case here */
	if (errnum >= RTE_MAX_ERRNO)
		snprintf(ret, RETVAL_SZ, "Unknown error%s %d", sep, errnum);
	else
		switch (errnum){
		case E_RTE_SECONDARY:
			return "Invalid call in secondary process";
		case E_RTE_NO_CONFIG:
			return "Missing rte_config structure";
		default:
#ifdef _WIN64
		    	strerror_s(RTE_PER_LCORE(retval), RETVAL_SZ, errnum);
#else
			if (strerror_r(errnum, ret, RETVAL_SZ) != 0)
				snprintf(ret, RETVAL_SZ, "Unknown error%s %d",
						sep, errnum);
#endif
		}

	return ret;
}
