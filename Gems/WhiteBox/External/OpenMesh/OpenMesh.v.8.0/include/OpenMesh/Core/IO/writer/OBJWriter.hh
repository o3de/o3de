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
//  Implements an IOManager writer module for OBJ files
//
//=============================================================================


#ifndef __OBJWRITER_HH__
#define __OBJWRITER_HH__


//=== INCLUDES ================================================================


#include <string>
#include <fstream>

#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Utils/SingletonT.hh>
#include <OpenMesh/Core/IO/exporter/BaseExporter.hh>
#include <OpenMesh/Core/IO/writer/BaseWriter.hh>


//== NAMESPACES ===============================================================


namespace OpenMesh {
namespace IO {


//=== IMPLEMENTATION ==========================================================


/**
    This class defines the OBJ writer. This class is further singleton'ed
    by SingletonT to OBJWriter.
*/
class OPENMESHDLLEXPORT _OBJWriter_ : public BaseWriter
{
public:

  _OBJWriter_();

  /// Destructor
  virtual ~_OBJWriter_() {};

  std::string get_description() const  { return "Alias/Wavefront"; }
  std::string get_extensions()  const  { return "obj"; }

  bool write(const std::string&, BaseExporter&, Options, std::streamsize _precision = 6) const;

  bool write(std::ostream&, BaseExporter&, Options, std::streamsize _precision = 6) const;

  size_t binary_size(BaseExporter&, Options) const { return 0; }

private:

  mutable std::string path_;
  mutable std::string objName_;

  mutable std::vector< OpenMesh::Vec3f > material_;
  mutable std::vector< OpenMesh::Vec4f > materialA_;

  size_t getMaterial(OpenMesh::Vec3f _color) const;

  size_t getMaterial(OpenMesh::Vec4f _color) const;

  bool writeMaterial(std::ostream& _out, BaseExporter&, Options) const;


};


//== TYPE DEFINITION ==========================================================


/// Declare the single entity of the OBJ writer
extern _OBJWriter_  __OBJWriterinstance;
OPENMESHDLLEXPORT _OBJWriter_& OBJWriter();


//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
#endif
//=============================================================================
