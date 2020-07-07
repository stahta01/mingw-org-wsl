/*
 * wcsrtombs.c
 *
 * MinGW.org implementation of the wcsrtombs() function; supports use of
 * this function on any legacy Windows version, for which Microsoft do not
 * provide it, and replaces the Microsoft implementation, when they do.
 *
 *
 * $Id: wcsrtombs.c,v 28b17d1c4eab 2020/07/07 21:02:51 keith $
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

static __mb_inline__ size_t __mingw_wcsrtombs_internal
( char *restrict mbs, const wchar_t **restrict wcs, size_t len,
  mbstate_t *restrict ps
)
{ /* Internal implementation of the wcsrtombs() function; this will be
   * expanded inline, within the body of the public implementation.
   *
   * Initially, save the current errno state, so that we may restore
   * it on return, clear it to zero for internal checking, and compute
   * the size of buffer required to accommodate the conversion.
   */
  int errno_reset = save_error_status_and_clear( errno, 0 );
  union { mbstate_t ps; wchar_t wc[2]; } resume = { (mbstate_t)(0) };
  size_t count = (size_t)(0), wanted = (size_t)(0);

  /* This wcsrtombs() implementation will not use any mbstate...
   */
  if( ps != NULL )
  { /* ...unless it is provided by the caller, in which case we will,
     * ultimately, reset it to initial state, after processing it...
     */
    resume.ps = *ps;
    *ps = (mbstate_t)(0);
    if( IS_SURROGATE_PAIR( resume.wc[0], **wcs ) )
    { /* ...subject to the expectation that it represents deferred
       * completion of a surrogate pair.
       */
      resume.wc[1] = *(*wcs)++;
      count = __mingw_wctomb_convert( NULL, 0, resume.wc, 2 );
    }
  }

  /* The total buffer space wanted is the aggregate of any deferred
   * surrogate pair completion, plus the contribution from conversion
   * of the remainder of the wide character string.
   */
  wanted = count + __mingw_wctomb_convert( NULL, 0, *wcs, -1 );

  if( mbs == NULL )
    /* There is no buffer designated to store the encoded multibyte
     * character sequence; we are only interested in the size of the
     * buffer which would otherwise be required, and we've already
     * determined that, so simply return it.
     */
    return (errno == 0) ? errout( errno_reset, wanted - 1 ) : wanted;

  if( (errno == 0) && (len >= wanted) )
  { /* There is an encoding buffer designated, its size is sufficient
     * to accommodate the encoding of the entire NUL terminated input
     * sequence, and there was no incipient encoding error during the
     * initial minimum buffer size determination; encode the entire
     * input sequence for return, and clean up the input state.
     */
    if( count != (size_t)(0) )
      mbs += __mingw_wctomb_convert( mbs, len, resume.wc, 2 );
    count += __mingw_wctomb_convert( mbs, len - count, *wcs, -1 ) - 1;
    *wcs = NULL;
  }

  else
  { /* There is an encoding buffer designated, but either it is too
     * small, or a incipient encoding error has been detected; rescan
     * the input sequence, encoding one code point at a time, until we
     * either exhaust the encoding buffer space, or we encounter the
     * encoding error previously identified.
     */
    errno = 0;

    /* Initially, if there's a pending surrogate completion, and there
     * is insufficient buffer space to accommodate its conversion, then
     * we must squash all conversion...
     */
    if( count > len ) count = len = 0;
    else if( count != 0 )
    { /* ...otherwise, we store the completed surrogate conversion, at
       * the start of the buffer, adjusting the buffer pointer, and its
       * residual length counter, to suit.
       */
      mbs += __mingw_wctomb_convert( mbs, len, resume.wc, 2 );
      len -= count;
    }
    while( (len >= __mingw_wctomb_convert( NULL, 0, *wcs, 1 )) && (errno == 0) )
    {
      /* There is still sufficient space to store the encoding of one
       * more input code point, and we haven't yet fallen foul of any
       * incipient encoding error; store this encoding, and adjust to
       * prepare for the next.
       */
      size_t step = __mingw_wctomb_convert( mbs, len, (*wcs)++, 1 );
      count += step; len -= step; mbs += step;
    }

    /* Check that we didn't fall foul of any incipient encoding error;
     * if we did, then we must bail out.
     */
    if( errno != 0 ) return (size_t)(-1);
  }
  /* We have now successfully encoded as much of the input sequence
   * as possible, without encountering any encoding error; restore
   * the saved errno state, and return the encoded byte count.
   */
  return errout( errno_reset, count );
}

size_t wcsrtombs( char *mbs, const wchar_t **wcs, size_t len, mbstate_t *ps )
{
  /* Implementation of ISO-C99 wcsrtombs() function, in libmingwex.a;
   * before proceeding, we must ensure that the wcs argument specifies
   * an indirect reference to a non-NULL wchar_t array.
   */
  if( (wcs == NULL) || (*wcs == NULL) ) return errout( EINVAL, (size_t)(-1) );

  /* With a valid wcs reference, store the effective codeset, and
   * hand the conversion off to the inline expansion of the preceding
   * implementation.
   */
  (void)(__mingw_wctomb_codeset_init() );
  return __mingw_wcsrtombs_internal( mbs, wcs, len, ps );
}

/* $RCSfile: wcsrtombs.c,v $: end of file */
