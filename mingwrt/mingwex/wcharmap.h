/*
 * wcharmap.h
 *
 * Private header file, declaring common components of the MinGW.org
 * fallback implementations of wide to multi-byte (and complementary)
 * character set conversion API functions.
 *
 * $Id$
 *
 * Written by Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2019, 2020, MinGW.org Project
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
 * disclaimer shall be included in all copies or substantial portions of
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
#include <wchar.h>
#include <winnls.h>
#include <stdlib.h>
#include <errno.h>

/* Provide a shorthand notation for declaring functions which
 * we would like to always be expanded in line.
 */
#define __mb_inline__ __inline__ __attribute__((__always_inline__))

/* Define a pair of inline helper functions, to facilitate preservation
 * of the "errno" state on entry, such that it may be restored or modified,
 * as necessary for ISO-C99 conformance, on function return.
 *
 * First, a helper to save, and clear, error state on entry...
 */
static __mb_inline__
int save_error_status_and_clear (int state, int clear)
{ errno = clear; return state; }

/* ...and the complementary helper, which may be used to either restore
 * the saved state, or to report a new error condition, on return.
 */
static __mb_inline__
size_t errout (int errcode, size_t status){ errno = errcode; return status; }

/* Generic codeset management functions.
 */
unsigned int __mb_codeset_for_locale (void);
unsigned int __mb_cur_max_for_codeset (unsigned int);

/* Codeset initializers, and internal helper functions for
 * wide character to multi-byte sequence conversions.
 */
unsigned int __mingw_wctomb_codeset_init (void);
unsigned int __mingw_wctomb_cur_max_init (unsigned int);
size_t __mingw_wctomb_convert (char *, int, const wchar_t *, int);
unsigned int __mingw_wctomb_cur_max (void);

/* The legacy MinGW implementation used a get_codepage() function,
 * which was effectively the same as our __mb_codeset_for_locale();
 * this alias may, eventually, become redundant.
 */
static __mb_inline__
unsigned int get_codepage( void ){ return __mb_codeset_for_locale(); }

/* A private helper function, to furnish an internal conversion state
 * buffer, for use in any case where a conversion function was called,
 * and the caller didn't provide one.
 */
static __mb_inline__
mbstate_t *__mbrtowc_state( mbstate_t *reference_state )
{
  static mbstate_t internal_state = (mbstate_t)(0);
  return (reference_state == NULL) ? &internal_state : reference_state;
}

/* $RCSfile$: end of file */
