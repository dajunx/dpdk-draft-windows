#if !defined(__UNISTD_H_SOURCED__) && !defined(__GETOPT_LONG_H__)
#define __GETOPT_LONG_H__

#ifdef __cplusplus
extern "C" {
#endif

	struct option		/* specification for a long form option...	*/
	{
		const char *name;		/* option name, without leading hyphens */
		int         has_arg;		/* does it take an argument?		*/
		int        *flag;		/* where to save its status, or NULL	*/
		int         val;		/* its associated status value		*/
	};

	enum    		/* permitted values for its `has_arg' field...	*/
	{
		no_argument = 0,      	/* option never takes an argument	*/
		required_argument,		/* option always requires an argument	*/
		optional_argument		/* option may take an argument		*/
	};

	extern int getopt_long(int nargc, char * const *nargv, const char *options,
		const struct option *long_options, int *idx);
	extern int getopt_long_only(int nargc, char * const *nargv, const char *options,
		const struct option *long_options, int *idx);
	/*
	 * Previous MinGW implementation had...
	 */
#ifndef HAVE_DECL_GETOPT
	 /*
	  * ...for the long form API only; keep this for compatibility.
	  */
# define HAVE_DECL_GETOPT	1
#endif

#ifdef __cplusplus
}
#endif

#endif /* !defined(__UNISTD_H_SOURCED__) && !defined(__GETOPT_LONG_H__) */
#ifndef __GETOPT_H__
/**
 * DISCLAIMER
 * This file is a part of the w64 mingw-runtime package.
 *
 * The w64 mingw-runtime package and its code is distributed in the hope that it 
 * will be useful but WITHOUT ANY WARRANTY.  ALL WARRANTIES, EXPRESSED OR 
 * IMPLIED ARE HEREBY DISCLAIMED.  This includes but is not limited to 
 * warranties of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
* Copyright (c) 2002 Todd C. Miller <Todd.Miller@courtesan.com>
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*
* Sponsored in part by the Defense Advanced Research Projects
* Agency (DARPA) and Air Force Research Laboratory, Air Force
* Materiel Command, USAF, under agreement number F39502-99-1-0512.
*/
/*-
* Copyright (c) 2000 The NetBSD Foundation, Inc.
* All rights reserved.
*
* This code is derived from software contributed to The NetBSD Foundation
* by Dieter Baron and Thomas Klausner.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
* ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

#pragma warning(disable:4996);

#define __GETOPT_H__

/* All the headers include this file. */
#include <crtdefs.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	REPLACE_GETOPT		/* use this getopt as the system getopt(3) */

#define PRINT_ERROR	((opterr) && (*options != ':'))

#define FLAG_PERMUTE	0x01	/* permute non-options to the end of argv */
#define FLAG_ALLARGS	0x02	/* treat non-options as args to option "-1" */
#define FLAG_LONGONLY	0x04	/* operate as getopt_long_only */

/* return values */
#define	BADCH		(int)'?'
#define	BADARG		((*options == ':') ? (int)':' : (int)'?')
#define	INORDER 	(int)1

#ifndef __CYGWIN__
#define __progname __argv[0]
#else
	extern char __declspec(dllimport) *__progname;
#endif

#ifdef __CYGWIN__
	static char EMSG[] = "";
#else
#define	EMSG		""
#endif
extern int optind;		/* index of first non-option in argv      */
extern int optopt;		/* single option character, as parsed     */
extern int opterr;		/* flag to enable built-in diagnostics... */
				/* (user may set to zero, to suppress)    */

extern char *optarg;		/* pointer to argument of current option  */

extern int getopt(int nargc, char * const *nargv, const char *options);

static int getopt_internal(int, char * const *, const char *,
                           const struct option *, int *, int);
static int gcd(int, int);
static void permute_args(int, int, int, char * const *);

static void
_vwarnx(const char *fmt, va_list ap)
{
    (void)fprintf(stderr, "%s: ", __progname);
    if (fmt != NULL)
	(void)vfprintf(stderr, fmt, ap);
    (void)fprintf(stderr, "\n");
}

static void
warnx(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    _vwarnx(fmt, ap);
    va_end(ap);
}

/*
* Compute the greatest common divisor of a and b.
*/
static int
gcd(int a, int b)
{
    int c;

    c = a % b;
    while (c != 0) {
	    a = b;
	    b = c;
	    c = a % b;
    }

    return (b);
}

/*
* Exchange the block from nonopt_start to nonopt_end with the block
* from nonopt_end to opt_end (keeping the same order of arguments
* in each block).
*/
static void
permute_args(int panonopt_start, int panonopt_end, int opt_end, char * const *nargv)
{
    int cstart, cyclelen, i, j, ncycle, nnonopts, nopts, pos;
    char *swap;

    /*
    * compute lengths of blocks and number and size of cycles
    */
    nnonopts = panonopt_end - panonopt_start;
    nopts = opt_end - panonopt_end;
    ncycle = gcd(nnonopts, nopts);
    cyclelen = (opt_end - panonopt_start) / ncycle;

    for (i = 0; i < ncycle; i++) {
	cstart = panonopt_end + i;
	pos = cstart;
	for (j = 0; j < cyclelen; j++) {
	    if (pos >= panonopt_end)
		pos -= nnonopts;
	    else
		pos += nopts;
	    swap = nargv[pos];
	    /* LINTED const cast */
	    ((char **)nargv)[pos] = nargv[cstart];
	    /* LINTED const cast */
	    ((char **)nargv)[cstart] = swap;
	}
    }
}


#ifdef _BSD_SOURCE
/*
 * BSD adds the non-standard `optreset' feature, for reinitialisation
 * of `getopt' parsing.  We support this feature, for applications which
 * proclaim their BSD heritage, before including this header; however,
 * to maintain portability, developers are advised to avoid it.
 */
# define optreset  __mingw_optreset
extern int optreset;
#endif
#ifdef __cplusplus
}
#endif
/*
 * POSIX requires the `getopt' API to be specified in `unistd.h';
 * thus, `unistd.h' includes this header.  However, we do not want
 * to expose the `getopt_long' or `getopt_long_only' APIs, when
 * included in this manner.  Thus, close the standard __GETOPT_H__
 * declarations block, and open an additional __GETOPT_LONG_H__
 * specific block, only when *not* __UNISTD_H_SOURCED__, in which
 * to declare the extended API.
 */
#endif /* !defined(__GETOPT_H__) */
