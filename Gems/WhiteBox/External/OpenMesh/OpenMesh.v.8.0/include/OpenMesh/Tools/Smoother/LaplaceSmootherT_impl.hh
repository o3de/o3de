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



/** \file LaplaceSmootherT_impl.hh
    
 */

//=============================================================================
//
//  CLASS LaplaceSmootherT - IMPLEMENTATION
//
//=============================================================================

#define OPENMESH_LAPLACE_SMOOTHERT_C

//== INCLUDES =================================================================

#include <OpenMesh/Tools/Smoother/LaplaceSmootherT.hh>


//== NAMESPACES ===============================================================


namespace OpenMesh {
namespace Smoother {


//== IMPLEMENTATION ========================================================== 


template <class Mesh>
LaplaceSmootherT<Mesh>::
LaplaceSmootherT(Mesh& _mesh)
  : SmootherT<Mesh>(_mesh)
{
  // custom properties
  Base::mesh_.add_property(vertex_weights_);
  Base::mesh_.add_property(edge_weights_);
}


//-----------------------------------------------------------------------------


template <class Mesh>
LaplaceSmootherT<Mesh>::
~LaplaceSmootherT()
{
  // free custom properties
  Base::mesh_.remove_property(vertex_weights_);
  Base::mesh_.remove_property(edge_weights_);
}


//-----------------------------------------------------------------------------


template <class Mesh>
void
LaplaceSmootherT<Mesh>::
initialize(Component _comp, Continuity _cont)
{
  SmootherT<Mesh>::initialize(_comp, _cont);

  // calculate weights
  switch (_comp)
  {
    case Base::Tangential:
      compute_weights(UniformWeighting);
      break;


    case Base::Normal:
      compute_weights(CotWeighting);
      break;
      

    case Base::Tangential_and_Normal:
      compute_weights(UniformWeighting);
      break;
  }
}


//-----------------------------------------------------------------------------


template <class Mesh>
void
LaplaceSmootherT<Mesh>::
compute_weights(LaplaceWeighting _weighting)
{
  typename Mesh::VertexIter        v_it, v_end(Base::mesh_.vertices_end());
  typename Mesh::EdgeIter          e_it, e_end(Base::mesh_.edges_end());
  typename Mesh::HalfedgeHandle    heh0, heh1, heh2;
  typename Mesh::VertexHandle      v0, v1;
  const typename Mesh::Point       *p0, *p1, *p2;
  typename Mesh::Normal            d0, d1;
  typename Mesh::Scalar            weight, lb(-1.0), ub(1.0);



  // init vertex weights
  for (v_it=Base::mesh_.vertices_begin(); v_it!=v_end; ++v_it)
    Base::mesh_.property(vertex_weights_, *v_it) = 0.0;



  switch (_weighting)
  {
    // Uniform weighting
    case UniformWeighting:
    {
      for (e_it=Base::mesh_.edges_begin(); e_it!=e_end; ++e_it)
      {
        heh0   = Base::mesh_.halfedge_handle(*e_it, 0);
        heh1   = Base::mesh_.halfedge_handle(*e_it, 1);
        v0     = Base::mesh_.to_vertex_handle(heh0);
        v1     = Base::mesh_.to_vertex_handle(heh1);

        Base::mesh_.property(edge_weights_, *e_it)  = 1.0;
        Base::mesh_.property(vertex_weights_, v0)  += 1.0;
        Base::mesh_.property(vertex_weights_, v1)  += 1.0;
      }

      break;
    }


    // Cotangent weighting
    case CotWeighting:
    {
      for (e_it=Base::mesh_.edges_begin(); e_it!=e_end; ++e_it)
      {
        weight = 0.0;

        heh0   = Base::mesh_.halfedge_handle(*e_it, 0);
        v0     = Base::mesh_.to_vertex_handle(heh0);
        p0     = &Base::mesh_.point(v0);

        heh1   = Base::mesh_.halfedge_handle(*e_it, 1);
        v1     = Base::mesh_.to_vertex_handle(heh1);
        p1     = &Base::mesh_.point(v1);

        heh2   = Base::mesh_.next_halfedge_handle(heh0);
        p2     = &Base::mesh_.point(Base::mesh_.to_vertex_handle(heh2));
        d0     = (*p0 - *p2); d0.normalize();
        d1     = (*p1 - *p2); d1.normalize();
        weight += static_cast<typename Mesh::Scalar>(1.0) / tan(acos(std::max(lb, std::min(ub, dot(d0,d1) ))));

        heh2   = Base::mesh_.next_halfedge_handle(heh1);
        p2     = &Base::mesh_.point(Base::mesh_.to_vertex_handle(heh2));
        d0     = (*p0 - *p2); d0.normalize();
        d1     = (*p1 - *p2); d1.normalize();
        weight += static_cast<typename Mesh::Scalar>(1.0) / tan(acos(std::max(lb, std::min(ub, dot(d0,d1) ))));

        Base::mesh_.property(edge_weights_, *e_it) = weight;
        Base::mesh_.property(vertex_weights_, v0)  += weight;
        Base::mesh_.property(vertex_weights_, v1)  += weight;
      }
      break;
    }
  }

  
  // invert vertex weights:
  // before: sum of edge weights
  // after: one over sum of edge weights
  for (v_it=Base::mesh_.vertices_begin(); v_it!=v_end; ++v_it)
  {
    weight = Base::mesh_.property(vertex_weights_, *v_it);
    if (weight)
      Base::mesh_.property(vertex_weights_, *v_it) = static_cast<typename Mesh::Scalar>(1.0) / weight;
  }
}



//=============================================================================
} // namespace Smoother
} // namespace OpenMesh
//=============================================================================
