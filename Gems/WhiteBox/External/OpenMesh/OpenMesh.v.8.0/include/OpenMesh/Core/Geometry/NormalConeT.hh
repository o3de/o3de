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
//  CLASS NormalCone
//
//=============================================================================


#ifndef OPENMESH_NORMALCONE_HH
#define OPENMESH_NORMALCONE_HH


//== INCLUDES =================================================================


#include <OpenMesh/Core/Geometry/VectorT.hh>


//== NAMESPACES ===============================================================


namespace OpenMesh {


//== CLASS DEFINITION =========================================================


/** /class NormalCone NormalCone.hh <ACG/Geometry/Types/NormalCone.hh>

    NormalCone that can be merged with other normal cones. Provides
    the center normal and the opening angle.
**/

template <typename Scalar>
class NormalConeT
{
public:

  // typedefs
  typedef VectorT<Scalar, 3>  Vec3;


  //! default constructor (not initialized)
  NormalConeT() {}

  //! Initialize cone with center (unit vector) and angle (radius in radians)
  NormalConeT(const Vec3& _center_normal, Scalar _angle=0.0);

  //! return max. distance (radians) unit vector to cone (distant side)
  Scalar max_angle(const Vec3&) const;

  //! return max. distance (radians) cone to cone (distant sides)
  Scalar max_angle(const NormalConeT&) const;

  //! merge _cone; this instance will then enclose both former cones
  void merge(const NormalConeT&);

  //! returns center normal
  const Vec3& center_normal() const { return center_normal_; }

  //! returns size of cone (radius in radians)
  inline Scalar angle() const { return angle_; }

private:

  Vec3    center_normal_;
  Scalar  angle_;
};


//=============================================================================
} // namespace OpenMesh
//=============================================================================
#if defined(OM_INCLUDE_TEMPLATES) && !defined(OPENMESH_NORMALCONE_C)
#define OPENMESH_NORMALCONE_TEMPLATES
#include "NormalConeT_impl.hh"
#endif
//=============================================================================
#endif // OPENMESH_NORMALCONE_HH defined
//=============================================================================

