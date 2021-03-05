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



//== IMPLEMENTATION ==========================================================
#include <OpenMesh/Core/Mesh/PolyConnectivity.hh>
#include <set>

namespace OpenMesh {

const PolyConnectivity::VertexHandle    PolyConnectivity::InvalidVertexHandle;
const PolyConnectivity::HalfedgeHandle  PolyConnectivity::InvalidHalfedgeHandle;
const PolyConnectivity::EdgeHandle      PolyConnectivity::InvalidEdgeHandle;
const PolyConnectivity::FaceHandle      PolyConnectivity::InvalidFaceHandle;

//-----------------------------------------------------------------------------

PolyConnectivity::HalfedgeHandle
PolyConnectivity::find_halfedge(VertexHandle _start_vh, VertexHandle _end_vh ) const
{
  assert(_start_vh.is_valid() && _end_vh.is_valid());

  for (ConstVertexOHalfedgeIter voh_it = cvoh_iter(_start_vh); voh_it.is_valid(); ++voh_it)
    if (to_vertex_handle(*voh_it) == _end_vh)
      return *voh_it;

  return InvalidHalfedgeHandle;
}


bool PolyConnectivity::is_boundary(FaceHandle _fh, bool _check_vertex) const
{
  for (ConstFaceEdgeIter cfeit = cfe_iter( _fh ); cfeit.is_valid(); ++cfeit)
      if (is_boundary( *cfeit ) )
        return true;

  if (_check_vertex)
  {
      for (ConstFaceVertexIter cfvit = cfv_iter( _fh ); cfvit.is_valid(); ++cfvit)
        if (is_boundary( *cfvit ) )
            return true;
  }
  return false;
}

bool PolyConnectivity::is_manifold(VertexHandle _vh) const
{
  /* The vertex is non-manifold if more than one gap exists, i.e.
    more than one outgoing boundary halfedge. If (at least) one
    boundary halfedge exists, the vertex' halfedge must be a
    boundary halfedge. If iterating around the vertex finds another
    boundary halfedge, the vertex is non-manifold. */

  ConstVertexOHalfedgeIter vh_it(*this, _vh);
  if (vh_it.is_valid())
    for (++vh_it; vh_it.is_valid(); ++vh_it)
        if (is_boundary(*vh_it))
          return false;
  return true;
}

//-----------------------------------------------------------------------------
void PolyConnectivity::adjust_outgoing_halfedge(VertexHandle _vh)
{
  for (ConstVertexOHalfedgeIter vh_it=cvoh_iter(_vh); vh_it.is_valid(); ++vh_it)
  {
    if (is_boundary(*vh_it))
    {
      set_halfedge_handle(_vh, *vh_it);
      break;
    }
  }
}

//-----------------------------------------------------------------------------

PolyConnectivity::FaceHandle
PolyConnectivity::add_face(const VertexHandle* _vertex_handles, size_t _vhs_size)
{
  VertexHandle                   vh;
  size_t                         i, ii, n(_vhs_size);
  HalfedgeHandle                 inner_next, inner_prev,
                                 outer_next, outer_prev,
                                 boundary_next, boundary_prev,
                                 patch_start, patch_end;


  // Check sufficient working storage available
  if (edgeData_.size() < n)
  {
    edgeData_.resize(n);
    next_cache_.resize(6*n);
  }

  size_t next_cache_count = 0;

  // don't allow degenerated faces
  assert (n > 2);

  // test for topological errors
  for (i=0, ii=1; i<n; ++i, ++ii, ii%=n)
  {
    if ( !is_boundary(_vertex_handles[i]) )
    {
      omerr() << "PolyMeshT::add_face: complex vertex\n";
      return InvalidFaceHandle;
    }

    // Initialise edge attributes
    edgeData_[i].halfedge_handle = find_halfedge(_vertex_handles[i],
                                                 _vertex_handles[ii]);
    edgeData_[i].is_new = !edgeData_[i].halfedge_handle.is_valid();
    edgeData_[i].needs_adjust = false;

    if (!edgeData_[i].is_new && !is_boundary(edgeData_[i].halfedge_handle))
    {
      omerr() << "PolyMeshT::add_face: complex edge\n";
      return InvalidFaceHandle;
    }
  }

  // re-link patches if necessary
  for (i=0, ii=1; i<n; ++i, ++ii, ii%=n)
  {
    if (!edgeData_[i].is_new && !edgeData_[ii].is_new)
    {
      inner_prev = edgeData_[i].halfedge_handle;
      inner_next = edgeData_[ii].halfedge_handle;


      if (next_halfedge_handle(inner_prev) != inner_next)
      {
        // here comes the ugly part... we have to relink a whole patch

        // search a free gap
        // free gap will be between boundary_prev and boundary_next
        outer_prev = opposite_halfedge_handle(inner_next);
        outer_next = opposite_halfedge_handle(inner_prev);
        boundary_prev = outer_prev;
        do
          boundary_prev =
            opposite_halfedge_handle(next_halfedge_handle(boundary_prev));
        while (!is_boundary(boundary_prev));
        boundary_next = next_halfedge_handle(boundary_prev);

        // ok ?
        if (boundary_prev == inner_prev)
        {
          omerr() << "PolyMeshT::add_face: patch re-linking failed\n";
          return InvalidFaceHandle;
        }

        assert(is_boundary(boundary_prev));
        assert(is_boundary(boundary_next));

        // other halfedges' handles
        patch_start = next_halfedge_handle(inner_prev);
        patch_end   = prev_halfedge_handle(inner_next);

        assert(boundary_prev.is_valid());
        assert(patch_start.is_valid());
        assert(patch_end.is_valid());
        assert(boundary_next.is_valid());
        assert(inner_prev.is_valid());
        assert(inner_next.is_valid());

        // relink
        next_cache_[next_cache_count++] = std::make_pair(boundary_prev, patch_start);
        next_cache_[next_cache_count++] = std::make_pair(patch_end, boundary_next);
        next_cache_[next_cache_count++] = std::make_pair(inner_prev, inner_next);
      }
    }
  }

  // create missing edges
  for (i=0, ii=1; i<n; ++i, ++ii, ii%=n)
    if (edgeData_[i].is_new)
      edgeData_[i].halfedge_handle = new_edge(_vertex_handles[i], _vertex_handles[ii]);

  // create the face
  FaceHandle fh(new_face());
  set_halfedge_handle(fh, edgeData_[n-1].halfedge_handle);

  // setup halfedges
  for (i=0, ii=1; i<n; ++i, ++ii, ii%=n)
  {
    vh         = _vertex_handles[ii];

    inner_prev = edgeData_[i].halfedge_handle;
    inner_next = edgeData_[ii].halfedge_handle;
    assert(inner_prev.is_valid());
    assert(inner_next.is_valid());

    size_t id = 0;
    if (edgeData_[i].is_new)  id |= 1;
    if (edgeData_[ii].is_new) id |= 2;


    if (id)
    {
      outer_prev = opposite_halfedge_handle(inner_next);
      outer_next = opposite_halfedge_handle(inner_prev);
      assert(outer_prev.is_valid());
      assert(outer_next.is_valid());

      // set outer links
      switch (id)
      {
        case 1: // prev is new, next is old
          boundary_prev = prev_halfedge_handle(inner_next);
          assert(boundary_prev.is_valid());
          next_cache_[next_cache_count++] = std::make_pair(boundary_prev, outer_next);
          set_halfedge_handle(vh, outer_next);
          break;

        case 2: // next is new, prev is old
          boundary_next = next_halfedge_handle(inner_prev);
          assert(boundary_next.is_valid());
          next_cache_[next_cache_count++] = std::make_pair(outer_prev, boundary_next);
          set_halfedge_handle(vh, boundary_next);
          break;

        case 3: // both are new
          if (!halfedge_handle(vh).is_valid())
          {
            set_halfedge_handle(vh, outer_next);
            next_cache_[next_cache_count++] = std::make_pair(outer_prev, outer_next);
          }
          else
          {
            boundary_next = halfedge_handle(vh);
            boundary_prev = prev_halfedge_handle(boundary_next);
            assert(boundary_prev.is_valid());
            assert(boundary_next.is_valid());
            next_cache_[next_cache_count++] = std::make_pair(boundary_prev, outer_next);
            next_cache_[next_cache_count++] = std::make_pair(outer_prev, boundary_next);
          }
          break;
      }

      // set inner link
      next_cache_[next_cache_count++] = std::make_pair(inner_prev, inner_next);
    }
    else edgeData_[ii].needs_adjust = (halfedge_handle(vh) == inner_next);


    // set face handle
    set_face_handle(edgeData_[i].halfedge_handle, fh);
  }

  // process next halfedge cache
  for (i = 0; i < next_cache_count; ++i)
    set_next_halfedge_handle(next_cache_[i].first, next_cache_[i].second);


  // adjust vertices' halfedge handle
  for (i=0; i<n; ++i)
    if (edgeData_[i].needs_adjust)
      adjust_outgoing_halfedge(_vertex_handles[i]);

  return fh;
}

//-----------------------------------------------------------------------------

FaceHandle PolyConnectivity::add_face(VertexHandle _vh0, VertexHandle _vh1, VertexHandle _vh2, VertexHandle _vh3)
{
  VertexHandle vhs[4] = { _vh0, _vh1, _vh2, _vh3 };
  return add_face(vhs, 4);
}

//-----------------------------------------------------------------------------

FaceHandle PolyConnectivity::add_face(VertexHandle _vh0, VertexHandle _vh1, VertexHandle _vh2)
{
  VertexHandle vhs[3] = { _vh0, _vh1, _vh2 };
  return add_face(vhs, 3);
}

//-----------------------------------------------------------------------------

FaceHandle PolyConnectivity::add_face(const std::vector<VertexHandle>& _vhandles)
{ return add_face(&_vhandles.front(), _vhandles.size()); }


//-----------------------------------------------------------------------------
bool PolyConnectivity::is_collapse_ok(HalfedgeHandle v0v1)
{
  //is edge already deleteed?
  if (status(edge_handle(v0v1)).deleted())
  {
    return false;
  }

  HalfedgeHandle v1v0(opposite_halfedge_handle(v0v1));
  VertexHandle v0(to_vertex_handle(v1v0));
  VertexHandle v1(to_vertex_handle(v0v1));  

  bool v0v1_triangle = false;
  bool v1v0_triangle = false;
  
  if (!is_boundary(v0v1))
    v0v1_triangle = valence(face_handle(v0v1)) == 3;
  
  if (!is_boundary(v1v0))
    v1v0_triangle = valence(face_handle(v1v0)) == 3;

  //in a quadmesh we dont have the "next" or "previous" vhandle, so we need to look at previous and next on both sides
  //VertexHandle v_01_p = from_vertex_handle(prev_halfedge_handle(v0v1));
  VertexHandle v_01_n = to_vertex_handle(next_halfedge_handle(v0v1));  

  //VertexHandle v_10_p = from_vertex_handle(prev_halfedge_handle(v1v0));
  VertexHandle v_10_n = to_vertex_handle(next_halfedge_handle(v1v0));

  //are the vertices already deleted ?
  if (status(v0).deleted() || status(v1).deleted())
  {
    return false;
  }

  //the edges v1-vl and vl-v0 must not be both boundary edges
  //this test makes only sense in a polymesh if the side face is a triangle
  VertexHandle vl;
  if (!is_boundary(v0v1))
  {
    if (v0v1_triangle)
    {
      HalfedgeHandle h1 = next_halfedge_handle(v0v1);
      HalfedgeHandle h2 = next_halfedge_handle(h1);
      
      vl = to_vertex_handle(h1);
      
      if (is_boundary(opposite_halfedge_handle(h1)) && is_boundary(opposite_halfedge_handle(h2)))
        return false;
    }
  }

  //the edges v0-vr and vr-v1 must not be both boundary edges
  //this test makes only sense in a polymesh if the side face is a triangle
  VertexHandle vr;
  if (!is_boundary(v1v0))
  {
    if (v1v0_triangle)
    {
      HalfedgeHandle h1 = next_halfedge_handle(v1v0);
      HalfedgeHandle h2 = next_halfedge_handle(h1);
      
      vr = to_vertex_handle(h1);
      
      if (is_boundary(opposite_halfedge_handle(h1)) && is_boundary(opposite_halfedge_handle(h2)))
        return false;
    }
  }

  // if vl and vr are equal and valid (e.g. triangle case) -> fail
  if ( vl.is_valid() && (vl == vr)) return false;

  // edge between two boundary vertices should be a boundary edge
  if ( is_boundary(v0) && is_boundary(v1) && !is_boundary(v0v1) && !is_boundary(v1v0))
    return false;
  
  VertexVertexIter vv_it;
  // test intersection of the one-rings of v0 and v1
  for (vv_it = vv_iter(v0); vv_it.is_valid(); ++vv_it)
  {
    status(*vv_it).set_tagged(false);
  }

  for (vv_it = vv_iter(v1); vv_it.is_valid(); ++vv_it)
  {
    status(*vv_it).set_tagged(true);
  }

  for (vv_it = vv_iter(v0); vv_it.is_valid(); ++vv_it)
  {
    if (status(*vv_it).tagged() &&
      !(*vv_it == v_01_n && v0v1_triangle) &&
      !(*vv_it == v_10_n && v1v0_triangle)
      )
    {
      return false;
    }
  }
  
  //test for a face on the backside/other side that might degenerate
  if (v0v1_triangle)
  {
    HalfedgeHandle one, two;
    one = next_halfedge_handle(v0v1);
    two = next_halfedge_handle(one);
    
    one = opposite_halfedge_handle(one);
    two = opposite_halfedge_handle(two);
    
    if (face_handle(one) == face_handle(two) && valence(face_handle(one)) != 3)
    {
      return false;
    }
  }
  
  if (v1v0_triangle)
  {
    HalfedgeHandle one, two;
    one = next_halfedge_handle(v1v0);
    two = next_halfedge_handle(one);
    
    one = opposite_halfedge_handle(one);
    two = opposite_halfedge_handle(two);
    
    if (face_handle(one) == face_handle(two) && valence(face_handle(one)) != 3)
    {
      return false;
    }
  }

  if (status(*vv_it).tagged() && v_01_n == v_10_n && v0v1_triangle && v1v0_triangle)
  {
    return false;
  }

  // passed all tests
  return true;
}

//-----------------------------------------------------------------------------

void PolyConnectivity::delete_vertex(VertexHandle _vh, bool _delete_isolated_vertices)
{
  // store incident faces
  std::vector<FaceHandle> face_handles;
  face_handles.reserve(8);
  for (VFIter vf_it(vf_iter(_vh)); vf_it.is_valid(); ++vf_it)
    face_handles.push_back(*vf_it);


  // delete collected faces
  std::vector<FaceHandle>::iterator fh_it(face_handles.begin()),
                                    fh_end(face_handles.end());

  for (; fh_it!=fh_end; ++fh_it)
    delete_face(*fh_it, _delete_isolated_vertices);

  status(_vh).set_deleted(true);
}

//-----------------------------------------------------------------------------

void PolyConnectivity::delete_edge(EdgeHandle _eh, bool _delete_isolated_vertices)
{
  FaceHandle fh0(face_handle(halfedge_handle(_eh, 0)));
  FaceHandle fh1(face_handle(halfedge_handle(_eh, 1)));

  if (fh0.is_valid())  delete_face(fh0, _delete_isolated_vertices);
  if (fh1.is_valid())  delete_face(fh1, _delete_isolated_vertices);

  // If there is no face, we delete the edge
  // here
  if ( ! fh0.is_valid() && !fh1.is_valid()) {
    // mark edge deleted if the mesh has a edge status
    if ( has_edge_status() )
      status(_eh).set_deleted(true);

    // mark corresponding halfedges as deleted
    // As the deleted edge is boundary,
    // all corresponding halfedges will also be deleted.
    if ( has_halfedge_status() ) {
      status(halfedge_handle(_eh, 0)).set_deleted(true);
      status(halfedge_handle(_eh, 1)).set_deleted(true);
    }
  }
}

//-----------------------------------------------------------------------------

void PolyConnectivity::delete_face(FaceHandle _fh, bool _delete_isolated_vertices)
{
  assert(_fh.is_valid() && !status(_fh).deleted());

  // mark face deleted
  status(_fh).set_deleted(true);


  // this vector will hold all boundary edges of face _fh
  // these edges will be deleted
  std::vector<EdgeHandle> deleted_edges;
  deleted_edges.reserve(3);


  // this vector will hold all vertices of face _fh
  // for updating their outgoing halfedge
  std::vector<VertexHandle>  vhandles;
  vhandles.reserve(3);


  // for all halfedges of face _fh do:
  //   1) invalidate face handle.
  //   2) collect all boundary halfedges, set them deleted
  //   3) store vertex handles
  HalfedgeHandle hh;
  for (FaceHalfedgeIter fh_it(fh_iter(_fh)); fh_it.is_valid(); ++fh_it)
  {
    hh = *fh_it;

    set_boundary(hh);//set_face_handle(hh, InvalidFaceHandle);

    if (is_boundary(opposite_halfedge_handle(hh)))
        deleted_edges.push_back(edge_handle(hh));

    vhandles.push_back(to_vertex_handle(hh));
  }


  // delete all collected (half)edges
  // these edges were all boundary
  // delete isolated vertices (if _delete_isolated_vertices is true)
  if (!deleted_edges.empty())
  {
    std::vector<EdgeHandle>::iterator del_it(deleted_edges.begin()),
                                      del_end(deleted_edges.end());
    HalfedgeHandle h0, h1, next0, next1, prev0, prev1;
    VertexHandle   v0, v1;

    for (; del_it!=del_end; ++del_it)
    {
      h0    = halfedge_handle(*del_it, 0);
      v0    = to_vertex_handle(h0);
      next0 = next_halfedge_handle(h0);
      prev0 = prev_halfedge_handle(h0);

      h1    = halfedge_handle(*del_it, 1);
      v1    = to_vertex_handle(h1);
      next1 = next_halfedge_handle(h1);
      prev1 = prev_halfedge_handle(h1);

      // adjust next and prev handles
      set_next_halfedge_handle(prev0, next1);
      set_next_halfedge_handle(prev1, next0);

      // mark edge deleted if the mesh has a edge status
      if ( has_edge_status() )
    	 status(*del_it).set_deleted(true);


      // mark corresponding halfedges as deleted
      // As the deleted edge is boundary,
      // all corresponding halfedges will also be deleted.
      if ( has_halfedge_status() ) {
        status(h0).set_deleted(true);
        status(h1).set_deleted(true);
      }

      // update v0
      if (halfedge_handle(v0) == h1)
      {
        // isolated ?
        if (next0 == h1)
        {
          if (_delete_isolated_vertices)
            status(v0).set_deleted(true);
          set_isolated(v0);
        }
        else set_halfedge_handle(v0, next0);
      }

      // update v1
      if (halfedge_handle(v1) == h0)
      {
        // isolated ?
        if (next1 == h0)
        {
          if (_delete_isolated_vertices)
            status(v1).set_deleted(true);
          set_isolated(v1);
        }
        else  set_halfedge_handle(v1, next1);
      }
    }
  }

  // update outgoing halfedge handles of remaining vertices
  std::vector<VertexHandle>::iterator v_it(vhandles.begin()),
                                      v_end(vhandles.end());
  for (; v_it!=v_end; ++v_it)
    adjust_outgoing_halfedge(*v_it);
}

//-----------------------------------------------------------------------------
PolyConnectivity::VertexIter PolyConnectivity::vertices_begin()
{
  return VertexIter(*this, VertexHandle(0));
}

//-----------------------------------------------------------------------------
PolyConnectivity::ConstVertexIter PolyConnectivity::vertices_begin() const
{
  return ConstVertexIter(*this, VertexHandle(0));
}

//-----------------------------------------------------------------------------
PolyConnectivity::VertexIter PolyConnectivity::vertices_end()
{
  return VertexIter(*this, VertexHandle( int(n_vertices() ) ));
}

//-----------------------------------------------------------------------------
PolyConnectivity::ConstVertexIter PolyConnectivity::vertices_end() const
{
  return ConstVertexIter(*this, VertexHandle( int(n_vertices()) ));
}

//-----------------------------------------------------------------------------
PolyConnectivity::HalfedgeIter PolyConnectivity::halfedges_begin()
{
  return HalfedgeIter(*this, HalfedgeHandle(0));
}

//-----------------------------------------------------------------------------
PolyConnectivity::ConstHalfedgeIter PolyConnectivity::halfedges_begin() const
{
  return ConstHalfedgeIter(*this, HalfedgeHandle(0));
}

//-----------------------------------------------------------------------------
PolyConnectivity::HalfedgeIter PolyConnectivity::halfedges_end()
{
  return HalfedgeIter(*this, HalfedgeHandle(int(n_halfedges())));
}

//-----------------------------------------------------------------------------
PolyConnectivity::ConstHalfedgeIter PolyConnectivity::halfedges_end() const
{
  return ConstHalfedgeIter(*this, HalfedgeHandle(int(n_halfedges())));
}

//-----------------------------------------------------------------------------
PolyConnectivity::EdgeIter PolyConnectivity::edges_begin()
{
  return EdgeIter(*this, EdgeHandle(0));
}

//-----------------------------------------------------------------------------
PolyConnectivity::ConstEdgeIter PolyConnectivity::edges_begin() const
{
  return ConstEdgeIter(*this, EdgeHandle(0));
}

//-----------------------------------------------------------------------------
PolyConnectivity::EdgeIter PolyConnectivity::edges_end()
{
  return EdgeIter(*this, EdgeHandle(int(n_edges())));
}

//-----------------------------------------------------------------------------
PolyConnectivity::ConstEdgeIter PolyConnectivity::edges_end() const
{
  return ConstEdgeIter(*this, EdgeHandle(int(n_edges())));
}

//-----------------------------------------------------------------------------
PolyConnectivity::FaceIter PolyConnectivity::faces_begin()
{
  return FaceIter(*this, FaceHandle(0));
}

//-----------------------------------------------------------------------------
PolyConnectivity::ConstFaceIter PolyConnectivity::faces_begin() const
{
  return ConstFaceIter(*this, FaceHandle(0));
}

//-----------------------------------------------------------------------------
PolyConnectivity::FaceIter PolyConnectivity::faces_end()
{
  return FaceIter(*this, FaceHandle(int(n_faces())));
}

//-----------------------------------------------------------------------------
PolyConnectivity::ConstFaceIter PolyConnectivity::faces_end() const
{
  return ConstFaceIter(*this, FaceHandle(int(n_faces())));
}

//-----------------------------------------------------------------------------
void PolyConnectivity::collapse(HalfedgeHandle _hh)
{
  HalfedgeHandle h0 = _hh;
  HalfedgeHandle h1 = next_halfedge_handle(h0);
  HalfedgeHandle o0 = opposite_halfedge_handle(h0);
  HalfedgeHandle o1 = next_halfedge_handle(o0);

  // remove edge
  collapse_edge(h0);

  // remove loops
  if (next_halfedge_handle(next_halfedge_handle(h1)) == h1)
    collapse_loop(next_halfedge_handle(h1));
  if (next_halfedge_handle(next_halfedge_handle(o1)) == o1)
    collapse_loop(o1);
}

//-----------------------------------------------------------------------------
void PolyConnectivity::collapse_edge(HalfedgeHandle _hh)
{
  HalfedgeHandle  h  = _hh;
  HalfedgeHandle  hn = next_halfedge_handle(h);
  HalfedgeHandle  hp = prev_halfedge_handle(h);

  HalfedgeHandle  o  = opposite_halfedge_handle(h);
  HalfedgeHandle  on = next_halfedge_handle(o);
  HalfedgeHandle  op = prev_halfedge_handle(o);

  FaceHandle      fh = face_handle(h);
  FaceHandle      fo = face_handle(o);

  VertexHandle    vh = to_vertex_handle(h);
  VertexHandle    vo = to_vertex_handle(o);



  // halfedge -> vertex
  for (VertexIHalfedgeIter vih_it(vih_iter(vo)); vih_it.is_valid(); ++vih_it)
    set_vertex_handle(*vih_it, vh);


  // halfedge -> halfedge
  set_next_halfedge_handle(hp, hn);
  set_next_halfedge_handle(op, on);


  // face -> halfedge
  if (fh.is_valid())  set_halfedge_handle(fh, hn);
  if (fo.is_valid())  set_halfedge_handle(fo, on);


  // vertex -> halfedge
  if (halfedge_handle(vh) == o)  set_halfedge_handle(vh, hn);
  adjust_outgoing_halfedge(vh);
  set_isolated(vo);

  // delete stuff
  status(edge_handle(h)).set_deleted(true);
  status(vo).set_deleted(true);
  if (has_halfedge_status())
  {
    status(h).set_deleted(true);
    status(o).set_deleted(true);
  }
}

//-----------------------------------------------------------------------------
void PolyConnectivity::collapse_loop(HalfedgeHandle _hh)
{
  HalfedgeHandle  h0 = _hh;
  HalfedgeHandle  h1 = next_halfedge_handle(h0);

  HalfedgeHandle  o0 = opposite_halfedge_handle(h0);
  HalfedgeHandle  o1 = opposite_halfedge_handle(h1);

  VertexHandle    v0 = to_vertex_handle(h0);
  VertexHandle    v1 = to_vertex_handle(h1);

  FaceHandle      fh = face_handle(h0);
  FaceHandle      fo = face_handle(o0);



  // is it a loop ?
  assert ((next_halfedge_handle(h1) == h0) && (h1 != o0));


  // halfedge -> halfedge
  set_next_halfedge_handle(h1, next_halfedge_handle(o0));
  set_next_halfedge_handle(prev_halfedge_handle(o0), h1);


  // halfedge -> face
  set_face_handle(h1, fo);


  // vertex -> halfedge
  set_halfedge_handle(v0, h1);  adjust_outgoing_halfedge(v0);
  set_halfedge_handle(v1, o1);  adjust_outgoing_halfedge(v1);


  // face -> halfedge
  if (fo.is_valid() && halfedge_handle(fo) == o0)
  {
    set_halfedge_handle(fo, h1);
  }

  // delete stuff
  if (fh.is_valid())  
  {
    set_halfedge_handle(fh, InvalidHalfedgeHandle);
    status(fh).set_deleted(true);
  }
  status(edge_handle(h0)).set_deleted(true);
  if (has_halfedge_status())
  {
    status(h0).set_deleted(true);
    status(o0).set_deleted(true);
  }
}

//-----------------------------------------------------------------------------
bool PolyConnectivity::is_simple_link(EdgeHandle _eh) const
{
  HalfedgeHandle heh0 = halfedge_handle(_eh, 0);
  HalfedgeHandle heh1 = halfedge_handle(_eh, 1);
  
  //FaceHandle fh0 = face_handle(heh0);//fh0 or fh1 might be a invalid,
  FaceHandle fh1 = face_handle(heh1);//i.e., representing the boundary
  
  HalfedgeHandle next_heh = next_halfedge_handle(heh0);
  while (next_heh != heh0)
  {//check if there are no other edges shared between fh0 & fh1
    if (opposite_face_handle(next_heh) == fh1)
    {
      return false;
    }
    next_heh = next_halfedge_handle(next_heh);
  }
  return true;
}

//-----------------------------------------------------------------------------
bool PolyConnectivity::is_simply_connected(FaceHandle _fh) const
{
  std::set<FaceHandle> nb_fhs;
  for (ConstFaceFaceIter cff_it = cff_iter(_fh); cff_it.is_valid(); ++cff_it)
  {
    if (nb_fhs.find(*cff_it) == nb_fhs.end())
    {
      nb_fhs.insert(*cff_it);
    }
    else
    {//there is more than one link
      return false;
    }
  }
  return true;
}
  
//-----------------------------------------------------------------------------
PolyConnectivity::FaceHandle 
PolyConnectivity::remove_edge(EdgeHandle _eh)
{
  //don't allow "dangling" vertices and edges
  assert(!status(_eh).deleted() && is_simple_link(_eh));
  
  HalfedgeHandle heh0 = halfedge_handle(_eh, 0);
  HalfedgeHandle heh1 = halfedge_handle(_eh, 1);
  
  //deal with the faces
  FaceHandle rem_fh = face_handle(heh0), del_fh = face_handle(heh1);
  if (!del_fh.is_valid())
  {//boundary case - we must delete the rem_fh
    std::swap(del_fh, rem_fh);
  }
  assert(del_fh.is_valid());
/*  for (FaceHalfedgeIter fh_it = fh_iter(del_fh); fh_it; ++fh_it)
  {//set the face handle of the halfedges of del_fh to point to rem_fh
    set_face_handle(fh_it, rem_fh);  
  } */
  //fix the halfedge relations
  HalfedgeHandle prev_heh0 = prev_halfedge_handle(heh0);
  HalfedgeHandle prev_heh1 = prev_halfedge_handle(heh1);

  HalfedgeHandle next_heh0 = next_halfedge_handle(heh0);
  HalfedgeHandle next_heh1 = next_halfedge_handle(heh1);
  
  set_next_halfedge_handle(prev_heh0, next_heh1);
  set_next_halfedge_handle(prev_heh1, next_heh0);
  //correct outgoing vertex handles for the _eh vertices (if needed)
  VertexHandle vh0 = to_vertex_handle(heh0);
  VertexHandle vh1 = to_vertex_handle(heh1);
  
  if (halfedge_handle(vh0) == heh1)
  {
    set_halfedge_handle(vh0, next_heh0);
  }
  if (halfedge_handle(vh1) == heh0)
  {
    set_halfedge_handle(vh1, next_heh1);
  }
  
  //correct the hafledge handle of rem_fh if needed and preserve its first vertex
  if (halfedge_handle(rem_fh) == heh0)
  {//rem_fh is the face at heh0
    set_halfedge_handle(rem_fh, prev_heh1);
  }
  else if (halfedge_handle(rem_fh) == heh1)
  {//rem_fh is the face at heh1
    set_halfedge_handle(rem_fh, prev_heh0);
  }
  for (FaceHalfedgeIter fh_it = fh_iter(rem_fh); fh_it.is_valid(); ++fh_it)
  {//set the face handle of the halfedges of del_fh to point to rem_fh
    set_face_handle(*fh_it, rem_fh);
  } 
  
  status(_eh).set_deleted(true);  
  status(del_fh).set_deleted(true);  
  return rem_fh;//returns the remaining face handle
}

//-----------------------------------------------------------------------------
void PolyConnectivity::reinsert_edge(EdgeHandle _eh)
{
  //this does not work without prev_halfedge_handle
  assert_compile(sizeof(Halfedge) == sizeof(Halfedge_with_prev));
  //shoudl be deleted  
  assert(status(_eh).deleted());
  status(_eh).set_deleted(false);  
  
  HalfedgeHandle heh0 = halfedge_handle(_eh, 0);
  HalfedgeHandle heh1 = halfedge_handle(_eh, 1);
  FaceHandle rem_fh = face_handle(heh0), del_fh = face_handle(heh1);
  if (!del_fh.is_valid())
  {//boundary case - we must delete the rem_fh
    std::swap(del_fh, rem_fh);
  }
  assert(status(del_fh).deleted());
  status(del_fh).set_deleted(false); 
  
  //restore halfedge relations
  HalfedgeHandle prev_heh0 = prev_halfedge_handle(heh0);
  HalfedgeHandle prev_heh1 = prev_halfedge_handle(heh1);

  HalfedgeHandle next_heh0 = next_halfedge_handle(heh0);
  HalfedgeHandle next_heh1 = next_halfedge_handle(heh1);
  
  set_next_halfedge_handle(prev_heh0, heh0);
  set_prev_halfedge_handle(next_heh0, heh0);
  
  set_next_halfedge_handle(prev_heh1, heh1);
  set_prev_halfedge_handle(next_heh1, heh1);
  
  for (FaceHalfedgeIter fh_it = fh_iter(del_fh); fh_it.is_valid(); ++fh_it)
  {//reassign halfedges to del_fh  
    set_face_handle(*fh_it, del_fh);
  }
   
  if (face_handle(halfedge_handle(rem_fh)) == del_fh)
  {//correct the halfedge handle of rem_fh
    if (halfedge_handle(rem_fh) == prev_heh0)
    {//rem_fh is the face at heh1
      set_halfedge_handle(rem_fh, heh1);
    }
    else
    {//rem_fh is the face at heh0
      assert(halfedge_handle(rem_fh) == prev_heh1);
      set_halfedge_handle(rem_fh, heh0);
    }
  }
}

//-----------------------------------------------------------------------------
PolyConnectivity::HalfedgeHandle
PolyConnectivity::insert_edge(HalfedgeHandle _prev_heh, HalfedgeHandle _next_heh)
{
  assert(face_handle(_prev_heh) == face_handle(_next_heh));//only the manifold case
  assert(next_halfedge_handle(_prev_heh) != _next_heh);//this can not be done
  VertexHandle vh0 = to_vertex_handle(_prev_heh);
  VertexHandle vh1 = from_vertex_handle(_next_heh);
  //create the link between vh0 and vh1
  HalfedgeHandle heh0 = new_edge(vh0, vh1);
  HalfedgeHandle heh1 = opposite_halfedge_handle(heh0);
  HalfedgeHandle next_prev_heh = next_halfedge_handle(_prev_heh);
  HalfedgeHandle prev_next_heh = prev_halfedge_handle(_next_heh);
  set_next_halfedge_handle(_prev_heh, heh0);
  set_next_halfedge_handle(heh0, _next_heh);
  set_next_halfedge_handle(prev_next_heh, heh1);
  set_next_halfedge_handle(heh1, next_prev_heh);
  
  //now set the face handles - the new face is assigned to heh0
  FaceHandle new_fh = new_face();
  set_halfedge_handle(new_fh, heh0);
  for (FaceHalfedgeIter fh_it = fh_iter(new_fh); fh_it.is_valid(); ++fh_it)
  {
    set_face_handle(*fh_it, new_fh);
  }  
  FaceHandle old_fh = face_handle(next_prev_heh);
  set_face_handle(heh1, old_fh);   
  if (old_fh.is_valid() && face_handle(halfedge_handle(old_fh)) == new_fh)
  {//fh pointed to one of the halfedges now assigned to new_fh
    set_halfedge_handle(old_fh, heh1);
  }
  adjust_outgoing_halfedge(vh0);  
  adjust_outgoing_halfedge(vh1);  
  return heh0;
}

//-----------------------------------------------------------------------------
void PolyConnectivity::triangulate(FaceHandle _fh)
{
  /*
    Split an arbitrary face into triangles by connecting
    each vertex of fh after its second to vh.

    - fh will remain valid (it will become one of the
      triangles)
    - the halfedge handles of the new triangles will
      point to the old halfedges
  */

  HalfedgeHandle base_heh(halfedge_handle(_fh));
  VertexHandle start_vh = from_vertex_handle(base_heh);
  HalfedgeHandle prev_heh(prev_halfedge_handle(base_heh));
  HalfedgeHandle next_heh(next_halfedge_handle(base_heh));

  while (to_vertex_handle(next_halfedge_handle(next_heh)) != start_vh)
  {
    HalfedgeHandle next_next_heh(next_halfedge_handle(next_heh));

    FaceHandle new_fh = new_face();
    set_halfedge_handle(new_fh, base_heh);

    HalfedgeHandle new_heh = new_edge(to_vertex_handle(next_heh), start_vh);

    set_next_halfedge_handle(base_heh, next_heh);
    set_next_halfedge_handle(next_heh, new_heh);
    set_next_halfedge_handle(new_heh, base_heh);

    set_face_handle(base_heh, new_fh);
    set_face_handle(next_heh, new_fh);
    set_face_handle(new_heh,  new_fh);

    copy_all_properties(prev_heh, new_heh, true);
    copy_all_properties(prev_heh, opposite_halfedge_handle(new_heh), true);
    copy_all_properties(_fh, new_fh, true);

    base_heh = opposite_halfedge_handle(new_heh);
    next_heh = next_next_heh;
  }
  set_halfedge_handle(_fh, base_heh);  //the last face takes the handle _fh

  set_next_halfedge_handle(base_heh, next_heh);
  set_next_halfedge_handle(next_halfedge_handle(next_heh), base_heh);

  set_face_handle(base_heh, _fh);
}

//-----------------------------------------------------------------------------
void PolyConnectivity::triangulate()
{
  /* The iterators will stay valid, even though new faces are added,
     because they are now implemented index-based instead of
     pointer-based.
  */
  FaceIter f_it(faces_begin()), f_end(faces_end());
  for (; f_it!=f_end; ++f_it)
    triangulate(*f_it);
}

//-----------------------------------------------------------------------------
void PolyConnectivity::split(FaceHandle fh, VertexHandle vh)
{
  HalfedgeHandle hend = halfedge_handle(fh);
  HalfedgeHandle hh   = next_halfedge_handle(hend);

  HalfedgeHandle hold = new_edge(to_vertex_handle(hend), vh);

  set_next_halfedge_handle(hend, hold);
  set_face_handle(hold, fh);

  hold = opposite_halfedge_handle(hold);

  while (hh != hend) {

    HalfedgeHandle hnext = next_halfedge_handle(hh);

    FaceHandle fnew = new_face();
    set_halfedge_handle(fnew, hh);

    HalfedgeHandle hnew = new_edge(to_vertex_handle(hh), vh);

    set_next_halfedge_handle(hnew, hold);
    set_next_halfedge_handle(hold, hh);
    set_next_halfedge_handle(hh, hnew);

    set_face_handle(hnew, fnew);
    set_face_handle(hold, fnew);
    set_face_handle(hh,   fnew);

    hold = opposite_halfedge_handle(hnew);

    hh = hnext;
  }

  set_next_halfedge_handle(hold, hend);
  set_next_halfedge_handle(next_halfedge_handle(hend), hold);

  set_face_handle(hold, fh);

  set_halfedge_handle(vh, hold);
}


void PolyConnectivity::split_copy(FaceHandle fh, VertexHandle vh) {

  // Split the given face (fh will still be valid)
  split(fh, vh);

  // Copy the property of the original face to all new faces
  for(VertexFaceIter vf_it =  vf_iter(vh); vf_it.is_valid(); ++vf_it)
    copy_all_properties(fh, *vf_it, true);
}

//-----------------------------------------------------------------------------
uint PolyConnectivity::valence(VertexHandle _vh) const
{
  uint count(0);
  for (ConstVertexVertexIter vv_it=cvv_iter(_vh); vv_it.is_valid(); ++vv_it)
    ++count;
  return count;
}

//-----------------------------------------------------------------------------
uint PolyConnectivity::valence(FaceHandle _fh) const
{
  uint count(0);
  for (ConstFaceVertexIter fv_it=cfv_iter(_fh); fv_it.is_valid(); ++fv_it)
    ++count;
  return count;
}

//-----------------------------------------------------------------------------
void PolyConnectivity::split_edge(EdgeHandle _eh, VertexHandle _vh)
{
  HalfedgeHandle h0 = halfedge_handle(_eh, 0);
  HalfedgeHandle h1 = halfedge_handle(_eh, 1);
  
  VertexHandle vfrom = from_vertex_handle(h0);

  HalfedgeHandle ph0 = prev_halfedge_handle(h0);
  //HalfedgeHandle ph1 = prev_halfedge_handle(h1);
  
  //HalfedgeHandle nh0 = next_halfedge_handle(h0);
  HalfedgeHandle nh1 = next_halfedge_handle(h1);
  
  bool boundary0 = is_boundary(h0);
  bool boundary1 = is_boundary(h1);
  
  //add the new edge
  HalfedgeHandle new_e = new_edge(from_vertex_handle(h0), _vh);
  
  //fix the vertex of the opposite halfedge
  set_vertex_handle(h1, _vh);
  
  //fix the halfedge connectivity
  set_next_halfedge_handle(new_e, h0);
  set_next_halfedge_handle(h1, opposite_halfedge_handle(new_e));
  
  set_next_halfedge_handle(ph0, new_e);
  set_next_halfedge_handle(opposite_halfedge_handle(new_e), nh1);
  
//  set_prev_halfedge_handle(new_e, ph0);
//  set_prev_halfedge_handle(opposite_halfedge_handle(new_e), h1);
  
//  set_prev_halfedge_handle(nh1, opposite_halfedge_handle(new_e));
//  set_prev_halfedge_handle(h0, new_e);
  
  if (!boundary0)
  {
    set_face_handle(new_e, face_handle(h0));
  }
  else
  {
    set_boundary(new_e);
  }
  
  if (!boundary1)
  {
    set_face_handle(opposite_halfedge_handle(new_e), face_handle(h1));
  }
  else
  {
    set_boundary(opposite_halfedge_handle(new_e));
  }

  set_halfedge_handle( _vh, h0 );
  adjust_outgoing_halfedge( _vh );
  
  if (halfedge_handle(vfrom) == h0)
  {
    set_halfedge_handle(vfrom, new_e);
    adjust_outgoing_halfedge( vfrom );
  }
}

//-----------------------------------------------------------------------------

void PolyConnectivity::split_edge_copy(EdgeHandle _eh, VertexHandle _vh)
{
  // Split the edge (handle is kept!)
  split_edge(_eh, _vh);

  // Navigate to the new edge
  EdgeHandle eh0 = edge_handle( next_halfedge_handle( halfedge_handle(_eh, 1) ) );

  // Copy the property from the original to the new edge
  copy_all_properties(_eh, eh0, true);
}

} // namespace OpenMesh

