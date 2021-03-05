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


/** \file CollapseInfoT.hh
 Provides data class CollapseInfoT for storing all information
 about a halfedge collapse.
 */

//=============================================================================
//
//  STRUCT CollpaseInfoT
//
//=============================================================================
#ifndef OPENMESH_DECIMATER_COLLAPSEINFOT_HH
#define OPENMESH_DECIMATER_COLLAPSEINFOT_HH

//== INCLUDES =================================================================

//== NAMESPACE ================================================================

namespace OpenMesh {
namespace Decimater {

//== CLASS DEFINITION =========================================================

/** Stores information about a halfedge collapse.

 The class stores information about a halfedge collapse. The most
 important information is \c v0v1, \c v1v0, \c v0, \c v1, \c vl,
 \c vr, which you can lookup in the following image:
 \image html collapse_info.png
 \see ModProgMeshT::Info
 */
template<class Mesh>
struct CollapseInfoT {
  public:
    /** Initializing constructor.
     *
     *  Given a mesh and a halfedge handle of the halfedge to be collapsed
     *  all important information of a halfedge collapse will be stored.
     * \param _mesh Mesh source
     * \param _heh Halfedge to collapse. The direction of the halfedge
     *        defines the direction of the collapse, i.e. the from-vertex
     *        will be removed and the to-vertex remains.
     */
    CollapseInfoT(Mesh& _mesh, typename Mesh::HalfedgeHandle _heh);

    Mesh& mesh;

    typename Mesh::HalfedgeHandle v0v1; ///< Halfedge to be collapsed
    typename Mesh::HalfedgeHandle v1v0; ///< Reverse halfedge
    typename Mesh::VertexHandle v0; ///< Vertex to be removed
    typename Mesh::VertexHandle v1; ///< Remaining vertex
    typename Mesh::Point p0; ///< Position of removed vertex
    typename Mesh::Point p1; ///< Positions of remaining vertex
    typename Mesh::FaceHandle fl; ///< Left face
    typename Mesh::FaceHandle fr; ///< Right face
    typename Mesh::VertexHandle vl; ///< Left vertex
    typename Mesh::VertexHandle vr; ///< Right vertex
    //@{
    /** Outer remaining halfedge of diamond spanned by \c v0, \c v1,
     *  \c vl, and \c vr
     */
    typename Mesh::HalfedgeHandle vlv1, v0vl, vrv0, v1vr;
    //@}
};

//-----------------------------------------------------------------------------

/**
*   Local configuration of halfedge collapse to be stored in CollapseInfoT:
*
*       vl
*        *
*       / \
*      /   \
*     / fl  \
* v0 *------>* v1
*     \ fr  /
*      \   /
*       \ /
*        *
*        vr
*
*
* @param _mesh Reference to mesh
* @param  _heh The halfedge (v0 -> v1) defining the collapse
*/
template<class Mesh>
inline CollapseInfoT<Mesh>::CollapseInfoT(Mesh& _mesh,
    typename Mesh::HalfedgeHandle _heh) :
    mesh(_mesh), v0v1(_heh), v1v0(_mesh.opposite_halfedge_handle(v0v1)), v0(
        _mesh.to_vertex_handle(v1v0)), v1(_mesh.to_vertex_handle(v0v1)), p0(
        _mesh.point(v0)), p1(_mesh.point(v1)), fl(_mesh.face_handle(v0v1)), fr(
        _mesh.face_handle(v1v0))

{
  // get vl
  if (fl.is_valid()) {
    vlv1 = mesh.next_halfedge_handle(v0v1);
    v0vl = mesh.next_halfedge_handle(vlv1);
    vl = mesh.to_vertex_handle(vlv1);
    vlv1 = mesh.opposite_halfedge_handle(vlv1);
    v0vl = mesh.opposite_halfedge_handle(v0vl);
  }

  // get vr
  if (fr.is_valid()) {
    vrv0 = mesh.next_halfedge_handle(v1v0);
    v1vr = mesh.next_halfedge_handle(vrv0);
    vr = mesh.to_vertex_handle(vrv0);
    vrv0 = mesh.opposite_halfedge_handle(vrv0);
    v1vr = mesh.opposite_halfedge_handle(v1vr);
  }
}

//=============================================================================
}// END_NS_DECIMATER
} // END_NS_OPENMESH
//=============================================================================
#endif // OPENMESH_DECIMATER_COLLAPSEINFOT_HH defined
//=============================================================================

