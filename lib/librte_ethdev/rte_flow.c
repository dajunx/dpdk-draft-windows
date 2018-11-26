/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016 6WIND S.A.
 * Copyright 2016 Mellanox Technologies, Ltd
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <rte_common.h>
#include <rte_errno.h>
#include <rte_branch_prediction.h>
#include "rte_ethdev.h"
#include "rte_flow_driver.h"
#include "rte_flow.h"

/**
 * Flow elements description tables.
 */
struct rte_flow_desc_data {
	const char *name;
	size_t size;
};

/** Generate flow_item[] entry. */
#define MK_FLOW_ITEM(t, s) \
	[RTE_FLOW_ITEM_TYPE_ ## t] = { \
		.name = # t, \
		.size = s, \
	}

/** Information about known flow pattern items. */
static const struct rte_flow_desc_data rte_flow_desc_item[] = {
	MK_FLOW_ITEM(END, 0),
	MK_FLOW_ITEM(VOID, 0),
	MK_FLOW_ITEM(INVERT, 0),
	MK_FLOW_ITEM(ANY, sizeof(struct rte_flow_item_any)),
	MK_FLOW_ITEM(PF, 0),
	MK_FLOW_ITEM(VF, sizeof(struct rte_flow_item_vf)),
	MK_FLOW_ITEM(PHY_PORT, sizeof(struct rte_flow_item_phy_port)),
	MK_FLOW_ITEM(PORT_ID, sizeof(struct rte_flow_item_port_id)),
	MK_FLOW_ITEM(RAW, sizeof(struct rte_flow_item_raw)),
	MK_FLOW_ITEM(ETH, sizeof(struct rte_flow_item_eth)),
	MK_FLOW_ITEM(VLAN, sizeof(struct rte_flow_item_vlan)),
	MK_FLOW_ITEM(IPV4, sizeof(struct rte_flow_item_ipv4)),
	MK_FLOW_ITEM(IPV6, sizeof(struct rte_flow_item_ipv6)),
	MK_FLOW_ITEM(ICMP, sizeof(struct rte_flow_item_icmp)),
	MK_FLOW_ITEM(UDP, sizeof(struct rte_flow_item_udp)),
	MK_FLOW_ITEM(TCP, sizeof(struct rte_flow_item_tcp)),
	MK_FLOW_ITEM(SCTP, sizeof(struct rte_flow_item_sctp)),
	MK_FLOW_ITEM(VXLAN, sizeof(struct rte_flow_item_vxlan)),
	MK_FLOW_ITEM(MPLS, sizeof(struct rte_flow_item_mpls)),
	MK_FLOW_ITEM(GRE, sizeof(struct rte_flow_item_gre)),
	MK_FLOW_ITEM(E_TAG, sizeof(struct rte_flow_item_e_tag)),
	MK_FLOW_ITEM(NVGRE, sizeof(struct rte_flow_item_nvgre)),
	MK_FLOW_ITEM(GENEVE, sizeof(struct rte_flow_item_geneve)),
	MK_FLOW_ITEM(VXLAN_GPE, sizeof(struct rte_flow_item_vxlan_gpe)),
	MK_FLOW_ITEM(ARP_ETH_IPV4, sizeof(struct rte_flow_item_arp_eth_ipv4)),
	MK_FLOW_ITEM(IPV6_EXT, sizeof(struct rte_flow_item_ipv6_ext)),
	MK_FLOW_ITEM(ICMP6, sizeof(struct rte_flow_item_icmp6)),
	MK_FLOW_ITEM(ICMP6_ND_NS, sizeof(struct rte_flow_item_icmp6_nd_ns)),
	MK_FLOW_ITEM(ICMP6_ND_NA, sizeof(struct rte_flow_item_icmp6_nd_na)),
	MK_FLOW_ITEM(ICMP6_ND_OPT, sizeof(struct rte_flow_item_icmp6_nd_opt)),
	MK_FLOW_ITEM(ICMP6_ND_OPT_SLA_ETH,
		     sizeof(struct rte_flow_item_icmp6_nd_opt_sla_eth)),
	MK_FLOW_ITEM(ICMP6_ND_OPT_TLA_ETH,
		     sizeof(struct rte_flow_item_icmp6_nd_opt_tla_eth)),
};

/** Generate flow_action[] entry. */
#define MK_FLOW_ACTION(t, s) \
	[RTE_FLOW_ACTION_TYPE_ ## t] = { \
		.name = # t, \
		.size = s, \
	}

/** Information about known flow actions. */
static const struct rte_flow_desc_data rte_flow_desc_action[] = {
	MK_FLOW_ACTION(END, 0),
	MK_FLOW_ACTION(VOID, 0),
	MK_FLOW_ACTION(PASSTHRU, 0),
	MK_FLOW_ACTION(MARK, sizeof(struct rte_flow_action_mark)),
	MK_FLOW_ACTION(FLAG, 0),
	MK_FLOW_ACTION(QUEUE, sizeof(struct rte_flow_action_queue)),
	MK_FLOW_ACTION(DROP, 0),
	MK_FLOW_ACTION(COUNT, sizeof(struct rte_flow_action_count)),
	MK_FLOW_ACTION(RSS, sizeof(struct rte_flow_action_rss)),
	MK_FLOW_ACTION(PF, 0),
	MK_FLOW_ACTION(VF, sizeof(struct rte_flow_action_vf)),
	MK_FLOW_ACTION(PHY_PORT, sizeof(struct rte_flow_action_phy_port)),
	MK_FLOW_ACTION(PORT_ID, sizeof(struct rte_flow_action_port_id)),
	MK_FLOW_ACTION(OF_SET_MPLS_TTL,
		       sizeof(struct rte_flow_action_of_set_mpls_ttl)),
	MK_FLOW_ACTION(OF_DEC_MPLS_TTL, 0),
	MK_FLOW_ACTION(OF_SET_NW_TTL,
		       sizeof(struct rte_flow_action_of_set_nw_ttl)),
	MK_FLOW_ACTION(OF_DEC_NW_TTL, 0),
	MK_FLOW_ACTION(OF_COPY_TTL_OUT, 0),
	MK_FLOW_ACTION(OF_COPY_TTL_IN, 0),
	MK_FLOW_ACTION(OF_POP_VLAN, 0),
	MK_FLOW_ACTION(OF_PUSH_VLAN,
		       sizeof(struct rte_flow_action_of_push_vlan)),
	MK_FLOW_ACTION(OF_SET_VLAN_VID,
		       sizeof(struct rte_flow_action_of_set_vlan_vid)),
	MK_FLOW_ACTION(OF_SET_VLAN_PCP,
		       sizeof(struct rte_flow_action_of_set_vlan_pcp)),
	MK_FLOW_ACTION(OF_POP_MPLS,
		       sizeof(struct rte_flow_action_of_pop_mpls)),
	MK_FLOW_ACTION(OF_PUSH_MPLS,
		       sizeof(struct rte_flow_action_of_push_mpls)),
};

static int
flow_err(uint16_t port_id, int ret, struct rte_flow_error *error)
{
	if (ret == 0)
		return 0;
	if (rte_eth_dev_is_removed(port_id))
		return rte_flow_error_set(error, EIO,
					  RTE_FLOW_ERROR_TYPE_UNSPECIFIED,
					  NULL, rte_strerror(EIO));
	return ret;
}

/* Get generic flow operations structure from a port. */
const struct rte_flow_ops *
rte_flow_ops_get(uint16_t port_id, struct rte_flow_error *error)
{
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];
	const struct rte_flow_ops *ops;
	int code;

	if (unlikely(!rte_eth_dev_is_valid_port(port_id)))
		code = ENODEV;
	else if (unlikely(!dev->dev_ops->filter_ctrl ||
			  dev->dev_ops->filter_ctrl(dev,
						    RTE_ETH_FILTER_GENERIC,
						    RTE_ETH_FILTER_GET,
						    &ops) ||
			  !ops))
		code = ENOSYS;
	else
		return ops;
	rte_flow_error_set(error, code, RTE_FLOW_ERROR_TYPE_UNSPECIFIED,
			   NULL, rte_strerror(code));
	return NULL;
}

/* Check whether a flow rule can be created on a given port. */
int
rte_flow_validate(uint16_t port_id,
		  const struct rte_flow_attr *attr,
		  const struct rte_flow_item pattern[],
		  const struct rte_flow_action actions[],
		  struct rte_flow_error *error)
{
	const struct rte_flow_ops *ops = rte_flow_ops_get(port_id, error);
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];

	if (unlikely(!ops))
		return -rte_errno;
	if (likely(!!ops->validate))
		return flow_err(port_id, ops->validate(dev, attr, pattern,
						       actions, error), error);
	return rte_flow_error_set(error, ENOSYS,
				  RTE_FLOW_ERROR_TYPE_UNSPECIFIED,
				  NULL, rte_strerror(ENOSYS));
}

/* Create a flow rule on a given port. */
struct rte_flow *
rte_flow_create(uint16_t port_id,
		const struct rte_flow_attr *attr,
		const struct rte_flow_item pattern[],
		const struct rte_flow_action actions[],
		struct rte_flow_error *error)
{
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];
	struct rte_flow *flow;
	const struct rte_flow_ops *ops = rte_flow_ops_get(port_id, error);

	if (unlikely(!ops))
		return NULL;
	if (likely(!!ops->create)) {
		flow = ops->create(dev, attr, pattern, actions, error);
		if (flow == NULL)
			flow_err(port_id, -rte_errno, error);
		return flow;
	}
	rte_flow_error_set(error, ENOSYS, RTE_FLOW_ERROR_TYPE_UNSPECIFIED,
			   NULL, rte_strerror(ENOSYS));
	return NULL;
}

/* Destroy a flow rule on a given port. */
int
rte_flow_destroy(uint16_t port_id,
		 struct rte_flow *flow,
		 struct rte_flow_error *error)
{
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];
	const struct rte_flow_ops *ops = rte_flow_ops_get(port_id, error);

	if (unlikely(!ops))
		return -rte_errno;
	if (likely(!!ops->destroy))
		return flow_err(port_id, ops->destroy(dev, flow, error),
				error);
	return rte_flow_error_set(error, ENOSYS,
				  RTE_FLOW_ERROR_TYPE_UNSPECIFIED,
				  NULL, rte_strerror(ENOSYS));
}

/* Destroy all flow rules associated with a port. */
int
rte_flow_flush(uint16_t port_id,
	       struct rte_flow_error *error)
{
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];
	const struct rte_flow_ops *ops = rte_flow_ops_get(port_id, error);

	if (unlikely(!ops))
		return -rte_errno;
	if (likely(!!ops->flush))
		return flow_err(port_id, ops->flush(dev, error), error);
	return rte_flow_error_set(error, ENOSYS,
				  RTE_FLOW_ERROR_TYPE_UNSPECIFIED,
				  NULL, rte_strerror(ENOSYS));
}

/* Query an existing flow rule. */
int
rte_flow_query(uint16_t port_id,
	       struct rte_flow *flow,
	       const struct rte_flow_action *action,
	       void *data,
	       struct rte_flow_error *error)
{
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];
	const struct rte_flow_ops *ops = rte_flow_ops_get(port_id, error);

	if (!ops)
		return -rte_errno;
	if (likely(!!ops->query))
		return flow_err(port_id, ops->query(dev, flow, action, data,
						    error), error);
	return rte_flow_error_set(error, ENOSYS,
				  RTE_FLOW_ERROR_TYPE_UNSPECIFIED,
				  NULL, rte_strerror(ENOSYS));
}

/* Restrict ingress traffic to the defined flow rules. */
int
rte_flow_isolate(uint16_t port_id,
		 int set,
		 struct rte_flow_error *error)
{
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];
	const struct rte_flow_ops *ops = rte_flow_ops_get(port_id, error);

	if (!ops)
		return -rte_errno;
	if (likely(!!ops->isolate))
		return flow_err(port_id, ops->isolate(dev, set, error), error);
	return rte_flow_error_set(error, ENOSYS,
				  RTE_FLOW_ERROR_TYPE_UNSPECIFIED,
				  NULL, rte_strerror(ENOSYS));
}

/* Initialize flow error structure. */
int
rte_flow_error_set(struct rte_flow_error *error,
		   int code,
		   enum rte_flow_error_type type,
		   const void *cause,
		   const char *message)
{
	if (error) {
		*error = (struct rte_flow_error){
			.type = type,
			.cause = cause,
			.message = message,
		};
	}
	rte_errno = code;
	return -code;
}

/** Compute storage space needed by item specification. */
static void
flow_item_spec_size(const struct rte_flow_item *item,
		    size_t *size, size_t *pad)
{
	if (!item->spec) {
		*size = 0;
		goto empty;
	}
	switch (item->type) {
		union {
			const struct rte_flow_item_raw *raw;
		} spec;

	/* Not a fall-through */
	case RTE_FLOW_ITEM_TYPE_RAW:
		spec.raw = item->spec;
		*size = offsetof(struct rte_flow_item_raw, pattern) +
			spec.raw->length * sizeof(*spec.raw->pattern);
		break;
	default:
		*size = rte_flow_desc_item[item->type].size;
		break;
	}
empty:
	*pad = RTE_ALIGN_CEIL(*size, sizeof(double)) - *size;
}

/** Compute storage space needed by action configuration. */
static void
flow_action_conf_size(const struct rte_flow_action *action,
		      size_t *size, size_t *pad)
{
	if (!action->conf) {
		*size = 0;
		goto empty;
	}
	switch (action->type) {
		union {
			const struct rte_flow_action_rss *rss;
		} conf;

	/* Not a fall-through. */
	case RTE_FLOW_ACTION_TYPE_RSS:
		conf.rss = action->conf;
		*size = offsetof(struct rte_flow_action_rss, queue) +
			conf.rss->queue_num * sizeof(*conf.rss->queue);
		break;
	default:
		*size = rte_flow_desc_action[action->type].size;
		break;
	}
empty:
	*pad = RTE_ALIGN_CEIL(*size, sizeof(double)) - *size;
}

/** Store a full rte_flow description. */
size_t
rte_flow_copy(struct rte_flow_desc *desc, size_t len,
	      const struct rte_flow_attr *attr,
	      const struct rte_flow_item *items,
	      const struct rte_flow_action *actions)
{
	struct rte_flow_desc *fd = NULL;
	size_t tmp;
	size_t pad;
	size_t off1 = 0;
	size_t off2 = 0;
	size_t size = 0;

store:
	if (items) {
		const struct rte_flow_item *item;

		item = items;
		if (fd)
			fd->items = (void *)&fd->data[off1];
		do {
			struct rte_flow_item *dst = NULL;

			if ((size_t)item->type >=
				RTE_DIM(rte_flow_desc_item) ||
			    !rte_flow_desc_item[item->type].name) {
				rte_errno = ENOTSUP;
				return 0;
			}
			if (fd)
				dst = memcpy(fd->data + off1, item,
					     sizeof(*item));
			off1 += sizeof(*item);
			flow_item_spec_size(item, &tmp, &pad);
			if (item->spec) {
				if (fd)
					dst->spec = memcpy(fd->data + off2,
							   item->spec, tmp);
				off2 += tmp + pad;
			}
			if (item->last) {
				if (fd)
					dst->last = memcpy(fd->data + off2,
							   item->last, tmp);
				off2 += tmp + pad;
			}
			if (item->mask) {
				if (fd)
					dst->mask = memcpy(fd->data + off2,
							   item->mask, tmp);
				off2 += tmp + pad;
			}
			off2 = RTE_ALIGN_CEIL(off2, sizeof(double));
		} while ((item++)->type != RTE_FLOW_ITEM_TYPE_END);
		off1 = RTE_ALIGN_CEIL(off1, sizeof(double));
	}
	if (actions) {
		const struct rte_flow_action *action;

		action = actions;
		if (fd)
			fd->actions = (void *)&fd->data[off1];
		do {
			struct rte_flow_action *dst = NULL;

			if ((size_t)action->type >=
				RTE_DIM(rte_flow_desc_action) ||
			    !rte_flow_desc_action[action->type].name) {
				rte_errno = ENOTSUP;
				return 0;
			}
			if (fd)
				dst = memcpy(fd->data + off1, action,
					     sizeof(*action));
			off1 += sizeof(*action);
			flow_action_conf_size(action, &tmp, &pad);
			if (action->conf) {
				if (fd)
					dst->conf = memcpy(fd->data + off2,
							   action->conf, tmp);
				off2 += tmp + pad;
			}
			off2 = RTE_ALIGN_CEIL(off2, sizeof(double));
		} while ((action++)->type != RTE_FLOW_ACTION_TYPE_END);
	}
	if (fd != NULL)
		return size;
	off1 = RTE_ALIGN_CEIL(off1, sizeof(double));
	tmp = RTE_ALIGN_CEIL(offsetof(struct rte_flow_desc, data),
			     sizeof(double));
	size = tmp + off1 + off2;
	if (size > len)
		return size;
	fd = desc;
	if (fd != NULL) {
		*fd = (const struct rte_flow_desc) {
			.size = size,
			.attr = *attr,
		};
		tmp -= offsetof(struct rte_flow_desc, data);
		off2 = tmp + off1;
		off1 = tmp;
		goto store;
	}
	return 0;
}

/**
 * Expand RSS flows into several possible flows according to the RSS hash
 * fields requested and the driver capabilities.
 */
int __rte_experimental
rte_flow_expand_rss(struct rte_flow_expand_rss *buf, size_t size,
		    const struct rte_flow_item *pattern, uint64_t types,
		    const struct rte_flow_expand_node graph[],
		    int graph_root_index)
{
	const int elt_n = 8;
	const struct rte_flow_item *item;
	const struct rte_flow_expand_node *node = &graph[graph_root_index];
	const int *next_node;
	const int *stack[elt_n];
	int stack_pos = 0;
	struct rte_flow_item flow_items[elt_n];
	unsigned int i;
	size_t lsize;
	size_t user_pattern_size = 0;
	void *addr = NULL;

	lsize = offsetof(struct rte_flow_expand_rss, entry) +
		elt_n * sizeof(buf->entry[0]);
	if (lsize <= size) {
		buf->entry[0].priority = 0;
		buf->entry[0].pattern = (void *)&buf->entry[elt_n];
		buf->entries = 0;
		addr = buf->entry[0].pattern;
	}
	for (item = pattern; item->type != RTE_FLOW_ITEM_TYPE_END; item++) {
		const struct rte_flow_expand_node *next = NULL;

		for (i = 0; node->next && node->next[i]; ++i) {
			next = &graph[node->next[i]];
			if (next->type == item->type)
				break;
		}
		if (next)
			node = next;
		user_pattern_size += sizeof(*item);
	}
	user_pattern_size += sizeof(*item); /* Handle END item. */
	lsize += user_pattern_size;
	/* Copy the user pattern in the first entry of the buffer. */
	if (lsize <= size) {
		rte_memcpy(addr, pattern, user_pattern_size);
		addr = (void *)(((uintptr_t)addr) + user_pattern_size);
		buf->entries = 1;
	}
	/* Start expanding. */
	memset(flow_items, 0, sizeof(flow_items));
	user_pattern_size -= sizeof(*item);
	next_node = node->next;
	stack[stack_pos] = next_node;
	node = next_node ? &graph[*next_node] : NULL;
	while (node) {
		flow_items[stack_pos].type = node->type;
		if (node->rss_types & types) {
			/*
			 * compute the number of items to copy from the
			 * expansion and copy it.
			 * When the stack_pos is 0, there are 1 element in it,
			 * plus the addition END item.
			 */
			int elt = stack_pos + 2;

			flow_items[stack_pos + 1].type = RTE_FLOW_ITEM_TYPE_END;
			lsize += elt * sizeof(*item) + user_pattern_size;
			if (lsize <= size) {
				size_t n = elt * sizeof(*item);

				buf->entry[buf->entries].priority =
					stack_pos + 1;
				buf->entry[buf->entries].pattern = addr;
				buf->entries++;
				rte_memcpy(addr, buf->entry[0].pattern,
					   user_pattern_size);
				addr = (void *)(((uintptr_t)addr) +
						user_pattern_size);
				rte_memcpy(addr, flow_items, n);
				addr = (void *)(((uintptr_t)addr) + n);
			}
		}
		/* Go deeper. */
		if (node->next) {
			next_node = node->next;
			if (stack_pos++ == elt_n) {
				rte_errno = E2BIG;
				return -rte_errno;
			}
			stack[stack_pos] = next_node;
		} else if (*(next_node + 1)) {
			/* Follow up with the next possibility. */
			++next_node;
		} else {
			/* Move to the next path. */
			if (stack_pos)
				next_node = stack[--stack_pos];
			next_node++;
			stack[stack_pos] = next_node;
		}
		node = *next_node ? &graph[*next_node] : NULL;
	};
	return lsize;
}
