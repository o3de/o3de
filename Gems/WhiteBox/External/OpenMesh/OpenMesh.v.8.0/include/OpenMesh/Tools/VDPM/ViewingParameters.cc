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
//  CLASS newClass - IMPLEMENTATION
//
//=============================================================================


//== INCLUDES =================================================================

#include <OpenMesh/Tools/VDPM/ViewingParameters.hh>
#include <iostream>

//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace VDPM {


//== IMPLEMENTATION ========================================================== 


ViewingParameters::
ViewingParameters()
{
  for ( unsigned int i = 0; i < 16; ++i)
    modelview_matrix_[i] = 0.0;

  fovy_ = 45.0f;
  aspect_ = 1.0f;
  tolerance_square_ = 0.001f;
}

void
ViewingParameters::
update_viewing_configurations()
{
  // |a11 a12 a13|-1       |  a33a22-a32a23  -(a33a12-a32a13)   a23a12-a22a13 |
  // |a21 a22 a23| = 1/DET*|-(a33a21-a31a23)   a33a11-a31a13  -(a23a11-a21a13)|
  // |a31 a32 a33|         |  a32a21-a31a22  -(a32a11-a31a12)   a22a11-a21a12 |
  //  DET  =  a11(a33a22-a32a23)-a21(a33a12-a32a13)+a31(a23a12-a22a13)

  float   invdet;  
  float   a11, a12, a13, a21, a22, a23, a31, a32, a33;
  Vec3f  trans;

// Workaround for internal compiler error on Visual Studio 2015 Update 1
#if ((defined(_MSC_VER) && (_MSC_VER >= 1800)) )
  Vec3f inv_rot[3]{ {},{},{} };
  Vec3f normal[4]{ {},{},{},{} };
#else
  Vec3f inv_rot[3];
  Vec3f normal[4];
#endif

  a11      = (float) modelview_matrix_[0]; 
  a12      = (float) modelview_matrix_[4]; 
  a13      = (float) modelview_matrix_[8];     
  trans[0] = (float) modelview_matrix_[12];

  a21      = (float) modelview_matrix_[1];
  a22      = (float) modelview_matrix_[5];
  a23      = (float) modelview_matrix_[9];
  trans[1] = (float) modelview_matrix_[13];
  
  a31      = (float) modelview_matrix_[2];
  a32      = (float) modelview_matrix_[6];
  a33      = (float) modelview_matrix_[10];
  trans[2] = (float) modelview_matrix_[14];

  invdet = a11*(a33*a22-a32*a23) - a21*(a33*a12-a32*a13) + a31*(a23*a12-a22*a13);
  invdet= (float) 1.0/invdet;

  (inv_rot[0])[0] =  (a33*a22-a32*a23) * invdet;
  (inv_rot[0])[1] = -(a33*a12-a32*a13) * invdet;
  (inv_rot[0])[2] =  (a23*a12-a22*a13) * invdet;
  (inv_rot[1])[0] = -(a33*a21-a31*a23) * invdet;
  (inv_rot[1])[1] =  (a33*a11-a31*a13) * invdet;
  (inv_rot[1])[2] = -(a23*a11-a21*a13) * invdet;
  (inv_rot[2])[0] =  (a32*a21-a31*a22) * invdet;
  (inv_rot[2])[1] = -(a32*a11-a31*a12) * invdet;
  (inv_rot[2])[2] =  (a22*a11-a21*a12) * invdet;

  eye_pos_   = - Vec3f(dot(inv_rot[0], trans), 
		      dot(inv_rot[1], trans), 
		      dot(inv_rot[2], trans));
  right_dir_ =   Vec3f(a11, a12, a13);
  up_dir_    =   Vec3f(a21, a22, a23);
  view_dir_  = - Vec3f(a31, a32, a33);

  //float	aspect = width() / height();
  const float	half_theta = fovy() * 0.5f;
  const float	half_phi = atanf(aspect() * tanf(half_theta));
  
  const float sin1 = sinf(half_theta);
  const float cos1 = cosf(half_theta);
  const float sin2 = sinf(half_phi);
  const float cos2 = cosf(half_phi);
  
  normal[0] =  cos2 * right_dir_ + sin2 * view_dir_;
  normal[1] = -cos1 * up_dir_    - sin1 * view_dir_;
  normal[2] = -cos2 * right_dir_ + sin2 * view_dir_;
  normal[3] =  cos1 * up_dir_    - sin1 * view_dir_;

  for (int i=0; i<4; i++)
    frustum_plane_[i] = Plane3d(normal[i], eye_pos_);
}

void
ViewingParameters::
PrintOut()
{
  std::cout << "  ModelView matrix: " << std::endl;
  std::cout << "    |" << modelview_matrix_[0] << " " << modelview_matrix_[4] << " " << modelview_matrix_[8] << " " << modelview_matrix_[12] << "|" << std::endl;
  std::cout << "    |" << modelview_matrix_[1] << " " << modelview_matrix_[5] << " " << modelview_matrix_[9] << " " << modelview_matrix_[13] << "|" << std::endl;
  std::cout << "    |" << modelview_matrix_[2] << " " << modelview_matrix_[6] << " " << modelview_matrix_[10] << " " << modelview_matrix_[14] << "|" << std::endl;
  std::cout << "    |" << modelview_matrix_[3] << " " << modelview_matrix_[7] << " " << modelview_matrix_[11] << " " << modelview_matrix_[15] << "|" << std::endl; 
  std::cout << "  Fovy: " << fovy_ << std::endl;
  std::cout << "  Aspect: " << aspect_ << std::endl;
  std::cout << "  Tolerance^2: " << tolerance_square_ << std::endl;
  std::cout << "  Eye Pos: " << eye_pos_ << std::endl;
  std::cout << "  Right dir: " << right_dir_ << std::endl;
  std::cout << "  Up dir: " << up_dir_ << std::endl;
  std::cout << "  View dir: " << view_dir_ << std::endl;
}

//=============================================================================
} // namespace VDPM
} // namespace OpenMesh
//=============================================================================
