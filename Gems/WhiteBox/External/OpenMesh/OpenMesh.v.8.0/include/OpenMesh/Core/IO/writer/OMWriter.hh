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
//  Implements a writer module for OM files
//
//=============================================================================


#ifndef __OMWRITER_HH__
#define __OMWRITER_HH__


//=== INCLUDES ================================================================


// STD C++
#include <iosfwd>
#include <string>

// OpenMesh
#include <OpenMesh/Core/IO/BinaryHelper.hh>
#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Utils/SingletonT.hh>
#include <OpenMesh/Core/IO/OMFormat.hh>
#include <OpenMesh/Core/IO/IOManager.hh>
#include <OpenMesh/Core/IO/writer/BaseWriter.hh>


//== NAMESPACES ===============================================================


namespace OpenMesh {
namespace IO {

//=== FORWARDS ================================================================


class BaseExporter;


//=== IMPLEMENTATION ==========================================================


/**
 *  Implementation of the OM format writer. This class is singleton'ed by
 *  SingletonT to OMWriter.
 */
class OPENMESHDLLEXPORT _OMWriter_ : public BaseWriter
{
public:

  /// Constructor
  _OMWriter_();

  /// Destructor
  virtual ~_OMWriter_() {};

  std::string get_description() const
  { return "OpenMesh Format"; }

  std::string get_extensions() const
  { return "om"; }

  bool write(std::ostream&, BaseExporter&, Options, std::streamsize _precision = 6) const;

  size_t binary_size(BaseExporter& _be, Options _opt) const;

  static OMFormat::uint8 get_version() { return version_; }


protected:

  static const OMFormat::uchar magic_[3];
  static const OMFormat::uint8 version_;

  bool write(const std::string&, BaseExporter&, Options, std::streamsize _precision = 6) const;

  bool write_binary(std::ostream&, BaseExporter&, Options) const;


  size_t store_binary_custom_chunk( std::ostream&, const BaseProperty&,
				    OMFormat::Chunk::Entity, bool) const;
};


//== TYPE DEFINITION ==========================================================


/// Declare the single entity of the OM writer.
extern _OMWriter_  __OMWriterInstance;
OPENMESHDLLEXPORT _OMWriter_& OMWriter();


//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
#endif
//=============================================================================
