/* ========================================================================= *
 *                                                                           *
 *                               OpenMesh                                    *
 *           Copyright (c) 2001-2015, RWTH-Aachen University                 *
 *           Department of Computer Graphics and Multimedia                  *
 *                          All rights reserved.                             *
 *                            www.openmesh.org                               *
 *                                                                           *
 *---------------------------------------------------------------------------*
 * This file is part of OpenMesh.                                            *
 *---------------------------------------------------------------------------*
 *                                                                           *
 * Redistribution and use in source and binary forms, with or without        *
 * modification, are permitted provided that the following conditions        *
 * are met:                                                                  *
 *                                                                           *
 * 1. Redistributions of source code must retain the above copyright notice, *
 *    this list of conditions and the following disclaimer.                  *
 *                                                                           *
 * 2. Redistributions in binary form must reproduce the above copyright      *
 *    notice, this list of conditions and the following disclaimer in the    *
 *    documentation and/or other materials provided with the distribution.   *
 *                                                                           *
 * 3. Neither the name of the copyright holder nor the names of its          *
 *    contributors may be used to endorse or promote products derived from   *
 *    this software without specific prior written permission.               *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       *
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED *
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A           *
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER *
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,  *
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,       *
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR        *
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    *
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      *
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        *
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              *
 *                                                                           *
 * ========================================================================= */




//=============================================================================
//
//  Helper Functions for binary reading / writing
//
//=============================================================================

#ifndef OPENMESH_SR_TYPES_HH
#define OPENMESH_SR_TYPES_HH


//== INCLUDES =================================================================

#include <OpenMesh/Core/System/config.h>


//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace IO {


//=============================================================================


/** \name Handling binary input/output.
    These functions take care of swapping bytes to get the right Endian.
*/
//@{

//-----------------------------------------------------------------------------

typedef unsigned char    uchar;
typedef unsigned short   ushort;
typedef unsigned long    ulong;

typedef signed char    int8_t;  typedef unsigned char      uint8_t;
typedef short          int16_t; typedef unsigned short     uint16_t;

// Int should be 32 bit on all archs.
// long is 32 under windows but 64 under unix 64 bit
typedef int            int32_t; typedef unsigned int       uint32_t;
#if defined(OM_CC_MSVC)
typedef __int64        int64_t; typedef unsigned __int64   uint64_t;
#else
typedef long long      int64_t; typedef unsigned long long uint64_t;
#endif

typedef float          float32_t;
typedef double         float64_t;

typedef uint8_t        rgb_t[3];
typedef uint8_t        rgba_t[4];

//@}


//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_MESHREADER_HH defined
//=============================================================================

