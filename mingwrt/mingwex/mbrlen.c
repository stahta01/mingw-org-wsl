/*
 * mbrlen.c
 *
 * MinGW.org replacement for the ISO-C99 mbrlen() function, supporting its
 * use on any legacy Windows version for which Microsoft does not provide it,
 * while replacing the Microsoft implementation on those Windows versions
 * for which it is provided.
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

size_t mbrlen( const char *restrict s, size_t n, mbstate_t *restrict ps )
{
  /* Implementation of ISO-C99 mbrlen() function, in libmingwex.a;
   * this is simply delegated to the common handler, which services
   * both the mbrlen(), and mbrtowc() functions.
   */
  return __mingw_mbrtowc_handler( NULL, s, n, __mbrtowc_state( ps ) );
}

/* FIXME: these aliases are provided for link-compatibitity with
 * libraries compiled against mingwrt-5.3.x; they may be removed
 * from future versions of mingwrt.
 */
size_t __mingw_mbrlen( const char *restrict, size_t, mbstate_t *restrict )
__attribute__((__weak__,__alias__("mbrlen")));

size_t __msvcrt_mbrlen( const char *restrict, size_t, mbstate_t *restrict )
__attribute__((__weak__,__alias__("mbrlen")));

/* $RCSfile$: end of file */
