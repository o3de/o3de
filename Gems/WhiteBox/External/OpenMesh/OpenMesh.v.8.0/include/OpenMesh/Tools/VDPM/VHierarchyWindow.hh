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

#ifndef OPENMESH_VDPROGMESH_VHIERARCHYWINDOWS_HH
#define OPENMESH_VDPROGMESH_VHIERARCHYWINDOWS_HH


//== INCLUDES =================================================================

#include <OpenMesh/Tools/VDPM/VHierarchy.hh>
#include <algorithm>

//== FORWARDDECLARATIONS ======================================================


//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace VDPM {

//== CLASS DEFINITION =========================================================

	      
/** \todo VHierarchyWindow documentation
*/
class VHierarchyWindow
{
private:

  // reference of vertex hierarchy
  VHierarchy    *vhierarchy_;

  // bits buffer (byte units)
  unsigned char *buffer_;
  int           buffer_min_;
  size_t        buffer_max_;
  int           current_pos_;

  // window (byte units)
  int           window_min_;
  int           window_max_;
  

  // # of right shift (bit units)
  unsigned char n_shift_;           // [0, 7]

  unsigned char flag8(unsigned char n_shift) const
  { return 0x80 >> n_shift; }  

  unsigned char flag8(VHierarchyNodeHandle _node_handle) const
  {
    assert(_node_handle.idx() >= 0);
    return  0x80 >> (unsigned int) (_node_handle.idx() % 8);
  }
  int byte_idx(VHierarchyNodeHandle _node_handle) const
  {
    assert(_node_handle.idx() >= 0);
    return  _node_handle.idx() / 8;
  }
  int buffer_idx(VHierarchyNodeHandle _node_handle) const
  { return  byte_idx(_node_handle) - buffer_min_; } 

  bool before_window(VHierarchyNodeHandle _node_handle) const
  { return (_node_handle.idx()/8 < window_min_) ? true : false; }

  bool after_window(VHierarchyNodeHandle _node_handle) const
  { return (_node_handle.idx()/8 < window_max_) ? false : true; }

  bool underflow(VHierarchyNodeHandle _node_handle) const
  { return (_node_handle.idx()/8 < buffer_min_) ? true : false; }

  bool overflow(VHierarchyNodeHandle _node_handle) const
  { return (_node_handle.idx()/8 < int(buffer_max_) ) ? false : true; }

  bool update_buffer(VHierarchyNodeHandle _node_handle);

public:
  VHierarchyWindow();
  VHierarchyWindow(VHierarchy &_vhierarchy);
  ~VHierarchyWindow(void);
  
  void set_vertex_hierarchy(VHierarchy &_vhierarchy)
  { vhierarchy_ = &_vhierarchy; }

  void begin()
  {
    int new_window_min = window_min_;
    for (current_pos_=window_min_-buffer_min_; 
	 current_pos_ < window_size(); ++current_pos_)
    {
      if (buffer_[current_pos_] == 0)   
	++new_window_min;
      else
      {
        n_shift_ = 0;
        while ((buffer_[current_pos_] & flag8(n_shift_)) == 0)
          ++n_shift_;
        break;
      }
    }
    window_min_ = new_window_min;
  }

  void next()
  {
    ++n_shift_;
    if (n_shift_ == 8)
    {
      n_shift_ = 0;
      ++current_pos_;
    }

    while (current_pos_ < window_max_-buffer_min_)
    {
      if (buffer_[current_pos_] != 0) // if the current byte has non-zero bits
      {
        while (n_shift_ != 8)
        {
          if ((buffer_[current_pos_] & flag8(n_shift_)) != 0)
            return;                     // find 1 bit in the current byte
          ++n_shift_;
        }
      }
      n_shift_ = 0;
      ++current_pos_;
    }
  }
  bool end() { return !(current_pos_ < window_max_-buffer_min_); }

  int window_size() const      { return  window_max_ - window_min_; }
  size_t buffer_size() const      { return  buffer_max_ - buffer_min_; }

  VHierarchyNodeHandle node_handle()
  {
    return  VHierarchyNodeHandle(8*(buffer_min_+current_pos_) + (int)n_shift_);
  }

  void activate(VHierarchyNodeHandle _node_handle)
  {
    update_buffer(_node_handle);
    buffer_[buffer_idx(_node_handle)] |= flag8(_node_handle);
    window_min_ = std::min(window_min_, byte_idx(_node_handle));
    window_max_ = std::max(window_max_, 1+byte_idx(_node_handle));
  }


  void inactivate(VHierarchyNodeHandle _node_handle)
  {
    if (is_active(_node_handle) != true)  return;
    buffer_[buffer_idx(_node_handle)] ^= flag8(_node_handle);
  }


  bool is_active(VHierarchyNodeHandle _node_handle) const
  {
    if (before_window(_node_handle) == true ||
	after_window(_node_handle) == true)
      return  false;
    return ((buffer_[buffer_idx(_node_handle)] & flag8(_node_handle)) > 0);
  }

  void init(VHierarchyNodeHandleContainer &_roots);
  void update_with_vsplit(VHierarchyNodeHandle _parent_handle);
  void update_with_ecol(VHierarchyNodeHandle _parent_handle);
};

//=============================================================================
} // namespace VDPM
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_VDPROGMESH_VHIERARCHYWINDOWS_HH
//=============================================================================

