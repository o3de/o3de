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


/** \file LongestEdgeT.hh

*/

//=============================================================================
//
//  CLASS LongestEdgeT
//
//=============================================================================


#ifndef LINEAR_H
#define LINEAR_H

#include <OpenMesh/Tools/Subdivider/Uniform/SubdividerT.hh>
#include <OpenMesh/Core/Utils/vector_cast.hh>
#include <OpenMesh/Core/Utils/Property.hh>
// -------------------- STL
#include <vector>
#include <queue>
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

template <typename MeshType, typename RealType = double>
class CompareLengthFunction {
  public:

    typedef std::pair<typename MeshType::EdgeHandle, RealType> queueElement;

    bool operator()(const queueElement& t1, const queueElement& t2) // Returns true if t1 is smaller than t2
    {
      return (t1.second < t2.second);
    }
};


/** %Uniform LongestEdgeT subdivision algorithm
 *
 * Very simple algorithm splitting all edges which are longer than given via
 * set_max_edge_length(). The split is always performed on the longest
 * edge in the mesh.
 */
template <typename MeshType, typename RealType = float>
class LongestEdgeT : public SubdividerT<MeshType, RealType>
{
public:

  typedef RealType                                real_t;
  typedef MeshType                                mesh_t;
  typedef SubdividerT< mesh_t, real_t >           parent_t;

  typedef std::vector< std::vector<real_t> >      weights_t;
  typedef std::vector<real_t>                     weight_t;

  typedef std::pair< typename mesh_t::EdgeHandle, real_t > queueElement;

public:


  LongestEdgeT() : parent_t()
  {  }


  LongestEdgeT( mesh_t& _m) : parent_t(_m)
  {  }


  ~LongestEdgeT() {}


public:


  const char *name() const { return "Longest Edge Split"; }

  void set_max_edge_length(double _value) {
    max_edge_length_squared_ = _value * _value;
  }

protected:


  bool prepare( mesh_t& _m )
  {
    return true;
  }


  bool cleanup( mesh_t& _m )
  {
    return true;
  }


  bool subdivide( MeshType& _m, size_t _n , const bool _update_points = true)
  {

    // Sorted queue containing all edges sorted by their decreasing length
    std::priority_queue< queueElement, std::vector< queueElement > , CompareLengthFunction< mesh_t, real_t > > queue;

    // Build the initial queue
    // First element should be longest edge
    typename mesh_t::EdgeIter edgesEnd = _m.edges_end();
    for ( typename mesh_t::EdgeIter eit  = _m.edges_begin(); eit != edgesEnd; ++eit) {
      const typename MeshType::Point to   = _m.point(_m.to_vertex_handle(_m.halfedge_handle(*eit,0)));
      const typename MeshType::Point from = _m.point(_m.from_vertex_handle(_m.halfedge_handle(*eit,0)));

      real_t length = (to - from).sqrnorm();

      // Only push the edges that need to be split
      if ( length > max_edge_length_squared_ )
        queue.push( queueElement(*eit,length) );
    }

    bool stop = false;
    while ( !stop && ! queue.empty() ) {
      queueElement a = queue.top();
      queue.pop();

      if ( a.second < max_edge_length_squared_ ) {
        stop = true;
        break;
      } else {
        const typename MeshType::Point to   = _m.point(_m.to_vertex_handle(_m.halfedge_handle(a.first,0)));
        const typename MeshType::Point from = _m.point(_m.from_vertex_handle(_m.halfedge_handle(a.first,0)));
        const typename MeshType::Point midpoint = static_cast<typename MeshType::Point::value_type>(0.5) * (to + from);

        const typename MeshType::VertexHandle newVertex = _m.add_vertex(midpoint);
        _m.split(a.first,newVertex);

        for ( typename MeshType::VertexOHalfedgeIter voh_it(_m,newVertex); voh_it.is_valid(); ++voh_it) {
          typename MeshType::EdgeHandle eh = _m.edge_handle(*voh_it);
          const typename MeshType::Point to   = _m.point(_m.to_vertex_handle(*voh_it));
          const typename MeshType::Point from = _m.point(_m.from_vertex_handle(*voh_it));
          real_t length = (to - from).sqrnorm();

          // Only push the edges that need to be split
          if ( length > max_edge_length_squared_ )
            queue.push( queueElement(eh,length) );

        }
      }
    }

#if defined(_DEBUG) || defined(DEBUG)
      // Now we have an consistent mesh!
      assert( OpenMesh::Utils::MeshCheckerT<mesh_t>(_m).check() );
#endif


    return true;
  }


private: // data
  real_t max_edge_length_squared_;

};

} // END_NS_UNIFORM
} // END_NS_SUBDIVIDER
} // END_NS_OPENMESH
#endif

