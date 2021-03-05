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

/*
  Compute the dual of a mesh:
  - each face of the original mesh is replaced by a vertex at the center of gravity of the vertices of the face
  - each vertex of the original mesh is replaced by a face containing the dual vertices of its primal adjacent faces

  Changelog:
    - 29 mar 2010: initial work

  Programmer: 
    Clement Courbet - clement.courbet@ecp.fr

  (c) Clement Courbet 2010
*/

#ifndef OPENMESH_MESH_DUAL_H
#define OPENMESH_MESH_DUAL_H

//== INCLUDES =================================================================

// -------------------- STL
#include <vector>
#if defined(OM_CC_MIPS)
#  include <math.h>
#else
#  include <cmath>
#endif

#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>
#include <OpenMesh/Core/Utils/Property.hh>

//== FORWARDDECLARATIONS ======================================================

//== NAMESPACES ===============================================================


namespace OpenMesh   { 
namespace Util { 

//== Function DEFINITION =========================================================

/** \brief create a dual mesh
*
* This function takes a mesh and computes the dual mesh of it. Each face of the original mesh is replaced by a vertex at the center of gravity of the vertices of the face.
* Each vertex of the original mesh is replaced by a face containing the dual vertices of its primal adjacent faces.
*/
template <typename MeshTraits>
PolyMesh_ArrayKernelT<MeshTraits>* MeshDual (PolyMesh_ArrayKernelT<MeshTraits> &primal)
{
  PolyMesh_ArrayKernelT<MeshTraits>* dual = new PolyMesh_ArrayKernelT<MeshTraits>();

  //we will need to reference which vertex in the dual is attached to each face in the primal
  //and which face of the dual is attached to each vertex in the primal.

  FPropHandleT< typename PolyMesh_ArrayKernelT<MeshTraits>::VertexHandle > primalToDual;
  primal.add_property(primalToDual);

  //for each face in the primal mesh, add a vertex at the center of gravity of the face
  for(typename PolyMesh_ArrayKernelT<MeshTraits>::ConstFaceIter fit=primal.faces_begin(); fit!=primal.faces_end(); ++fit)
  {
      typename PolyMesh_ArrayKernelT<MeshTraits>::Point centerPoint(0,0,0);
      typename PolyMesh_ArrayKernelT<MeshTraits>::Scalar degree= 0.0;
      for(typename PolyMesh_ArrayKernelT<MeshTraits>::ConstFaceVertexIter vit=primal.cfv_iter(*fit); vit.is_valid(); ++vit, ++degree)
        centerPoint += primal.point(*vit);
        assert(degree!=0);
        centerPoint /= degree;
        primal.property(primalToDual, *fit) = dual->add_vertex(centerPoint);
  }

  //for each vertex in the primal, add a face in the dual
  std::vector< typename PolyMesh_ArrayKernelT<MeshTraits>::VertexHandle >  face_vhandles;
  for(typename PolyMesh_ArrayKernelT<MeshTraits>::ConstVertexIter vit=primal.vertices_begin(); vit!=primal.vertices_end(); ++vit)
  {
    if(!primal.is_boundary(*vit))
    {
      face_vhandles.clear();
      for(typename PolyMesh_ArrayKernelT<MeshTraits>::ConstVertexFaceIter fit=primal.cvf_iter(*vit); fit.is_valid(); ++fit)
        face_vhandles.push_back(primal.property(primalToDual, *fit));
      dual->add_face(face_vhandles);
    }
  }

  primal.remove_property(primalToDual);

  return dual;

}

//=============================================================================
} // namespace Util
} // namespace OpenMesh
//=============================================================================

//=============================================================================
#endif // OPENMESH_MESH_DUAL_H defined
//=============================================================================


