#pragma once

/* 
 * $NetBSD: queue.h,v 1.68 2014/11/19 08:10:01 uebayasi Exp $
 * 
 * Need to define _KERNEL to avoid the __launder_type()_ function
 *
 */
#define _KERNEL
#include "netbsd\queue.h"
#undef _KERNEL
