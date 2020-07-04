/*
 * mbsrtowcs.c
 *
 * MinGW.org replacement for the ISO-C99 mbsrtowcs() function, supporting
 * its use on legacy Windows versions, for which Microsoft does not provide
 * it, while replacing the Microsoft implementation on any Windows version
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

#include <limits.h>

static __mb_inline__
boolean __mingw_mbtowc_verify( const char *src, size_t len )
{
  /* Internal convenience wrapper, for checking viability of any
   * bounded-length MBCS to wchar_t string conversion.
   */
  return __mingw_mbtowc_copy( NULL, src, len ) != (size_t)(-1);
}

static __mb_inline__ size_t __mbsrtowcs_internal
( wchar_t *restrict wcs, const char **restrict src, size_t len,
  mbstate_t *restrict ps
)
{ /* Internal implementation of the mbsrtowcs() function; this is
     expanded inline, within the public implementation.
   */
  size_t count = (size_t)(0);
  if( (src != NULL) && (*src != NULL) )
  { /* There is an MBCS sequence to convert; we may need more than
     * one conversion pass, so save duplicates of the initial state,
     * for use when a deferred second pass is required.
     */
    boolean deferred = false;
    const char *srcptr = *src;
    mbstate_t psdup = *ps;

    /* Begin by checking for any pending state, and determine the
     * number of wchar_t elements needed to represent its completed
     * conversion...
     */
    count = __mingw_mbrscan_begin( NULL, &srcptr, NULL, ps );
    if( count != (size_t)(-1) )
    { /* ...followed by the number of additional elements required
       * to represent the conversion of any remaining MBCS sequence,
       * including its mandatory terminating NUL.
       */
      size_t extra = __mingw_mbtowc_convert( srcptr, 0, NULL, 0 );
      if( extra > (size_t)(0) )
      {
	/* The specified MBCS sequence is valid, but its conversion
	 * has not (yet) been stored; (if storage is requested, then
	 * a further conversion pass is required).  First, we adjust
	 * the agregate count, discounting the terminating NUL...
	 */
	count += extra - 1;

	/* ...then proceed with whatever further processing may be
	 * required.
	 */
	if( wcs == NULL )
	{ /* This was an unbounded scan, so it has terminated at the
	   * NUL terminator of the MBCS sequence; in this case, we do
	   * not store the converted result, so no further conversion
	   * pass is required, and we may immediately return the scan
	   * count, as already computed.
	   */
	  return count;
	}
	else if( len > count )
	{ /* This is a bounded scan, with sufficient buffer length to
	   * accommodate the entire converted MBCS sequence, including
	   * its terminating NUL; we must now perform the conversion
	   * again, this time actually storing the result.
	   */
	  (void)(__mingw_mbrscan_begin( &wcs, src, &len, &psdup ));
	  (void)(__mingw_mbtowc_convert( *src, 0, wcs, len ));

	  /* Since we've converted the entire MBCS sequence, including
	   * the terminating NUL, the ISO-C99 standard decrees that we
	   * must reset the original MBCS pointer to NULL, and we must
	   * return the converted count, discounting the NUL.
	   */
	  *src = NULL;
	  return count;
	}
	else
	{ /* The converted MBCS sequence is longer than the declared
	   * length of the available buffer space can accommodate; we
	   * defer conversion for now; we will revisit it below.
	   */
	  deferred = true;
	}
      }
      else if( (wcs != NULL) && (len > count) )
      { /* There is an encoding error, somewhere within the original
	 * MBCS sequence; we must rescan it, to determine whether or
	 * not the singularity occurs within the declared length of
	 * the designated conversion buffer.
	 */
	deferred = __mingw_mbtowc_verify( srcptr, len - count );
      }
      if( deferred )
      { /* This occurs, only if a non-NULL buffer address was given,
	 * for storage of the converted MBCS sequence, but the actual
	 * store operation was deferred because the specified buffer
	 * length was insufficient to store the complete conversion,
	 * or an MBCS encoding error was detected, beyond the point
	 * at which the conversion buffer would be exhausted.  Do
	 * the conversion again, storing the converted data, until
	 * the buffer has been completely filled.
	 */
	count = __mingw_mbrscan_begin( &wcs, src, &len, &psdup );
	return count + __mingw_mbtowc_copy( wcs, *src, len );
      }
    }
    /* If we get to here, an illegal MBCS sequence was detected, and
     * it lay within the required conversion span; abort the entire
     * conversion, setting errno per ISO-C99 specification...
     */
    return errout( EILSEQ, (size_t)(-1) );
  }
  /* ...whereas, if we get to here, there was no MBCS sequence for us
   * to convert; ISO-C99 doesn't specify any particular action to take
   * in this case, so we simply do nothing.
   */
  return count;
}

size_t mbsrtowcs
( wchar_t *restrict wcs, const char **restrict src, size_t len,
  mbstate_t *restrict ps
)
{ /* Implementation of ISO-C99 mbsrtowcs() function, in libmingwex.a;
   * this stores the effective codeset properties, before returning the
   * result from expansion of the preceding inline implementation.
   */
  (void)(__mingw_mbrlen_cur_max_init( __mingw_mbrtowc_codeset_init() ));
  return __mbsrtowcs_internal( wcs, src, len, __mbrtowc_state( ps ) );
}

/* FIXME: these aliases are provided for link-compatibitity with
 * libraries compiled against mingwrt-5.3.x; they may be removed
 * from future versions of mingwrt.
 */
size_t __mingw_mbsrtowcs
( wchar_t *restrict, const char **restrict, size_t, mbstate_t *restrict )
__attribute__((__weak__,__alias__("mbsrtowcs")));

size_t __msvcrt_mbsrtowcs
( wchar_t *restrict, const char **restrict, size_t, mbstate_t *restrict )
__attribute__((__weak__,__alias__("mbsrtowcs")));

/* $RCSfile$: end of file */
