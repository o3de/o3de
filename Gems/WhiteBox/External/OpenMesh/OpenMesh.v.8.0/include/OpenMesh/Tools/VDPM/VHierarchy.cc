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

#include <OpenMesh/Tools/VDPM/VHierarchy.hh>


//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace VDPM {

//== IMPLEMENTATION ========================================================== 


VHierarchy::
VHierarchy() :
  n_roots_(0), tree_id_bits_(0)
{
  clear();
}

void
VHierarchy::
set_num_roots(unsigned int _n_roots)
{
  n_roots_ = _n_roots;
  
  tree_id_bits_ = 0;
  while (n_roots_ > ((unsigned int) 0x00000001 << tree_id_bits_))
    ++tree_id_bits_;
}


VHierarchyNodeHandle 
VHierarchy::
add_node()
{
  return  add_node(VHierarchyNode());
}

VHierarchyNodeHandle 
VHierarchy::
add_node(const VHierarchyNode &_node)
{
  nodes_.push_back(_node);

  return  VHierarchyNodeHandle(int(nodes_.size() - 1));
}


void
VHierarchy::
make_children(VHierarchyNodeHandle &_parent_handle)
{
  VHierarchyNodeHandle lchild_handle = add_node();
  VHierarchyNodeHandle rchild_handle = add_node();    

  VHierarchyNode &parent = node(_parent_handle);
  VHierarchyNode &lchild = node(lchild_handle);
  VHierarchyNode &rchild = node(rchild_handle);

  parent.set_children_handle(lchild_handle);
  lchild.set_parent_handle(_parent_handle);
  rchild.set_parent_handle(_parent_handle);

  lchild.set_index(VHierarchyNodeIndex(parent.node_index().tree_id(tree_id_bits_), 2*parent.node_index().node_id(tree_id_bits_), tree_id_bits_));
  rchild.set_index(VHierarchyNodeIndex(parent.node_index().tree_id(tree_id_bits_), 2*parent.node_index().node_id(tree_id_bits_)+1, tree_id_bits_));
}

VHierarchyNodeHandle
VHierarchy::
node_handle(VHierarchyNodeIndex _node_index)
{
  if (_node_index.is_valid(tree_id_bits_) != true)
    return  InvalidVHierarchyNodeHandle;

  VHierarchyNodeHandle node_handle = root_handle(_node_index.tree_id(tree_id_bits_));
  unsigned int node_id = _node_index.node_id(tree_id_bits_);
  unsigned int flag = 0x80000000;

  while (!(node_id & flag))   flag >>= 1;
  flag >>= 1;

  while (flag > 0 && is_leaf_node(node_handle) != true)
  {
    if (node_id & flag)     // 1: rchild
    {
      node_handle = rchild_handle(node_handle);
    }
    else                    // 0: lchild
    {
      node_handle = lchild_handle(node_handle);
    }
    flag >>= 1;
  }

  return  node_handle;
}

bool
VHierarchy::
is_ancestor(VHierarchyNodeIndex _ancestor_index, VHierarchyNodeIndex _descendent_index)
{
  if (_ancestor_index.tree_id(tree_id_bits_) != _descendent_index.tree_id(tree_id_bits_))
    return  false;

  unsigned int ancestor_node_id   =  _ancestor_index.node_id(tree_id_bits_);
  unsigned int descendent_node_id =  _descendent_index.node_id(tree_id_bits_);

  if (ancestor_node_id > descendent_node_id)
    return  false;

  while (descendent_node_id > 0)
  {
    if (ancestor_node_id == descendent_node_id)
      return  true;
    descendent_node_id >>= 1;       // descendent_node_id /= 2
  }

  return  false;
}

//=============================================================================
} // namespace VDPM
} // namespace OpenMesh
//=============================================================================
