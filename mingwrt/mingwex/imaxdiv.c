/*
 * imaxdiv.c
 *
 * Implements the imaxdiv() function, as specified in ISO/IEC 9899:1999
 * clause 7.8.2.2, and its functionally equivalent lldiv() function, as
 * specified in ISO/IEC 9899:1999 clause 7.20.6.2.
 *
 * $Id$
 *
 * Written by Doug Gwyn <gwyn@arl.mil>
 * Copyright (C) 1999, 2018, MinGW.org Project.
 *
 *
 * Abstracted from the Q8 package, which was originally placed, by the
 * above named author, in the PUBLIC DOMAIN.  In any jurisdiction where
 * PUBLIC DOMAIN is not acceptable as a licensing waiver, the following
 * licence shall apply:
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
#include <inttypes.h>

imaxdiv_t imaxdiv (intmax_t numer, intmax_t denom)
{
  imaxdiv_t result;
  result.quot = numer / denom;
  result.rem = numer % denom;
  return result;
}

/* lldiv() is effectively equivalent to imaxdiv(), so we may implement
 * it as an alias.  However, the two function prototypes differ in the
 * formal data types of their arguments, and return values.  Although
 * these differing data types are effectively interchangeable, GCC may
 * not recognize this, so disable associated warnings.
 */
#pragma GCC diagnostic ignored "-Wattribute-alias"

#include <stdlib.h>

lldiv_t __attribute__ ((alias ("imaxdiv"))) lldiv (long long, long long);

/* $RCSfile$: end of file */
