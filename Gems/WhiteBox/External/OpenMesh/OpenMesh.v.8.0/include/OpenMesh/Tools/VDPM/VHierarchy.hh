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

#ifndef OPENMESH_VDPROGMESH_VHIERARCHY_HH
#define OPENMESH_VDPROGMESH_VHIERARCHY_HH


//== INCLUDES =================================================================

#include <vector>
#include <OpenMesh/Tools/VDPM/VHierarchyNode.hh>


//== FORWARDDECLARATIONS ======================================================


//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace VDPM {

//== CLASS DEFINITION =========================================================

	      
/** Keeps the vertex hierarchy build during analyzing a progressive mesh.
 */
class OPENMESHDLLEXPORT VHierarchy
{
public:

  typedef unsigned int    id_t; ///< Type for tree and node ids

private:

  VHierarchyNodeContainer nodes_;
  unsigned int            n_roots_;
  unsigned char           tree_id_bits_; // node_id_bits_ = 32-tree_id_bits_;

public:
  
  VHierarchy();

  void clear()                        { nodes_.clear();   n_roots_ = 0; }
  unsigned char tree_id_bits() const  { return tree_id_bits_; }
  unsigned int num_roots() const      { return n_roots_; }
  size_t num_nodes() const            { return nodes_.size(); }

  VHierarchyNodeIndex generate_node_index(id_t _tree_id, id_t _node_id)
  {
    return  VHierarchyNodeIndex(_tree_id, _node_id, tree_id_bits_);
  }


  void set_num_roots(unsigned int _n_roots);
  
  VHierarchyNodeHandle root_handle(unsigned int i) const
  {
    return  VHierarchyNodeHandle( (int)i );
  }


  const VHierarchyNode& node(VHierarchyNodeHandle _vhierarchynode_handle) const
  {
    return nodes_[_vhierarchynode_handle.idx()];
  }


  VHierarchyNode& node(VHierarchyNodeHandle _vhierarchynode_handle)
  {
    return nodes_[_vhierarchynode_handle.idx()];
  }

  VHierarchyNodeHandle add_node();
  VHierarchyNodeHandle add_node(const VHierarchyNode &_node);

  void make_children(VHierarchyNodeHandle &_parent_handle);

  bool is_ancestor(VHierarchyNodeIndex _ancestor_index, 
		   VHierarchyNodeIndex _descendent_index);
  
  bool is_leaf_node(VHierarchyNodeHandle _node_handle)  
  { return nodes_[_node_handle.idx()].is_leaf(); }

  bool is_root_node(VHierarchyNodeHandle _node_handle)  
  { return nodes_[_node_handle.idx()].is_root(); }


  const OpenMesh::Vec3f& normal(VHierarchyNodeHandle _node_handle) const  
  {
    return  nodes_[_node_handle.idx()].normal(); 
  }

  const VHierarchyNodeIndex& 
  node_index(VHierarchyNodeHandle _node_handle) const
  { return  nodes_[_node_handle.idx()].node_index(); }

  VHierarchyNodeIndex& node_index(VHierarchyNodeHandle _node_handle)
  { return  nodes_[_node_handle.idx()].node_index(); }

  const VHierarchyNodeIndex& 
  fund_lcut_index(VHierarchyNodeHandle _node_handle) const
  { return  nodes_[_node_handle.idx()].fund_lcut_index(); }

  VHierarchyNodeIndex& fund_lcut_index(VHierarchyNodeHandle _node_handle)
  { return  nodes_[_node_handle.idx()].fund_lcut_index(); }

  const VHierarchyNodeIndex& 
  fund_rcut_index(VHierarchyNodeHandle _node_handle) const
  { return  nodes_[_node_handle.idx()].fund_rcut_index(); }

  VHierarchyNodeIndex& fund_rcut_index(VHierarchyNodeHandle _node_handle)
  { return  nodes_[_node_handle.idx()].fund_rcut_index(); }     
  
  VertexHandle  vertex_handle(VHierarchyNodeHandle _node_handle)
  { return  nodes_[_node_handle.idx()].vertex_handle(); }

  VHierarchyNodeHandle  parent_handle(VHierarchyNodeHandle _node_handle)
  { return nodes_[_node_handle.idx()].parent_handle(); }

  VHierarchyNodeHandle  lchild_handle(VHierarchyNodeHandle _node_handle)
  { return nodes_[_node_handle.idx()].lchild_handle(); }

  VHierarchyNodeHandle  rchild_handle(VHierarchyNodeHandle _node_handle)
  { return nodes_[_node_handle.idx()].rchild_handle(); }

  VHierarchyNodeHandle  node_handle(VHierarchyNodeIndex _node_index);

private:
  
  VHierarchyNodeHandle compute_dependency(VHierarchyNodeIndex index0, 
					  VHierarchyNodeIndex index1);

};



//=============================================================================
} // namespace VDPM
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_VDPROGMESH_VHIERARCHY_HH defined
//=============================================================================
