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
//  Implements the baseclass for IOManager writer modules
//
//=============================================================================


#ifndef __BASEWRITER_HH__
#define __BASEWRITER_HH__


//=== INCLUDES ================================================================


// STD C++
#include <iosfwd>
#include <string>

// OpenMesh
#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/IO/Options.hh>
#include <OpenMesh/Core/IO/exporter/BaseExporter.hh>


//== NAMESPACES ===============================================================


namespace OpenMesh {
namespace IO {


//=== IMPLEMENTATION ==========================================================


/**
   Base class for all writer modules. The module should register itself at
   the IOManager by calling the register_module function.
*/
class OPENMESHDLLEXPORT BaseWriter
{
public:

  typedef unsigned int Option;

  /// Destructor
  virtual ~BaseWriter() {};

  /// Return short description of the supported file format.
  virtual std::string get_description() const = 0;

  /// Return file format's extension.
  virtual std::string get_extensions() const = 0;

  /** \brief Returns true if writer can write _filename (checks extension).
   * _filename can also provide an extension without a name for a file e.g. _filename == "om" checks, if the writer can write the "om" extension
   * @param _filename complete name of a file or just the extension
   * @result true, if writer can write data with the given extension
   */
  virtual bool can_u_write(const std::string& _filename) const;

  /** Write to a file
   * @param _filename write to file with the given filename
   * @param _be BaseExporter, which specifies the data source
   * @param _opt writing options
   * @param _precision can be used to specify the precision of the floating point notation.
   */
  virtual bool write(const std::string& _filename,
		     BaseExporter& _be,
                     Options _opt,
                     std::streamsize _precision = 6) const = 0;

  /** Write to a std::ostream
   * @param _os write to std::ostream
   * @param _be BaseExporter, which specifies the data source
   * @param _opt writing options
   * @param _precision can be used to specify the precision of the floating point notation.
   */
  virtual bool write(std::ostream& _os,
		     BaseExporter& _be,
                     Options _opt,
                     std::streamsize _precision = 6) const = 0;

  /// Returns expected size of file if binary format is supported else 0.
  virtual size_t binary_size(BaseExporter&, Options) const { return 0; }



protected:

  bool check(BaseExporter& _be, Options _opt) const
  {
    return (_opt.check(Options::VertexNormal ) <= _be.has_vertex_normals())
       &&  (_opt.check(Options::VertexTexCoord)<= _be.has_vertex_texcoords())
       &&  (_opt.check(Options::VertexColor)   <= _be.has_vertex_colors())
       &&  (_opt.check(Options::FaceNormal)    <= _be.has_face_normals())
       &&  (_opt.check(Options::FaceColor)     <= _be.has_face_colors());
  }
};


//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
#endif
//=============================================================================
