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




#ifndef OPENMESH_MESHCHECKER_HH
#define OPENMESH_MESHCHECKER_HH


//== INCLUDES =================================================================


#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/System/omstream.hh>
#include <OpenMesh/Core/Utils/GenProg.hh>
#include <OpenMesh/Core/Mesh/Attributes.hh>
#include <ostream>


//== NAMESPACES ===============================================================


namespace OpenMesh {
namespace Utils {

//== CLASS DEFINITION =========================================================

	      
/** Check integrity of mesh.
 *
 *  This class provides several functions to check the integrity of a mesh.
 */
template <class Mesh>
class MeshCheckerT
{
public:
   
  /// constructor
  explicit MeshCheckerT(const Mesh& _mesh) : mesh_(_mesh) {}
 
  /// destructor
  ~MeshCheckerT() {}


  /// what should be checked?
  enum CheckTargets
  {
    CHECK_EDGES     = 1,
    CHECK_VERTICES  = 2,
    CHECK_FACES     = 4,
    CHECK_ALL       = 255
  };

  
  /// check it, return true iff ok
  bool check( unsigned int _targets=CHECK_ALL,
	      std::ostream&  _os= omerr());


private:

  bool is_deleted(typename Mesh::VertexHandle _vh) 
  { return (mesh_.has_vertex_status() ? mesh_.status(_vh).deleted() : false); }

  bool is_deleted(typename Mesh::EdgeHandle _eh) 
  { return (mesh_.has_edge_status() ? mesh_.status(_eh).deleted() : false); }

  bool is_deleted(typename Mesh::FaceHandle _fh) 
  { return (mesh_.has_face_status() ? mesh_.status(_fh).deleted() : false); }


  // ref to mesh
  const Mesh&  mesh_;
};


//=============================================================================
} // namespace Utils
} // namespace OpenMesh
//=============================================================================
#if defined(OM_INCLUDE_TEMPLATES) && !defined(OPENMESH_MESHCHECKER_C)
#define OPENMESH_MESHCHECKER_TEMPLATES
#include "MeshCheckerT_impl.hh"
#endif
//=============================================================================
#endif // OPENMESH_MESHCHECKER_HH defined
//=============================================================================

