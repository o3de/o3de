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




/** \file Tools/Kernel_OSG/Traits.hh
    This file defines the default traits and some convenienve macros.
*/


//=============================================================================
//
//  CLASS Traits
//
//=============================================================================

#ifndef OPENMESH_KERNEL_OSG_TRAITS_HH
#define OPENMESH_KERNEL_OSG_TRAITS_HH


//== INCLUDES =================================================================


#include <OpenMesh/Core/Mesh/Traits.hh>
#include <OpenMesh/Tools/Kernel_OSG/VectorAdapter.hh>
//
#include <osg/Geometry>

//== NAMESPACES ===============================================================

namespace OpenMesh  {
namespace Kernel_OSG {

//== CLASS DEFINITION =========================================================


//== CLASS DEFINITION =========================================================


/** Base class for all mesh traits using the OSGArrayKernelT.
 *
 *  \see The Mesh docu section on \ref mesh_type.
 *  \see Traits.hh for a list of macros for traits classes.
 */
struct Traits : DefaultTraits
{
  typedef osg::Pnt3f            Point;
  typedef osg::Color3ub         Color;
  typedef osg::Vec3f            Normal;
  typedef osg::Vec2f            TexCoord;
  typedef osg::Vec3f::ValueType Scalar;
  
};

//=============================================================================
} // namespace Kernel_OSG
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_TRAITS_HH defined
//=============================================================================

