/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
*/

#include <rte_common.h>
#include <rte_interrupts.h>
#include "eal_private.h"

int
rte_intr_callback_register(const struct rte_intr_handle *intr_handle,
				rte_intr_callback_fn cb,
				void *cb_arg)
{
	return -ENOTSUP;
}

int
rte_intr_callback_unregister(const struct rte_intr_handle *intr_handle,
				rte_intr_callback_fn cb_fn,
				void *cb_arg)
{
	return -ENOTSUP;
}

int
rte_intr_enable(const struct rte_intr_handle *intr_handle)
{
	return -ENOTSUP;
}

int
rte_intr_disable(const struct rte_intr_handle *intr_handle)
{
	return -ENOTSUP;
}

int
rte_eal_intr_init(void)
{
	return 0;
}

int
rte_intr_rx_ctl(struct rte_intr_handle *intr_handle,
		int epfd, int op, unsigned int vec, void *data)
{
	RTE_SET_USED(intr_handle);
	RTE_SET_USED(epfd);
	RTE_SET_USED(op);
	RTE_SET_USED(vec);
	RTE_SET_USED(data);

	return -ENOTSUP;
}

int
rte_intr_efd_enable(struct rte_intr_handle *intr_handle, uint32_t nb_efd)
{
	RTE_SET_USED(intr_handle);
	RTE_SET_USED(nb_efd);

	return 0;
}

void
rte_intr_efd_disable(struct rte_intr_handle *intr_handle)
{
	RTE_SET_USED(intr_handle);
}

int
rte_intr_dp_is_en(struct rte_intr_handle *intr_handle)
{
	RTE_SET_USED(intr_handle);
	return 0;
}

int
rte_intr_allow_others(struct rte_intr_handle *intr_handle)
{
	RTE_SET_USED(intr_handle);
	return 1;
}

int
rte_intr_cap_multiple(struct rte_intr_handle *intr_handle)
{
	RTE_SET_USED(intr_handle);
	return 0;
}
