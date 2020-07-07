/*
 * btowc.c
 *
 * Implementation of an ISO-C99 conforming btowc() function; note that,
 * since this considers only one byte for conversion, and a single byte
 * can never convert to a surrogate pair, this is not susceptible to the
 * potential wchar_t overflow error, which may occur with functions such
 * as mbrtowc(), which may need to return surrogate pairs.
 *
 *
 * $Id: btowc.c,v 28b17d1c4eab 2020/07/07 21:02:51 keith $
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

/* We also need <stdio.h>, for EOF.
 */
#include <stdio.h>

wint_t btowc( int c )
{ /* Implementation of ISO-C99 btowc() function, in libmingwex.a;
   * this performs an MBCS to wchar_t conversion on the given single
   * character argument, (expressed as an int), returning WEOF in
   * the event that conversion fails.
   */
  if( c != EOF )
  { wint_t wc_result;
    (void)(__mingw_mbrtowc_codeset_init());
    if( __mingw_mbtowc_convert( (char *)(&c), 1, &wc_result, 1) == 1 )
      return wc_result;
  }
  return WEOF;
}

/* $RCSfile: btowc.c,v $: end of file */
