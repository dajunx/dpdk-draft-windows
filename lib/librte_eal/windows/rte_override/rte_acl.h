#pragma once

#include "..\..\..\librte_acl\rte_acl.h"

#undef RTE_ACL_MASKLEN_TO_BITMASK
#define	RTE_ACL_MASKLEN_TO_BITMASK(v, s)	\
((v) == 0 ? (v) : ((uint64_t)-1 << ((s) * CHAR_BIT - (v))))

