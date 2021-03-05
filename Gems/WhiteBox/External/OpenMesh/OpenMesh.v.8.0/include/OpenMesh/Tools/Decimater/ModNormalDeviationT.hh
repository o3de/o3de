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


/** \file ModNormalDeviationT.hh
 */

//=============================================================================
//
//  CLASS ModNormalDeviationT
//
//=============================================================================


#ifndef OPENMESH_DECIMATER_MODNORMALDEVIATIONT_HH
#define OPENMESH_DECIMATER_MODNORMALDEVIATIONT_HH


//== INCLUDES =================================================================

#include <OpenMesh/Tools/Decimater/ModBaseT.hh>
#include <OpenMesh/Core/Utils/Property.hh>
#include <OpenMesh/Core/Geometry/NormalConeT.hh>


//== NAMESPACES ===============================================================

namespace OpenMesh  {
namespace Decimater {


//== CLASS DEFINITION =========================================================


/** \brief Use Normal deviation to control decimation
  *
  * The module tracks the normals while decimating
  * a normal cone consisting of all normals of the
  * faces collapsed together is computed and if
  * a collapse would increase the size of
  * the cone to a value greater than the given value
  * the collapse will be illegal.
  *
  * In binary and mode, the collapse is legal if:
  *  - The normal deviation after the collapse is lower than the given value
  *
  * In continuous mode the maximal deviation is returned
  */
template <class MeshT>
class ModNormalDeviationT : public ModBaseT< MeshT >
{
public:

  DECIMATING_MODULE( ModNormalDeviationT, MeshT, NormalDeviation );

  typedef typename Mesh::Scalar                     Scalar;
  typedef typename Mesh::Point                      Point;
  typedef typename Mesh::Normal                     Normal;
  typedef typename Mesh::VertexHandle               VertexHandle;
  typedef typename Mesh::FaceHandle                 FaceHandle;
  typedef typename Mesh::EdgeHandle                 EdgeHandle;
  typedef NormalConeT<Scalar>                       NormalCone;



public:

  /// Constructor
  ModNormalDeviationT(MeshT& _mesh, float _max_dev = 180.0)
  : Base(_mesh, true), mesh_(Base::mesh())
  {
    set_normal_deviation(_max_dev);
    mesh_.add_property(normal_cones_);

    const bool mesh_has_normals = _mesh.has_face_normals();
    _mesh.request_face_normals();

    if (!mesh_has_normals)
    {
      omerr() << "Mesh has no face normals. Compute them automatically." << std::endl;
      _mesh.update_face_normals();
    }
  }


  /// Destructor
  ~ModNormalDeviationT() {
    mesh_.remove_property(normal_cones_);
    mesh_.release_face_normals();
  }


  /// Get normal deviation ( 0 .. 360 )
  Scalar normal_deviation() const {
    return normal_deviation_ / M_PI * 180.0;
  }

  /// Set normal deviation ( 0 .. 360 )
  void set_normal_deviation(Scalar _s) {
    normal_deviation_ = _s / static_cast<Scalar>(180.0) * static_cast<Scalar>(M_PI);
  }


  /// Allocate and init normal cones
  void  initialize() {
    if (!normal_cones_.is_valid())
      mesh_.add_property(normal_cones_);

    typename Mesh::FaceIter f_it  = mesh_.faces_begin(),
        f_end = mesh_.faces_end();

    for (; f_it != f_end; ++f_it)
      mesh_.property(normal_cones_, *f_it) = NormalCone(mesh_.normal(*f_it));
  }

  /** \brief Control normals when Decimating
   *
   * Binary and Cont. mode.
   *
   * The module tracks the normals while decimating
   * a normal cone consisting of all normals of the
   * faces collapsed together is computed and if
   * a collapse would increase the size of
   * the cone to a value greater than the given value
   * the collapse will be illegal.
   *
   * @param _ci Collapse info data
   * @return Half of the normal cones size (radius in radians)
   */
  float collapse_priority(const CollapseInfo& _ci) {
    // simulate collapse
    mesh_.set_point(_ci.v0, _ci.p1);


    typename Mesh::Scalar               max_angle(0.0);
    typename Mesh::ConstVertexFaceIter  vf_it(mesh_, _ci.v0);
    typename Mesh::FaceHandle           fh, fhl, fhr;

    if (_ci.v0vl.is_valid())  fhl = mesh_.face_handle(_ci.v0vl);
    if (_ci.vrv0.is_valid())  fhr = mesh_.face_handle(_ci.vrv0);

    for (; vf_it.is_valid(); ++vf_it) {
      fh = *vf_it;
      if (fh != _ci.fl && fh != _ci.fr) {
        NormalCone nc = mesh_.property(normal_cones_, fh);

        nc.merge(NormalCone(mesh_.calc_face_normal(fh)));
        if (fh == fhl) nc.merge(mesh_.property(normal_cones_, _ci.fl));
        if (fh == fhr) nc.merge(mesh_.property(normal_cones_, _ci.fr));

        if (nc.angle() > max_angle) {
          max_angle = nc.angle();
          if (max_angle > 0.5 * normal_deviation_)
            break;
        }
      }
    }


    // undo simulation changes
    mesh_.set_point(_ci.v0, _ci.p0);


    return (max_angle < 0.5 * normal_deviation_ ? max_angle : float( Base::ILLEGAL_COLLAPSE ));
  }

  /// set the percentage of normal deviation
  void set_error_tolerance_factor(double _factor) {
    if (_factor >= 0.0 && _factor <= 1.0) {
      // the smaller the factor, the smaller normal_deviation_ gets
      // thus creating a stricter constraint
      // division by error_tolerance_factor_ is for normalization
      Scalar normal_deviation = normal_deviation_ *  static_cast<Scalar>(  180.0 / M_PI * _factor / this->error_tolerance_factor_);

      set_normal_deviation(normal_deviation);
      this->error_tolerance_factor_ = _factor;
    }
  }


  void  postprocess_collapse(const CollapseInfo& _ci) {
    // account for changed normals
    typename Mesh::VertexFaceIter vf_it(mesh_, _ci.v1);
    for (; vf_it.is_valid(); ++vf_it)
      mesh_.property(normal_cones_, *vf_it).
      merge(NormalCone(mesh_.normal(*vf_it)));


    // normal cones of deleted triangles
    typename Mesh::FaceHandle fh;

    if (_ci.vlv1.is_valid()) {
      fh = mesh_.face_handle(mesh_.opposite_halfedge_handle(_ci.vlv1));
      if (fh.is_valid())
        mesh_.property(normal_cones_, fh).
        merge(mesh_.property(normal_cones_, _ci.fl));
    }

    if (_ci.v1vr.is_valid()) {
      fh = mesh_.face_handle(mesh_.opposite_halfedge_handle(_ci.v1vr));
      if (fh.is_valid())
        mesh_.property(normal_cones_, fh).
        merge(mesh_.property(normal_cones_, _ci.fr));
    }
  }



private:

  Mesh&                               mesh_;
  Scalar                              normal_deviation_;
  OpenMesh::FPropHandleT<NormalCone>  normal_cones_;
};


//=============================================================================
} // END_NS_DECIMATER
} // END_NS_OPENMESH
//=============================================================================
#endif // OPENMESH_DECIMATER_MODNORMALDEVIATIONT_HH defined
//=============================================================================

