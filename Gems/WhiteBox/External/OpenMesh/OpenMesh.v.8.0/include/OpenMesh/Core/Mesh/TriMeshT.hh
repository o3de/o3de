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
//  CLASS TriMeshT
//
//=============================================================================


#ifndef OPENMESH_TRIMESH_HH
#define OPENMESH_TRIMESH_HH


//== INCLUDES =================================================================


#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Mesh/PolyMeshT.hh>
#include <OpenMesh/Core/Mesh/Tags.hh>
#include <vector>


//== NAMESPACES ===============================================================


namespace OpenMesh {


//== CLASS DEFINITION =========================================================


/** \class TriMeshT TriMeshT.hh <OpenMesh/Mesh/TriMeshT.hh>

    Base type for a triangle mesh.

    Base type for a triangle mesh, parameterized by a mesh kernel. The
    mesh inherits all methods from the kernel class and the
    more general polygonal mesh PolyMeshT. Therefore it provides
    the same types for items, handles, iterators and so on.

    \param Kernel: template argument for the mesh kernel
    \note You should use the predefined mesh-kernel combinations in
    \ref mesh_types_group
    \see \ref mesh_type
    \see OpenMesh::PolyMeshT
*/

template <class Kernel>
class TriMeshT : public PolyMeshT<Kernel>
{

public:


  // self
  typedef TriMeshT<Kernel>                      This;
  typedef PolyMeshT<Kernel>                     PolyMesh;

  //@{
  /// Determine whether this is a PolyMeshT or TriMeshT (This function does not check the per face vertex count! It only checks if the datatype is PolyMeshT or TriMeshT)
  static constexpr bool is_polymesh() { return false; }
  static constexpr bool is_trimesh()  { return true;  }
  using ConnectivityTag = TriConnectivityTag;
  enum { IsPolyMesh = 0 };
  enum { IsTriMesh  = 1 };
  //@}

  //--- items ---

  typedef typename PolyMesh::Scalar             Scalar;
  typedef typename PolyMesh::Point              Point;
  typedef typename PolyMesh::Normal             Normal;
  typedef typename PolyMesh::Color              Color;
  typedef typename PolyMesh::TexCoord1D         TexCoord1D;
  typedef typename PolyMesh::TexCoord2D         TexCoord2D;
  typedef typename PolyMesh::TexCoord3D         TexCoord3D;
  typedef typename PolyMesh::Vertex             Vertex;
  typedef typename PolyMesh::Halfedge           Halfedge;
  typedef typename PolyMesh::Edge               Edge;
  typedef typename PolyMesh::Face               Face;


  //--- handles ---

  typedef typename PolyMesh::VertexHandle       VertexHandle;
  typedef typename PolyMesh::HalfedgeHandle     HalfedgeHandle;
  typedef typename PolyMesh::EdgeHandle         EdgeHandle;
  typedef typename PolyMesh::FaceHandle         FaceHandle;


  //--- iterators ---

  typedef typename PolyMesh::VertexIter         VertexIter;
  typedef typename PolyMesh::ConstVertexIter    ConstVertexIter;
  typedef typename PolyMesh::EdgeIter           EdgeIter;
  typedef typename PolyMesh::ConstEdgeIter      ConstEdgeIter;
  typedef typename PolyMesh::FaceIter           FaceIter;
  typedef typename PolyMesh::ConstFaceIter      ConstFaceIter;



  //--- circulators ---

  typedef typename PolyMesh::VertexVertexIter         VertexVertexIter;
  typedef typename PolyMesh::VertexOHalfedgeIter      VertexOHalfedgeIter;
  typedef typename PolyMesh::VertexIHalfedgeIter      VertexIHalfedgeIter;
  typedef typename PolyMesh::VertexEdgeIter           VertexEdgeIter;
  typedef typename PolyMesh::VertexFaceIter           VertexFaceIter;
  typedef typename PolyMesh::FaceVertexIter           FaceVertexIter;
  typedef typename PolyMesh::FaceHalfedgeIter         FaceHalfedgeIter;
  typedef typename PolyMesh::FaceEdgeIter             FaceEdgeIter;
  typedef typename PolyMesh::FaceFaceIter             FaceFaceIter;
  typedef typename PolyMesh::ConstVertexVertexIter    ConstVertexVertexIter;
  typedef typename PolyMesh::ConstVertexOHalfedgeIter ConstVertexOHalfedgeIter;
  typedef typename PolyMesh::ConstVertexIHalfedgeIter ConstVertexIHalfedgeIter;
  typedef typename PolyMesh::ConstVertexEdgeIter      ConstVertexEdgeIter;
  typedef typename PolyMesh::ConstVertexFaceIter      ConstVertexFaceIter;
  typedef typename PolyMesh::ConstFaceVertexIter      ConstFaceVertexIter;
  typedef typename PolyMesh::ConstFaceHalfedgeIter    ConstFaceHalfedgeIter;
  typedef typename PolyMesh::ConstFaceEdgeIter        ConstFaceEdgeIter;
  typedef typename PolyMesh::ConstFaceFaceIter        ConstFaceFaceIter;

  // --- constructor/destructor

  /// Default constructor
  TriMeshT() : PolyMesh() {}
  explicit TriMeshT(PolyMesh rhs) : PolyMesh((rhs.triangulate(), rhs))
  {
  }

  /// Destructor
  virtual ~TriMeshT() {}

  //--- halfedge collapse / vertex split ---

  /** \brief Vertex Split: inverse operation to collapse().
   *
   * Insert the new vertex at position v0. The vertex will be added
   * as the inverse of the vertex split. The faces above the split
   * will be correctly attached to the two new edges
   *
   * Before:
   * v_l     v0     v_r
   *  x      x      x
   *   \           /
   *    \         /
   *     \       /
   *      \     /
   *       \   /
   *        \ /
   *         x
   *         v1
   *
   * After:
   * v_l    v0      v_r
   *  x------x------x
   *   \     |     /
   *    \    |    /
   *     \   |   /
   *      \  |  /
   *       \ | /
   *        \|/
   *         x
   *         v1
   *
   * @param _v0_point Point position for the new point
   * @param _v1       Vertex that will be split
   * @param _vl       Left vertex handle
   * @param _vr       Right vertex handle
   * @return Newly inserted halfedge
   */
  inline HalfedgeHandle vertex_split(Point _v0_point,  VertexHandle _v1,
                                     VertexHandle _vl, VertexHandle _vr)
  { return PolyMesh::vertex_split(this->add_vertex(_v0_point), _v1, _vl, _vr); }

  /** \brief Vertex Split: inverse operation to collapse().
   *
   * Insert the new vertex at position v0. The vertex will be added
   * as the inverse of the vertex split. The faces above the split
   * will be correctly attached to the two new edges
   *
   * Before:
   * v_l     v0     v_r
   *  x      x      x
   *   \           /
   *    \         /
   *     \       /
   *      \     /
   *       \   /
   *        \ /
   *         x
   *         v1
   *
   * After:
   * v_l    v0      v_r
   *  x------x------x
   *   \     |     /
   *    \    |    /
   *     \   |   /
   *      \  |  /
   *       \ | /
   *        \|/
   *         x
   *         v1
   *
   * @param _v0 Vertex handle for the newly inserted point (Input has to be unconnected!)
   * @param _v1 Vertex that will be split
   * @param _vl Left vertex handle
   * @param _vr Right vertex handle
   * @return Newly inserted halfedge
   */
  inline HalfedgeHandle vertex_split(VertexHandle _v0, VertexHandle _v1,
                                     VertexHandle _vl, VertexHandle _vr)
  { return PolyMesh::vertex_split(_v0, _v1, _vl, _vr); }

  /** \brief Edge split (= 2-to-4 split)
   *
   * The properties of the new edges will be undefined!
   *
   *
   * @param _eh Edge handle that should be splitted
   * @param _p  New point position that will be inserted at the edge
   * @return    Vertex handle of the newly added vertex
   */
  inline VertexHandle split(EdgeHandle _eh, const Point& _p)
  {
    //Do not call PolyMeshT function below as this does the wrong operation
    const VertexHandle vh = this->add_vertex(_p); Kernel::split(_eh, vh); return vh;
  }

  /** \brief Edge split (= 2-to-4 split)
   *
   * The properties of the new edges will be adjusted to the properties of the original edge
   *
   * @param _eh Edge handle that should be splitted
   * @param _p  New point position that will be inserted at the edge
   * @return    Vertex handle of the newly added vertex
   */
  inline VertexHandle split_copy(EdgeHandle _eh, const Point& _p)
  {
    //Do not call PolyMeshT function below as this does the wrong operation
    const VertexHandle vh = this->add_vertex(_p); Kernel::split_copy(_eh, vh); return vh;
  }

  /** \brief Edge split (= 2-to-4 split)
   *
   * The properties of the new edges will be undefined!
   *
   * @param _eh Edge handle that should be splitted
   * @param _vh Vertex handle that will be inserted at the edge
   */
  inline void split(EdgeHandle _eh, VertexHandle _vh)
  {
    //Do not call PolyMeshT function below as this does the wrong operation
    Kernel::split(_eh, _vh);
  }

  /** \brief Edge split (= 2-to-4 split)
   *
   * The properties of the new edges will be adjusted to the properties of the original edge
   *
   * @param _eh Edge handle that should be splitted
   * @param _vh Vertex handle that will be inserted at the edge
   */
  inline void split_copy(EdgeHandle _eh, VertexHandle _vh)
  {
    //Do not call PolyMeshT function below as this does the wrong operation
    Kernel::split_copy(_eh, _vh);
  }

  /** \brief Face split (= 1-to-3 split, calls corresponding PolyMeshT function).
   *
   * The properties of the new faces will be undefined!
   *
   * @param _fh Face handle that should be splitted
   * @param _p  New point position that will be inserted in the face
   *
   * @return Vertex handle of the new vertex
   */
  inline VertexHandle split(FaceHandle _fh, const Point& _p)
  { const VertexHandle vh = this->add_vertex(_p); PolyMesh::split(_fh, vh); return vh; }

  /** \brief Face split (= 1-to-3 split, calls corresponding PolyMeshT function).
   *
   * The properties of the new faces will be adjusted to the properties of the original face
   *
   * @param _fh Face handle that should be splitted
   * @param _p  New point position that will be inserted in the face
   *
   * @return Vertex handle of the new vertex
   */
  inline VertexHandle split_copy(FaceHandle _fh, const Point& _p)
  { const VertexHandle vh = this->add_vertex(_p);  PolyMesh::split_copy(_fh, vh); return vh; }


  /** \brief Face split (= 1-to-4) split, splits edges at midpoints and adds 4 new faces in the interior).
   *
   * @param _fh Face handle that should be splitted
   */
  inline void split(FaceHandle _fh)
  {
    // Collect halfedges of face
    HalfedgeHandle he0 = this->halfedge_handle(_fh);
    HalfedgeHandle he1 = this->next_halfedge_handle(he0);
    HalfedgeHandle he2 = this->next_halfedge_handle(he1);

    EdgeHandle eh0 = this->edge_handle(he0);
    EdgeHandle eh1 = this->edge_handle(he1);
    EdgeHandle eh2 = this->edge_handle(he2);

    // Collect points of face
    VertexHandle p0 = this->to_vertex_handle(he0);
    VertexHandle p1 = this->to_vertex_handle(he1);
    VertexHandle p2 = this->to_vertex_handle(he2);

    // Calculate midpoint coordinates
    const Point new0 = (this->point(p0) + this->point(p2)) * static_cast<typename vector_traits<Point>::value_type >(0.5);
    const Point new1 = (this->point(p0) + this->point(p1)) * static_cast<typename vector_traits<Point>::value_type >(0.5);
    const Point new2 = (this->point(p1) + this->point(p2)) * static_cast<typename vector_traits<Point>::value_type >(0.5);

    // Add vertices at midpoint coordinates
    VertexHandle v0 = this->add_vertex(new0);
    VertexHandle v1 = this->add_vertex(new1);
    VertexHandle v2 = this->add_vertex(new2);

    const bool split0 = !this->is_boundary(eh0);
    const bool split1 = !this->is_boundary(eh1);
    const bool split2 = !this->is_boundary(eh2);

    // delete original face
    this->delete_face(_fh);

    // split boundary edges of deleted face ( if not boundary )
    if ( split0 ) {
      this->split(eh0,v0);
    }

    if ( split1 ) {
      this->split(eh1,v1);
    }

    if ( split2 ) {
      this->split(eh2,v2);
    }

    // Retriangulate
    this->add_face(v0 , p0, v1);
    this->add_face(p2, v0 , v2);
    this->add_face(v2,v1,p1);
    this->add_face(v2 , v0, v1);
  }

  /** \brief Face split (= 1-to-3 split, calls corresponding PolyMeshT function).
   *
   * The properties of the new faces will be undefined!
   *
   * @param _fh Face handle that should be splitted
   * @param _vh Vertex handle that will be inserted at the face
   */
  inline void split(FaceHandle _fh, VertexHandle _vh)
  { PolyMesh::split(_fh, _vh); }

  /** \brief Face split (= 1-to-3 split, calls corresponding PolyMeshT function).
   *
   * The properties of the new faces will be adjusted to the properties of the original face
   *
   * @param _fh Face handle that should be splitted
   * @param _vh Vertex handle that will be inserted at the face
   */
  inline void split_copy(FaceHandle _fh, VertexHandle _vh)
  { PolyMesh::split_copy(_fh, _vh); }
  
  /** \name Normal vector computation
  */
  //@{

  /** Calculate normal vector for face _fh (specialized for TriMesh). */
  Normal calc_face_normal(FaceHandle _fh) const;

  //@}
};


//=============================================================================
} // namespace OpenMesh
//=============================================================================
#if defined(OM_INCLUDE_TEMPLATES) && !defined(OPENMESH_TRIMESH_C)
#define OPENMESH_TRIMESH_TEMPLATES
#include "TriMeshT_impl.hh"
#endif
//=============================================================================
#endif // OPENMESH_TRIMESH_HH defined
//=============================================================================
