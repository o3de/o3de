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
//  CLASS newClass
//
//=============================================================================

#ifndef OPENMESH_VDPROGMESH_VIEWINGPARAMETERS_HH
#define OPENMESH_VDPROGMESH_VIEWINGPARAMETERS_HH


//== INCLUDES =================================================================

#include <OpenMesh/Core/Geometry/VectorT.hh>
#include <OpenMesh/Core/Geometry/Plane3d.hh>


//== FORWARDDECLARATIONS ======================================================


//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace VDPM {

//== CLASS DEFINITION =========================================================

	      
/** \todo ViewerParameters documentation
 */
class OPENMESHDLLEXPORT ViewingParameters
{
private:
  double    modelview_matrix_[16];
  float     fovy_;
  float     aspect_;
  float     tolerance_square_;

  Vec3f   eye_pos_;
  Vec3f   right_dir_;
  Vec3f   up_dir_;
  Vec3f   view_dir_;

  Plane3d           frustum_plane_[4];

public:

  ViewingParameters();

  void increase_tolerance()           { tolerance_square_ *= 5.0f; }
  void decrease_tolerance()           { tolerance_square_ /= 5.0f; }  

  float fovy() const                  { return  fovy_; }
  float aspect() const                { return  aspect_; }
  float tolerance_square() const      { return  tolerance_square_; } 
  
  void set_fovy(float _fovy)                            { fovy_ = _fovy; }
  void set_aspect(float _aspect)                        { aspect_ = _aspect; }
  void set_tolerance_square(float _tolerance_square)    { tolerance_square_ = _tolerance_square; }

  const Vec3f& eye_pos() const    { return eye_pos_; }
  const Vec3f& right_dir() const  { return right_dir_; }
  const Vec3f& up_dir() const     { return up_dir_; }
  const Vec3f& view_dir() const   { return view_dir_; }
  Vec3f& eye_pos()                { return eye_pos_; }
  Vec3f& right_dir()              { return right_dir_; }
  Vec3f& up_dir()                 { return up_dir_; }
  Vec3f& view_dir()               { return view_dir_; }

  void frustum_planes( Plane3d _plane[4] )
  {
    for (unsigned int i=0; i<4; ++i)
      _plane[i] = frustum_plane_[i];
  }
   
  void get_modelview_matrix(double _modelview_matrix[16])  
  {
    for (unsigned int i=0; i<16; ++i)
      _modelview_matrix[i] = modelview_matrix_[i];
  }  

  void set_modelview_matrix(const double _modelview_matrix[16])
  {
    for (unsigned int i=0; i<16; ++i)
      modelview_matrix_[i] = _modelview_matrix[i];   
  }

  void update_viewing_configurations();

  void PrintOut();
};


//=============================================================================
} // namespace VDPM
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_VDPROGMESH_VIEWINGPARAMETERS_HH defined
//=============================================================================

