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
//  CLASS NormalConeT - IMPLEMENTATION
//
//=============================================================================

#define OPENMESH_NORMALCONE_C

//== INCLUDES =================================================================

#include <math.h>
#include "NormalConeT.hh"

#ifdef max
#  undef max
#endif

#ifdef min
#  undef min
#endif


//== NAMESPACES ===============================================================


namespace OpenMesh {


//== IMPLEMENTATION ========================================================== 

template <typename Scalar>
NormalConeT<Scalar>::
NormalConeT(const Vec3& _center_normal, Scalar _angle)
  : center_normal_(_center_normal), angle_(_angle)
{
}


//----------------------------------------------------------------------------


template <typename Scalar>
Scalar 
NormalConeT<Scalar>::
max_angle(const Vec3& _norm) const
{
  Scalar dotp = (center_normal_ | _norm);
  return (dotp >= 1.0 ? 0.0 : (dotp <= -1.0 ? M_PI : acos(dotp)))
    + angle_;
}


//----------------------------------------------------------------------------


template <typename Scalar>
Scalar 
NormalConeT<Scalar>::
max_angle(const NormalConeT& _cone) const
{
  Scalar dotp = (center_normal_ | _cone.center_normal_);
  Scalar centerAngle = dotp >= 1.0 ? 0.0 : (dotp <= -1.0 ? M_PI : acos(dotp));
  Scalar sideAngle0 = std::max(angle_-centerAngle, _cone.angle_);
  Scalar sideAngle1 = std::max(_cone.angle_-centerAngle, angle_);

  return centerAngle + sideAngle0 + sideAngle1;
}


//----------------------------------------------------------------------------


template <typename Scalar>
void 
NormalConeT<Scalar>::
merge(const NormalConeT& _cone)
{
  Scalar dotp = (center_normal_ | _cone.center_normal_);

  if (fabs(dotp) < 0.99999f)
  {
    // new angle
    Scalar centerAngle = acos(dotp);
    Scalar minAngle    = std::min(-angle(), centerAngle - _cone.angle());
    Scalar maxAngle    = std::max( angle(), centerAngle + _cone.angle());
    angle_     = (maxAngle - minAngle) * Scalar(0.5f);

    // axis by SLERP
    Scalar axisAngle =  Scalar(0.5f) * (minAngle + maxAngle);
    center_normal_ = ((center_normal_ * sin(centerAngle-axisAngle)
		                   + _cone.center_normal_ * sin(axisAngle))
		                    / sin(centerAngle));
  }
  else
  {
    // axes point in same direction
    if (dotp > 0.0f)
      angle_ = std::max(angle_, _cone.angle_);

    // axes point in opposite directions
    else
      angle_ =  Scalar(2.0f * M_PI);
  }
}


//=============================================================================
} // namespace OpenMesh
//=============================================================================
