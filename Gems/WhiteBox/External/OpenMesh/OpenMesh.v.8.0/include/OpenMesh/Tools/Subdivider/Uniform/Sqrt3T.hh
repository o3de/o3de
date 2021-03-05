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



/** \file Sqrt3T.hh
    
 */

//=============================================================================
//
//  CLASS Sqrt3T
//
//=============================================================================

#ifndef OPENMESH_SUBDIVIDER_UNIFORM_SQRT3T_HH
#define OPENMESH_SUBDIVIDER_UNIFORM_SQRT3T_HH


//== INCLUDES =================================================================

#include <OpenMesh/Core/Mesh/Handles.hh>
#include <OpenMesh/Core/System/config.hh>
#include <OpenMesh/Tools/Subdivider/Uniform/SubdividerT.hh>
#if defined(_DEBUG) || defined(DEBUG)
// Makes life lot easier, when playing/messing around with low-level topology
// changing methods of OpenMesh
#  include <OpenMesh/Tools/Utils/MeshCheckerT.hh>
#  define ASSERT_CONSISTENCY( T, m ) \
     assert(OpenMesh::Utils::MeshCheckerT<T>(m).check())
#else
#  define ASSERT_CONSISTENCY( T, m )
#endif
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
namespace Uniform    { // BEGIN_NS_DECIMATER


//== CLASS DEFINITION =========================================================


/** %Uniform Sqrt3 subdivision algorithm
 *
 *  Implementation as described in
 *
 *  L. Kobbelt, <a href="http://www-i8.informatik.rwth-aachen.de/publications/downloads/sqrt3.pdf">"Sqrt(3) subdivision"</a>, Proceedings of SIGGRAPH 2000.
 */
template <typename MeshType, typename RealType = double>
class Sqrt3T : public SubdividerT< MeshType, RealType >
{
public:

  typedef RealType                                real_t;
  typedef MeshType                                mesh_t;
  typedef SubdividerT< mesh_t, real_t >           parent_t;

  typedef std::pair< real_t, real_t >             weight_t;
  typedef std::vector< std::pair<real_t,real_t> > weights_t;

public:


  Sqrt3T(void) : parent_t(), _1over3( real_t(1.0/3.0) ), _1over27( real_t(1.0/27.0) )
  { init_weights(); }

  Sqrt3T(MeshType &_m) : parent_t(_m), _1over3( real_t(1.0/3.0) ), _1over27( real_t(1.0/27.0) )
  { init_weights(); }

  virtual ~Sqrt3T() {}


public:


  const char *name() const { return "Uniform Sqrt3"; }

  
  /// Pre-compute weights
  void init_weights(size_t _max_valence=50)
  {
    weights_.resize(_max_valence);
    std::generate(weights_.begin(), weights_.end(), compute_weight());
  }


protected:


  bool prepare( MeshType& _m )
  {
    _m.request_edge_status();
    _m.add_property( vp_pos_ );
    _m.add_property( ep_nv_ );
    _m.add_property( mp_gen_ );
    _m.property( mp_gen_ ) = 0;

    return _m.has_edge_status() && vp_pos_.is_valid() 
      &&   ep_nv_.is_valid() && mp_gen_.is_valid();
  }


  bool cleanup( MeshType& _m )
  {
    _m.release_edge_status();
    _m.remove_property( vp_pos_ );
    _m.remove_property( ep_nv_ );
    _m.remove_property( mp_gen_ );
    return true;
  }

  bool subdivide( MeshType& _m, size_t _n , const bool _update_points = true)
  {

    ///TODO:Implement fixed positions

    typename MeshType::VertexIter       vit;
    typename MeshType::VertexVertexIter vvit;
    typename MeshType::EdgeIter         eit;
    typename MeshType::FaceIter         fit;
    typename MeshType::FaceVertexIter   fvit;
    typename MeshType::VertexHandle     vh;
    typename MeshType::HalfedgeHandle   heh;
    typename MeshType::Point            pos(0,0,0), zero(0,0,0);
    size_t                            &gen = _m.property( mp_gen_ );

    for (size_t l=0; l<_n; ++l)
    {
      // tag existing edges
      for (eit=_m.edges_begin(); eit != _m.edges_end();++eit)
      {
        _m.status( *eit ).set_tagged( true );
        if ( (gen%2) && _m.is_boundary(*eit) )
          compute_new_boundary_points( _m, *eit ); // *) creates new vertices
      }

      // do relaxation of old vertices, but store new pos in property vp_pos_

      for (vit=_m.vertices_begin(); vit!=_m.vertices_end(); ++vit)
      {
        if ( _m.is_boundary(*vit) )
        {
          if ( gen%2 )
          {
            heh  = _m.halfedge_handle(*vit);
            if (heh.is_valid()) // skip isolated newly inserted vertices *)
            {
              typename OpenMesh::HalfedgeHandle 
                prev_heh = _m.prev_halfedge_handle(heh);

              assert( _m.is_boundary(heh     ) );
              assert( _m.is_boundary(prev_heh) );
            
              pos  = _m.point(_m.to_vertex_handle(heh));
              pos += _m.point(_m.from_vertex_handle(prev_heh));
              pos *= real_t(4.0);

              pos += real_t(19.0) * _m.point( *vit );
              pos *= _1over27;

              _m.property( vp_pos_, *vit ) = pos;
            }
          }
          else
            _m.property( vp_pos_, *vit ) = _m.point( *vit );
        }
        else
        {
          size_t valence=0;

          pos = zero;
          for ( vvit = _m.vv_iter(*vit); vvit.is_valid(); ++vvit)
          {
            pos += _m.point( *vvit );
            ++valence;
          }
          pos *= weights_[ valence ].second;
          pos += weights_[ valence ].first * _m.point(*vit);
          _m.property( vp_pos_, *vit ) =  pos;
        }
      }   

      // insert new vertices, but store pos in vp_pos_
      typename MeshType::FaceIter fend = _m.faces_end();
      for (fit = _m.faces_begin();fit != fend; ++fit)
      {
        if ( (gen%2) && _m.is_boundary(*fit))
        {
          boundary_split( _m, *fit );
        }
        else
        {
          fvit = _m.fv_iter( *fit );
          pos  = _m.point(  *fvit);
          pos += _m.point(*(++fvit));
          pos += _m.point(*(++fvit));
          pos *= _1over3;
          vh   = _m.add_vertex( zero );
          _m.property( vp_pos_, vh ) = pos;
          _m.split( *fit, vh );
        }
      }

      // commit new positions (now iterating over all vertices)
      for (vit=_m.vertices_begin();vit != _m.vertices_end(); ++vit)
        _m.set_point(*vit, _m.property( vp_pos_, *vit ) );
      
      // flip old edges
      for (eit=_m.edges_begin(); eit != _m.edges_end(); ++eit)
        if ( _m.status( *eit ).tagged() && !_m.is_boundary( *eit ) )
          _m.flip(*eit);

      // Now we have an consistent mesh!
      ASSERT_CONSISTENCY( MeshType, _m );

      // increase generation by one
      ++gen;
    }
    return true;
  }

private:

  /// Helper functor to compute weights for sqrt(3)-subdivision
  /// \internal
  struct compute_weight 
  {
    compute_weight() : valence(-1) { }    
    weight_t operator() (void) 
    { 
#if !defined(OM_CC_MIPS)
      using std::cos;
#endif
      if (++valence)
      {
        real_t alpha = real_t( (4.0-2.0*cos(2.0*M_PI / real_t(valence)) )/9.0 );
        return weight_t( real_t(1)-alpha, alpha/real_t(valence) );
      }
      return weight_t(real_t(0.0), real_t(0.0) );
    }    
    int valence;
  };

private:

  // Pre-compute location of new boundary points for odd generations
  // and store them in the edge property ep_nv_;
  void compute_new_boundary_points( MeshType& _m, 
                                    const typename MeshType::EdgeHandle& _eh)
  {
    assert( _m.is_boundary(_eh) );

    typename MeshType::HalfedgeHandle heh;
    typename MeshType::VertexHandle   vh1, vh2, vh3, vh4, vhl, vhr;
    typename MeshType::Point          zero(0,0,0), P1, P2, P3, P4;

    /*
    //       *---------*---------*
    //      / \       / \       / \
    //     /   \     /   \     /   \
    //    /     \   /     \   /     \
    //   /       \ /       \ /       \
    //  *---------*--#---#--*---------*
    //                
    //  ^         ^  ^   ^  ^         ^
    //  P1        P2 pl  pr P3        P4
    */
    // get halfedge pointing from P3 to P2 (outer boundary halfedge)

    heh = _m.halfedge_handle(_eh, 
                             _m.is_boundary(_m.halfedge_handle(_eh,1)));
    
    assert( _m.is_boundary( _m.next_halfedge_handle( heh ) ) );
    assert( _m.is_boundary( _m.prev_halfedge_handle( heh ) ) );

    vh1 = _m.to_vertex_handle( _m.next_halfedge_handle( heh ) );
    vh2 = _m.to_vertex_handle( heh );
    vh3 = _m.from_vertex_handle( heh );
    vh4 = _m.from_vertex_handle( _m.prev_halfedge_handle( heh ));
    
    P1  = _m.point(vh1);
    P2  = _m.point(vh2);
    P3  = _m.point(vh3);
    P4  = _m.point(vh4);
    
    vhl = _m.add_vertex(zero);
    vhr = _m.add_vertex(zero);
 
    _m.property(vp_pos_, vhl ) = (P1 + real_t(16.0f) * P2 + real_t(10.0f) * P3) * _1over27;
    _m.property(vp_pos_, vhr ) = ( real_t(10.0f) * P2 + real_t(16.0f) * P3 + P4) * _1over27;
    _m.property(ep_nv_, _eh).first  = vhl;
    _m.property(ep_nv_, _eh).second = vhr; 
  }


  void boundary_split( MeshType& _m, const typename MeshType::FaceHandle& _fh )
  {
    assert( _m.is_boundary(_fh) );

    typename MeshType::VertexHandle     vhl, vhr;
    typename MeshType::FaceEdgeIter     fe_it;
    typename MeshType::HalfedgeHandle   heh;

    // find boundary edge
    for( fe_it=_m.fe_iter( _fh ); fe_it.is_valid() && !_m.is_boundary( *fe_it ); ++fe_it ) {};

    // use precomputed, already inserted but not linked vertices
    vhl = _m.property(ep_nv_, *fe_it).first;
    vhr = _m.property(ep_nv_, *fe_it).second;

    /*
    //       *---------*---------*
    //      / \       / \       / \
    //     /   \     /   \     /   \
    //    /     \   /     \   /     \
    //   /       \ /       \ /       \
    //  *---------*--#---#--*---------*
    //                
    //  ^         ^  ^   ^  ^         ^
    //  P1        P2 pl  pr P3        P4
    */
    // get halfedge pointing from P2 to P3 (inner boundary halfedge)

    heh = _m.halfedge_handle(*fe_it,
                             _m.is_boundary(_m.halfedge_handle(*fe_it,0)));

    typename MeshType::HalfedgeHandle pl_P3;

    // split P2->P3 (heh) in P2->pl (heh) and pl->P3
    boundary_split( _m, heh, vhl );         // split edge
    pl_P3 = _m.next_halfedge_handle( heh ); // store next halfedge handle
    boundary_split( _m, heh );              // split face

    // split pl->P3 in pl->pr and pr->P3
    boundary_split( _m, pl_P3, vhr );
    boundary_split( _m, pl_P3 );

    assert( _m.is_boundary( vhl ) && _m.halfedge_handle(vhl).is_valid() );
    assert( _m.is_boundary( vhr ) && _m.halfedge_handle(vhr).is_valid() );
  }

  void boundary_split(MeshType& _m, 
                      const typename MeshType::HalfedgeHandle& _heh,
                      const typename MeshType::VertexHandle& _vh)
  {
    assert( _m.is_boundary( _m.edge_handle(_heh) ) );

    typename MeshType::HalfedgeHandle 
      heh(_heh),
      opp_heh( _m.opposite_halfedge_handle(_heh) ),
      new_heh, opp_new_heh;
    typename MeshType::VertexHandle   to_vh(_m.to_vertex_handle(heh));
    typename MeshType::HalfedgeHandle t_heh;
    
    /*
     *            P5
     *             *
     *            /|\
     *           /   \
     *          /     \
     *         /       \
     *        /         \
     *       /_ heh  new \
     *      *-----\*-----\*\-----*
     *             ^      ^ t_heh
     *            _vh     to_vh
     *
     *     P1     P2     P3     P4
     */
    // Re-Setting Handles
    
    // find halfedge point from P4 to P3
    for(t_heh = heh; 
        _m.next_halfedge_handle(t_heh) != opp_heh; 
        t_heh = _m.opposite_halfedge_handle(_m.next_halfedge_handle(t_heh)))
    {}
    
    assert( _m.is_boundary( t_heh ) );

    new_heh     = _m.new_edge( _vh, to_vh );
    opp_new_heh = _m.opposite_halfedge_handle(new_heh);

    // update halfedge connectivity

    _m.set_next_halfedge_handle(t_heh,   opp_new_heh); // P4-P3 -> P3-P2
    // P2-P3 -> P3-P5
    _m.set_next_halfedge_handle(new_heh, _m.next_halfedge_handle(heh));    
    _m.set_next_halfedge_handle(heh,         new_heh); // P1-P2 -> P2-P3
    _m.set_next_halfedge_handle(opp_new_heh, opp_heh); // P3-P2 -> P2-P1

    // both opposite halfedges point to same face
    _m.set_face_handle(opp_new_heh, _m.face_handle(opp_heh));

    // let heh finally point to new inserted vertex
    _m.set_vertex_handle(heh, _vh); 

    // let heh and new_heh point to same face
    _m.set_face_handle(new_heh, _m.face_handle(heh));

    // let opp_new_heh be the new outgoing halfedge for to_vh 
    // (replaces for opp_heh)
    _m.set_halfedge_handle( to_vh, opp_new_heh );

    // let opp_heh be the outgoing halfedge for _vh
    _m.set_halfedge_handle( _vh, opp_heh );
  }

  void boundary_split( MeshType& _m, 
                       const typename MeshType::HalfedgeHandle& _heh)
  {
    assert( _m.is_boundary( _m.opposite_halfedge_handle( _heh ) ) );

    typename MeshType::HalfedgeHandle 
      heh(_heh),
      n_heh(_m.next_halfedge_handle(heh));

    typename MeshType::VertexHandle   
      to_vh(_m.to_vertex_handle(heh));

    typename MeshType::HalfedgeHandle 
      heh2(_m.new_edge(to_vh,
                       _m.to_vertex_handle(_m.next_halfedge_handle(n_heh)))),
      heh3(_m.opposite_halfedge_handle(heh2));

    typename MeshType::FaceHandle
      new_fh(_m.new_face()),
      fh(_m.face_handle(heh));
    
    // Relink (half)edges    

#define set_next_heh set_next_halfedge_handle
#define next_heh next_halfedge_handle

    _m.set_face_handle(heh,  new_fh);
    _m.set_face_handle(heh2, new_fh);
    _m.set_next_heh(heh2, _m.next_heh(_m.next_heh(n_heh)));
    _m.set_next_heh(heh,  heh2);
    _m.set_face_handle( _m.next_heh(heh2), new_fh);

    // _m.set_face_handle( _m.next_heh(_m.next_heh(heh2)), new_fh);

    _m.set_next_heh(heh3,                           n_heh);
    _m.set_next_heh(_m.next_halfedge_handle(n_heh), heh3);
    _m.set_face_handle(heh3,  fh);
    // _m.set_face_handle(n_heh, fh);

    _m.set_halfedge_handle(    fh, n_heh);
    _m.set_halfedge_handle(new_fh, heh);

#undef set_next_halfedge_handle
#undef next_halfedge_handle

  }

private:

  weights_t     weights_;
  OpenMesh::VPropHandleT< typename MeshType::Point >                    vp_pos_;
  OpenMesh::EPropHandleT< std::pair< typename MeshType::VertexHandle,
                                     typename MeshType::VertexHandle> > ep_nv_;
  OpenMesh::MPropHandleT< size_t >                                      mp_gen_;

  const real_t _1over3;
  const real_t _1over27;
};


//=============================================================================
} // END_NS_UNIFORM
} // END_NS_SUBDIVIDER
} // END_NS_OPENMESH
//=============================================================================
#endif // OPENMESH_SUBDIVIDER_UNIFORM_SQRT3T_HH
//=============================================================================
