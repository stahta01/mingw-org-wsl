/*
 * wcrtomb.c
 *
 * MinGW.org replacement for the wcrtomb() function; delegates to the
 * Microsoft implementation, if available in the C runtime DLL, otherwise
 * handles the call locally.
 *
 * $Id$
 *
 * Written by Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2019, 2020, MinGW.org Project
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

static __mb_inline__ size_t __wcrtomb_fallback
( char *restrict mb, wchar_t wc, mbstate_t *ps )
{
  /* Fallback function, providing an implementation of the wcrtomb()
   * function, when none is available within the Microsoft C runtime,
   * or the user has explicitly overridden accessibility of any such
   * Microsoft implementation.
   */
  if( *ps != (mbstate_t)(0) )
  {
    /* Microsoft's MBCS implementation does not use shift states;
     * however, it is possible that an immediately preceding call
     * may have stopped with a dangling high surrogate, and thus
     * a restart to complete this, by adding a low surrogate, and
     * converting the pair, may be appropriate.
     */
    if( IS_HIGH_SURROGATE( *ps ) && IS_LOW_SURROGATE( wc ) )
    {
      /* Reassemble the surrogate pair, in a local buffer, and
       * return its conversion, having reset the restart state.
       */
      wchar_t buf[2] = { (wchar_t)(*ps), wc }; *ps = (mbstate_t)(0);
      return __mingw_wctomb_convert( mb, __mingw_wctomb_cur_max(), buf, 2 );
    }
    else
    { /* We expected a low surrogate, but didn't get one; reset
       * the restart state, and abort this conversion.
       */
      *ps = (mbstate_t)(0); return errout( EILSEQ, (size_t)(-1) );
    }
  }
  /* When mb is a NULL pointer, ISO-C99 decrees that the call shall
   * be interpreted as the equivalent of:
   *
   *   wcrtomb( internal_buffer, L'\0', ps );
   *
   * with the encoding of the NUL wchar, preceded by any sequence
   * of bytes needed to restore ps to the initial shift state, being
   * stored in the internal buffer, (and thus, inaccessible to the
   * caller).  Since Microsoft's MBCS encodings do not use shift
   * states, and the encoding for NUL is always a single NUL byte,
   * this becomes the equivalent of returning (size_t)(1).
   */
  if( mb == NULL ) return (size_t)(1);

  /* When mb is not a NULL pointer, then we are obliged to assume
   * that it points to a buffer of at least MB_CUR_MAX bytes, and
   * we may proceed with a normal conversion, (except that, when
   * wc lies in the range reserved for surrogates, we must handle
   * it as a special case.
   */
  if( IS_HIGH_SURROGATE( wc ) )
  { /* A high surrogate is permitted, but it cannot be converted
     * at this time; instead, we simply record that it is present,
     * (subverting ps for this purpose), and move on, without any
     * conversion being performed, and thus storing no converted
     * bytes, in the expection that the next wc passed will be a
     * low surrogate, thus allowing completion of the conversion.
     */
    *ps = (mbstate_t)(wc); return (size_t)(0);
  }
  if( IS_LOW_SURROGATE( wc ) )
    /* A low surrogate, detected here, is an orphan (not paired
     * with a high surrogate from an immediately preceding call);
     * this is not permitted, so report it as invalid.
     */
    return errout( EILSEQ, (size_t)(-1) );

  /* If we get this far, we may proceed with conversion; we return
   * the byte count, and effect of encoding the single wchar which
   * was passed by value in wc.
   */
  return __mingw_wctomb_convert( mb, __mingw_wctomb_cur_max(), &wc, 1 );
}

static size_t __mingw_wcrtomb_fallback
( char *restrict mb, wchar_t wc, mbstate_t *ps )
{
  /* A thin wrapper around the preceding fallback implementation,
   * (which is expanded in-line); this serves as the sole interface
   * between either of the two following public API entry points, and
   * the fallback implementation, ensuring that a private mbstate_t
   * reference is provided, if the caller doesn't supply its own.
   */
  return __wcrtomb_fallback( mb, wc, __mbrtowc_state( ps ) );
}

size_t __mingw_wcrtomb
( char *restrict mb, wchar_t wc, mbstate_t *restrict ps )
{
  /* Wrapper for the wcrtomb() function; this will unconditionally
   * delegate the call to the MinGW fallback implementation, (defined
   * above), irrespective of availability of any Microsoft-provided
   * implementation.
   */
  (void)(__mingw_wctomb_cur_max_init( __mingw_wctomb_codeset_init() ));
  return __mingw_wcrtomb_fallback( mb, wc, ps );
}

size_t __msvcrt_wcrtomb( char *restrict mb, wchar_t wc, mbstate_t *restrict ps )
{
  /* Wrapper for the wcrtomb() function; this will initially attempt
   * to delegate the call to a Microsoft-provided implementation, but
   * if no such implementation can be found, fall back to the MinGW
   * substitute (defined above).
   */
  typedef size_t (*redirect_t)( char *restrict, wchar_t, mbstate_t *restrict );
  static redirect_t redirector_hook = NULL;

  /* MSVCRT.DLL's setlocale() cannot reliably handle code pages with
   * more than two bytes per code point, (e.g. UTF-7 and UTF-8); thus,
   * Microsoft's wcrtomb() is likely to be similarly unreliable, so
   * always use the MinGW fallback with such code pages.
   */
  if( (__mingw_wctomb_cur_max_init( __mingw_wctomb_codeset_init() )) > 2 )
    return __mingw_wcrtomb_fallback( mb, wc, ps );

  /* On first time call, we don't know which implementation is to be
   * selected; look for a Microsoft implementation, which, if available,
   * may be registered for immediate use on this, and any subsequent,
   * calls to this function wrapper...
   */
  if(  (redirector_hook == NULL)
  &&  ((redirector_hook = dlsym( RTLD_DEFAULT, "wcrtomb" )) == NULL)  )

    /* ...but when no Microsoft implementation can be found, register
     * the MinGW fall back in its stead.
     */
    redirector_hook = __mingw_wcrtomb_fallback;

  /* Finally, delegate the call to whichever implementation has been
   * registered on first-time call.
   */
  return redirector_hook( mb, wc, ps );
}

/* $RCSfile$: end of file */
