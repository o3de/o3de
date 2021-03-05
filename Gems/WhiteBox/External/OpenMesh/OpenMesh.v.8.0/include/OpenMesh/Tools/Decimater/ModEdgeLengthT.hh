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


/** \file ModEdgeLengthT.hh
 */

//=============================================================================
//
//  CLASS ModEdgeLengthT
//
//=============================================================================
#ifndef OPENMESH_DECIMATER_MODEDGELENGTHT_HH
#define OPENMESH_DECIMATER_MODEDGELENGTHT_HH

//== INCLUDES =================================================================

#include <OpenMesh/Tools/Decimater/ModBaseT.hh>
#include <cfloat>

//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace Decimater {

//== CLASS DEFINITION =========================================================

/** \brief Use edge length to control decimation
 *
 * This module computes the edge length.
 *
 * In binary and continuous mode, the collapse is legal if:
 *  - The length after the collapse is lower than the given tolerance
 *
 */
template<class MeshT>
class ModEdgeLengthT: public ModBaseT<MeshT> {
  public:

    DECIMATING_MODULE( ModEdgeLengthT, MeshT, EdgeLength )
    ;

    /// Constructor
    ModEdgeLengthT(MeshT& _mesh, float _edge_length = FLT_MAX,
        bool _is_binary = true);

    /// get edge_length
    float edge_length() const {
      return edge_length_;
    }

    /// set edge_length
    void set_edge_length(typename Mesh::Scalar _f) {
      edge_length_ = _f;
      sqr_edge_length_ = _f * _f;
    }

    /** Compute priority:
     Binary mode: Don't collapse edges longer then edge_length_
     Cont. mode:  Collapse smallest edge first, but
     don't collapse edges longer as edge_length_
     */
    float collapse_priority(const CollapseInfo& _ci);

    /// set the percentage of edge length
    void set_error_tolerance_factor(double _factor);

  private:

    Mesh& mesh_;
    typename Mesh::Scalar edge_length_, sqr_edge_length_;
};

//=============================================================================
}// END_NS_DECIMATER
} // END_NS_OPENMESH
//=============================================================================
#if defined(OM_INCLUDE_TEMPLATES) && !defined(OPENMESH_DECIMATER_MODEDGELENGTHT_C)
#define MODEDGELENGTHT_TEMPLATES
#include "ModEdgeLengthT_impl.hh"
#endif
//=============================================================================
#endif // OPENMESH_DECIMATER_MODEDGELENGTHT_HH defined
//=============================================================================

