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



/** \file McDecimaterT.hh
 */

//=============================================================================
//
//  CLASS McDecimaterT
//
//=============================================================================

#ifndef OPENMESH_MC_DECIMATER_DECIMATERT_HH
#define OPENMESH_MC_DECIMATER_DECIMATERT_HH


//== INCLUDES =================================================================

#include <memory>
#include <OpenMesh/Tools/Decimater/BaseDecimaterT.hh>



//== NAMESPACE ================================================================

namespace OpenMesh  {
namespace Decimater {


//== CLASS DEFINITION =========================================================


/** Multiple choice decimater framework
    \see BaseModT, \ref decimater_docu
*/
template < typename MeshT >
class McDecimaterT : virtual public BaseDecimaterT<MeshT> //virtual especially for the mixed decimater
{
public: //-------------------------------------------------------- public types

  typedef McDecimaterT< MeshT >         Self;
  typedef MeshT                         Mesh;
  typedef CollapseInfoT<MeshT>          CollapseInfo;
  typedef ModBaseT<MeshT>               Module;
  typedef std::vector< Module* >        ModuleList;
  typedef typename ModuleList::iterator ModuleListIterator;

public: //------------------------------------------------------ public methods

  /// Constructor
  McDecimaterT( Mesh& _mesh );

  /// Destructor
  ~McDecimaterT();

public:

  /** Decimate (perform _n_collapses collapses). Return number of
      performed collapses. If _n_collapses is not given reduce as
      much as possible */
  size_t decimate( size_t _n_collapses );

  /// Decimate to target complexity, returns number of collapses
  size_t decimate_to( size_t  _n_vertices )
  {
    return ( (_n_vertices < this->mesh().n_vertices()) ?
	     decimate( this->mesh().n_vertices() - _n_vertices ) : 0 );
  }

  /** Decimate to target complexity (vertices and faces).
   *  Stops when the number of vertices or the number of faces is reached.
   *  Returns number of performed collapses.
   */
  size_t decimate_to_faces( size_t  _n_vertices=0, size_t _n_faces=0 );

  /**
   * Decimate only with constraints, while _factor gives the
   * percentage of the constraints that should be used
   */
  size_t decimate_constraints_only(float _factor);

  size_t samples(){return randomSamples_;}
  void set_samples(const size_t _value){randomSamples_ = _value;}

private: //------------------------------------------------------- private data


  // reference to mesh
  Mesh&      mesh_;

  size_t randomSamples_;

};

//=============================================================================
} // END_NS_DECIMATER
} // END_NS_OPENMESH
//=============================================================================
#if defined(OM_INCLUDE_TEMPLATES) && !defined(OPENMESH_MULTIPLE_CHOICE_DECIMATER_DECIMATERT_CC)
#define OPENMESH_MULTIPLE_CHOICE_DECIMATER_TEMPLATES
#include "McDecimaterT_impl.hh"
#endif
//=============================================================================
#endif // OPENMESH_MC_DECIMATER_DECIMATERT_HH defined
//=============================================================================

