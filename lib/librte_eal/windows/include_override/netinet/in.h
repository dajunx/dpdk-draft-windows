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
	} __u6_addr;			/* 128-bit IP6 address */
};

#define INET6_ADDRSTRLEN	46

#ifndef _IN_ADDR_T_DECLARED
typedef	uint32_t		in_addr_t;
#define	_IN_ADDR_T_DECLARED
#endif

#define	_STRUCT_IN_ADDR_DECLARED
/* Internet address (a structure for historical reasons). */
#ifndef	_STRUCT_IN_ADDR_DECLARED
struct in_addr {
	in_addr_t s_addr;
};
#define	_STRUCT_IN_ADDR_DECLARED
#endif

#endif
