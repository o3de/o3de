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



#ifndef OPENMESH_PROPERTYCONTAINER
#define OPENMESH_PROPERTYCONTAINER

// Use static casts when not debugging
#ifdef NDEBUG
#define OM_FORCE_STATIC_CAST
#endif

#include <OpenMesh/Core/Utils/Property.hh>

//-----------------------------------------------------------------------------
namespace OpenMesh
{
//== FORWARDDECLARATIONS ======================================================
  class BaseKernel;

//== CLASS DEFINITION =========================================================
/// A a container for properties.
class PropertyContainer
{
public:

  //-------------------------------------------------- constructor / destructor

  PropertyContainer() {}
  virtual ~PropertyContainer() { std::for_each(properties_.begin(), properties_.end(), Delete()); }


  //------------------------------------------------------------- info / access

  typedef std::vector<BaseProperty*> Properties;
  const Properties& properties() const { return properties_; }
  size_t size() const { return properties_.size(); }



  //--------------------------------------------------------- copy / assignment

  PropertyContainer(const PropertyContainer& _rhs) { operator=(_rhs); }

  PropertyContainer& operator=(const PropertyContainer& _rhs)
  {
    // The assignment below relies on all previous BaseProperty* elements having been deleted
    std::for_each(properties_.begin(), properties_.end(), Delete());
    properties_ = _rhs.properties_;
    Properties::iterator p_it=properties_.begin(), p_end=properties_.end();
    for (; p_it!=p_end; ++p_it)
      if (*p_it)
        *p_it = (*p_it)->clone();
    return *this;
  }



  //--------------------------------------------------------- manage properties

  template <class T>
  BasePropHandleT<T> add(const T&, const std::string& _name="<unknown>")
  {
    Properties::iterator p_it=properties_.begin(), p_end=properties_.end();
    int idx=0;
    for ( ; p_it!=p_end && *p_it!=NULL; ++p_it, ++idx ) {};
    if (p_it==p_end) properties_.push_back(NULL);
    properties_[idx] = new PropertyT<T>(_name);
    return BasePropHandleT<T>(idx);
  }


  template <class T>
  BasePropHandleT<T> handle(const T&, const std::string& _name) const
  {
    Properties::const_iterator p_it = properties_.begin();
    for (int idx=0; p_it != properties_.end(); ++p_it, ++idx)
    {
      if (*p_it != NULL &&
         (*p_it)->name() == _name  //skip deleted properties
// Skip type check
#ifndef OM_FORCE_STATIC_CAST
          && dynamic_cast<PropertyT<T>*>(properties_[idx]) != NULL //check type
#endif
         )
      {
        return BasePropHandleT<T>(idx);
      }
    }
    return BasePropHandleT<T>();
  }

  BaseProperty* property( const std::string& _name ) const
  {
    Properties::const_iterator p_it = properties_.begin();
    for (int idx=0; p_it != properties_.end(); ++p_it, ++idx)
    {
      if (*p_it != NULL && (*p_it)->name() == _name) //skip deleted properties
      {
        return *p_it;
      }
    }
    return NULL;
  }

  template <class T> PropertyT<T>& property(BasePropHandleT<T> _h)
  {
    assert(_h.idx() >= 0 && _h.idx() < (int)properties_.size());
    assert(properties_[_h.idx()] != NULL);
#ifdef OM_FORCE_STATIC_CAST
    return *static_cast  <PropertyT<T>*> (properties_[_h.idx()]);
#else
    PropertyT<T>* p = dynamic_cast<PropertyT<T>*>(properties_[_h.idx()]);
    assert(p != NULL);
    return *p;
#endif
  }


  template <class T> const PropertyT<T>& property(BasePropHandleT<T> _h) const
  {
    assert(_h.idx() >= 0 && _h.idx() < (int)properties_.size());
    assert(properties_[_h.idx()] != NULL);
#ifdef OM_FORCE_STATIC_CAST
    return *static_cast<PropertyT<T>*>(properties_[_h.idx()]);
#else
    PropertyT<T>* p = dynamic_cast<PropertyT<T>*>(properties_[_h.idx()]);
    assert(p != NULL);
    return *p;
#endif
  }


  template <class T> void remove(BasePropHandleT<T> _h)
  {
    assert(_h.idx() >= 0 && _h.idx() < (int)properties_.size());
    delete properties_[_h.idx()];
    properties_[_h.idx()] = NULL;
  }


  void clear()
  {
	// Clear properties vector:
	// Replaced the old version with new one
	// which performs a swap to clear values and
	// deallocate memory.

	// Old version (changed 22.07.09) {
	// std::for_each(properties_.begin(), properties_.end(), Delete());
    // }

	std::for_each(properties_.begin(), properties_.end(), ClearAll());
  }


  //---------------------------------------------------- synchronize properties

/*
 * In C++11 an beyond we can introduce more efficient and more legible
 * implementations of the following methods.
 */
#if ((defined(_MSC_VER) && (_MSC_VER >= 1800)) || __cplusplus > 199711L || defined(__GXX_EXPERIMENTAL_CXX0X__)) && !defined(OPENMESH_VECTOR_LEGACY)
  /**
   * Reserves space for \p _n elements in all property vectors.
   */
  void reserve(size_t _n) const {
    std::for_each(properties_.begin(), properties_.end(),
            [_n](BaseProperty* p) { if (p) p->reserve(_n); });
  }

  /**
   * Resizes all property vectors to the specified size.
   */
  void resize(size_t _n) const {
    std::for_each(properties_.begin(), properties_.end(),
            [_n](BaseProperty* p) { if (p) p->resize(_n); });
  }

  /**
   * Same as resize() but ignores property vectors that have a size larger
   * than \p _n.
   *
   * Use this method instead of resize() if you plan to frequently reduce
   * and enlarge the property container and you don't want to waste time
   * reallocating the property vectors every time.
   */
  void resize_if_smaller(size_t _n) const {
    std::for_each(properties_.begin(), properties_.end(),
            [_n](BaseProperty* p) { if (p && p->n_elements() < _n) p->resize(_n); });
  }

  /**
   * Swaps the items with index \p _i0 and index \p _i1 in all property
   * vectors.
   */
  void swap(size_t _i0, size_t _i1) const {
    std::for_each(properties_.begin(), properties_.end(),
            [_i0, _i1](BaseProperty* p) { if (p) p->swap(_i0, _i1); });
  }
#else
  /**
   * Reserves space for \p _n elements in all property vectors.
   */
  void reserve(size_t _n) const {
    std::for_each(properties_.begin(), properties_.end(), Reserve(_n));
  }

  /**
   * Resizes all property vectors to the specified size.
   */
  void resize(size_t _n) const {
    std::for_each(properties_.begin(), properties_.end(), Resize(_n));
  }

  /**
   * Same as \sa resize() but ignores property vectors that have a size
   * larger than \p _n.
   *
   * Use this method instead of \sa resize() if you plan to frequently reduce
   * and enlarge the property container and you don't want to waste time
   * reallocating the property vectors every time.
   */
  void resize_if_smaller(size_t _n) const {
    std::for_each(properties_.begin(), properties_.end(), ResizeIfSmaller(_n));
  }

  /**
   * Swaps the items with index \p _i0 and index \p _i1 in all property
   * vectors.
   */
  void swap(size_t _i0, size_t _i1) const {
    std::for_each(properties_.begin(), properties_.end(), Swap(_i0, _i1));
  }
#endif



protected: // generic add/get

  size_t _add( BaseProperty* _bp )
  {
    Properties::iterator p_it=properties_.begin(), p_end=properties_.end();
    size_t idx=0;
    for (; p_it!=p_end && *p_it!=NULL; ++p_it, ++idx) {};
    if (p_it==p_end) properties_.push_back(NULL);
    properties_[idx] = _bp;
    return idx;
  }

  BaseProperty& _property( size_t _idx )
  {
    assert( _idx < properties_.size());
    assert( properties_[_idx] != NULL);
    BaseProperty *p = properties_[_idx];
    assert( p != NULL );
    return *p;
  }

  const BaseProperty& _property( size_t _idx ) const
  {
    assert( _idx < properties_.size());
    assert( properties_[_idx] != NULL);
    BaseProperty *p = properties_[_idx];
    assert( p != NULL );
    return *p;
  }


  typedef Properties::iterator       iterator;
  typedef Properties::const_iterator const_iterator;
  iterator begin() { return properties_.begin(); }
  iterator end()   { return properties_.end(); }
  const_iterator begin() const { return properties_.begin(); }
  const_iterator end() const   { return properties_.end(); }

  friend class BaseKernel;

private:

  //-------------------------------------------------- synchronization functors

#ifndef DOXY_IGNORE_THIS
  struct Reserve
  {
    Reserve(size_t _n) : n_(_n) {}
    void operator()(BaseProperty* _p) const { if (_p) _p->reserve(n_); }
    size_t n_;
  };

  struct Resize
  {
    Resize(size_t _n) : n_(_n) {}
    void operator()(BaseProperty* _p) const { if (_p) _p->resize(n_); }
    size_t n_;
  };

  struct ResizeIfSmaller
  {
    ResizeIfSmaller(size_t _n) : n_(_n) {}
    void operator()(BaseProperty* _p) const { if (_p && _p->n_elements() < n_) _p->resize(n_); }
    size_t n_;
  };

  struct ClearAll
  {
    ClearAll() {}
    void operator()(BaseProperty* _p) const { if (_p) _p->clear(); }
  };

  struct Swap
  {
    Swap(size_t _i0, size_t _i1) : i0_(_i0), i1_(_i1) {}
    void operator()(BaseProperty* _p) const { if (_p) _p->swap(i0_, i1_); }
    size_t i0_, i1_;
  };

  struct Delete
  {
    Delete() {}
    void operator()(BaseProperty* _p) const { if (_p) delete _p; _p=NULL; }
  };
#endif

  Properties   properties_;
};

}//namespace OpenMesh

#endif//OPENMESH_PROPERTYCONTAINER

