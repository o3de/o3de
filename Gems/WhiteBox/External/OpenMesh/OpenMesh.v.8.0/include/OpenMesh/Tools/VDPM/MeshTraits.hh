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
//  CLASS VDPMTraits
//
//=============================================================================


#ifndef OPENMESH_VDPM_TRAITS_HH
#define OPENMESH_VDPM_TRAITS_HH


//== INCLUDES =================================================================


#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Mesh/Traits.hh>
#include <OpenMesh/Tools/VDPM/VHierarchy.hh>

//== FORWARDDECLARATIONS ======================================================


//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace VDPM {

//== CLASS DEFINITION =========================================================

	      
/** \class MeshTraits MeshTraits.hh <OpenMesh/Tools/VDPM/MeshTraits.hh>

    Mesh traits for View Dependent Progressive Meshes  
*/

struct OPENMESHDLLEXPORT MeshTraits : public DefaultTraits
{
  VertexTraits
  {
  public:

    VHierarchyNodeHandle vhierarchy_node_handle()
    {
      return node_handle_; 
    }

    void set_vhierarchy_node_handle(VHierarchyNodeHandle _node_handle)
    {
      node_handle_ = _node_handle; 
    }
    
    bool is_ancestor(const VHierarchyNodeIndex &_other)
    {
      return false; 
    }

  private:

    VHierarchyNodeHandle  node_handle_;
   
  };
  
  VertexAttributes(OpenMesh::Attributes::Status |
		   OpenMesh::Attributes::Normal);
  HalfedgeAttributes(OpenMesh::Attributes::PrevHalfedge);
  EdgeAttributes(OpenMesh::Attributes::Status);
  FaceAttributes(OpenMesh::Attributes::Status |
		 OpenMesh::Attributes::Normal);
};


//=============================================================================
} // namespace VDPM
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_VDPM_TRAITS_HH defined
//=============================================================================

