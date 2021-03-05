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



/** \file JacobiLaplaceSmootherT_impl.hh
    
 */

//=============================================================================
//
//  CLASS JacobiLaplaceSmootherT - IMPLEMENTATION
//
//=============================================================================

#define OPENMESH_JACOBI_LAPLACE_SMOOTHERT_C

//== INCLUDES =================================================================

#include <OpenMesh/Tools/Smoother/JacobiLaplaceSmootherT.hh>


//== NAMESPACES ===============================================================


namespace OpenMesh {
namespace Smoother {


//== IMPLEMENTATION ========================================================== 


template <class Mesh>
void
JacobiLaplaceSmootherT<Mesh>::
smooth(unsigned int _n)
{
  if (Base::continuity() > Base::C0)
  {
    Base::mesh_.add_property(umbrellas_);
    if (Base::continuity() > Base::C1)
      Base::mesh_.add_property(squared_umbrellas_);
  }

  LaplaceSmootherT<Mesh>::smooth(_n);

  if (Base::continuity() > Base::C0)
  {
    Base::mesh_.remove_property(umbrellas_);
    if (Base::continuity() > Base::C1)
      Base::mesh_.remove_property(squared_umbrellas_);
  }
}


//-----------------------------------------------------------------------------


template <class Mesh>
void
JacobiLaplaceSmootherT<Mesh>::
compute_new_positions_C0()
{
  typename Mesh::VertexIter  							v_it, v_end(Base::mesh_.vertices_end());
  typename Mesh::ConstVertexOHalfedgeIter voh_it;
  typename Mesh::Normal      							u, p, zero(0,0,0);
  typename Mesh::Scalar      							w;

  for (v_it=Base::mesh_.vertices_begin(); v_it!=v_end; ++v_it)
  {
    if (this->is_active(*v_it))
    {
      // compute umbrella
      u = zero;
      for (voh_it = Base::mesh_.cvoh_iter(*v_it); voh_it.is_valid(); ++voh_it) {
        w = this->weight(Base::mesh_.edge_handle(*voh_it));
        u += vector_cast<typename Mesh::Normal>(Base::mesh_.point(Base::mesh_.to_vertex_handle(*voh_it))) * w;
      }
      u *= this->weight(*v_it);
      u -= vector_cast<typename Mesh::Normal>(Base::mesh_.point(*v_it));

      // damping
      u *= static_cast< typename Mesh::Scalar >(0.5);
    
      // store new position
      p  = vector_cast<typename Mesh::Normal>(Base::mesh_.point(*v_it));
      p += u;
      this->set_new_position(*v_it, p);
    }
  }
}


//-----------------------------------------------------------------------------


template <class Mesh>
void
JacobiLaplaceSmootherT<Mesh>::
compute_new_positions_C1()
{
  typename Mesh::VertexIter  							v_it, v_end(Base::mesh_.vertices_end());
  typename Mesh::ConstVertexOHalfedgeIter voh_it;
  typename Mesh::Normal      							u, uu, p, zero(0,0,0);
  typename Mesh::Scalar      							w, diag;


  // 1st pass: compute umbrellas
  for (v_it=Base::mesh_.vertices_begin(); v_it!=v_end; ++v_it)
  {
    u = zero;
    for (voh_it = Base::mesh_.cvoh_iter(*v_it); voh_it.is_valid(); ++voh_it) {
      w  = this->weight(Base::mesh_.edge_handle(*voh_it));
      u -= vector_cast<typename Mesh::Normal>(Base::mesh_.point(Base::mesh_.to_vertex_handle(*voh_it)))*w;
    }
    u *= this->weight(*v_it);
    u += vector_cast<typename Mesh::Normal>(Base::mesh_.point(*v_it));

    Base::mesh_.property(umbrellas_, *v_it) = u;
  }


  // 2nd pass: compute updates
  for (v_it=Base::mesh_.vertices_begin(); v_it!=v_end; ++v_it)
  {
    if (this->is_active(*v_it))
    {
      uu   = zero;
      diag = 0.0;   
      for (voh_it = Base::mesh_.cvoh_iter(*v_it); voh_it.is_valid(); ++voh_it) {
        w  = this->weight(Base::mesh_.edge_handle(*voh_it));
        uu   -= Base::mesh_.property(umbrellas_, Base::mesh_.to_vertex_handle(*voh_it));
        diag += (w * this->weight(Base::mesh_.to_vertex_handle(*voh_it)) + static_cast<typename Mesh::Scalar>(1.0) ) * w;
      }
      uu   *= this->weight(*v_it);
      diag *= this->weight(*v_it);
      uu   += Base::mesh_.property(umbrellas_, *v_it);
      if (diag) uu *= static_cast<typename Mesh::Scalar>(1.0) / diag;

      // damping
      uu *= static_cast<typename vector_traits<typename Mesh::Normal>::value_type>(0.25);
    
      // store new position
      p  = vector_cast<typename Mesh::Normal>(Base::mesh_.point(*v_it));
      p -= uu;
      this->set_new_position(*v_it, p);
    }
  }
}


//=============================================================================
} // namespace Smoother
} // namespace OpenMesh
//=============================================================================
