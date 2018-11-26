/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
*/


#ifndef _SCHED_H_
#define _SCHED_H_

/* Re-defined for Windows */
 
#ifdef __cplusplus
extern "C" {
#endif

int sched_yield(void);

#ifdef __cplusplus
}
#endif

#endif /* _SCHED_H_ */
