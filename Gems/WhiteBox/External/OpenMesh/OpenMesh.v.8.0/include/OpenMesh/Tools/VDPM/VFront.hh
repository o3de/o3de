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
//  CLASS VFront
//
//=============================================================================

#ifndef OPENMESH_VDPROGMESH_VFRONT_HH
#define OPENMESH_VDPROGMESH_VFRONT_HH


//== INCLUDES =================================================================

#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Tools/VDPM/VHierarchyNode.hh>
#include <vector>


//== FORWARDDECLARATIONS ======================================================


//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace VDPM {

//== CLASS DEFINITION =========================================================

	      
/** Active nodes in vertex hierarchy.
    \todo VFront documentation
*/
class OPENMESHDLLEXPORT VFront
{
private:

  typedef VHierarchyNodeHandleList::iterator  VHierarchyNodeHandleListIter;
  enum VHierarchyNodeStatus { kSplit, kActive, kCollapse };
  
  VHierarchyNodeHandleList                    front_;
  VHierarchyNodeHandleListIter                front_it_;
  std::vector<VHierarchyNodeHandleListIter>   front_location_;

public:

  VFront();

  void clear() { front_.clear(); front_location_.clear(); }
  void begin() { front_it_ = front_.begin(); }
  bool end()   { return (front_it_ == front_.end()) ? true : false; }
  void next()  { ++front_it_; }
  int size()   { return (int) front_.size(); }
  VHierarchyNodeHandle node_handle()    { return  *front_it_; }

  void add(VHierarchyNodeHandle _node_handle);
  void remove(VHierarchyNodeHandle _node_handle);
  bool is_active(VHierarchyNodeHandle _node_handle);
  void init(VHierarchyNodeHandleContainer &_roots, unsigned int _n_details);  
};


//=============================================================================
} // namespace VDPM
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_VDPROGMESH_VFRONT_HH defined
//=============================================================================
