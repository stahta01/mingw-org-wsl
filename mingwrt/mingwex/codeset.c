/*
 * codeset.c
 *
 * Provides implementation-private helper functions, to identify the
 * code page which is associated with the active process locale, and to
 * establish the effective MB_CUR_MAX value for this code page.
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
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <winnls.h>

unsigned int __mb_codeset_for_locale( void );
unsigned int __mb_cur_max_for_codeset( unsigned int );

unsigned int __mb_codeset_for_locale( void )
{
  /* Extract the code page identification string (if any) from the LC_CTYPE
   * identification string, as returned in "language[_region[.codeset]]", or
   * ".codeset" format, by a setlocale() query on the current locale.
   */
  char *default_locale_specification, *codeset_string;
  if( (default_locale_specification = setlocale( LC_CTYPE, NULL )) != NULL )
  {
    /* An unfortunate -- albeit documented -- limitation of Microsoft's
     * setlocale() implementation is that it cannot correctly process any
     * locale specification which refers to a MBCS codeset which may use
     * more than two bytes for any single code point; to mitigate this,
     * when the active locale matches the system default...
     */
    char string_buffer[1 + strlen( default_locale_specification )];
    codeset_string = strcpy( string_buffer, default_locale_specification );
    if( strcmp( codeset_string, setlocale( LC_CTYPE, "" )) == 0 )
    {
      /* ...although Microsoft's setlocale() doesn't support it, (and
       * is neither expected to, nor required to), we may adopt POSIX.1
       * convention, in this particular case, to acquire a preferred
       * default locale specification from the environment...
       */
      if( ((default_locale_specification = getenv( "LC_ALL" )) != NULL)
       || ((default_locale_specification = getenv( "LC_CTYPE" )) != NULL)
       || ((default_locale_specification = getenv( "LANG" )) != NULL)     )

	/* ...and use that in place of Microsoft's setlocale() notion
	 * of the current effective LC_CTYPE locale category.
	 */
	codeset_string = default_locale_specification;
    }
    else
    { /* The originally active locale does NOT match the system default,
       * but we made it do so, by checking, so restore the original.
       */
      setlocale( LC_CTYPE, codeset_string );
    }
    /* Regardless of how we established the effective LC_CTYPE category
     * for the active locale, we may extract its codeset element...
     */
    if( (codeset_string = strchr( codeset_string, '.' )) != NULL )
    {
      /* ...interpreting the resultant string as its equivalent integer
       * value, for validation and return.
       */
      unsigned int retval = (unsigned int)(atoi( codeset_string + 1 ));
      if( __mb_cur_max_for_codeset( retval ) > 0 ) return retval;
    }
  }
  /* In the event that LC_CTYPE doesn't include a codeset identification,
   * return an effective value of zero, which we may later interpret as a
   * default representation for the "C" locale.
   */
  return 0;
}

unsigned int __mb_cur_max_for_codeset( unsigned int codeset )
{
  /* Identify the length of the longest valid multibyte character encoding
   * sequence, used within the specified MS-Windows code page, by consulting
   * the relevant Win32 API database.  Returns the appropriate byte count,
   * or zero if the codeset identifier is not valid.
   */
  CPINFO codeset_info;
  return (GetCPInfo( codeset, &codeset_info )) ? codeset_info.MaxCharSize : 0;
}

/* $RCSfile$: end of file */
