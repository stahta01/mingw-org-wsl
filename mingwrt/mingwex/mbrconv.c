/*
 * mbrconv.c
 *
 * Implementation of back-end MBCS to wchar_t conversion infrastructure
 * routines to support the MinGW mbrlen(), and mbrtowc() functions.
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

/* We use the MB_LEN_MAX macro, to declare the size of internal
 * MBCS storage buffers; it is defined in <limits.h>
 */
#include <limits.h>

static __mb_inline__ size_t mbrlen_min
( const char *restrict s, size_t n, wchar_t *restrict wc )
{
  /* Internal helper function to determine the minimum number of
   * initial bytes, within a MBCS sequence, which are required to
   * represent a single wide character code point, (which may be
   * represented as a single wchar_t entity, or alternatively as
   * a surrogate pair of two such wchar_t entities).  At most,
   * the routine will examine the initial "n" bytes of the given
   * MBCS sequence, (with "n" nominally limited to the effective
   * MB_CUR_MAX for the specified codeset).
   */
  int status, len = 1;
  do { status = __mingw_mbtowc_convert( s, len, wc, 2 );
     } while( (status == 0) && (n >= ++len) );
  return len;
}

size_t __mingw_mbrtowc_handler
( wchar_t *restrict pwc, const char *restrict s, size_t n,
  mbstate_t *restrict ps
)
{ /* Common handler for MinGW mbrtowc() and mbrlen() functions.
   */
  (void)(__mingw_mbrlen_cur_max_init( __mingw_mbrtowc_codeset_init() ));
  { union { mbstate_t st; wchar_t wc[2]; } retval;
    union { mbstate_t st; char mb[MB_LEN_MAX]; wchar_t wc[2]; } state = { *ps };
    unsigned int mbrlen_cur_max = __mingw_mbrlen_cur_max();
    size_t pending, len = 0, count = 0;

    /* Any residual state, from a preceding call, has been captured
     * in the local "state" union; assume that this call will clear
     * any such state, leaving no further residual.
     */
    *ps = (mbstate_t)(0);

    /* Normally, it makes no sense to call mbrlen(), or mbrtowc(),
     * with a look-ahead byte count limit of zero; however, due to
     * the constraints imposed by MS-Windows using UTF-16LE as the
     * underlying encoding for wchar_t...
     */
    if( n == 0 )
    { /* ...we allow this, as a special case, so that, when any
       * immediately preceding call to mbrtowc() has returned a
       * high surrogate, the accompanying low surrogate...
       */
      if( IS_SURROGATE_PAIR( state.wc[0], state.wc[1] ) )
      {
	/* ...may be returned to the caller, without consuming
	 * any further bytes from the original MBCS sequence.
	 */
	if( pwc != NULL ) *pwc = state.wc[1];
	return (size_t)(0);
      }
      /* When the conversion state does not represent a deferred
       * low surrogate, then restore it, and pass this through as
       * an effective no-op.
       */
      *ps = state.st;
      return (size_t)(-2);
    }
    /* In any context, other than the preceding (special) n == 0
     * case, for retrieval of a deferred low surrogate, a pending
     * conversion state which represents a surrogate pair is not
     * a valid state; reject it.
     */
    if( IS_SURROGATE_PAIR( state.wc[0], state.wc[1] ) )
      return errout( EINVAL, (size_t)(-1) );

    /* Step over any pending MBCS bytes, which may already be
     * present within the conversion state buffer, accumulating
     * both the count of such pending bytes, together with a
     * partial count of total bytes for conversion.
     */
    while( (len < sizeof( mbstate_t )) && (state.mb[len] != '\0') )
      ++len;
    pending = len;

    /* Append MBCS bytes from the input sequence, to the pending
     * state buffer, up to the specified look-ahead count limit, or
     * until the filled length of the buffer becomes equivalent to
     * the effective value of MB_CUR_MAX.
     */
    while( (len < mbrlen_cur_max) && (count < n) && (s[count] != '\0') )
      state.mb[len++] = s[count++];

    /* If the pending look-ahead state has not yet been padded
     * to the full MB_CUR_MAX length, ensure that it is encoded
     * as a NUL terminated MBCS sequence, before attempting to
     * interpret it as a complete MBCS sequence.
     */
    if( len < mbrlen_cur_max ) state.mb[len] = '\0';
    if( (int)(count = mbrlen_min( state.mb, len, retval.wc )) > 0 )
    {
      /* No valid conversion state should ever exist, where no
       * additional bytes are required to complete a previously
       * deferred multibyte character.
       */
      if( pending >= count ) return errout( EILSEQ, (size_t)(-1) );

      /* The accumulated encoding state does now represent a
       * complete MBCS sequence; when servicing an mbrtowc() call,
       * with non-NULL return value pointer, we must store that
       * return value...
       */
      if( pwc != NULL )
      { /* ...noting that, under MS-Windows, we may not be able
	 * to accommodate the entire converted value in a single
	 * UTF-16 wchar_t, in which case we must return it as a
	 * surrogate pair, of which only the high surrogate can
	 * be returned now...
	 */
	if( IS_HIGH_SURROGATE( *pwc = retval.wc[0] ) )
	  /* ...with the entire pair being stored at the passed
	   * mbstate_t reference buffer, allowing for subsequent
	   * retrieval of the low surrogate.
	   */
	  *ps = retval.st;
      }
      /* In the case that the wchar_t return value represents a
       * NUL character, ISO-C99 prescribes that, whichever of the
       * supported functions is being serviced, the returned byte
       * count, of converted MBCS bytes, must be zero.
       */
      if( retval.wc[0] == L'\0' ) return (size_t)(0);

      /* The effective function return value, for this case, is
       * the count of bytes accumulated into the completed MBCS
       * byte sequence, discounting those which were deferred
       * from any preceding call.
       */
      return (count - pending);
    }
    else if( count < mbrlen_cur_max )
    { /* The accumulated encoding state does not represent a
       * complete, and valid MBCS sequence, but we have not yet
       * accumulated as many bytes as the effective MB_CUR_MAX
       * length can accommodate; save the encoding state for
       * deferred reprocessing, and return the appropriate
       * pseudo-count to inform the caller that this encoding
       * state may yet develop into a valid MBCS sequence.
       */
      *ps = retval.st;
      return (size_t)(-2);
    }
  }
  /* If neither of the preceding encoding states prevails, then
   * the current state must represent an invalid MBCS sequence;
   * report it via errno, and appropriate return value.
   */
  return errout( EILSEQ, (size_t)(-1) );
}

/* $RCSfile$: end of file */
