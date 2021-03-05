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


/** \file ModEdgeLengthT_impl.hh
 */

//=============================================================================
//
//  CLASS ModEdgeLengthT - IMPLEMENTATION
//
//=============================================================================
#define OPENMESH_DECIMATER_MODEDGELENGTHT_C

//== INCLUDES =================================================================

#include "ModEdgeLengthT.hh"

//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace Decimater {

//== IMPLEMENTATION ==========================================================

template<class MeshT>
ModEdgeLengthT<MeshT>::ModEdgeLengthT(MeshT &_mesh, float _edge_length,
    bool _is_binary) :
    Base(_mesh, _is_binary), mesh_(Base::mesh()) {
  set_edge_length(_edge_length);
}

//-----------------------------------------------------------------------------

template<class MeshT>
float ModEdgeLengthT<MeshT>::collapse_priority(const CollapseInfo& _ci) {
  typename Mesh::Scalar sqr_length = (_ci.p0 - _ci.p1).sqrnorm();

  return ( (sqr_length <= sqr_edge_length_) ? sqr_length : float(Base::ILLEGAL_COLLAPSE));
}

//-----------------------------------------------------------------------------

template<class MeshT>
void ModEdgeLengthT<MeshT>::set_error_tolerance_factor(double _factor) {
  if (_factor >= 0.0 && _factor <= 1.0) {
    // the smaller the factor, the smaller edge_length_ gets
    // thus creating a stricter constraint
    // division by error_tolerance_factor_ is for normalization
    typename Mesh::Scalar edge_length = edge_length_ * static_cast<typename Mesh::Scalar>(_factor / this->error_tolerance_factor_);
    set_edge_length(edge_length);
    this->error_tolerance_factor_ = _factor;
  }
}

//=============================================================================
}
}
//=============================================================================
