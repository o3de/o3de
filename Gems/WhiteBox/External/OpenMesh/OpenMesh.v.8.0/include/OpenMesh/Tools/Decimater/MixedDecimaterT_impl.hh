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


/** \file MixedDecimaterT_impl.hh
*/

//=============================================================================
//
//  CLASS MixedDecimaterT - IMPLEMENTATION
//
//=============================================================================
#define OPENMESH_MIXED_DECIMATER_DECIMATERT_CC

//== INCLUDES =================================================================

#include <OpenMesh/Tools/Decimater/MixedDecimaterT.hh>

#include <vector>
#if defined(OM_CC_MIPS)
#  include <float.h>
#else
#  include <cfloat>
#endif

//== NAMESPACE ===============================================================
namespace OpenMesh {
namespace Decimater {

//== IMPLEMENTATION ==========================================================

template<class Mesh>
MixedDecimaterT<Mesh>::MixedDecimaterT(Mesh& _mesh) :
  BaseDecimaterT<Mesh>(_mesh),McDecimaterT<Mesh>(_mesh), DecimaterT<Mesh>(_mesh) {

}

//-----------------------------------------------------------------------------

template<class Mesh>
MixedDecimaterT<Mesh>::~MixedDecimaterT() {

}

//-----------------------------------------------------------------------------
template<class Mesh>
size_t MixedDecimaterT<Mesh>::decimate(const size_t _n_collapses, const float _mc_factor) {

  if (_mc_factor > 1.0)
    return 0;

  size_t n_collapses_mc = static_cast<size_t>(_mc_factor*_n_collapses);
  size_t n_collapses_inc = static_cast<size_t>(_n_collapses - n_collapses_mc);

  size_t r_collapses = 0;
  if (_mc_factor > 0.0)
    r_collapses = McDecimaterT<Mesh>::decimate(n_collapses_mc);

  // returns, if the previous steps were aborted by the observer
  if (this->observer() && this->observer()->abort())
      return r_collapses;

  if (_mc_factor < 1.0)
    r_collapses += DecimaterT<Mesh>::decimate(n_collapses_inc);

  return r_collapses;

}

template<class Mesh>
size_t MixedDecimaterT<Mesh>::decimate_to_faces(const size_t  _n_vertices,const size_t _n_faces, const float _mc_factor ){

  if (_mc_factor > 1.0)
    return 0;

  std::size_t r_collapses = 0;
  if (_mc_factor > 0.0)
  {
    bool constraintsOnly = (_n_vertices == 0) && (_n_faces == 1);
    if (!constraintsOnly) {
      size_t mesh_faces = this->mesh().n_faces();
      size_t mesh_vertices = this->mesh().n_vertices();
      //reduce the mesh only for _mc_factor
      size_t n_vertices_mc = static_cast<size_t>(mesh_vertices - _mc_factor * (mesh_vertices - _n_vertices));
      size_t n_faces_mc = static_cast<size_t>(mesh_faces - _mc_factor * (mesh_faces - _n_faces));

      r_collapses = McDecimaterT<Mesh>::decimate_to_faces(n_vertices_mc, n_faces_mc);
    } else {

      const size_t samples = this->samples();

      // MinimalSample count for the McDecimater
      const size_t min = 2;

      // Maximal number of samples for the McDecimater
      const size_t max = samples;

      // Number of incremental steps
      const size_t steps = 7;

      for ( size_t i = 0; i < steps; ++i ) {

        // Compute number of samples to be used
        size_t samples = int (double( min) + double(i)/(double(steps)-1.0) * (max-2) ) ;

        // We won't allow 1 here, as this is the last step in the incremental part
        float decimaterLevel = (float(i + 1)) * _mc_factor / (float(steps) );

        this->set_samples(samples);
        r_collapses += McDecimaterT<Mesh>::decimate_constraints_only(decimaterLevel);
      }
    }
  }

  //Update the mesh::n_vertices function, otherwise the next Decimater function will delete too much
  this->mesh().garbage_collection();

  // returns, if the previous steps were aborted by the observer
  if (this->observer() && this->observer()->abort())
      return r_collapses;

  //reduce the rest of the mesh
  if (_mc_factor < 1.0) {
    r_collapses += DecimaterT<Mesh>::decimate_to_faces(_n_vertices,_n_faces);
  }


  return r_collapses;
}

//=============================================================================
}// END_NS_MC_DECIMATER
} // END_NS_OPENMESH
//=============================================================================
