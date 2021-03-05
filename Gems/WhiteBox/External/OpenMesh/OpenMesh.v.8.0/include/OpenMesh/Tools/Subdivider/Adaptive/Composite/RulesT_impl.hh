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



/** \file RulesT_impl.hh
    
 */

//=============================================================================
//
//  Rules - IMPLEMENTATION
//
//=============================================================================


#define OPENMESH_SUBDIVIDER_ADAPTIVE_RULEST_CC


//== INCLUDES =================================================================

#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/IO/MeshIO.hh>
#include "RulesT.hh"
// --------------------
#if defined(OM_CC_MIPS)
#  include <math.h>
#else
#  include <cmath>
#endif

#if defined(OM_CC_MSVC)
#  pragma warning(disable:4244)
#endif

//== NAMESPACE ================================================================

namespace OpenMesh   { // BEGIN_NS_OPENMESH
namespace Subdivider { // BEGIN_NS_DECIMATER
namespace Adaptive   { // BEGIN_NS_ADAPTIVE


//== IMPLEMENTATION ========================================================== 

#define MOBJ Base::mesh_.data
#define FH face_handle
#define VH vertex_handle
#define EH edge_handle
#define HEH halfedge_handle
#define NHEH next_halfedge_handle
#define PHEH prev_halfedge_handle
#define OHEH opposite_halfedge_handle
#define TVH  to_vertex_handle
#define FVH  from_vertex_handle

// ------------------------------------------------------------------ Tvv3 ----


template<class M>
void
Tvv3<M>::raise(typename M::FaceHandle& _fh, state_t _target_state) 
{
  if (MOBJ(_fh).state() < _target_state) 
  {
    this->update(_fh, _target_state);

    typename M::VertexVertexIter          vv_it;
    typename M::FaceVertexIter            fv_it;
    typename M::VertexHandle              vh;
    typename M::Point                     position(0.0, 0.0, 0.0);
    typename M::Point                     face_position;
    const typename M::Point               zero_point(0.0, 0.0, 0.0);
    std::vector<typename M::VertexHandle> vertex_vector;


    // raise all adjacent vertices to level x-1
    for (fv_it = Base::mesh_.fv_iter(_fh); fv_it.is_valid(); ++fv_it) {

      vertex_vector.push_back(*fv_it);
    }

    while(!vertex_vector.empty()) {

      vh = vertex_vector.back();
      vertex_vector.pop_back();

      if (_target_state > 1)
        Base::prev_rule()->raise(vh, _target_state - 1);
    }

    face_position = MOBJ(_fh).position(_target_state - 1);
    
    typename M::EdgeHandle               eh;
    std::vector<typename M::EdgeHandle>  edge_vector;

    // interior face
    if (!Base::mesh_.is_boundary(_fh) || MOBJ(_fh).final()) { 

      // insert new vertex
      vh = Base::mesh_.new_vertex();

      Base::mesh_.split(_fh, vh);

      typename M::Scalar valence(0.0);

      // calculate display position for new vertex
      for (vv_it = Base::mesh_.vv_iter(vh); vv_it.is_valid(); ++vv_it)
      {
        position += Base::mesh_.point(*vv_it);
        valence += 1.0;
      }

      position /= valence;

      // set attributes for new vertex
      Base::mesh_.set_point(vh, position);
      MOBJ(vh).set_position(_target_state, zero_point);
      MOBJ(vh).set_state(_target_state);
      MOBJ(vh).set_not_final();

      typename M::VertexOHalfedgeIter      voh_it;
      // check for edge flipping
      for (voh_it = Base::mesh_.voh_iter(vh); voh_it.is_valid(); ++voh_it) {

        if (Base::mesh_.FH(*voh_it).is_valid()) {

          MOBJ(Base::mesh_.FH(*voh_it)).set_state(_target_state);
          MOBJ(Base::mesh_.FH(*voh_it)).set_not_final();
          MOBJ(Base::mesh_.FH(*voh_it)).set_position(_target_state - 1, face_position);


          for (state_t j = 0; j < _target_state; ++j) {
            MOBJ(Base::mesh_.FH(*voh_it)).set_position(j, MOBJ(_fh).position(j));
          }

          if (Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(*voh_it))).is_valid()) {

            if (MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(*voh_it)))).state() == _target_state) {

              if (Base::mesh_.is_flip_ok(Base::mesh_.EH(Base::mesh_.NHEH(*voh_it)))) {

                edge_vector.push_back(Base::mesh_.EH(Base::mesh_.NHEH(*voh_it)));
              }
            }
          }
        }
      }
    }
    
    // boundary face
    else { 

      typename M::VertexHandle vh1 = Base::mesh_.new_vertex(),
	                          vh2 = Base::mesh_.new_vertex();
      
      typename M::HalfedgeHandle hh2 = Base::mesh_.HEH(_fh),
                                    hh1, hh3;
      
      while (!Base::mesh_.is_boundary(Base::mesh_.OHEH(hh2)))
	hh2 = Base::mesh_.NHEH(hh2);
      
      eh = Base::mesh_.EH(hh2);
      
      hh2 = Base::mesh_.NHEH(hh2);
      hh1 = Base::mesh_.NHEH(hh2);
      
      assert(Base::mesh_.is_boundary(eh));

      Base::mesh_.split(eh, vh1);

      eh = Base::mesh_.EH(Base::mesh_.PHEH(hh2));

      assert(Base::mesh_.is_boundary(eh));

      Base::mesh_.split(eh, vh2);

      hh3 = Base::mesh_.NHEH(Base::mesh_.OHEH(Base::mesh_.PHEH(hh1)));

      typename M::VertexHandle   vh0(Base::mesh_.TVH(hh1)),
	                            vh3(Base::mesh_.FVH(hh2));

      // set display position and attributes for new vertices
      Base::mesh_.set_point(vh1, (Base::mesh_.point(vh0) * static_cast<typename M::Point::value_type>(2.0) + Base::mesh_.point(vh3)) / static_cast<typename M::Point::value_type>(3.0) );

      MOBJ(vh1).set_position(_target_state, zero_point);
      MOBJ(vh1).set_state(_target_state);
      MOBJ(vh1).set_not_final();

      MOBJ(vh0).set_position(_target_state, MOBJ(vh0).position(_target_state - 1) *  static_cast<typename M::Point::value_type>(3.0));
      MOBJ(vh0).set_state(_target_state);
      MOBJ(vh0).set_not_final();

      // set display position and attributes for old vertices
      Base::mesh_.set_point(vh2, (Base::mesh_.point(vh3) * static_cast<typename M::Point::value_type>(2.0) + Base::mesh_.point(vh0)) / static_cast<typename M::Point::value_type>(3.0) );
      MOBJ(vh2).set_position(_target_state, zero_point);
      MOBJ(vh2).set_state(_target_state);
      MOBJ(vh2).set_not_final();

      MOBJ(vh3).set_position(_target_state, MOBJ(vh3).position(_target_state - 1) *  static_cast<typename M::Point::value_type>(3.0) );
      MOBJ(vh3).set_state(_target_state);
      MOBJ(vh3).set_not_final();

      // init 3 faces
      MOBJ(Base::mesh_.FH(hh1)).set_state(_target_state);
      MOBJ(Base::mesh_.FH(hh1)).set_not_final();
      MOBJ(Base::mesh_.FH(hh1)).set_position(_target_state - 1, face_position);
      
      MOBJ(Base::mesh_.FH(hh2)).set_state(_target_state);
      MOBJ(Base::mesh_.FH(hh2)).set_not_final();
      MOBJ(Base::mesh_.FH(hh2)).set_position(_target_state - 1, face_position);

      MOBJ(Base::mesh_.FH(hh3)).set_state(_target_state);
      MOBJ(Base::mesh_.FH(hh3)).set_final();
      MOBJ(Base::mesh_.FH(hh3)).set_position(_target_state - 1, face_position);
      

      for (state_t j = 0; j < _target_state; ++j) {
	MOBJ(Base::mesh_.FH(hh1)).set_position(j, MOBJ(_fh).position(j));
      }
      
      for (state_t j = 0; j < _target_state; ++j) {

	MOBJ(Base::mesh_.FH(hh2)).set_position(j, MOBJ(_fh).position(j));
      }
      
      for (state_t j = 0; j < _target_state; ++j) {

	MOBJ(Base::mesh_.FH(hh3)).set_position(j, MOBJ(_fh).position(j));
      }
      
      // check for edge flipping
      if (Base::mesh_.FH(Base::mesh_.OHEH(hh1)).is_valid()) {

	if (MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(hh1))).state() == _target_state) {

	  if (Base::mesh_.is_flip_ok(Base::mesh_.EH(hh1))) {

	    edge_vector.push_back(Base::mesh_.EH(hh1));
	  }
	}
      }

      if (Base::mesh_.FH(Base::mesh_.OHEH(hh2)).is_valid()) {

	if (MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(hh2))).state() == _target_state) {

	  if (Base::mesh_.is_flip_ok(Base::mesh_.EH(hh2))) {

	    edge_vector.push_back(Base::mesh_.EH(hh2));
	  }
	}
      }
    }
  
    // flip edges
    while (!edge_vector.empty()) {

      eh = edge_vector.back();
      edge_vector.pop_back();
      
      assert(Base::mesh_.is_flip_ok(eh));

      Base::mesh_.flip(eh);

      MOBJ(Base::mesh_.FH(Base::mesh_.HEH(eh, 0))).set_final();
      MOBJ(Base::mesh_.FH(Base::mesh_.HEH(eh, 1))).set_final();

      MOBJ(Base::mesh_.FH(Base::mesh_.HEH(eh, 0))).set_state(_target_state);
      MOBJ(Base::mesh_.FH(Base::mesh_.HEH(eh, 1))).set_state(_target_state);

      MOBJ(Base::mesh_.FH(Base::mesh_.HEH(eh, 0))).set_position(_target_state, face_position);
      MOBJ(Base::mesh_.FH(Base::mesh_.HEH(eh, 1))).set_position(_target_state, face_position);
    }
  }
}


template<class M>
void Tvv3<M>::raise(typename M::VertexHandle& _vh, state_t _target_state) {

  if (MOBJ(_vh).state() < _target_state) {

    this->update(_vh, _target_state);

    // multiply old position by 3
    MOBJ(_vh).set_position(_target_state, MOBJ(_vh).position(_target_state - 1) *  static_cast<typename M::Point::value_type>(3.0) );

    MOBJ(_vh).inc_state();

    assert(MOBJ(_vh).state() == _target_state);
  }
}


// ------------------------------------------------------------------ Tvv4 ----


template<class M>
void
Tvv4<M>::raise(typename M::FaceHandle& _fh, state_t _target_state) 
{

  if (MOBJ(_fh).state() < _target_state) {

    this->update(_fh, _target_state);

    typename M::FaceVertexIter              fv_it;
    typename M::VertexHandle                temp_vh;
    typename M::Point                       face_position;
    const typename M::Point                 zero_point(0.0, 0.0, 0.0);
    std::vector<typename M::VertexHandle>   vertex_vector;
    std::vector<typename M::HalfedgeHandle> halfedge_vector;

    // raise all adjacent vertices to level x-1
    for (fv_it = Base::mesh_.fv_iter(_fh); fv_it.is_valid(); ++fv_it) {

      vertex_vector.push_back(*fv_it);
    }

    while(!vertex_vector.empty()) {

      temp_vh = vertex_vector.back();
      vertex_vector.pop_back();

      if (_target_state > 1) {
        Base::prev_rule()->raise(temp_vh, _target_state - 1);
      }
    }

    face_position = MOBJ(_fh).position(_target_state - 1);

    typename M::HalfedgeHandle hh[3];
    typename M::VertexHandle   vh[3];
    typename M::VertexHandle   new_vh[3];
    typename M::FaceHandle     fh[4];
    typename M::EdgeHandle     eh;
    typename M::HalfedgeHandle temp_hh;

    // normal (final) face
    if (MOBJ(_fh).final()) {

      // define three halfedge handles around the face
      hh[0] = Base::mesh_.HEH(_fh);
      hh[1] = Base::mesh_.NHEH(hh[0]);
      hh[2] = Base::mesh_.NHEH(hh[1]);

      assert(hh[0] == Base::mesh_.NHEH(hh[2]));

      vh[0] = Base::mesh_.TVH(hh[0]);
      vh[1] = Base::mesh_.TVH(hh[1]);
      vh[2] = Base::mesh_.TVH(hh[2]);
      
      new_vh[0] = Base::mesh_.add_vertex(zero_point);
      new_vh[1] = Base::mesh_.add_vertex(zero_point);
      new_vh[2] = Base::mesh_.add_vertex(zero_point);

      // split three edges
      split_edge(hh[0], new_vh[0], _target_state);
      eh = Base::mesh_.EH(Base::mesh_.PHEH(hh[2]));
      split_edge(hh[1], new_vh[1], _target_state);
      split_edge(hh[2], new_vh[2], _target_state);

      assert(Base::mesh_.FVH(hh[2]) == vh[1]);
      assert(Base::mesh_.FVH(hh[1]) == vh[0]);
      assert(Base::mesh_.FVH(hh[0]) == vh[2]);

      if (Base::mesh_.FH(Base::mesh_.OHEH(hh[0])).is_valid()) 
      {
        temp_hh = Base::mesh_.OHEH(Base::mesh_.PHEH(Base::mesh_.OHEH(hh[0])));
        if (MOBJ(Base::mesh_.FH(temp_hh)).red_halfedge() != temp_hh)
          halfedge_vector.push_back(temp_hh);
      }

      if (Base::mesh_.FH(Base::mesh_.OHEH(hh[1])).is_valid()) {

	temp_hh = Base::mesh_.OHEH(Base::mesh_.PHEH(Base::mesh_.OHEH(hh[1])));
	if (MOBJ(Base::mesh_.FH(temp_hh)).red_halfedge() != temp_hh)
	  halfedge_vector.push_back(temp_hh);
      }

      if (Base::mesh_.FH(Base::mesh_.OHEH(hh[2])).is_valid()) {

	temp_hh = Base::mesh_.OHEH(Base::mesh_.PHEH(Base::mesh_.OHEH(hh[2])));
	if (MOBJ(Base::mesh_.FH(temp_hh)).red_halfedge() != temp_hh)
	  halfedge_vector.push_back(temp_hh);
      }
    }
    
    // splitted face, check for type
    else {

      // define red halfedge handle
      typename M::HalfedgeHandle red_hh(MOBJ(_fh).red_halfedge());

      if (Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.PHEH(red_hh))).is_valid() 
          && Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(Base::mesh_.OHEH(red_hh)))).is_valid() 
          && MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.PHEH(red_hh)))).red_halfedge() == red_hh 
          && MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(Base::mesh_.OHEH(red_hh))))).red_halfedge() == red_hh) 
      {

        // three times divided face
        vh[0] = Base::mesh_.TVH(Base::mesh_.NHEH(Base::mesh_.OHEH(Base::mesh_.NHEH(Base::mesh_.OHEH(red_hh)))));
        vh[1] = Base::mesh_.TVH(red_hh);
        vh[2] = Base::mesh_.TVH(Base::mesh_.NHEH(Base::mesh_.OHEH(Base::mesh_.PHEH(red_hh))));
          
        new_vh[0] = Base::mesh_.FVH(red_hh);
        new_vh[1] = Base::mesh_.TVH(Base::mesh_.NHEH(Base::mesh_.OHEH(red_hh)));
        new_vh[2] = Base::mesh_.TVH(Base::mesh_.NHEH(red_hh));
    
        hh[0] = Base::mesh_.PHEH(Base::mesh_.OHEH(Base::mesh_.PHEH(red_hh)));
        hh[1] = Base::mesh_.PHEH(Base::mesh_.OHEH(Base::mesh_.NHEH(Base::mesh_.OHEH(red_hh))));
        hh[2] = Base::mesh_.NHEH(red_hh);
    
        eh = Base::mesh_.EH(red_hh);
      }
      
      else 
      {

	if ((Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.PHEH(red_hh))).is_valid() && 
             MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.PHEH(red_hh)))).red_halfedge() 
             == red_hh )
	    || (Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(Base::mesh_.OHEH(red_hh)))).is_valid() 
	    && MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(Base::mesh_.OHEH(red_hh))))).red_halfedge() == red_hh))
        {

	  // double divided face
	  if (MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.PHEH(red_hh)))).red_halfedge() == red_hh) 
	  {
        // first case
        vh[0] = Base::mesh_.TVH(Base::mesh_.NHEH(Base::mesh_.OHEH(red_hh)));
        vh[1] = Base::mesh_.TVH(red_hh);
        vh[2] = Base::mesh_.TVH(Base::mesh_.NHEH(Base::mesh_.OHEH(Base::mesh_.PHEH(red_hh))));
      
        new_vh[0] = Base::mesh_.FVH(red_hh);
        new_vh[1] = Base::mesh_.add_vertex(zero_point);
        new_vh[2] = Base::mesh_.TVH(Base::mesh_.NHEH(red_hh));
  
        hh[0] = Base::mesh_.PHEH(Base::mesh_.OHEH(Base::mesh_.PHEH(red_hh)));
        hh[1] = Base::mesh_.PHEH(Base::mesh_.OHEH(red_hh));
        hh[2] = Base::mesh_.NHEH(red_hh);
  
        // split one edge
        eh = Base::mesh_.EH(red_hh);
  
        split_edge(hh[1], new_vh[1], _target_state);
  
        assert(Base::mesh_.FVH(hh[2]) == vh[1]);
        assert(Base::mesh_.FVH(hh[1]) == vh[0]);
        assert(Base::mesh_.FVH(hh[0]) == vh[2]);
        
        if (Base::mesh_.FH(Base::mesh_.OHEH(hh[1])).is_valid()) 
        {
          temp_hh = Base::mesh_.OHEH(Base::mesh_.PHEH(Base::mesh_.OHEH(hh[1])));
          if (MOBJ(Base::mesh_.FH(temp_hh)).red_halfedge() != temp_hh)
            halfedge_vector.push_back(temp_hh);
        }
	  }
	  else 
	  {

	    // second case
	    vh[0] = Base::mesh_.TVH(Base::mesh_.NHEH(Base::mesh_.OHEH(Base::mesh_.NHEH(Base::mesh_.OHEH(red_hh)))));
	    vh[1] = Base::mesh_.TVH(red_hh);
	    vh[2] = Base::mesh_.TVH(Base::mesh_.NHEH(red_hh));
      
	    new_vh[0] = Base::mesh_.FVH(red_hh);
	    new_vh[1] = Base::mesh_.TVH(Base::mesh_.NHEH(Base::mesh_.OHEH(red_hh)));
	    new_vh[2] = Base::mesh_.add_vertex(zero_point);

	    hh[0] = Base::mesh_.PHEH(red_hh);
	    hh[1] = Base::mesh_.PHEH(Base::mesh_.OHEH(Base::mesh_.NHEH(Base::mesh_.OHEH(red_hh))));
	    hh[2] = Base::mesh_.NHEH(red_hh);

	    // split one edge
	    eh = Base::mesh_.EH(red_hh);

	    split_edge(hh[2], new_vh[2], _target_state);

	    assert(Base::mesh_.FVH(hh[2]) == vh[1]);
	    assert(Base::mesh_.FVH(hh[1]) == vh[0]);
	    assert(Base::mesh_.FVH(hh[0]) == vh[2]);

	    if (Base::mesh_.FH(Base::mesh_.OHEH(hh[2])).is_valid()) {
	    
	      temp_hh = Base::mesh_.OHEH(Base::mesh_.PHEH(Base::mesh_.OHEH(hh[2])));
	      if (MOBJ(Base::mesh_.FH(temp_hh)).red_halfedge() != temp_hh)
		halfedge_vector.push_back(temp_hh);
	    }
	  }
	}

	else {
	  
	  // one time divided face
	  vh[0] = Base::mesh_.TVH(Base::mesh_.NHEH(Base::mesh_.OHEH(red_hh)));
	  vh[1] = Base::mesh_.TVH(red_hh);
	  vh[2] = Base::mesh_.TVH(Base::mesh_.NHEH(red_hh));
      
	  new_vh[0] = Base::mesh_.FVH(red_hh);
	  new_vh[1] = Base::mesh_.add_vertex(zero_point);
	  new_vh[2] = Base::mesh_.add_vertex(zero_point);

	  hh[0] = Base::mesh_.PHEH(red_hh);
	  hh[1] = Base::mesh_.PHEH(Base::mesh_.OHEH(red_hh));
	  hh[2] = Base::mesh_.NHEH(red_hh);

	  // split two edges
	  eh = Base::mesh_.EH(red_hh);

	  split_edge(hh[1], new_vh[1], _target_state);
	  split_edge(hh[2], new_vh[2], _target_state);

	  assert(Base::mesh_.FVH(hh[2]) == vh[1]);
	  assert(Base::mesh_.FVH(hh[1]) == vh[0]);
	  assert(Base::mesh_.FVH(hh[0]) == vh[2]);

	  if (Base::mesh_.FH(Base::mesh_.OHEH(hh[1])).is_valid()) {

	    temp_hh = Base::mesh_.OHEH(Base::mesh_.PHEH(Base::mesh_.OHEH(hh[1])));
	    if (MOBJ(Base::mesh_.FH(temp_hh)).red_halfedge() != temp_hh)
	      halfedge_vector.push_back(temp_hh);
	  }

	  if (Base::mesh_.FH(Base::mesh_.OHEH(hh[2])).is_valid()) {

	    temp_hh = Base::mesh_.OHEH(Base::mesh_.PHEH(Base::mesh_.OHEH(hh[2])));
	    if (MOBJ(Base::mesh_.FH(temp_hh)).red_halfedge() != temp_hh)
	      halfedge_vector.push_back(temp_hh);
	  }
      	}
      }
    }

    // continue here for all cases
	
    // flip edge
    if (Base::mesh_.is_flip_ok(eh)) {
      
      Base::mesh_.flip(eh);
    }
    
    // search new faces
    fh[0] = Base::mesh_.FH(hh[0]);
    fh[1] = Base::mesh_.FH(hh[1]);
    fh[2] = Base::mesh_.FH(hh[2]);
    fh[3] = Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(hh[0])));

    assert(_fh == fh[0] || _fh == fh[1] || _fh == fh[2] || _fh == fh[3]); 

    // init new faces
    for (int i = 0; i <= 3; ++i) {

      MOBJ(fh[i]).set_state(_target_state);
      MOBJ(fh[i]).set_final();
      MOBJ(fh[i]).set_position(_target_state, face_position);
      MOBJ(fh[i]).set_red_halfedge(Base::mesh_.InvalidHalfedgeHandle);
    }

    // init new vertices and edges
    for (int i = 0; i <= 2; ++i) {
	
      MOBJ(new_vh[i]).set_position(_target_state, zero_point);
      MOBJ(new_vh[i]).set_state(_target_state);
      MOBJ(new_vh[i]).set_not_final();

      Base::mesh_.set_point(new_vh[i], (Base::mesh_.point(vh[i]) + Base::mesh_.point(vh[(i + 2) % 3])) * 0.5);

      MOBJ(Base::mesh_.EH(hh[i])).set_state(_target_state);
      MOBJ(Base::mesh_.EH(hh[i])).set_position(_target_state, zero_point);
      MOBJ(Base::mesh_.EH(hh[i])).set_final();

      MOBJ(Base::mesh_.EH(Base::mesh_.NHEH(hh[i]))).set_state(_target_state);
      MOBJ(Base::mesh_.EH(Base::mesh_.NHEH(hh[i]))).set_position(_target_state, zero_point);
      MOBJ(Base::mesh_.EH(Base::mesh_.NHEH(hh[i]))).set_final();

      MOBJ(Base::mesh_.EH(Base::mesh_.PHEH(hh[i]))).set_state(_target_state);
      MOBJ(Base::mesh_.EH(Base::mesh_.PHEH(hh[i]))).set_position(_target_state, zero_point);
      MOBJ(Base::mesh_.EH(Base::mesh_.PHEH(hh[i]))).set_final();
    }

    // check, if opposite triangle needs splitting
    while (!halfedge_vector.empty()) {

      temp_hh = halfedge_vector.back();
      halfedge_vector.pop_back();

      check_edge(temp_hh, _target_state);
    }

    assert(MOBJ(fh[0]).state() == _target_state);
    assert(MOBJ(fh[1]).state() == _target_state);
    assert(MOBJ(fh[2]).state() == _target_state);
    assert(MOBJ(fh[3]).state() == _target_state);
  }
}


template<class M>
void
Tvv4<M>::raise(typename M::VertexHandle& _vh, state_t _target_state)
{
 
  if (MOBJ(_vh).state() < _target_state)
  {

    this->update(_vh, _target_state);

    // multiply old position by 4
    MOBJ(_vh).set_position(_target_state, MOBJ(_vh).position(_target_state - 1) * static_cast<typename M::Point::value_type>(4.0));

    MOBJ(_vh).inc_state();
  }
}


template<class M>
void
Tvv4<M>::raise(typename M::EdgeHandle& _eh, state_t _target_state) 
{ 
  if (MOBJ(_eh).state() < _target_state) 
  {
    this->update(_eh, _target_state);

    typename M::FaceHandle fh(Base::mesh_.FH(Base::mesh_.HEH(_eh, 0)));

    if (!fh.is_valid())
      fh=Base::mesh_.FH(Base::mesh_.HEH(_eh, 1));

    raise(fh, _target_state);

    assert(MOBJ(_eh).state() == _target_state);
  }
}

#ifndef DOXY_IGNORE_THIS

template<class M>
void
Tvv4<M>::split_edge(typename M::HalfedgeHandle &_hh, 
                       typename M::VertexHandle   &_vh, 
                       state_t _target_state) 
                       {
  typename M::HalfedgeHandle temp_hh;

  if (Base::mesh_.FH(Base::mesh_.OHEH(_hh)).is_valid()) 
  {
    if (!MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(_hh))).final()) 
    {    
      if (MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(_hh))).red_halfedge().is_valid()) 
      {        
        temp_hh = MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(_hh))).red_halfedge();
      }
      else 
      {
        // two cases for divided, but not visited face
        if (MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.PHEH(Base::mesh_.OHEH(_hh))))).state()
            == MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(_hh))).state()) 
        {
          temp_hh = Base::mesh_.PHEH(Base::mesh_.OHEH(_hh));
        }

        else if (MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(Base::mesh_.OHEH(_hh))))).state()
            == MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(_hh))).state())
        {
          temp_hh = Base::mesh_.NHEH(Base::mesh_.OHEH(_hh));
        }
      }
    }
    else
      temp_hh = Base::mesh_.InvalidHalfedgeHandle;
  }
  else
    temp_hh = Base::mesh_.InvalidHalfedgeHandle;

  // split edge
  Base::mesh_.split(Base::mesh_.EH(_hh), _vh);

  if (Base::mesh_.FVH(_hh) == _vh) 
  {	    
    MOBJ(Base::mesh_.EH(Base::mesh_.PHEH(Base::mesh_.OHEH(Base::mesh_.PHEH(_hh))))).set_state(MOBJ(Base::mesh_.EH(_hh)).state());
    _hh = Base::mesh_.PHEH(Base::mesh_.OHEH(Base::mesh_.PHEH(_hh)));
  }

  if (Base::mesh_.FH(Base::mesh_.OHEH(_hh)).is_valid()) {
	  
    MOBJ(Base::mesh_.EH(Base::mesh_.PHEH(Base::mesh_.OHEH(_hh)))).set_not_final();
    MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(_hh))).set_state(_target_state-1);
    MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(Base::mesh_.OHEH(Base::mesh_.NHEH(_hh)))))).set_state(_target_state-1);

    MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(_hh))).set_not_final();
    MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(Base::mesh_.OHEH(Base::mesh_.NHEH(_hh)))))).set_not_final();

    MOBJ(Base::mesh_.EH(Base::mesh_.PHEH(Base::mesh_.OHEH(_hh)))).set_state(_target_state);

    if (temp_hh.is_valid()) {
	
      MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(_hh))).set_red_halfedge(temp_hh);
      MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(Base::mesh_.OHEH(Base::mesh_.NHEH(_hh)))))).set_red_halfedge(temp_hh);
    } 
    else {

      typename M::FaceHandle 
        fh1(Base::mesh_.FH(Base::mesh_.OHEH(_hh))),
        fh2(Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(Base::mesh_.OHEH(Base::mesh_.NHEH(_hh))))));

      MOBJ(fh1).set_red_halfedge(Base::mesh_.OHEH(Base::mesh_.PHEH(Base::mesh_.OHEH(_hh))));
      MOBJ(fh2).set_red_halfedge(Base::mesh_.OHEH(Base::mesh_.PHEH(Base::mesh_.OHEH(_hh))));

      const typename M::Point zero_point(0.0, 0.0, 0.0);

      MOBJ(fh1).set_position(_target_state - 1, zero_point);
      MOBJ(fh2).set_position(_target_state - 1, zero_point);
    }
  }

  // init edges
  MOBJ(Base::mesh_.EH(Base::mesh_.NHEH(Base::mesh_.OHEH(Base::mesh_.NHEH(_hh))))).set_state(_target_state - 1);
  MOBJ(Base::mesh_.EH(Base::mesh_.NHEH(Base::mesh_.OHEH(Base::mesh_.NHEH(_hh))))).set_final();

  MOBJ(Base::mesh_.EH(_hh)).set_state(_target_state - 1);
  MOBJ(Base::mesh_.EH(_hh)).set_final();
}  


template<class M>
void Tvv4<M>::check_edge(const typename M::HalfedgeHandle& _hh, 
                         state_t _target_state) 
{
  typename M::FaceHandle fh1(Base::mesh_.FH(_hh)),
    fh2(Base::mesh_.FH(Base::mesh_.OHEH(_hh)));

  assert(fh1.is_valid());
  assert(fh2.is_valid());

  typename M::HalfedgeHandle red_hh(MOBJ(fh1).red_halfedge());
  
  if (!MOBJ(fh1).final()) {

    assert (MOBJ(fh1).final() == MOBJ(fh2).final());
    assert (!MOBJ(fh1).final());
    assert (MOBJ(fh1).red_halfedge() == MOBJ(fh2).red_halfedge());

    const typename M::Point zero_point(0.0, 0.0, 0.0);

    MOBJ(fh1).set_position(_target_state - 1, zero_point);
    MOBJ(fh2).set_position(_target_state - 1, zero_point);

    assert(red_hh.is_valid());

    if (!red_hh.is_valid()) {

      MOBJ(fh1).set_state(_target_state - 1);
      MOBJ(fh2).set_state(_target_state - 1);

      MOBJ(fh1).set_red_halfedge(_hh);
      MOBJ(fh2).set_red_halfedge(_hh);

      MOBJ(Base::mesh_.EH(_hh)).set_not_final();
      MOBJ(Base::mesh_.EH(_hh)).set_state(_target_state - 1);
    }

    else {

      MOBJ(Base::mesh_.EH(_hh)).set_not_final();
      MOBJ(Base::mesh_.EH(_hh)).set_state(_target_state - 1);

      raise(fh1, _target_state);

      assert(MOBJ(fh1).state() == _target_state);
    }
  }
}


// -------------------------------------------------------------------- VF ----


template<class M>
void VF<M>::raise(typename M::FaceHandle& _fh, state_t _target_state) 
{
  if (MOBJ(_fh).state() < _target_state) {

    this->update(_fh, _target_state);

    // raise all neighbor vertices to level x-1
    typename M::FaceVertexIter            fv_it;
    typename M::VertexHandle              vh;
    std::vector<typename M::VertexHandle> vertex_vector;

    if (_target_state > 1) {

      for (fv_it = Base::mesh_.fv_iter(_fh); fv_it.is_valid(); ++fv_it) {

        vertex_vector.push_back(*fv_it);
      }

      while (!vertex_vector.empty()) {

        vh = vertex_vector.back();
        vertex_vector.pop_back();

        Base::prev_rule()->raise(vh, _target_state - 1);
      }
    }

    // calculate new position
    typename M::Point  position(0.0, 0.0, 0.0);
    typename M::Scalar valence(0.0);

    for (fv_it = Base::mesh_.fv_iter(_fh); fv_it.is_valid(); ++fv_it) {

      valence  += 1.0;
      position += Base::mesh_.data(*fv_it).position(_target_state - 1);
    }

    position /= valence;

    // boundary rule
    if (Base::number() == Base::subdiv_rule()->number() + 1 && 
        Base::mesh_.is_boundary(_fh)                  && 
        !MOBJ(_fh).final())
      position *= static_cast<typename M::Scalar>(0.5);

    MOBJ(_fh).set_position(_target_state, position);
    MOBJ(_fh).inc_state();

    assert(_target_state == MOBJ(_fh).state());
  }
}


// -------------------------------------------------------------------- FF ----


template<class M>
void FF<M>::raise(typename M::FaceHandle& _fh, state_t _target_state) {

  if (MOBJ(_fh).state() < _target_state) {

    this->update(_fh, _target_state);

    // raise all neighbor faces to level x-1
    typename M::FaceFaceIter              ff_it;
    typename M::FaceHandle                fh;
    std::vector<typename M::FaceHandle>   face_vector;

    if (_target_state > 1) {

      for (ff_it = Base::mesh_.ff_iter(_fh); ff_it.is_valid(); ++ff_it) {

        face_vector.push_back(*ff_it);
      }

      while (!face_vector.empty()) {

        fh = face_vector.back();
        face_vector.pop_back();

        Base::prev_rule()->raise(fh, _target_state - 1);
      }

      for (ff_it = Base::mesh_.ff_iter(_fh); ff_it.is_valid(); ++ff_it) {

        face_vector.push_back(*ff_it);
      }

      while (!face_vector.empty()) {

        fh = face_vector.back();
        face_vector.pop_back();

        while (MOBJ(fh).state() < _target_state - 1)
          Base::prev_rule()->raise(fh, _target_state - 1);
      }
    }

    // calculate new position
    typename M::Point  position(0.0, 0.0, 0.0);
    typename M::Scalar valence(0.0);

    for (ff_it = Base::mesh_.ff_iter(_fh); ff_it.is_valid(); ++ff_it) {

      valence  += 1.0;

      position += Base::mesh_.data(*ff_it).position(_target_state - 1);
    }

    position /= valence;

    MOBJ(_fh).set_position(_target_state, position);
    MOBJ(_fh).inc_state();
  }
}


// ------------------------------------------------------------------- FFc ----


template<class M>
void FFc<M>::raise(typename M::FaceHandle& _fh, state_t _target_state) 
{
  if (MOBJ(_fh).state() < _target_state) {

    this->update(_fh, _target_state);

    // raise all neighbor faces to level x-1
    typename M::FaceFaceIter              ff_it(Base::mesh_.ff_iter(_fh));
    typename M::FaceHandle                fh;
    std::vector<typename M::FaceHandle>   face_vector;

    if (_target_state > 1) 
    {
      for (; ff_it.is_valid(); ++ff_it)
        face_vector.push_back(*ff_it);

      while (!face_vector.empty()) 
      {
        fh = face_vector.back();
        face_vector.pop_back();
        Base::prev_rule()->raise(fh, _target_state - 1);
      }

      for (ff_it = Base::mesh_.ff_iter(_fh); ff_it.is_valid(); ++ff_it)
        face_vector.push_back(*ff_it);

      while (!face_vector.empty()) {

        fh = face_vector.back();
        face_vector.pop_back();

        while (MOBJ(fh).state() < _target_state - 1)
          Base::prev_rule()->raise(fh, _target_state - 1);
      }
    }

    // calculate new position
    typename M::Point  position(0.0, 0.0, 0.0);
    typename M::Scalar valence(0.0);

    for (ff_it = Base::mesh_.ff_iter(_fh); ff_it.is_valid(); ++ff_it)
    {
      valence += 1.0;
      position += Base::mesh_.data(*ff_it).position(_target_state - 1);
    }

    position /= valence;

    // choose coefficient c
    typename M::Scalar c = Base::coeff();

    position *= (static_cast<typename M::Scalar>(1.0) - c);
    position += MOBJ(_fh).position(_target_state - 1) * c;

    MOBJ(_fh).set_position(_target_state, position);
    MOBJ(_fh).inc_state();
  }
}


// -------------------------------------------------------------------- FV ----


template<class M>
void FV<M>::raise(typename M::VertexHandle& _vh, state_t _target_state) 
{

  if (MOBJ(_vh).state() < _target_state) {

    this->update(_vh, _target_state);

    // raise all neighbor vertices to level x-1
    typename M::VertexFaceIter            vf_it(Base::mesh_.vf_iter(_vh));
    typename M::FaceHandle                fh;
    std::vector<typename M::FaceHandle>   face_vector;

    if (_target_state > 1) {

      for (; vf_it.is_valid(); ++vf_it) {

        face_vector.push_back(*vf_it);
      }

      while (!face_vector.empty()) {

        fh = face_vector.back();
        face_vector.pop_back();

        Base::prev_rule()->raise(fh, _target_state - 1);
      }

      for (vf_it = Base::mesh_.vf_iter(_vh); vf_it.is_valid(); ++vf_it) {

        face_vector.push_back(*vf_it);
      }

      while (!face_vector.empty()) {

        fh = face_vector.back();
        face_vector.pop_back();

        while (MOBJ(fh).state() < _target_state - 1)
          Base::prev_rule()->raise(fh, _target_state - 1);
      }
    }

    // calculate new position
    typename M::Point  position(0.0, 0.0, 0.0);
    typename M::Scalar valence(0.0);

    for (vf_it = Base::mesh_.vf_iter(_vh); vf_it.is_valid(); ++vf_it) {

      valence  += 1.0;
      position += Base::mesh_.data(*vf_it).position(_target_state - 1);
    }

    position /= valence;

    MOBJ(_vh).set_position(_target_state, position);
    MOBJ(_vh).inc_state();

    if (Base::number() == Base::n_rules() - 1) {

      Base::mesh_.set_point(_vh, position);
      MOBJ(_vh).set_final();
    }
  }
}


// ------------------------------------------------------------------- FVc ----


template<class M>
void FVc<M>::raise(typename M::VertexHandle& _vh, state_t _target_state) 
{
  if (MOBJ(_vh).state() < _target_state) {

    this->update(_vh, _target_state);

    typename M::VertexOHalfedgeIter       voh_it;
    typename M::FaceHandle                fh;
    std::vector<typename M::FaceHandle>   face_vector;
    int                                      valence(0);

    face_vector.clear();

    // raise all neighbour faces to level x-1
    if (_target_state > 1) {

      for (voh_it = Base::mesh_.voh_iter(_vh); voh_it.is_valid(); ++voh_it) {

        if (Base::mesh_.FH(*voh_it).is_valid()) {

          face_vector.push_back(Base::mesh_.FH(*voh_it));

          if (Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(*voh_it))).is_valid()) {

            face_vector.push_back(Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(*voh_it))));
          }
        }
      }

      while (!face_vector.empty()) {

        fh = face_vector.back();
        face_vector.pop_back();

        Base::prev_rule()->raise(fh, _target_state - 1);
      }

      for (voh_it = Base::mesh_.voh_iter(_vh); voh_it.is_valid(); ++voh_it) {

        if (Base::mesh_.FH(*voh_it).is_valid()) {

          face_vector.push_back(Base::mesh_.FH(*voh_it));

          if (Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(*voh_it))).is_valid()) {

            face_vector.push_back(Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(*voh_it))));
          }
        }
      }

      while (!face_vector.empty()) {

        fh = face_vector.back();
        face_vector.pop_back();

        while (MOBJ(fh).state() < _target_state - 1)
          Base::prev_rule()->raise(fh, _target_state - 1);
      }

      for (voh_it = Base::mesh_.voh_iter(_vh); voh_it.is_valid(); ++voh_it) {

        if (Base::mesh_.FH(*voh_it).is_valid()) {

          face_vector.push_back(Base::mesh_.FH(*voh_it));

          if (Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(*voh_it))).is_valid()) {

            face_vector.push_back(Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(*voh_it))));
          }
        }
      }

      while (!face_vector.empty()) {

        fh = face_vector.back();
        face_vector.pop_back();

        while (MOBJ(fh).state() < _target_state - 1)
          Base::prev_rule()->raise(fh, _target_state - 1);
      }
    }

    // calculate new position
    typename M::Point               position(0.0, 0.0, 0.0);
    typename M::Scalar              c;
#if 0
    const typename M::Scalar        _2pi(2.0*M_PI);
    const typename M::Scalar        _2over3(2.0/3.0);

    for (voh_it = Base::mesh_.voh_iter(_vh); voh_it; ++voh_it) 
    {     
      ++valence;
    }

    // choose coefficient c
    c = _2over3 * ( cos( _2pi / valence) + 1.0);
#else
    valence = Base::mesh_.valence(_vh);
    c       = typename M::Scalar(coeff(valence));
#endif


    for (voh_it = Base::mesh_.voh_iter(_vh); voh_it.is_valid(); ++voh_it) {

      fh = Base::mesh_.FH(*voh_it);
      if (fh.is_valid())
        Base::prev_rule()->raise(fh, _target_state - 1);

      fh = Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(*voh_it)));
      if (fh.is_valid())
        Base::prev_rule()->raise(fh, _target_state - 1);

      if (Base::mesh_.FH(*voh_it).is_valid()) {

        if (Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(*voh_it))).is_valid()) {

          position += MOBJ(Base::mesh_.FH(*voh_it)).position(_target_state - 1) * c;

          position += MOBJ(Base::mesh_.FH(Base::mesh_.OHEH(Base::mesh_.NHEH(*voh_it)))).position(_target_state - 1) * ( typename M::Scalar(1.0) - c);
        }
        else {

          position += MOBJ(Base::mesh_.FH(*voh_it)).position(_target_state - 1);
        }
      }

      else {

        --valence;
      }
    } 

    position /= typename M::Scalar(valence);

    MOBJ(_vh).set_position(_target_state, position);
    MOBJ(_vh).inc_state();

    assert(MOBJ(_vh).state() == _target_state);

    // check if last rule
    if (Base::number() == Base::n_rules() - 1) {

      Base::mesh_.set_point(_vh, position);
      MOBJ(_vh).set_final();
    }
  }
}

template<class M>
std::vector<double> FVc<M>::coeffs_;

template <class M>
void FVc<M>::init_coeffs(size_t _max_valence)
{
  if ( coeffs_.size() == _max_valence+1 )
    return;

  if ( coeffs_.size() < _max_valence+1 )
  {
    const double _2pi(2.0*M_PI);
    const double _2over3(2.0/3.0);

    if (coeffs_.empty())
      coeffs_.push_back(0.0); // dummy for valence 0
      
    for(size_t v=coeffs_.size(); v <= _max_valence; ++v)
      coeffs_.push_back(_2over3 * ( cos( _2pi / v) + 1.0));    
  }
}


// -------------------------------------------------------------------- VV ----


template<class M>
void VV<M>::raise(typename M::VertexHandle& _vh, state_t _target_state) 
{
  if (MOBJ(_vh).state() < _target_state) 
  {
    this->update(_vh, _target_state);

    // raise all neighbor vertices to level x-1
    typename M::VertexVertexIter              vv_it(Base::mesh_.vv_iter(_vh));
    typename M::VertexHandle                  vh;
    std::vector<typename M::VertexHandle>     vertex_vector;

    if (_target_state > 1) {

      for (; vv_it.is_valid(); ++vv_it) {

        vertex_vector.push_back(*vv_it);
      }

      while (!vertex_vector.empty()) {

        vh = vertex_vector.back();
        vertex_vector.pop_back();

        Base::prev_rule()->raise(vh, _target_state - 1);
      }

      for (; vv_it.is_valid(); ++vv_it) {

        vertex_vector.push_back(*vv_it);
      }

      while (!vertex_vector.empty()) {

        vh = vertex_vector.back();
        vertex_vector.pop_back();

        Base::prev_rule()->raise(vh, _target_state - 1);
      }
    }

    // calculate new position
    typename M::Point  position(0.0, 0.0, 0.0);
    typename M::Scalar valence(0.0);

    for (vv_it = Base::mesh_.vv_iter(_vh); vv_it.is_valid(); ++vv_it) {

      valence  += 1.0;
      position += Base::mesh_.data(*vv_it).position(_target_state - 1);
    }

    position /= valence;

    MOBJ(_vh).set_position(_target_state, position);
    MOBJ(_vh).inc_state();

    // check if last rule
    if (Base::number() == Base::n_rules() - 1) {

      Base::mesh_.set_point(_vh, position);
      MOBJ(_vh).set_final();
    }
  }
}


// ------------------------------------------------------------------- VVc ----


template<class M>
void VVc<M>::raise(typename M::VertexHandle& _vh, state_t _target_state)
{
  if (MOBJ(_vh).state() < _target_state) {

    this->update(_vh, _target_state);

    // raise all neighbor vertices to level x-1
    typename M::VertexVertexIter              vv_it(Base::mesh_.vv_iter(_vh));
    typename M::VertexHandle                  vh;
    std::vector<typename M::VertexHandle>     vertex_vector;

    if (_target_state > 1) {

      for (; vv_it.is_valid(); ++vv_it) {

        vertex_vector.push_back(*vv_it);
      }

      while (!vertex_vector.empty()) {

        vh = vertex_vector.back();
        vertex_vector.pop_back();

        Base::prev_rule()->raise(vh, _target_state - 1);
      }

      for (; vv_it.is_valid(); ++vv_it) {

        vertex_vector.push_back(*vv_it);
      }

      while (!vertex_vector.empty()) {

        vh = vertex_vector.back();
        vertex_vector.pop_back();

        Base::prev_rule()->raise(vh, _target_state - 1);
      }
    }

    // calculate new position
    typename M::Point  position(0.0, 0.0, 0.0);
    typename M::Scalar valence(0.0);
    typename M::Scalar c;

    for (vv_it = Base::mesh_.vv_iter(_vh); vv_it.is_valid(); ++vv_it)
    {
      valence += 1.0;
      position += Base::mesh_.data(*vv_it).position(_target_state - 1);
    }

    position /= valence;

    // choose coefficient c
    c = Base::coeff();

    position *= (static_cast<typename M::Scalar>(1.0) - c);
    position += MOBJ(_vh).position(_target_state - 1) * c;

    MOBJ(_vh).set_position(_target_state, position);
    MOBJ(_vh).inc_state();

    if (Base::number() == Base::n_rules() - 1) 
    {
      Base::mesh_.set_point(_vh, position);
      MOBJ(_vh).set_final();
    }
  }
}


// -------------------------------------------------------------------- VE ----


template<class M>
void VE<M>::raise(typename M::EdgeHandle& _eh, state_t _target_state) 
{
  if (MOBJ(_eh).state() < _target_state) {

    this->update(_eh, _target_state);

    // raise all neighbour vertices to level x-1
    typename M::VertexHandle          vh;
    typename M::HalfedgeHandle        hh1(Base::mesh_.HEH(_eh, 0)),
                                         hh2(Base::mesh_.HEH(_eh, 1));

    if (_target_state > 1) {

      vh = Base::mesh_.TVH(hh1);
      
      Base::prev_rule()->raise(vh, _target_state - 1);

      vh = Base::mesh_.TVH(hh2);
      
      Base::prev_rule()->raise(vh, _target_state - 1);
    }

    // calculate new position
    typename M::Point  position(0.0, 0.0, 0.0);
    const typename M::Scalar valence(2.0);

    position += MOBJ(Base::mesh_.TVH(hh1)).position(_target_state - 1);
    position += MOBJ(Base::mesh_.TVH(hh2)).position(_target_state - 1);

    position /= valence;

    MOBJ(_eh).set_position(_target_state, position);
    MOBJ(_eh).inc_state();
  }
}


// ------------------------------------------------------------------- VdE ----


template<class M>
void VdE<M>::raise(typename M::EdgeHandle& _eh, state_t _target_state) 
{
  if (MOBJ(_eh).state() < _target_state) 
  {
    this->update(_eh, _target_state);

    // raise all neighbor vertices to level x-1
    typename M::VertexHandle             vh;
    typename M::HalfedgeHandle           hh1(Base::mesh_.HEH(_eh, 0)),
        hh2(Base::mesh_.HEH(_eh, 1));
    typename M::FaceHandle                fh1, fh2;

    if (_target_state > 1) {

      fh1 = Base::mesh_.FH(hh1);
      fh2 = Base::mesh_.FH(hh2);

      if (fh1.is_valid()) {

        Base::prev_rule()->raise(fh1, _target_state - 1);

        vh = Base::mesh_.TVH(Base::mesh_.NHEH(hh1));
        Base::prev_rule()->raise(vh, _target_state - 1);
      }

      if (fh2.is_valid()) {

        Base::prev_rule()->raise(fh2, _target_state - 1);

        vh = Base::mesh_.TVH(Base::mesh_.NHEH(hh2));
        Base::prev_rule()->raise(vh, _target_state - 1);
      }

      vh = Base::mesh_.TVH(hh1);
      Base::prev_rule()->raise(vh, _target_state - 1);

      vh = Base::mesh_.TVH(hh2);
      Base::prev_rule()->raise(vh, _target_state - 1);
    }

    // calculate new position
    typename M::Point  position(0.0, 0.0, 0.0);
    typename M::Scalar valence(2.0);

    position += MOBJ(Base::mesh_.TVH(hh1)).position(_target_state - 1);
    position += MOBJ(Base::mesh_.TVH(hh2)).position(_target_state - 1);

    if (fh1.is_valid()) {

      position += MOBJ(Base::mesh_.TVH(Base::mesh_.NHEH(hh1))).position(_target_state - 1);
      valence += 1.0;
    }

    if (fh2.is_valid()) {
      
      position += MOBJ(Base::mesh_.TVH(Base::mesh_.NHEH(hh2))).position(_target_state - 1);
      valence += 1.0;
    }

    if (Base::number() == Base::subdiv_rule()->Base::number() + 1) 
      valence = 4.0;

    position /= valence;

    MOBJ(_eh).set_position(_target_state, position);
    MOBJ(_eh).inc_state();
  }
}


// ------------------------------------------------------------------ VdEc ----


template<class M>
void
VdEc<M>::raise(typename M::EdgeHandle& _eh, state_t _target_state) 
{
  if (MOBJ(_eh).state() < _target_state) 
  {
    this->update(_eh, _target_state);

    // raise all neighbor vertices to level x-1
    typename M::VertexHandle             vh;
    typename M::HalfedgeHandle           hh1(Base::mesh_.HEH(_eh, 0)),
                                            hh2(Base::mesh_.HEH(_eh, 1));
    std::vector<typename M::VertexHandle> vertex_vector;
    typename M::FaceHandle                fh1, fh2;

    if (_target_state > 1) {

      fh1 = Base::mesh_.FH(Base::mesh_.HEH(_eh, 0));
      fh2 = Base::mesh_.FH(Base::mesh_.HEH(_eh, 1));

      Base::prev_rule()->raise(fh1, _target_state - 1);
      Base::prev_rule()->raise(fh2, _target_state - 1);

      vertex_vector.push_back(Base::mesh_.TVH(hh1));
      vertex_vector.push_back(Base::mesh_.TVH(hh2));

      vertex_vector.push_back(Base::mesh_.TVH(Base::mesh_.NHEH(hh1)));
      vertex_vector.push_back(Base::mesh_.TVH(Base::mesh_.NHEH(hh2)));

      while (!vertex_vector.empty()) {

        vh = vertex_vector.back();
        vertex_vector.pop_back();

        Base::prev_rule()->raise(vh, _target_state - 1);
      }

      vertex_vector.push_back(Base::mesh_.TVH(hh1));
      vertex_vector.push_back(Base::mesh_.TVH(hh2));

      vertex_vector.push_back(Base::mesh_.TVH(Base::mesh_.NHEH(hh1)));
      vertex_vector.push_back(Base::mesh_.TVH(Base::mesh_.NHEH(hh2)));

      while (!vertex_vector.empty()) {

        vh = vertex_vector.back();
        vertex_vector.pop_back();

        Base::prev_rule()->raise(vh, _target_state - 1);
      }
    }

    // calculate new position
    typename M::Point  position(0.0, 0.0, 0.0);
    const typename M::Scalar valence(4.0);
    typename M::Scalar c;

    // choose coefficient c
    c = Base::coeff();

    position += MOBJ(Base::mesh_.TVH(hh1)).position(_target_state - 1) * c;
    position += MOBJ(Base::mesh_.TVH(hh2)).position(_target_state - 1) * c;
    position += MOBJ(Base::mesh_.TVH(Base::mesh_.NHEH(hh1))).position(_target_state - 1) * (0.5 - c);
    position += MOBJ(Base::mesh_.TVH(Base::mesh_.NHEH(hh2))).position(_target_state - 1) * (0.5 - c);

    position /= valence;

    MOBJ(_eh).set_position(_target_state, position);
    MOBJ(_eh).inc_state();
  }
}


// -------------------------------------------------------------------- EV ----


template<class M>
void EV<M>::raise(typename M::VertexHandle& _vh, state_t _target_state) 
{
  if (MOBJ(_vh).state() < _target_state) {

    this->update(_vh, _target_state);

    // raise all neighbor vertices to level x-1
    typename M::VertexEdgeIter            ve_it(Base::mesh_.ve_iter(_vh));
    typename M::EdgeHandle                eh;
    std::vector<typename M::EdgeHandle>   edge_vector;

    if (_target_state > 1) {

      for (; ve_it.is_valid(); ++ve_it) {

        edge_vector.push_back(*ve_it);
      }

      while (!edge_vector.empty()) {

        eh = edge_vector.back();
        edge_vector.pop_back();

        Base::prev_rule()->raise(eh, _target_state - 1);
      }

      for (ve_it = Base::mesh_.ve_iter(_vh); ve_it.is_valid(); ++ve_it) {

        edge_vector.push_back(*ve_it);
      }

      while (!edge_vector.empty()) {

        eh = edge_vector.back();
        edge_vector.pop_back();

        while (MOBJ(eh).state() < _target_state - 1)
          Base::prev_rule()->raise(eh, _target_state - 1);
      }
    }

    // calculate new position
    typename M::Point  position(0.0, 0.0, 0.0);
    typename M::Scalar valence(0.0);

    for (ve_it = Base::mesh_.ve_iter(_vh); ve_it.is_valid(); ++ve_it) {

      if (Base::mesh_.data(*ve_it).final()) {

        valence += 1.0;

        position += Base::mesh_.data(*ve_it).position(_target_state - 1);
      }
    }

    position /= valence;

    MOBJ(_vh).set_position(_target_state, position);
    MOBJ(_vh).inc_state();

    // check if last rule
    if (Base::number() == Base::n_rules() - 1) {

      Base::mesh_.set_point(_vh, position);
      MOBJ(_vh).set_final();
    }
  }
}


// ------------------------------------------------------------------- EVc ----

template<class M>
std::vector<double> EVc<M>::coeffs_;

template<class M>
void EVc<M>::raise(typename M::VertexHandle& _vh, state_t _target_state) 
{
  if (MOBJ(_vh).state() < _target_state) 
  {
    this->update(_vh, _target_state);

    // raise all neighbour vertices to level x-1
    typename M::VertexOHalfedgeIter       voh_it;
    typename M::EdgeHandle                eh;
    typename M::FaceHandle                fh;
    std::vector<typename M::EdgeHandle>   edge_vector;
    std::vector<typename M::FaceHandle>   face_vector;

    if (_target_state > 1) {

      for (voh_it = Base::mesh_.voh_iter(_vh); voh_it.is_valid(); ++voh_it) {

        face_vector.push_back(Base::mesh_.FH(*voh_it));
      }

      while (!face_vector.empty()) {

        fh = face_vector.back();
        face_vector.pop_back();

        if (fh.is_valid())
          Base::prev_rule()->raise(fh, _target_state - 1);
      }

      for (voh_it = Base::mesh_.voh_iter(_vh); voh_it.is_valid(); ++voh_it) {

        edge_vector.push_back(Base::mesh_.EH(*voh_it));

        edge_vector.push_back(Base::mesh_.EH(Base::mesh_.NHEH(*voh_it)));
      }

      while (!edge_vector.empty()) {

        eh = edge_vector.back();
        edge_vector.pop_back();

        while (MOBJ(eh).state() < _target_state - 1)
          Base::prev_rule()->raise(eh, _target_state - 1);
      }
    }


    // calculate new position
    typename M::Point        position(0.0, 0.0, 0.0);
    typename M::Scalar       c;
    typename M::Point        zero_point(0.0, 0.0, 0.0);
    size_t                   valence(0);

    valence = Base::mesh_.valence(_vh);
    c       = static_cast<typename M::Scalar>(coeff( valence ));

    for (voh_it = Base::mesh_.voh_iter(_vh); voh_it.is_valid(); ++voh_it)
    {
      if (MOBJ(Base::mesh_.EH(*voh_it)).final()) 
      {
        position += MOBJ(Base::mesh_.EH(*voh_it)).position(_target_state-1)*c;

        if ( Base::mesh_.FH(*voh_it).is_valid() && 
            MOBJ(Base::mesh_.EH(Base::mesh_.NHEH(*voh_it))).final() &&
            MOBJ(Base::mesh_.EH(Base::mesh_.NHEH(*voh_it))).position(_target_state - 1) != zero_point)
        {
          position += MOBJ(Base::mesh_.EH(Base::mesh_.NHEH(*voh_it))).position(_target_state-1) * (1.0-c);
        }
        else {
          position += MOBJ(Base::mesh_.EH(*voh_it)).position(_target_state - 1) * (1.0 - c);
        }
      }
      else {
        --valence;
      }
    }

    position /= valence;

    MOBJ(_vh).set_position(_target_state, position);
    MOBJ(_vh).inc_state();

    // check if last rule
    if (Base::number() == Base::n_rules() - 1) {

      Base::mesh_.set_point(_vh, position);
      MOBJ(_vh).set_final();
    }
  }
}


template <class M>
void 
EVc<M>::init_coeffs(size_t _max_valence)
{
  if ( coeffs_.size() == _max_valence+1 ) // equal? do nothing
    return;

  if (coeffs_.size() < _max_valence+1) // less than? add additional valences
  {
    const double _2pi = 2.0*M_PI;
  
    if (coeffs_.empty())
      coeffs_.push_back(0.0); // dummy for invalid valences 0,1,2

    for(size_t v=coeffs_.size(); v <= _max_valence; ++v)
    {
      // ( 3/2 + cos ( 2 PI / valence ) )� / 2 - 1
      double c = 1.5 + cos( _2pi / v );
      c = c * c * 0.5 - 1.0;
      coeffs_.push_back(c);
    }
  }
}


// -------------------------------------------------------------------- EF ----

template<class M>
void
EF<M>::raise(typename M::FaceHandle& _fh, state_t _target_state) {

  if (MOBJ(_fh).state() < _target_state) {

    this->update(_fh, _target_state);

    // raise all neighbour edges to level x-1
    typename M::FaceEdgeIter              fe_it(Base::mesh_.fe_iter(_fh));
    typename M::EdgeHandle                eh;
    std::vector<typename M::EdgeHandle>   edge_vector;

    if (_target_state > 1) {

      for (; fe_it.is_valid(); ++fe_it) {

        edge_vector.push_back(*fe_it);
      }

      while (!edge_vector.empty()) {

        eh = edge_vector.back();
        edge_vector.pop_back();

        Base::prev_rule()->raise(eh, _target_state - 1);
      }

      for (fe_it = Base::mesh_.fe_iter(_fh); fe_it.is_valid(); ++fe_it) {

        edge_vector.push_back(*fe_it);
      }

      while (!edge_vector.empty()) {

        eh = edge_vector.back();
        edge_vector.pop_back();

        while (MOBJ(eh).state() < _target_state - 1)
          Base::prev_rule()->raise(eh, _target_state - 1);
      }
    }

    // calculate new position
    typename M::Point  position(0.0, 0.0, 0.0);
    typename M::Scalar valence(0.0);

    for (fe_it = Base::mesh_.fe_iter(_fh); fe_it.is_valid(); ++fe_it) {

      if (Base::mesh_.data(*fe_it).final()) {

        valence += 1.0;

        position += Base::mesh_.data(*fe_it).position(_target_state - 1);
      }
    }

    assert (valence == 3.0);

    position /= valence;

    MOBJ(_fh).set_position(_target_state, position);
    MOBJ(_fh).inc_state();
  }
}


// -------------------------------------------------------------------- FE ----


template<class M>
void
FE<M>::raise(typename M::EdgeHandle& _eh, state_t _target_state) {

  if (MOBJ(_eh).state() < _target_state) {

    this->update(_eh, _target_state);

    // raise all neighbor faces to level x-1
    typename M::FaceHandle                fh;

    if (_target_state > 1) {

      fh = Base::mesh_.FH(Base::mesh_.HEH(_eh, 0));
      Base::prev_rule()->raise(fh, _target_state - 1);

      fh = Base::mesh_.FH(Base::mesh_.HEH(_eh, 1));
      Base::prev_rule()->raise(fh, _target_state - 1);

      fh = Base::mesh_.FH(Base::mesh_.HEH(_eh, 0));
      Base::prev_rule()->raise(fh, _target_state - 1);

      fh = Base::mesh_.FH(Base::mesh_.HEH(_eh, 1));
      Base::prev_rule()->raise(fh, _target_state - 1);
    }

    // calculate new position
    typename M::Point  position(0.0, 0.0, 0.0);
    const typename M::Scalar valence(2.0);

    position += MOBJ(Base::mesh_.FH(Base::mesh_.HEH(_eh, 0))).position(_target_state - 1);
    
    position += MOBJ(Base::mesh_.FH(Base::mesh_.HEH(_eh, 1))).position(_target_state - 1);
    
    position /= valence;

    MOBJ(_eh).set_position(_target_state, position);
    MOBJ(_eh).inc_state();
  }
}


// ------------------------------------------------------------------- EdE ----


template<class M>
void
EdE<M>::raise(typename M::EdgeHandle& _eh, state_t _target_state) {

  if (MOBJ(_eh).state() < _target_state) {

    this->update(_eh, _target_state);

    // raise all neighbor faces and edges to level x-1
    typename M::HalfedgeHandle            hh1, hh2;
    typename M::FaceHandle                fh;
    typename M::EdgeHandle                eh;

    hh1 = Base::mesh_.HEH(_eh, 0);
    hh2 = Base::mesh_.HEH(_eh, 1);

    if (_target_state > 1) {

      fh = Base::mesh_.FH(hh1);
      Base::prev_rule()->raise(fh, _target_state - 1);

      fh = Base::mesh_.FH(hh2);
      Base::prev_rule()->raise(fh, _target_state - 1);

      eh = Base::mesh_.EH(Base::mesh_.NHEH(hh1));
      Base::prev_rule()->raise(eh, _target_state - 1);

      eh = Base::mesh_.EH(Base::mesh_.PHEH(hh1));
      Base::prev_rule()->raise(eh, _target_state - 1);

      eh = Base::mesh_.EH(Base::mesh_.NHEH(hh2));
      Base::prev_rule()->raise(eh, _target_state - 1);

      eh = Base::mesh_.EH(Base::mesh_.PHEH(hh2));
      Base::prev_rule()->raise(eh, _target_state - 1);
    }

    // calculate new position
    typename M::Point  position(0.0, 0.0, 0.0);
    const typename M::Scalar valence(4.0);

    position += MOBJ(Base::mesh_.EH(Base::mesh_.NHEH(hh1))).position(_target_state - 1);
    position += MOBJ(Base::mesh_.EH(Base::mesh_.PHEH(hh1))).position(_target_state - 1);
    position += MOBJ(Base::mesh_.EH(Base::mesh_.NHEH(hh2))).position(_target_state - 1);
    position += MOBJ(Base::mesh_.EH(Base::mesh_.PHEH(hh2))).position(_target_state - 1);

    position /= valence;

    MOBJ(_eh).set_position(_target_state, position);
    MOBJ(_eh).inc_state();
  }
}


// ------------------------------------------------------------------ EdEc ----


template<class M>
void
EdEc<M>::raise(typename M::EdgeHandle& _eh, state_t _target_state) 
{
  if (MOBJ(_eh).state() < _target_state) {

    this->update(_eh, _target_state);

    // raise all neighbor faces and edges to level x-1
    typename M::HalfedgeHandle            hh1, hh2;
    typename M::FaceHandle                fh;
    typename M::EdgeHandle                eh;

    hh1 = Base::mesh_.HEH(_eh, 0);
    hh2 = Base::mesh_.HEH(_eh, 1);

    if (_target_state > 1) {

      fh = Base::mesh_.FH(hh1);
      Base::prev_rule()->raise(fh, _target_state - 1);

      fh = Base::mesh_.FH(hh2);
      Base::prev_rule()->raise(fh, _target_state - 1);

      eh = Base::mesh_.EH(Base::mesh_.NHEH(hh1));
      Base::prev_rule()->raise(eh, _target_state - 1);

      eh = Base::mesh_.EH(Base::mesh_.PHEH(hh1));
      Base::prev_rule()->raise(eh, _target_state - 1);

      eh = Base::mesh_.EH(Base::mesh_.NHEH(hh2));
      Base::prev_rule()->raise(eh, _target_state - 1);

      eh = Base::mesh_.EH(Base::mesh_.PHEH(hh2));
      Base::prev_rule()->raise(eh, _target_state - 1);
    }

    // calculate new position
    typename M::Point  position(0.0, 0.0, 0.0);
    const typename M::Scalar valence(4.0);
    typename M::Scalar c;

    position += MOBJ(Base::mesh_.EH(Base::mesh_.NHEH(hh1))).position(_target_state - 1);
    position += MOBJ(Base::mesh_.EH(Base::mesh_.PHEH(hh1))).position(_target_state - 1);
    position += MOBJ(Base::mesh_.EH(Base::mesh_.NHEH(hh2))).position(_target_state - 1);
    position += MOBJ(Base::mesh_.EH(Base::mesh_.PHEH(hh2))).position(_target_state - 1);

    position /= valence;

    // choose coefficient c
    c = Base::coeff();

    position *= ( static_cast<typename M::Scalar>(1.0) - c);

    position += MOBJ(_eh).position(_target_state - 1) * c;

    MOBJ(_eh).set_position(_target_state, position);
    MOBJ(_eh).inc_state();
  }
}

#endif // DOXY_IGNORE_THIS

#undef FH 
#undef VH 
#undef EH 
#undef HEH
#undef M
#undef MOBJ

//=============================================================================
} // END_NS_ADAPTIVE
} // END_NS_SUBDIVIDER
} // END_NS_OPENMESH
//=============================================================================
