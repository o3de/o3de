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


/** \file ModAspectRatioT.hh
 */

//=============================================================================
//
//  CLASS ModAspectRatioT
//
//=============================================================================

#ifndef OPENMESH_DECIMATER_MODASPECTRATIOT_HH
#define OPENMESH_DECIMATER_MODASPECTRATIOT_HH

//== INCLUDES =================================================================

#include <OpenMesh/Tools/Decimater/ModBaseT.hh>
#include <OpenMesh/Core/Utils/Property.hh>

//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace Decimater {

//== CLASS DEFINITION =========================================================

/** \brief Use aspect ratio to control decimation
 *
 * This module computes the aspect ratio.
 *
 * In binary mode, the collapse is legal if:
 *  - The aspect ratio after the collapse is greater
 *  - The aspect ratio after the collapse is greater than the given minimum
 *
 * In continuous mode the collapse is illegal if:
 *  - The aspect ratio after the collapse is smaller than the given minimum
 *
 *
 */
template<class MeshT>
class ModAspectRatioT: public ModBaseT<MeshT> {
  public:

    DECIMATING_MODULE( ModAspectRatioT, MeshT, AspectRatio )
    ;

    typedef typename Mesh::Scalar Scalar;
    typedef typename Mesh::Point Point;

    /// constructor
    ModAspectRatioT(MeshT& _mesh, float _min_aspect = 5.0, bool _is_binary =
        true) :
        Base(_mesh, _is_binary), mesh_(Base::mesh()), min_aspect_(
            1.f / _min_aspect) {
      mesh_.add_property(aspect_);
    }

    /// destructor
    ~ModAspectRatioT() {
      mesh_.remove_property(aspect_);
    }

    /// get aspect ratio
    float aspect_ratio() const {
      return 1.f / min_aspect_;
    }

    /// set aspect ratio
    void set_aspect_ratio(float _f) {
      min_aspect_ = 1.f / _f;
    }

    /// precompute face aspect ratio
    void initialize();

    /// Returns the collapse priority
    float collapse_priority(const CollapseInfo& _ci);

    /// update aspect ratio of one-ring
    void preprocess_collapse(const CollapseInfo& _ci);

    /// set percentage of aspect ratio
    void set_error_tolerance_factor(double _factor);

  private:

    /** \brief return aspect ratio (length/height) of triangle
     *
     */
    Scalar aspectRatio(const Point& _v0, const Point& _v1, const Point& _v2);

  private:

    Mesh& mesh_;
    float min_aspect_;
    FPropHandleT<float> aspect_;
};

//=============================================================================
}// END_NS_DECIMATER
} // END_NS_OPENMESH
//=============================================================================
#if defined(OM_INCLUDE_TEMPLATES) && !defined(OPENMESH_DECIMATER_MODASPECTRATIOT_C)
#define OPENMESH_DECIMATER_MODASPECTRATIOT_TEMPLATES
#include "ModAspectRatioT_impl.hh"
#endif
//=============================================================================
#endif // OPENMESH_DECIMATER_MODASPECTRATIOT_HH defined
//=============================================================================

