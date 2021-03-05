// Modifications copyright Amazon.com, Inc. or its affiliates.
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


//STL
#include <vector>
#include <istream>
#include <fstream>

// OpenMesh
#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/System/omstream.hh>
#include <OpenMesh/Core/Utils/Endian.hh>
#include <OpenMesh/Core/IO/OMFormat.hh>
#include <OpenMesh/Core/IO/reader/OMReader.hh>
#include <OpenMesh/Core/IO/writer/OMWriter.hh>


//=== NAMESPACES ==============================================================


namespace OpenMesh {
namespace IO {


//=== INSTANCIATE =============================================================


// register the OMReader singleton with MeshReader
_OMReader_  __OMReaderInstance;
_OMReader_&  OMReader() { return __OMReaderInstance; }



//=== IMPLEMENTATION ==========================================================


_OMReader_::_OMReader_()
{
  IOManager().register_module(this);
}


//-----------------------------------------------------------------------------


bool _OMReader_::read(const std::string& _filename, BaseImporter& _bi, Options& _opt)
{
  // check whether importer can give us an OpenMesh BaseKernel
  if (!_bi.kernel())
    return false;

  _opt += Options::Binary; // only binary format supported!
  fileOptions_ = Options::Binary;

  // Open file
  std::ifstream ifs(_filename.c_str(), std::ios::binary);

  /* Clear formatting flag skipws (Skip whitespaces). If set, operator>> will
   * skip bytes set to whitespace chars (e.g. 0x20 bytes) in
   * Property<bool>::restore.
   */
  ifs.unsetf(std::ios::skipws);

  if (!ifs.is_open() || !ifs.good()) {
    omerr() << "[OMReader] : cannot not open file " << _filename << std::endl;
    return false;
  }

  // Pass stream to read method, remember result
  bool result = read(ifs, _bi, _opt);

  // close input stream
  ifs.close();

  _opt = _opt & fileOptions_;

  return result;
}

//-----------------------------------------------------------------------------


bool _OMReader_::read(std::istream& _is, BaseImporter& _bi, Options& _opt)
{
  // check whether importer can give us an OpenMesh BaseKernel
  if (!_bi.kernel())
    return false;

  _opt += Options::Binary; // only binary format supported!
  fileOptions_ = Options::Binary;

  if (!_is.good()) {
    omerr() << "[OMReader] : cannot read from stream " << std::endl;
    return false;
  }

  // Pass stream to read method, remember result
  bool result = read_binary(_is, _bi, _opt);

  if (result)
    _opt += Options::Binary;

  _opt = _opt & fileOptions_;

  return result;
}



//-----------------------------------------------------------------------------

bool _OMReader_::read_ascii(std::istream& /* _is */, BaseImporter& /* _bi */, Options& /* _opt */) const
{
  // not supported yet!
  return false;
}


//-----------------------------------------------------------------------------

bool _OMReader_::read_binary(std::istream& _is, BaseImporter& _bi, Options& _opt) const
{
  bool swap = _opt.check(Options::Swap) || (Endian::local() == Endian::MSB);

  // Initialize byte counter
  bytes_ = 0;

  bytes_ += restore(_is, header_, swap);


  if (header_.version_ > _OMWriter_::get_version())
  {
    omerr() << "File uses .om version " << OMFormat::as_string(header_.version_) << " but reader only "
            << "supports up to version " << OMFormat::as_string(_OMWriter_::get_version()) << ".\n"
            << "Please update your OpenMesh." << std::endl;
    return false;
  }


  while (!_is.eof()) {
    bytes_ += restore(_is, chunk_header_, swap);

    if (_is.eof())
      break;

    // Is this a named property restore the name
    if (chunk_header_.name_) {
      OMFormat::Chunk::PropertyName pn;
      bytes_ += restore(_is, property_name_, swap);
    }

    // Read in the property data. If it is an anonymous or unknown named
    // property, then skip data.
    switch (chunk_header_.entity_) {
      case OMFormat::Chunk::Entity_Vertex:
        if (!read_binary_vertex_chunk(_is, _bi, _opt, swap))
          return false;
        break;
      case OMFormat::Chunk::Entity_Face:
        if (!read_binary_face_chunk(_is, _bi, _opt, swap))
          return false;
        break;
      case OMFormat::Chunk::Entity_Edge:
        if (!read_binary_edge_chunk(_is, _bi, _opt, swap))
          return false;
        break;
      case OMFormat::Chunk::Entity_Halfedge:
        if (!read_binary_halfedge_chunk(_is, _bi, _opt, swap))
          return false;
        break;
      case OMFormat::Chunk::Entity_Mesh:
        if (!read_binary_mesh_chunk(_is, _bi, _opt, swap))
          return false;
        break;
      case OMFormat::Chunk::Entity_Sentinel:
        return true;
      default:
        return false;
    }

  }

  // File was successfully parsed.
  return true;
}


//-----------------------------------------------------------------------------

bool _OMReader_::can_u_read(const std::string& _filename) const
{
  // !!! Assuming BaseReader::can_u_parse( std::string& )
  // does not call BaseReader::read_magic()!!!
  if (this->BaseReader::can_u_read(_filename)) {
    std::ifstream ifile(_filename.c_str());
    if (ifile && can_u_read(ifile))
      return true;
  }
  return false;
}

//-----------------------------------------------------------------------------

bool _OMReader_::can_u_read(std::istream& _is) const
{
  std::vector<char> evt;
  evt.reserve(20);

  // read first 4 characters into a buffer
  while (evt.size() < 4)
    evt.push_back(static_cast<char>(_is.get()));

  // put back all read characters
  std::vector<char>::reverse_iterator it = evt.rbegin();
  while (it != evt.rend())
    _is.putback(*it++);

  // evaluate header information
  OMFormat::Header *hdr = (OMFormat::Header*) &evt[0];

  // first two characters must be 'OM'
  if (hdr->magic_[0] != 'O' || hdr->magic_[1] != 'M')
    return false;

  // 3rd characters defines the mesh type:
  switch (hdr->mesh_) {
    case 'T': // Triangle Mesh
    case 'Q': // Quad Mesh
    case 'P': // Polygonal Mesh
      break;
    default:  // ?
      return false;
  }

  // 4th characters encodes the version
  return supports(hdr->version_);
}

//-----------------------------------------------------------------------------

bool _OMReader_::supports(const OMFormat::uint8 /* version */) const
{
  return true;
}


//-----------------------------------------------------------------------------

bool _OMReader_::read_binary_vertex_chunk(std::istream &_is, BaseImporter &_bi, Options &_opt, bool _swap) const
{
  using OMFormat::Chunk;

  assert( chunk_header_.entity_ == Chunk::Entity_Vertex);

  OpenMesh::Vec3f v3f;
  OpenMesh::Vec2f v2f;
  OpenMesh::Vec3uc v3uc; // rgb
  OpenMesh::Attributes::StatusInfo status;

  OMFormat::Chunk::PropertyName custom_prop;

  size_t vidx = 0;
  switch (chunk_header_.type_) {
    case Chunk::Type_Pos:
      assert( OMFormat::dimensions(chunk_header_) == size_t(OpenMesh::Vec3f::dim()));

      for (; vidx < header_.n_vertices_ && !_is.eof(); ++vidx) {
        bytes_ += vector_restore(_is, v3f, _swap);
        _bi.add_vertex(v3f);
      }
      break;

    case Chunk::Type_Normal:
      assert( OMFormat::dimensions(chunk_header_) == size_t(OpenMesh::Vec3f::dim()));

      fileOptions_ += Options::VertexNormal;
      for (; vidx < header_.n_vertices_ && !_is.eof(); ++vidx) {
        bytes_ += vector_restore(_is, v3f, _swap);
        if (fileOptions_.vertex_has_normal() && _opt.vertex_has_normal())
          _bi.set_normal(VertexHandle(int(vidx)), v3f);
      }
      break;

    case Chunk::Type_Texcoord:
      assert( OMFormat::dimensions(chunk_header_) == size_t(OpenMesh::Vec2f::dim()));

      fileOptions_ += Options::VertexTexCoord;
      for (; vidx < header_.n_vertices_ && !_is.eof(); ++vidx) {
        bytes_ += vector_restore(_is, v2f, _swap);
        if (fileOptions_.vertex_has_texcoord() && _opt.vertex_has_texcoord())
          _bi.set_texcoord(VertexHandle(int(vidx)), v2f);
      }
      break;

    case Chunk::Type_Color:

      assert( OMFormat::dimensions(chunk_header_) == 3);

      fileOptions_ += Options::VertexColor;

      for (; vidx < header_.n_vertices_ && !_is.eof(); ++vidx) {
        bytes_ += vector_restore(_is, v3uc, _swap);
        if (fileOptions_.vertex_has_color() && _opt.vertex_has_color())
          _bi.set_color(VertexHandle(int(vidx)), v3uc);
      }
      break;

    case Chunk::Type_Status:
    {
      assert( OMFormat::dimensions(chunk_header_) == 1);

      fileOptions_ += Options::Status;

      for (; vidx < header_.n_vertices_ && !_is.eof(); ++vidx) {
        bytes_ += restore(_is, status, _swap);
        if (fileOptions_.vertex_has_status() && _opt.vertex_has_status())
          _bi.set_status(VertexHandle(int(vidx)), status);
      }
      break;
    }

    case Chunk::Type_Custom:

      bytes_ += restore_binary_custom_data(_is, _bi.kernel()->_get_vprop(property_name_), header_.n_vertices_, _swap);

      vidx = header_.n_vertices_;

      break;

    case Chunk::Type_Topology:
    {
      for (; vidx < header_.n_vertices_; ++vidx)
      {
        int halfedge_id = 0;
        bytes_ += restore( _is, halfedge_id, OMFormat::Chunk::Integer_Size(chunk_header_.bits_), _swap );

        _bi.set_halfedge(VertexHandle(static_cast<int>(vidx)), HalfedgeHandle(halfedge_id));
      }
    }

      break;

    default: // skip unknown chunks
    {
      omerr() << "Unknown chunk type ignored!\n";
      size_t size_of = header_.n_vertices_ * OMFormat::vector_size(chunk_header_);
      _is.ignore(size_of);
      bytes_ += size_of;
    }
  }

  // all chunk data has been read..?!
  return vidx == header_.n_vertices_;
}


//-----------------------------------------------------------------------------

bool _OMReader_::read_binary_face_chunk(std::istream &_is, BaseImporter &_bi, Options &_opt, bool _swap) const
{
  using OMFormat::Chunk;

  assert( chunk_header_.entity_ == Chunk::Entity_Face);

  size_t fidx = 0;
  OpenMesh::Vec3f v3f;  // normal
  OpenMesh::Vec3uc v3uc; // rgb
  OpenMesh::Attributes::StatusInfo status;

  switch (chunk_header_.type_) {
    case Chunk::Type_Topology:
    {
      if (header_.version_ < OMFormat::mk_version(2,0))
      {
        // add faces based on vertex indices
        BaseImporter::VHandles vhandles;
        size_t nV = 0;
        size_t vidx = 0;

        switch (header_.mesh_) {
          case 'T':
            nV = 3;
            break;
          case 'Q':
            nV = 4;
            break;
        }

        for (; fidx < header_.n_faces_; ++fidx) {
          if (header_.mesh_ == 'P')
            bytes_ += restore(_is, nV, Chunk::Integer_16, _swap);

          vhandles.clear();
          for (size_t j = 0; j < nV; ++j) {
            bytes_ += restore(_is, vidx, Chunk::Integer_Size(chunk_header_.bits_), _swap);

            vhandles.push_back(VertexHandle(int(vidx)));
          }

          _bi.add_face(vhandles);
        }
      }
      else
      {
        // add faces by simply setting an incident halfedge
        for (; fidx < header_.n_faces_; ++fidx)
        {
          int halfedge_id = 0;
          bytes_ += restore( _is, halfedge_id, OMFormat::Chunk::Integer_Size(chunk_header_.bits_), _swap );

          _bi.add_face(HalfedgeHandle(halfedge_id));
        }
      }
    }
      break;

    case Chunk::Type_Normal:
      assert( OMFormat::dimensions(chunk_header_) == size_t(OpenMesh::Vec3f::dim()));

      fileOptions_ += Options::FaceNormal;
      for (; fidx < header_.n_faces_ && !_is.eof(); ++fidx) {
        bytes_ += vector_restore(_is, v3f, _swap);
        if( fileOptions_.face_has_normal() && _opt.face_has_normal())
          _bi.set_normal(FaceHandle(int(fidx)), v3f);
      }
      break;

    case Chunk::Type_Color:

      assert( OMFormat::dimensions(chunk_header_) == 3);

      fileOptions_ += Options::FaceColor;
      for (; fidx < header_.n_faces_ && !_is.eof(); ++fidx) {
        bytes_ += vector_restore(_is, v3uc, _swap);
        if( fileOptions_.face_has_color() && _opt.face_has_color())
          _bi.set_color(FaceHandle(int(fidx)), v3uc);
      }
      break;
    case Chunk::Type_Status:
    {
      assert( OMFormat::dimensions(chunk_header_) == 1);

      fileOptions_ += Options::Status;

      for (; fidx < header_.n_faces_ && !_is.eof(); ++fidx) {
        bytes_ += restore(_is, status, _swap);
        if (fileOptions_.face_has_status() && _opt.face_has_status())
          _bi.set_status(FaceHandle(int(fidx)), status);
      }
      break;
    }

    case Chunk::Type_Custom:

      bytes_ += restore_binary_custom_data(_is, _bi.kernel()->_get_fprop(property_name_), header_.n_faces_, _swap);

      fidx = header_.n_faces_;

      break;

    default: // skip unknown chunks
    {
      omerr() << "Unknown chunk type ignore!\n";
      size_t size_of = OMFormat::chunk_data_size(header_, chunk_header_);
      _is.ignore(size_of);
      bytes_ += size_of;
    }
  }
  return fidx == header_.n_faces_;
}


//-----------------------------------------------------------------------------

bool _OMReader_::read_binary_edge_chunk(std::istream &_is, BaseImporter &_bi, Options &_opt, bool _swap) const
{
  using OMFormat::Chunk;

  assert( chunk_header_.entity_ == Chunk::Entity_Edge);

  size_t b = bytes_;

  OpenMesh::Attributes::StatusInfo status;

  switch (chunk_header_.type_) {
    case Chunk::Type_Custom:

      bytes_ += restore_binary_custom_data(_is, _bi.kernel()->_get_eprop(property_name_), header_.n_edges_, _swap);

      break;

    case Chunk::Type_Status:
    {
      assert( OMFormat::dimensions(chunk_header_) == 1);

      fileOptions_ += Options::Status;

      for (size_t eidx = 0; eidx < header_.n_edges_ && !_is.eof(); ++eidx) {
        bytes_ += restore(_is, status, _swap);
        if (fileOptions_.edge_has_status() && _opt.edge_has_status())
          _bi.set_status(EdgeHandle(int(eidx)), status);
      }
      break;
    }

    default:
      // skip unknown type
      size_t size_of = OMFormat::chunk_data_size(header_, chunk_header_);
      _is.ignore(size_of);
      bytes_ += size_of;
  }

  return b < bytes_;
}


//-----------------------------------------------------------------------------

bool _OMReader_::read_binary_halfedge_chunk(std::istream &_is, BaseImporter &_bi, Options & _opt, bool _swap) const
{
  using OMFormat::Chunk;

  assert( chunk_header_.entity_ == Chunk::Entity_Halfedge);

  size_t b = bytes_;
  OpenMesh::Attributes::StatusInfo status;

  switch (chunk_header_.type_) {
    case Chunk::Type_Custom:

      bytes_ += restore_binary_custom_data(_is, _bi.kernel()->_get_hprop(property_name_), 2 * header_.n_edges_, _swap);
      break;

    case Chunk::Type_Topology:
    {
      std::vector<HalfedgeHandle> next_halfedges;
      for (size_t e_idx = 0; e_idx < header_.n_edges_; ++e_idx)
      {
        int next_id_0      = -1;
        int to_vertex_id_0 = -1;
        int face_id_0      = -1;
        bytes_ += restore( _is, next_id_0,      OMFormat::Chunk::Integer_Size(chunk_header_.bits_), _swap );
        bytes_ += restore( _is, to_vertex_id_0, OMFormat::Chunk::Integer_Size(chunk_header_.bits_), _swap );
        bytes_ += restore( _is, face_id_0,      OMFormat::Chunk::Integer_Size(chunk_header_.bits_), _swap );

        int next_id_1      = -1;
        int to_vertex_id_1 = -1;
        int face_id_1      = -1;
        bytes_ += restore( _is, next_id_1,      OMFormat::Chunk::Integer_Size(chunk_header_.bits_), _swap );
        bytes_ += restore( _is, to_vertex_id_1, OMFormat::Chunk::Integer_Size(chunk_header_.bits_), _swap );
        bytes_ += restore( _is, face_id_1,      OMFormat::Chunk::Integer_Size(chunk_header_.bits_), _swap );

        auto heh0 = _bi.add_edge(VertexHandle(to_vertex_id_1), VertexHandle(to_vertex_id_0));
        auto heh1 = HalfedgeHandle(heh0.idx() + 1);

        next_halfedges.push_back(HalfedgeHandle(next_id_0));
        next_halfedges.push_back(HalfedgeHandle(next_id_1));

        _bi.set_face(heh0, FaceHandle(face_id_0));
        _bi.set_face(heh1, FaceHandle(face_id_1));
      }

      for (size_t i = 0; i < next_halfedges.size(); ++i)
        _bi.set_next(HalfedgeHandle(static_cast<int>(i)), next_halfedges[i]);
    }

      break;

    case Chunk::Type_Status:
    {
      assert( OMFormat::dimensions(chunk_header_) == 1);

      fileOptions_ += Options::Status;

      for (size_t hidx = 0; hidx < header_.n_edges_ * 2 && !_is.eof(); ++hidx) {
        bytes_ += restore(_is, status, _swap);
        if (fileOptions_.halfedge_has_status() && _opt.halfedge_has_status())
          _bi.set_status(HalfedgeHandle(int(hidx)), status);
      }
      break;
    }

    // lumberyard change begin
    case Chunk::Type_Texcoord:
    {
      assert(OMFormat::dimensions(chunk_header_) == 2);

      fileOptions_ += Options::FaceTexCoord;

      OpenMesh::Vec2f v2f;
      for (size_t hidx = 0; hidx < header_.n_edges_ * 2 && !_is.eof(); ++hidx) {
        bytes_ += restore(_is, v2f, _swap);
        if (fileOptions_.face_has_texcoord() && _opt.face_has_texcoord())
          _bi.set_texcoord(HalfedgeHandle(int(hidx)), v2f);
      }
      break;
    }
    // lumberyard change end

    default:
      // skip unknown chunk
      omerr() << "Unknown chunk type ignored!\n";
      size_t size_of = OMFormat::chunk_data_size(header_, chunk_header_);
      _is.ignore(size_of);
      bytes_ += size_of;
  }

  return b < bytes_;
}


//-----------------------------------------------------------------------------

bool _OMReader_::read_binary_mesh_chunk(std::istream &_is, BaseImporter &_bi, Options & /* _opt */, bool _swap) const
{
  using OMFormat::Chunk;

  assert( chunk_header_.entity_ == Chunk::Entity_Mesh);

  size_t b = bytes_;

  switch (chunk_header_.type_) {
    case Chunk::Type_Custom:

      bytes_ += restore_binary_custom_data(_is, _bi.kernel()->_get_mprop(property_name_), 1, _swap);

      break;

    default:
      // skip unknown chunk
      size_t size_of = OMFormat::chunk_data_size(header_, chunk_header_);
      _is.ignore(size_of);
      bytes_ += size_of;
  }

  return b < bytes_;
}


//-----------------------------------------------------------------------------


size_t _OMReader_::restore_binary_custom_data(std::istream& _is, BaseProperty* _bp, size_t _n_elem, bool _swap) const
{
  assert( !_bp || (_bp->name() == property_name_));

  using OMFormat::Chunk;

  size_t bytes = 0;
  Chunk::esize_t block_size;
  Chunk::PropertyName custom_prop;

  bytes += restore(_is, block_size, OMFormat::Chunk::Integer_32, _swap);

  if (_bp) {
    size_t n_bytes = _bp->size_of(_n_elem);

    if (((n_bytes == BaseProperty::UnknownSize) || (n_bytes == block_size))
        && (_bp->element_size() == BaseProperty::UnknownSize || (_n_elem * _bp->element_size() == block_size))) {
#if defined(OM_DEBUG)
      size_t b;
      bytes += (b=_bp->restore( _is, _swap ));
#else
      bytes += _bp->restore(_is, _swap);
#endif

#if defined(OM_DEBUG)
      assert( block_size == b );
#endif

      assert( block_size == _bp->size_of());

      block_size = 0;
    } else {
      omerr() << "Warning! Property " << _bp->name() << " not loaded: " << "Mismatching data sizes!n";
    }
  }

  if (block_size) {
    _is.ignore(block_size);
    bytes += block_size;
  }

  return bytes;
}


//-----------------------------------------------------------------------------

//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
