/*
 * remque.c
 *
 * POSIX.1-1996 compatible doubly-linked list management API; provides the
 * remque() function, for removal of an element from an arbitrary position
 * within a linear, or circular, doubly-linked list.
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
 * argument specified as a pointer to a structure of the preceding type,
 * offers a conventient mechanism for mapping the generic pointer of the
 * public API to the minimal required form.
 */
__CRT_ALIAS void do_remque( struct qnode *element )
{
  /* Remove "element" from the list which contains it; becomes a
   * no-op, if "element" is NULL...
   */
  if( element != NULL )
  {
    /* ...otherwise, updates the backward link in the successor of
     * "element", if any, to point to the predecessor of "element"...
     */
    if( element->fwdlink != NULL )
      element->fwdlink->bkwdlink = element->bkwdlink;

    /* ...and the forward link in the predecessor of "element", if
     * any, to point to the successor of "element".
     */
    if( element->bkwdlink != NULL )
      element->bkwdlink->fwdlink = element->fwdlink;
  }
}

/* The public API simply expands the inline implementation, mapping
 * the generic pointer argument to the minimally specified structure.
 */
void remque( void *element )
{ do_remque( (struct qnode *)(element) ); }

/* $RCSfile$: end of file */
