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



/** \file LaplaceSmootherT.hh
    
 */

//=============================================================================
//
//  CLASS LaplaceSmootherT
//
//=============================================================================

#ifndef OPENMESH_LAPLACE_SMOOTHERT_HH
#define OPENMESH_LAPLACE_SMOOTHERT_HH



//== INCLUDES =================================================================

#include <OpenMesh/Tools/Smoother/SmootherT.hh>


//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace Smoother {

//== CLASS DEFINITION =========================================================

/// Laplacian Smoothing.      
template <class Mesh>
class LaplaceSmootherT : public SmootherT<Mesh>
{
private:
  typedef SmootherT<Mesh>                   Base;
public:

  typedef typename SmootherT<Mesh>::Component     Component;
  typedef typename SmootherT<Mesh>::Continuity    Continuity;
  typedef typename SmootherT<Mesh>::Scalar        Scalar;
  typedef typename SmootherT<Mesh>::VertexHandle  VertexHandle;
  typedef typename SmootherT<Mesh>::EdgeHandle    EdgeHandle;
  

  LaplaceSmootherT( Mesh& _mesh );
  virtual ~LaplaceSmootherT();


  void initialize(Component _comp, Continuity _cont);


protected:

  // misc helpers

  Scalar weight(VertexHandle _vh) const 
  { return Base::mesh_.property(vertex_weights_, _vh); }

  Scalar weight(EdgeHandle _eh) const 
  { return Base::mesh_.property(edge_weights_, _eh); }


private:

  enum LaplaceWeighting { UniformWeighting, CotWeighting };
  void compute_weights(LaplaceWeighting _mode);


  OpenMesh::VPropHandleT<Scalar>  vertex_weights_;
  OpenMesh::EPropHandleT<Scalar>  edge_weights_;
};


//=============================================================================
} // namespace Smoother
} // namespace OpenMesh
//=============================================================================
#if defined(OM_INCLUDE_TEMPLATES) && !defined(OPENMESH_LAPLACE_SMOOTHERT_C)
#define OPENMESH_LAPLACE_SMOOTHERT_TEMPLATES
#include "LaplaceSmootherT_impl.hh"
#endif
//=============================================================================
#endif // OPENMESH_LAPLACE_SMOOTHERT_HH defined
//=============================================================================

