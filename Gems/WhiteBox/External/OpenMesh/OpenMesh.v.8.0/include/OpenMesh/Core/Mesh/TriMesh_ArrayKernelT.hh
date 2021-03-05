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
//  CLASS TriMesh_ArrayKernelT
//
//=============================================================================


#ifndef OPENMESH_TRIMESH_ARRAY_KERNEL_HH
#define OPENMESH_TRIMESH_ARRAY_KERNEL_HH


//== INCLUDES =================================================================


#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Mesh/TriConnectivity.hh>
#include <OpenMesh/Core/Mesh/Traits.hh>
#include <OpenMesh/Core/Mesh/FinalMeshItemsT.hh>
#include <OpenMesh/Core/Mesh/AttribKernelT.hh>
#include <OpenMesh/Core/Mesh/TriMeshT.hh>


//== NAMESPACES ===============================================================


namespace OpenMesh {

template<class Traits>
class PolyMesh_ArrayKernelT;
//== CLASS DEFINITION =========================================================


/// Helper class to create a TriMesh-type based on ArrayKernelT
template <class Traits>
struct TriMesh_ArrayKernel_GeneratorT
{
  typedef FinalMeshItemsT<Traits, true>               MeshItems;
  typedef AttribKernelT<MeshItems, TriConnectivity>   AttribKernel;
  typedef TriMeshT<AttribKernel>                      Mesh;
};



/** \ingroup mesh_types_group
    Triangle mesh based on the ArrayKernel.
    \see OpenMesh::TriMeshT
    \see OpenMesh::ArrayKernelT
*/
template <class Traits = DefaultTraits>
class TriMesh_ArrayKernelT
  : public TriMesh_ArrayKernel_GeneratorT<Traits>::Mesh
{
public:
  TriMesh_ArrayKernelT() {}
  template<class OtherTraits>
   TriMesh_ArrayKernelT( const PolyMesh_ArrayKernelT<OtherTraits> & t)
  {
     //assign the connectivity and standard properties
     this->assign(t,true);
  }
};


//=============================================================================
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_TRIMESH_ARRAY_KERNEL_HH
//=============================================================================
