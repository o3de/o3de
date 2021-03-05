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



//=============================================================================
//
//  CLASS StripifierT
//
//=============================================================================


#ifndef OPENMESH_STRIPIFIERT_HH
#define OPENMESH_STRIPIFIERT_HH


//== INCLUDES =================================================================

#include <vector>
#include <OpenMesh/Core/Utils/Property.hh>


//== FORWARDDECLARATIONS ======================================================


//== NAMESPACES ===============================================================

namespace OpenMesh {


//== CLASS DEFINITION =========================================================




/** \class StripifierT StripifierT.hh <OpenMesh/Tools/Utils/StripifierT.hh>
    This class decomposes a triangle mesh into several triangle strips.
*/

template <class Mesh>
class StripifierT
{
public:

  typedef unsigned int                      Index;
  typedef std::vector<Index>                Strip;
  typedef typename Strip::const_iterator    IndexIterator;
  typedef std::vector<Strip>                Strips;
  typedef typename Strips::const_iterator   StripsIterator;


  /// Default constructor
  StripifierT(Mesh& _mesh);

  /// Destructor
  ~StripifierT();

  /// Compute triangle strips, returns number of strips
  size_t stripify();

  /// delete all strips
  void clear() { Strips().swap(strips_); }

  /// returns number of strips
  size_t n_strips() const { return strips_.size(); }

  /// are strips computed?
  bool is_valid() const { return !strips_.empty(); }

  /// Access strips
  StripsIterator begin() const { return strips_.begin(); }
  /// Access strips
  StripsIterator end()   const { return strips_.end(); }


private:

  typedef std::vector<typename Mesh::FaceHandle>  FaceHandles;


  /// this method does the main work
  void build_strips();

  /// build a strip from a given halfedge (in both directions)
  void build_strip(typename Mesh::HalfedgeHandle _start_hh,
		   Strip& _strip,
		   FaceHandles& _faces);

  FPropHandleT<bool>::reference  processed(typename Mesh::FaceHandle _fh) {
    return mesh_.property(processed_, _fh);
  }
  FPropHandleT<bool>::reference  used(typename Mesh::FaceHandle _fh) {
    return mesh_.property(used_, _fh);
  }



private:

  Mesh&                mesh_;
  Strips               strips_;
  FPropHandleT<bool>   processed_, used_;
};


//=============================================================================
} // namespace OpenMesh
//=============================================================================
#if defined(OM_INCLUDE_TEMPLATES) && !defined(OPENMESH_STRIPIFIERT_C)
#define OPENMESH_STRIPIFIERT_TEMPLATES
#include "StripifierT_impl.hh"
#endif
//=============================================================================
#endif // OPENMESH_STRIPIFIERT_HH defined
//=============================================================================
