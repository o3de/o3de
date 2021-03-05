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

#ifndef OPENMESH_BINARY_HELPER_HH
#define OPENMESH_BINARY_HELPER_HH


//== INCLUDES =================================================================

#include <OpenMesh/Core/System/config.h>
// -------------------- STL
#if defined( OM_CC_MIPS )
#  include <stdio.h>
#else
#  include <cstdio>
#endif
#include <iosfwd>
// -------------------- OpenMesh


//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace IO {


//=============================================================================


/** \name Handling binary input/output.
    These functions take care of swapping bytes to get the right Endian.
*/
//@{

//-----------------------------------------------------------------------------


/** Binary read a \c short from \c _is and perform byte swapping if
    \c _swap is true */
short int read_short(FILE* _in, bool _swap=false);

/** Binary read an \c int from \c _is and perform byte swapping if
    \c _swap is true */
int read_int(FILE* _in, bool _swap=false);

/** Binary read a \c float from \c _is and perform byte swapping if
    \c _swap is true */
float read_float(FILE* _in, bool _swap=false);

/** Binary read a \c double from \c _is and perform byte swapping if
    \c _swap is true */
double read_double(FILE* _in, bool _swap=false);

/** Binary read a \c short from \c _is and perform byte swapping if
    \c _swap is true */
short int read_short(std::istream& _in, bool _swap=false);

/** Binary read an \c int from \c _is and perform byte swapping if
    \c _swap is true */
int read_int(std::istream& _in, bool _swap=false);

/** Binary read a \c float from \c _is and perform byte swapping if
    \c _swap is true */
float read_float(std::istream& _in, bool _swap=false);

/** Binary read a \c double from \c _is and perform byte swapping if
    \c _swap is true */
double read_double(std::istream& _in, bool _swap=false);


/** Binary write a \c short to \c _os and perform byte swapping if
    \c _swap is true */
void write_short(short int _i, FILE* _out, bool _swap=false);

/** Binary write an \c int to \c _os and perform byte swapping if
    \c _swap is true */
void write_int(int _i, FILE* _out, bool _swap=false);

/** Binary write a \c float to \c _os and perform byte swapping if
    \c _swap is true */
void write_float(float _f, FILE* _out, bool _swap=false);

/** Binary write a \c double to \c _os and perform byte swapping if
    \c _swap is true */
void write_double(double _d, FILE* _out, bool _swap=false);

/** Binary write a \c short to \c _os and perform byte swapping if
    \c _swap is true */
void write_short(short int _i, std::ostream& _out, bool _swap=false);

/** Binary write an \c int to \c _os and perform byte swapping if
    \c _swap is true */
void write_int(int _i, std::ostream& _out, bool _swap=false);

/** Binary write a \c float to \c _os and perform byte swapping if
    \c _swap is true */
void write_float(float _f, std::ostream& _out, bool _swap=false);

/** Binary write a \c double to \c _os and perform byte swapping if
    \c _swap is true */
void write_double(double _d, std::ostream& _out, bool _swap=false);

//@}


//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_MESHREADER_HH defined
//=============================================================================

