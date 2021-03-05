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

/** \file ModQuadricT.hh

 */

//=============================================================================
//
//  CLASS ModQuadricT
//
//=============================================================================
#ifndef OPENMESH_TOOLS_MODINDEPENDENTSETST_HH
#define OPENMESH_TOOLS_MODINDEPENDENTSETST_HH

//== INCLUDES =================================================================

#include <OpenMesh/Tools/Decimater/ModBaseT.hh>

//== NAMESPACE ================================================================

namespace OpenMesh { // BEGIN_NS_OPENMESH
namespace Decimater { // BEGIN_NS_DECIMATER

//== CLASS DEFINITION =========================================================

/** Lock one-ring around remaining vertex after a collapse to prevent
 *  further collapses of halfedges incident to the one-ring vertices.
 */
template<class MeshT>
class ModIndependentSetsT: public ModBaseT<MeshT> {
  public:
    DECIMATING_MODULE( ModIndependentSetsT, MeshT, IndependentSets )
    ;

    /// Constructor
    ModIndependentSetsT(MeshT &_mesh) :
        Base(_mesh, true) {
    }

    /// override
    void postprocess_collapse(const CollapseInfo& _ci) {
      typename Mesh::VertexVertexIter vv_it;

      Base::mesh().status(_ci.v1).set_locked(true);
      vv_it = Base::mesh().vv_iter(_ci.v1);
      for (; vv_it.is_valid(); ++vv_it)
        Base::mesh().status(*vv_it).set_locked(true);
    }

  private:

    /// hide this method
    void set_binary(bool _b) { }
};

//=============================================================================
}// END_NS_DECIMATER
} // END_NS_OPENMESH
//=============================================================================
#endif // OPENMESH_TOOLS_MODINDEPENDENTSETST_HH defined
//=============================================================================

