/* vsnprintf.c
 *
 * $Id$
 *
 * Provides an implementation of the "vsnprintf" function, conforming
 * generally to C99 and SUSv3/POSIX specifications, with extensions
 * to support Microsoft's non-standard format specifications.  This
 * is included in libmingwex.a, replacing the redirection through
 * libmoldnames.a, to the MSVCRT standard "_vsnprintf" function; (the
 * standard MSVCRT function remains available, and may  be invoked
 * directly, using this fully qualified form of its name).
 *
 * Written by Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2008, 2019, MinGW.org Project
 *
 * This replaces earlier, substantially different implementations,
 * originally provided as vsnprintf.c, and later encapsulated within
 * gdtoa/mingw_snprintf.c:
 *
 * Written by Danny Smith <dannysmith@users.sourceforge.net>
 * Copyright (C) 2002, 2003, 2007, 2008, MinGW.org Project
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
#include <stdarg.h>
#include <stddef.h>

#include "pformat.h"

int __cdecl __vsnprintf (char *, size_t, const char *fmt, va_list) __MINGW_NOTHROW;
int __cdecl __mingw_alias(vsnprintf) (char *, size_t, const char *, va_list) __MINGW_NOTHROW;

int __cdecl __vsnprintf( char *buf, size_t length, const char *fmt, va_list argv )
{
  register int retval;

  if( length == (size_t)(0) )
    /*
     * No buffer; simply compute and return the size required,
     * without actually emitting any data.
     */
    return __pformat( 0, buf, 0, fmt, argv );

  /* If we get to here, then we have a buffer...
   * Emit data up to the limit of buffer length less one,
   * then add the requisite NUL terminator.
   */
  retval = __pformat( 0, buf, --length, fmt, argv );
  buf[retval < length ? retval : length] = '\0';

  return retval;
}

/* $RCSfile$: end of file */
