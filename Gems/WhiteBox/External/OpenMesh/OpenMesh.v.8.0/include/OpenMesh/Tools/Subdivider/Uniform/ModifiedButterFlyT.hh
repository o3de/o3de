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

/** \file ModifiedButterFlyT.hh

The modified butterfly scheme of Denis Zorin, Peter Schröder and Wim Sweldens, 
``Interpolating subdivision for meshes with arbitrary topology,'' in Proceedings 
of SIGGRAPH 1996, ACM SIGGRAPH, 1996, pp. 189-192.

Clement Courbet - clement.courbet@ecp.fr
*/

//=============================================================================
//
//  CLASS ModifiedButterflyT
//
//=============================================================================


#ifndef SP_MODIFIED_BUTTERFLY_H
#define SP_MODIFIED_BUTTERFLY_H

#include <OpenMesh/Tools/Subdivider/Uniform/SubdividerT.hh>
#include <OpenMesh/Core/Utils/vector_cast.hh>
#include <OpenMesh/Core/Utils/Property.hh>
// -------------------- STL
#include <vector>
#if defined(OM_CC_MIPS)
#  include <math.h>
#else
#  include <cmath>
#endif


//== NAMESPACE ================================================================

namespace OpenMesh   { // BEGIN_NS_OPENMESH
namespace Subdivider { // BEGIN_NS_DECIMATER
namespace Uniform    { // BEGIN_NS_UNIFORM


//== CLASS DEFINITION =========================================================


/** Modified Butterfly subdivision algorithm
 *
 * Implementation of the modified butterfly scheme of Denis Zorin, Peter Schröder and Wim Sweldens, 
 * ``Interpolating subdivision for meshes with arbitrary topology,'' in Proceedings 
 * of SIGGRAPH 1996, ACM SIGGRAPH, 1996, pp. 189-192.
 *
 * Clement Courbet - clement.courbet@ecp.fr
 */
template <typename MeshType, typename RealType = double>
class ModifiedButterflyT : public SubdividerT<MeshType, RealType>
{
public:

  typedef RealType                                real_t;
  typedef MeshType                                mesh_t;
  typedef SubdividerT< mesh_t, real_t >           parent_t;

  typedef std::vector< std::vector<real_t> >      weights_t;
  typedef std::vector<real_t>                     weight_t;

public:


  ModifiedButterflyT() : parent_t()
  { init_weights(); }


  ModifiedButterflyT( mesh_t& _m) : parent_t(_m)
  { init_weights(); }


  ~ModifiedButterflyT() {}


public:


  const char *name() const { return "Uniform Spectral"; }


  /// Pre-compute weights
  void init_weights(size_t _max_valence=30)
  {
    weights.resize(_max_valence);

    //special case: K==3, K==4
    weights[3].resize(4);
    weights[3][0] = real_t(5.0)/12;
    weights[3][1] = real_t(-1.0)/12;
    weights[3][2] = real_t(-1.0)/12;
    weights[3][3] = real_t(3.0)/4;

    weights[4].resize(5);
    weights[4][0] = real_t(3.0)/8;
    weights[4][1] = 0;
    weights[4][2] = real_t(-1.0)/8;
    weights[4][3] = 0;
    weights[4][4] = real_t(3.0)/4;

    for(unsigned int K = 5; K<_max_valence; ++K)
    {
        weights[K].resize(K+1);
        // s(j) = ( 1/4 + cos(2*pi*j/K) + 1/2 * cos(4*pi*j/K) )/K
        double invK  = 1.0/static_cast<double>(K);
        real_t sum = 0;
        for(unsigned int j=0; j<K; ++j)
        {
            weights[K][j] = static_cast<real_t>((0.25 + cos(2.0*M_PI*static_cast<double>(j)*invK) + 0.5*cos(4.0*M_PI*static_cast<double>(j)*invK))*invK);
            sum += weights[K][j];
        }
        weights[K][K] = static_cast<real_t>(1.0) - sum;
    }
  }


protected:


  bool prepare( mesh_t& _m )
  {
    _m.add_property( vp_pos_ );
    _m.add_property( ep_pos_ );
    return true;
  }


  bool cleanup( mesh_t& _m )
  {
    _m.remove_property( vp_pos_ );
    _m.remove_property( ep_pos_ );
    return true;
  }


  bool subdivide( MeshType& _m, size_t _n , const bool _update_points = true)
  {

    ///TODO:Implement fixed positions

    typename mesh_t::FaceIter   fit, f_end;
    typename mesh_t::EdgeIter   eit, e_end;
    typename mesh_t::VertexIter vit;

    // Do _n subdivisions
    for (size_t i=0; i < _n; ++i)
    {

      // This is an interpolating scheme, old vertices remain the same.
      typename mesh_t::VertexIter initialVerticesEnd = _m.vertices_end();
      for ( vit  = _m.vertices_begin(); vit != initialVerticesEnd; ++vit)
        _m.property( vp_pos_, *vit ) = _m.point(*vit);

      // Compute position for new vertices and store them in the edge property
      for (eit=_m.edges_begin(); eit != _m.edges_end(); ++eit)
        compute_midpoint( _m, *eit );


      // Split each edge at midpoint and store precomputed positions (stored in
      // edge property ep_pos_) in the vertex property vp_pos_;

      // Attention! Creating new edges, hence make sure the loop ends correctly.
      e_end = _m.edges_end();
      for (eit=_m.edges_begin(); eit != e_end; ++eit)
        split_edge(_m, *eit );


      // Commit changes in topology and reconsitute consistency

      // Attention! Creating new faces, hence make sure the loop ends correctly.
      f_end   = _m.faces_end();
      for (fit = _m.faces_begin(); fit != f_end; ++fit)
        split_face(_m, *fit );


      // Commit changes in geometry
      for ( vit  = /*initialVerticesEnd;*/_m.vertices_begin();
            vit != _m.vertices_end(); ++vit)
        _m.set_point(*vit, _m.property( vp_pos_, *vit ) );

#if defined(_DEBUG) || defined(DEBUG)
      // Now we have an consistent mesh!
      assert( OpenMesh::Utils::MeshCheckerT<mesh_t>(_m).check() );
#endif
    }

    return true;
  }

private: // topological modifiers

  void split_face(mesh_t& _m, const typename mesh_t::FaceHandle& _fh)
  {
    typename mesh_t::HalfedgeHandle
      heh1(_m.halfedge_handle(_fh)),
      heh2(_m.next_halfedge_handle(_m.next_halfedge_handle(heh1))),
      heh3(_m.next_halfedge_handle(_m.next_halfedge_handle(heh2)));

    // Cutting off every corner of the 6_gon
    corner_cutting( _m, heh1 );
    corner_cutting( _m, heh2 );
    corner_cutting( _m, heh3 );
  }


  void corner_cutting(mesh_t& _m, const typename mesh_t::HalfedgeHandle& _he)
  {
    // Define Halfedge Handles
    typename mesh_t::HalfedgeHandle
      heh1(_he),
      heh5(heh1),
      heh6(_m.next_halfedge_handle(heh1));

    // Cycle around the polygon to find correct Halfedge
    for (; _m.next_halfedge_handle(_m.next_halfedge_handle(heh5)) != heh1;
         heh5 = _m.next_halfedge_handle(heh5))
    {}

    typename mesh_t::VertexHandle
      vh1 = _m.to_vertex_handle(heh1),
      vh2 = _m.to_vertex_handle(heh5);

    typename mesh_t::HalfedgeHandle
      heh2(_m.next_halfedge_handle(heh5)),
      heh3(_m.new_edge( vh1, vh2)),
      heh4(_m.opposite_halfedge_handle(heh3));

    /* Intermediate result
     *
     *            *
     *         5 /|\
     *          /_  \
     *    vh2> *     *
     *        /|\3   |\
     *       /_  \|4   \
     *      *----\*----\*
     *          1 ^   6
     *            vh1 (adjust_outgoing halfedge!)
     */

    // Old and new Face
    typename mesh_t::FaceHandle     fh_old(_m.face_handle(heh6));
    typename mesh_t::FaceHandle     fh_new(_m.new_face());


    // Re-Set Handles around old Face
    _m.set_next_halfedge_handle(heh4, heh6);
    _m.set_next_halfedge_handle(heh5, heh4);

    _m.set_face_handle(heh4, fh_old);
    _m.set_face_handle(heh5, fh_old);
    _m.set_face_handle(heh6, fh_old);
    _m.set_halfedge_handle(fh_old, heh4);

    // Re-Set Handles around new Face
    _m.set_next_halfedge_handle(heh1, heh3);
    _m.set_next_halfedge_handle(heh3, heh2);

    _m.set_face_handle(heh1, fh_new);
    _m.set_face_handle(heh2, fh_new);
    _m.set_face_handle(heh3, fh_new);

    _m.set_halfedge_handle(fh_new, heh1);
  }


  void split_edge(mesh_t& _m, const typename mesh_t::EdgeHandle& _eh)
  {
    typename mesh_t::HalfedgeHandle
      heh     = _m.halfedge_handle(_eh, 0),
      opp_heh = _m.halfedge_handle(_eh, 1);

    typename mesh_t::HalfedgeHandle new_heh, opp_new_heh, t_heh;
    typename mesh_t::VertexHandle   vh;
    typename mesh_t::VertexHandle   vh1(_m.to_vertex_handle(heh));
    typename mesh_t::Point          zero(0,0,0);

    // new vertex
    vh                = _m.new_vertex( zero );

    // memorize position, will be set later
    _m.property( vp_pos_, vh ) = _m.property( ep_pos_, _eh );


    // Re-link mesh entities
    if (_m.is_boundary(_eh))
    {
      for (t_heh = heh;
           _m.next_halfedge_handle(t_heh) != opp_heh;
           t_heh = _m.opposite_halfedge_handle(_m.next_halfedge_handle(t_heh)))
      {}
    }
    else
    {
      for (t_heh = _m.next_halfedge_handle(opp_heh);
           _m.next_halfedge_handle(t_heh) != opp_heh;
           t_heh = _m.next_halfedge_handle(t_heh) )
      {}
    }

    new_heh     = _m.new_edge(vh, vh1);
    opp_new_heh = _m.opposite_halfedge_handle(new_heh);
    _m.set_vertex_handle( heh, vh );

    _m.set_next_halfedge_handle(t_heh, opp_new_heh);
    _m.set_next_halfedge_handle(new_heh, _m.next_halfedge_handle(heh));
    _m.set_next_halfedge_handle(heh, new_heh);
    _m.set_next_halfedge_handle(opp_new_heh, opp_heh);

    if (_m.face_handle(opp_heh).is_valid())
    {
      _m.set_face_handle(opp_new_heh, _m.face_handle(opp_heh));
      _m.set_halfedge_handle(_m.face_handle(opp_new_heh), opp_new_heh);
    }

    _m.set_face_handle( new_heh, _m.face_handle(heh) );
    _m.set_halfedge_handle( vh, new_heh);
    _m.set_halfedge_handle( _m.face_handle(heh), heh );
    _m.set_halfedge_handle( vh1, opp_new_heh );

    // Never forget this, when playing with the topology
    _m.adjust_outgoing_halfedge( vh );
    _m.adjust_outgoing_halfedge( vh1 );
  }

private: // geometry helper

  void compute_midpoint(mesh_t& _m, const typename mesh_t::EdgeHandle& _eh)
  {
    typename mesh_t::HalfedgeHandle heh, opp_heh;

    heh      = _m.halfedge_handle( _eh, 0);
    opp_heh  = _m.halfedge_handle( _eh, 1);

    typename mesh_t::Point pos(0,0,0);

    typename mesh_t::VertexHandle a_0(_m.to_vertex_handle(heh));
    typename mesh_t::VertexHandle a_1(_m.to_vertex_handle(opp_heh));

    // boundary edge: 4-point scheme
    if (_m.is_boundary(_eh) )
    {
        pos = _m.point(a_0);
        pos += _m.point(a_1);
        pos *= static_cast<typename mesh_t::Point::value_type>(9.0/16.0);
        typename mesh_t::Point tpos;
        if(_m.is_boundary(heh))
        {
            tpos = _m.point(_m.to_vertex_handle(_m.next_halfedge_handle(heh)));
            tpos += _m.point(_m.to_vertex_handle(_m.opposite_halfedge_handle(_m.prev_halfedge_handle(heh))));
        }
        else
        {
            assert(_m.is_boundary(opp_heh));
            tpos = _m.point(_m.to_vertex_handle(_m.next_halfedge_handle(opp_heh)));
            tpos += _m.point(_m.to_vertex_handle(_m.opposite_halfedge_handle(_m.prev_halfedge_handle(opp_heh))));
        }
        tpos *= static_cast<typename mesh_t::Point::value_type>(-1.0/16.0);
        pos += tpos;
    }
    else
    {
        int valence_a_0 = _m.valence(a_0);
        int valence_a_1 = _m.valence(a_1);
        assert(valence_a_0>2);
        assert(valence_a_1>2);

        if( (valence_a_0==6 && valence_a_1==6) || (_m.is_boundary(a_0) && valence_a_1==6) || (_m.is_boundary(a_1) && valence_a_0==6) || (_m.is_boundary(a_0) && _m.is_boundary(a_1)) )// use 8-point scheme
        {
            real_t alpha    = real_t(1.0/2);
            real_t beta     = real_t(1.0/8);
            real_t gamma    = real_t(-1.0/16);

            //get points
            typename mesh_t::VertexHandle b_0, b_1, c_0, c_1, c_2, c_3;
            typename mesh_t::HalfedgeHandle t_he;

            t_he = _m.next_halfedge_handle(_m.opposite_halfedge_handle(heh));
            b_0 = _m.to_vertex_handle(t_he);
            if(!_m.is_boundary(_m.opposite_halfedge_handle(t_he)))
            {
                t_he = _m.next_halfedge_handle(_m.opposite_halfedge_handle(t_he));
                c_0 = _m.to_vertex_handle(t_he);
            }

            t_he = _m.opposite_halfedge_handle(_m.prev_halfedge_handle(heh));
            b_1 = _m.to_vertex_handle(t_he);
            if(!_m.is_boundary(t_he))
            {
                t_he = _m.opposite_halfedge_handle(_m.prev_halfedge_handle(t_he));
                c_1 = _m.to_vertex_handle(t_he);
            }

            t_he = _m.next_halfedge_handle(_m.opposite_halfedge_handle(opp_heh));
            assert(b_1.idx()==_m.to_vertex_handle(t_he).idx());
            if(!_m.is_boundary(_m.opposite_halfedge_handle(t_he)))
            {
                t_he = _m.next_halfedge_handle(_m.opposite_halfedge_handle(t_he));
                c_2 = _m.to_vertex_handle(t_he);
            }

            t_he = _m.opposite_halfedge_handle(_m.prev_halfedge_handle(opp_heh));
            assert(b_0==_m.to_vertex_handle(t_he));
            if(!_m.is_boundary(t_he))
            {
                t_he = _m.opposite_halfedge_handle(_m.prev_halfedge_handle(t_he));
                c_3 = _m.to_vertex_handle(t_he);
            }

            //compute position.
            //a0,a1,b0,b1 must exist.
            assert(a_0.is_valid());
            assert(a_1.is_valid());
            assert(b_0.is_valid());
            assert(b_1.is_valid());
            //The other vertices may be created from symmetry is they are on the other side of the boundary.

            pos = _m.point(a_0);
            pos += _m.point(a_1);
            pos *= alpha;

            typename mesh_t::Point tpos ( _m.point(b_0) );
            tpos += _m.point(b_1);
            tpos *= beta;
            pos += tpos;

            typename mesh_t::Point pc_0, pc_1, pc_2, pc_3;
            if(c_0.is_valid())
                pc_0 = _m.point(c_0);
            else //create the point by symmetry
            {
                    pc_0 = _m.point(a_1) + _m.point(b_0) - _m.point(a_0);
            }
            if(c_1.is_valid())
                pc_1 = _m.point(c_1);
            else //create the point by symmetry
            {
                    pc_1 = _m.point(a_1) + _m.point(b_1) - _m.point(a_0);
            }
            if(c_2.is_valid())
                pc_2 = _m.point(c_2);
            else //create the point by symmetry
            {
                    pc_2 = _m.point(a_0) + _m.point(b_1) - _m.point(a_1);
            }
            if(c_3.is_valid())
                pc_3 = _m.point(c_3);
            else //create the point by symmetry
            {
                    pc_3 = _m.point(a_0) + _m.point(b_0) - _m.point(a_1);
            }
            tpos = pc_0;
            tpos += pc_1;
            tpos += pc_2;
            tpos += pc_3;
            tpos *= gamma;
            pos += tpos;
        }
        else //at least one endpoint is [irregular and not in boundary]
        {
            typename mesh_t::Point::value_type normFactor = static_cast<typename mesh_t::Point::value_type>(0.0);

            if(valence_a_0!=6 && !_m.is_boundary(a_0))
            {
                assert((int)weights[valence_a_0].size()==valence_a_0+1);
                typename mesh_t::HalfedgeHandle t_he = opp_heh;
                for(int i = 0; i < valence_a_0 ; t_he=_m.next_halfedge_handle(_m.opposite_halfedge_handle(t_he)), ++i)
                {
                    pos += weights[valence_a_0][i] * _m.point(_m.to_vertex_handle(t_he));
                }
                assert(t_he==opp_heh);

                //add irregular vertex:
                pos += weights[valence_a_0][valence_a_0] * _m.point(a_0);
                ++normFactor;
            }

            if(valence_a_1!=6  && !_m.is_boundary(a_1))
            {
                assert((int)weights[valence_a_1].size()==valence_a_1+1);
                typename mesh_t::HalfedgeHandle t_he = heh;
                for(int i = 0; i < valence_a_1 ; t_he=_m.next_halfedge_handle(_m.opposite_halfedge_handle(t_he)), ++i)
                {
                    pos += weights[valence_a_1][i] * _m.point(_m.to_vertex_handle(t_he));
                }
                assert(t_he==heh);
                //add irregular vertex:
                pos += weights[valence_a_1][valence_a_1] * _m.point(a_1);
                ++normFactor;
            }

            assert(normFactor>0.1); //normFactor should be 1 or 2

            //if both vertices are irregular, average positions:
            pos /= normFactor;
        }
    }
    _m.property( ep_pos_, _eh ) = pos;
  }

private: // data

  OpenMesh::VPropHandleT< typename mesh_t::Point > vp_pos_;
  OpenMesh::EPropHandleT< typename mesh_t::Point > ep_pos_;

  weights_t     weights;

};

} // END_NS_UNIFORM
} // END_NS_SUBDIVIDER
} // END_NS_OPENMESH
#endif

