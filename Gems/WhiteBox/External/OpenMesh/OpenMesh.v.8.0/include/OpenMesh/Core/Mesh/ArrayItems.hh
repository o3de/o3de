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



#ifndef OPENMESH_ARRAY_ITEMS_HH
#define OPENMESH_ARRAY_ITEMS_HH


//== INCLUDES =================================================================


#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Utils/GenProg.hh>
#include <OpenMesh/Core/Mesh/Handles.hh>


//== NAMESPACES ===============================================================

namespace OpenMesh {


//== CLASS DEFINITION =========================================================


/// Definition of mesh items for use in the ArrayKernel
struct ArrayItems
{

  //------------------------------------------------------ internal vertex type

  /// The vertex item
  class Vertex
  {
    friend class ArrayKernel;
    HalfedgeHandle  halfedge_handle_;
  };


  //---------------------------------------------------- internal halfedge type

#ifndef DOXY_IGNORE_THIS
  class Halfedge_without_prev
  {
    friend class ArrayKernel;
    FaceHandle      face_handle_;
    VertexHandle    vertex_handle_;
    HalfedgeHandle  next_halfedge_handle_;
  };
#endif

#ifndef DOXY_IGNORE_THIS
  class Halfedge_with_prev : public Halfedge_without_prev
  {
    friend class ArrayKernel;
    HalfedgeHandle  prev_halfedge_handle_;
  };
#endif

  //TODO: should be selected with config.h define
  typedef Halfedge_with_prev                Halfedge;
  typedef GenProg::Bool2Type<true>          HasPrevHalfedge;

  //-------------------------------------------------------- internal edge type
#ifndef DOXY_IGNORE_THIS
  class Edge
  {
    friend class ArrayKernel;
    Halfedge  halfedges_[2];
  };
#endif

  //-------------------------------------------------------- internal face type
#ifndef DOXY_IGNORE_THIS
  class Face
  {
    friend class ArrayKernel;
    HalfedgeHandle  halfedge_handle_;
  };
};
#endif

//=============================================================================
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_ITEMS_HH defined
//=============================================================================
