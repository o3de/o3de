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
//  Implements a writer module for PLY files
//
//=============================================================================


#ifndef __PLYWRITER_HH__
#define __PLYWRITER_HH__


//=== INCLUDES ================================================================

#include <string>
#include <ostream>
#include <vector>

#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Utils/SingletonT.hh>
#include <OpenMesh/Core/IO/exporter/BaseExporter.hh>
#include <OpenMesh/Core/IO/writer/BaseWriter.hh>
#include <OpenMesh/Core/Utils/GenProg.hh>


//== NAMESPACES ===============================================================


namespace OpenMesh {
namespace IO {


//=== IMPLEMENTATION ==========================================================


/**
    Implementation of the PLY format writer. This class is singleton'ed by
    SingletonT to PLYWriter.

    currently supported options:
    - VertexColors
    - FaceColors
    - Binary
    - Binary -> MSB
*/
class OPENMESHDLLEXPORT _PLYWriter_ : public BaseWriter
{
public:

  _PLYWriter_();

  /// Destructor
  virtual ~_PLYWriter_() {};

  std::string get_description() const { return "PLY polygon file format"; }
  std::string get_extensions() const  { return "ply"; }

  bool write(const std::string&, BaseExporter&, Options, std::streamsize _precision = 6) const;

  bool write(std::ostream&, BaseExporter&, Options, std::streamsize _precision = 6) const;

  size_t binary_size(BaseExporter& _be, Options _opt) const;

  enum ValueType {
    Unsupported = 0,
    ValueTypeFLOAT32, ValueTypeFLOAT,
    ValueTypeINT32, ValueTypeINT , ValueTypeUINT,
    ValueTypeUCHAR, ValueTypeCHAR, ValueTypeUINT8,
    ValueTypeUSHORT, ValueTypeSHORT,
    ValueTypeDOUBLE
  };

private:
  mutable Options options_;

  struct CustomProperty
  {
    ValueType type;
    const BaseProperty*  property;
    CustomProperty(const BaseProperty* const _p):type(Unsupported),property(_p){}
  };

  const char* nameOfType_[12];

  /// write custom persistant properties into the header for the current element, returns all properties, which were written sorted
  std::vector<CustomProperty> writeCustomTypeHeader(std::ostream& _out, BaseKernel::const_prop_iterator _begin, BaseKernel::const_prop_iterator _end) const;
  template<bool binary>
  void write_customProp(std::ostream& _our, const CustomProperty& _prop, size_t _index) const;
  template<typename T>
  void writeProxy(ValueType _type, std::ostream& _out, T _value, OpenMesh::GenProg::TrueType /*_binary*/) const
  {
    writeValue(_type, _out, _value);
  }
  template<typename T>
  void writeProxy(ValueType _type, std::ostream& _out, T _value, OpenMesh::GenProg::FalseType /*_binary*/) const
  {
    _out << " " << _value;
  }

protected:
  void writeValue(ValueType _type, std::ostream& _out, signed char value) const;
  void writeValue(ValueType _type, std::ostream& _out, unsigned char value) const;
  void writeValue(ValueType _type, std::ostream& _out, short value) const;
  void writeValue(ValueType _type, std::ostream& _out, unsigned short value) const;
  void writeValue(ValueType _type, std::ostream& _out, int value) const;
  void writeValue(ValueType _type, std::ostream& _out, unsigned int value) const;
  void writeValue(ValueType _type, std::ostream& _out, float value) const;
  void writeValue(ValueType _type, std::ostream& _out, double value) const;

  bool write_ascii(std::ostream& _out, BaseExporter&, Options) const;
  bool write_binary(std::ostream& _out, BaseExporter&, Options) const;
  /// write header into the stream _out. Returns custom properties (vertex and face) which are written into the header
  void write_header(std::ostream& _out, BaseExporter& _be, Options& _opt, std::vector<CustomProperty>& _ovProps, std::vector<CustomProperty>& _ofProps) const;
};


//== TYPE DEFINITION ==========================================================


/// Declare the single entity of the PLY writer.
extern _PLYWriter_  __PLYWriterInstance;
OPENMESHDLLEXPORT _PLYWriter_& PLYWriter();


//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
#endif
//=============================================================================
