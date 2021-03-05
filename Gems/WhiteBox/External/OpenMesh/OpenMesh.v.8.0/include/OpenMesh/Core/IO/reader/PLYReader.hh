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


#ifndef __PLYREADER_HH__
#define __PLYREADER_HH__


//=== INCLUDES ================================================================



#include <iosfwd>
#include <string>
#include <cstdio>
#include <vector>

#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Utils/SingletonT.hh>
#include <OpenMesh/Core/IO/reader/BaseReader.hh>
#include <OpenMesh/Core/Utils/GenProg.hh>


//== NAMESPACES ===============================================================


namespace OpenMesh {
namespace IO {


//== FORWARDS =================================================================


class BaseImporter;


//== IMPLEMENTATION ===========================================================


/**
    Implementation of the PLY format reader. This class is singleton'ed by
    SingletonT to OFFReader. It can read custom properties, accessible via the name
    of the custom properties. List properties has the type std::vector<Type>.

*/

class OPENMESHDLLEXPORT _PLYReader_ : public BaseReader
{
public:

  _PLYReader_();

  std::string get_description() const { return "PLY polygon file format"; }
  std::string get_extensions()  const { return "ply"; }
  std::string get_magic()       const { return "PLY"; }

  bool read(const std::string& _filename,
        BaseImporter& _bi,
        Options& _opt);

  bool read(std::istream& _is,
            BaseImporter& _bi,
            Options& _opt);

  bool can_u_read(const std::string& _filename) const;

  enum ValueType {
    Unsupported,
    ValueTypeINT8, ValueTypeCHAR,
    ValueTypeUINT8, ValueTypeUCHAR,
    ValueTypeINT16, ValueTypeSHORT,
    ValueTypeUINT16, ValueTypeUSHORT,
    ValueTypeINT32, ValueTypeINT,
    ValueTypeUINT32, ValueTypeUINT,
    ValueTypeFLOAT32, ValueTypeFLOAT,
    ValueTypeFLOAT64, ValueTypeDOUBLE
  };

private:

  bool can_u_read(std::istream& _is) const;

  bool read_ascii(std::istream& _in, BaseImporter& _bi, const Options& _opt) const;
  bool read_binary(std::istream& _in, BaseImporter& _bi, bool swap, const Options& _opt) const;

  float readToFloatValue(ValueType _type , std::fstream& _in) const;

  void readValue(ValueType _type , std::istream& _in, float& _value) const;
  void readValue(ValueType _type , std::istream& _in, double& _value) const;
  void readValue(ValueType _type , std::istream& _in, unsigned int& _value) const;
  void readValue(ValueType _type , std::istream& _in, unsigned short& _value) const;
  void readValue(ValueType _type , std::istream& _in, unsigned char& _value) const;
  void readValue(ValueType _type , std::istream& _in, int& _value) const;
  void readValue(ValueType _type , std::istream& _in, short& _value) const;
  void readValue(ValueType _type , std::istream& _in, signed char& _value) const;

  void readInteger(ValueType _type, std::istream& _in, int& _value) const;
  void readInteger(ValueType _type, std::istream& _in, unsigned int& _value) const;

  /// Read unsupported properties in PLY file
  void consume_input(std::istream& _in, int _count) const {
	  _in.read(reinterpret_cast<char*>(&buff[0]), _count);
  }

  mutable unsigned char buff[8];

  /// Available per file options for reading
  mutable Options options_;

  /// Options that the user wants to read
  mutable Options userOptions_;

  mutable unsigned int vertexCount_;
  mutable unsigned int faceCount_;

  mutable uint vertexDimension_;

  enum Property {
    XCOORD,YCOORD,ZCOORD,
    TEXX,TEXY,
    COLORRED,COLORGREEN,COLORBLUE,COLORALPHA,
    XNORM,YNORM,ZNORM, CUSTOM_PROP, VERTEX_INDICES,
    UNSUPPORTED
  };

  /// Stores sizes of property types
  mutable std::map<ValueType, int> scalar_size_;

  // Number of vertex properties
  struct PropertyInfo
  {
    Property       property;
    ValueType      value;
    std::string    name;//for custom properties
    ValueType      listIndexType;//if type is unsupported, the poerty is not a list. otherwise, it the index type
    PropertyInfo():property(UNSUPPORTED),value(Unsupported),name(""),listIndexType(Unsupported){}
    PropertyInfo(Property _p, ValueType _v):property(_p),value(_v),name(""),listIndexType(Unsupported){}
    PropertyInfo(Property _p, ValueType _v, const std::string& _n):property(_p),value(_v),name(_n),listIndexType(Unsupported){}
  };

  enum Element {
	  VERTEX,
	  FACE,
	  UNKNOWN
  };

  // Information on the elements
  struct ElementInfo
  {
    Element element_;
    std::string name_;
    unsigned int count_;
    std::vector< PropertyInfo > properties_;
  };

  mutable std::vector< ElementInfo > elements_;

  template<typename T>
  inline void read(_PLYReader_::ValueType _type, std::istream& _in, T& _value, OpenMesh::GenProg::TrueType /*_binary*/) const
  {
    readValue(_type, _in, _value);
  }

  template<typename T>
  inline void read(_PLYReader_::ValueType _type, std::istream& _in, T& _value, OpenMesh::GenProg::FalseType /*_binary*/) const
  {
    _in >> _value;
  }

  //read and assign custom properties with the given type. Also creates property, if not exist
  template<bool binary, typename T, typename Handle>
  void readCreateCustomProperty(std::istream& _in, BaseImporter& _bi, Handle _h, const std::string& _propName, const ValueType _valueType, const ValueType _listType) const;

  template<bool binary, typename Handle>
  void readCustomProperty(std::istream& _in, BaseImporter& _bi, Handle _h, const std::string& _propName, const _PLYReader_::ValueType _valueType, const _PLYReader_::ValueType _listIndexType) const;
};


//== TYPE DEFINITION ==========================================================


/// Declare the single entity of the PLY reader
extern _PLYReader_  __PLYReaderInstance;
OPENMESHDLLEXPORT _PLYReader_&  PLYReader();


//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
#endif
//=============================================================================
