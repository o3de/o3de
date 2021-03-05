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
//  CLASS Plane3D
//
//=============================================================================


#ifndef OPENMESH_PLANE3D_HH
#define OPENMESH_PLANE3D_HH


//== INCLUDES =================================================================

#include <OpenMesh/Core/Geometry/VectorT.hh>


//== FORWARDDECLARATIONS ======================================================

//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace VDPM {

//== CLASS DEFINITION =========================================================

	      
/** \class Plane3d Plane3d.hh <OpenMesh/Tools/VDPM/Plane3d.hh>

    ax + by + cz + d = 0
*/


class OPENMESHDLLEXPORT Plane3d
{
public:

  typedef OpenMesh::Vec3f         vector_type;
  typedef vector_type::value_type value_type;

public:

  Plane3d()
    : d_(0)
  { }

  Plane3d(const vector_type &_dir, const vector_type &_pnt)
    : n_(_dir), d_(0)
  { 
    n_.normalize();
    d_ = -dot(n_,_pnt); 
  }

  value_type signed_distance(const OpenMesh::Vec3f &_p)
  {
    return  dot(n_ , _p) + d_;
  }

  // back compatibility
  value_type singed_distance(const OpenMesh::Vec3f &point)
  { return signed_distance( point ); }

public:

  vector_type n_;
  value_type  d_;

};

//=============================================================================
} // namespace VDPM
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_PLANE3D_HH defined
//=============================================================================
