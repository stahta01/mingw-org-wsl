/*
 * ftelli64.c
 *
 * Provides a fall-back implementation of Microsoft's _ftelli64() function,
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

__int64 __cdecl __mingw_ftelli64( FILE *stream )
{
  /* Emulate _ftelli64() on the basis of the underlying OS data stream
   * pointer, as returned by the _telli64() function, (which, unlike the
   * _ftelli64() function, has been exported from all known versions of
   * MSVCRT.DLL).  Note that, unlike a previous MinGW implementation of
   * the effectively equivalent ftello64() function, this does not rely
   * on any undocumented assumptions regarding the content of the opaque
   * fpos_t data, returned by the fgetpos() function; however, it does
   * still require the use of fgetpos(), followed by fsetpos(), without
   * moving the FILE stream pointer, to ensure that the internal buffer
   * associated with the FILE stream is marked as "clean", and thus that
   * the FILE stream pointer is synchronized with the underlying OS data
   * stream pointer, before reading the latter.
   */
  fpos_t pos;
  return ((fgetpos( stream, &pos ) == 0) && (fsetpos( stream, &pos ) == 0))
    ? _telli64( _fileno( stream )) : -1;
}

/* Since return types __int64 and __off64_t are effectively congruent
 * 64-bit integer types, the preceding implementation is also suitable
 * as an implementation for an __off64_t returning variation of the
 * POSIX.1 ftello() function.
 */
__off64_t __cdecl ftello64( FILE * )__attribute__((alias("__mingw_ftelli64")));

/* $RCSfile$: end of file */
