/*
 * malloc.h
 *
 * Declarations for non-standard heap management, and memory allocation
 * functions.  These augment the standard functions, which are declared
 * in <stdlib.h>
 *
 * $Id$
 *
 * Written by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 * Copyright (C) 1997-1999, 2001-2005, 2007, 2018, MinGW.org Project.
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, this permission notice, and the following
 * disclaimer, shall be included in all copies or substantial portions of
 * the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OF OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */
#ifndef _MALLOC_H
#pragma GCC system_header
#define _MALLOC_H

/* All MinGW headers assume that <_mingw.h> is included; including
 * <stdlib.h>, which we also need here, is sufficient to make it so.
 */
#include <stdlib.h>

#ifndef RC_INVOKED

/* Microsoft stipulate that the alloca() API should be defined in this
 * header, whereas GNU specify it in its own dedicated header file; to
 * comply with both, we adopt the GNU stratagem, and then include the
 * GNU style dedicated header file here.
 */
#include "alloca.h"

typedef
struct _heapinfo
{ /* The structure used to control operation, and return information,
   * when walking the heap using the _heapwalk() function.
   */
  int		*_pentry;
  size_t	 _size;
  int		 _useflag;
} _HEAPINFO;

/* Status codes returned by _heapwalk()
 */
#define _HEAPEMPTY		(-1)
#define _HEAPOK 		(-2)
#define _HEAPBADBEGIN		(-3)
#define _HEAPBADNODE		(-4)
#define _HEAPEND		(-5)
#define _HEAPBADPTR		(-6)

/* Values returned by _heapwalk(), in the _HEAPINFO.useflag
 */
#define _FREEENTRY		 (0)
#define _USEDENTRY		 (1)

/* Maximum size permitted for a heap memory allocation request
 */
#define _HEAP_MAXREQ	(0xFFFFFFE0)

_BEGIN_C_DECLS

/* The _heap memory allocation functions are supported on WinNT, but not on
 * Win9X, (on which they always simply set errno to ENOSYS).
 */
_CRTIMP __cdecl __MINGW_NOTHROW  int    _heapwalk (_HEAPINFO *);

_CRTIMP __cdecl __MINGW_NOTHROW  int    _heapchk (void);
_CRTIMP __cdecl __MINGW_NOTHROW  int    _heapmin (void);

_CRTIMP __cdecl __MINGW_NOTHROW  int    _heapset (unsigned int);

_CRTIMP __cdecl __MINGW_NOTHROW  size_t _msize (void *);
_CRTIMP __cdecl __MINGW_NOTHROW  size_t _get_sbh_threshold (void);
_CRTIMP __cdecl __MINGW_NOTHROW  int    _set_sbh_threshold (size_t);
_CRTIMP __cdecl __MINGW_NOTHROW  void  *_expand (void *, size_t);

#ifndef _NO_OLDNAMES
/* Legacy versions of Microsoft runtimes may have supported this alternative
 * name for the _heapwalk() API.
 */
_CRTIMP __cdecl __MINGW_NOTHROW  int     heapwalk (_HEAPINFO *);
#endif	/* !_NO_OLDNAMES */

#if __MSVCRT_VERSION__ >= __MSVCR70_DLL
/* First introduced in non-free MSVCR70.DLL, the following were subsequently
 * made available from MSVCRT.DLL, from the release of WinXP onwards; however,
 * we choose to declare them only for the non-free case, preferring to emulate
 * them, in terms of libmingwex.a replacement implementations, for consistent
 * behaviour across ALL MSVCRT.DLL versions.
 */
_CRTIMP __cdecl __MINGW_NOTHROW
void *_aligned_offset_malloc (size_t, size_t, size_t);

_CRTIMP __cdecl __MINGW_NOTHROW
void *_aligned_offset_realloc (void *, size_t, size_t, size_t);

_CRTIMP __cdecl __MINGW_NOTHROW  void *_aligned_malloc (size_t, size_t);
_CRTIMP __cdecl __MINGW_NOTHROW  void *_aligned_realloc (void *, size_t, size_t);
_CRTIMP __cdecl __MINGW_NOTHROW  void  _aligned_free (void *);

/* Curiously, there are no "calloc()" alike variants of the following pair of
 * "recalloc()" alike functions; furthermore, neither of these is provided by
 * any version of pseudo-free MSVCRT.DLL
 */
_CRTIMP __cdecl __MINGW_NOTHROW
void *_aligned_recalloc (void *, size_t, size_t, size_t);

_CRTIMP __cdecl __MINGW_NOTHROW
void *_aligned_offset_recalloc (void *, size_t, size_t, size_t, size_t);

#endif	/* Non-free MSVCR70.DLL, or later */

/* The following emulations are provided in libmingwex.a; they are suitable
 * for use on any Windows version, irrespective of the limited availability
 * of the preceding Microsoft implementations.
 */
__cdecl __MINGW_NOTHROW
void *__mingw_aligned_malloc (size_t, size_t);

__cdecl __MINGW_NOTHROW
void *__mingw_aligned_offset_malloc (size_t, size_t, size_t);

__cdecl __MINGW_NOTHROW
void *__mingw_aligned_offset_realloc (void *, size_t, size_t, size_t);

__cdecl __MINGW_NOTHROW
void *__mingw_aligned_realloc (void *, size_t, size_t);

__cdecl __MINGW_NOTHROW
void  __mingw_aligned_free (void *);

_END_C_DECLS

#endif	/* ! RC_INVOKED */
#endif	/* !_MALLOC_H: $RCSfile$: end of file */
