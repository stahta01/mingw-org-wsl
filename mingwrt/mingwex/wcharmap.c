/*
 * wcharmap.c
 *
 * Provides an implementation-private helper function, to facilitate
 * conversion from UTF-16LE wchar_t data, of arbitrary length, to an
 * equivalent multi-byte character encoding sequence.
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

/* We need a definition of macro UCHAR_MAX; it is defined in <limits.h>
 */
#include <limits.h>

/* The working codeset, and its associated effective MB_CUR_MAX,
 * are stored with file-scope visibility, to facilitate passing
 * them to individual elements of the implementation...
 */
static __thread unsigned int codeset, wctomb_cur_max;

/* ...but, in this instance, we also need to provide a mechanism
 * for initializing each of these from the global scope...
 */
unsigned int __mingw_wctomb_codeset_init( void )
{ return codeset = __mb_codeset_for_locale(); }

unsigned int __mingw_wctomb_cur_max_init( unsigned int codeset )
{ return wctomb_cur_max = __mb_cur_max_for_codeset( codeset ); }

/* ...and also, an accessor to make the effective MB_CUR_MAX
 * available in the global scope.
 */
unsigned int __mingw_wctomb_cur_max( void )
{ return wctomb_cur_max; }

size_t __mingw_wctomb_convert
( char *mbs, int mblen, const wchar_t *wcs, int wclen )
{
  /* Helper function to map a sequence of wchars to their corresponding
   * sequence of multibyte characters, encoded as is appropriate for the
   * specified code page, (which is nominally the code page associated
   * with the current locale).
   *
   * Inputs:
   *   mbs	Buffer in which the encoded multibyte sequence may be
   *    	returned, or NULL, if only the sequence length is to
   *    	be determined, discarding the encoded data.
   *
   *   mblen	Number of bytes available in mbs; ignured if mbs is
   *    	passed as NULL.
   *
   *   wcs	The sequence of wchars which is to be encoded.
   *
   *   wclen	The number of wchars in wcs; if passed as (size_t)(-1),
   *    	scan until (wchar_t)(0), or until a wchar with no valid
   *    	encoding, or space in the encoding buffer is exhausted.
   *
   * Returns:
   *   The number of encoded bytes (which would be) stored into mbs, if
   *   mbs is not NULL, and all specifed wchars in wcs are successfully
   *   encoded; otherwise, returns (size_t)(-1), and sets errno to:
   *
   *   EILSEQ	If encoding is interrupted by a wchar with no valid
   *    	encoding within the specified code page.
   *
   *   ENOMEM	The mbs pointer isn't NULL, but there is insufficient
   *    	space in the designated buffer to store the encoded
   *    	multibyte character sequence.
   */
  size_t retval; int eilseq_flag = 0;

  if( codeset == 0 )
  { /* Code page zero is assumed to represent the encoding which applies
     * within the "C" locale; this is a single-byte encoding, with wchar
     * values in the range L'\0'..L'\255' mapped to their identical byte
     * values, and all greater wchar values considered to be invalid.
     *
     * Simply scan, count, and optionally store valid byte values,
     * starting from an initial count of zero.
     */
    retval = 0;

    if( (size_t)(wclen) == (size_t)(-1) )
      do { /* This is an unbounded scan; simply check that each
	    * successive wchar lies in the valid range...
	    */
	   if( (unsigned)(*wcs) > UCHAR_MAX )
	     /* ...otherwise, report an invalid encoding, and
	      * bail out.
	      */
	     return errout( EILSEQ, wclen );

	   /* We got a valid input wchar...
	    */
	   if( mbs != NULL )
	   { /* ...which we are now expected to store...
	      */
	     if( mblen-- > 0 ) *mbs++ = (unsigned char)(*wcs);

	     /* ...but, we must bail out, if there is no
	      * space left in the encoding buffer.
	      */
	     else return errout( ENOMEM, (size_t)(-1) );
	   }

	   /* We've accepted the current input wchar; count
	    * it, and then, provided it isn't the terminating
	    * NUL, move on to the next.
	    */
	   ++retval;
	 } while( *wcs++ != L'\0' );

    else while( wclen-- > 0 )
    { /* This is a bounded scan; as in the unbounded case, take
       * each input wchar in turn, and verify that each lies in
       * the valid encoding range.
       */
      if( (unsigned)(*wcs) > UCHAR_MAX )
	return errout( EILSEQ, (size_t)(-1) );

      /* We got a valid input wchar...
       */
      if( mbs != NULL )
      { /* ...which we are now expected to store...
	 */
	if( mblen-- > 0 ) *mbs++ = (unsigned char)(*wcs);

	/* ...but, we must bail out, if there is no
   	 * space left in the encoding buffer.
	 */
	else return errout( ENOMEM, (size_t)(-1) );
      }

      /* Ensure that we don't scan beyond a terminating NUL
       * wchar, even if this lies within the bounded count.
       */
      if( *wcs++ == L'\0' ) wclen = 0;

      /* In any case, count the current encoded byte.
       */
      ++retval;
    }

    /* We now have the final count, for a code page zero encoding;
     * we are done.
     */
    return retval;
  }

  /* For any code page other than zero, we delegate both encoding
   * and byte counting to the Windows API; note that for code pages
   * other than CP_UTF7 or CP_UTF8, (and CP_UTF8 is the only code
   * page with an identifier greater than that for CP_UTF7), there
   * may be unrepresentable UTF-16 code points, and we must pass a
   * flag reference to detect their presence in the UTF-16LE input
   * sequence; OTOH, any valid UTF-16 code point is representable
   * in both CP_UTF7 and CP_UTF8, so no such flag is required, and
   * WideCharToMultiByte() will choke, if the flag reference is
   * not passed as NULL.
   */
  retval = WideCharToMultiByte( codeset, 0, wcs, wclen, mbs, mblen, NULL,
      (CP_UTF7 > codeset) ? &eilseq_flag : NULL
    );
  return (eilseq_flag || (retval == 0)) ? errout( EILSEQ, (size_t)(-1) )
    : retval;
}

/* $RCSfile$: end of file */
