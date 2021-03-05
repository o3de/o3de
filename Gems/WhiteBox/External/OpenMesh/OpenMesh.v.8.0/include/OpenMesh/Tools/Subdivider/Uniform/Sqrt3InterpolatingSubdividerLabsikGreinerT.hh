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

/** \file Sqrt3InterpolatingSubdividerLabsikGreinerT.hh
 *
 * Interpolating Labsik Greiner Subdivider as described in 
 * "Interpolating sqrt(3) subdivision" Labsik & Greiner, 2000
 *
 * Clement Courbet - clement.courbet@ecp.fr
 *
*/

//=============================================================================
//
//  CLASS InterpolatingSqrt3LGT
//
//=============================================================================

#ifndef OPENMESH_SUBDIVIDER_UNIFORM_INTERP_SQRT3T_LABSIK_GREINER_HH
#define OPENMESH_SUBDIVIDER_UNIFORM_INTERP_SQRT3T_LABSIK_GREINER_HH


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

//#define MIRROR_TRIANGLES
//#define MIN_NORM

//== NAMESPACE ================================================================

namespace OpenMesh   { // BEGIN_NS_OPENMESH
namespace Subdivider { // BEGIN_NS_DECIMATER
namespace Uniform    { // BEGIN_NS_UNIFORM


//== CLASS DEFINITION =========================================================


/** %Uniform Interpolating Sqrt3 subdivision algorithm
 *
 * Implementation of the interpolating Labsik Greiner Subdivider as described in 
 * "interpolating sqrt(3) subdivision" Labsik & Greiner, 2000
 *
 * Clement Courbet - clement.courbet@ecp.fr
*/

template <typename MeshType, typename RealType = double>
class InterpolatingSqrt3LGT : public SubdividerT< MeshType, RealType >
{
public:

  typedef RealType                                real_t;
  typedef MeshType                                mesh_t;
  typedef SubdividerT< mesh_t, real_t >           parent_t;

  typedef std::vector< std::vector<real_t> >      weights_t;

public:


  InterpolatingSqrt3LGT(void) : parent_t()
  { init_weights(); }

  InterpolatingSqrt3LGT(MeshType &_m) : parent_t(_m)
  { init_weights(); }

  virtual ~InterpolatingSqrt3LGT() {}


public:


  const char *name() const { return "Uniform Interpolating Sqrt3"; }

  /// Pre-compute weights
  void init_weights(size_t _max_valence=50)
  {
    weights_.resize(_max_valence);

    weights_[3].resize(4);
    weights_[3][0] = real_t(+4.0/27);
    weights_[3][1] = real_t(-5.0/27);
    weights_[3][2] = real_t(+4.0/27);
    weights_[3][3] = real_t(+8.0/9);

    weights_[4].resize(5);
    weights_[4][0] = real_t(+2.0/9);
    weights_[4][1] = real_t(-1.0/9);
    weights_[4][2] = real_t(-1.0/9);
    weights_[4][3] = real_t(+2.0/9);
    weights_[4][4] = real_t(+7.0/9);

    for(unsigned int K=5; K<_max_valence; ++K)
    {
        weights_[K].resize(K+1);
        double aH = 2.0*cos(M_PI/static_cast<double>(K))/3.0;
        weights_[K][K] = static_cast<real_t>(1.0 - aH*aH);
        for(unsigned int i=0; i<K; ++i)
        {

            weights_[K][i] = static_cast<real_t>((aH*aH + 2.0*aH*cos(2.0*static_cast<double>(i)*M_PI/static_cast<double>(K) + M_PI/static_cast<double>(K)) +
                                      2.0*aH*aH*cos(4.0*static_cast<double>(i)*M_PI/static_cast<double>(K) + 2.0*M_PI/static_cast<double>(K)))/static_cast<double>(K));
        }
    }

    //just to be sure:
    weights_[6].resize(0);

  }


protected:


  bool prepare( MeshType& _m )
  {
    _m.request_edge_status();
    _m.add_property( fp_pos_ );
    _m.add_property( ep_nv_ );
    _m.add_property( mp_gen_ );
    _m.property( mp_gen_ ) = 0;

    return _m.has_edge_status() 
      &&   ep_nv_.is_valid() && mp_gen_.is_valid();
  }


  bool cleanup( MeshType& _m )
  {
    _m.release_edge_status();
    _m.remove_property( fp_pos_ );
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
    typename MeshType::FaceHalfedgeIter fheit;
    typename MeshType::VertexHandle     vh;
    typename MeshType::HalfedgeHandle   heh;
    typename MeshType::Point            pos(0,0,0), zero(0,0,0);
    size_t                              &gen = _m.property( mp_gen_ );

    for (size_t l=0; l<_n; ++l)
    {
      // tag existing edges
      for (eit=_m.edges_begin(); eit != _m.edges_end();++eit)
      {
        _m.status( *eit ).set_tagged( true );
        if ( (gen%2) && _m.is_boundary(*eit) )
          compute_new_boundary_points( _m, *eit ); // *) creates new vertices
      }

      // insert new vertices, and store pos in vp_pos_
      typename MeshType::FaceIter fend = _m.faces_end();
      for (fit = _m.faces_begin();fit != fend; ++fit)
      {
        if (_m.is_boundary(*fit))
        {
            if(gen%2)
                _m.property(fp_pos_, *fit).invalidate();
            else
            {
                //find the interior boundary halfedge
                for( heh = _m.halfedge_handle(*fit); !_m.is_boundary( _m.opposite_halfedge_handle(heh) ); heh = _m.next_halfedge_handle(heh) )
                  ;
                assert(_m.is_boundary( _m.opposite_halfedge_handle(heh) ));
                pos = zero;
                //check for two boundaries case:
                if( _m.is_boundary(_m.next_halfedge_handle(heh)) || _m.is_boundary(_m.prev_halfedge_handle(heh)) )
                {
                    if(_m.is_boundary(_m.prev_halfedge_handle(heh)))
                        heh = _m.prev_halfedge_handle(heh); //ensure that the boundary halfedges are heh and heh->next
                    //check for three boundaries case:
                    if(_m.is_boundary(_m.next_halfedge_handle(_m.next_halfedge_handle(heh))))
                    {
                        //three boundaries, use COG of triangle
                        pos += real_t(1.0/3) * _m.point(_m.to_vertex_handle(heh));
                        pos += real_t(1.0/3) * _m.point(_m.to_vertex_handle(_m.next_halfedge_handle(heh)));
                        pos += real_t(1.0/3) * _m.point(_m.to_vertex_handle(_m.prev_halfedge_handle(heh)));
                    }
                    else
                    {
#ifdef MIRROR_TRIANGLES
                        //two boundaries, mirror two triangles
                        pos += real_t(2.0/9) * _m.point(_m.to_vertex_handle(heh));
                        pos += real_t(4.0/9) * _m.point(_m.to_vertex_handle(_m.next_halfedge_handle(heh)));
                        pos += real_t(4.0/9) * _m.point(_m.to_vertex_handle(_m.prev_halfedge_handle(heh)));
                        pos += real_t(-1.0/9) * _m.point(_m.to_vertex_handle(_m.next_halfedge_handle(_m.opposite_halfedge_handle(_m.prev_halfedge_handle(heh)))));
#else
                        pos += real_t(7.0/24) * _m.point(_m.to_vertex_handle(heh));
                        pos += real_t(3.0/8) * _m.point(_m.to_vertex_handle(_m.next_halfedge_handle(heh)));
                        pos += real_t(3.0/8) * _m.point(_m.to_vertex_handle(_m.prev_halfedge_handle(heh)));
                        pos += real_t(-1.0/24) * _m.point(_m.to_vertex_handle(_m.next_halfedge_handle(_m.opposite_halfedge_handle(_m.prev_halfedge_handle(heh)))));
#endif
                    }
                }
                else
                {
                    vh = _m.to_vertex_handle(_m.next_halfedge_handle(heh));
                    //check last vertex regularity
                    if((_m.valence(vh) == 6) || _m.is_boundary(vh))
                    {
#ifdef MIRROR_TRIANGLES
                        //use regular rule and mirror one triangle
                        pos += real_t(5.0/9) * _m.point(vh);
                        pos += real_t(3.0/9) * _m.point(_m.to_vertex_handle(heh));
                        pos += real_t(3.0/9) * _m.point(_m.to_vertex_handle(_m.opposite_halfedge_handle(heh)));
                        pos += real_t(-1.0/9) * _m.point(_m.to_vertex_handle(_m.next_halfedge_handle(_m.opposite_halfedge_handle(_m.next_halfedge_handle(heh)))));
                        pos += real_t(-1.0/9) * _m.point(_m.to_vertex_handle(_m.next_halfedge_handle(_m.opposite_halfedge_handle(_m.prev_halfedge_handle(heh)))));
#else
#ifdef MIN_NORM
                        pos += real_t(1.0/9) * _m.point(vh);
                        pos += real_t(1.0/3) * _m.point(_m.to_vertex_handle(heh));
                        pos += real_t(1.0/3) * _m.point(_m.to_vertex_handle(_m.opposite_halfedge_handle(heh)));
                        pos += real_t(1.0/9) * _m.point(_m.to_vertex_handle(_m.next_halfedge_handle(_m.opposite_halfedge_handle(_m.next_halfedge_handle(heh)))));
                        pos += real_t(1.0/9) * _m.point(_m.to_vertex_handle(_m.next_halfedge_handle(_m.opposite_halfedge_handle(_m.prev_halfedge_handle(heh)))));
#else
                        pos += real_t(1.0/2) * _m.point(vh);
                        pos += real_t(1.0/3) * _m.point(_m.to_vertex_handle(heh));
                        pos += real_t(1.0/3) * _m.point(_m.to_vertex_handle(_m.opposite_halfedge_handle(heh)));
                        pos += real_t(-1.0/12) * _m.point(_m.to_vertex_handle(_m.next_halfedge_handle(_m.opposite_halfedge_handle(_m.next_halfedge_handle(heh)))));
                        pos += real_t(-1.0/12) * _m.point(_m.to_vertex_handle(_m.next_halfedge_handle(_m.opposite_halfedge_handle(_m.prev_halfedge_handle(heh)))));
#endif
#endif
                    }
                    else
                    {
                        //irregular setting, use usual irregular rule
                        unsigned int K = _m.valence(vh);
                        pos += weights_[K][K]*_m.point(vh);
                        heh = _m.opposite_halfedge_handle( _m.next_halfedge_handle(heh) );
                        for(unsigned int i = 0; i<K; ++i, heh = _m.opposite_halfedge_handle(_m.prev_halfedge_handle(heh)) )
                        {
                            pos += weights_[K][i]*_m.point(_m.to_vertex_handle(heh));
                        }
                    }
                }
                vh   = _m.add_vertex( pos );
                _m.property(fp_pos_, *fit) = vh;
            }
        }
        else
        {
            pos = zero;
            int nOrdinary = 0;
            
            //check number of extraordinary vertices
            for(fvit = _m.fv_iter( *fit ); fvit.is_valid(); ++fvit)
                if( (_m.valence(*fvit)) == 6 || _m.is_boundary(*fvit) )
                    ++nOrdinary;

            if(nOrdinary==3)
            {
                for(fheit = _m.fh_iter( *fit ); fheit.is_valid(); ++fheit)
                {
                    //one ring vertex has weight 32/81
                    heh = *fheit;
                    assert(_m.to_vertex_handle(heh).is_valid());
                    pos += real_t(32.0/81) * _m.point(_m.to_vertex_handle(heh));
                    //tip vertex has weight -1/81
                    heh = _m.opposite_halfedge_handle(heh);
                    assert(heh.is_valid());
                    assert(_m.next_halfedge_handle(heh).is_valid());
                    assert(_m.to_vertex_handle(_m.next_halfedge_handle(heh)).is_valid());
                    pos -= real_t(1.0/81) * _m.point(_m.to_vertex_handle(_m.next_halfedge_handle(heh)));
                    //outer vertices have weight -2/81
                    heh = _m.opposite_halfedge_handle(_m.prev_halfedge_handle(heh));
                    assert(heh.is_valid());
                    assert(_m.next_halfedge_handle(heh).is_valid());
                    assert(_m.to_vertex_handle(_m.next_halfedge_handle(heh)).is_valid());
                    pos -= real_t(2.0/81) * _m.point(_m.to_vertex_handle(_m.next_halfedge_handle(heh)));
                    heh = _m.opposite_halfedge_handle(_m.prev_halfedge_handle(heh));
                    assert(heh.is_valid());
                    assert(_m.next_halfedge_handle(heh).is_valid());
                    assert(_m.to_vertex_handle(_m.next_halfedge_handle(heh)).is_valid());
                    pos -= real_t(2.0/81) * _m.point(_m.to_vertex_handle(_m.next_halfedge_handle(heh)));
                }
            }
            else
            {
                //only use irregular vertices:
                for(fheit = _m.fh_iter( *fit ); fheit.is_valid(); ++fheit)
                {
                    vh = _m.to_vertex_handle(*fheit);
                    if( (_m.valence(vh) != 6) && (!_m.is_boundary(vh)) )
                    {
                        unsigned int K = _m.valence(vh);
                        pos += weights_[K][K]*_m.point(vh);
                        heh = _m.opposite_halfedge_handle( *fheit );
                        for(unsigned int i = 0; i<K; ++i, heh = _m.opposite_halfedge_handle(_m.prev_halfedge_handle(heh)) )
                        {
                            pos += weights_[K][i]*_m.point(_m.to_vertex_handle(heh));
                        }
                    }
                }
                pos *= real_t(1.0/(3-nOrdinary));
            }

            vh   = _m.add_vertex( pos );
            _m.property(fp_pos_, *fit) = vh;
        }
      }

      //split faces
      for (fit = _m.faces_begin();fit != fend; ++fit)
      {
        if ( _m.is_boundary(*fit) && (gen%2))
        {
                boundary_split( _m, *fit );
        }
        else
        {
            assert(_m.property(fp_pos_, *fit).is_valid());
            _m.split( *fit, _m.property(fp_pos_, *fit) );
        }
      }

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

    vhl = _m.add_vertex(real_t(-5.0/81)*P1 + real_t(20.0/27)*P2 + real_t(10.0/27)*P3 + real_t(-4.0/81)*P4);
    vhr = _m.add_vertex(real_t(-5.0/81)*P4 + real_t(20.0/27)*P3 + real_t(10.0/27)*P2 + real_t(-4.0/81)*P1);

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

    heh = _m.halfedge_handle(*fe_it, _m.is_boundary(_m.halfedge_handle(*fe_it,0)));

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
    _m.set_next_halfedge_handle(t_heh,   opp_new_heh);                  // P4-P3 -> P3-P2
    _m.set_next_halfedge_handle(new_heh, _m.next_halfedge_handle(heh)); // P2-P3 -> P3-P5   
    _m.set_next_halfedge_handle(heh,         new_heh);                  // P1-P2 -> P2-P3
    _m.set_next_halfedge_handle(opp_new_heh, opp_heh);                  // P3-P2 -> P2-P1

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
    _m.set_face_handle(heh,  new_fh);
    _m.set_face_handle(heh2, new_fh);
    _m.set_next_halfedge_handle(heh2, _m.next_halfedge_handle(_m.next_halfedge_handle(n_heh)));
    _m.set_next_halfedge_handle(heh,  heh2);
    _m.set_face_handle( _m.next_halfedge_handle(heh2), new_fh);

    _m.set_next_halfedge_handle(heh3,                           n_heh);
    _m.set_next_halfedge_handle(_m.next_halfedge_handle(n_heh), heh3);
    _m.set_face_handle(heh3,  fh);

    _m.set_halfedge_handle(    fh, n_heh);
    _m.set_halfedge_handle(new_fh, heh);


  }

private:

  weights_t     weights_;
  OpenMesh::FPropHandleT< typename MeshType::VertexHandle >             fp_pos_;
  OpenMesh::EPropHandleT< std::pair< typename MeshType::VertexHandle,
                                     typename MeshType::VertexHandle> > ep_nv_;
  OpenMesh::MPropHandleT< size_t >                                      mp_gen_;
};


//=============================================================================
} // END_NS_UNIFORM
} // END_NS_SUBDIVIDER
} // END_NS_OPENMESH
//=============================================================================
#endif // OPENMESH_SUBDIVIDER_UNIFORM_SQRT3T_HH
//=============================================================================
