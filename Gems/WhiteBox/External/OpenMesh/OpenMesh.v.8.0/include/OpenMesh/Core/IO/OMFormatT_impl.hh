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


#define OPENMESH_IO_OMFORMAT_CC


//== INCLUDES =================================================================

#include <OpenMesh/Core/IO/OMFormat.hh>
#include <algorithm>
#include <iomanip>

//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace IO {

  // helper to store a an integer
  template< typename T > 
  size_t 
  store( std::ostream& _os, 
	 const T& _val, 
	 OMFormat::Chunk::Integer_Size _b, 
	 bool _swap,
	 t_signed)
  {    
    assert( OMFormat::is_integer( _val ) );

    switch( _b ) 
    {
      case OMFormat::Chunk::Integer_8:
      { 	
	OMFormat::int8 v = static_cast<OMFormat::int8>(_val);
	return store( _os, v, _swap );
      }
      case OMFormat::Chunk::Integer_16:
      { 
	OMFormat::int16 v = static_cast<OMFormat::int16>(_val);
	return store( _os, v, _swap );
      }
      case OMFormat::Chunk::Integer_32:
      { 
	OMFormat::int32 v = static_cast<OMFormat::int32>(_val);
	return store( _os, v, _swap );
      }      
      case OMFormat::Chunk::Integer_64:
      { 
	OMFormat::int64 v = static_cast<OMFormat::int64>(_val);
	return store( _os, v, _swap );
      }
    }
    return 0;
  }


  // helper to store a an unsigned integer
  template< typename T > 
  size_t 
  store( std::ostream& _os, 
	 const T& _val, 
	 OMFormat::Chunk::Integer_Size _b, 
	 bool _swap,
	 t_unsigned)
  {    
    assert( OMFormat::is_integer( _val ) );

    switch( _b ) 
    {
      case OMFormat::Chunk::Integer_8:
      { 
	OMFormat::uint8 v = static_cast<OMFormat::uint8>(_val);
	return store( _os, v, _swap );
      }
      case OMFormat::Chunk::Integer_16:
      { 
	OMFormat::uint16 v = static_cast<OMFormat::uint16>(_val);
	return store( _os, v, _swap );
      }
      case OMFormat::Chunk::Integer_32:
      { 
	OMFormat::uint32 v = static_cast<OMFormat::uint32>(_val);
	return store( _os, v, _swap );
      }
      
      case OMFormat::Chunk::Integer_64:
      { 
	OMFormat::uint64 v = static_cast<OMFormat::uint64>(_val);
	return store( _os, v, _swap );
      }
    }
    return 0;
  }


  // helper to restore a an integer
  template< typename T > 
  size_t 
  restore( std::istream& _is, 
	   T& _val, 
	   OMFormat::Chunk::Integer_Size _b, 
	   bool _swap,
	   t_signed)
  {    
    assert( OMFormat::is_integer( _val ) );
    size_t bytes = 0;

    switch( _b ) 
    {
      case OMFormat::Chunk::Integer_8:
      { 	
	OMFormat::int8 v;
	bytes = restore( _is, v, _swap );
	_val = static_cast<T>(v);
	break;
      }
      case OMFormat::Chunk::Integer_16:
      { 
	OMFormat::int16 v;
	bytes = restore( _is, v, _swap );
	_val = static_cast<T>(v);
	break;
      }
      case OMFormat::Chunk::Integer_32:
      { 
	OMFormat::int32 v;
	bytes = restore( _is, v, _swap );
	_val = static_cast<T>(v);
	break;
      }      
      case OMFormat::Chunk::Integer_64:
      { 
	OMFormat::int64 v;
	bytes = restore( _is, v, _swap );
	_val = static_cast<T>(v);
	break;
      }
    }
    return bytes;
  }


  // helper to restore a an unsigned integer
  template< typename T > 
  size_t 
  restore( std::istream& _is, 
	   T& _val, 
	   OMFormat::Chunk::Integer_Size _b, 
	   bool _swap,
	   t_unsigned)
  {    
    assert( OMFormat::is_integer( _val ) );
    size_t bytes = 0;

    switch( _b ) 
    {
      case OMFormat::Chunk::Integer_8:
      { 
	OMFormat::uint8 v;
	bytes = restore( _is, v, _swap );
	_val = static_cast<T>(v);
	break;
      }
      case OMFormat::Chunk::Integer_16:
      { 
	OMFormat::uint16 v;
	bytes = restore( _is, v, _swap );
	_val = static_cast<T>(v);
	break;
      }
      case OMFormat::Chunk::Integer_32:
      { 
	OMFormat::uint32 v;
	bytes = restore( _is, v, _swap );
	_val = static_cast<T>(v);
	break;
      }
      
      case OMFormat::Chunk::Integer_64:
      { 
	OMFormat::uint64 v;
	bytes = restore( _is, v, _swap );
	_val = static_cast<T>(v);
	break;
      }
    }
    return bytes;
  }

//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
