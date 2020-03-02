/*
 * mbsrtowcs.c
 *
 * MinGW.org replacement for the ISO-C99 mbsrtowcs() function; may delegate
 * to a Microsoft implementation, if available in the C runtime DLL, (unless
 * this is overridden by user choice); otherwise handles the call locally.
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

#include <dlfcn.h>
#include <limits.h>

static __mb_inline__
boolean __mingw_mbtowc_verify( const char *src, size_t len )
{
  /* Internal convenience wrapper, for checking viability of any
   * bounded-length MBCS to wchar_t string conversion.
   */
  return __mingw_mbtowc_copy( NULL, src, len ) != (size_t)(-1);
}

static __mb_inline__ size_t __mbsrtowcs_fallback
( wchar_t *restrict wcs, const char **restrict src, size_t len,
  mbstate_t *restrict ps
)
{ /* Internal fallback function, providing an implementation of the
   * mbsrtowcs() function, when none is available in the Microsoft C
   * runtime DLL, or the user has explicitly overridden selection of
   * any such Microsoft implementation.
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

static size_t __mingw_mbsrtowcs_fallback
( wchar_t *restrict wcs, const char **restrict src, size_t len,
  mbstate_t *restrict ps __attribute__((__unused__))
)
{ /* MinGW fallback implementation for the mbsrtowcs() function; this
   * is a trivial wrapper around the preceding implementation, (which
   * should be expanded in-line), ensuring that an internal buffer is
   * assigned for the "ps" argument, if the caller doesn't pass one.
   */
  return __mbsrtowcs_fallback( wcs, src, len, __mbrtowc_state( ps ) );
}

size_t __mingw_mbsrtowcs
( wchar_t *restrict wcs, const char **restrict src, size_t len,
  mbstate_t *restrict ps
)
{ /* Wrapper for the mbsrtowcs() function; this will unconditionally
   * delegate the call to the MinGW fallback implementation, (defined
   * above), irrespective of availability of any Microsoft-provided
   * implementation.
   *
   * Note that, before handing off the call, we must unconditionally
   * initialize the working codeset, and its effective MB_CUR_MAX.
   */
  (void)(__mingw_mbrlen_cur_max_init( __mingw_mbrtowc_codeset_init() ));
  return __mingw_mbsrtowcs_fallback( wcs, src, len, ps );
}

size_t __msvcrt_mbsrtowcs
( wchar_t *restrict wcs, const char **restrict src, size_t len,
  mbstate_t *restrict ps
)
{ /* Wrapper for the mbsrtowcs() function; this will initially attempt
   * to delegate the call to a Microsoft-provided implementation, but if
   * no such implementation can be found, it will fall back to the MinGW
   * substitute (defined above).
   */
  typedef size_t (*redirector_t)
  ( wchar_t *restrict, const char **restrict, size_t, mbstate_t *restrict );
  static redirector_t redirector_hook = NULL;

  /* MSVCRT.DLL's setlocale() cannot reliably handle code pages with
   * more than two bytes per code point, (e.g. UTF-7 and UTF-8); thus,
   * Microsoft's mbsrtowcs() is likely to be similarly unreliable, so
   * always use the MinGW fallback with such code pages.
   */
  if( __mingw_mbrlen_cur_max_init( __mingw_mbrtowc_codeset_init() ) > 2 )
    return __mingw_mbsrtowcs_fallback( wcs, src, len, ps );

  /* On first time call, we don't know which implementation is to be
   * selected; look for a Microsoft implementation, which, if available,
   * may be registered for immediate use on this, and any subsequent,
   * calls to this function wrapper...
   */
  if(  (redirector_hook == NULL)
  &&  ((redirector_hook = dlsym( RTLD_DEFAULT, "mbsrtowcs" )) == NULL)  )

    /* ...but when no Microsoft implementation can be found, register
     * the MinGW fall back in its stead.
     */
    redirector_hook = __mingw_mbsrtowcs_fallback;

  /* Finally, delegate the call to whichever implementation has been
   * registered on first-time call.
   */
  return redirector_hook( wcs, src, len, ps );
}

/* $RCSfile$: end of file */
