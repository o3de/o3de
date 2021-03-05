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



#ifndef OPENMESH_KERNEL_OSG_COLOR_CAST_HH
#define OPENMESH_KERNEL_OSG_COLOR_CAST_HH

#include <algorithm>
#include <OpenMesh/Core/Utils/color_cast.hh>
#include <OpenSG/OSGGeometry.h>

namespace OpenMesh {

/// Helper struct
/// \internal
template <>
struct color_caster<osg::Color3ub,osg::Color3f>
{
  typedef osg::Color3ub return_type;
  typedef unsigned char ub;

  inline static return_type cast(const osg::Color3f& _src)
  {
    return return_type( (ub)std::min((_src[0]* 255.0f + 0.5f),255.0f),
                        (ub)std::min((_src[1]* 255.0f + 0.5f),255.0f),
                        (ub)std::min((_src[2]* 255.0f + 0.5f),255.0f) );
  }
};

/// Helper struct
/// \internal
template <>
struct color_caster<osg::Color3f,osg::Color3ub>
{
  typedef osg::Color3f return_type;

  inline static return_type cast(const osg::Color3ub& _src)
  {
    return return_type( (float)(_src[0] / 255.0f ),
                        (float)(_src[1] / 255.0f ),
                        (float)(_src[2] / 255.0f ) );
  }
};

} // namespace OpenMesh

#endif // OPENMESH_KERNEL_OSG_COLOR_CAST_HH 
