// Modifications copyright Amazon.com, Inc. or its affiliates.
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




//  CLASS TriMeshT - IMPLEMENTATION

#include <OpenMesh/Core/Mesh/TriConnectivity.hh>

namespace OpenMesh
{

TriConnectivity::FaceHandle
TriConnectivity::add_face(const VertexHandle* _vertex_handles, size_t _vhs_size)
{
  // need at least 3 vertices
  if (_vhs_size < 3) return InvalidFaceHandle;

  /// face is triangle -> ok
  if (_vhs_size == 3)
    return PolyConnectivity::add_face(_vertex_handles, _vhs_size);

  /// face is not a triangle -> triangulate
  else
  {
    //omlog() << "triangulating " << _vhs_size << "_gon\n";

    VertexHandle vhandles[3];
    vhandles[0] = _vertex_handles[0];

    FaceHandle fh;
    unsigned int i(1);
    --_vhs_size;

    while (i < _vhs_size)
    {
      vhandles[1] = _vertex_handles[i];
      vhandles[2] = _vertex_handles[++i];
      fh = PolyConnectivity::add_face(vhandles, 3);
    }

    return fh;
  }
}

//-----------------------------------------------------------------------------

FaceHandle TriConnectivity::add_face(const std::vector<VertexHandle>& _vhandles)
{
  return add_face(&_vhandles.front(), _vhandles.size());
}

//-----------------------------------------------------------------------------


FaceHandle TriConnectivity::add_face(VertexHandle _vh0, VertexHandle _vh1, VertexHandle _vh2)
{
  VertexHandle vhs[3] = { _vh0, _vh1, _vh2 };
  return PolyConnectivity::add_face(vhs, 3);
}

//-----------------------------------------------------------------------------

bool TriConnectivity::is_collapse_ok(HalfedgeHandle v0v1)
{
  // is the edge already deleted?
  if ( status(edge_handle(v0v1)).deleted() )
    return false;

  HalfedgeHandle  v1v0(opposite_halfedge_handle(v0v1));
  VertexHandle    v0(to_vertex_handle(v1v0));
  VertexHandle    v1(to_vertex_handle(v0v1));

  // are vertices already deleted ?
  if (status(v0).deleted() || status(v1).deleted())
    return false;

  VertexHandle    vl, vr;
  HalfedgeHandle  h1, h2;

  // the edges v1-vl and vl-v0 must not be both boundary edges
  if (!is_boundary(v0v1))
  {

    h1 = next_halfedge_handle(v0v1);
    h2 = next_halfedge_handle(h1);

    vl = to_vertex_handle(h1);

    if (is_boundary(opposite_halfedge_handle(h1)) &&
        is_boundary(opposite_halfedge_handle(h2)))
    {
      return false;
    }
  }


  // the edges v0-vr and vr-v1 must not be both boundary edges
  if (!is_boundary(v1v0))
  {

    h1 = next_halfedge_handle(v1v0);
    h2 = next_halfedge_handle(h1);

    vr = to_vertex_handle(h1);

    if (is_boundary(opposite_halfedge_handle(h1)) &&
        is_boundary(opposite_halfedge_handle(h2)))
      return false;
  }

  // if vl and vr are equal or both invalid -> fail
  if (vl == vr) return false;

  VertexVertexIter  vv_it;

  // test intersection of the one-rings of v0 and v1
  for (vv_it = vv_iter(v0); vv_it.is_valid(); ++vv_it)
    status(*vv_it).set_tagged(false);

  for (vv_it = vv_iter(v1); vv_it.is_valid(); ++vv_it)
    status(*vv_it).set_tagged(true);

  for (vv_it = vv_iter(v0); vv_it.is_valid(); ++vv_it)
    if (status(*vv_it).tagged() && *vv_it != vl && *vv_it != vr)
      return false;


  // edge between two boundary vertices should be a boundary edge
  if ( is_boundary(v0) && is_boundary(v1) &&
       !is_boundary(v0v1) && !is_boundary(v1v0))
    return false;

  // passed all tests
  return true;
}

//-----------------------------------------------------------------------------
TriConnectivity::HalfedgeHandle
TriConnectivity::vertex_split(VertexHandle v0, VertexHandle v1,
                              VertexHandle vl, VertexHandle vr)
{
  HalfedgeHandle v1vl, vlv1, vrv1, v0v1;

  // build loop from halfedge v1->vl
  if (vl.is_valid())
  {
    v1vl = find_halfedge(v1, vl);
    assert(v1vl.is_valid());
    vlv1 = insert_loop(v1vl);
  }

  // build loop from halfedge vr->v1
  if (vr.is_valid())
  {
    vrv1 = find_halfedge(vr, v1);
    assert(vrv1.is_valid());
    insert_loop(vrv1);
  }

  // handle boundary cases
  if (!vl.is_valid())
    vlv1 = prev_halfedge_handle(halfedge_handle(v1));
  if (!vr.is_valid())
    vrv1 = prev_halfedge_handle(halfedge_handle(v1));


  // split vertex v1 into edge v0v1
  v0v1 = insert_edge(v0, vlv1, vrv1);


  return v0v1;
}

//-----------------------------------------------------------------------------
TriConnectivity::HalfedgeHandle
TriConnectivity::insert_loop(HalfedgeHandle _hh)
{
  HalfedgeHandle  h0(_hh);
  HalfedgeHandle  o0(opposite_halfedge_handle(h0));

  VertexHandle    v0(to_vertex_handle(o0));
  VertexHandle    v1(to_vertex_handle(h0));

  HalfedgeHandle  h1 = new_edge(v1, v0);
  HalfedgeHandle  o1 = opposite_halfedge_handle(h1);

  FaceHandle      f0 = face_handle(h0);
  FaceHandle      f1 = new_face();

  // halfedge -> halfedge
  set_next_halfedge_handle(prev_halfedge_handle(h0), o1);
  set_next_halfedge_handle(o1, next_halfedge_handle(h0));
  set_next_halfedge_handle(h1, h0);
  set_next_halfedge_handle(h0, h1);

  // halfedge -> face
  set_face_handle(o1, f0);
  set_face_handle(h0, f1);
  set_face_handle(h1, f1);

  // face -> halfedge
  set_halfedge_handle(f1, h0);
  if (f0.is_valid())
    set_halfedge_handle(f0, o1);


  // vertex -> halfedge
  adjust_outgoing_halfedge(v0);
  adjust_outgoing_halfedge(v1);

  return h1;
}

//-----------------------------------------------------------------------------
TriConnectivity::HalfedgeHandle
TriConnectivity::insert_edge(VertexHandle _vh, HalfedgeHandle _h0, HalfedgeHandle _h1)
{
  assert(_h0.is_valid() && _h1.is_valid());

  VertexHandle  v0 = _vh;
  VertexHandle  v1 = to_vertex_handle(_h0);

  assert( v1 == to_vertex_handle(_h1));

  HalfedgeHandle v0v1 = new_edge(v0, v1);
  HalfedgeHandle v1v0 = opposite_halfedge_handle(v0v1);



  // vertex -> halfedge
  set_halfedge_handle(v0, v0v1);
  set_halfedge_handle(v1, v1v0);


  // halfedge -> halfedge
  set_next_halfedge_handle(v0v1, next_halfedge_handle(_h0));
  set_next_halfedge_handle(_h0, v0v1);
  set_next_halfedge_handle(v1v0, next_halfedge_handle(_h1));
  set_next_halfedge_handle(_h1, v1v0);


  // halfedge -> vertex
  for (VertexIHalfedgeIter vih_it(vih_iter(v0)); vih_it.is_valid(); ++vih_it)
    set_vertex_handle(*vih_it, v0);


  // halfedge -> face
  set_face_handle(v0v1, face_handle(_h0));
  set_face_handle(v1v0, face_handle(_h1));


  // face -> halfedge
  if (face_handle(v0v1).is_valid())
    set_halfedge_handle(face_handle(v0v1), v0v1);
  if (face_handle(v1v0).is_valid())
    set_halfedge_handle(face_handle(v1v0), v1v0);


  // vertex -> halfedge
  adjust_outgoing_halfedge(v0);
  adjust_outgoing_halfedge(v1);


  return v0v1;
}

//-----------------------------------------------------------------------------
bool TriConnectivity::is_flip_ok(EdgeHandle _eh) const
{
  // boundary edges cannot be flipped
  if (is_boundary(_eh)) return false;


  HalfedgeHandle hh = halfedge_handle(_eh, 0);
  HalfedgeHandle oh = halfedge_handle(_eh, 1);


  // check if the flipped edge is already present
  // in the mesh

  VertexHandle ah = to_vertex_handle(next_halfedge_handle(hh));
  VertexHandle bh = to_vertex_handle(next_halfedge_handle(oh));

  if (ah == bh)   // this is generally a bad sign !!!
    return false;

  for (ConstVertexVertexIter vvi(*this, ah); vvi.is_valid(); ++vvi)
    if (*vvi == bh)
      return false;

  return true;
}

//-----------------------------------------------------------------------------
void TriConnectivity::flip(EdgeHandle _eh)
{
  // CAUTION : Flipping a halfedge may result in
  // a non-manifold mesh, hence check for yourself
  // whether this operation is allowed or not!
  assert(is_flip_ok(_eh));//let's make it sure it is actually checked
  assert(!is_boundary(_eh));

  HalfedgeHandle a0 = halfedge_handle(_eh, 0);
  HalfedgeHandle b0 = halfedge_handle(_eh, 1);

  HalfedgeHandle a1 = next_halfedge_handle(a0);
  HalfedgeHandle a2 = next_halfedge_handle(a1);

  HalfedgeHandle b1 = next_halfedge_handle(b0);
  HalfedgeHandle b2 = next_halfedge_handle(b1);

  VertexHandle   va0 = to_vertex_handle(a0);
  VertexHandle   va1 = to_vertex_handle(a1);

  VertexHandle   vb0 = to_vertex_handle(b0);
  VertexHandle   vb1 = to_vertex_handle(b1);

  FaceHandle     fa  = face_handle(a0);
  FaceHandle     fb  = face_handle(b0);

  set_vertex_handle(a0, va1);
  set_vertex_handle(b0, vb1);

  set_next_halfedge_handle(a0, a2);
  set_next_halfedge_handle(a2, b1);
  set_next_halfedge_handle(b1, a0);

  set_next_halfedge_handle(b0, b2);
  set_next_halfedge_handle(b2, a1);
  set_next_halfedge_handle(a1, b0);

  set_face_handle(a1, fb);
  set_face_handle(b1, fa);

  set_halfedge_handle(fa, a0);
  set_halfedge_handle(fb, b0);

  if (halfedge_handle(va0) == b0)
    set_halfedge_handle(va0, a1);
  if (halfedge_handle(vb0) == a0)
    set_halfedge_handle(vb0, b1);
}


//-----------------------------------------------------------------------------

void TriConnectivity::split(EdgeHandle _eh, VertexHandle _vh)
{
  HalfedgeHandle h0 = halfedge_handle(_eh, 0);
  HalfedgeHandle o0 = halfedge_handle(_eh, 1);

  VertexHandle   v2 = to_vertex_handle(o0);

  HalfedgeHandle e1 = new_edge(_vh, v2);
  HalfedgeHandle t1 = opposite_halfedge_handle(e1);

  FaceHandle     f0 = face_handle(h0);
  FaceHandle     f3 = face_handle(o0);

  set_halfedge_handle(_vh, h0);
  set_vertex_handle(o0, _vh);

  if (!is_boundary(h0))
  {
    HalfedgeHandle h1 = next_halfedge_handle(h0);
    HalfedgeHandle h2 = next_halfedge_handle(h1);

    VertexHandle v1 = to_vertex_handle(h1);

    HalfedgeHandle e0 = new_edge(_vh, v1);
    HalfedgeHandle t0 = opposite_halfedge_handle(e0);

    FaceHandle f1 = new_face();
    set_halfedge_handle(f0, h0);
    set_halfedge_handle(f1, h2);

    set_face_handle(h1, f0);
    set_face_handle(t0, f0);
    set_face_handle(h0, f0);

    set_face_handle(h2, f1);
    set_face_handle(t1, f1);
    set_face_handle(e0, f1);

    set_next_halfedge_handle(h0, h1);
    set_next_halfedge_handle(h1, t0);
    set_next_halfedge_handle(t0, h0);

    set_next_halfedge_handle(e0, h2);
    set_next_halfedge_handle(h2, t1);
    set_next_halfedge_handle(t1, e0);
  }
  else
  {
    set_next_halfedge_handle(prev_halfedge_handle(h0), t1);
    set_next_halfedge_handle(t1, h0);
    // halfedge handle of _vh already is h0
  }


  if (!is_boundary(o0))
  {
    HalfedgeHandle o1 = next_halfedge_handle(o0);
    HalfedgeHandle o2 = next_halfedge_handle(o1);

    VertexHandle v3 = to_vertex_handle(o1);

    HalfedgeHandle e2 = new_edge(_vh, v3);
    HalfedgeHandle t2 = opposite_halfedge_handle(e2);

    FaceHandle f2 = new_face();
    set_halfedge_handle(f2, o1);
    set_halfedge_handle(f3, o0);

    set_face_handle(o1, f2);
    set_face_handle(t2, f2);
    set_face_handle(e1, f2);

    set_face_handle(o2, f3);
    set_face_handle(o0, f3);
    set_face_handle(e2, f3);

    set_next_halfedge_handle(e1, o1);
    set_next_halfedge_handle(o1, t2);
    set_next_halfedge_handle(t2, e1);

    set_next_halfedge_handle(o0, e2);
    set_next_halfedge_handle(e2, o2);
    set_next_halfedge_handle(o2, o0);
  }
  else
  {
    set_next_halfedge_handle(e1, next_halfedge_handle(o0));
    set_next_halfedge_handle(o0, e1);
    set_halfedge_handle(_vh, e1);
  }

  if (halfedge_handle(v2) == h0)
    set_halfedge_handle(v2, t1);
}

//-----------------------------------------------------------------------------

void TriConnectivity::split_copy(EdgeHandle _eh, VertexHandle _vh)
{
  const VertexHandle v0 = to_vertex_handle(halfedge_handle(_eh, 0));
  const VertexHandle v1 = to_vertex_handle(halfedge_handle(_eh, 1));

  const auto nf = n_faces(); // lumberyard-change

  // Split the halfedge ( handle will be preserved)
  split(_eh, _vh);

  // Copy the properties of the original edge to all neighbor edges that
  // have been created
  for(VEIter ve_it = ve_iter(_vh); ve_it.is_valid(); ++ve_it)
    copy_all_properties(_eh, *ve_it, true);

  for (auto vh : {v0, v1})
  {
    // get the halfedge pointing from new vertex to old vertex
    const HalfedgeHandle h = find_halfedge(_vh, vh);
    if (!is_boundary(h)) // for boundaries there are no faces whose properties need to be copied
    {
      FaceHandle fh0 = face_handle(h);
      FaceHandle fh1 = face_handle(opposite_halfedge_handle(prev_halfedge_handle(h)));
      if (fh0.idx() >= nf) // is fh0 the new face?
        std::swap(fh0, fh1);

      // copy properties from old face to new face
      copy_all_properties(fh0, fh1, true);
    }
  }
}

}// namespace OpenMesh
