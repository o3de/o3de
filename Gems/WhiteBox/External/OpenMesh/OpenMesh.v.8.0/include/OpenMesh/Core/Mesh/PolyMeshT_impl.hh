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




//=============================================================================
//
//  CLASS PolyMeshT - IMPLEMENTATION
//
//=============================================================================


#define OPENMESH_POLYMESH_C


//== INCLUDES =================================================================

#include <OpenMesh/Core/Mesh/PolyMeshT.hh>
#include <OpenMesh/Core/Geometry/LoopSchemeMaskT.hh>
#include <OpenMesh/Core/Utils/GenProg.hh>
#include <OpenMesh/Core/Utils/vector_cast.hh>
#include <OpenMesh/Core/Utils/vector_traits.hh>
#include <OpenMesh/Core/System/omstream.hh>
#include <vector>


//== NAMESPACES ===============================================================


namespace OpenMesh {

//== IMPLEMENTATION ==========================================================

template <class Kernel>
uint PolyMeshT<Kernel>::find_feature_edges(Scalar _angle_tresh)
{
  assert(Kernel::has_edge_status());//this function needs edge status property
  uint n_feature_edges = 0;
  for (EdgeIter e_it = Kernel::edges_begin(); e_it != Kernel::edges_end(); ++e_it)
  {
    if (fabs(calc_dihedral_angle(*e_it)) > _angle_tresh)
    {//note: could be optimized by comparing cos(dih_angle) vs. cos(_angle_tresh)
      this->status(*e_it).set_feature(true);
      n_feature_edges++;
    }
    else
    {
      this->status(*e_it).set_feature(false);
    }
  }
  return n_feature_edges;
}

//-----------------------------------------------------------------------------

template <class Kernel>
typename PolyMeshT<Kernel>::Normal
PolyMeshT<Kernel>::calc_face_normal(FaceHandle _fh) const
{
  return calc_face_normal_impl(_fh, typename GenProg::IF<
    vector_traits<PolyMeshT<Kernel>::Point>::size_ == 3,
    PointIs3DTag,
    PointIsNot3DTag
  >::Result());
}

// lumberyard-change begin
template<typename Point, typename Normal>
void newell_norm(
    Normal& n, const Point& a, const Point& b)
{
    n[0] += static_cast<typename vector_traits<Normal>::value_type>(a[1] * b[2]);
    n[1] += static_cast<typename vector_traits<Normal>::value_type>(a[2] * b[0]);
    n[2] += static_cast<typename vector_traits<Normal>::value_type>(a[0] * b[1]);
}
// lumberyard-change end

template <class Kernel>
typename PolyMeshT<Kernel>::Normal
PolyMeshT<Kernel>::calc_face_normal_impl(FaceHandle _fh, PointIs3DTag) const
{
  assert(this->halfedge_handle(_fh).is_valid());
  ConstFaceVertexIter fv_it(this->cfv_iter(_fh));

  // Safeguard for 1-gons
  if (!(++fv_it).is_valid()) return Normal(0, 0, 0);

  // Safeguard for 2-gons
  if (!(++fv_it).is_valid()) return Normal(0, 0, 0);

  // use Newell's Method to compute the surface normal
  Normal n(0,0,0);
  for(fv_it = this->cfv_iter(_fh); fv_it.is_valid(); ++fv_it)
  {
    // next vertex
    ConstFaceVertexIter fv_itn = fv_it;
    ++fv_itn;

    if (!fv_itn.is_valid())
      fv_itn = this->cfv_iter(_fh);

    // http://www.opengl.org/wiki/Calculating_a_Surface_Normal
    const Point a = this->point(*fv_it) - this->point(*fv_itn);
    const Point b = this->point(*fv_it) + this->point(*fv_itn);

    // Due to traits, the value types of normals and points can be different.
    // Therefore we cast them here.
    newell_norm(n, a, b); // lumberyard-change
  }

  const typename vector_traits<Normal>::value_type length = norm(n);

  // The expression ((n *= (1.0/norm)),n) is used because the OpenSG
  // vector class does not return self after component-wise
  // self-multiplication with a scalar!!!
  return (length != typename vector_traits<Normal>::value_type(0))
          ? ((n *= (typename vector_traits<Normal>::value_type(1)/length)), n)
          : Normal(0, 0, 0);
}

template <class Kernel>
typename PolyMeshT<Kernel>::Normal
PolyMeshT<Kernel>::calc_face_normal_impl(FaceHandle, PointIsNot3DTag) const
{
  // Dummy fallback implementation
  // Returns just an initialized all 0 normal
  // This function is only used if we don't hate a matching implementation
  // for normal computation with the current vector type defined in the mesh traits

  assert(false);

  Normal normal;
  vectorize(normal,0);
  return normal;
}

//-----------------------------------------------------------------------------

template <class Kernel>
typename PolyMeshT<Kernel>::Normal
PolyMeshT<Kernel>::
calc_face_normal(const Point& _p0,
     const Point& _p1,
     const Point& _p2) const
{
  return calc_face_normal_impl(_p0, _p1, _p2, typename GenProg::IF<
    vector_traits<PolyMeshT<Kernel>::Point>::size_ == 3,
    PointIs3DTag,
    PointIsNot3DTag
  >::Result());
}

template <class Kernel>
typename PolyMeshT<Kernel>::Normal
PolyMeshT<Kernel>::
calc_face_normal_impl(const Point& _p0,
     const Point& _p1,
     const Point& _p2,
     PointIs3DTag) const
{
#if 1
  // The OpenSG <Vector>::operator -= () does not support the type Point
  // as rhs. Therefore use vector_cast at this point!!!
  // Note! OpenSG distinguishes between Normal and Point!!!
  Normal p1p0(vector_cast<Normal>(_p0));  p1p0 -= vector_cast<Normal>(_p1);
  Normal p1p2(vector_cast<Normal>(_p2));  p1p2 -= vector_cast<Normal>(_p1);

  Normal n    = cross(p1p2, p1p0);
  typename vector_traits<Normal>::value_type length = norm(n);

  // The expression ((n *= (1.0/norm)),n) is used because the OpenSG
  // vector class does not return self after component-wise
  // self-multiplication with a scalar!!!
  return (length != typename vector_traits<Normal>::value_type(0))
          ? ((n *= (typename vector_traits<Normal>::value_type(1)/length)),n)
          : Normal(0,0,0);
#else
  Point p1p0 = _p0;  p1p0 -= _p1;
  Point p1p2 = _p2;  p1p2 -= _p1;

  Normal n = vector_cast<Normal>(cross(p1p2, p1p0));
  typename vector_traits<Normal>::value_type length = norm(n);

  return (length != 0.0) ? n *= (1.0/length) : Normal(0,0,0);
#endif
}

template <class Kernel>
typename PolyMeshT<Kernel>::Normal
PolyMeshT<Kernel>::calc_face_normal_impl(const Point&, const Point&, const Point&, PointIsNot3DTag) const
{

  // Dummy fallback implementation
  // Returns just an initialized all 0 normal
  // This function is only used if we don't hate a matching implementation
  // for normal computation with the current vector type defined in the mesh traits

  assert(false);

  Normal normal;
  vectorize(normal,0);
  return normal;
}

//-----------------------------------------------------------------------------

template <class Kernel>
typename PolyMeshT<Kernel>::Point
PolyMeshT<Kernel>::
calc_face_centroid(FaceHandle _fh) const
{
  Point _pt;
  vectorize(_pt, 0);
  Scalar valence = 0.0;
  for (ConstFaceVertexIter cfv_it = this->cfv_iter(_fh); cfv_it.is_valid(); ++cfv_it, valence += 1.0)
  {
    _pt += this->point(*cfv_it);
  }
  _pt /= valence;
  return _pt;
}
//-----------------------------------------------------------------------------


template <class Kernel>
void
PolyMeshT<Kernel>::
update_normals()
{
  // Face normals are required to compute the vertex and the halfedge normals
  if (Kernel::has_face_normals() ) {
    update_face_normals();

    if (Kernel::has_vertex_normals() ) update_vertex_normals();
    if (Kernel::has_halfedge_normals()) update_halfedge_normals();
  }
}


//-----------------------------------------------------------------------------


template <class Kernel>
void
PolyMeshT<Kernel>::
update_face_normals()
{
  FaceIter f_it(Kernel::faces_sbegin()), f_end(Kernel::faces_end());

  for (; f_it != f_end; ++f_it)
    this->set_normal(*f_it, calc_face_normal(*f_it));
}


//-----------------------------------------------------------------------------


template <class Kernel>
void
PolyMeshT<Kernel>::
update_halfedge_normals(const double _feature_angle)
{
  HalfedgeIter h_it(Kernel::halfedges_begin()), h_end(Kernel::halfedges_end());

  for (; h_it != h_end; ++h_it)
    this->set_normal(*h_it, calc_halfedge_normal(*h_it, _feature_angle));
}


//-----------------------------------------------------------------------------


template <class Kernel>
typename PolyMeshT<Kernel>::Normal
PolyMeshT<Kernel>::
calc_halfedge_normal(HalfedgeHandle _heh, const double _feature_angle) const
{
  if(Kernel::is_boundary(_heh))
    return Normal(0,0,0);
  else
  {
    std::vector<FaceHandle> fhs; fhs.reserve(10);

    HalfedgeHandle heh = _heh;

    // collect CW face-handles
    do
    {
      fhs.push_back(Kernel::face_handle(heh));

      heh = Kernel::next_halfedge_handle(heh);
      heh = Kernel::opposite_halfedge_handle(heh);
    }
    while(heh != _heh && !Kernel::is_boundary(heh) && !is_estimated_feature_edge(heh, _feature_angle));

    // collect CCW face-handles
    if(heh != _heh && !is_estimated_feature_edge(_heh, _feature_angle))
    {
      heh = Kernel::opposite_halfedge_handle(_heh);

      if ( !Kernel::is_boundary(heh) ) {
        do
        {

          fhs.push_back(Kernel::face_handle(heh));

          heh = Kernel::prev_halfedge_handle(heh);
          heh = Kernel::opposite_halfedge_handle(heh);
        }
        while(!Kernel::is_boundary(heh) && !is_estimated_feature_edge(heh, _feature_angle));
      }
    }

    Normal n(0,0,0);
    for(unsigned int i=0; i<fhs.size(); ++i)
      n += Kernel::normal(fhs[i]);

    return normalize(n);
  }
}


//-----------------------------------------------------------------------------


template <class Kernel>
bool
PolyMeshT<Kernel>::
is_estimated_feature_edge(HalfedgeHandle _heh, const double _feature_angle) const
{
  EdgeHandle eh = Kernel::edge_handle(_heh);

  if(Kernel::has_edge_status())
  {
    if(Kernel::status(eh).feature())
      return true;
  }

  if(Kernel::is_boundary(eh))
    return false;

  // compute angle between faces
  FaceHandle fh0 = Kernel::face_handle(_heh);
  FaceHandle fh1 = Kernel::face_handle(Kernel::opposite_halfedge_handle(_heh));

  Normal fn0 = Kernel::normal(fh0);
  Normal fn1 = Kernel::normal(fh1);

  // dihedral angle above angle threshold
  return ( dot(fn0,fn1) < cos(_feature_angle) );
}


//-----------------------------------------------------------------------------


template <class Kernel>
typename PolyMeshT<Kernel>::Normal
PolyMeshT<Kernel>::
calc_vertex_normal(VertexHandle _vh) const
{
  Normal n;
  calc_vertex_normal_fast(_vh,n);

  Scalar length = norm(n);
  if (length != 0.0) n *= (Scalar(1.0)/length);

  return n;
}

//-----------------------------------------------------------------------------
template <class Kernel>
void PolyMeshT<Kernel>::
calc_vertex_normal_fast(VertexHandle _vh, Normal& _n) const
{
  vectorize(_n, Scalar(0.0)); // lumberyard-fix
  for (ConstVertexFaceIter vf_it = this->cvf_iter(_vh); vf_it.is_valid(); ++vf_it)
    _n += this->normal(*vf_it);
}

//-----------------------------------------------------------------------------
template <class Kernel>
void PolyMeshT<Kernel>::
calc_vertex_normal_correct(VertexHandle _vh, Normal& _n) const
{
  vectorize(_n, Scalar(0.0)); // lumberyard-fix
  ConstVertexIHalfedgeIter cvih_it = this->cvih_iter(_vh);
  if (! cvih_it.is_valid() )
  {//don't crash on isolated vertices
    return;
  }
  Normal in_he_vec;
  calc_edge_vector(*cvih_it, in_he_vec);
  for ( ; cvih_it.is_valid(); ++cvih_it)
  {//calculates the sector normal defined by cvih_it and adds it to _n
    if (this->is_boundary(*cvih_it))
    {
      continue;
    }
    HalfedgeHandle out_heh(this->next_halfedge_handle(*cvih_it));
    Normal out_he_vec;
    calc_edge_vector(out_heh, out_he_vec);
    _n += cross(in_he_vec, out_he_vec);//sector area is taken into account
    in_he_vec = out_he_vec;
    in_he_vec *= -1;//change the orientation
  }
}

//-----------------------------------------------------------------------------
template <class Kernel>
void PolyMeshT<Kernel>::
calc_vertex_normal_loop(VertexHandle _vh, Normal& _n) const
{
  static const LoopSchemeMaskDouble& loop_scheme_mask__ =
                  LoopSchemeMaskDoubleSingleton::Instance();

  Normal t_v(0.0,0.0,0.0), t_w(0.0,0.0,0.0);
  unsigned int vh_val = this->valence(_vh);
  unsigned int i = 0;
  for (ConstVertexOHalfedgeIter cvoh_it = this->cvoh_iter(_vh); cvoh_it.is_valid(); ++cvoh_it, ++i)
  {
    VertexHandle r1_v( this->to_vertex_handle(*cvoh_it) );
    t_v += (typename vector_traits<Point>::value_type)(loop_scheme_mask__.tang0_weight(vh_val, i))*this->point(r1_v);
    t_w += (typename vector_traits<Point>::value_type)(loop_scheme_mask__.tang1_weight(vh_val, i))*this->point(r1_v);
  }
  _n = cross(t_w, t_v);//hack: should be cross(t_v, t_w), but then the normals are reversed?
}

//-----------------------------------------------------------------------------


template <class Kernel>
void
PolyMeshT<Kernel>::
update_vertex_normals()
{
  VertexIter  v_it(Kernel::vertices_begin()), v_end(Kernel::vertices_end());

  for (; v_it!=v_end; ++v_it)
    this->set_normal(*v_it, calc_vertex_normal(*v_it));
}

//=============================================================================
} // namespace OpenMesh
//=============================================================================
