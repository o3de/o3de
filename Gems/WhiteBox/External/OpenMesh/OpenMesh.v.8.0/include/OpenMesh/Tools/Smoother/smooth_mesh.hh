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



#ifndef SMOOTH_MESH_HH
#define SMOOTH_MESH_HH

//== INCLUDES =================================================================

#include <OpenMesh/Core/Utils/Property.hh>

//== NAMESPACE ================================================================

namespace OpenMesh { //BEGIN_NS_OPENMESH

template <class _Mesh, class _PropertyHandle>
void smooth_mesh_property(unsigned int _n_iters, _Mesh& _m, _PropertyHandle _pph)
{
  typedef typename _PropertyHandle::Value   Value;

  std::vector<Value> temp_values(_m.n_vertices());

  for (unsigned int i=0; i < _n_iters; ++i)
  {
    for ( typename _Mesh::ConstVertexIter cv_it = _m.vertices_begin();
          cv_it != _m.vertices_end(); ++cv_it)
    {
      unsigned int valence = 0;

      Value& temp_value = temp_values[cv_it->idx()];

      temp_value.vectorize(0);

      for ( typename _Mesh::ConstVertexVertexIter cvv_it = _m.cvv_iter(cv_it);
            cvv_it; ++cvv_it)
      {
        temp_value += _m.property(_pph,cvv_it);
        ++valence;
      }
      if (valence > 0)
      {//guard against isolated vertices
        temp_value *= (typename Value::value_type)(1.0 / valence);
      }
      else
      {
        temp_value = _m.property(_pph, cv_it);
      }
    }

    for ( typename _Mesh::ConstVertexIter cv_it = _m.vertices_begin();
          cv_it != _m.vertices_end(); ++cv_it)
    {
      _m.property(_pph,cv_it) = temp_values[cv_it->idx()];
    }
  }
}

template <class _Mesh>
void smooth_mesh(_Mesh& _m, uint _n_iters)
{
  smooth_mesh_property(_n_iters, _m, _m.points_pph());
}

};//namespace OpenMesh

#endif//SMOOTH_MESH_HH
