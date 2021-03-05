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



#ifndef OPENMESH_BASEPROPERTY_HH
#define OPENMESH_BASEPROPERTY_HH

#include <string>
#include <OpenMesh/Core/IO/StoreRestore.hh>
#include <OpenMesh/Core/System/omstream.hh>

namespace OpenMesh {

//== CLASS DEFINITION =========================================================

/** \class BaseProperty Property.hh <OpenMesh/Core/Utils/PropertyT.hh>

    Abstract class defining the basic interface of a dynamic property.
**/

class OPENMESHDLLEXPORT BaseProperty
{
public:

  /// Indicates an error when a size is returned by a member.
  static const size_t UnknownSize = size_t(-1);

public:

  /// \brief Default constructor.
  ///
  /// In %OpenMesh all mesh data is stored in so-called properties.
  /// We distinuish between standard properties, which can be defined at
  /// compile time using the Attributes in the traits definition and
  /// at runtime using the request property functions defined in one of
  /// the kernels.
  ///
  /// If the property should be stored along with the default properties
  /// in the OM-format one must name the property and enable the persistant
  /// flag with set_persistent().
  ///
  /// \param _name Optional textual name for the property.
  ///
  BaseProperty(const std::string& _name = "<unknown>")
  : name_(_name), persistent_(false)
  {}

  /// \brief Copy constructor
  BaseProperty(const BaseProperty & _rhs)
      : name_( _rhs.name_ ), persistent_( _rhs.persistent_ ) {}

  /// Destructor.
  virtual ~BaseProperty() {}

public: // synchronized array interface

  /// Reserve memory for n elements.
  virtual void reserve(size_t _n) = 0;

  /// Resize storage to hold n elements.
  virtual void resize(size_t _n) = 0;

  /// Clear all elements and free memory.
  virtual void clear() = 0;

  /// Extend the number of elements by one.
  virtual void push_back() = 0;

  /// Let two elements swap their storage place.
  virtual void swap(size_t _i0, size_t _i1) = 0;

  /// Copy one element to another
  virtual void copy(size_t _io, size_t _i1) = 0;
  
  /// Return a deep copy of self.
  virtual BaseProperty* clone () const = 0;

public: // named property interface

  /// Return the name of the property
  const std::string& name() const { return name_; }

  virtual void stats(std::ostream& _ostr) const;

public: // I/O support

  /// Returns true if the persistent flag is enabled else false.
  bool persistent(void) const { return persistent_; }

  /// Enable or disable persistency. Self must be a named property to enable
  /// persistency.
  virtual void set_persistent( bool _yn ) = 0;

  /// Number of elements in property
  virtual size_t       n_elements() const = 0;

  /// Size of one element in bytes or UnknownSize if not known.
  virtual size_t       element_size() const = 0;

  /// Return size of property in bytes
  virtual size_t       size_of() const
  {
    return size_of( n_elements() );
  }

  /// Estimated size of property if it has _n_elem elements.
  /// The member returns UnknownSize if the size cannot be estimated.
  virtual size_t       size_of(size_t _n_elem) const
  {
    return (element_size()!=UnknownSize)
      ? (_n_elem*element_size())
      : UnknownSize;
  }

  /// Store self as one binary block
  virtual size_t store( std::ostream& _ostr, bool _swap ) const = 0;

  /** Restore self from a binary block. Uses reserve() to set the
      size of self before restoring.
  **/
  virtual size_t restore( std::istream& _istr, bool _swap ) = 0;

protected:

  // To be used in a derived class, when overloading set_persistent()
  template < typename T >
  void check_and_set_persistent( bool _yn )
  {
    if ( _yn && !IO::is_streamable<T>() )
      omerr() << "Warning! Type of property value is not binary storable!\n";
    persistent_ = IO::is_streamable<T>() && _yn;
  }

private:

  std::string name_;
  bool        persistent_;
};

}//namespace OpenMesh

#endif //OPENMESH_BASEPROPERTY_HH


