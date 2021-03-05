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

#include <OpenMesh/Core/System/config.h>
// -------------------- STL
#if defined( OM_CC_MIPS )
  #include <time.h>
  #include <string.h>
#else
  #include <ctime>
  #include <cstring>
#endif

#include <fstream>
#include <vector>

// -------------------- OpenMesh
#include <OpenMesh/Core/IO/OMFormat.hh>
#include <OpenMesh/Core/IO/exporter/BaseExporter.hh>
#include <OpenMesh/Core/IO/writer/OMWriter.hh>

//=== NAMESPACES ==============================================================


namespace OpenMesh {
namespace IO {


//=== INSTANCIATE =============================================================


// register the OMLoader singleton with MeshLoader
_OMWriter_  __OMWriterInstance;
_OMWriter_& OMWriter() { return __OMWriterInstance; }


//=== IMPLEMENTATION ==========================================================


const OMFormat::uchar _OMWriter_::magic_[3] = "OM";
const OMFormat::uint8 _OMWriter_::version_  = OMFormat::mk_version(2,0);


_OMWriter_::
_OMWriter_()
{
  IOManager().register_module(this);
}


bool
_OMWriter_::write(const std::string& _filename, BaseExporter& _be,
                   Options _opt, std::streamsize /*_precision*/) const
{
  // check whether exporter can give us an OpenMesh BaseKernel
  if (!_be.kernel()) return false;


  // check for om extension in filename, we can only handle OM
  if (_filename.rfind(".om") == std::string::npos)
    return false;

  _opt += Options::Binary; // only binary format supported

  std::ofstream ofs(_filename.c_str(), std::ios::binary);

  // check if file is open
  if (!ofs.is_open())
  {
    omerr() << "[OMWriter] : cannot open file " << _filename << std::endl;
    return false;
  }

  // call stream save method
  bool rc = write(ofs, _be, _opt);

  // close filestream
  ofs.close();

  // return success/failure notice
  return rc;
}


//-----------------------------------------------------------------------------

bool
_OMWriter_::write(std::ostream& _os, BaseExporter& _be, Options _opt, std::streamsize /*_precision*/) const
{
//   std::clog << "[OMWriter]::write( stream )\n";

  // check exporter features
  if ( !check( _be, _opt ) )
  {
    omerr() << "[OMWriter]: exporter does not support wanted feature!\n";
    return false;
  }

  // Maybe an ascii version will be implemented in the future.
  // For now, support only a binary format
  if ( !_opt.check( Options::Binary ) )
    _opt += Options::Binary;

  // Ignore LSB/MSB bit. Always store in LSB (little endian)
  _opt += Options::LSB;
  _opt -= Options::MSB;

  return write_binary(_os, _be, _opt);
}


//-----------------------------------------------------------------------------


#ifndef DOXY_IGNORE_THIS
template <typename T> struct Enabler
{
  explicit Enabler( T& obj ) : obj_(obj)
  {}

  ~Enabler() { obj_.enable(); }

  T& obj_;
};
#endif


bool _OMWriter_::write_binary(std::ostream& _os, BaseExporter& _be,
                               Options _opt) const
{
  #ifndef DOXY_IGNORE_THIS
    Enabler<mostream> enabler(omlog());
  #endif

  size_t bytes = 0;

  bool swap = _opt.check(Options::Swap) || (Endian::local() == Endian::MSB);

  unsigned int i, nV, nF;
  Vec3f v;
  Vec2f t;

  // -------------------- write header
  OMFormat::Header header;

  header.magic_[0]   = 'O';
  header.magic_[1]   = 'M';
  header.mesh_       = _be.is_triangle_mesh() ? 'T' : 'P';
  header.version_    = version_;
  header.n_vertices_ = int(_be.n_vertices());
  header.n_faces_    = int(_be.n_faces());
  header.n_edges_    = int(_be.n_edges());

  bytes += store( _os, header, swap );

  // ---------------------------------------- write chunks

  OMFormat::Chunk::Header chunk_header;


  // -------------------- write vertex data

  // ---------- write vertex position
  if (_be.n_vertices())
  {
    v = _be.point(VertexHandle(0));
    chunk_header.reserved_ = 0;
    chunk_header.name_     = false;
    chunk_header.entity_   = OMFormat::Chunk::Entity_Vertex;
    chunk_header.type_     = OMFormat::Chunk::Type_Pos;
    chunk_header.signed_   = OMFormat::is_signed(v[0]);
    chunk_header.float_    = OMFormat::is_float(v[0]);
    chunk_header.dim_      = OMFormat::dim(v);
    chunk_header.bits_     = OMFormat::bits(v[0]);

    bytes += store( _os, chunk_header, swap );
    for (i=0, nV=header.n_vertices_; i<nV; ++i)
      bytes += vector_store( _os, _be.point(VertexHandle(i)), swap );
  }


  // ---------- write vertex normal
  if (_be.n_vertices() && _opt.check( Options::VertexNormal ))
  {
    Vec3f n = _be.normal(VertexHandle(0));

    chunk_header.name_     = false;
    chunk_header.entity_   = OMFormat::Chunk::Entity_Vertex;
    chunk_header.type_     = OMFormat::Chunk::Type_Normal;
    chunk_header.signed_   = OMFormat::is_signed(n[0]);
    chunk_header.float_    = OMFormat::is_float(n[0]);
    chunk_header.dim_      = OMFormat::dim(n);
    chunk_header.bits_     = OMFormat::bits(n[0]);

    bytes += store( _os, chunk_header, swap );
    for (i=0, nV=header.n_vertices_; i<nV; ++i)
      bytes += vector_store( _os, _be.normal(VertexHandle(i)), swap );
  }

  // ---------- write vertex color
  if (_be.n_vertices() && _opt.check( Options::VertexColor ) && _be.has_vertex_colors() )
  {
    Vec3uc c = _be.color(VertexHandle(0));

    chunk_header.name_     = false;
    chunk_header.entity_   = OMFormat::Chunk::Entity_Vertex;
    chunk_header.type_     = OMFormat::Chunk::Type_Color;
    chunk_header.signed_   = OMFormat::is_signed( c[0] );
    chunk_header.float_    = OMFormat::is_float( c[0] );
    chunk_header.dim_      = OMFormat::dim( c );
    chunk_header.bits_     = OMFormat::bits( c[0] );

    bytes += store( _os, chunk_header, swap );
    for (i=0, nV=header.n_vertices_; i<nV; ++i)
      bytes += vector_store( _os, _be.color(VertexHandle(i)), swap );
  }

  // ---------- write vertex texture coords
  if (_be.n_vertices() && _opt.check(Options::VertexTexCoord)) {

    t = _be.texcoord(VertexHandle(0));

    chunk_header.name_ = false;
    chunk_header.entity_ = OMFormat::Chunk::Entity_Vertex;
    chunk_header.type_ = OMFormat::Chunk::Type_Texcoord;
    chunk_header.signed_ = OMFormat::is_signed(t[0]);
    chunk_header.float_ = OMFormat::is_float(t[0]);
    chunk_header.dim_ = OMFormat::dim(t);
    chunk_header.bits_ = OMFormat::bits(t[0]);

    // std::clog << chunk_header << std::endl;
    bytes += store(_os, chunk_header, swap);

    for (i = 0, nV = header.n_vertices_; i < nV; ++i)
      bytes += vector_store(_os, _be.texcoord(VertexHandle(i)), swap);

  }

  // ---------- write halfedge data
  if (_be.n_edges())
  {
    chunk_header.reserved_ = 0;
    chunk_header.name_     = false;
    chunk_header.entity_   = OMFormat::Chunk::Entity_Halfedge;
    chunk_header.type_     = OMFormat::Chunk::Type_Topology;
    chunk_header.signed_   = true;
    chunk_header.float_    = true; // TODO: is this correct? This causes a scalar size of 1 in OMFormat.hh scalar_size which we need I think?
    chunk_header.dim_      = OMFormat::Chunk::Dim_3D;
    chunk_header.bits_     = OMFormat::needed_bits(_be.n_edges()*4); // *2 due to halfedge ids being stored, *2 due to signedness

    bytes += store( _os, chunk_header, swap );
    auto nE=header.n_edges_*2;
    for (i=0; i<nE; ++i)
    {
      auto next_id      = _be.get_next_halfedge_id(HalfedgeHandle(static_cast<int>(i)));
      auto to_vertex_id = _be.get_to_vertex_id(HalfedgeHandle(static_cast<int>(i)));
      auto face_id      = _be.get_face_id(HalfedgeHandle(static_cast<int>(i)));

      bytes += store( _os, next_id,      OMFormat::Chunk::Integer_Size(chunk_header.bits_), swap );
      bytes += store( _os, to_vertex_id, OMFormat::Chunk::Integer_Size(chunk_header.bits_), swap );
      bytes += store( _os, face_id,      OMFormat::Chunk::Integer_Size(chunk_header.bits_), swap );
    }
  }

  // lumberyard-change begin
  // ---------- write halfedge texture coords
  if (_be.n_edges() && _be.n_faces() && _opt.check(Options::FaceTexCoord)) {

    t = _be.texcoord(HalfedgeHandle(0));

    chunk_header.name_ = false;
    chunk_header.entity_ = OMFormat::Chunk::Entity_Halfedge;
    chunk_header.type_ = OMFormat::Chunk::Type_Texcoord;
    chunk_header.signed_ = OMFormat::is_signed(t[0]);
    chunk_header.float_ = OMFormat::is_float(t[0]);
    chunk_header.dim_ = OMFormat::dim(t);
    chunk_header.bits_ = OMFormat::bits(t[0]);

    // std::clog << chunk_header << std::endl;
    bytes += store(_os, chunk_header, swap);

    auto nHE=header.n_edges_*2;
    for (i = 0; i < nHE; ++i)
      bytes += vector_store(_os, _be.texcoord(HalfedgeHandle(i)), swap);
  }
  // lumberyard-change end


  // ---------- write vertex topology (outgoing halfedge)
  if (_be.n_vertices())
  {
    chunk_header.reserved_ = 0;
    chunk_header.name_     = false;
    chunk_header.entity_   = OMFormat::Chunk::Entity_Vertex;
    chunk_header.type_     = OMFormat::Chunk::Type_Topology;
    chunk_header.signed_   = true;
    chunk_header.float_    = true; // TODO: is this correct? This causes a scalar size of 1 in OMFormat.hh scalar_size which we need I think?
    chunk_header.dim_      = OMFormat::Chunk::Dim_1D;
    chunk_header.bits_     = OMFormat::needed_bits(_be.n_edges()*4); // *2 due to halfedge ids being stored, *2 due to signedness

    bytes += store( _os, chunk_header, swap );
    for (i=0, nV=header.n_vertices_; i<nV; ++i)
      bytes += store( _os, _be.get_halfedge_id(VertexHandle(i)), OMFormat::Chunk::Integer_Size(chunk_header.bits_), swap );
  }


  // -------------------- write face data

  // ---------- write topology
  {
    chunk_header.name_     = false;
    chunk_header.entity_   = OMFormat::Chunk::Entity_Face;
    chunk_header.type_     = OMFormat::Chunk::Type_Topology;
    chunk_header.signed_   = true;
    chunk_header.float_    = true; // TODO: is this correct? This causes a scalar size of 1 in OMFormat.hh scalar_size which we need I think?
    chunk_header.dim_      = OMFormat::Chunk::Dim_1D;
    chunk_header.bits_     = OMFormat::needed_bits(_be.n_edges()*4); // *2 due to halfedge ids being stored, *2 due to signedness

    bytes += store( _os, chunk_header, swap );

    for (i=0, nF=header.n_faces_; i<nF; ++i)
    {
      auto size = OMFormat::Chunk::Integer_Size(chunk_header.bits_);
      bytes += store( _os, _be.get_halfedge_id(FaceHandle(i)), size, swap);
    }
  }

  // ---------- write face normals

  if (_be.n_faces() && _be.has_face_normals() && _opt.check(Options::FaceNormal) )
  {
#define NEW_STYLE 0
#if NEW_STYLE
    const BaseProperty *bp = _be.kernel()._get_fprop("f:normals");

    if (bp)
    {
#endif
      Vec3f n = _be.normal(FaceHandle(0));

      chunk_header.name_     = false;
      chunk_header.entity_   = OMFormat::Chunk::Entity_Face;
      chunk_header.type_     = OMFormat::Chunk::Type_Normal;
      chunk_header.signed_   = OMFormat::is_signed(n[0]);
      chunk_header.float_    = OMFormat::is_float(n[0]);
      chunk_header.dim_      = OMFormat::dim(n);
      chunk_header.bits_     = OMFormat::bits(n[0]);

      bytes += store( _os, chunk_header, swap );
#if !NEW_STYLE
      for (i=0, nF=header.n_faces_; i<nF; ++i)
        bytes += vector_store( _os, _be.normal(FaceHandle(i)), swap );
#else
      bytes += bp->store(_os, swap );
    }
    else
      return false;
#endif
#undef NEW_STYLE
  }


  // ---------- write face color

  if (_be.n_faces() && _be.has_face_colors() && _opt.check( Options::FaceColor ))
  {
#define NEW_STYLE 0
#if NEW_STYLE
    const BaseProperty *bp = _be.kernel()._get_fprop("f:colors");

    if (bp)
    {
#endif
      Vec3uc c;

      chunk_header.name_     = false;
      chunk_header.entity_   = OMFormat::Chunk::Entity_Face;
      chunk_header.type_     = OMFormat::Chunk::Type_Color;
      chunk_header.signed_   = OMFormat::is_signed( c[0] );
      chunk_header.float_    = OMFormat::is_float( c[0] );
      chunk_header.dim_      = OMFormat::dim( c );
      chunk_header.bits_     = OMFormat::bits( c[0] );

      bytes += store( _os, chunk_header, swap );
#if !NEW_STYLE
      for (i=0, nF=header.n_faces_; i<nF; ++i)
        bytes += vector_store( _os, _be.color(FaceHandle(i)), swap );
#else
      bytes += bp->store(_os, swap);
    }
    else
      return false;
#endif
  }

  // ---------- write vertex status
  if (_be.n_vertices() && _be.has_vertex_status() && _opt.check(Options::Status))
  {
    auto s = _be.status(VertexHandle(0));
    chunk_header.name_ = false;
    chunk_header.entity_ = OMFormat::Chunk::Entity_Vertex;
    chunk_header.type_ = OMFormat::Chunk::Type_Status;
    chunk_header.signed_ = false;
    chunk_header.float_ = false;
    chunk_header.dim_ = OMFormat::Chunk::Dim_1D;
    chunk_header.bits_ = OMFormat::bits(s);

    // std::clog << chunk_header << std::endl;
    bytes += store(_os, chunk_header, swap);

    for (i = 0, nV = header.n_vertices_; i < nV; ++i)
      bytes += store(_os, _be.status(VertexHandle(i)), swap);
  }

  // ---------- write edge status
  if (_be.n_edges() && _be.has_edge_status() && _opt.check(Options::Status))
  {
    auto s = _be.status(EdgeHandle(0));
    chunk_header.name_ = false;
    chunk_header.entity_ = OMFormat::Chunk::Entity_Edge;
    chunk_header.type_ = OMFormat::Chunk::Type_Status;
    chunk_header.signed_ = false;
    chunk_header.float_ = false;
    chunk_header.dim_ = OMFormat::Chunk::Dim_1D;
    chunk_header.bits_ = OMFormat::bits(s);

    // std::clog << chunk_header << std::endl;
    bytes += store(_os, chunk_header, swap);

    for (i = 0, nV = header.n_edges_; i < nV; ++i)
      bytes += store(_os, _be.status(EdgeHandle(i)), swap);
  }

  // ---------- write halfedge status
  if (_be.n_edges() && _be.has_halfedge_status() && _opt.check(Options::Status))
  {
    auto s = _be.status(HalfedgeHandle(0));
    chunk_header.name_ = false;
    chunk_header.entity_ = OMFormat::Chunk::Entity_Halfedge;
    chunk_header.type_ = OMFormat::Chunk::Type_Status;
    chunk_header.signed_ = false;
    chunk_header.float_ = false;
    chunk_header.dim_ = OMFormat::Chunk::Dim_1D;
    chunk_header.bits_ = OMFormat::bits(s);

    // std::clog << chunk_header << std::endl;
    bytes += store(_os, chunk_header, swap);

    for (i = 0, nV = header.n_edges_ * 2; i < nV; ++i)
      bytes += store(_os, _be.status(HalfedgeHandle(i)), swap);
  }

  // ---------- write face status
  if (_be.n_faces() && _be.has_face_status() && _opt.check(Options::Status))
  {
    auto s = _be.status(FaceHandle(0));
    chunk_header.name_ = false;
    chunk_header.entity_ = OMFormat::Chunk::Entity_Face;
    chunk_header.type_ = OMFormat::Chunk::Type_Status;
    chunk_header.signed_ = false;
    chunk_header.float_ = false;
    chunk_header.dim_ = OMFormat::Chunk::Dim_1D;
    chunk_header.bits_ = OMFormat::bits(s);

    // std::clog << chunk_header << std::endl;
    bytes += store(_os, chunk_header, swap);

    for (i = 0, nV = header.n_faces_; i < nV; ++i)
      bytes += store(_os, _be.status(FaceHandle(i)), swap);
  }

  // -------------------- write custom properties


  BaseKernel::const_prop_iterator prop;

  for (prop  = _be.kernel()->vprops_begin();
       prop != _be.kernel()->vprops_end(); ++prop)
  {
    if ( !*prop ) continue;
    if ( (*prop)->name()[1]==':') continue;
    bytes += store_binary_custom_chunk(_os, **prop,
				       OMFormat::Chunk::Entity_Vertex, swap );
  }
  for (prop  = _be.kernel()->fprops_begin();
       prop != _be.kernel()->fprops_end(); ++prop)
  {
    if ( !*prop ) continue;
    if ( (*prop)->name()[1]==':') continue;
    bytes += store_binary_custom_chunk(_os, **prop,
				       OMFormat::Chunk::Entity_Face, swap );
  }
  for (prop  = _be.kernel()->eprops_begin();
       prop != _be.kernel()->eprops_end(); ++prop)
  {
    if ( !*prop ) continue;
    if ( (*prop)->name()[1]==':') continue;
    bytes += store_binary_custom_chunk(_os, **prop,
				       OMFormat::Chunk::Entity_Edge, swap );
  }
  for (prop  = _be.kernel()->hprops_begin();
       prop != _be.kernel()->hprops_end(); ++prop)
  {
    if ( !*prop ) continue;
    if ( (*prop)->name()[1]==':') continue;
    bytes += store_binary_custom_chunk(_os, **prop,
				       OMFormat::Chunk::Entity_Halfedge, swap );
  }
  for (prop  = _be.kernel()->mprops_begin();
       prop != _be.kernel()->mprops_end(); ++prop)
  {
    if ( !*prop ) continue;
    if ( (*prop)->name()[1]==':') continue;
    bytes += store_binary_custom_chunk(_os, **prop,
				       OMFormat::Chunk::Entity_Mesh, swap );
  }

  memset(&chunk_header, 0, sizeof(chunk_header));
  chunk_header.name_ = false;
  chunk_header.entity_ = OMFormat::Chunk::Entity_Sentinel;
  bytes += store(_os, chunk_header, swap);

  std::clog << "#bytes written: " << bytes << std::endl;

  return true;
}

// ----------------------------------------------------------------------------

size_t _OMWriter_::store_binary_custom_chunk(std::ostream& _os,
					     const BaseProperty& _bp,
					     OMFormat::Chunk::Entity _entity,
					     bool _swap) const
{
  //omlog() << "Custom Property " << OMFormat::as_string(_entity) << " property ["
  //	<< _bp.name() << "]" << std::endl;

  // Don't store if
  // 1. it is not persistent
  // 2. it's name is empty
  if ( !_bp.persistent() || _bp.name().empty() )
  {
    //omlog() << "  skipped\n";
    return 0;
  }

  size_t bytes = 0;

  OMFormat::Chunk::esize_t element_size   = OMFormat::Chunk::esize_t(_bp.element_size());
  OMFormat::Chunk::Header  chdr;

  // set header
  chdr.name_     = true;
  chdr.entity_   = _entity;
  chdr.type_     = OMFormat::Chunk::Type_Custom;
  chdr.signed_   = 0;
  chdr.float_    = 0;
  chdr.dim_      = OMFormat::Chunk::Dim_1D; // ignored
  chdr.bits_     = element_size;


  // write custom chunk

  // 1. chunk header
  bytes += store( _os, chdr, _swap );

  // 2. property name
  bytes += store( _os, OMFormat::Chunk::PropertyName(_bp.name()), _swap );

  // 3. block size
  bytes += store( _os, _bp.size_of(), OMFormat::Chunk::Integer_32, _swap );
  //omlog() << "  n_bytes = " << _bp.size_of() << std::endl;

  // 4. data
  {
    size_t b;
    bytes += ( b=_bp.store( _os, _swap ) );
    //omlog() << "  b       = " << b << std::endl;
    assert( b == _bp.size_of() );
  }
  return bytes;
}

// ----------------------------------------------------------------------------

size_t _OMWriter_::binary_size(BaseExporter& /* _be */, Options /* _opt */) const
{
  // std::clog << "[OMWriter]: binary_size()" << std::endl;
  size_t bytes  = sizeof( OMFormat::Header );

  // !!!TODO!!!

  return bytes;
}

//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
