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




/**
    \file Attributes.hh
    This file provides some macros containing attribute usage.
*/


#ifndef OPENMESH_ATTRIBUTES_HH
#define OPENMESH_ATTRIBUTES_HH


//== INCLUDES =================================================================


#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Mesh/Status.hh>


//== NAMESPACES ===============================================================


namespace OpenMesh {
namespace Attributes {


//== CLASS DEFINITION  ========================================================

/** Attribute bits
 *
 *  Use the bits to define a standard property at compile time using traits.
 *
 *  \include traits5.cc
 *
 *  \see \ref mesh_type
 */
enum AttributeBits
{
  None          = 0,  ///< Clear all attribute bits
  Normal        = 1,  ///< Add normals to mesh item (vertices/faces)
  Color         = 2,  ///< Add colors to mesh item (vertices/faces/edges)
  PrevHalfedge  = 4,  ///< Add storage for previous halfedge (halfedges). The bit is set by default in the DefaultTraits.
  Status        = 8,  ///< Add status to mesh item (all items)
  TexCoord1D    = 16, ///< Add 1D texture coordinates (vertices, halfedges)
  TexCoord2D    = 32, ///< Add 2D texture coordinates (vertices, halfedges)
  TexCoord3D    = 64, ///< Add 3D texture coordinates (vertices, halfedges)
  TextureIndex  = 128 ///< Add texture index (faces)
};


//=============================================================================
} // namespace Attributes
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_ATTRIBUTES_HH defined
//=============================================================================
