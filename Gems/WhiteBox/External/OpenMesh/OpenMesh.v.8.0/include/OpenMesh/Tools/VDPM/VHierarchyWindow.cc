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

#include <OpenMesh/Tools/VDPM/VHierarchyWindow.hh>

#include <cstring>

#ifndef WIN32
#else
  #if defined(__MINGW32__)
    #include <stdlib.h>
    #include <string.h>
  #endif
#endif

//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace VDPM {


//== IMPLEMENTATION ========================================================== 


VHierarchyWindow::
VHierarchyWindow() :
  vhierarchy_(NULL), buffer_(NULL),buffer_min_ (0), buffer_max_(0), current_pos_(0) , window_min_(0), window_max_(0),  n_shift_(0)
{

}


VHierarchyWindow::
VHierarchyWindow(VHierarchy &_vhierarchy) :
  vhierarchy_(&_vhierarchy),buffer_(NULL),buffer_min_ (0), buffer_max_(0), current_pos_(0) , window_min_(0), window_max_(0) ,n_shift_(0)
{
}


VHierarchyWindow::
~VHierarchyWindow(void)
{
  if (buffer_ != NULL)
    free(buffer_);
}


bool
VHierarchyWindow::
update_buffer(VHierarchyNodeHandle _node_handle)
{
  if (underflow(_node_handle) != true && overflow(_node_handle) != true)
    return  false;

  // tightly update window_min_ & window_max_
  int none_zero_pos;
  for (none_zero_pos=int(buffer_size()-1); none_zero_pos >= 0; --none_zero_pos)
  {
    if (buffer_[none_zero_pos] != 0)  break;
  }
  window_max_ = buffer_min_ + none_zero_pos + 1;
  for(none_zero_pos=0; none_zero_pos < int(buffer_size()); ++none_zero_pos)
  {
    if (buffer_[none_zero_pos] != 0)  break;
  }
  window_min_ = buffer_min_ + none_zero_pos;  
  
  assert(window_min_ < window_max_);
  
  while (underflow(_node_handle) == true)   buffer_min_ /= 2;
  while (overflow(_node_handle) == true)
  {
    buffer_max_ *= 2;
    if (buffer_max_ > vhierarchy_->num_nodes() / 8)
      buffer_max_ = 1 + vhierarchy_->num_nodes() / 8;
  }
  
  unsigned char *new_buffer = (unsigned char *) malloc(buffer_size());
  memset(new_buffer, 0, buffer_size());
  memcpy(&(new_buffer[window_min_-buffer_min_]), 
	 &(buffer_[none_zero_pos]), 
	 window_size());
  free(buffer_);
  buffer_ = new_buffer;

  return  true;
}

void
VHierarchyWindow::init(VHierarchyNodeHandleContainer &_roots)
{
  if (buffer_ != NULL)
    free(buffer_);

  buffer_min_ = 0;
  buffer_max_ = _roots.size() / 8;
  if (_roots.size() % 8 > 0)
    ++buffer_max_;

  buffer_ = (unsigned char *) malloc(buffer_size());
  memset(buffer_, 0, buffer_size());

  window_min_ = 0;
  window_max_= 0;
  current_pos_ = 0;
  n_shift_ = 0;
  
  for (unsigned int i=0; i<_roots.size(); i++)
  {
    activate(VHierarchyNodeHandle((int) i));
  }
}


void
VHierarchyWindow::
update_with_vsplit(VHierarchyNodeHandle _parent_handle)
{
  VHierarchyNodeHandle  
    lchild_handle = vhierarchy_->lchild_handle(_parent_handle),
    rchild_handle = vhierarchy_->rchild_handle(_parent_handle);

  assert(is_active(_parent_handle) == true);
  assert(is_active(lchild_handle) != true);
  assert(is_active(rchild_handle) != true);

  inactivate(_parent_handle);
  activate(rchild_handle);
  activate(lchild_handle);
}

void
VHierarchyWindow::
update_with_ecol(VHierarchyNodeHandle _parent_handle)
{
  VHierarchyNodeHandle 
    lchild_handle = vhierarchy_->lchild_handle(_parent_handle),
    rchild_handle = vhierarchy_->rchild_handle(_parent_handle);

  assert(is_active(_parent_handle) != true);
  assert(is_active(lchild_handle) == true);
  assert(is_active(rchild_handle) == true);
  
  activate(_parent_handle);
  inactivate(rchild_handle);
  inactivate(lchild_handle);
}

//=============================================================================
} // namespace VDPM
} // namespace OpenMesh
//=============================================================================
