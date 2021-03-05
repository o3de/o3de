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


#ifndef __OFFREADER_HH__
#define __OFFREADER_HH__


//=== INCLUDES ================================================================


#include <iosfwd>
#include <string>
#include <cstdio>

#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Utils/SingletonT.hh>
#include <OpenMesh/Core/IO/reader/BaseReader.hh>


//== NAMESPACES ===============================================================


namespace OpenMesh {
namespace IO {


//== FORWARDS =================================================================


class BaseImporter;


//== IMPLEMENTATION ===========================================================


/**
    Implementation of the OFF format reader. This class is singleton'ed by
    SingletonT to OFFReader.

    By passing Options to the read function you can manipulate the reading behavoir.
    The following options can be set:

    VertexNormal
    VertexColor
    VertexTexCoord
    FaceColor
    ColorAlpha [only when reading binary]

    These options define if the corresponding data should be read (if available)
    or if it should be omitted.

    After execution of the read function. The options object contains information about
    what was actually read.

    e.g. if VertexNormal was true when the read function was called, but the file
    did not contain vertex normals then it is false afterwards.

    When reading a binary off with Color Flag in the header it is assumed that all vertices
    and faces have colors in the format "int int int".
    If ColorAlpha is set the format "int int int int" is assumed.

*/

class OPENMESHDLLEXPORT _OFFReader_ : public BaseReader
{
public:

  _OFFReader_();

  /// Destructor
  virtual ~_OFFReader_() {};

  std::string get_description() const { return "Object File Format"; }
  std::string get_extensions()  const { return "off"; }
  std::string get_magic()       const { return "OFF"; }

  bool read(const std::string& _filename,
	    BaseImporter& _bi,
	    Options& _opt);

  bool can_u_read(const std::string& _filename) const;

  bool read(std::istream& _in, BaseImporter& _bi, Options& _opt );

private:

  bool can_u_read(std::istream& _is) const;

  bool read_ascii(std::istream& _in, BaseImporter& _bi, Options& _opt) const;
  bool read_binary(std::istream& _in, BaseImporter& _bi, Options& _opt, bool swap) const;

  void readValue(std::istream& _in, float& _value) const;
  void readValue(std::istream& _in, int& _value) const;
  void readValue(std::istream& _in, unsigned int& _value) const;

  int getColorType(std::string & _line, bool _texCoordsAvailable) const;

  //available options for reading
  mutable Options options_;
  //options that the user wants to read
  mutable Options userOptions_;
};


//== TYPE DEFINITION ==========================================================


/// Declare the single entity of the OFF reader
extern _OFFReader_  __OFFReaderInstance;
OPENMESHDLLEXPORT _OFFReader_& OFFReader();


//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
#endif
//=============================================================================
