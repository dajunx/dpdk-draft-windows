/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
*/

#pragma once

#include "..\..\common\include\rte_per_lcore.h"

/* Undefine the stuff that is problematic for windows and redefine */
#undef RTE_DEFINE_PER_LCORE
#undef RTE_DECLARE_PER_LCORE


/**
 * @file
 * Per-lcore variables in RTE on windows environment
 */

/**
 * Macro to define a per lcore variable "name" of type "type", don't
 * use keywords like "static" or "volatile" in type, just prefix the
 * whole macro.
 */
#define RTE_DEFINE_PER_LCORE(type, name)			__declspec(thread) type per_lcore_##name

/**
 * Macro to declare an extern per lcore variable "name" of type "type"
 */
#define RTE_DECLARE_PER_LCORE(type, name)			__declspec(thread) extern type per_lcore_##name


