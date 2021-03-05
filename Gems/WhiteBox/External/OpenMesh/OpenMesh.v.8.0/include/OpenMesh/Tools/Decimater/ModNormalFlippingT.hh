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



/** \file ModNormalFlippingT.hh

 */

//=============================================================================
//
//  CLASS ModNormalFlipping
//
//=============================================================================


#ifndef OPENMESH_DECIMATER_MODNORMALFLIPPING_HH
#define OPENMESH_DECIMATER_MODNORMALFLIPPING_HH


//== INCLUDES =================================================================

#include <OpenMesh/Tools/Decimater/ModBaseT.hh>

//== NAMESPACES ===============================================================

namespace OpenMesh { // BEGIN_NS_OPENMESH
namespace Decimater { // BEGIN_NS_DECIMATER


//== CLASS DEFINITION =========================================================

/** Decimating module to avoid flipping of faces.
 *
 *  This module can be used only as a binary module. The criterion
 *  of allowing/disallowing the collapse is the angular deviation between
 *  the face normal of the original faces and normals of the faces after the
 *  collapse. The collapse will pass the test, if the deviation is below
 *  a given threshold.
 */
template <typename MeshT>
class ModNormalFlippingT : public ModBaseT< MeshT >
{
public:

  DECIMATING_MODULE( ModNormalFlippingT, MeshT, NormalFlipping );

public:

  /// Constructor
  ModNormalFlippingT( MeshT &_mesh) : Base(_mesh, true)
  {
    set_max_normal_deviation( 90.0f );
    const bool mesh_has_normals = _mesh.has_face_normals();
    _mesh.request_face_normals();

    if (!mesh_has_normals)
    {
      omerr() << "Mesh has no face normals. Compute them automatically." << std::endl;
      _mesh.update_face_normals();
    }
  }


  ~ModNormalFlippingT()
  {
    Base::mesh().release_face_normals();
  }


public:

  /** Compute collapse priority due to angular deviation of face normals
   *  before and after a collapse.
   *
   *  -# Compute for each adjacent face of \c _ci.v0 the face
   *  normal if the collpase would be executed.
   *
   *  -# Prevent the collapse, if the cosine of the angle between the
   *     original and the new normal is below a given threshold.
   *
   *  \param _ci The collapse description
   *  \return LEGAL_COLLAPSE or ILLEGAL_COLLAPSE
   *
   *  \see set_max_normal_deviation()
   */
  float collapse_priority(const CollapseInfo& _ci)
  {
    // simulate collapse
    Base::mesh().set_point(_ci.v0, _ci.p1);

    // check for flipping normals
    typename Mesh::ConstVertexFaceIter vf_it(Base::mesh(), _ci.v0);
    typename Mesh::FaceHandle          fh;
    typename Mesh::Scalar              c(1.0);

    for (; vf_it.is_valid(); ++vf_it)
    {
      fh = *vf_it;
      if (fh != _ci.fl && fh != _ci.fr)
      {
        typename Mesh::Normal n1 = Base::mesh().normal(fh);
        typename Mesh::Normal n2 = Base::mesh().calc_face_normal(fh);

        c = dot(n1, n2);

        if (c < min_cos_)
          break;
      }
    }

    // undo simulation changes
    Base::mesh().set_point(_ci.v0, _ci.p0);

    return float( (c < min_cos_) ? Base::ILLEGAL_COLLAPSE : Base::LEGAL_COLLAPSE );
  }

  /// set the percentage of maximum normal deviation
  void set_error_tolerance_factor(double _factor) {
    if (_factor >= 0.0 && _factor <= 1.0) {
      // the smaller the factor, the smaller max_deviation_ gets
      // thus creating a stricter constraint
      // division by error_tolerance_factor_ is for normalization
      double max_normal_deviation = (max_deviation_ * 180.0/M_PI) * _factor / this->error_tolerance_factor_;
      set_max_normal_deviation(max_normal_deviation);
      this->error_tolerance_factor_ = _factor;
    }
  }


public:

  /// get normal deviation
  double max_normal_deviation() const { return max_deviation_ / M_PI * 180.0; }

  /** Set normal deviation
   *
   *  Set the maximum angular deviation of the orignal normal and the new
   *  normal in degrees.
   */
  void set_max_normal_deviation(double _d) {
    max_deviation_ = _d / 180.0 * M_PI;
    min_cos_       = cos(max_deviation_);
  }

private:

  // hide this method
  void set_binary(bool _b) {}

private:

  // maximum normal deviation
  double max_deviation_, min_cos_;
};


//=============================================================================
} // END_NS_DECIMATER
} // END_NS_OPENMESH
//=============================================================================
#endif // OPENACG_MODNORMALFLIPPING_HH defined
//=============================================================================

