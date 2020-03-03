/*
 * wctob.c
 *
 * Implementation of ISO-C99 wctob() function, supporting it on legacy
 * Windows versions, for which MSVCRT.DLL doesn't provide it, otherwise
 * delegating to the Microsoft implementation, except in specific cases
 * when that implementation may not support the active MBCS codeset.
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

/* For runtime delegation, we need a mechanism for detection of an
 * implementation, within the default C runtime DLL; we may use the
 * MinGW dlfcn emulation, to facilitate this.
 */
#include <dlfcn.h>
#include <stdio.h>

static int __mingw_wctob_fallback( wint_t wc )
{ /* Fallback function, providing an implementation of the wctob()
   * function, when none is available within the Microsoft runtime.
   * This performs a wchar_t to MBCS conversion on the given single
   * wide character argument, capturing the conversion into a local
   * buffer, checks that the result occupies exactly one byte, for
   * which the coercion of that byte value to int is returned, or
   * otherwise returns EOF.
   */
  union { unsigned char u; char c; } retval;
  return (__mingw_wctomb_convert( &retval.c, 1, &wc, 1 ) == 1)
    ? (int)(retval.u) : EOF;
}

int __mingw_wctob( wint_t wc )
{ /* Wrapper for the wctob() function; this variant will unconditionally
   * delegate the call to the MinGW fallback implementation, after first
   * storing the effective codeset index.
   */
  (void)(__mingw_wctomb_codeset_init());
  return __mingw_wctob_fallback( wc );
}

int __msvcrt_wctob( wint_t wc )
{ /* Wrapper for the wctob() function; it will initially attempt
   * to delegate the call to a Microsoft-provided implementation,
   * but if no such implementation can be found, fall back to the
   * MinGW substitute (defined above).
   */
  static int (*redirector_hook)( wchar_t ) = NULL;

  /* MSVCRT.DLL's setlocale() cannot reliably handle code pages with
   * more than two bytes per code point, (e.g. UTF-7 and UTF-8); thus,
   * Microsoft's wctob() is likely to be similarly unreliable, so we
   * always use the MinGW fallback with such code pages.
   */
  if( __mingw_wctomb_cur_max_init(__mingw_wctomb_codeset_init()) > 2 )
    return __mingw_wctob_fallback( wc );

  /* On first time call, we don't know which implementation is to be
   * selected; look for a Microsoft implementation, which, if available,
   * may be registered for immediate use on this, and any subsequent,
   * calls to this function wrapper...
   */
  if(  (redirector_hook == NULL)
  &&  ((redirector_hook = dlsym( RTLD_DEFAULT, "wctob" )) == NULL)  )

    /* ...but when no Microsoft implementation can be found, register
     * the MinGW fall back in its stead.
     */
    redirector_hook = __mingw_wctob_fallback;

  /* Finally, delegate the call to whichever implementation has been
   * registered on first-time call.
   */
  return redirector_hook( wc );
}

/* $RCSfile$: end of file */
