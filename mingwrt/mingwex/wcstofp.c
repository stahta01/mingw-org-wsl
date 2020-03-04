/*
 * wcstofp.c
 *
 * Implementation of ISO-C99 compatible wcstod(), wcstold(), and wcstof()
 * functions, placed into the "__mingw_" pseudo-namespace, with enhanced
 * C99 compatibility, and codeset coverage.
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
 *
 * Compile with "-D FUNCTION=wcstod -o wcstod.o", or equivalent for each
 * of wcstold() and wcstof(), to create free-standing object modules for
 * each supported function; additionally, compile without "-D FUNCTION",
 * and with "-o wcstofp.o", (or with "-D FUNCTION=wcstofp -o wcstofp.o"),
 * to create the mandatory common supporting object code module.
 *
 */
#define _ISOC99_SOURCE

/* Parsing of floating point values, from wchar_t strings, is performed
 * after conversion to the MBCS domain; to support any codeset with more
 * than two bytes per code point, we require MinGW.org's extended codeset
 * mapping API.
 */
#include "wcharmap.h"

/* Declare prototypes, visible within each derived compilation unit, for
 * each supporting function which is to be compiled into the common unit.
 */
const wchar_t *__mingw_wcstofp_prescan( const wchar_t * );
size_t __mingw_wcstofp_prepare( const wchar_t *, char *, size_t );
size_t __mingw_wcstofp_bufsize( const wchar_t * );

/* Ensure that the function to be compiled has been specified...
 */
#ifndef FUNCTION
/* ...or alternatively, fall back to compilation of the common "wcstofp"
 * support code module...
 */
#define FUNCTION		wcstofp
#endif
/* ...and define a symbolic selector for the latter.
 */
#define wcstofp  		1

#if FUNCTION
/* Default FUNCTION assignment -- compile the core support routines,
 * common to all public API entry points.
 */
const wchar_t *__mingw_wcstofp_prescan( const wchar_t *nptr )
{
  /* Helper function to locate the effective starting point of a wchar_t
   * string, ignoring any leading white-space.
   */
  if( nptr == NULL ) errno = EINVAL;
  else while( iswspace( *nptr ) ) ++nptr;
  return nptr;
}

size_t __mingw_wcstofp_prepare ( const wchar_t *nptr, char *mbs, size_t max )
{
  /* Helper function to prepare for interpretation of a wchar_t string
   * representation of a floating point number; determines the size of
   * buffer required, and optionally converts to MBCS representation,
   * for interpretation by an apropriate string to binary converter.
   */
  size_t nbytes = (size_t)(0);
  while( *nptr != L'\0' )
  { /* Excluding the terminating NUL, convert wchar_t string elements
     * one by one...
     */
    size_t count = __mingw_wctomb_convert( mbs, max, nptr++, 1 );

    /* ...and, for each successfully converted, without exceeding the
     * specified maximum conversion buffer length...
     */
    if( count != (size_t)(-1) )
    { /* ...optionally store its MBCS equivalent, while unconditionally
       * the actual buffer length requirement...
       */
      if( mbs != NULL ) { mbs += count; max -= count; }
      nbytes += count;
    }
    /* Bail out early, if any element cannot be converted successfully,
     * returning the count of MBCS bytes up to point of failure...
     */
    else return nbytes;
  }
  /* ...or similarly, the count of MBCS bytes for complete conversion,
   * when the entire wchar_t string can be successfully converted.
   */
  return nbytes;
}

/* A wrapper around the preceding function, to determine the required
 * buffer size, without storing the MBCS conversion; used by callers,
 * to allocate buffers of suitable size.
 */
size_t __mingw_wcstofp_bufsize( const wchar_t *nptr )
{ return 1 + __mingw_wcstofp_prepare( nptr, NULL, 0 ); }

#else
/* Compile function code for one specific public API entry point.
 */
#undef wcstod
#undef wcstold
#undef wcstof

/* Define macros to specify the one specific entry point name...
 */
#define __mingw_redirect(FUNCTION)	set(__mingw,FUNCTION)

/* ...and the associated data type, initial value, and corresponding
 * MBCS string to binary conversion function name.
 */
#define set(FUNCTION,NAME)		FUNCTION##_##NAME
#define datatype(FUNCTION)		set(FUNCTION,datatype)
#define initval(FUNCTION)		set(FUNCTION,initval)
#define strtofp(FUNCTION)		set(FUNCTION,strtofp)

/* Specify MBCS converter, data type, and initial value for the
 * __mingw_wcstod() function.
 */
#define wcstod_strtofp  		strtod
#define wcstod_datatype 		double
#define wcstod_initval  		0.0

/* Likewise, for the __mingw_wcstold() function...
 */
#define wcstold_strtofp 		strtold
#define wcstold_datatype		long double
#define wcstold_initval 		0.0L

/* ...and the __mingw_wcstof() function.
 */
#define wcstof_strtofp  		strtof
#define wcstof_datatype 		float
#define wcstof_initval  		0.0F

/* Generic API function implementation, in terms of the above.
 */
datatype(FUNCTION) __mingw_redirect(FUNCTION)
( const wchar_t *restrict nptr, wchar_t **restrict endptr )
{
  /* Initialize, to return appropriately typed zero, in the event
   * of no valid floating point representation being found.
   */
  datatype(FUNCTION) retval = initval(FUNCTION);

  /* Advance the wchar_t string pointer, beyond any white-space
   * characters which may be present.
   */
  if( (nptr = __mingw_wcstofp_prescan( nptr )) != NULL )
  {
    /* We found a candidate wchar_t string for interpretation;
     * allocate buffer space, for conversion to an MBCS string,
     * with respect to the codeset for the current locale.
     */
    size_t buflen;
    (void)(__mingw_wctomb_codeset_init());
    if( (buflen = __mingw_wcstofp_bufsize( nptr )) > 0 )
    { char mbstr[buflen], *endmark;

      /* Convert to MBCS, appending NUL terminator, and attempt
       * equivalent binary floating point interpretation.
       */
      mbstr[__mingw_wcstofp_prepare( nptr, mbstr, buflen )] = '\0';
      retval = strtofp(FUNCTION)( mbstr, &endmark );

      if( endptr != NULL )
      { /* Caller wants to check for any junk, following the
	 * numeric representation within the original wchar_t
	 * string, but we know only the corresponding offset
	 * of trailing junk within the MBCS string; step along
	 * the wchar_t string, converting one element at a time,
	 * until the aggregate conversion length matches the
	 * known MBCS junk offset.
	 */
	char *p = mbstr;
	while( p < endmark )
	  p += __mingw_wctomb_convert( NULL, 0, nptr++, 1 );
	*endptr = (wchar_t *)(nptr);
      }
    }
  }
  /* Return the floating point result, whether it was interpreted
   * from the given wchar_t string, or remains as initial default.
   */
  return retval;
}

/* Microsoft's runtime library provides its own implementation for
 * wcstod(), but (prior to non-free MSVCR120.DLL) not for wcstof(),
 * or wcstold(); request creation of aliases for the latter pair...
 */
#define wcstof			1
#define wcstold 		1

#if FUNCTION
/* ...then implement macros...
 */
#define stringify(NAME) 	#NAME
#define mkstring(NAME)		stringify(NAME)

/* ...avoiding substitution of the function names...
 */
#undef wcstof
#undef wcstold

/* ...to actually implement these aliases.
 */
datatype(FUNCTION)
__attribute__((__weak__,__alias__(mkstring(__mingw_redirect(FUNCTION)))))
FUNCTION ( const wchar_t *restrict nptr, wchar_t **restrict endptr );

#endif
#endif

/* $RCSfile$: end of file */
