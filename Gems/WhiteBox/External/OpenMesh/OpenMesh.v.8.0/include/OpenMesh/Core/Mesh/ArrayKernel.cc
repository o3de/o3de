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



#include <OpenMesh/Core/Mesh/ArrayKernel.hh>

namespace OpenMesh
{

ArrayKernel::ArrayKernel()
: refcount_vstatus_(0), refcount_hstatus_(0),
  refcount_estatus_(0), refcount_fstatus_(0)
{
  init_bit_masks(); //Status bit masks initialization
}

ArrayKernel::~ArrayKernel()
{
  clear();
}

// ArrayKernel::ArrayKernel(const ArrayKernel& _rhs)
// : BaseKernel(_rhs),
//   vertices_(_rhs.vertices_), edges_(_rhs.edges_), faces_(_rhs.faces_),
//   vertex_status_(_rhs.vertex_status_), halfedge_status_(_rhs.halfedge_status_),
//   edge_status_(_rhs.edge_status_), face_status_(_rhs.face_status_),
//   refcount_vstatus_(_rhs.refcount_vstatus_), refcount_hstatus_(_rhs.refcount_hstatus_),
//   refcount_estatus_(_rhs.refcount_estatus_), refcount_fstatus_(_rhs.refcount_fstatus_)
// {}


void ArrayKernel::assign_connectivity(const ArrayKernel& _other)
{
  vertices_ = _other.vertices_;
  edges_ = _other.edges_;
  faces_ = _other.faces_;
  
  vprops_resize(n_vertices());
  hprops_resize(n_halfedges());
  eprops_resize(n_edges());
  fprops_resize(n_faces());
  
  //just copy status properties for now,
  //until a proper solution for refcounted
  //properties is available
  vertex_status_ = _other.vertex_status_;
  halfedge_status_ = _other.halfedge_status_;
  edge_status_ = _other.edge_status_;
  face_status_ = _other.face_status_;

  //initialize refcounter to 1 for the new mesh, 
  //if status is available.
  refcount_estatus_ = _other.refcount_estatus_ > 0 ? 1 : 0;
  refcount_vstatus_ = _other.refcount_vstatus_ > 0 ? 1 : 0;
  refcount_hstatus_ = _other.refcount_hstatus_ > 0 ? 1 : 0;
  refcount_fstatus_ = _other.refcount_fstatus_ > 0 ? 1 : 0;
}

// --- handle -> item ---
VertexHandle ArrayKernel::handle(const Vertex& _v) const
{
   return VertexHandle( int( &_v - &vertices_.front()));
}

HalfedgeHandle ArrayKernel::handle(const Halfedge& _he) const
{
  // Calculate edge belonging to given halfedge
  // There are two halfedges stored per edge
  // Get memory position inside edge vector and devide by size of an edge
  // to get the corresponding edge for the requested halfedge
  size_t eh = ( (char*)&_he - (char*)&edges_.front() ) /  sizeof(Edge)  ;
  assert((&_he == &edges_[eh].halfedges_[0]) ||
         (&_he == &edges_[eh].halfedges_[1]));
  return ((&_he == &edges_[eh].halfedges_[0]) ?
                    HalfedgeHandle( int(eh)<<1) : HalfedgeHandle((int(eh)<<1)+1));
}

EdgeHandle ArrayKernel::handle(const Edge& _e) const
{
  return EdgeHandle( int(&_e - &edges_.front() ) );
}

FaceHandle ArrayKernel::handle(const Face& _f) const
{
  return FaceHandle( int(&_f - &faces_.front()) );
}

#define SIGNED(x) signed( (x) )

bool ArrayKernel::is_valid_handle(VertexHandle _vh) const
{
  return 0 <= _vh.idx() && _vh.idx() < SIGNED(n_vertices());
}

bool ArrayKernel::is_valid_handle(HalfedgeHandle _heh) const
{
  return 0 <= _heh.idx() && _heh.idx() < SIGNED(n_edges()*2);
}

bool ArrayKernel::is_valid_handle(EdgeHandle _eh) const
{
  return 0 <= _eh.idx() && _eh.idx() < SIGNED(n_edges());
}

bool ArrayKernel::is_valid_handle(FaceHandle _fh) const
{
  return 0 <= _fh.idx() && _fh.idx() < SIGNED(n_faces());
}

#undef SIGNED

unsigned int ArrayKernel::delete_isolated_vertices()
{
  assert(has_vertex_status());//this function requires vertex status property
  unsigned int n_isolated = 0;
  for (KernelVertexIter v_it = vertices_begin(); v_it != vertices_end(); ++v_it)
  {
    if (is_isolated(handle(*v_it)))
    {
      status(handle(*v_it)).set_deleted(true);
      n_isolated++;
    }
  }
  return n_isolated;
}

void ArrayKernel::garbage_collection(bool _v, bool _e, bool _f)
{
  std::vector<VertexHandle*> empty_vh;
  std::vector<HalfedgeHandle*> empty_hh;
  std::vector<FaceHandle*> empty_fh;
  garbage_collection( empty_vh,empty_hh,empty_fh,_v, _e, _f);
}

void ArrayKernel::clean_keep_reservation()
{
    vertices_.clear();

    edges_.clear();

    faces_.clear();

}

void ArrayKernel::clean()
{

  vertices_.clear();
  VertexContainer().swap( vertices_ );

  edges_.clear();
  EdgeContainer().swap( edges_ );

  faces_.clear();
  FaceContainer().swap( faces_ );

}


void ArrayKernel::clear()
{
  vprops_clear();
  eprops_clear();
  hprops_clear();
  fprops_clear();

  clean();
}



void ArrayKernel::resize( size_t _n_vertices, size_t _n_edges, size_t _n_faces )
{
  vertices_.resize(_n_vertices);
  edges_.resize(_n_edges);
  faces_.resize(_n_faces);

  vprops_resize(n_vertices());
  hprops_resize(n_halfedges());
  eprops_resize(n_edges());
  fprops_resize(n_faces());
}

void ArrayKernel::reserve(size_t _n_vertices, size_t _n_edges, size_t _n_faces )
{
  vertices_.reserve(_n_vertices);
  edges_.reserve(_n_edges);
  faces_.reserve(_n_faces);

  vprops_reserve(_n_vertices);
  hprops_reserve(_n_edges*2);
  eprops_reserve(_n_edges);
  fprops_reserve(_n_faces);
}

// Status Sets API
void ArrayKernel::init_bit_masks(BitMaskContainer& _bmc)
{
  for (unsigned int i = Attributes::UNUSED; i != 0; i <<= 1)
  {
    _bmc.push_back(i);
  }
}

void ArrayKernel::init_bit_masks()
{
  init_bit_masks(vertex_bit_masks_);
  edge_bit_masks_ = vertex_bit_masks_;//init_bit_masks(edge_bit_masks_);
  face_bit_masks_ = vertex_bit_masks_;//init_bit_masks(face_bit_masks_);
  halfedge_bit_masks_= vertex_bit_masks_;//init_bit_masks(halfedge_bit_masks_);
}


};

