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

#ifndef OPENMESH_SR_BINARY_HH
#define OPENMESH_SR_BINARY_HH


//== INCLUDES =================================================================

#include <OpenMesh/Core/System/config.h>
// -------------------- STL
#include <typeinfo>
#include <stdexcept>
#include <sstream>
#include <numeric>   // accumulate
// -------------------- OpenMesh


//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace IO {


//=============================================================================


//-----------------------------------------------------------------------------

  const static size_t UnknownSize(size_t(-1));


//-----------------------------------------------------------------------------
// struct binary, helper for storing/restoring

#define X \
   std::ostringstream msg; \
   msg << "Type not supported: " << typeid(value_type).name(); \
   throw std::logic_error(msg.str())

/// \struct binary SR_binary.hh <OpenMesh/Core/IO/SR_binary.hh>
///
/// The struct defines how to store and restore the type T.
/// It's used by the OM reader/writer modules.
///
/// The following specialization are provided:
/// - Fundamental types except \c long \c double
/// - %OpenMesh vector types
/// - %OpenMesh::StatusInfo
/// - std::string (max. length 65535)
///
/// \todo Complete documentation of members
template < typename T > struct binary
{
  typedef T     value_type;

  static const bool is_streamable = false;

  static size_t size_of(void) { return UnknownSize; }
  static size_t size_of(const value_type&) { return UnknownSize; }

  static 
  size_t store( std::ostream& /* _os */,
		const value_type& /* _v */,
		bool /* _swap=false */)
  { X; return 0; }

  static 
  size_t restore( std::istream& /* _is */,
		  value_type& /* _v */,
		  bool /* _swap=false */)
  { X; return 0; }
};

#undef X


//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_SR_RBO_HH defined
//=============================================================================

