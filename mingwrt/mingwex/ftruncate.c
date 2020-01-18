/*
 * ftruncate.c
 *
 * Implement a 64-bit file size capable ftruncate() function; GCC-9.x
 * gratuitously assumes that this is available, via the ftruncate64()
 * entry point.
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
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */
#include <dlfcn.h>
#include <unistd.h>
#include <winbase.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/* The following in-line function is provided to facilitate abnormal return
 * from ftruncate64(), (in which case the return value is always -1), while
 * setting the global errno indicator to an appropriate error code.
 */
static __inline__ __attribute__((__always_inline__))
int errout( int error_code ){ errno = error_code; return -1L; }

/* When running on Vista, or later, or for applications which have been
 * linked against non-free MSVCR80.DLL, or later, we may be able to simply
 * substitute a call to Microsoft's _chsize_s() function, (which behaves
 * as a 64-bit variant of the universally available _chsize() function).
 * On legacy Windows versions, which are unlikely to provide _chsize_s(),
 * we need to provide our own fallback 64-bit chsize() implementation;
 * this may be static, but cannot be inlined, because we need a physical
 * entry point address to which execution may be redirected.
 */
static int mingw_chsize64_fallback( int fd, __off64_t offset )
{
  /* POSIX.1 requires the file pointer to be unchanged, as a consequence
   * of calling ftruncate(), (and Microsoft's _chsize() functions do seem
   * to satisfy this requirement); however, to mark a new end of file, we
   * need move the file pointer to the new end of file offset, so we need
   * to save the original pointer now, to restore later.
   */
  __off64_t cur_offset = _lseeki64( fd, 0LL, SEEK_CUR );

  /* In the event that the new end of file offset requires the file to be
   * extended beyond its current end of file offset, POSIX.1 also requires
   * NUL byte padding to be written to the extended file space, (and again,
   * Microsoft's _chsize() functions seem to do this); we may reposition
   * the file pointer to its current end of file offset, in preparation
   * for the possibility that we need to fulfil this requirement.
   */
  __off64_t end_offset = _lseeki64( fd, 0LL, SEEK_END );

  /* We will eventually need to restore the original file pointer, AFTER
   * we have evaluated the return status code, so we will need to save
   * this.
   */
  int retval;

  /* There are two possible options for repositioning the end of file
   * pointer:
   */
  if( offset > end_offset )
  {
    /* In this case, the file is to be extended beyond its current
     * end of file offset; initialize a NUL filled buffer, which we
     * may then copy to the extended region of the file, to satisfy
     * the POSIX.1 requirement that this region shall be NUL filled.
     */
    char padding[BUFSIZ];
    memset( padding, 0, sizeof( padding ) );

    /* Recompute the desired offset, relative to the current end of
     * file, then repeatedly write copies of the NUL filled buffer,
     * until the file space represented by this relative offset has
     * been completely filled; (this results in advancement of the
     * file pointer to the desired new end of file offset).
     */
    offset -= end_offset;
    while( offset > (__off64_t)(sizeof( padding )) )
      offset -= write( fd, padding, sizeof( padding ) );
    write( fd, padding, offset );
  }
  else
    /* In the alternative case, the new end of file pointer will lie
     * within the space already occupied by the file; we may simply
     * seek directly to the desired offset.
     */
    _lseeki64( fd, offset, SEEK_SET );

  /* We have now adjusted the file pointer to be coincident with the
   * desired new end of file offset; this is exactly what is required
   * by the Windows API function, to mark the new end of file.
   */
  retval = SetEndOfFile( (void *)(_get_osfhandle( fd )) )
    ? 0 : errout( EBADF );

  /* Finally, we must restore the originally saved file pointer, before
   * we return the status code from the ftruncate() operation.
   */
  _lseeki64( fd, cur_offset, SEEK_SET );
  return retval;
}

/* Regardless of the platform version, Microsoft do not provide an
 * implementation of ftruncate64(); all link-time references to this
 * function will be resolved by this libmingwex.a implementation.
 */
int ftruncate64( int fd, __off64_t offset )
{
  /* The offset parameter MUST be positive valued; bail out if not.
   */
  if( 0LL > offset ) return errout( EINVAL );

  /* For offsets which may be represented by a 32-bit integer, we
   * may ALWAYS delegate this call to Microsoft's _chsize().
   */
  if( INT32_MAX >= offset ) return _chsize( fd, (off_t)(offset) );

  { /* For offsets which cannot be represented within 32-bits, we
     * MAY be able to delegate this call, (and also any subsequent
     * calls), to Microsoft's _chsize_s(); set up a redirector to
     * handle such delegation...
     */
    static int (*redirector_hook)( int, __off64_t ) = NULL;

    /* ...initially checking for _chsize_s() availability...
     */
    if(  (redirector_hook == NULL)
    &&  ((redirector_hook = dlsym( RTLD_DEFAULT, "_chsize_s" )) == NULL)  )

      /* ...and setting up a suitable fallback if not...
       */
      redirector_hook = mingw_chsize64_fallback;

    /* ...and ultimately, on initial selection, (and directly on
     * all subsequent calls), hand off execution to the selected
     * delegate function.
     */
    return redirector_hook( fd, offset );
  }
}

/* $RCSfile$: end of file */
