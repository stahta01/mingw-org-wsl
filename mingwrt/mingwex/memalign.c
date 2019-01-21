/*
 * memalign.c
 *
 * MinGW.org retrofit replacements for Microsoft's aligned heap memory
 * management APIs, extending related functionality to legacy versions
 * of Windows, which lack native support for these APIs.
 *
 *
 * $Id$
 *
 * Written by Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2018, 2019, MinGW.org Project
 *
 * Derived (with extensive modification) from, and replacing, the original
 * mingw-aligned-malloc.c implementation:
 *
 * Written by Steven G. Johnson <stevenj@alum.mit.edu>
 * Copyright (C) 2004, MinGW.org Project
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
 * This translation unit furnishes common source for multiple components of
 * the MinGW.org aligned heap memory management API; to create each separate
 * component, it must be compiled multiple times, e.g. thus:
 *
 *   gcc -c -D__mingw_memalign_lwm_case -o memalign-lwm.o memalign.c
 *   gcc -c -D__mingw_memalign_base_case -o memalign-base.o memalign.c
 *   gcc -c -D__mingw_aligned_malloc_case -o aligned-malloc.o memalign.c
 *   gcc -c -D__mingw_memalign_realloc_case -o memalign-realloc.o memalign.c
 *   gcc -c -D__mingw_aligned_realloc_case -o aligned-realloc.o memalign.c
 *   gcc -c -D__mingw_realloc_case -o mingw-realloc.o memalign.c
 *   gcc -c -D__mingw_free_case -o mingw-free.o memalign.c
 *
 */
#include <malloc.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>

/* Regardless of whatever may have been inherited from the preceding
 * headers, ensure that we can use "dllimport" semantics when referring
 * to specific MSVCRT.DLL functions.
 */
#undef  DLL_IMPORT
#define DLL_IMPORT  __declspec(dllimport)

/* Alignment must always be specified as an integer power of two; this pair
 * of macros facilitate verification of this, assigning appropriate "errno"
 * values, and bailing out, in the event of violation of this, or any other
 * parameter validation criterion.
 */
#define is_power_of_two(x)  memalign_is_power_of_two((x))
#define error_return(c,r)  {errno = (c); return (r);}

/* We need "sizeof_ptr" to represent the smallest integer power of two
 * which is greater than or equal to "sizeof (void *)".  On _WIN32, and
 * on _WIN64, "sizeof (void *)" itself is 4 bytes, or 8 bytes respectively;
 * since both of these are integer powers of two anyway, we may safely:
 */
#define sizeof_ptr  sizeof (void *)

struct memalign
{ /* A convenience structure, encapsulating the data elements which are
   * recorded within an alignment header.  Note that the layout of this
   * structure does not mimic the alignment header layout, (which isn't
   * fixed), but it does incorporate all of the data elements which may
   * be stored therein.
   */
  union
  { /* Mapping the base pointer into this union makes it convenient to
     * access its equivalent numeric value, without casting.
     */
    void	*ptr;
    uintptr_t	 ref;
  };
  unsigned int	 flags;
  size_t	 alignment;
  size_t	 offset;
};

/* Within the alignment header itself, the flags are encoded as a 2-bit
 * field within the base pointer value; the following macros facilitate
 * extraction, manipulation, and interpretation of the flags.
 */
#define MEMALIGN_FLAG(INDEX)	   (1 << (INDEX))

#define MEMALIGN_OVER_ALIGNED     MEMALIGN_FLAG (0)
#define MEMALIGN_OFFSET_ALIGNED   MEMALIGN_FLAG (1)

#define MEMALIGN_FLAGS_MASK	 (MEMALIGN_FLAG (2) - 1)

/* The following functions are considered private to the implementation,
 * but they must expose a public interface, to make them accessible to
 * discrete API components; declare privately visible prototypes.
 */
void *__mingw_memalign_base (void *, struct memalign *);
void *__mingw_memalign_realloc( void *, struct memalign *, size_t );

/* Other helper functions, which facilitate communication between
 * discrete API components, are conveniently implemented as in-line
 * functions; (declare them as __CRT_ALIAS, to ensure that GCC will
 * always expand them in-line).
 */
__CRT_ALIAS int memalign_is_power_of_two( int x )
{
  /* This furnishes the implementation of the is_power_of_two() macro,
   * as an in-line function expansion, to avoid GCC warnings which may
   * arise as side effects of passing an assignment as macro parameter.
   */
  return ((x > 0) && ((x & (x -1)) == 0));
}

__CRT_ALIAS size_t memalign_min_alignment( void )
{
  /* This determines the size of a structure comprising a single byte
   * element, followed by sufficient padding to extend to the alignment
   * boundary on which a following element of any fundamental data type
   * would be correctly aligned.  This should match the alignment of a
   * heap block allocated by malloc(), (which Microsoft documents as
   * eight bytes on Win32, or sixteen bytes on Win64).
   *
   * Since this function effectively returns a fixed value, GCC should
   * optimize calls away, simply substituting the compile-time constant
   * equivalent value.
   */
  struct memalign_min_alignment
  { char c; union { void *p; long long l; intptr_t i; long double f; } a[];
  };
  /* Structure alignment should normally exceed the size of a pointer;
   * nonetheless, we will reject any lesser result.
   */
  return (sizeof (struct memalign_min_alignment) > sizeof_ptr)
    ? sizeof (struct memalign_min_alignment) : sizeof_ptr;
}

__CRT_ALIAS size_t memalign_normalized( size_t alignment )
{
  /* Normalize an alignment specification, rejecting any value which
   * is less than the minimum determined by the preceding function, in
   * favour the least integer power of two which is not less than the
   * preceding minimum.
   *
   * Note that any value which exceeds the preceding minimum will not
   * be modified; thus, any such value must have been confirmed as an
   * integer power of two, before normalization.
   */
  if(  (alignment < memalign_min_alignment())
  &&  !is_power_of_two( alignment = memalign_min_alignment())  )
  {
    /* Here we ensure that the normalized value is an integer power of
     * two; with expected values, this should already be the case, so
     * GCC may be expected to optimize this away.
     */
    alignment = is_power_of_two( sizeof_ptr ) ? sizeof_ptr : 2;
    while( alignment < memalign_min_alignment() ) alignment <<= 1;
  }
  return alignment;
}

__CRT_ALIAS size_t memalign_padding( unsigned int flags, size_t alignment )
{
  /* Compute the size of the padding field which must be added to any
   * malloc() allocation request, to accommodate an alignment header,
   * and achieve the required alignment for the following data block.
   */
  alignment += sizeof_ptr - 1 + sizeof (size_t);
  if( (flags & MEMALIGN_FLAGS_MASK) == MEMALIGN_FLAGS_MASK )
    alignment += sizeof (size_t);
  return alignment;
}

/* Helper to set the address of the data block, at the appropriately
 * aligned offset within the preceding padding field; (this represents
 * the greatest suitably aligned address within the padding field, and
 * may truncate an over-specified padding field, relocating the excess
 * padding to the end of the data block allocation).
 */
__CRT_ALIAS uintptr_t aligned( void *ptr, size_t alignment, int offset )
{ return (((uintptr_t)(ptr) + offset) & ~(alignment - 1)); }

__CRT_ALIAS
/* Helper to compute the resultant pointer, with specified alignment,
 * offset, and padding, aligned w.r.t. a specified base "ptr".
 */
void *aligned_ptr( void *ptr, size_t alignment, int offset, size_t padding )
{ return (void *)(aligned( ptr, alignment, offset + padding ) - offset); }

/* Tracking storage for the aligned heap's "low water mark"; technically,
 * this represents the lowest valued pointer assigned by malloc(), or maybe
 * as adjusted by realloc(), for any over-aligned, or offset-aligned block,
 * but for convenience, we store it as its equivalent numeric value.
 */
extern uintptr_t __mingw_memalign_lwm;

__CRT_ALIAS void record_low_water_mark( void *ptr )
{ /* Helper function to track the numerically lowest heap block address
   * allocated for over-aligned, or offset-aligned use.  (Note that this
   * function should never be passed a NULL value for "ptr"; this should
   * never happen, in any context where we call it, so we choose to omit
   * a redundant test for ptr != NULL).
   */
  if( (__mingw_memalign_lwm == 0) || ((uintptr_t)(ptr) < __mingw_memalign_lwm) )
    __mingw_memalign_lwm = (uintptr_t)(ptr);
}

/* The following two prototypes declare aliases for the MSVCRT.DLL
 * implementations of free(), and realloc() respectively; users are
 * expected to call these, via their unaliased names, but we need
 * these aliases here, so that we may access the MSVCRT.DLL code
 * from within our own replacement function stubs.
 */
DLL_IMPORT __cdecl __MINGW_NOTHROW  void  __msvcrt_free (void *);
DLL_IMPORT __cdecl __MINGW_NOTHROW  void *__msvcrt_realloc (void *, size_t );

#if __mingw_memalign_lwm_case
/* Reserve the "low water mark" tracking storage; initialize it to zero,
 * which is invalid as a heap block address, indicating that no block has
 * yet been allocated with an alignment header, and thus, the "low water
 * mark" has yet to be recorded.
 */
uintptr_t __mingw_memalign_lwm = 0;

#elif __mingw_memalign_base_case
/* A private API component, providing support for identification and
 * interpretation of alignment headers.
 */
struct memalign_hdr
{ /* Structure representing the layout of a minimally sized alignment
   * header; this is used only to establish the minimum size which it
   * is necessary to allocate for such a header, (comprising at least
   * one alignment property element...
   */
  size_t  align[1 /* maybe 2, but 1 is minimum */];
  /*
   * ...followed by zero or one additional alignment property values,
   * zero or more arbitrary padding bytes, and finally, a reference
   * pointer to the base address of the alignment header itself).
   */
  void	 *base;
};

/* Some convenient abbreviations...
 */
#define lwm __mingw_memalign_lwm
#define sizeof_hdr sizeof (struct memalign_hdr)

/* ...including shorthand notation for the aligned_ptr() argument
 * list, as it should be passed from within __mingw_memalign_base().
 */
#define offset_aligned(base)  base->alignment, offset_padded(base)
#define offset_padding(base)  memalign_padding( base->flags, base->alignment )
#define offset_padded(base)   base->offset, offset_padding(base)

void *__mingw_memalign_base( void *ptr, struct memalign *base )
{
  /* Helper to retrieve the base address of a possibly over-aligned,
   * or offset-aligned heap memory pointer; checks for the presence of
   * an alignment header, preceding "ptr" in memory, verifies that any
   * such header includes a valid alignment reference to "ptr", before
   * returning the base address of that alignment header, or otherwise
   * returning the original pointer, unchanged.
   *
   * As a special case, if "ptr" is NULL, there can be no associated
   * alignment header, but any attempt to identify one would induce a
   * segmentation fault; thus we immediately return the NULL pointer,
   * without further evaluation.
   */
  if( ptr == NULL ) return NULL;

  /* In the normal case, when "ptr" is non-NULL, then it may represent
   * an over-aligned, or an offset-aligned allocation, only if the "low
   * water mark" address for such allocations has been established, and
   * "ptr" itself refers to an address which is sufficiently far "above"
   * this "low water mark" to accommodate a mimimal alignment header.
   */
  if( (lwm > 0) && ((uintptr_t)(ptr) >= (lwm + sizeof_hdr)) )
  {
    /* When the "above low water mark" criterion is satisfied, we may
     * safely extract a potential base pointer, and its associated flags,
     * from the address space in which the containing alignment header
     * would have been stored.
     */
    base->ref = *((uintptr_t *)(aligned( ptr, sizeof_ptr, -sizeof_ptr )));
    base->ref ^= (base->flags = base->ref & MEMALIGN_FLAGS_MASK);

    /* Any such base pointer must represent an address which lies within
     * the interval between the low water mark and the minimum size of an
     * alignment header below the address represented by "ptr".
     */
    if( (base->ref >= lwm) && (((uintptr_t)(ptr) - sizeof_hdr) >= base->ref) )
    {
      /* When this tentatively determined base pointer does represent an
       * address within the expected interval, then we may also retrieve
       * the alignment parameters which would have been stored within an
       * alignment header, at that address...
       */
      base->alignment = ((base->flags & MEMALIGN_OVER_ALIGNED) == 0)
	? memalign_min_alignment() : *((size_t *)(base->ptr));
      base->offset = ((base->flags & MEMALIGN_OFFSET_ALIGNED) == 0)
	? 0 : ((size_t *)(base->ptr))[(1 + base->flags) >> 2];

      /* ...and finally, we must verify that the combination of this base
       * pointer, and alignment parameters, as stored within this possible
       * alignment header, represent the same aligned address as "ptr", in
       * which case we return this base pointer...
       */
      if( aligned_ptr( base->ptr, offset_aligned( base )) == ptr )
	return base->ptr;
    }
  }
  /* ...otherwise, "ptr" does not statisfy the criteria for identification
   * as an aligned heap reference, so we return the specified, and possibly
   * unaligned, original pointer.
   */
  return ptr;
}

#elif __mingw_aligned_malloc_case
/* The fundamental handler for all new aligned heap allocation requests;
 * all MinGW.org aligned allocators, (but not re-allocators), should call
 * this, to obtain a heap block which will fulfil the request.
 */
void *__mingw_aligned_offset_malloc( size_t want, size_t align, size_t offset )
{
  /* MinGW.org replacement for Microsoft's _aligned_offset_malloc(); if
   * called with an "offset" argument of zero, it may also be used as an
   * effective replacement for Microsoft's _aligned_malloc(), and with
   * appropriate wrappers, to map out argument distinctions, it may even
   * deliver functionality which is equivalent to posix_memalign(), or to
   * ISO C11's aligned_alloc() functions.
   */
  void *retptr; struct memalign base;

  /* Alignment MUST be an integer power of two, and offset, if specified
   * as non-zero, MUST be less than the requested allocation.
   */
  if( !(is_power_of_two( align ) && ((offset == 0) || (want > offset))) )
    error_return( EINVAL, NULL );

  /* Set flags to identify what parameters, if any, must be recorded in
   * an alignment header for this allocation.
   */
  base.flags = (offset != 0) ? MEMALIGN_OFFSET_ALIGNED : 0;
  if( (align = memalign_normalized( align )) > memalign_min_alignment() )
    base.flags |= MEMALIGN_OVER_ALIGNED;

  /* Only requests which specify an alignment greater than the fundamental
   * minimum, and/or a non-zero offset, actually need an alignment header;
   * otherwise, an unadorned malloc() request is sufficient.
   */
  if( base.flags == 0 ) return malloc( want );

  /* For requests which do need an alignment header, estimate the amount
   * by which the request must be padded, to accommodate the header.
   */
  base.alignment = memalign_padding( base.flags, align );

  /* Request allocation of an appropiately padded data block.  Bail out
   * on failure; on success, compute the properly aligned effective data
   * pointer, within the allocated block, for return, and ensure that the
   * recorded "low water mark" for the aligned heap is correctly adjusted,
   * relative to this allocation.
   */
  if( (base.ptr = malloc( want + base.alignment )) == NULL ) return NULL;
  retptr = aligned_ptr( base.ptr, align, offset, base.alignment );
  record_low_water_mark( base.ptr );

  /* For over-aligned data, (i.e. alignment boundary is greater than the
   * fundamental minimum), store the alignment as the first entry within
   * the alignment header.
   */
  if( (base.flags & MEMALIGN_OVER_ALIGNED) == MEMALIGN_OVER_ALIGNED )
    ((size_t *)(base.ptr))[0] = align;

  /* For offset-aligned data, store the offset immediately following the
   * alignment, if it was stored, or otherwise as the first entry within
   * the alignment header.
   */
  if( (base.flags & MEMALIGN_OFFSET_ALIGNED) == MEMALIGN_OFFSET_ALIGNED )
    ((size_t *)(base.ptr))[(base.flags & MEMALIGN_OVER_ALIGNED) ? 1 : 0] = offset;

  /* Fold the alignment flags into the allocation pointer...
   */
  base.ref |= base.flags;

  /* ...and store the resultant value at the greatest address boundary,
   * which is suitably aligned for pointer storage, below the effective
   * data pointer which is to be returned.
   */
  *(void **)(aligned( retptr, sizeof_ptr, -sizeof_ptr )) = base.ptr;

  /* Ultimately, return the computed address of the effective data area,
   * within the allocated heap memory block.
   */
  return retptr;
}

#elif __mingw_aligned_realloc_case
/* The first of two public entry points, for access to the MinGW.org
 * aligned heap memory reallocation API.  Most applications should use
 * the alternative entry point, as specified below, but this entry point
 * may be preferred when strict semantic compatibility with Microsoft's
 * aligned heap reallocation API is desired.
 */
void *__mingw_aligned_offset_realloc
( void *ptr, size_t want, size_t align, size_t offset )
{
  /* MinGW.org replacement for Microsoft's _aligned_offset_realloc();
   * also suitable as a replacement for Microsoft's _aligned_realloc(),
   * when called with an "offset" argument of zero.
   */
  if( ptr != NULL )
  { /* When called with a non-NULL "ptr" argument, this is expected
     * to be associated with an alignment header, and certain argument
     * validation prerequisites must be fulfilled.
     */
    do { struct memalign base;
	 /* Any "break" out of this block indicates violation
	  * of some argument validation constraint.
	  *
	  * Irrespective of any other prerequisite, the "align"
	  * argument must represent an integer power of two.
	  */
	 if( ! is_power_of_two( align ) ) break;

	 if( __mingw_memalign_base( ptr, &base ) == ptr )
	 {
	   /* When the specified "ptr" argument is NOT associated
	    * with an alignment header, then the "offset" argument
	    * MUST be specified as zero, and the specified "align"
	    * argument may be no greater than minimum alignment
	    * for any fundamental data type.
	    */
	   if( offset != 0 ) break;
	   if( memalign_normalized( align ) > memalign_min_alignment() )
	     break;

	   /* Strictly, the "align" argument should be identically
	    * equal to that passed to the _aligned_offset_malloc()
	    * call, which created the allocation at "ptr", but we
	    * have no way to verify this; assume it is okay, and
	    * reallocate on a fundamental alignment boundary.
	    */
	   return __msvcrt_realloc( ptr, want );
	 }
	 /* If we get this far, then the power of two constraint on
	  * "align" is satisfied, and we do have an alignment header;
	  * "align" and "offset" arguments MUST now identically match
	  * their corresponding specifications within the alignment
	  * header, and the "want" request MUST either be zero, or
	  * it MUST exceed "offset".
	  */
	 if( memalign_normalized( align ) != base.alignment ) break;
	 if( (offset != base.offset) || ((want > 0) && (offset >= want)) )
	   break;

	 /* All argument constraints are satisfied; provided the "want"
	  * size request is greater than zero, hand off this request to
	  * the aligned memory reallocator...
	  */
	 return (want > 0) ? __mingw_memalign_realloc( ptr, &base, want )
	   /*
	    * ...while, for zero "want" requests, we preserve Microsoft
	    * compatibility, (which is documented as freeing the block),
	    * by handing off the base pointer to the realloc() API.
	    */
	   : __msvcrt_realloc( base.ptr, want );
       } while( 0 );

    /* The only way to get to here, is by breaking out of the preceding
     * block, on detection of an argument constraint violation.
     */
    error_return( EINVAL, NULL );
  }
  /* When called with a NULL "ptr" argument, this becomes an effective
   * equivalent for MinGW.org's __mingw_aligned_offset_realloc().
   */
  return __mingw_aligned_offset_malloc( want, align, offset );
}

#elif __mingw_realloc_case
/* The second, and nominally preferred, of two public entry points to the
 * MinGW.org aligned heap memory reallocation API.  This exhibits semantics
 * matching those of realloc(), while supporting operation on both pointers
 * as returned by __mingw_aligned_offset_malloc(), in addition to pointers
 * as returned directly by malloc(); thus, it offers a broader spectrum of
 * compatibility with non-Microsoft memory allocation stratagems.
 */
void *__mingw_realloc( void *ptr, size_t want )
{ /* An alternative to __mingw_aligned_offset_realloc(), (and implicitly
   * also to __mingw_aligned_realloc()), for resizing aligned heap memory
   * blocks; checks for the presence of a valid MinGW specific alignment
   * control block, immediately preceding "*ptr", before proceeding with
   * aligned reallocation, using the "alignment" and "offset" values as
   * recorded within any such control block; in the absence of any such
   * valid alignment control block, this falls back to become a simple
   * call to Microsoft's realloc(), on "*ptr" directly, and thus also
   * supports resizing of blocks allocated by malloc(), or calloc().
   */
  if( ptr != NULL )
  { /* When passed a non-NULL pointer, an associated alignment header
     * will provide alignment specifications...
     */
    struct memalign base;
    if( __mingw_memalign_base( ptr, &base ) != ptr )
    {
      /* ...but these are relevant only for new size requests which
       * specify a wanted size greater than zero...
       */
      if( want > 0 )
      { /* ...in which case, the argument constraints which affect
	 * __mingw_aligned_offset_realloc() are implicitly satisfied,
	 * with the exception that the new size MUST remain greater
	 * than the original "offset"...
	 */
	if( base.offset >= want ) error_return( EINVAL, NULL );

	/* ...so, provided this is satisfied, we may hand off the
	 * request, to the MinGW.org aligned heap reallocator.
	 */
	return __mingw_memalign_realloc( ptr, &base, want );
      }
      /* If we get to here, the new size request is specified as zero.
       * Microsoft documents that, in this case, the memory allocation
       * associated with "ptr" will be freed!  Ideally, no user should
       * ever write code which relies on such anomalous behaviour, (the
       * preferred choice would be to call __mingw_aligned_free()), but
       * we will preserve Microsoft compatibility, by forwarding the
       * associated base pointer to Microsoft's realloc() API.
       */
      ptr = base.ptr;
    }
  }
  /* For the cases of "ptr" being NULL, or "want" being zero, or there
   * being no alignment header associated with "ptr", we fall through
   * to handle this as a regular realloc() request.
   */
  return __msvcrt_realloc( ptr, want );
}

#elif __mingw_memalign_realloc_case
/* Core implementation for the MinGW.org aligned heap memory reallocator;
 * provides the common component of the reallocator, shared by each of the
 * preceding user visible APIs, and, although offering a publicly exposed
 * entry point, it is considered private to the implementation.
 *
 * In addition to the header files enumerated previously, this requires
 * <string.h>, to obtain a declaration for the memmove() API.
 */
#include <string.h>

void *__mingw_memalign_realloc( void *ptr, struct memalign *base, size_t want )
{
  /* Complete a reallocation request, which is subject to over-alignment
   * or offset-alignment, as specified by the passed alignment structure.
   *
   * Begin by making a note of the original block size, then compute the
   * amount of padding by which the new size request must be augmented to
   * accommodate the alignment header, and request the reallocation.
   */
  size_t oldsize = _msize( base->ptr );
  size_t padding = memalign_padding( base->flags, base->alignment );
  void *retptr = __msvcrt_realloc( base->ptr, want + padding );

  /* If the reallocation was accomplished without any change in the base
   * address, then both the original data pointer, and original alignment
   * header remain valid, and in effect: no further action is needed.
   */
  if( retptr == base->ptr ) return ptr;

  /* Conversely, provided the reallocation was successful, (as indicated
   * by a non-NULL return pointer)...
   */
  if( retptr != NULL )
  { /* ...the realloc() operation will have copied the original data, and
     * the associated alignment header.  The "alignment", and the "offset"
     * values, within the header, will remain valid, but the base pointer
     * has changed, so we must update the header's record of this, and we
     * must recompute the data pointer, which must also have changed, for
     * return.  We must also note that; while the original data will have
     * been copied, without change, it may no longer be correctly aligned
     * within the reallocated block, so we must be prepared to reposition
     * it, for correct alignment relative to the new base pointer.
     */
    ptrdiff_t shift = (uintptr_t)(ptr) - base->ref;
    record_low_water_mark( ptr = base->ptr = retptr ); base->ref |= base->flags;
    retptr = aligned_ptr( ptr, base->alignment, base->offset, padding );

    /* We've now updated "ptr" to match the new base pointer, and "shift"
     * records the offset between the old base pointer, and its associated
     * data pointer; if the new base pointer, plus "shift" does not match
     * the newly computed data pointer...
     */
    if( (void *)((uintptr_t)(ptr) + shift) != retptr )
    {
      /* ...the copied data is NOT correctly positioned within the newly
       * allocated memory block; we must relocate the copied data, up to
       * the lesser of the original and new data lengths, such that its
       * starting point coincides with the new data pointer.
       */
      if( want > (oldsize -= shift) ) want = oldsize;
      memmove( retptr, (char *)(ptr) + shift, want );
    }
    /* Finally, we update the base pointer and flags record, within the
     * alignment header...
     */
    *(void **)(aligned( retptr, sizeof_ptr, -sizeof_ptr )) = base->ptr;
  }
  /* ...and return the new data pointer, (or NULL, if realloc() failed).
   */
  return retptr;
}

#elif __mingw_free_case
/* The MinGW.org API for freeing allocated heap memory, regardless of
 * the original method of allocation.
 */
void __mingw_free( void * )__attribute__((alias("__mingw_aligned_free")));
void __mingw_aligned_free( void *ptr )
{ /* Free heap memory allocated by malloc(), calloc(), or any associate
   * of __mingw_aligned_offset_malloc(); unlike Microsoft's free() API,
   * this checks for the presence of a MinGW specific alignment control
   * block, immediately preceding "*ptr", and promotes itself to become
   * the MinGW equivalent of Microsoft's _aligned_free(), if necessary.
   */
  struct memalign base;__msvcrt_free(__mingw_memalign_base( ptr, &base ));
}
#endif

/* $RCSfile$: end of file */
