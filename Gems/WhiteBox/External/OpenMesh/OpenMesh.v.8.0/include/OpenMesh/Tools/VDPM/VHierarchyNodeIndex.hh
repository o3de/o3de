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
//  CLASS newClass
//
//=============================================================================

#ifndef OPENMESH_VDPROGMESH_VHIERARCHYNODEINDEX_HH
#define OPENMESH_VDPROGMESH_VHIERARCHYNODEINDEX_HH

//== INCLUDES =================================================================

#include <vector>
#include <cassert>

//== FORWARDDECLARATIONS ======================================================


//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace VDPM {

//== CLASS DEFINITION =========================================================

	      
/** Index of vertex hierarchy node
 */


class VHierarchyNodeIndex
{
private:
  unsigned int value_;

public:

  static const VHierarchyNodeIndex  InvalidIndex;

public:

  VHierarchyNodeIndex()
  { value_ = 0; }
  
  explicit VHierarchyNodeIndex(unsigned int _value)
  { value_ = _value; }

  VHierarchyNodeIndex(const VHierarchyNodeIndex &_other)
  { value_ = _other.value_; }

  VHierarchyNodeIndex(unsigned int   _tree_id, 
		      unsigned int   _node_id, 
		      unsigned short _tree_id_bits)
  {
    assert(_tree_id < ((unsigned int) 0x00000001 << _tree_id_bits));
    assert(_node_id < ((unsigned int) 0x00000001 << (32 - _tree_id_bits)));
    value_ = (_tree_id << (32 - _tree_id_bits)) | _node_id;
  }

  bool is_valid(unsigned short _tree_id_bits) const
  { return  node_id(_tree_id_bits) != 0 ? true : false;  }

  unsigned int tree_id(unsigned short _tree_id_bits) const
  { return  value_ >> (32 - _tree_id_bits); }
  
  unsigned int node_id(unsigned short _tree_id_bits) const
  { return  value_ & ((unsigned int) 0xFFFFFFFF >> _tree_id_bits); }

  bool operator< (const VHierarchyNodeIndex &other) const
  { return  (value_ < other.value_) ? true : false; }

  unsigned int value() const
  { return  value_; }
};


/// Container for vertex hierarchy node indices
typedef std::vector<VHierarchyNodeIndex>    VHierarchyNodeIndexContainer;


//=============================================================================
} // namespace VDPM
} // namespace OpenMesh
//=============================================================================
#endif //  OPENMESH_VDPROGMESH_VHIERARCHYNODEINDEX_HH defined
//=============================================================================
