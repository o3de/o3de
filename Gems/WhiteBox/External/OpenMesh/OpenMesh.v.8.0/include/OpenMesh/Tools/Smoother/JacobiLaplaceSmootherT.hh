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



/** \file JacobiLaplaceSmootherT.hh
    
 */


//=============================================================================
//
//  CLASS JacobiLaplaceSmootherT
//
//=============================================================================

#ifndef OPENMESH_JACOBI_LAPLACE_SMOOTHERT_HH
#define OPENMESH_JACOBI_LAPLACE_SMOOTHERT_HH


//== INCLUDES =================================================================

#include <OpenMesh/Tools/Smoother/LaplaceSmootherT.hh>


//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace Smoother {

//== CLASS DEFINITION =========================================================

/** Laplacian Smoothing.
 *
 */
template <class Mesh>
class JacobiLaplaceSmootherT : public LaplaceSmootherT<Mesh>
{
private:
  typedef LaplaceSmootherT<Mesh>            Base;
  
public:

  JacobiLaplaceSmootherT( Mesh& _mesh ) : LaplaceSmootherT<Mesh>(_mesh) {}

  // override: alloc umbrellas
  void smooth(unsigned int _n);


protected:

  virtual void compute_new_positions_C0();
  virtual void compute_new_positions_C1();


private:

  OpenMesh::VPropHandleT<typename Mesh::Normal>   umbrellas_;
  OpenMesh::VPropHandleT<typename Mesh::Normal>   squared_umbrellas_;
};


//=============================================================================
} // namespace Smoother
} // namespace OpenMesh
//=============================================================================
#if defined(OM_INCLUDE_TEMPLATES) && !defined(OPENMESH_JACOBI_LAPLACE_SMOOTHERT_C)
#define OPENMESH_JACOBI_LAPLACE_SMOOTHERT_TEMPLATES
#include "JacobiLaplaceSmootherT_impl.hh"
#endif
//=============================================================================
#endif // OPENMESH_JACOBI_LAPLACE_SMOOTHERT_HH defined
//=============================================================================

