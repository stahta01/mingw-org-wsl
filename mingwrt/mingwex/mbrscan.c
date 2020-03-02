/*
 * mbrscan.c
 *
 * Implementation of the infrastructure routines to support the mbrlen(),
 * and mbrtowc() functions, for use in those applications where Microsoft
 * does not provide adequate support.
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

/* The working codeset, and its associated effective MB_CUR_MAX,
 * are stored with file-scope visibility, to facilitate passing
 * them to individual elements of the implementation...
 */
static __thread unsigned int codeset, mbrlen_cur_max;

/* ...but, in this instance, we also need to provide a mechanism
 * for initializing each of these from the global scope...
 */
unsigned int __mingw_mbrtowc_codeset_init( void )
{ return codeset = __mb_codeset_for_locale(); }

unsigned int __mingw_mbrlen_cur_max_init( unsigned int codeset )
{ return mbrlen_cur_max = __mb_cur_max_for_codeset( codeset ); }

/* ...and also, an accessor to make the effective MB_CUR_MAX
 * available in the global scope.
 */
unsigned int __mingw_mbrlen_cur_max( void )
{ return mbrlen_cur_max; }

int __mingw_mbtowc_convert
( const char *s, size_t n, wchar_t *wc, size_t wmax )
{
  /* Public helper function to determine if a MBCS sequence, of any
   * arbitrary length, may be completely converted to a corresponding
   * wchar_t sequence of specified maximum length, initially subject
   * to the MB_ERR_INVALID_CHARS flag, and subsequently retrying with
   * no flags, in the event that conversion with this flag yields an
   * ERROR_INVALID_FLAGS exception.
   *
   * A maximum of "n" bytes of the given MBCS sequence, "s", will be
   * examined, unless a NUL byte is encountered before "n" bytes have
   * been evaluated; if "n" is specified as zero, it will be ignored,
   * and the full sequence, assumed to be of unlimited length, will
   * be examined until a NUL byte is encountered.
   *
   * Conversion of the given MBCS byte sequence will succeed, only
   * if it represents a whole number of complete and valid code point
   * encodings, and the fully converted representation of all of these
   * code points can be accommodated within "wmax" wchar_t entities;
   * (if "wmax" is specified as zero, it is treated as unlimited).
   *
   * If conversion is successful, the return value is the number of
   * wchar_t entities required to accommodate the fully converted MBCS
   * sequence; if conversion is unsuccessful, zero is returned.
   */
  int st;
  unsigned int flags = MB_ERR_INVALID_CHARS;
  if( n == 0 ) n = (size_t)(-1);

  do { SetLastError( 0 );
       st = MultiByteToWideChar( codeset, flags, s, n, wc, wmax );
     } while( (st == (flags = 0)) && (GetLastError() == ERROR_INVALID_FLAGS) );
  return st;
}

size_t __mingw_mbrscan_begin
( wchar_t *restrict *wcs, const char **restrict src, size_t *len,
  mbstate_t *restrict ps
)
{ /* Public helper function, to retrieve, interpret, and complete
   * conversion state, as passed to any MBCS to wchar_t conversion
   * routine, via its mbstate_t reference parameter.
   */
  size_t count = (size_t)(0);

  /* This becomes a no-op, if there is no pending state data...
   */
  if( *ps != (mbstate_t)(0) )
  { /* ...otherwise, we capture, and map the pending state, for
     * completion and interpretation...
     */
    union { mbstate_t st; char mb[MB_LEN_MAX]; wchar_t wc[2]; }
      state = { *ps };

    /* ...and mark the passed mbstate_t as completed.
     */
    *ps = (mbstate_t)(0);
    if( IS_SURROGATE_PAIR( state.wc[0], state.wc[1] ) )
    { /* When the pending state represents a surrogate pair, then
       * the high surrogate will have been returned previously; it
       * is the low surrogate which remains pending, and should now
       * be inserted into the return buffer, if any.
       */
      if( (wcs != NULL) && (*wcs != NULL) )
      { *(*wcs)++ = state.wc[1];
	if( *len > 0 ) --*len;
      }
      /* In any case, we must account for the low surrogate, which
       * is represented by this pending state.
       */
      ++count;
    }
    else
    { /* The pending state represents a previously scanned, but not
       * yet complete MBCS sequence; we must now add additional bytes,
       * from the MBCS input sequence, until the pending sequence is
       * either completed, or can be ruled as invalid.
       */
      int copy, scan = 0, mark = 0;

      /* To determine completion state, we need a scratch conversion
       * buffer which may subsequently be interpreted as mbstate.
       */
      union { mbstate_t st; wchar_t wc[2]; } buf;

      /* First, we mark the offset within the pending state buffer,
       * where the first additional byte should be appended...
       */
      while( state.mb[mark] != '\0' ) ++mark;
      while( scan == 0 )
      { /* ...then we extend this, by appending bytes from the MBCS
	 * input, until we either NUL terminate it, or we reach the
	 * effective maximum MBCS length for a single code point.
	 */
	for( copy = mark; ((*src)[scan] != '\0') && (copy < mbrlen_cur_max); )
	  state.mb[copy++] = (*src)[scan++];

	/* In the case of NUL termination, the terminating byte has
	 * yet to be stored.
	 */
	if( copy < mbrlen_cur_max ) state.mb[copy] = '\0';

	/* Having now captured a potential single code point MBCS
	 * sequence, in the state buffer, we now examine that, in
	 * incremental steps of its initial byte sequence, until
	 * we can successfully convert it, or we must reject it.
	 */
	do { copy = __mingw_mbtowc_convert( state.mb, ++scan, buf.wc, 2 );
	   } while( (copy == 0) && (scan < mbrlen_cur_max) );

	/* If conversion is unsuccessful...
	 */
	if( copy == 0 )
	{ /* ...and we have extended the sequence to the maximum
	   * length allowed for a single code point, then we must
	   * reject the entire input sequence...
	   */
	  if( scan >= mbrlen_cur_max )
	    return errout( EILSEQ, (size_t)(-1) );

	  /* ...otherwise, there is still a possibility that we
	   * may be able to complete this sequence during a later
	   * call, so return it as pending state.
	   */
	  *ps = buf.st;
	  return (size_t)(0);
	}
	/* A successful conversion, which requires more than one
	 * wchar_t, MUST be represented as a surrogate pair; any
	 * other longer representation is invalid.
	 */
	if( (copy > 1) && ! IS_SURROGATE_PAIR( buf.wc[0], buf.wc[1] ) )
	  return errout( EILSEQ, (size_t)(-1) );

	/* When the representation of a successful conversion is
	 * accepted as valid, and...
	 */
	if( (wcs != NULL) && (*wcs != NULL) )
	{
	  /* ...the caller has provided a buffer, in which to
	   * return it, then we return at least the first wchar
	   * of its representation, and then...
	   */
	  *(*wcs)++ = buf.wc[0];
	  if( *len >= (size_t)(copy) )
	  { /* ...when the declared buffer length is sufficient
	     * to accommodate more, and the conversion represents
	     * a surrogate pair, we also return the low surrogate,
	     * and adjust the length to account for it...
	     */
	    if( copy > 1 ) *(*wcs)++ = buf.wc[1];
	    *len -= (size_t)(copy);
	  }
	  else if( copy > 1 )
	  { /* ...otherwise, when we have a surrogate pair to be
	     * returned, but only sufficient buffer to accommodate
	     * the high surrogate, we defer the low surrogate for
	     * return during a subsequent call.
	     */
	    *ps = buf.st;
	    return (size_t)(1);
	  }
	}
	/* Increment the return count, to account for each wchar
	 * which has been interpreted, thus far, from the given
	 * pending state.
	 */
	count += (size_t)(copy);

	/* Check that we have consumed all content from the given
	 * pending state...
	 */
	if( mark > scan )
	{ /* ...or otherwise, discard what we have consumed, and
	   * promote the residual for further consideration.
	   */
	  state.mb[mark] = '\0';
	  for( mark = 0; state.mb[mark] != '\0'; ++mark, ++scan )
	    state.mb[mark] = state.mb[scan];
	  scan = 0;
	}
	/* When all pending state has been consumed, adjust the
	 * input MBCS sequence pointer, to account for any bytes
	 * used to complete that pending state.
	 */
	else *src += (scan - mark);
      }
    }
  }
  /* Ultimately, return the count of wchar elements, if any, which
   * result from conversion of pending state.
   */
  return count;
}

size_t __mingw_mbtowc_copy
( wchar_t *restrict wcs, const char *restrict src, size_t len )
{
  /* Public helper function to copy a sequence of one or more wchar_t
   * elements, which result from conversion of the given MBCS sequence,
   * either to a caller-provided buffer, (or, if none is provided, use
   * an internal scratch buffer, to facilitate counting the number of
   * such elements which would be copied, without storing them).
   */
  wchar_t scratch[2]; size_t count = (size_t)(0);
  while( count < len )
  {
    int copy, scan = 0;
    wchar_t *wc = (wcs == NULL) ? scratch : wcs;
    do { copy = __mingw_mbtowc_convert( src, ++scan, wc, 2 );
       } while( (copy == 0) && (scan < mbrlen_cur_max) );

    if( copy == 0 ) return errout( EILSEQ, (size_t)(-1) );

    if( *wc == L'\0' ) len = count;
    else
    { count += copy;
      if( wcs != NULL ) wcs += copy;
      src += scan;
    }
  }
  return count;
}

/* $RCSfile$: end of file */
