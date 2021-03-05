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




//== INCLUDES =================================================================


// STL
#include <map>

#include <float.h>
#include <fstream>
#include <cstring>

// OpenMesh
#include <OpenMesh/Core/IO/BinaryHelper.hh>
#include <OpenMesh/Core/IO/reader/STLReader.hh>
#include <OpenMesh/Core/IO/IOManager.hh>

//comppare strings crossplatform ignorign case
#ifdef _WIN32
  #define strnicmp _strnicmp
#else
  #define strnicmp strncasecmp
#endif


//=== NAMESPACES ==============================================================


namespace OpenMesh {
namespace IO {


//=== INSTANCIATE =============================================================


// register the STLReader singleton with MeshReader
_STLReader_  __STLReaderInstance;
_STLReader_&  STLReader() { return __STLReaderInstance; }


//=== IMPLEMENTATION ==========================================================


_STLReader_::
_STLReader_()
  : eps_(FLT_MIN)
{
  IOManager().register_module(this);
}


//-----------------------------------------------------------------------------


bool
_STLReader_::
read(const std::string& _filename, BaseImporter& _bi, Options& _opt)
{
  bool result = false;

  STL_Type file_type = NONE;

  if ( check_extension( _filename, "stla" ) )
  {
    file_type = STLA;
  }

  else if ( check_extension( _filename, "stlb" ) )
  {
    file_type = STLB;
  }

  else if ( check_extension( _filename, "stl" ) )
  {
    file_type = check_stl_type(_filename);
  }


  switch (file_type)
  {
    case STLA:
    {
      result = read_stla(_filename, _bi, _opt);
      _opt -= Options::Binary;
      break;
    }

    case STLB:
    {
      result = read_stlb(_filename, _bi, _opt);
      _opt += Options::Binary;
      break;
    }

    default:
    {
      result = false;
      break;
    }
  }


  return result;
}

bool
_STLReader_::read(std::istream& _is,
		 BaseImporter& _bi,
		 Options& _opt)
{

  bool result = false;

  if (_opt & Options::Binary)
    result = read_stlb(_is, _bi, _opt);
  else
    result = read_stla(_is, _bi, _opt);

  return result;
}


//-----------------------------------------------------------------------------


#ifndef DOXY_IGNORE_THIS

class CmpVec
{
public:

  explicit CmpVec(float _eps=FLT_MIN) : eps_(_eps) {}

  bool operator()( const Vec3f& _v0, const Vec3f& _v1 ) const
  {
    if (fabs(_v0[0] - _v1[0]) <= eps_)
    {
      if (fabs(_v0[1] - _v1[1]) <= eps_)
      {
	return (_v0[2] < _v1[2] - eps_);
      }
      else return (_v0[1] < _v1[1] - eps_);
    }
    else return (_v0[0] < _v1[0] - eps_);
  }

private:
  float eps_;
};

#endif


//-----------------------------------------------------------------------------

void trimStdString( std::string& _string) {
  // Trim Both leading and trailing spaces

  size_t start = _string.find_first_not_of(" \t\r\n");
  size_t end   = _string.find_last_not_of(" \t\r\n");

  if(( std::string::npos == start ) || ( std::string::npos == end))
    _string = "";
  else
    _string = _string.substr( start, end-start+1 );
}

//-----------------------------------------------------------------------------

bool
_STLReader_::
read_stla(const std::string& _filename, BaseImporter& _bi, Options& _opt) const
{
  std::fstream in( _filename.c_str(), std::ios_base::in );

  if (!in)
  {
    omerr() << "[STLReader] : cannot not open file "
	  << _filename
	  << std::endl;
    return false;
  }

  bool res = read_stla(in, _bi, _opt);

  if (in)
    in.close();

  return res;
}

//-----------------------------------------------------------------------------

bool
_STLReader_::
read_stla(std::istream& _in, BaseImporter& _bi, Options& _opt) const
{

  unsigned int               i;
  OpenMesh::Vec3f            v;
  OpenMesh::Vec3f            n;
  BaseImporter::VHandles     vhandles;

  CmpVec comp(eps_);
  std::map<Vec3f, VertexHandle, CmpVec>            vMap(comp);
  std::map<Vec3f, VertexHandle, CmpVec>::iterator  vMapIt;

  std::string line;

  std::string        garbage;
  std::stringstream  strstream;

  bool facet_normal(false);

  while( _in && !_in.eof() ) {

    // Get one line
    std::getline(_in, line);
    if ( _in.bad() ){
      omerr() << "  Warning! Could not read stream properly!\n";
      return false;
    }

    // Trim Both leading and trailing spaces
    trimStdString(line);

    // Normal found?
    if (line.find("facet normal") != std::string::npos) {
      strstream.str(line);
      strstream.clear();

      // facet
      strstream >> garbage;

      // normal
      strstream >> garbage;

      strstream >> n[0];
      strstream >> n[1];
      strstream >> n[2];

      facet_normal = true;
    }

    // Detected a triangle
    if ( (line.find("outer") != std::string::npos) ||  (line.find("OUTER") != std::string::npos ) ) {

      vhandles.clear();

      for (i=0; i<3; ++i) {
        // Get one vertex
        std::getline(_in, line);
        trimStdString(line);

        strstream.str(line);
        strstream.clear();

        strstream >> garbage;

        strstream >> v[0];
        strstream >> v[1];
        strstream >> v[2];

        // has vector been referenced before?
        if ((vMapIt=vMap.find(v)) == vMap.end())
        {
          // No : add vertex and remember idx/vector mapping
          VertexHandle handle = _bi.add_vertex(v);
          vhandles.push_back(handle);
          vMap[v] = handle;
        }
        else
          // Yes : get index from map
          vhandles.push_back(vMapIt->second);

      }

      // Add face only if it is not degenerated
      if ((vhandles[0] != vhandles[1]) &&
          (vhandles[0] != vhandles[2]) &&
          (vhandles[1] != vhandles[2])) {


        FaceHandle fh = _bi.add_face(vhandles);

        // set the normal if requested
        // if a normal was requested but could not be found we unset the option
        if (facet_normal) {
          if (fh.is_valid() && _opt.face_has_normal())
            _bi.set_normal(fh, n);
        } else
          _opt -= Options::FaceNormal;
      }

      facet_normal = false;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------

bool
_STLReader_::
read_stlb(const std::string& _filename, BaseImporter& _bi, Options& _opt) const
{
  std::fstream in( _filename.c_str(), std::ios_base::in | std::ios_base::binary);

  if (!in)
  {
    omerr() << "[STLReader] : cannot not open file "
	  << _filename
	  << std::endl;
    return false;
  }

  bool res = read_stlb(in, _bi, _opt);

  if (in)
    in.close();

  return res;
}

//-----------------------------------------------------------------------------

bool
_STLReader_::
read_stlb(std::istream& _in, BaseImporter& _bi, Options& _opt) const
{
  char                       dummy[100];
  bool                       swapFlag;
  unsigned int               i, nT;
  OpenMesh::Vec3f            v, n;
  BaseImporter::VHandles     vhandles;

  std::map<Vec3f, VertexHandle, CmpVec>  vMap;
  std::map<Vec3f, VertexHandle, CmpVec>::iterator vMapIt;


  // check size of types
  if ((sizeof(float) != 4) || (sizeof(int) != 4)) {
    omerr() << "[STLReader] : wrong type size\n";
    return false;
  }

  // determine endian mode
  union { unsigned int i; unsigned char c[4]; } endian_test;
  endian_test.i = 1;
  swapFlag = (endian_test.c[3] == 1);

  // read number of triangles
  _in.read(dummy, 80);
  nT = read_int(_in, swapFlag);

  // read triangles
  while (nT)
  {
    vhandles.clear();

    // read triangle normal
    n[0] = read_float(_in, swapFlag);
    n[1] = read_float(_in, swapFlag);
    n[2] = read_float(_in, swapFlag);

    // triangle's vertices
    for (i=0; i<3; ++i)
    {
      v[0] = read_float(_in, swapFlag);
      v[1] = read_float(_in, swapFlag);
      v[2] = read_float(_in, swapFlag);

      // has vector been referenced before?
      if ((vMapIt=vMap.find(v)) == vMap.end())
      {
        // No : add vertex and remember idx/vector mapping
        VertexHandle handle = _bi.add_vertex(v);
        vhandles.push_back(handle);
        vMap[v] = handle;
      }
      else
        // Yes : get index from map
        vhandles.push_back(vMapIt->second);
    }


    // Add face only if it is not degenerated
    if ((vhandles[0] != vhandles[1]) &&
	(vhandles[0] != vhandles[2]) &&
	(vhandles[1] != vhandles[2])) {
      FaceHandle fh = _bi.add_face(vhandles);

      if (fh.is_valid() && _opt.face_has_normal())
        _bi.set_normal(fh, n);
    }

    _in.read(dummy, 2);
    --nT;
  }

  return true;
}

//-----------------------------------------------------------------------------

_STLReader_::STL_Type
_STLReader_::
check_stl_type(const std::string& _filename) const
{

   // open file
   std::ifstream ifs (_filename.c_str(), std::ifstream::binary);
   if(!ifs.good())
   {
     omerr() << "could not open file" << _filename << std::endl;
     return NONE;
   }

   //find first non whitespace character
   std::string line = "";
   std::size_t firstChar;
   while(line.empty() && ifs.good())
   {
     std::getline(ifs,line);
     firstChar = line.find_first_not_of("\t ");
   }

   //check for ascii keyword solid
   if(strnicmp("solid",&line[firstChar],5) == 0)
   {
     return STLA;
   }
   ifs.close();

   //if the file does not start with solid it is probably STLB
   //check the file size to verify it.

   //open the file
   FILE* in = fopen(_filename.c_str(), "rb");
   if (!in) return NONE;

   // determine endian mode
   union { unsigned int i; unsigned char c[4]; } endian_test;
   endian_test.i = 1;
   bool swapFlag = (endian_test.c[3] == 1);


   // read number of triangles
   char dummy[100];
   fread(dummy, 1, 80, in);
   size_t nT = read_int(in, swapFlag);


   // compute file size from nT
   size_t binary_size = 84 + nT*50;

   // get actual file size
   size_t file_size(0);
   rewind(in);
   while (!feof(in))
     file_size += fread(dummy, 1, 100, in);
   fclose(in);

   // if sizes match -> it's STLB
      return (binary_size == file_size ? STLB : NONE);
}


//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
