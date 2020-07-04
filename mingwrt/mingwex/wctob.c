/*
 * wctob.c
 *
 * Implementation of ISO-C99 wctob() function, supporting it on legacy
 * Windows versions, for which MSVCRT.DLL doesn't provide it, and also
 * replacing the Microsoft implementation, on Windows versions with an
 * MSVCRT.DLL, or MSVCRn.DLL which does.
 *
 *
 * $Id$
 *
 * Written by Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2020, MinGW.org Project
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
#include "wcharmap.h"

#include <stdio.h>

int wctob( wint_t wc )
{ /* Implementation of ISO-C99 wctob() function, in libmingwex.a;
   * after first storing the effective codeset index, this performs
   * a wchar_t to MBCS conversion on the given single wide character
   * argument, capturing the conversion into a local buffer, checks
   * that the result occupies exactly one byte, for which the byte
   * value is coerced to int and returned; otherwise returns EOF.
   */
  (void)(__mingw_wctomb_codeset_init());
  union { unsigned char u; char c; } retval;
  return (__mingw_wctomb_convert( &retval.c, 1, &wc, 1 ) == 1)
    ? (int)(retval.u) : EOF;
}

/* FIXME: these aliases are provided for link-compatibitity with
 * libraries compiled against mingwrt-5.3.x; they may be removed
 * from future versions of mingwrt.
 */
int __msvcrt_wctob( wint_t )__attribute__((__weak__,__alias__("wctob")));
int __mingw_wctob( wint_t )__attribute__((__weak__,__alias__("wctob")));

/* $RCSfile$: end of file */
