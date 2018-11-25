/*
 * fseeki64.c
 *
 * Provides a fall-back implementation of Microsoft's _fseeki64() function,
 * suitable for deployment when linking with legacy MSVCRT.DLL versions, from
 * which this API is not exported.
 *
 *
 * $Id$
 *
 * Written by Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2018, MinGW.org Project
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */
#include <io.h>
#include <stdio.h>

int __cdecl __mingw_fseeki64( FILE *stream, __int64 offset, int whence )
{
  /* Emulate _fseeki64() on the basis of the underlying OS data stream
   * pointer, as manipulated by the _lseeki64() function, (which, unlike
   * the _fseeki64() function, has been exported from all known versions
   * of MSVCRT.DLL).  Note that, unlike a previous MinGW implementation of
   * the effectively equivalent fseeko64() function, this does not rely on
   * any undocumented assumptions regarding the (opaque) content of fpos_t
   * data, returned by the fgetpos() function; however, it does first use
   * fgetpos(), followed by fsetpos(), without moving the FILE stream
   * pointer, to ensure that the internal buffer associated with the FILE
   * stream is marked as "clean", and thus that the FILE stream pointer
   * is synchronized with the underlying OS data stream pointer, before
   * calling _lseeki64() to adjust the latter; (this has the effect of
   * keeping the two pointers synchronized, following the adjustment
   * resulting from the _lseeki64() call).
   */
  fpos_t pos;
  return ((fgetpos( stream, &pos ) == 0) && (fsetpos( stream, &pos ) == 0))
    ? ((_lseeki64( _fileno( stream ), offset, whence ) == -1LL) ? -1 : 0)
    : -1;
}

/* Since __int64 and __off64_t are effectively congruent 64-bit integer
 * types, the preceding implementation is also suitable as an implementation
 * for a variation of the POSIX.1 fseeko() function, in which the offset is
 * specified in terms of the __off64_t data type.
 */
int __cdecl fseeko64
( FILE *, __off64_t, int )__attribute__((alias("__mingw_fseeki64")));

/* $RCSfile$: end of file */
