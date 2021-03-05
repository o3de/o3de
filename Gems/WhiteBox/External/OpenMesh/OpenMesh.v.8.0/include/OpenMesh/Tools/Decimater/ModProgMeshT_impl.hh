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



/** \file ModProgMeshT_impl.hh
 */


//=============================================================================
//
//  CLASS ModProgMeshT - IMPLEMENTATION
//
//=============================================================================

#define OPENMESH_DECIMATER_MODPROGMESH_CC


//== INCLUDES =================================================================

#include <vector>
#include <fstream>
// --------------------
#include <OpenMesh/Core/Utils/vector_cast.hh>
#include <OpenMesh/Core/IO/BinaryHelper.hh>
#include <OpenMesh/Core/Utils/Endian.hh>
// --------------------
#include <OpenMesh/Tools/Decimater/ModProgMeshT.hh>


//== NAMESPACE =============================================================== 

namespace OpenMesh  {
namespace Decimater {

   

//== IMPLEMENTATION ========================================================== 


template <class MeshT>
bool 
ModProgMeshT<MeshT>::
write( const std::string& _ofname )
{
  // sort vertices
  size_t i=0, N=Base::mesh().n_vertices(), n_base_vertices(0), n_base_faces(0);
  std::vector<typename Mesh::VertexHandle>  vhandles(N);


  // base vertices
  typename Mesh::VertexIter 
    v_it=Base::mesh().vertices_begin(), 
    v_end=Base::mesh().vertices_end();

  for (; v_it != v_end; ++v_it)  
    if (!Base::mesh().status(*v_it).deleted())
    {
      vhandles[i] = *v_it;
      Base::mesh().property( idx_, *v_it ) = i;
      ++i;
    }
  n_base_vertices = i;


  // deleted vertices
  typename InfoList::reverse_iterator
    r_it=pmi_.rbegin(), r_end=pmi_.rend();

  for (; r_it!=r_end; ++r_it)  
  { 
    vhandles[i] = r_it->v0;  
    Base::mesh().property( idx_, r_it->v0) = i;  
    ++i; 
  }


  // base faces
  typename Mesh::ConstFaceIter f_it  = Base::mesh().faces_begin(), 
                               f_end = Base::mesh().faces_end();
  for (; f_it != f_end; ++f_it) 
    if (!Base::mesh().status(*f_it).deleted())
      ++n_base_faces;

  // ---------------------------------------- write progressive mesh

  std::ofstream out( _ofname.c_str(), std::ios::binary );
    
  if (!out)
    return false;   

  // always use little endian byte ordering
  bool swap = Endian::local() != Endian::LSB;

  // write header
  out << "ProgMesh";
  IO::store( out, static_cast<unsigned int>(n_base_vertices), swap );//store in 32-bit
  IO::store( out, static_cast<unsigned int>(n_base_faces)   , swap );
  IO::store( out, static_cast<unsigned int>(pmi_.size())    , swap );

  Vec3f p;

  // write base vertices
  for (i=0; i<n_base_vertices; ++i)
  {
    assert (!Base::mesh().status(vhandles[i]).deleted());
    p  = vector_cast< Vec3f >( Base::mesh().point(vhandles[i]) );
    
    IO::store( out, p, swap );
  }
    

  // write base faces
  for (f_it=Base::mesh().faces_begin(); f_it != f_end; ++f_it)  
  {
    if (!Base::mesh().status(*f_it).deleted())
    {
      typename Mesh::ConstFaceVertexIter fv_it(Base::mesh(), *f_it);
      
      IO::store( out, static_cast<unsigned int>(Base::mesh().property( idx_,   *fv_it )) );
      IO::store( out, static_cast<unsigned int>(Base::mesh().property( idx_, *(++fv_it ))) );
      IO::store( out, static_cast<unsigned int>(Base::mesh().property( idx_, *(++fv_it ))) );
    }
  }
  
  
  // write detail info
  for (r_it=pmi_.rbegin(); r_it!=r_end; ++r_it)  
  { 
    // store v0.pos, v1.idx, vl.idx, vr.idx
    IO::store( out, vector_cast<Vec3f>(Base::mesh().point(r_it->v0)));
    IO::store(out, static_cast<unsigned int>(Base::mesh().property(idx_, r_it->v1)));
    IO::store( out, 
        r_it->vl.is_valid() ? static_cast<unsigned int>(Base::mesh().property(idx_, r_it->vl)) : -1);
    IO::store( out, 
        r_it->vr.is_valid() ? static_cast<unsigned int>(Base::mesh().property(idx_, r_it->vr)) : -1);
  }

  return true;
}



//=============================================================================
} // END_NS_DECIMATER
} // END_NS_OPENMESH
//=============================================================================

