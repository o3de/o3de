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




#define OPENMESH_MESHCHECKER_C


//== INCLUDES =================================================================


#include <OpenMesh/Tools/Utils/MeshCheckerT.hh>


//== NAMESPACES ============================================================== 


namespace OpenMesh {
namespace Utils {

//== IMPLEMENTATION ========================================================== 


template <class Mesh>
bool 
MeshCheckerT<Mesh>::
check(unsigned int _targets, std::ostream& _os)
{
  bool  ok(true);



  //--- vertex checks ---

  if (_targets & CHECK_VERTICES)
  {
    typename Mesh::ConstVertexIter v_it(mesh_.vertices_begin()), v_end(mesh_.vertices_end());
    typename Mesh::VertexHandle    vh;
    typename Mesh::ConstVertexVertexCWIter vv_it;
    typename Mesh::HalfedgeHandle  heh;
    unsigned int                   count;
    const unsigned int             max_valence(10000);


    for (; v_it != v_end; ++v_it)
    {
      if (!is_deleted(*v_it))
      {
        vh = *v_it;


        /* The outgoing halfedge of a boundary vertex has to be a boundary halfedge */
        heh = mesh_.halfedge_handle(vh);
        if (heh.is_valid() && !mesh_.is_boundary(heh))
        {
          for (typename Mesh::ConstVertexOHalfedgeIter vh_it(mesh_, vh);
              vh_it.is_valid(); ++vh_it)
          {
            if (mesh_.is_boundary(*vh_it))
            {
              _os << "MeshChecker: vertex " << vh
                  << ": outgoing halfedge not on boundary error\n";
              ok = false;
            }
          }
        }


      
        // outgoing halfedge has to refer back to vertex
        if (mesh_.halfedge_handle(vh).is_valid() &&
            mesh_.from_vertex_handle(mesh_.halfedge_handle(vh)) != vh)
        {
          _os << "MeshChecker: vertex " << vh
              << ": outgoing halfedge does not reference vertex\n";
          ok = false;
        }


        // check whether circulators are still in order
        vv_it = mesh_.cvv_cwiter(vh);
        for (count=0; vv_it.is_valid() && (count < max_valence); ++vv_it, ++count) {};
        if (count == max_valence)
        {
          _os << "MeshChecker: vertex " << vh
              << ": ++circulator problem, one ring corrupt\n";
          ok = false;
        }
        vv_it = mesh_.cvv_cwiter(vh);
        for (count=0; vv_it.is_valid() && (count < max_valence); --vv_it, ++count) {};
        if (count == max_valence)
        {
          _os << "MeshChecker: vertex " << vh
              << ": --circulator problem, one ring corrupt\n";
          ok = false;
        }
      }
    }
  }



  //--- halfedge checks ---

  if (_targets & CHECK_EDGES)
  {
    typename Mesh::ConstHalfedgeIter  h_it(mesh_.halfedges_begin()), 
        h_end(mesh_.halfedges_end());
    typename Mesh::HalfedgeHandle     hh, hstart, hhh;
    size_t                            count, n_halfedges = 2*mesh_.n_edges();

    for (; h_it != h_end; ++h_it)
    {
      if (!is_deleted(mesh_.edge_handle(*h_it)))
      {
        hh = *h_it;


        // degenerated halfedge ?
        if (mesh_.from_vertex_handle(hh) == mesh_.to_vertex_handle(hh))
        {
          _os << "MeshChecker: halfedge " << hh
              << ": to-vertex == from-vertex\n";
          ok = false;
        }


        // next <-> prev check
        if (mesh_.next_halfedge_handle(mesh_.prev_halfedge_handle(hh)) != hh)
        {
          _os << "MeshChecker: halfedge " << hh
              << ": prev->next != this\n";
          ok = false;
        }


        // halfedges should form a cycle
        count=0; hstart=hhh=hh;
        do
        {
          hhh = mesh_.next_halfedge_handle(hhh);
          ++count;
        } while (hhh != hstart && count < n_halfedges);

        if (count == n_halfedges)
        {
          _os << "MeshChecker: halfedges starting from " << hh
              << " do not form a cycle\n";
          ok = false;
        }
      }
    }
  }



  //--- face checks ---

  if (_targets & CHECK_FACES)
  {
    typename Mesh::ConstFaceIter          f_it(mesh_.faces_begin()), 
        f_end(mesh_.faces_end());
    typename Mesh::FaceHandle             fh;
    typename Mesh::ConstFaceHalfedgeIter  fh_it;

    for (; f_it != f_end; ++f_it)
    {
      if (!is_deleted(*f_it))
      {
        fh = *f_it;

        for (fh_it=mesh_.cfh_iter(fh); fh_it.is_valid(); ++fh_it)
        {
          if (mesh_.face_handle(*fh_it) != fh)
          {
            _os << "MeshChecker: face " << fh
                << ": its halfedge does not reference face\n";
            ok = false;
          }
        }
      }
    }
  }



  return ok;
}


//=============================================================================
} // naespace Utils
} // namespace OpenMesh
//=============================================================================
