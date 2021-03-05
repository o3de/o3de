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



/** \file DecimaterT.hh
 */

//=============================================================================
//
//  CLASS DecimaterT
//
//=============================================================================

#ifndef OPENMESH_DECIMATER_DECIMATERT_HH
#define OPENMESH_DECIMATER_DECIMATERT_HH


//== INCLUDES =================================================================

#include <memory>

#include <OpenMesh/Core/Utils/Property.hh>
#include <OpenMesh/Tools/Utils/HeapT.hh>
#include <OpenMesh/Tools/Decimater/BaseDecimaterT.hh>

//== NAMESPACE ================================================================

namespace OpenMesh  {
namespace Decimater {


//== CLASS DEFINITION =========================================================


/** Decimater framework.
    \see BaseModT, \ref decimater_docu
*/
template < typename MeshT >
class DecimaterT : virtual public BaseDecimaterT<MeshT> //virtual especially for the mixed decimater
{
public: //-------------------------------------------------------- public types

  typedef DecimaterT< MeshT >           Self;
  typedef MeshT                         Mesh;
  typedef CollapseInfoT<MeshT>          CollapseInfo;
  typedef ModBaseT<MeshT>               Module;
  typedef std::vector< Module* >        ModuleList;
  typedef typename ModuleList::iterator ModuleListIterator;

public: //------------------------------------------------------ public methods

  /// Constructor
  DecimaterT( Mesh& _mesh );

  /// Destructor
  ~DecimaterT();

public:

  /**
   * @brief Perform a number of collapses on the mesh.
   * @param _n_collapses Desired number of collapses. If zero (default), attempt
   *                     to do as many collapses as possible.
   * @return Number of collapses that were actually performed.
   * @note This operation only marks the removed mesh elements for deletion. In
   *       order to actually remove the decimated elements from the mesh, a
   *       subsequent call to ArrayKernel::garbage_collection() is required.
   */
  size_t decimate( size_t _n_collapses = 0 );

  /**
   * @brief Decimate the mesh to a desired target vertex complexity.
   * @param _n_vertices Target complexity, i.e. desired number of remaining
   *                    vertices after decimation.
   * @return Number of collapses that were actually performed.
   * @note This operation only marks the removed mesh elements for deletion. In
   *       order to actually remove the decimated elements from the mesh, a
   *       subsequent call to ArrayKernel::garbage_collection() is required.
   */
  size_t decimate_to( size_t  _n_vertices )
  {
    return ( (_n_vertices < this->mesh().n_vertices()) ?
	     decimate( this->mesh().n_vertices() - _n_vertices ) : 0 );
  }

  /**
   * @brief Attempts to decimate the mesh until a desired vertex or face
   *        complexity is achieved.
   * @param _n_vertices Target vertex complexity.
   * @param _n_faces Target face complexity.
   * @return Number of collapses that were actually performed.
   * @note Decimation stops as soon as either one of the two complexity bounds
   *       is satisfied.
   * @note This operation only marks the removed mesh elements for deletion. In
   *       order to actually remove the decimated elements from the mesh, a
   *       subsequent call to ArrayKernel::garbage_collection() is required.
   */
  size_t decimate_to_faces( size_t  _n_vertices=0, size_t _n_faces=0 );

public:

  typedef typename Mesh::VertexHandle    VertexHandle;
  typedef typename Mesh::HalfedgeHandle  HalfedgeHandle;

  /// Heap interface
  class HeapInterface
  {
  public:

    HeapInterface(Mesh&               _mesh,
      VPropHandleT<float> _prio,
      VPropHandleT<int>   _pos)
      : mesh_(_mesh), prio_(_prio), pos_(_pos)
    { }

    inline bool
    less( VertexHandle _vh0, VertexHandle _vh1 )
    { return mesh_.property(prio_, _vh0) < mesh_.property(prio_, _vh1); }

    inline bool
    greater( VertexHandle _vh0, VertexHandle _vh1 )
    { return mesh_.property(prio_, _vh0) > mesh_.property(prio_, _vh1); }

    inline int
    get_heap_position(VertexHandle _vh)
    { return mesh_.property(pos_, _vh); }

    inline void
    set_heap_position(VertexHandle _vh, int _pos)
    { mesh_.property(pos_, _vh) = _pos; }


  private:
    Mesh&                mesh_;
    VPropHandleT<float>  prio_;
    VPropHandleT<int>    pos_;
  };

  typedef Utils::HeapT<VertexHandle, HeapInterface>  DeciHeap;


private: //---------------------------------------------------- private methods

  /// Insert vertex in heap
  void heap_vertex(VertexHandle _vh);

private: //------------------------------------------------------- private data


  // reference to mesh
  Mesh&      mesh_;

  // heap
  #if (defined(_MSC_VER) && (_MSC_VER >= 1800)) || __cplusplus > 199711L || defined( __GXX_EXPERIMENTAL_CXX0X__ )
    std::unique_ptr<DeciHeap> heap_;
  #else
    std::auto_ptr<DeciHeap> heap_;
  #endif

  // vertex properties
  VPropHandleT<HalfedgeHandle>  collapse_target_;
  VPropHandleT<float>           priority_;
  VPropHandleT<int>             heap_position_;

};

//=============================================================================
} // END_NS_DECIMATER
} // END_NS_OPENMESH
//=============================================================================
#if defined(OM_INCLUDE_TEMPLATES) && !defined(OPENMESH_DECIMATER_DECIMATERT_CC)
#define OPENMESH_DECIMATER_TEMPLATES
#include "DecimaterT_impl.hh"
#endif
//=============================================================================
#endif // OPENMESH_DECIMATER_DECIMATERT_HH defined
//=============================================================================

