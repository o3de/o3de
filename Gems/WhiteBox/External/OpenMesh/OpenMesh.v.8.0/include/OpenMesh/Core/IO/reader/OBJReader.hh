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
//  Implements an reader module for OBJ files
//
//=============================================================================


#ifndef __OBJREADER_HH__
#define __OBJREADER_HH__


//=== INCLUDES ================================================================


#include <iosfwd>
#include <string>
#include <map>

#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Utils/SingletonT.hh>
#include <OpenMesh/Core/IO/importer/BaseImporter.hh>
#include <OpenMesh/Core/IO/reader/BaseReader.hh>


//== NAMESPACES ===============================================================


namespace OpenMesh {
namespace IO {


//== IMPLEMENTATION ===========================================================


/**
    Implementation of the OBJ format reader.
*/
class OPENMESHDLLEXPORT _OBJReader_ : public BaseReader
{
public:

  _OBJReader_();

  virtual ~_OBJReader_() { }

  std::string get_description() const { return "Alias/Wavefront"; }
  std::string get_extensions()  const { return "obj"; }

  bool read(const std::string& _filename,
	    BaseImporter& _bi,
	    Options& _opt);

  bool read(std::istream& _in,
          BaseImporter& _bi,
          Options& _opt);

private:

#ifndef DOXY_IGNORE_THIS
  class Material
  {
  public:

    Material() { cleanup(); }

    void cleanup()
    {
      Kd_is_set_     = false;
      Ka_is_set_     = false;
      Ks_is_set_     = false;
      Tr_is_set_     = false;
      map_Kd_is_set_ = false;
    }

    bool is_valid(void) const
    { return Kd_is_set_ || Ka_is_set_ || Ks_is_set_ || Tr_is_set_ || map_Kd_is_set_; }

    bool has_Kd(void)     { return Kd_is_set_;     }
    bool has_Ka(void)     { return Ka_is_set_;     }
    bool has_Ks(void)     { return Ks_is_set_;     }
    bool has_Tr(void)     { return Tr_is_set_;     }
    bool has_map_Kd(void) { return map_Kd_is_set_; }

    void set_Kd( float r, float g, float b )
    { Kd_=Vec3f(r,g,b); Kd_is_set_=true; }

    void set_Ka( float r, float g, float b )
    { Ka_=Vec3f(r,g,b); Ka_is_set_=true; }

    void set_Ks( float r, float g, float b )
    { Ks_=Vec3f(r,g,b); Ks_is_set_=true; }

    void set_Tr( float t )
    { Tr_=t;            Tr_is_set_=true; }

    void set_map_Kd( std::string _name, int _index_Kd )
    { map_Kd_ = _name, index_Kd_ = _index_Kd; map_Kd_is_set_ = true; };

    const Vec3f& Kd( void ) const { return Kd_; }
    const Vec3f& Ka( void ) const { return Ka_; }
    const Vec3f& Ks( void ) const { return Ks_; }
    float  Tr( void ) const { return Tr_; }
    const std::string& map_Kd( void ) { return map_Kd_ ; }
    const int& map_Kd_index( void ) { return index_Kd_ ; }

  private:

    Vec3f Kd_;                          bool Kd_is_set_; // diffuse
    Vec3f Ka_;                          bool Ka_is_set_; // ambient
    Vec3f Ks_;                          bool Ks_is_set_; // specular
    float Tr_;                          bool Tr_is_set_; // transperency

    std::string map_Kd_; int index_Kd_; bool map_Kd_is_set_; // Texture

  };
#endif

  typedef std::map<std::string, Material> MaterialList;

  MaterialList materials_;

  bool read_material( std::fstream& _in );


private:

  bool read_vertices(std::istream& _in, BaseImporter& _bi, Options& _opt,
                     std::vector<Vec3f> & normals,
                     std::vector<Vec3f> & colors,
                     std::vector<Vec3f> & texcoords3d,
                     std::vector<Vec2f> & texcoords,
                     std::vector<VertexHandle> & vertexHandles,
                     Options & fileOptions);

  std::string path_;

};


//== TYPE DEFINITION ==========================================================


extern _OBJReader_  __OBJReaderInstance;
OPENMESHDLLEXPORT _OBJReader_& OBJReader();


//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
#endif
//=============================================================================
