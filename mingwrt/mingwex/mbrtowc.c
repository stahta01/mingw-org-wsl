/*
 * mbrtowc.c
 *
 * MinGW.org replacement for the ISO-C99 mbrtowc() function, supporting
 * use of this function on legacy Windows versions, for which Microsoft
 * does not provide it, while replacing the Microsoft implementation on
 * those Windows versions for which it is provided.
 *
 *
 * $Id: mbrtowc.c,v 28b17d1c4eab 2020/07/07 21:02:51 keith $
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

size_t mbrtowc
( wchar_t *restrict pwc, const char *restrict s, size_t n,
  mbstate_t *restrict ps
)
{ /* Implementation of ISO-C99 mbrtowc() function, in libmingwex.a
   *
   * When s is a NULL pointer, ISO-C99 decrees that the call shall
   * be interpreted as the equivalent of:
   *
   *   mbrtowc( NULL, "", 1, ps )
   *
   * with any other supplied values for pwc and n being ignored.
   */
  if( s == NULL ) return mbrtowc( NULL, "", 1, ps );

  /* Otherwise, we simply delegate the the call to the common
   * handler, which implements the action for both the mbrlen()
   * function, and the mbrtowc() function.
   */
  return __mingw_mbrtowc_handler( pwc, s, n, __mbrtowc_state( ps ) );
}

/* $RCSfile: mbrtowc.c,v $: end of file */
