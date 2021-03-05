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
//  Implements a reader module for OFF files
//
//=============================================================================


#ifndef __OMREADER_HH__
#define __OMREADER_HH__


//=== INCLUDES ================================================================

// OpenMesh
#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Utils/SingletonT.hh>
#include <OpenMesh/Core/IO/OMFormat.hh>
#include <OpenMesh/Core/IO/IOManager.hh>
#include <OpenMesh/Core/IO/importer/BaseImporter.hh>
#include <OpenMesh/Core/IO/reader/BaseReader.hh>

// STD C++
#include <iosfwd>
#include <string>


//== NAMESPACES ===============================================================


namespace OpenMesh {
namespace IO {


//== IMPLEMENTATION ===========================================================


/**
    Implementation of the OM format reader. This class is singleton'ed by
    SingletonT to OMReader.
*/
class OPENMESHDLLEXPORT _OMReader_ : public BaseReader
{
public:

  _OMReader_();
  virtual ~_OMReader_() { }

  std::string get_description() const { return "OpenMesh File Format"; }
  std::string get_extensions()  const { return "om"; }
  std::string get_magic()       const { return "OM"; }

  bool read(const std::string& _filename,
	    BaseImporter& _bi,
	    Options& _opt );

//!  Stream Reader for std::istream input in binary format
  bool read(std::istream& _is,
	    BaseImporter& _bi,
	    Options& _opt );

  virtual bool can_u_read(const std::string& _filename) const;
  virtual bool can_u_read(std::istream& _is) const;


private:

  bool supports( const OMFormat::uint8 version ) const;

  bool read_ascii(std::istream& _is, BaseImporter& _bi, Options& _opt) const;
  bool read_binary(std::istream& _is, BaseImporter& _bi, Options& _opt) const;

  typedef OMFormat::Header              Header;
  typedef OMFormat::Chunk::Header       ChunkHeader;
  typedef OMFormat::Chunk::PropertyName PropertyName;

  // initialized/updated by read_binary*/read_ascii*
  mutable size_t       bytes_;
  mutable Options      fileOptions_;
  mutable Header       header_;
  mutable ChunkHeader  chunk_header_;
  mutable PropertyName property_name_;

  bool read_binary_vertex_chunk(   std::istream      &_is,
				   BaseImporter      &_bi,
				   Options           &_opt,
				   bool              _swap) const;

  bool read_binary_face_chunk(     std::istream      &_is,
			           BaseImporter      &_bi,
			           Options           &_opt,
				   bool              _swap) const;

  bool read_binary_edge_chunk(     std::istream      &_is,
			           BaseImporter      &_bi,
			           Options           &_opt,
				   bool              _swap) const;

  bool read_binary_halfedge_chunk( std::istream      &_is,
				   BaseImporter      &_bi,
				   Options           &_opt,
				   bool              _swap) const;

  bool read_binary_mesh_chunk(     std::istream      &_is,
				   BaseImporter      &_bi,
				   Options           &_opt,
				   bool              _swap) const;

  size_t restore_binary_custom_data( std::istream& _is,
				     BaseProperty* _bp,
				     size_t _n_elem,
				     bool _swap) const;

};


//== TYPE DEFINITION ==========================================================


/// Declare the single entity of the OM reader.
extern _OMReader_  __OMReaderInstance;
OPENMESHDLLEXPORT _OMReader_&  OMReader();


//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
#endif
//=============================================================================
