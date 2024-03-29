/* $OpenBSD: xmalloc.c,v 1.27 2006/08/03 03:34:42 deraadt Exp $ */
/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * Versions of malloc and friends that check their results, and never return
 * failure (they call fatal if they encounter an error).
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 */

#include "includes.h"

#include <sys/param.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xmalloc.h"
#include "log.h"

#if defined(_TOH_) && defined(_PFDBG_)
#include "odsf.h"
#endif /* _TOH_ && _PFDBG */

void *
#if defined(_TOH_) && defined(_PFDBG_)
xmalloc_dbg(size_t size, const char *fn, int ln)
#else  /* _TOH_ && _PFDBG_ */
xmalloc(size_t size)
#endif /* _TOH_ && _PFDBG */
{
	void *ptr;

	if (size == 0)
		fatal("xmalloc: zero size");
	ptr = malloc(size);
#if defined(_TOH_) && defined(_PFDBG_)
	odsf("%s: %d: xmalloc: size(%u) ret(%p)\n", fn, ln, size, ptr);
#endif /* _TOH_ && _PFDBG */
	if (ptr == NULL)
		fatal("xmalloc: out of memory (allocating %lu bytes)", (u_long) size);
	return ptr;
}

void *
#if defined(_TOH_) && defined(_PFDBG_)
xcalloc_dbg(size_t nmemb, size_t size, const char *fn, int ln)
#else  /* _TOH_ && _PFDBG_ */
xcalloc(size_t nmemb, size_t size)
#endif /* _TOH_ && _PFDBG */
{
	void *ptr;

	if (size == 0 || nmemb == 0)
		fatal("xcalloc: zero size");
	if (SIZE_T_MAX / nmemb < size)
		fatal("xcalloc: nmemb * size > SIZE_T_MAX");
	ptr = calloc(nmemb, size);
#if defined(_TOH_) && defined(_PFDBG_)
	odsf("%s: %d: xcalloc: size(%u) ret(%p)\n", fn, ln, nmemb * size, ptr);
#endif /* _TOH_ && _PFDBG */
	if (ptr == NULL)
		fatal("xcalloc: out of memory (allocating %lu bytes)",
		    (u_long)(size * nmemb));
	return ptr;
}

void *
#if defined(_TOH_) && defined(_PFDBG_)
xrealloc_dbg(void *ptr, size_t nmemb, size_t size, const char *fn, int ln)
#else  /* _TOH_ && _PFDBG_ */
xrealloc(void *ptr, size_t nmemb, size_t size)
#endif /* _TOH_ && _PFDBG */
{
	void *new_ptr;
	size_t new_size = nmemb * size;

	if (new_size == 0)
		fatal("xrealloc: zero size");
	if (SIZE_T_MAX / nmemb < size)
		fatal("xrealloc: nmemb * size > SIZE_T_MAX");
#if defined(_TOH_) && defined(_PFDBG_)
	odsf("%s: %d: xrealloc: ptr(%p) size(%u)\n", fn, ln, ptr, new_size);
#endif /* _TOH_ && _PFDBG */
	if (ptr == NULL)
		new_ptr = malloc(new_size);
	else
		new_ptr = realloc(ptr, new_size);
#if defined(_TOH_) && defined(_PFDBG_)
	odsf("%s: %d: xrealloc: ret(%p)\n", fn, ln, new_ptr);
#endif /* _TOH_ && _PFDBG */
	if (new_ptr == NULL)
		fatal("xrealloc: out of memory (new_size %lu bytes)",
		    (u_long) new_size);
	return new_ptr;
}

void
#if defined(_TOH_) && defined(_PFDBG_)
xfree_dbg(void *ptr, const char *fn, int ln)
#else  /* _TOH_ && _PFDBG_ */
xfree(void *ptr)
#endif /* _TOH_ && _PFDBG */
{
#if defined(_TOH_) && defined(_PFDBG_)
	odsf("%s: %d: xfree: ptr(%p)\n", fn, ln, ptr);
#endif /* _TOH_ && _PFDBG */
	if (ptr == NULL)
		fatal("xfree: NULL pointer given as argument");
	free(ptr);
}

char *
#if defined(_TOH_) && defined(_PFDBG_)
xstrdup_dbg(const char *str, const char *fn, int ln)
#else  /* _TOH_ && _PFDBG_ */
xstrdup(const char *str)
#endif /* _TOH_ && _PFDBG */
{
	size_t len;
	char *cp;

#if defined(_TOH_) && defined(_PFDBG_)
	odsf("%s: %d: xstrdup: ptr(%p)\n", fn, ln, str);
#endif /* _TOH_ && _PFDBG */
	len = strlen(str) + 1;
	cp = xmalloc(len);
	strlcpy(cp, str, len);
	return cp;
}

int
xasprintf(char **ret, const char *fmt, ...)
{
	va_list ap;
	int i;

	va_start(ap, fmt);
	i = vasprintf(ret, fmt, ap);
	va_end(ap);

	if (i < 0 || *ret == NULL)
		fatal("xasprintf: could not allocate memory");

	return (i);
}
