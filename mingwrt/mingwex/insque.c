/*
 * insque.c
 *
 * POSIX.1-1996 compatible doubly-linked list management API; provides the
 * insque() function, for element insertion at an arbitrary position within
 * a linear, or circular, doubly-linked list.
 *
 *
 * $Id$
 *
 * Written by Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2018, MinGW.org Project.
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
#include <search.h>
#include <stddef.h>

/* This private structure definition specifies the minimal layout of a
 * doubly-linked list element; client code will typically append extra
 * data fields, but the first two fields must be pointers to identical
 * structures.
 */
struct qnode { struct qnode *fwdlink, *bkwdlink; };

/* Providing the actual implementation as an inline function, with its
 * arguments specified as pointers to structures of the preceding type,
 * offers a conventient mechanism for mapping the generic pointers of
 * the public API to the minimal required form.
 */
__CRT_ALIAS void do_insque( struct qnode *element, struct qnode *pred )
{
  /* Insert "element" into the list containing "pred", with insertion
   * point immediately following "pred"; becomes a no-op, if "element"
   * is specified as NULL...
   */
  if( element != NULL )
  {
    /* ...otherwise, sets the forward link of "element" to the current
     * forward link of "pred", (or to NULL, if "pred" is NULL), and the
     * backward link of the successor to "pred", if any, to point back
     * to "element"...
     */
    if( (element->fwdlink = (pred != NULL) ? pred->fwdlink : NULL) != NULL )
      element->fwdlink->bkwdlink = element;

    /* ...and completes the linking, by setting the backward link of
     * "element" to point to "pred", and the forward link of "pred",
     * (if "pred" isn't NULL), to point to "element".
     */
    if( (element->bkwdlink = pred) != NULL )
      pred->fwdlink = element;
  }
}

/* The public API simply expands the inline implementation, mapping
 * the generic pointer arguments to the minimally specified structure.
 */
void insque( void *element, void *pred )
{ do_insque( (struct qnode *)(element), (struct qnode *)(pred) ); }

/* $RCSfile$: end of file */
