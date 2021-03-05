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
//  Implements an exporter module for arbitrary OpenMesh meshes
//
//=============================================================================


#ifndef __EXPORTERT_HH__
#define __EXPORTERT_HH__


//=== INCLUDES ================================================================

// C++
#include <vector>

// OpenMesh
#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Geometry/VectorT.hh>
#include <OpenMesh/Core/Utils/GenProg.hh>
#include <OpenMesh/Core/Utils/vector_cast.hh>
#include <OpenMesh/Core/Utils/color_cast.hh>
#include <OpenMesh/Core/IO/exporter/BaseExporter.hh>


//=== NAMESPACES ==============================================================

namespace OpenMesh {
namespace IO {


//=== EXPORTER CLASS ==========================================================

/**
 *  This class template provides an exporter module for OpenMesh meshes.
 */
template <class Mesh>
class ExporterT : public BaseExporter
{
public:

  // Constructor
  ExporterT(const Mesh& _mesh) : mesh_(_mesh) {}


  // get vertex data

  Vec3f  point(VertexHandle _vh)    const override
  {
    return vector_cast<Vec3f>(mesh_.point(_vh));
  }

  Vec3f  normal(VertexHandle _vh)   const override
  {
    return (mesh_.has_vertex_normals()
	    ? vector_cast<Vec3f>(mesh_.normal(_vh))
	    : Vec3f(0.0f, 0.0f, 0.0f));
  }

  Vec3uc color(VertexHandle _vh)    const override
  {
    return (mesh_.has_vertex_colors()
	    ? color_cast<Vec3uc>(mesh_.color(_vh))
	    : Vec3uc(0, 0, 0));
  }

  Vec4uc colorA(VertexHandle _vh)   const override
  {
    return (mesh_.has_vertex_colors()
      ? color_cast<Vec4uc>(mesh_.color(_vh))
      : Vec4uc(0, 0, 0, 0));
  }

  Vec3ui colori(VertexHandle _vh)    const override
  {
    return (mesh_.has_vertex_colors()
	    ? color_cast<Vec3ui>(mesh_.color(_vh))
	    : Vec3ui(0, 0, 0));
  }

  Vec4ui colorAi(VertexHandle _vh)   const override
  {
    return (mesh_.has_vertex_colors()
      ? color_cast<Vec4ui>(mesh_.color(_vh))
      : Vec4ui(0, 0, 0, 0));
  }

  Vec3f colorf(VertexHandle _vh)    const override
  {
    return (mesh_.has_vertex_colors()
	    ? color_cast<Vec3f>(mesh_.color(_vh))
	    : Vec3f(0, 0, 0));
  }

  Vec4f colorAf(VertexHandle _vh)   const override
  {
    return (mesh_.has_vertex_colors()
      ? color_cast<Vec4f>(mesh_.color(_vh))
      : Vec4f(0, 0, 0, 0));
  }

  Vec2f  texcoord(VertexHandle _vh) const override
  {
#if defined(OM_CC_GCC) && (OM_CC_VERSION<30000)
    // Workaround!
    // gcc 2.95.3 exits with internal compiler error at the
    // code below!??? **)
    if (mesh_.has_vertex_texcoords2D())
      return vector_cast<Vec2f>(mesh_.texcoord2D(_vh));
    return Vec2f(0.0f, 0.0f);
#else // **)
    return (mesh_.has_vertex_texcoords2D()
	    ? vector_cast<Vec2f>(mesh_.texcoord2D(_vh))
	    : Vec2f(0.0f, 0.0f));
#endif
  }

  Vec2f  texcoord(HalfedgeHandle _heh) const override
  {
    return (mesh_.has_halfedge_texcoords2D()
        ? vector_cast<Vec2f>(mesh_.texcoord2D(_heh))
        : Vec2f(0.0f, 0.0f));
  }

  OpenMesh::Attributes::StatusInfo  status(VertexHandle _vh) const override
  {
    if (mesh_.has_vertex_status())
      return mesh_.status(_vh);
    return OpenMesh::Attributes::StatusInfo();
  }

  // get edge data

  Vec3uc color(EdgeHandle _eh)    const override
  {
      return (mesh_.has_edge_colors()
      ? color_cast<Vec3uc>(mesh_.color(_eh))
      : Vec3uc(0, 0, 0));
  }

  Vec4uc colorA(EdgeHandle _eh)   const override
  {
      return (mesh_.has_edge_colors()
      ? color_cast<Vec4uc>(mesh_.color(_eh))
      : Vec4uc(0, 0, 0, 0));
  }

  Vec3ui colori(EdgeHandle _eh)    const override
  {
      return (mesh_.has_edge_colors()
      ? color_cast<Vec3ui>(mesh_.color(_eh))
      : Vec3ui(0, 0, 0));
  }

  Vec4ui colorAi(EdgeHandle _eh)   const override
  {
      return (mesh_.has_edge_colors()
      ? color_cast<Vec4ui>(mesh_.color(_eh))
      : Vec4ui(0, 0, 0, 0));
  }

  Vec3f colorf(EdgeHandle _eh)    const override
  {
    return (mesh_.has_vertex_colors()
	    ? color_cast<Vec3f>(mesh_.color(_eh))
	    : Vec3f(0, 0, 0));
  }

  Vec4f colorAf(EdgeHandle _eh)   const override
  {
    return (mesh_.has_vertex_colors()
      ? color_cast<Vec4f>(mesh_.color(_eh))
      : Vec4f(0, 0, 0, 0));
  }

  OpenMesh::Attributes::StatusInfo  status(EdgeHandle _eh) const override
  {
    if (mesh_.has_edge_status())
      return mesh_.status(_eh);
    return OpenMesh::Attributes::StatusInfo();
  }

  // get halfedge data

  int get_halfedge_id(VertexHandle _vh) override
  {
    return mesh_.halfedge_handle(_vh).idx();
  }

  int get_halfedge_id(FaceHandle _fh) override
  {
    return mesh_.halfedge_handle(_fh).idx();
  }

  int get_next_halfedge_id(HalfedgeHandle _heh) override
  {
    return mesh_.next_halfedge_handle(_heh).idx();
  }

  int get_to_vertex_id(HalfedgeHandle _heh) override
  {
    return mesh_.to_vertex_handle(_heh).idx();
  }

  int get_face_id(HalfedgeHandle _heh) override
  {
    return mesh_.face_handle(_heh).idx();
  }

  OpenMesh::Attributes::StatusInfo  status(HalfedgeHandle _heh) const override
  {
    if (mesh_.has_halfedge_status())
      return mesh_.status(_heh);
    return OpenMesh::Attributes::StatusInfo();
  }

  // get face data

  unsigned int get_vhandles(FaceHandle _fh,
			    std::vector<VertexHandle>& _vhandles) const override
  {
    unsigned int count(0);
    _vhandles.clear();
    for (typename Mesh::CFVIter fv_it=mesh_.cfv_iter(_fh); fv_it.is_valid(); ++fv_it)
    {
      _vhandles.push_back(*fv_it);
      ++count;
    }
    return count;
  }

  unsigned int get_face_texcoords(std::vector<Vec2f>& _hehandles) const override
  {
    unsigned int count(0);
    _hehandles.clear();
    for(typename Mesh::CHIter he_it=mesh_.halfedges_begin();
        he_it != mesh_.halfedges_end(); ++he_it)
    {
      _hehandles.push_back(vector_cast<Vec2f>(mesh_.texcoord2D( *he_it)));
      ++count;
    }

    return count;
  }

  HalfedgeHandle getHeh(FaceHandle _fh, VertexHandle _vh) const override
  {
    typename Mesh::ConstFaceHalfedgeIter fh_it;
    for(fh_it = mesh_.cfh_iter(_fh); fh_it.is_valid();++fh_it)
    {
      if(mesh_.to_vertex_handle(*fh_it) == _vh)
        return *fh_it;
    }
    return *fh_it;
  }

  Vec3f  normal(FaceHandle _fh)   const override
  {
    return (mesh_.has_face_normals()
            ? vector_cast<Vec3f>(mesh_.normal(_fh))
            : Vec3f(0.0f, 0.0f, 0.0f));
  }

  Vec3uc  color(FaceHandle _fh)   const override
  {
    return (mesh_.has_face_colors()
            ? color_cast<Vec3uc>(mesh_.color(_fh))
            : Vec3uc(0, 0, 0));
  }

  Vec4uc  colorA(FaceHandle _fh)   const override
  {
    return (mesh_.has_face_colors()
            ? color_cast<Vec4uc>(mesh_.color(_fh))
            : Vec4uc(0, 0, 0, 0));
  }

  Vec3ui  colori(FaceHandle _fh)   const override
  {
    return (mesh_.has_face_colors()
            ? color_cast<Vec3ui>(mesh_.color(_fh))
            : Vec3ui(0, 0, 0));
  }

  Vec4ui  colorAi(FaceHandle _fh)   const override
  {
    return (mesh_.has_face_colors()
            ? color_cast<Vec4ui>(mesh_.color(_fh))
            : Vec4ui(0, 0, 0, 0));
  }

  Vec3f colorf(FaceHandle _fh)    const override
  {
    return (mesh_.has_face_colors()
	    ? color_cast<Vec3f>(mesh_.color(_fh))
	    : Vec3f(0, 0, 0));
  }

  Vec4f colorAf(FaceHandle _fh)   const override
  {
    return (mesh_.has_face_colors()
      ? color_cast<Vec4f>(mesh_.color(_fh))
      : Vec4f(0, 0, 0, 0));
  }

  OpenMesh::Attributes::StatusInfo  status(FaceHandle _fh) const override
  {
    if (mesh_.has_face_status())
      return mesh_.status(_fh);
    return OpenMesh::Attributes::StatusInfo();
  }

  virtual const BaseKernel* kernel() override { return &mesh_; }


  // query number of faces, vertices, normals, texcoords
  size_t n_vertices()  const override { return mesh_.n_vertices(); }
  size_t n_faces()     const override { return mesh_.n_faces(); }
  size_t n_edges()     const override { return mesh_.n_edges(); }


  // property information
  bool is_triangle_mesh() const override
  { return Mesh::is_triangles(); }

  bool has_vertex_normals()   const override { return mesh_.has_vertex_normals();   }
  bool has_vertex_colors()    const override { return mesh_.has_vertex_colors();    }
  bool has_vertex_texcoords() const override { return mesh_.has_vertex_texcoords2D(); }
  bool has_vertex_status()    const override { return mesh_.has_vertex_status();    }
  bool has_edge_colors()      const override { return mesh_.has_edge_colors();      }
  bool has_edge_status()      const override { return mesh_.has_edge_status();      }
  bool has_halfedge_status()  const override { return mesh_.has_halfedge_status();  }
  bool has_face_normals()     const override { return mesh_.has_face_normals();     }
  bool has_face_colors()      const override { return mesh_.has_face_colors();      }
  bool has_face_status()      const override { return mesh_.has_face_status();      }

private:

   const Mesh& mesh_;
};


//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
#endif
//=============================================================================
