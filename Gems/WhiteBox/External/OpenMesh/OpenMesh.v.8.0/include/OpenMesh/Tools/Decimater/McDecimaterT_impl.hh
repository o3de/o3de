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


/** \file McDecimaterT_impl.hh
 */

//=============================================================================
//
//  CLASS McDecimaterT - IMPLEMENTATION
//
//=============================================================================
#define OPENMESH_MULTIPLE_CHOICE_DECIMATER_DECIMATERT_CC

//== INCLUDES =================================================================

#include <OpenMesh/Tools/Decimater/McDecimaterT.hh>

#include <vector>
#if defined(OM_CC_MIPS)
#  include <float.h>
#else
#  include <cfloat>
#endif

#ifdef WIN32
# include <OpenMesh/Core/Utils/RandomNumberGenerator.hh>
#endif

//== NAMESPACE ===============================================================

namespace OpenMesh {
namespace Decimater {

//== IMPLEMENTATION ==========================================================

template<class Mesh>
McDecimaterT<Mesh>::McDecimaterT(Mesh& _mesh) :
  BaseDecimaterT<Mesh>(_mesh),
    mesh_(_mesh), randomSamples_(10) {

  // default properties
  mesh_.request_vertex_status();
  mesh_.request_halfedge_status();
  mesh_.request_edge_status();
  mesh_.request_face_status();

}

//-----------------------------------------------------------------------------

template<class Mesh>
McDecimaterT<Mesh>::~McDecimaterT() {
  // default properties
  mesh_.release_vertex_status();
  mesh_.release_edge_status();
  mesh_.release_halfedge_status();
  mesh_.release_face_status();

}

//-----------------------------------------------------------------------------
template<class Mesh>
size_t McDecimaterT<Mesh>::decimate(size_t _n_collapses) {

  if (!this->is_initialized())
    return 0;

  unsigned int n_collapses(0);

  bool collapsesUnchanged = false;
  // old n_collapses in order to check for convergence
  unsigned int oldCollapses = 0;
  // number of iterations where no new collapses where
  // performed in a row
  unsigned int noCollapses = 0;

#ifdef WIN32
  RandomNumberGenerator randGen(mesh_.n_halfedges());
#endif

  const bool update_normals = mesh_.has_face_normals();

  while ( n_collapses <  _n_collapses) {

    if (noCollapses > 20) {
      omlog() << "[McDecimater] : no collapses performed in over 20 iterations in a row\n";
      break;
    }

    // Optimal id and value will be collected during the random sampling
    typename Mesh::HalfedgeHandle bestHandle(-1);
    typename Mesh::HalfedgeHandle tmpHandle(-1);
    double bestEnergy = FLT_MAX;
    double energy = FLT_MAX;

    // Generate random samples for collapses
    for ( int i = 0; i < (int)randomSamples_; ++i) {

      // Random halfedge handle
#ifdef WIN32
      tmpHandle = typename Mesh::HalfedgeHandle(int(randGen.getRand() * double(mesh_.n_halfedges() - 1.0)) );
#else
      tmpHandle = typename Mesh::HalfedgeHandle( (double(rand()) / double(RAND_MAX) ) * double(mesh_.n_halfedges()-1) );
#endif

      // if it is not deleted, we analyse it
      if ( ! mesh_.status(tmpHandle).deleted()  ) {

        CollapseInfo ci(mesh_, tmpHandle);

        // Check if legal we analyze the priority of this collapse operation
        if (this->is_collapse_legal(ci)) {
            energy = this->collapse_priority(ci);

            if (energy != ModBaseT<Mesh>::ILLEGAL_COLLAPSE) {
              // Check if the current samples energy is better than any energy before
              if ( energy < bestEnergy ) {
                bestEnergy = energy;
                bestHandle = tmpHandle;
              }
            }
        } else {
          continue;
        }
      }

    }

    // Found the best energy?
    if ( bestEnergy != FLT_MAX ) {

      // setup collapse info
      CollapseInfo ci(mesh_, bestHandle);

      // check topological correctness AGAIN !
      if (!this->is_collapse_legal(ci))
        continue;

      // pre-processing
      this->preprocess_collapse(ci);

      // perform collapse
      mesh_.collapse(bestHandle);
      ++n_collapses;

      // store current collapses state
      oldCollapses = n_collapses;
      noCollapses = 0;
      collapsesUnchanged = false;

      // update triangle normals
      if (update_normals)
      {
        typename Mesh::VertexFaceIter vf_it = mesh_.vf_iter(ci.v1);
        for (; vf_it.is_valid(); ++vf_it)
          if (!mesh_.status(*vf_it).deleted())
            mesh_.set_normal(*vf_it, mesh_.calc_face_normal(*vf_it));
      }

      // post-process collapse
      this->postprocess_collapse(ci);

      // notify observer and stop if the observer requests it
      if (!this->notify_observer(n_collapses))
          return n_collapses;

    } else {
      if (oldCollapses == n_collapses) {
        if (collapsesUnchanged == false) {
          noCollapses = 1;
          collapsesUnchanged = true;
        } else {
          noCollapses++;
        }
      }
    }

  }

  // DON'T do garbage collection here! It's up to the application.
  return n_collapses;
}

//-----------------------------------------------------------------------------

template<class Mesh>
size_t McDecimaterT<Mesh>::decimate_to_faces(size_t _nv, size_t _nf) {

  if (!this->is_initialized())
    return 0;

  // check if no vertex or face contraints were set
  if ( (_nv == 0) && (_nf == 1) )
    return decimate_constraints_only(1.0);

  size_t nv = mesh_.n_vertices();
  size_t nf = mesh_.n_faces();
  unsigned int n_collapses(0);


  bool collapsesUnchanged = false;
  // old n_collapses in order to check for convergence
  unsigned int oldCollapses = 0;
  // number of iterations where no new collapses where
  // performed in a row
  unsigned int noCollapses = 0;

#ifdef WIN32
  RandomNumberGenerator randGen(mesh_.n_halfedges());
#endif

  const bool update_normals = mesh_.has_face_normals();

  while ((_nv < nv) && (_nf < nf)) {

    if (noCollapses > 20) {
      omlog() << "[McDecimater] : no collapses performed in over 20 iterations in a row\n";
      break;
    }

    // Optimal id and value will be collected during the random sampling
    typename Mesh::HalfedgeHandle bestHandle(-1);
    typename Mesh::HalfedgeHandle tmpHandle(-1);
    double bestEnergy = FLT_MAX;
    double energy = FLT_MAX;

    // Generate random samples for collapses
    for (int i = 0; i < (int) randomSamples_; ++i) {

      // Random halfedge handle
#ifdef WIN32
      tmpHandle = typename Mesh::HalfedgeHandle(int(randGen.getRand() * double(mesh_.n_halfedges() - 1.0)) );
#else
      tmpHandle = typename Mesh::HalfedgeHandle( ( double(rand()) / double(RAND_MAX) ) * double(mesh_.n_halfedges() - 1));
#endif

      // if it is not deleted, we analyse it
      if (!mesh_.status(tmpHandle).deleted()) {

        CollapseInfo ci(mesh_, tmpHandle);

        // Check if legal we analyze the priority of this collapse operation
        if (this->is_collapse_legal(ci)) {
            energy = this->collapse_priority(ci);

            if (energy != ModBaseT<Mesh>::ILLEGAL_COLLAPSE) {
              // Check if the current samples energy is better than any energy before
              if (energy < bestEnergy) {
                bestEnergy = energy;
                bestHandle = tmpHandle;
              }
            }
        } else {
          continue;
        }
      }

    }

    // Found the best energy?
    if ( bestEnergy != FLT_MAX ) {

      // setup collapse info
      CollapseInfo ci(mesh_, bestHandle);

      // check topological correctness AGAIN !
      if (!this->is_collapse_legal(ci))
        continue;

      // adjust complexity in advance (need boundary status)

      // One vertex is killed by the collapse
      --nv;

      // If we are at a boundary, one face is lost,
      // otherwise two
      if (mesh_.is_boundary(ci.v0v1) || mesh_.is_boundary(ci.v1v0))
        --nf;
      else
        nf -= 2;

      // pre-processing
      this->preprocess_collapse(ci);

      // perform collapse
      mesh_.collapse(bestHandle);
      ++n_collapses;

      // store current collapses state
      oldCollapses = n_collapses;
      noCollapses = 0;
      collapsesUnchanged = false;

      if (update_normals)
      {
        // update triangle normals
        typename Mesh::VertexFaceIter vf_it = mesh_.vf_iter(ci.v1);
        for (; vf_it.is_valid(); ++vf_it)
          if (!mesh_.status(*vf_it).deleted())
            mesh_.set_normal(*vf_it, mesh_.calc_face_normal(*vf_it));
      }

      // post-process collapse
      this->postprocess_collapse(ci);

      // notify observer and stop if the observer requests it
      if (!this->notify_observer(n_collapses))
          return n_collapses;

    } else {
      if (oldCollapses == n_collapses) {
        if (collapsesUnchanged == false) {
          noCollapses = 1;
          collapsesUnchanged = true;
        } else {
          noCollapses++;
        }
      }
    }

  }

  // DON'T do garbage collection here! It's up to the application.
  return n_collapses;
}

//-----------------------------------------------------------------------------

template<class Mesh>
size_t McDecimaterT<Mesh>::decimate_constraints_only(float _factor) {

  if (!this->is_initialized())
    return 0;

  if (_factor < 1.0)
    this->set_error_tolerance_factor(_factor);

  unsigned int n_collapses(0);
  size_t       nv = mesh_.n_vertices();
  size_t       nf = mesh_.n_faces();

  bool lastCollapseIllegal = false;
  // number of illegal collapses that occurred in a row
  unsigned int illegalCollapses = 0;

  bool collapsesUnchanged = false;
  // old n_collapses in order to check for convergence
  unsigned int oldCollapses = 0;
  // number of iterations where no new collapses where
  // performed in a row
  unsigned int noCollapses = 0;

  double energy     = FLT_MAX;
  double bestEnergy = FLT_MAX;

#ifdef WIN32
  RandomNumberGenerator randGen(mesh_.n_halfedges());
#endif

  while ((noCollapses <= 50) && (illegalCollapses <= 50) && (nv > 0) && (nf > 1)) {

    // Optimal id and value will be collected during the random sampling
    typename Mesh::HalfedgeHandle bestHandle(-1);
    typename Mesh::HalfedgeHandle tmpHandle(-1);
    bestEnergy = FLT_MAX;


#ifndef WIN32
    const double randomNormalizer = (1.0 / RAND_MAX) * (mesh_.n_halfedges() - 1);
#endif

    // Generate random samples for collapses
    for (int i = 0; i < (int) randomSamples_; ++i) {

      // Random halfedge handle
#ifdef WIN32
      tmpHandle = typename Mesh::HalfedgeHandle(int(randGen.getRand()  * double(mesh_.n_halfedges() - 1.0)) );
#else
      tmpHandle = typename Mesh::HalfedgeHandle(int(rand() * randomNormalizer ) );
#endif

      // if it is not deleted, we analyze it
      if (!mesh_.status(mesh_.edge_handle(tmpHandle)).deleted()) {

        CollapseInfo ci(mesh_, tmpHandle);

        // Check if legal we analyze the priority of this collapse operation
        if (this->is_collapse_legal(ci)) {

            energy = this->collapse_priority(ci);

            if (energy == ModBaseT<Mesh>::ILLEGAL_COLLAPSE) {
              if (lastCollapseIllegal) {
                illegalCollapses++;
              } else {
                illegalCollapses = 1;
                lastCollapseIllegal = true;
              }
            } else {

              illegalCollapses = 0;
              lastCollapseIllegal = false;

              // Check if the current samples energy is better than any energy before
              if (energy < bestEnergy) {
                bestEnergy = energy;
                bestHandle = tmpHandle;
              }
            }
        } else {

          continue;
        }
      }

    }



    // Found the best energy?
    if ( bestEnergy != FLT_MAX ) {

      // setup collapse info
      CollapseInfo ci(mesh_, bestHandle);

      // check topological correctness AGAIN !
      if (!this->is_collapse_legal(ci))
        continue;

      // adjust complexity in advance (need boundary status)

      // One vertex is killed by the collapse
      --nv;

      // If we are at a boundary, one face is lost,
      // otherwise two
      if (mesh_.is_boundary(ci.v0v1) || mesh_.is_boundary(ci.v1v0))
        --nf;
      else
        nf -= 2;

      // pre-processing
      this->preprocess_collapse(ci);

      // perform collapse
      mesh_.collapse(bestHandle);
      ++n_collapses;

      // store current collapses state
      oldCollapses = n_collapses;
      noCollapses = 0;
      collapsesUnchanged = false;


      // update triangle normals
      typename Mesh::VertexFaceIter vf_it = mesh_.vf_iter(ci.v1);
      for (; vf_it.is_valid(); ++vf_it)
        if (!mesh_.status(*vf_it).deleted())
          mesh_.set_normal(*vf_it, mesh_.calc_face_normal(*vf_it));

      // post-process collapse
      this->postprocess_collapse(ci);

      // notify observer and stop if the observer requests it
      if (!this->notify_observer(n_collapses))
          return n_collapses;

    } else {
      if (oldCollapses == n_collapses) {
        if (collapsesUnchanged == false) {
          noCollapses = 1;
          collapsesUnchanged = true;
        } else {
          noCollapses++;
        }
      }
    }

  }

  if (_factor < 1.0)
    this->set_error_tolerance_factor(1.0);

  // DON'T do garbage collection here! It's up to the application.
  return n_collapses;

}

//=============================================================================
}// END_NS_MC_DECIMATER
} // END_NS_OPENMESH
//=============================================================================

