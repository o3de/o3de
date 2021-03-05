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


/** \file ModHausdorffT.hh
 */

//=============================================================================
//
//  CLASS ModHausdorffT
//
//=============================================================================

#ifndef OPENMESH_DECIMATER_MODHAUSDORFFT_HH
#define OPENMESH_DECIMATER_MODHAUSDORFFT_HH

//== INCLUDES =================================================================

#include <OpenMesh/Tools/Decimater/ModBaseT.hh>
#include <OpenMesh/Core/Utils/Property.hh>
#include <vector>
#include <cfloat>

//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace Decimater {

//== CLASS DEFINITION =========================================================

/** \brief Use Hausdorff distance to control decimation
 *
 * This module computes the aspect ratio.
 *
 * In binary mode, the collapse is legal if:
 *  - The distance after the collapse is lower than the given tolerance
 *
 * No continuous mode
 */
template<class MeshT>
class ModHausdorffT: public ModBaseT<MeshT> {
  public:

    DECIMATING_MODULE( ModHausdorffT, MeshT, Hausdorff );

    typedef typename Mesh::Scalar Scalar;
    typedef typename Mesh::Point Point;
    typedef typename Mesh::FaceHandle FaceHandle;
    typedef std::vector<Point> Points;

    /// Constructor
    ModHausdorffT(MeshT& _mesh, Scalar _error_tolerance = FLT_MAX) :
        Base(_mesh, true), mesh_(Base::mesh()), tolerance_(_error_tolerance) {
      mesh_.add_property(points_);
    }

    /// Destructor
    ~ModHausdorffT() {
      mesh_.remove_property(points_);
    }

    /// get max error tolerance
    Scalar tolerance() const {
      return tolerance_;
    }

    /// set max error tolerance
    void set_tolerance(Scalar _e) {
      tolerance_ = _e;
    }

    /// reset per-face point lists
    virtual void initialize();

    /** \brief compute Hausdorff error for one-ring
     *
     * This mod only allows collapses if the Hausdorff distance
     * after a collapse is lower than the given tolerance.
     *
     *
     * @param _ci Collapse info data
     * @return Binary return, if collapse is legal or illegal
     */

    virtual float collapse_priority(const CollapseInfo& _ci);

    /// re-distribute points
    virtual void postprocess_collapse(const CollapseInfo& _ci);

    /// set the percentage of tolerance
    void set_error_tolerance_factor(double _factor);

  private:

    /// squared distance from point _p to triangle (_v0, _v1, _v2)
    Scalar distPointTriangleSquared(const Point& _p, const Point& _v0, const Point& _v1, const Point& _v2);

    /// compute max error for face _fh w.r.t. its point list and _p
    Scalar compute_sqr_error(FaceHandle _fh, const Point& _p) const;

  private:

    /// Temporary point storage
    Points tmp_points_;

    Mesh&  mesh_;
    Scalar tolerance_;

    OpenMesh::FPropHandleT<Points> points_;
};

//=============================================================================
}// END_NS_DECIMATER
} // END_NS_OPENMESH
//=============================================================================
#if defined(OM_INCLUDE_TEMPLATES) && !defined(OPENMESH_DECIMATER_MODHAUSDORFFT_C)
#define OPENMESH_DECIMATER_MODHAUSDORFFT_TEMPLATES
#include "ModHausdorffT_impl.hh"
#endif
//=============================================================================
#endif // OPENMESH_DECIMATER_MODHAUSDORFFT_HH defined
//=============================================================================

