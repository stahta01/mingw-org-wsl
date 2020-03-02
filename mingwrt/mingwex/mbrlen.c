/*
 * mbrlen.c
 *
 * MinGW.org replacement for the ISO-C99 mbrlen() function; delegates to a
 * Microsoft implementation, if available in the C runtime DLL, (unless this
 * is overridden by user choice); otherwise handles the call locally.
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

static size_t __mingw_mbrlen_fallback
( const char *restrict s, size_t n, mbstate_t *restrict ps )
{
  /* Fallback function, providing an implementation of the mbrlen()
   * function, when none is available within the Microsoft C runtime,
   * or the user has explicitly overridden accessibility of any such
   * Microsoft implementation.
   *
   * This is simply delegated to common handler, for both mbrlen(),
   * and mbrtowc() fallback functions.
   */
  return __mingw_mbrtowc_handler( NULL, s, n, __mbrtowc_state( ps ) );
}

size_t __mingw_mbrlen
( const char *restrict s, size_t n, mbstate_t *restrict ps )
{
  /* Wrapper for the mbrlen() function; this will unconditionally
   * delegate the call to the MinGW fallback implementation, (defined
   * above), irrespective of availability of any Microsoft-provided
   * implementation.
   */
  __mingw_mbrlen_cur_max_init( __mingw_mbrtowc_codeset_init() );
  return __mingw_mbrlen_fallback( s, n, ps );
}

size_t __msvcrt_mbrlen
( const char *restrict s, size_t n, mbstate_t *restrict ps )
{
  /* Wrapper for the mbrlen() function; this will initially attempt
   * to delegate the call to a Microsoft-provided implementation, but
   * if no such implementation can be found, fall back to the MinGW
   * substitute (defined above).
   */
  typedef size_t (*redirector_t)
  ( const char *restrict, size_t, mbstate_t *restrict );
  static redirector_t redirector_hook = NULL;

  /* MSVCRT.DLL's setlocale() cannot reliably handle code pages with
   * more than two bytes per code point, (e.g. UTF-7 and UTF-8); thus,
   * Microsoft's mbrlen() is likely to be similarly unreliable, so
   * always use the MinGW fallback with such code pages.
   */
  if( __mingw_mbrlen_cur_max_init( __mingw_mbrtowc_codeset_init() ) > 2 )
    return __mingw_mbrlen_fallback( s, n, ps );

  /* On first time call, we don't know which implementation is to be
   * selected; look for a Microsoft implementation, which, if available,
   * may be registered for immediate use on this, and any subsequent,
   * calls to this function wrapper...
   */
  if(  (redirector_hook == NULL)
  &&  ((redirector_hook = dlsym( RTLD_DEFAULT, "mbrlen" )) == NULL)  )

    /* ...but when no Microsoft implementation can be found, register
     * the MinGW fall back in its stead.
     */
    redirector_hook = __mingw_mbrlen_fallback;

  /* Finally, delegate the call to whichever implementation has been
   * registered on first-time call.
   */
  return redirector_hook( s, n, ps );
}

/* $RCSfile$: end of file */
