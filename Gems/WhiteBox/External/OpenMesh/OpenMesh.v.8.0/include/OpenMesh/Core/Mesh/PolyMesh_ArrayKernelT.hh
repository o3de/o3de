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
//  CLASS PolyMesh_ArrayKernelT
//
//=============================================================================


#ifndef OPENMESH_POLY_MESH_ARRAY_KERNEL_HH
#define OPENMESH_POLY_MESH_ARRAY_KERNEL_HH


//== INCLUDES =================================================================


#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Mesh/PolyConnectivity.hh>
#include <OpenMesh/Core/Mesh/Traits.hh>
#include <OpenMesh/Core/Mesh/FinalMeshItemsT.hh>
#include <OpenMesh/Core/Mesh/AttribKernelT.hh>
#include <OpenMesh/Core/Mesh/PolyMeshT.hh>


//== NAMESPACES ===============================================================


namespace OpenMesh {

template<class Traits>
class TriMesh_ArrayKernelT;
//== CLASS DEFINITION =========================================================

/// Helper class to build a PolyMesh-type
template <class Traits>
struct PolyMesh_ArrayKernel_GeneratorT
{
  typedef FinalMeshItemsT<Traits, false>              MeshItems;
  typedef AttribKernelT<MeshItems, PolyConnectivity>  AttribKernel;
  typedef PolyMeshT<AttribKernel>                     Mesh;
};


/** \class PolyMesh_ArrayKernelT PolyMesh_ArrayKernelT.hh <OpenMesh/Mesh/Types/PolyMesh_ArrayKernelT.hh>

    \ingroup mesh_types_group
    Polygonal mesh based on the ArrayKernel.
    \see OpenMesh::PolyMeshT
    \see OpenMesh::ArrayKernel
*/
template <class Traits = DefaultTraits>
class PolyMesh_ArrayKernelT
  : public PolyMesh_ArrayKernel_GeneratorT<Traits>::Mesh
{
public:
  PolyMesh_ArrayKernelT() {}
  template<class OtherTraits>
   PolyMesh_ArrayKernelT( const TriMesh_ArrayKernelT<OtherTraits> & t)
  {
     //assign the connectivity and standard properties
     this->assign(t, true);

  }
};


//=============================================================================
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_POLY_MESH_ARRAY_KERNEL_HH
//=============================================================================
