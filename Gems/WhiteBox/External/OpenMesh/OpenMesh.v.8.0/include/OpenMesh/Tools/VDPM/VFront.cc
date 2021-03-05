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
//  CLASS newClass - IMPLEMENTATION
//
//=============================================================================


//== INCLUDES =================================================================

#include <OpenMesh/Tools/VDPM/VFront.hh>

//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace VDPM {

//== IMPLEMENTATION ========================================================== 


VFront::VFront()
{  
}


void
VFront::
add(VHierarchyNodeHandle _node_handle)
{
  front_location_[_node_handle.idx()] = front_.insert(front_.end(), _node_handle);
}


void
VFront::
remove(VHierarchyNodeHandle _node_handle)
{
  VHierarchyNodeHandleListIter node_it = front_location_[_node_handle.idx()];
  const bool isFront = (front_it_ == node_it);
  VHierarchyNodeHandleListIter next_it = front_.erase(node_it);
  front_location_[_node_handle.idx()] = front_.end();

  if (isFront)
    front_it_ = next_it;
}

bool
VFront::
is_active(VHierarchyNodeHandle _node_handle)
{
  return  (front_location_[_node_handle.idx()] != front_.end()) ? true : false;
}

void 
VFront::
init(VHierarchyNodeHandleContainer &_roots, unsigned int _n_details)
{
  unsigned int i;

  front_location_.resize(_roots.size() + 2*_n_details);
  for (i=0; i<front_location_.size(); ++i)
    front_location_[i] = front_.end();

  for (i=0; i<_roots.size(); ++i)
    add(_roots[i]);
}


//=============================================================================
} // namespace VDPM
} // namespace OpenMesh
//=============================================================================
