/*
 * This source file is part of the bstring string library.  This code was
 * written by Paul Hsieh in 2002-2010, and is covered by either the 3-clause 
 * BSD open source license or GPL v2.0. Refer to the accompanying documentation 
 * for details on usage and license.
 */
// Modifications copyright Amazon.com, Inc. or its affiliates

/*
 * bsafe.c
 *
 * This is an optional module that can be used to help enforce a safety
 * standard based on pervasive usage of bstrlib.  This file is not necessarily
 * portable, however, it has been tested to work correctly with Intel's C/C++
 * compiler, WATCOM C/C++ v11.x and Microsoft Visual C++.
 */

#include <stdio.h>
#include <stdlib.h>
#include "bsafe.h"
