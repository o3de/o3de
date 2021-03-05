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



/** \file Uniform/Composite/CompositeT_impl.hh

 */

//=============================================================================
//
//  CLASS CompositeT - IMPLEMENTATION
//
//=============================================================================

#ifndef OPENMESH_SUBDIVIDER_UNIFORM_COMPOSITE_CC
#define OPENMESH_SUBDIVIDER_UNIFORM_COMPOSITE_CC


//== INCLUDES =================================================================


#include <vector>
#include <OpenMesh/Tools/Subdivider/Uniform/Composite/CompositeT.hh>


//== NAMESPACE ================================================================

namespace OpenMesh   { // BEGIN_NS_OPENMESH
namespace Subdivider { // BEGIN_NS_DECIMATER
namespace Uniform    { // BEGIN_NS_UNIFORM


//== IMPLEMENTATION ==========================================================


template <typename MeshType, typename RealType>
bool CompositeT<MeshType,RealType>::prepare( MeshType& _m )
{
  // store mesh for later usage in subdivide(), cleanup() and all rules.
  p_mesh_ = &_m;

  typename MeshType::VertexIter v_it(_m.vertices_begin());

  for (; v_it != _m.vertices_end(); ++v_it)
    _m.data(*v_it).set_position(_m.point(*v_it));

  return true;
}



template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::Tvv3()
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::VertexHandle vh;
  typename MeshType::FaceIter     f_it;
  typename MeshType::EdgeIter     e_it;
  typename MeshType::VertexIter   v_it;
  typename MeshType::Point        zero_point(0.0, 0.0, 0.0);
  size_t                          n_edges, n_faces, n_vertices, j;

  // Store number of original edges
  n_faces    = mesh_.n_faces();
  n_edges    = mesh_.n_edges();
  n_vertices = mesh_.n_vertices();

  // reserve enough memory for iterator
  mesh_.reserve(n_vertices + n_faces, n_edges + 3 * n_faces, 3 * n_faces);

  // set new positions for vertices
  v_it = mesh_.vertices_begin();
  for (j = 0; j < n_vertices; ++j) {
    mesh_.data(*v_it).set_position(mesh_.data(*v_it).position() * static_cast<typename MeshType::Point::value_type>(3.0) );
    ++v_it;
  }

  // Split each face
  f_it = mesh_.faces_begin();
  for (j = 0; j < n_faces; ++j) {

    vh = mesh_.add_vertex(zero_point);

    mesh_.data(vh).set_position(zero_point);

    mesh_.split(*f_it, vh);

    ++f_it;
  }

  // Flip each old edge
  std::vector<typename MeshType::EdgeHandle> edge_vector;
  edge_vector.clear();

  e_it = mesh_.edges_begin();
  for (j = 0; j < n_edges; ++j) {
    if (mesh_.is_flip_ok(*e_it)) {
      mesh_.flip(*e_it);
    } else {
      edge_vector.push_back(*e_it);
    }
    ++e_it;
  }

  // split all boundary edges
  while (!edge_vector.empty()) {
    vh = mesh_.add_vertex(zero_point);
    mesh_.data(vh).set_position(zero_point);
    mesh_.split(edge_vector.back(), vh);
    edge_vector.pop_back();
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::Tvv4()
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::VertexHandle     vh;
  typename MeshType::FaceIter         f_it;
  typename MeshType::EdgeIter         e_it;
  typename MeshType::VertexIter       v_it;
  typename MeshType::Point            zero_point(0.0, 0.0, 0.0);
  size_t                              n_edges, n_faces, n_vertices, j;

  // Store number of original edges
  n_faces    = mesh_.n_faces();
  n_edges    = mesh_.n_edges();
  n_vertices = mesh_.n_vertices();

  // reserve memory ahead for the succeeding operations
  mesh_.reserve(n_vertices + n_edges, 2 * n_edges + 3 * n_faces, 4 * n_faces);

  // set new positions for vertices
  v_it = mesh_.vertices_begin();
  for (j = 0; j < n_vertices; ++j) {
    mesh_.data(*v_it).set_position(mesh_.data(*v_it).position() * static_cast<typename MeshType::Point::value_type>(4.0) );
    ++v_it;
  }

  // Split each edge
  e_it = mesh_.edges_begin();
  for (j = 0; j < n_edges; ++j) {

    vh = split_edge(mesh_.halfedge_handle(*e_it, 0));
    mesh_.data(vh).set_position(zero_point);

    ++e_it;
  }

  // Corner Cutting of Each Face
  f_it = mesh_.faces_begin();
  for (j = 0; j < n_faces; ++j) {
    typename MeshType::HalfedgeHandle heh1(mesh_.halfedge_handle(*f_it));
    typename MeshType::HalfedgeHandle heh2(mesh_.next_halfedge_handle(mesh_.next_halfedge_handle(heh1)));
    typename MeshType::HalfedgeHandle heh3(mesh_.next_halfedge_handle(mesh_.next_halfedge_handle(heh2)));

    // Cutting off every corner of the 6_gon

    corner_cutting(heh1);
    corner_cutting(heh2);
    corner_cutting(heh3);

    ++f_it;
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::Tfv()
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::VertexHandle     vh;
  typename MeshType::FaceIter         f_it;
  typename MeshType::EdgeIter         e_it;
  typename MeshType::VertexIter       v_it;
  typename MeshType::VertexFaceIter   vf_it;
  typename MeshType::FaceFaceIter     ff_it;
  typename MeshType::Point            cog;
  const typename MeshType::Point      zero_point(0.0, 0.0, 0.0);
  size_t                              n_edges, n_faces, n_vertices, j, valence;

  // Store number of original edges
  n_faces = mesh_.n_faces();
  n_edges = mesh_.n_edges();
  n_vertices = mesh_.n_vertices();

  // reserve enough space for iterator
  mesh_.reserve(n_vertices + n_faces, n_edges + 3 * n_faces, 3 * n_faces);

  // set new positions for vertices
  v_it = mesh_.vertices_begin();
  for (j = 0; j < n_vertices; ++j) {
    valence = 0;
    cog = zero_point;
    for (vf_it = mesh_.vf_iter(*v_it); vf_it; ++vf_it) {
      ++valence;
      cog += vf_it->position();
    }
    cog /= valence;

    v_it->set_position(cog);
    ++v_it;
  }

  // Split each face, insert new vertex and calculate position
  f_it = mesh_.faces_begin();
  for (j = 0; j < n_faces; ++j) {

    vh = mesh_.add_vertex();

    valence = 0;
    cog = zero_point;
    for (ff_it = mesh_.ff_iter(*f_it); ff_it; ++ff_it) {
      ++valence;
      cog += ff_it->position();
    }
    cog /= valence;

    mesh_.split(*f_it, vh);

    for (vf_it = mesh_.vf_iter(vh); vf_it; ++vf_it) {
      vf_it->set_position(f_it->position());
    }

    mesh_.deref(vh).set_position(cog);

    mesh_.set_point(vh, cog);

    ++f_it;
  }

  // Flip each old edge
  e_it = mesh_.edges_begin();
  for (j = 0; j < n_edges; ++j) {
    if (mesh_.is_flip_ok(*e_it))
      mesh_.flip(*e_it);
    ++e_it;
  }
}



template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::VF()
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::Point          cog,zero_point(0.0, 0.0, 0.0);
  typename MeshType::FaceVertexIter fv_it;
  typename MeshType::FaceIter       f_it;

  for (f_it = mesh_.faces_begin(); f_it != mesh_.faces_end(); ++f_it) {

    unsigned int valence = 0;
    cog = zero_point;

    for (fv_it = mesh_.fv_iter(*f_it); fv_it.is_valid(); ++fv_it) {
      cog += mesh_.data(*fv_it).position();
      ++valence;
    }
    cog /= valence;
    mesh_.data(*f_it).set_position(cog);
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::VFa(Coeff& _coeff)
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  unsigned int                        valence[3], i;
  typename MeshType::Point                cog,zero_point(0.0, 0.0, 0.0);
  typename MeshType::Scalar               alpha;
  typename MeshType::FaceIter             f_it;
  typename MeshType::HalfedgeHandle       heh;
  typename MeshType::VertexHandle         vh[3];
  typename MeshType::VertexOHalfedgeIter  voh_it;
  typename MeshType::FaceVertexIter       fv_it;

  for (f_it = mesh_.faces_begin(); f_it != mesh_.faces_end(); ++f_it) {

    heh = mesh_.halfedge_handle(*f_it);
    for (i = 0; i <= 2; ++i) {

      valence[i] = 0;
      vh[i] = mesh_.to_vertex_handle(heh);

      for (voh_it = mesh_.voh_iter(vh[i]); voh_it; ++voh_it) {
        ++valence[i];
      }

      heh = mesh_.next_halfedge_handle(heh);
    }

    if (valence[0] <= valence[1])
      if (valence[0] <= valence[2])
        i = 0;
      else
        i = 2;
    else
      if (valence[1] <= valence[2])
        i = 1;
      else
        i = 2;

    alpha = _coeff(valence[i]);

    cog = zero_point;

    for (fv_it = mesh_.fv_iter(*f_it); fv_it.is_valid(); ++fv_it) {
      if (*fv_it == vh[i]) {
        cog += fv_it->position() * alpha;
      } else {
        cog += fv_it->position() * (1.0 - alpha) / 2.0;
      }
    }

    f_it->set_position(cog);
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::VFa(scalar_t _alpha)
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  unsigned int                        valence[3], i;
  typename MeshType::Point                cog,
  zero_point(0.0, 0.0, 0.0);
  typename MeshType::FaceIter             f_it;
  typename MeshType::HalfedgeHandle       heh;
  typename MeshType::VertexHandle         vh[3];
  typename MeshType::VertexOHalfedgeIter  voh_it;
  typename MeshType::FaceVertexIter       fv_it;

  for (f_it = mesh_.faces_begin(); f_it != mesh_.faces_end(); ++f_it) {

    heh = mesh_.halfedge_handle(*f_it);
    for (i = 0; i <= 2; ++i) {

      valence[i] = 0;
      vh[i] = mesh_.to_vertex_handle(heh);

      for (voh_it = mesh_.voh_iter(vh[i]); voh_it; ++voh_it) {
        ++valence[i];
      }

      heh = mesh_.next_halfedge_handle(heh);
    }

    if (valence[0] <= valence[1])
      if (valence[0] <= valence[2])
        i = 0;
      else
        i = 2;
    else
      if (valence[1] <= valence[2])
        i = 1;
      else
        i = 2;

    cog = zero_point;

    for (fv_it = mesh_.fv_iter(*f_it); fv_it.is_valid(); ++fv_it) {
      if (*fv_it == vh[i]) {
        cog += fv_it->position() * _alpha;
      } else {
        cog += fv_it->position() * (1.0 - _alpha) / 2.0;
      }
    }

    f_it->set_position(cog);
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::FF()
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::Point              cog,
                                    zero_point(0.0, 0.0, 0.0);
  typename MeshType::FaceFaceIter       ff_it;
  typename MeshType::FaceIter           f_it;
  std::vector<typename MeshType::Point> point_vector;

  point_vector.clear();

  for (f_it = mesh_.faces_begin(); f_it != mesh_.faces_end(); ++f_it)
  {
    unsigned int valence = 0;
    cog = zero_point;

    for (ff_it = mesh_.ff_iter(*f_it); ff_it.is_valid(); ++ff_it)
    {
      cog += mesh_.data(*ff_it).position();
      ++valence;
    }
    cog /= valence;
    point_vector.push_back(cog);
  }

  for (f_it = mesh_.faces_end(); f_it != mesh_.faces_begin(); )
  {
    --f_it;
    mesh_.data(*f_it).set_position(point_vector.back());
    point_vector.pop_back();
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::FFc(Coeff& _coeff)
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::Point           cog,
                                 zero_point(0.0, 0.0, 0.0);
  typename MeshType::FaceFaceIter    ff_it;
  typename MeshType::FaceIter        f_it;
  typename MeshType::Scalar          c;
  std::vector<typename MeshType::Point> point_vector;

  for (f_it = mesh_.faces_begin(); f_it != mesh_.faces_end(); ++f_it) {

    unsigned int valence = 0;
    cog = zero_point;

    for (ff_it = mesh_.ff_iter(*f_it); ff_it; ++ff_it) {
      cog += ff_it->position();
      ++valence;
    }
    cog /= valence;

    c = _coeff(valence);

    cog = cog * (1.0 - c) + f_it->position() * c;

    point_vector.push_back(cog);

  }
  for (f_it = mesh_.faces_end(); f_it != mesh_.faces_begin(); ) {

    --f_it;
    f_it->set_position(point_vector.back());
    point_vector.pop_back();

  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::FFc(scalar_t _c)
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::Point           cog, zero_point(0.0, 0.0, 0.0);
  typename MeshType::FaceFaceIter    ff_it;
  typename MeshType::FaceIter        f_it;
  std::vector<typename MeshType::Point> point_vector;

  for (f_it = mesh_.faces_begin(); f_it != mesh_.faces_end(); ++f_it) {

    unsigned int valence = 0;
    cog = zero_point;

    for (ff_it = mesh_.ff_iter(*f_it); ff_it; ++ff_it) {
      cog += ff_it->position();
      ++valence;
    }
    cog /= valence;

    cog = cog * (1.0 - _c) + f_it->position() * _c;

    point_vector.push_back(cog);

  }
  for (f_it = mesh_.faces_end(); f_it != mesh_.faces_begin(); ) {

    --f_it;
    f_it->set_position(point_vector.back());
    point_vector.pop_back();
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::FV()
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::Point          cog,
                                zero_point(0.0, 0.0, 0.0);
  typename MeshType::VertexFaceIter vf_it;
  typename MeshType::VertexIter     v_it;

  for (v_it = mesh_.vertices_begin(); v_it != mesh_.vertices_end(); ++v_it) {

    unsigned int  valence = 0;
    cog = zero_point;

    for (vf_it = mesh_.vf_iter(*v_it); vf_it; ++vf_it) {
      cog += vf_it->position();
      ++valence;
    }
    cog /= valence;
    v_it->set_position(cog);
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::FVc(Coeff& _coeff)
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;


  typename MeshType::Point                cog,
                                      zero_point(0.0, 0.0, 0.0);
  scalar_t               c;
  typename MeshType::VertexOHalfedgeIter  voh_it;
  typename MeshType::VertexIter           v_it;

  for (v_it = mesh_.vertices_begin(); v_it != mesh_.vertices_end(); ++v_it) {

    unsigned int valence = 0;
    cog = zero_point;

    for (voh_it = mesh_.voh_iter(*v_it); voh_it.is_valid(); ++voh_it) {
      ++valence;
    }

    c = static_cast<real_t>(_coeff(valence));

    for (voh_it = mesh_.voh_iter(*v_it); voh_it.is_valid(); ++voh_it) {

      if (mesh_.face_handle(*voh_it).is_valid()) {

        if (mesh_.face_handle(mesh_.opposite_halfedge_handle(mesh_.next_halfedge_handle(*voh_it))).is_valid()) {
          cog += mesh_.data(mesh_.face_handle(*voh_it)).position() * c;
          cog += mesh_.data(mesh_.face_handle(mesh_.opposite_halfedge_handle(mesh_.next_halfedge_handle(*voh_it)))).position() * (static_cast<typename MeshType::Point::value_type>(1.0) - c);
        } else {
          cog += mesh_.data(mesh_.face_handle(*voh_it)).position();
        }
      } else {
        --valence;
      }
    }

    if (valence > 0)
      cog /= valence;

    mesh_.data(*v_it).set_position(cog);
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::FVc(scalar_t _c)
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::Point               cog, zero_point(0.0, 0.0, 0.0);
  typename MeshType::VertexOHalfedgeIter voh_it;
  typename MeshType::VertexIter          v_it;

  for (v_it = mesh_.vertices_begin(); v_it != mesh_.vertices_end(); ++v_it) {

    unsigned int valence = 0;
    cog = zero_point;

    for (voh_it = mesh_.voh_iter(*v_it); voh_it; ++voh_it) {
      ++valence;
    }

    for (voh_it = mesh_.voh_iter(*v_it); voh_it; ++voh_it) {

      if (mesh_.face_handle(*voh_it).is_valid()) {

        if (mesh_.face_handle(mesh_.opposite_halfedge_handle(mesh_.next_halfedge_handle(*voh_it))).is_valid()) {
          cog += mesh_.deref(mesh_.face_handle(*voh_it)).position() * _c;
          cog += mesh_.deref(mesh_.face_handle(mesh_.opposite_halfedge_handle(mesh_.next_halfedge_handle(*voh_it)))).position() * (1.0 - _c);
        } else {
          cog += mesh_.deref(mesh_.face_handle(*voh_it)).position();
        }
      } else {
        --valence;
      }
    }

    if (valence > 0)
      cog /= valence;

    v_it->set_position(cog);
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::VdE()
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::EdgeIter             e_it;
  typename MeshType::Point                cog, zero_point(0.0, 0.0, 0.0);
  typename MeshType::HalfedgeHandle       heh1, heh2;

  for (e_it = mesh_.edges_begin(); e_it != mesh_.edges_end(); ++e_it) {

    cog = zero_point;
    unsigned int valence = 2;

    heh1 = mesh_.halfedge_handle(*e_it, 0);
    heh2 = mesh_.opposite_halfedge_handle(heh1);
    cog += mesh_.data(mesh_.to_vertex_handle(heh1)).position();
    cog += mesh_.data(mesh_.to_vertex_handle(heh2)).position();

    if (!mesh_.is_boundary(heh1)) {
      cog += mesh_.data(mesh_.to_vertex_handle(mesh_.next_halfedge_handle(heh1))).position();
      ++valence;
    }

    if (!mesh_.is_boundary(heh2)) {
      cog += mesh_.data(mesh_.to_vertex_handle(mesh_.next_halfedge_handle(heh2))).position();
      ++valence;
    }

    cog /= valence;

    mesh_.data(*e_it).set_position(cog);
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::VdEc(scalar_t _c)
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::EdgeIter           e_it;
  typename MeshType::Point              cog, zero_point(0.0, 0.0, 0.0);
  typename MeshType::HalfedgeHandle     heh;

  for (e_it = mesh_.edges_begin(); e_it != mesh_.edges_end(); ++e_it) {

    cog = zero_point;

    for (int i = 0; i <= 1; ++i) {

      heh = mesh_.halfedge_handle(*e_it, i);
      if (!mesh_.is_boundary(heh))
      {
        cog += mesh_.point(mesh_.to_vertex_handle(mesh_.next_halfedge_handle(heh))) * (0.5 - _c);
        cog += mesh_.data(mesh_.to_vertex_handle(heh)).position() * _c;
      }
      else
      {
        cog += mesh_.data(mesh_.to_vertex_handle(heh)).position();
      }
    }

    mesh_.data(*e_it).set_position(cog);
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::VdEg(scalar_t _gamma)
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::EdgeIter             e_it;
  typename MeshType::Point                cog, zero_point(0.0, 0.0, 0.0);
  typename MeshType::HalfedgeHandle       heh;
  typename MeshType::VertexOHalfedgeIter  voh_it;
  unsigned int                        valence[2], i;

  for (e_it = mesh_.edges_begin(); e_it != mesh_.edges_end(); ++e_it) {

    cog = zero_point;

    for (i = 0; i <= 1; ++i)
    {
      heh = mesh_.halfedge_handle(*e_it, i);
      valence[i] = 0;

      // look for lowest valence vertex
      for (voh_it = mesh_.voh_iter(mesh_.to_vertex_handle(heh)); voh_it; ++voh_it)
      {
        ++valence[i];
      }
    }

    if (valence[0] < valence[1])
      i = 0;
    else
      i = 1;

    heh = mesh_.halfedge_handle(*e_it, i);

    if (!mesh_.is_boundary(heh)) {
      cog += mesh_.point(mesh_.to_vertex_handle(mesh_.next_halfedge_handle(heh))) * (_gamma);
      cog += mesh_.data(mesh_.to_vertex_handle(heh)).position() * (1.0 - 3.0 * _gamma);
    } else {
      cog += mesh_.data(mesh_.to_vertex_handle(heh)).position() * (1.0 - 2.0 * _gamma);
    }


    heh = mesh_.halfedge_handle(*e_it, 1-i);

    if (!mesh_.is_boundary(heh))
    {
      cog += mesh_.point(mesh_.to_vertex_handle(mesh_.next_halfedge_handle(heh))) * (_gamma);
      cog += mesh_.data(mesh_.to_vertex_handle(heh)).position() * _gamma;
    }
    else
    {
      cog += mesh_.data(mesh_.to_vertex_handle(heh)).position() * 2.0 * _gamma;
    }

    mesh_.data(*e_it).set_position(cog);
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::VdEg(Coeff& _coeff)
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::EdgeIter             e_it;
  typename MeshType::Point                cog, zero_point(0.0, 0.0, 0.0);
  typename MeshType::HalfedgeHandle       heh;
  typename MeshType::VertexOHalfedgeIter  voh_it;
  unsigned int                        valence[2], i;
  scalar_t               gamma;

  for (e_it = mesh_.edges_begin(); e_it != mesh_.edges_end(); ++e_it) {

    cog = zero_point;

    for (i = 0; i <= 1; ++i) {

      heh = mesh_.halfedge_handle(*e_it, i);
      valence[i] = 0;

      // look for lowest valence vertex
      for (voh_it = mesh_.voh_iter(mesh_.to_vertex_handle(heh)); voh_it; ++voh_it)
      {
        ++valence[i];
      }
    }

    if (valence[0] < valence[1])
      i = 0;
    else
      i = 1;

    gamma = _coeff(valence[i]);

    heh = mesh_.halfedge_handle(*e_it, i);

    if (!mesh_.is_boundary(heh))
    {
      cog += mesh_.point(mesh_.to_vertex_handle(mesh_.next_halfedge_handle(heh))) * (gamma);
      cog += mesh_.data(mesh_.to_vertex_handle(heh)).position() * (1.0 - 3.0 * gamma);
    }
    else
    {
      cog += mesh_.data(mesh_.to_vertex_handle(heh)).position() * (1.0 - 2.0 * gamma);
    }


    heh = mesh_.halfedge_handle(*e_it, 1-i);

    if (!mesh_.is_boundary(heh)) {
      cog += mesh_.point(mesh_.to_vertex_handle(mesh_.next_halfedge_handle(heh))) * (gamma);
      cog += mesh_.data(mesh_.to_vertex_handle(heh)).position() * gamma;
    } else {
      cog += mesh_.data(mesh_.to_vertex_handle(heh)).position() * 2.0 * gamma;
    }

    mesh_.data(*e_it).set_position(cog);
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::EV()
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::VertexIter       v_it;
  typename MeshType::Point            cog, zero_point(0.0, 0.0, 0.0);
  typename MeshType::VertexEdgeIter   ve_it;

  for (v_it = mesh_.vertices_begin(); v_it != mesh_.vertices_end(); ++v_it)
  {
    unsigned int valence = 0;
    cog = zero_point;

    for (ve_it = mesh_.ve_iter(*v_it); ve_it; ++ve_it) {
      cog += mesh_.data(ve_it).position();
      ++valence;
    }

    cog /= valence;

    mesh_.data(*v_it).set_position(cog);
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::EVc(Coeff& _coeff)
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::VertexIter            v_it;
  typename MeshType::Point                 cog, zero_point(0.0, 0.0, 0.0);
  typename MeshType::VertexOHalfedgeIter   voh_it;
  scalar_t                c;

  for (v_it = mesh_.vertices_begin(); v_it != mesh_.vertices_end(); ++v_it)
  {
    unsigned int valence = 0;
    cog = zero_point;

    for (voh_it = mesh_.voh_iter(*v_it); voh_it.is_valid(); ++voh_it)
    {
      ++valence;
    }

    // Coefficients always work on double so we cast them to the correct scalar here
    c = static_cast<scalar_t>(_coeff(valence));

    for (voh_it = mesh_.voh_iter(*v_it); voh_it.is_valid(); ++voh_it) {
      cog += mesh_.data(mesh_.edge_handle(*voh_it)).position() * c;
      cog += mesh_.data(mesh_.edge_handle(mesh_.next_halfedge_handle(*voh_it))).position() * (1.0 - c);
    }

    cog /= valence;

    mesh_.data(*v_it).set_position(cog);
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::EVc(scalar_t _c)
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;
  typename MeshType::VertexIter           v_it;
  typename MeshType::Point                cog, zero_point(0.0, 0.0, 0.0);
  typename MeshType::VertexOHalfedgeIter  voh_it;

  for (v_it = mesh_.vertices_begin(); v_it != mesh_.vertices_end(); ++v_it) {
    unsigned int valence = 0;
    cog = zero_point;

    for (voh_it = mesh_.voh_iter(*v_it); voh_it; ++voh_it) {
      ++valence;
    }

    for (voh_it = mesh_.voh_iter(*v_it); voh_it; ++voh_it) {
      cog += mesh_.data(mesh_.edge_handle(*voh_it)).position() * _c;
      cog += mesh_.data(mesh_.edge_handle(mesh_.next_halfedge_handle(*voh_it))).position() * (1.0 - _c);
    }

    cog /= valence;

    mesh_.data(*v_it).set_position(cog);
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::EF()
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::FaceIter         f_it;
  typename MeshType::FaceEdgeIter     fe_it;
  typename MeshType::Point            cog, zero_point(0.0, 0.0, 0.0);

  for (f_it = mesh_.faces_begin(); f_it != mesh_.faces_end(); ++f_it) {
    unsigned int valence = 0;
    cog = zero_point;

    for (fe_it = mesh_.fe_iter(*f_it); fe_it; ++fe_it) {
      ++valence;
      cog += mesh_.data(fe_it).position();
    }

    cog /= valence;
    mesh_.data(*f_it).set_position(cog);
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::FE()
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::EdgeIter       e_it;
  typename MeshType::Point          cog, zero_point(0.0, 0.0, 0.0);

  for (e_it = mesh_.edges_begin(); e_it != mesh_.edges_end(); ++e_it) {
    unsigned int valence = 0;
    cog = zero_point;

    if (mesh_.face_handle(mesh_.halfedge_handle(*e_it, 0)).is_valid()) {
      cog += mesh_.data(mesh_.face_handle(mesh_.halfedge_handle(*e_it, 0))).position();
      ++valence;
    }

    if (mesh_.face_handle(mesh_.halfedge_handle(*e_it, 1)).is_valid()) {
      cog += mesh_.data(mesh_.face_handle(mesh_.halfedge_handle(*e_it, 1))).position();
      ++valence;
    }

    cog /= valence;
    mesh_.data(*e_it).set_position(cog);
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::VE()
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::EdgeIter      e_it;
  typename MeshType::Point         cog;

  for (e_it = mesh_.edges_begin(); e_it != mesh_.edges_end(); ++e_it)
  {
    cog = mesh_.data(mesh_.to_vertex_handle(mesh_.halfedge_handle(*e_it, 0))).position();
    cog += mesh_.data(mesh_.to_vertex_handle(mesh_.halfedge_handle(*e_it, 1))).position();
    cog /= 2.0;
    mesh_.data(*e_it).set_position(cog);
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::VV()
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::Point                cog, zero_point(0.0, 0.0, 0.0);
  typename MeshType::VertexVertexIter     vv_it;
  typename MeshType::VertexIter           v_it;
  std::vector<typename MeshType::Point>   point_vector;

  point_vector.clear();

  for (v_it = mesh_.vertices_begin(); v_it != mesh_.vertices_end(); ++v_it) {

    unsigned int valence = 0;
    cog = zero_point;

    for (vv_it = mesh_.vv_iter(*v_it); vv_it; ++vv_it) {
      cog += vv_it->position();
      ++valence;
    }
    cog /= valence;
    point_vector.push_back(cog);
  }

  for (v_it = mesh_.vertices_end(); v_it != mesh_.vertices_begin(); )
  {
    --v_it;
    mesh_.data(*v_it).set_position(point_vector.back());
    point_vector.pop_back();
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::VVc(Coeff& _coeff)
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::Point              cog,
                                    zero_point(0.0, 0.0, 0.0);
  typename MeshType::VertexVertexIter   vv_it;
  typename MeshType::VertexIter         v_it;
  scalar_t             c;
  std::vector<typename MeshType::Point> point_vector;

  for (v_it = mesh_.vertices_begin(); v_it != mesh_.vertices_end(); ++v_it)
  {
    unsigned int valence = 0;
    cog = zero_point;

    for (vv_it = mesh_.vv_iter(*v_it); vv_it; ++vv_it)
    {
      cog += vv_it->position();
      ++valence;
    }
    cog /= valence;
    c = _coeff(valence);
    cog = cog * (1 - c) + mesh_.data(*v_it).position() * c;
    point_vector.push_back(cog);
  }
  for (v_it = mesh_.vertices_end(); v_it != mesh_.vertices_begin(); )
  {
    --v_it;
    mesh_.data(*v_it).set_position(point_vector.back());
    point_vector.pop_back();
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::VVc(scalar_t _c)
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::Point              cog, zero_point(0.0, 0.0, 0.0);
  typename MeshType::VertexVertexIter   vv_it;
  typename MeshType::VertexIter         v_it;
  std::vector<typename MeshType::Point> point_vector;

  for (v_it = mesh_.vertices_begin(); v_it != mesh_.vertices_end(); ++v_it) {

    unsigned int valence = 0;
    cog = zero_point;

    for (vv_it = mesh_.vv_iter(*v_it); vv_it; ++vv_it) {
      cog += mesh_.data(vv_it).position();
      ++valence;
    }
    cog /= valence;

    cog = cog * (1.0 - _c) + v_it->position() * _c;

    point_vector.push_back(cog);

  }
  for (v_it = mesh_.vertices_end(); v_it != mesh_.vertices_begin(); ) {

    --v_it;
    mesh_.data(*v_it).set_position(point_vector.back());
    point_vector.pop_back();

  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::EdE()
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::Point              cog, zero_point(0.0, 0.0, 0.0);
  typename MeshType::EdgeIter           e_it;
  typename MeshType::HalfedgeHandle     heh;
  std::vector<typename MeshType::Point> point_vector;

  point_vector.clear();

  for (e_it = mesh_.edges_begin(); e_it != mesh_.edges_end(); ++e_it) {

    unsigned int valence = 0;
    cog = zero_point;

    for (int i = 0; i <= 1; ++i) {
      heh = mesh_.halfedge_handle(*e_it, i);
      if (mesh_.face_handle(heh).is_valid())
      {
        cog += mesh_.data(mesh_.edge_handle(mesh_.next_halfedge_handle(heh))).position();
        cog += mesh_.data(mesh_.edge_handle(mesh_.next_halfedge_handle(mesh_.next_halfedge_handle(heh)))).position();
        ++valence;
        ++valence;
      }
    }

    cog /= valence;
    point_vector.push_back(cog);
  }

  for (e_it = mesh_.edges_end(); e_it != mesh_.edges_begin(); )
  {
    --e_it;
    mesh_.data(*e_it).set_position(point_vector.back());
    point_vector.pop_back();
  }
}


template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::EdEc(scalar_t _c)
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  typename MeshType::Point                 cog, zero_point(0.0, 0.0, 0.0);
  typename MeshType::EdgeIter              e_it;
  typename MeshType::HalfedgeHandle        heh;
  std::vector<typename MeshType::Point>    point_vector;

  point_vector.clear();

  for (e_it = mesh_.edges_begin(); e_it != mesh_.edges_end(); ++e_it)
  {
    unsigned int valence = 0;
    cog = zero_point;

    for (int i = 0; i <= 1; ++i) {
      heh = mesh_.halfedge_handle(*e_it, i);
      if (mesh_.face_handle(heh).is_valid())
      {
        cog += mesh_.data(mesh_.edge_handle(mesh_.next_halfedge_handle(heh))).position() * (1.0 - _c);
        cog += mesh_.data(mesh_.edge_handle(mesh_.next_halfedge_handle(mesh_.next_halfedge_handle(heh)))).position() * (1.0 - _c);
        ++valence;
        ++valence;
      }
    }

    cog /= valence;
    cog += mesh_.data(e_it).position() * _c;
    point_vector.push_back(cog);
  }

  for (e_it = mesh_.edges_end(); e_it != mesh_.edges_begin(); ) {

    --e_it;
    mesh_.data(*e_it).set_position(point_vector.back());
    point_vector.pop_back();
  }
}


/// Corner Cutting
template<typename MeshType, typename RealType>
void CompositeT<MeshType,RealType>::corner_cutting(HalfedgeHandle _heh)
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  // Define Halfedge Handles
  typename MeshType::HalfedgeHandle heh5(_heh);
  typename MeshType::HalfedgeHandle heh6(mesh_.next_halfedge_handle(_heh));

  // Cycle around the polygon to find correct Halfedge
  for (; mesh_.next_halfedge_handle(mesh_.next_halfedge_handle(heh5)) != _heh;
       heh5 = mesh_.next_halfedge_handle(heh5)) {};

  typename MeshType::HalfedgeHandle heh2(mesh_.next_halfedge_handle(heh5));
  typename MeshType::HalfedgeHandle
    heh3(mesh_.new_edge(mesh_.to_vertex_handle(_heh),
      mesh_.to_vertex_handle(heh5)));
  typename MeshType::HalfedgeHandle heh4(mesh_.opposite_halfedge_handle(heh3));

  // Old and new Face
  typename MeshType::FaceHandle     fh_old(mesh_.face_handle(heh6));
  typename MeshType::FaceHandle     fh_new(mesh_.new_face());

  // Init new face
  mesh_.data(fh_new).set_position(mesh_.data(fh_old).position());

  // Re-Set Handles around old Face
  mesh_.set_next_halfedge_handle(heh4, heh6);
  mesh_.set_next_halfedge_handle(heh5, heh4);

  mesh_.set_face_handle(heh4, fh_old);
  mesh_.set_face_handle(heh5, fh_old);
  mesh_.set_face_handle(heh6, fh_old);
  mesh_.set_halfedge_handle(fh_old, heh4);

  // Re-Set Handles around new Face
  mesh_.set_next_halfedge_handle(_heh, heh3);
  mesh_.set_next_halfedge_handle(heh3, heh2);

  mesh_.set_face_handle(_heh, fh_new);
  mesh_.set_face_handle(heh2, fh_new);
  mesh_.set_face_handle(heh3, fh_new);

  mesh_.set_halfedge_handle(fh_new, _heh);
}


/// Split Edge
template<typename MeshType, typename RealType>
typename MeshType::VertexHandle
CompositeT<MeshType,RealType>::split_edge(HalfedgeHandle _heh)
{
  assert(p_mesh_); MeshType& mesh_ = *p_mesh_;

  HalfedgeHandle heh1;
  HalfedgeHandle heh2;
  HalfedgeHandle heh3;
  HalfedgeHandle temp_heh;

  VertexHandle
    vh,
    vh1(mesh_.to_vertex_handle(_heh)),
    vh2(mesh_.from_vertex_handle(_heh));

  // Calculate and Insert Midpoint of Edge
  vh = mesh_.add_vertex((mesh_.point(vh2) + mesh_.point(vh1)) / static_cast<typename MeshType::Point::value_type>(2.0) );
  // Re-Set Handles
  heh2 = mesh_.opposite_halfedge_handle(_heh);

  if (!mesh_.is_boundary(mesh_.edge_handle(_heh))) {

    for (temp_heh = mesh_.next_halfedge_handle(heh2);
   mesh_.next_halfedge_handle(temp_heh) != heh2;
   temp_heh = mesh_.next_halfedge_handle(temp_heh) ) {}
  } else {
    for (temp_heh = _heh;
   mesh_.next_halfedge_handle(temp_heh) != heh2;
   temp_heh = mesh_.opposite_halfedge_handle(mesh_.next_halfedge_handle(temp_heh))) {}
  }

  heh1 = mesh_.new_edge(vh, vh1);
  heh3 = mesh_.opposite_halfedge_handle(heh1);
  mesh_.set_vertex_handle(_heh, vh);
  mesh_.set_next_halfedge_handle(temp_heh, heh3);
  mesh_.set_next_halfedge_handle(heh1, mesh_.next_halfedge_handle(_heh));
  mesh_.set_next_halfedge_handle(_heh, heh1);
  mesh_.set_next_halfedge_handle(heh3, heh2);
  if (mesh_.face_handle(heh2).is_valid()) {
    mesh_.set_face_handle(heh3, mesh_.face_handle(heh2));
    mesh_.set_halfedge_handle(mesh_.face_handle(heh3), heh3);
  }
  mesh_.set_face_handle(heh1, mesh_.face_handle(_heh));
  mesh_.set_halfedge_handle(vh, heh1);
  mesh_.set_halfedge_handle(mesh_.face_handle(_heh), _heh);
  mesh_.set_halfedge_handle(vh1, heh3);

  return vh;
}


//=============================================================================
} // END_NS_UNIFORM
} // END_NS_SUBDIVIDER
} // END_NS_OPENMESH
//=============================================================================
#endif // OPENMESH_SUBDIVIDER_UNIFORM_COMPOSITE_CC defined
//=============================================================================

