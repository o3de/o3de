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



#ifndef OPENMESH_AutoPropertyHandleT_HH
#define OPENMESH_AutoPropertyHandleT_HH

//== INCLUDES =================================================================
#include <assert.h>
#include <string>

//== NAMESPACES ===============================================================

namespace OpenMesh {

//== CLASS DEFINITION =========================================================

template <class Mesh_, class PropertyHandle_>
class AutoPropertyHandleT : public PropertyHandle_
{
public:
  typedef Mesh_                             Mesh;
  typedef PropertyHandle_                   PropertyHandle;
  typedef PropertyHandle                    Base;
  typedef typename PropertyHandle::Value    Value;
  typedef AutoPropertyHandleT<Mesh, PropertyHandle>
                                            Self;
protected:
  Mesh*                                     m_;
  bool                                      own_property_;//ref counting?

public:
  AutoPropertyHandleT()
  : m_(NULL), own_property_(false)
  {}
  
  AutoPropertyHandleT(const Self& _other)
  : Base(_other.idx()), m_(_other.m_), own_property_(false)
  {}
  
  explicit AutoPropertyHandleT(Mesh& _m, const std::string& _pp_name = std::string())
  { add_property(_m, _pp_name); }

  AutoPropertyHandleT(Mesh& _m, PropertyHandle _pph)
  : Base(_pph.idx()), m_(&_m), own_property_(false)
  {}

  ~AutoPropertyHandleT()
  {
    if (own_property_)
    {
      m_->remove_property(*this);
    }
  }

  inline void                               add_property(Mesh& _m, const std::string& _pp_name = std::string())
  {
    assert(!is_valid());
    m_ = &_m;
    own_property_ = _pp_name.empty() || !m_->get_property_handle(*this, _pp_name);
    if (own_property_)
    {
      m_->add_property(*this, _pp_name);
    }
  }
  
  inline void                               remove_property()
  {
    assert(own_property_);//only the owner can delete the property
    m_->remove_property(*this);
    own_property_ = false;
    invalidate();
  }
  
  template <class _Handle>
  inline Value&                             operator [] (_Handle _hnd)
  { return m_->property(*this, _hnd); }

  template <class _Handle>
  inline const Value&                       operator [] (_Handle _hnd) const
  { return m_->property(*this, _hnd); }

  inline bool                               own_property() const
  { return own_property_; }

  inline void                               free_property()
  { own_property_ = false; }
};

//=============================================================================
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_AutoPropertyHandleT_HH defined
//=============================================================================
