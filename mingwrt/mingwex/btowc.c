/*
 * btowc.c
 *
 * Implementation of an ISO-C99 conforming btowc() function; note that,
 * since this considers only one byte for conversion, and a single byte
 * can never convert to a surrogate pair, this is not susceptible to the
 * potential wchar_t overflow error, which may occur with functions such
 * as mbrtowc(), which may need to return surrogate pairs.
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

/* For runtime delegation, we need a mechanism for detection of an
 * implementation, within the default C runtime DLL; we may use the
 * MinGW dlfcn emulation, to facilitate this.
 */
#include <dlfcn.h>

/* We also need <stdio.h>, for EOF.
 */
#include <stdio.h>

static wint_t __mingw_btowc_fallback( int c )
{ /* Fallback function, providing an implementation of the btowc()
   * function, when none is available within the Microsoft runtime.
   * This performs an MBCS to wchar_t conversion on the given single
   * character argument, (expressed as an int), returning WEOF in
   * the event that conversion fails.
   */
  if( c != EOF )
  { wint_t wc_result;
    if( __mingw_mbtowc_convert( (char *)(&c), 1, &wc_result, 1) == 1 )
      return wc_result;
  }
  return WEOF;
}

wint_t __mingw_btowc( int c )
{ /* Wrapper for the btowc() function; this will unconditionally
   * delegate the call to the MinGW fallback implementation, (as
   * implemented above), after initialization of the effective
   * codeset file-global variable.
   */
  (void)(__mingw_mbrtowc_codeset_init());
  return __mingw_btowc_fallback( c );
}

wint_t __msvcrt_btowc( int c )
{ /* Wrapper for the btowc() function; it will initially attempt
   * to delegate the call to a Microsoft-provided implementation,
   * but if no such implementation can be found, fall back to the
   * MinGW substitute (defined above).
   */
  static wint_t (*redirector_hook)( int ) = NULL;

  /* MSVCRT.DLL's setlocale() cannot reliably handle code pages with
   * more than two bytes per code point, (e.g. UTF-7 and UTF-8); thus,
   * Microsoft's btowc() is likely to be similarly unreliable, so we
   * always use the MinGW fallback with such code pages.
   */
  if( __mb_cur_max_for_codeset(__mingw_mbrtowc_codeset_init()) > 2 )
    return __mingw_btowc_fallback( c );

  /* On first time call, we don't know which implementation is to be
   * selected; look for a Microsoft implementation, which, if available,
   * may be registered for immediate use on this, and any subsequent,
   * calls to this function wrapper...
   */
  if(  (redirector_hook == NULL)
  &&  ((redirector_hook = dlsym( RTLD_DEFAULT, "btowc" )) == NULL)  )

    /* ...but when no Microsoft implementation can be found, register
     * the MinGW fall back in its stead.
     */
    redirector_hook = __mingw_btowc_fallback;

  /* Finally, delegate the call to whichever implementation has been
   * registered on first-time call.
   */
  return redirector_hook( c );
}

/* $RCSfile$: end of file */
