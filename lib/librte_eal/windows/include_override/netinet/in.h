#ifndef _IN_H_
#define _IN_H_

/*
 * IPv6 address
 */
struct in6_addr {
	union {
		uint8_t		__u6_addr8[16];
		uint16_t	__u6_addr16[8];
		uint32_t	__u6_addr32[4];
	} __u6_addr;		/* 128-bit IP6 address */
};

#define INET6_ADDRSTRLEN	46

#ifndef _IN_ADDR_T_DECLARED
typedef uint32_t		in_addr_t;
#define	_IN_ADDR_T_DECLARED
#endif

// #define	_STRUCT_IN_ADDR_DECLARED
/* Internet address (a structure for historical reasons). */
#ifndef	_STRUCT_IN_ADDR_DECLARED
struct in_addr {
	in_addr_t s_addr;
};
#define	_STRUCT_IN_ADDR_DECLARED
#endif

#define AF_INET			0

#define IPPROTO_IP		0
#define IPPROTO_HOPOPTS		0
#define	IPPROTO_IPV4		4		/* IPv4 encapsulation */
#define	IPPROTO_IPIP		IPPROTO_IPV4	/* for compatibility */
#define IPPROTO_TCP		6
#define IPPROTO_UDP		17
#define	IPPROTO_IPV6		41		/* IP6 header */
#define	IPPROTO_ROUTING		43		/* IP6 routing header */
#define	IPPROTO_FRAGMENT	44		/* IP6 fragmentation header */
#define	IPPROTO_GRE		47		/* General Routing Encap. */
#define	IPPROTO_ESP		50		/* IP6 Encap Sec. Payload */
#define	IPPROTO_AH		51		/* IP6 Auth Header */
#define	IPPROTO_DSTOPTS		60		/* IP6 destination option */


#endif
