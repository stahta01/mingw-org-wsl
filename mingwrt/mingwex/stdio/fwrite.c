/*
 * fwrite.c
 *
 * Workaround for limitations on Win9x where extended file content
 * is not zeroed out if you seek past the end and then write.
 *
 *
 * $Id$
 *
 * Written by Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2018, MinGW.org Project.
 *
 *
 * Replaces mingw-fseek.c implementation
 * Written by Mumit Khan <khan@xraylith.wisc.edu>
 * Copyright (C) 1999, 2002-2005, 2011, 2015, MinGW.org Project.
 *
 * Originally abstracted from MinGW local patch to binutils/bfd/libbfd.c
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
 * Although specifically providing support for Win9x, we must compile this
 * with the associated feature test disabled, so that the wrappers provided
 * herein can access the underlying functions which they wrap.
 *
 */
#undef _WIN32_WINDOWS
#undef __USE_MINGW_FSEEK

#include <io.h>
#include <search.h>
#include <stdlib.h>
#include <stdio.h>

/* The fseek() handler control structures.
 */
static void fseek_handler_init( FILE * );
struct fseek_pending { struct fseek_pending *fwd, *bkwd; FILE *fp; };
static struct { struct fseek_pending *avail, *active; void (*action)(FILE *); }
fseek_handler = { NULL, NULL, fseek_handler_init };

/* The fseek() handler function redirector implementation.
 */
static __attribute__((__noinline__))
void *fseek_handler_trap_pending( FILE *fp )
{ /* Local helper, to check for any pending fseek trap associated
   * with "fp"; used from multiple locations, so we prefer to deny
   * GCC any choice to in-line it.
   *
   * Note that, if "fseek_handler.active" is NULL, there are no
   * pending stream traps, so there is nothing to check...
   */
  if( fseek_handler.active != NULL )
  { /* ...but when there is at least one active trap...
     */
    struct fseek_pending *trap = fseek_handler.active;
    do { /* ...we walk the queue of active traps, until we find
	  * (and immediately return) one associated with "fp", or
	  * we have examined all entries in the circular queue.
	  */
	 if( trap->fp == fp ) return trap;
       } while( (trap = trap->fwd) != fseek_handler.active );
  }
  /* If we get to here, there is no trap associated with "fp".
   */
  return NULL;
}

/* On WinNT, the fseek() handler is required to take no action; by
 * installing the following "no-op" handler, on first-time call, we
 * reduce the fseek() and fwrite() wrappers to operate as simple
 * pass-through filters...
 */
static void fseek_handler_nop( FILE *fp __attribute__((__unused__)) ){}

/* ...whereas, on Win9x, we install this active fseek() handler.
 */
static void fseek_handler_set_trap( FILE *fp )
{ /* This records fseek() requests, on a per-stream basis, such that
   * any subsequent fwrite() request can apply corrective action, to
   * ensure that any "holes" in the file stream are properly filled
   * with zeros, in the event that the fseek() has moved the file
   * pointer beyond EOF; however, we never assign more than one
   * active trap per stream.
   */
  if( fseek_handler_trap_pending( fp ) == NULL )
  { /* There is no active trap currently associated with "fp"; take
     * an unused trap record from the "avail" queue...
     */
    struct fseek_pending *avail;
    if( (avail = fseek_handler.avail) == NULL )
    { /* ...creating a new block of eight such records, if none are
       * currently available...
       */
      int avail_index = 8;
      avail = malloc( 8 * sizeof (struct fseek_pending) );

      /* ...and linking them into a linear queue.
       */
      avail->fwd = avail->bkwd = NULL;
      while( --avail_index > 0 ) insque( avail + avail_index, avail );
    }
    /* The taken record is popped from the front of the queue; thus
     * we must follow its forward link, to update the front-of-queue
     * pointer to the next available unused trap record.
     */
    remque( avail ); fseek_handler.avail = avail->fwd;

    /* Now, we must insert the "avail" record we've just acquired
     * into the "active" queue; this is managed as a circular queue,
     * so, if it is currently empty...
     */
    if( fseek_handler.active == NULL )
      /* ...then we must assign this new record as its sole entry,
       * with both links referring to itself...
       */
      fseek_handler.active = avail->fwd = avail->bkwd = avail;

    /* ...otherwise, the POSIX.1 insque() API will take care of it.
     */
    else insque( avail, fseek_handler.active );

    /* Finally, we must associate this new trap record with "fp".
     */
    avail->fp = fp;
  }
}

static void fseek_handler_init( FILE *fp )
{ /* fseek() handler initialization routine; invoked only the first time
   * that the fseek() handler itself is invoked, it checks whether we are
   * running on Win9x, and if not...
   */
  if( (_osver & 0x8000) == 0 )
    /* ...it installs a no-op handler for future calls, (since WinNT
     * doesn't require any further handling...
     */
    fseek_handler.action = fseek_handler_nop;

  else
  { /* ...otherwise, it installs the Win9x specific handler, which
     * traps seek requests to enable corrective action, which may be
     * required in a subsequent fwrite() call...
     */
    fseek_handler.action = fseek_handler_set_trap;

    /* ...and immediately sets a trap for this first-time call.
     */
    fseek_handler_set_trap( fp );
  }
}

/* Public API entry to the Win9x function redirector for the system
 * fseek() APIs; implemented in terms of fseeko64(), it is suitable as
 * a transparent wrapper for any of the fseek()-alike functions.
 */
int __mingw_fseek( FILE *fp, __off64_t offset, int whence )
{ fseek_handler.action( fp ); return fseeko64( fp, offset, whence ); }

static __off64_t fseek_handler_reset( struct fseek_pending *trap )
{ /* Bridging function, called exclusively by __mingw_fwrite(), to disarm
   * any fseek trap which it has identified as being active and associated
   * with its target stream; since it is called only from one location, we
   * may safely allow GCC to in-line it.
   */
  if( trap == fseek_handler.active )
  { /* The active trap is currently the lead entry, in the active trap
     * queue, then we must move the lead entry onward; if it continues
     * to refer to the same trap, then we must clear the queue...
     */
    if( fseek_handler.active->fwd == trap ) fseek_handler.active = NULL;

    /* ...otherwise, we simply remove the trap entry from the queue...
     */
    else remque( trap );
  }
  /* ...and likewise, if the trap entry is not the queue's lead entry.
   */
  else remque( trap );

  /* Having removed the trap from the "active" queue, we must return it
   * to the "avail" queue...
   */
  insque( trap, fseek_handler.avail );

  /* ...and, if that queue was previously empty, update its lead entry
   * reference pointer to match.
   */
  if( fseek_handler.avail == NULL ) fseek_handler.avail = trap;

  /* Finally, tell fwrite() where it must begin data transfer, after it
   * has completed any Win9x corrective action which may be required.
   */
  return __mingw_ftelli64( trap->fp );
}

size_t __mingw_fwrite( const void *buffer, size_t size, size_t count, FILE *fp )
{ /* A wrapper around the system fwrite() API; it ensures that padding
   * zero bytes are inserted, following EOF, when fwrite() is called on
   * Win9x, after any seek request which moves the file pointer to any
   * position which lies beyond the existing EOF.
   */
  struct fseek_pending *trap;
  if( (trap = fseek_handler_trap_pending( fp )) != NULL )
  { /* The fseek handler has determined that we are running on Win9x,
     * and that this fwrite operation was preceded by a seek; we don't
     * yet know if that seek has moved the position beyond the current
     * end of file, in which case Win9x may leave random garbage after
     * EOF, in the intervening space, (where ISO-C requires the effect
     * of zero bytes); check for this anomaly now.
     */
    __off64_t eof_pos, fwrite_pos = fseek_handler_reset( trap );
    if( fwrite_pos > (eof_pos = _lseeki64( fileno( fp ), 0LL, SEEK_END )) )
    { /* The original seek request HAD moved the fwrite position to
       * some point beyond EOF!  We've now moved it back to EOF, so
       * prepare to fill with zeros, to pad out the file until we
       * return to the original seek position.
       */
      char zero_bytes[BUFSIZ] = {'\0'};
      __int64 fill_len = fwrite_pos - eof_pos;

      /* Emit the requisite number of padding zeros, in blocks of
       * no more than BUFSIZ bytes...
       */
      while( fill_len > 0LL )
      { size_t len = (fill_len > BUFSIZ) ? BUFSIZ : fill_len;
	if( fwrite( zero_bytes, 1, len, fp ) != len )
	{ /* ...but, if any block falls short of its expected size,
	   * then an error has occurred; attempt to restore to the
	   * original seek position, and abort the fwrite request,
	   * having written NONE of its requested data.
	   */
	  __mingw_fseeki64( fp, fwrite_pos, SEEK_SET );
	  return 0;
	}
	/* A padding block has been successfully written; adjust
	 * the residual padding length, to account for it.
	 */
	fill_len -= len;
      }
    }
    else
      /* The preceding seek was not beyond end of file, so there is no
       * danger of leaving random garbage, but our check has moved the
       * fwrite position to end of file; move it back to the position
       * set by the original seek request.
       */
      __mingw_fseeki64( fp, fwrite_pos, SEEK_SET );
  }
  /* Ultimately, complete the original fwrite request, at the expected
   * position within the output file.
   */
  return fwrite( buffer, size, count, fp );
}

/* $RCSfile$: end of file */
