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

//== INCLUDES =================================================================

#include <OpenMesh/Core/IO/OMFormat.hh>

//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace IO {
namespace OMFormat {

//== IMPLEMENTATION ===========================================================

  Chunk::Integer_Size needed_bits( size_t s )
  {
    if (s <= 0x000100) return Chunk::Integer_8;
    if (s <= 0x010000) return Chunk::Integer_16;

#if 0
    // !Not tested yet! This most probably won't work!
    // NEED a 64bit system!
    if ( (sizeof( size_t ) == 8) && (s >= 0x100000000) )
      return Chunk::Integer_64;
#endif

    return Chunk::Integer_32;    
  }

//-----------------------------------------------------------------------------

  uint16& 
  operator << (uint16& val, const Chunk::Header& hdr)
  {
    val = 0;
    val |= hdr.name_   << OMFormat::Chunk::OFF_NAME;
    val |= hdr.entity_ << OMFormat::Chunk::OFF_ENTITY;
    val |= hdr.type_   << OMFormat::Chunk::OFF_TYPE;  
    val |= hdr.signed_ << OMFormat::Chunk::OFF_SIGNED;
    val |= hdr.float_  << OMFormat::Chunk::OFF_FLOAT; 
    val |= hdr.dim_    << OMFormat::Chunk::OFF_DIM;   
    val |= hdr.bits_   << OMFormat::Chunk::OFF_BITS;  
    return val;
  }


//-----------------------------------------------------------------------------

  Chunk::Header&
  operator << (Chunk::Header& hdr, const uint16 val)
  {
    hdr.reserved_ = 0;
    hdr.name_     = val >> OMFormat::Chunk::OFF_NAME;
    hdr.entity_   = val >> OMFormat::Chunk::OFF_ENTITY;
    hdr.type_     = val >> OMFormat::Chunk::OFF_TYPE;
    hdr.signed_   = val >> OMFormat::Chunk::OFF_SIGNED;
    hdr.float_    = val >> OMFormat::Chunk::OFF_FLOAT;
    hdr.dim_      = val >> OMFormat::Chunk::OFF_DIM;
    hdr.bits_     = val >> OMFormat::Chunk::OFF_BITS;
    return hdr;
  }

//-----------------------------------------------------------------------------


  std::string as_string(uint8 version)
  {
    std::stringstream ss;
    ss << major_version(version);
    ss << ".";
    ss << minor_version(version);
    return ss.str();
  }


//-----------------------------------------------------------------------------

  const char *as_string(Chunk::Entity e)
  {
    switch(e)
    {    
      case Chunk::Entity_Vertex:   return "Vertex";
      case Chunk::Entity_Mesh:     return "Mesh";
      case Chunk::Entity_Edge:     return "Edge";
      case Chunk::Entity_Halfedge: return "Halfedge";
      case Chunk::Entity_Face:     return "Face";
      default:
	std::clog << "as_string(Chunk::Entity): Invalid value!";
    }
    return NULL;
  }


//-----------------------------------------------------------------------------

  const char *as_string(Chunk::Type t)
  {
    switch(t)
    {    
      case Chunk::Type_Pos:      return "Pos";
      case Chunk::Type_Normal:   return "Normal";
      case Chunk::Type_Texcoord: return "Texcoord";
      case Chunk::Type_Status:   return "Status";
      case Chunk::Type_Color:    return "Color";
      case Chunk::Type_Custom:   return "Custom";
      case Chunk::Type_Topology: return "Topology";
    }
    return NULL;
  }


//-----------------------------------------------------------------------------

  const char *as_string(Chunk::Dim d)
  {
    switch(d)
    {    
      case Chunk::Dim_1D: return "1D";
      case Chunk::Dim_2D: return "2D";
      case Chunk::Dim_3D: return "3D";
      case Chunk::Dim_4D: return "4D";
      case Chunk::Dim_5D: return "5D";
      case Chunk::Dim_6D: return "6D";
      case Chunk::Dim_7D: return "7D";
      case Chunk::Dim_8D: return "8D";
    }
    return NULL;
  }


//-----------------------------------------------------------------------------

  const char *as_string(Chunk::Integer_Size d)
  {
    switch(d)
    {    
      case Chunk::Integer_8  : return "8";
      case Chunk::Integer_16 : return "16";
      case Chunk::Integer_32 : return "32";
      case Chunk::Integer_64 : return "64";
    }
    return NULL;
  }

  const char *as_string(Chunk::Float_Size d)
  {
    switch(d)
    {    
      case Chunk::Float_32 : return "32";
      case Chunk::Float_64 : return "64";
      case Chunk::Float_128: return "128";
    }
    return NULL;
  }


//-----------------------------------------------------------------------------

  std::ostream& operator << ( std::ostream& _os, const Chunk::Header& _c )
  {
    _os << "Chunk Header : 0x" << std::setw(4)
	<< std::hex << (*(uint16*)(&_c)) << std::dec << '\n';
    _os << "entity = " 
	<< as_string(Chunk::Entity(_c.entity_)) << '\n';
    _os << "type   = " 
	<< as_string(Chunk::Type(_c.type_));
    if ( Chunk::Type(_c.type_)!=Chunk::Type_Custom) 
    {
      _os << '\n'
	  << "signed = " 
	  << _c.signed_ << '\n';
      _os << "float  = " 
	  << _c.float_ << '\n';
      _os << "dim    = " 
	  << as_string(Chunk::Dim(_c.dim_)) << '\n';
      _os << "bits   = " 
	  << (_c.float_
	      ? as_string(Chunk::Float_Size(_c.bits_)) 
	      : as_string(Chunk::Integer_Size(_c.bits_)));
    }
    return _os;
  }


//-----------------------------------------------------------------------------

  std::ostream& operator << ( std::ostream& _os, const Header& _h )
  {
    _os << "magic   = '" << _h.magic_[0] << _h.magic_[1] << "'\n"
	<< "mesh    = '" << _h.mesh_ << "'\n"
	<< "version = 0x" << std::hex << (uint16)_h.version_ << std::dec
	<< " (" << major_version(_h.version_) 
	<< "."  << minor_version(_h.version_) << ")\n"
	<< "#V      = " << _h.n_vertices_ << '\n'
	<< "#F      = " << _h.n_faces_ << '\n'
	<< "#E      = " << _h.n_edges_;
    return _os;
  }


} // namespace OMFormat
  // --------------------------------------------------------------------------

//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
