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


/** \file ModAspectRatioT_impl.hh
 */

//=============================================================================
//
//  CLASS ModAspectRatioT - IMPLEMENTATION
//
//=============================================================================
#define OPENMESH_DECIMATER_MODASPECTRATIOT_C

//== INCLUDES =================================================================

#include "ModAspectRatioT.hh"

//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace Decimater {

//== IMPLEMENTATION ==========================================================

template<class MeshT>
typename ModAspectRatioT<MeshT>::Scalar ModAspectRatioT<MeshT>::aspectRatio(
    const Point& _v0, const Point& _v1, const Point& _v2) {
  Point d0 = _v0 - _v1;
  Point d1 = _v1 - _v2;

  // finds the max squared edge length
  Scalar l2, maxl2 = d0.sqrnorm();
  if ((l2 = d1.sqrnorm()) > maxl2)
    maxl2 = l2;
  // keep searching for the max squared edge length
  d1 = _v2 - _v0;
  if ((l2 = d1.sqrnorm()) > maxl2)
    maxl2 = l2;

  // squared area of the parallelogram spanned by d0 and d1
  Scalar a2 = (d0 % d1).sqrnorm();

  // the area of the triangle would be
  // sqrt(a2)/2 or length * height / 2
  // aspect ratio = length / height
  //              = length * length / (2*area)
  //              = length * length / sqrt(a2)

  // returns the length of the longest edge
  //         divided by its corresponding height
  return sqrt((maxl2 * maxl2) / a2);
}

//-----------------------------------------------------------------------------

template<class MeshT>
void ModAspectRatioT<MeshT>::initialize() {
  typename Mesh::FaceIter f_it, f_end(mesh_.faces_end());
  typename Mesh::FVIter fv_it;

  for (f_it = mesh_.faces_begin(); f_it != f_end; ++f_it) {
    fv_it = mesh_.fv_iter(*f_it);
    typename Mesh::Point& p0 = mesh_.point(*fv_it);
    typename Mesh::Point& p1 = mesh_.point(*(++fv_it));
    typename Mesh::Point& p2 = mesh_.point(*(++fv_it));

    mesh_.property(aspect_, *f_it) = static_cast<typename Mesh::Scalar>(1.0) / aspectRatio(p0, p1, p2);
  }
}

//-----------------------------------------------------------------------------

template<class MeshT>
void ModAspectRatioT<MeshT>::preprocess_collapse(const CollapseInfo& _ci) {
  typename Mesh::FaceHandle fh;
  typename Mesh::FVIter fv_it;

  for (typename Mesh::VFIter vf_it = mesh_.vf_iter(_ci.v0); vf_it.is_valid(); ++vf_it) {
    fh = *vf_it;
    if (fh != _ci.fl && fh != _ci.fr) {
      fv_it = mesh_.fv_iter(fh);
      typename Mesh::Point& p0 = mesh_.point(*fv_it);
      typename Mesh::Point& p1 = mesh_.point(*(++fv_it));
      typename Mesh::Point& p2 = mesh_.point(*(++fv_it));

      mesh_.property(aspect_, fh) = static_cast<typename Mesh::Scalar>(1.0) / aspectRatio(p0, p1, p2);
    }
  }
}

//-----------------------------------------------------------------------------

template<class MeshT>
float ModAspectRatioT<MeshT>::collapse_priority(const CollapseInfo& _ci) {
  typename Mesh::VertexHandle v2, v3;
  typename Mesh::FaceHandle fh;
  const typename Mesh::Point *p1(&_ci.p1), *p2, *p3;
  typename Mesh::Scalar r0, r1, r0_min(1.0), r1_min(1.0);
  typename Mesh::ConstVertexOHalfedgeIter voh_it(mesh_, _ci.v0);

  v3 = mesh_.to_vertex_handle(*voh_it);
  p3 = &mesh_.point(v3);

  while (voh_it.is_valid()) {
    v2 = v3;
    p2 = p3;

    ++voh_it;
    v3 = mesh_.to_vertex_handle(*voh_it);
    p3 = &mesh_.point(v3);

    fh = mesh_.face_handle(*voh_it);

    // if not boundary
    if (fh.is_valid()) {
      // aspect before
      if ((r0 = mesh_.property(aspect_, fh)) < r0_min)
        r0_min = r0;

      // aspect after
      if (!(v2 == _ci.v1 || v3 == _ci.v1))
        if ((r1 = static_cast<typename Mesh::Scalar>(1.0) / aspectRatio(*p1, *p2, *p3)) < r1_min)
          r1_min = r1;
    }
  }

  if (Base::is_binary()) {
    return
        ((r1_min > r0_min) || (r1_min > min_aspect_)) ? float(Base::LEGAL_COLLAPSE) :
            float(Base::ILLEGAL_COLLAPSE);

  } else {
    if (r1_min > r0_min)
      return 1.f - float(r1_min);
    else
      return
          (r1_min > min_aspect_) ? 2.f - float(r1_min) : float(Base::ILLEGAL_COLLAPSE);
  }
}

//-----------------------------------------------------------------------------

template<class MeshT>
void ModAspectRatioT<MeshT>::set_error_tolerance_factor(double _factor) {
  if (_factor >= 0.0 && _factor <= 1.0) {
    // the smaller the factor, the larger min_aspect_ gets
    // thus creating a stricter constraint
    // division by (2.0 - error_tolerance_factor_) is for normalization
    float min_aspect = min_aspect_ * (2.f - float(_factor)) / (2.f - float(this->error_tolerance_factor_));
    set_aspect_ratio(1.f/min_aspect);
    this->error_tolerance_factor_ = _factor;
  }
}

//=============================================================================
}
}
//=============================================================================
