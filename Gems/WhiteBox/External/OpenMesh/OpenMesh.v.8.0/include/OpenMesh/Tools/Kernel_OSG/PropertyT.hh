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



#ifndef OPENMESH_KERNEL_OSG_PROPERTYT_HH
#define OPENMESH_KERNEL_OSG_PROPERTYT_HH


//== INCLUDES =================================================================

#include <OpenMesh/Core/Mesh/Attributes.hh>
#include <OpenMesh/Core/Mesh/BaseKernel.hh>
#include <OpenMesh/Core/Utils/GenProg.hh>
#include <OpenMesh/Core/Utils/Property.hh>
//
#include <osg/Geometry>
//
#include <stdexcept>
#include <vector>


//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace Kernel_OSG {


//== CLASS DEFINITION =========================================================


// ----------------------------------------------------------------------------

/** Property adaptor for OpenSG GeoProperties
 *
 *  This class bridges the interfaces of %OpenMesh properties and
 *  OpenSG GeoProperties. The PropertyKernelT uses these adaptors to
 *  add all necessary property functions to the kernel, while the
 *  AttribKernelT extends the kernel with the standard properites.
 *  Finally the ArrayKernelT completes the kernel build with a specialized
 *  version of the garbage collections since the GeoIndices require
 *  special handling.
 *
 *  \attention Data will be shared with a geometry core when linking
 *  a mesh with a OpenSG geometry node using Kernel_OSG::bind.
 *  \internal
 */
template <typename GeoProperty>
class oPropertyT : public BaseProperty
{
public:

  // Type of the encapsulated OpenSG Geometry Property
  typedef GeoProperty                                    property_t;
  typedef typename property_t::PtrType                   property_ptr_t;

  typedef typename property_t::StoredFieldType           field_t;
  typedef typename field_t::StoredType                   element_t;
  typedef typename field_t::StoredType                   value_type;

public:

  //
  oPropertyT( property_ptr_t _geo_prop, 
              const std::string& _name = "<unknown>" ) 
    : BaseProperty(_name), data_( _geo_prop )
  { 
    osg_init_check();
  }

  //
  oPropertyT( const std::string& _name = "<unknown>" )
    : BaseProperty(_name), data_(NULL)
  {
    data_ = property_t::create();
    
    // make sure data_ is not null. In that case most probably
    // osg::osgInit() hasn't been executed!
    osg_init_check();
  }

  ///
  virtual ~oPropertyT() 
  { }

public:

  oPropertyT& operator = (const oPropertyT& _rhs )
  {
    // Shallow copy! Remember, data_ is a osg pointer type, and the assign
    // operator makes a shallow copy!
    data_ = _rhs.data_;
    return *this;

  }

  
public: // interface BaseProperty

  virtual void reserve(size_t _n) { data_->getField().reserve( _n );  }
  virtual void resize(size_t _n)  { data_->resize( _n ); }
  virtual void push_back()        { data_->resize( data_->size()+1 ); }
  virtual void swap(size_t _i0, size_t _i1)
  { std::swap( data_->getField()[_i0], data_->getField()[_i1] ); }

  virtual oPropertyT<property_t>* clone() const
  {
    oPropertyT<property_t> *dolly = new oPropertyT<property_t>();
    if (n_elements() > 0)
    {
      // OSGGeoProperty does not provide a deep copy
      dolly->resize(n_elements());
      element_t *begin = const_cast<element_t*>(data());
      element_t *end   = begin+n_elements();
      element_t *dst   = const_cast<element_t*>(dolly->data());
      std::copy( begin, end, dst );
    }
    return dolly;
  }

public:

  virtual void set_persistent( bool _yn )
  {
    check_and_set_persistent<element_t>(_yn);
  }

  virtual size_t       n_elements() const
  { return data_==osg::NullFC ? UnknownSize : data_->getSize(); }

  virtual size_t       element_size() const
  { return UnknownSize; }
  
  virtual size_t store( std::ostream& _ostr, bool _swap ) const
  { return 0; }

  virtual size_t restore( std::istream& _istr, bool _swap )      
  { return 0; }

  
public: // OpenSG GeoPropertyInterface compatibility

  void clear(void) { data_->clear(); }


public: // access to OpenSG GeoProperty

  property_ptr_t& osg_ptr() 
  { return data_; }

  const property_ptr_t& osg_ptr() const
  { return data_; }


  const element_t *data() const  
  { return &( (*this)[ 0 ] ); }

  element_t& operator[](size_t idx) 
  { return data_->getField()[ idx ]; }

  const element_t& operator[](size_t idx) const 
  { return data_->getField()[ idx ]; }


protected:

  property_ptr_t  data_;


private:

  void osg_init_check(void)
  {
    // make sure data_ is not null. In that case most probably
    // osg::osgInit() hasn't been executed!
    if ( data_ == osg::NullFC )
      throw std::logic_error("OpenSG Runtime Environment is not initialized: " \
                             "Use osg::osgInit()");
  }

  oPropertyT( const oPropertyT& );
};

// ----------------------------------------------------------------- class ----


// ------------------------------------------------------------ properties ----

/// OpenSG Vertex Properties Adaptors.
namespace VP {

  // ---------------------------------------- Positions
  /// \name GeoPositions
  //@{
  /// Adaptor for osg::GeoPositions
  typedef oPropertyT< osg::GeoPositions2d > GeoPositions2d;
  typedef oPropertyT< osg::GeoPositions2f > GeoPositions2f;
  typedef oPropertyT< osg::GeoPositions3d > GeoPositions3d;
  typedef oPropertyT< osg::GeoPositions3f > GeoPositions3f;
  typedef oPropertyT< osg::GeoPositions4d > GeoPositions4d;
  typedef oPropertyT< osg::GeoPositions4f > GeoPositions4f;
  //@}

  // ---------------------------------------- Normals
  /// \name GeoNormals
  //@{
  /// Adaptor for osg::GeoNormals
  typedef oPropertyT< osg::GeoNormals3f > GeoNormals3f;
  //@}

  // ---------------------------------------- TexCoords
  /// \name GeoTexCoords
  //@{
  /// Adaptor for osg::GeoTexCoords
  typedef oPropertyT< osg::GeoTexCoords1f > GeoTexCoords1f;
  typedef oPropertyT< osg::GeoTexCoords2f > GeoTexCoords2f;
  typedef oPropertyT< osg::GeoTexCoords3f > GeoTexCoords3f;
  //@}

  // ---------------------------------------- Colors
  /// \name GeoColors
  //@{
  /// Adaptor for osg::GeoColors
  typedef oPropertyT< osg::GeoColors3f  > GeoColors3f;
  typedef oPropertyT< osg::GeoColors3ub > GeoColors3ub;
  typedef oPropertyT< osg::GeoColors4f  > GeoColors4f;
  typedef oPropertyT< osg::GeoColors4ub > GeoColors4ub;
  //@}

} // namespace VP


/// OpenSG Face Properties Adaptors.
namespace FP {

  // ---------------------------------------- Types
  /// Adaptor for osg::GeoPTypesUI8
  typedef oPropertyT< osg::GeoPTypesUI8 > GeoPTypesUI8;

  // ---------------------------------------- Lengths
  /// Adaptor for osg::GeoPLengthsUI32
  typedef oPropertyT< osg::GeoPLengthsUI32 > GeoPLengthsUI32;

  // ---------------------------------------- Indices

  typedef oPropertyT< osg::GeoIndicesUI32 >  _GeoIndicesUI32;

  /// Adaptor for osg::GeoIndicesUI32
  template < typename IsTriMesh >
  class GeoIndicesUI32 : public _GeoIndicesUI32
  {
  public: // ---------------------------------------- typedefs

    typedef _GeoIndicesUI32                      inherited_t;
    typedef typename inherited_t::property_ptr_t property_ptr_t;

  public: // ---------------------------------------- ctor/dtor

    GeoIndicesUI32( property_ptr_t     _geo_prop,
		    GeoPTypesUI8&      _types,
		    GeoPLengthsUI32&   _lengths)
      : inherited_t( _geo_prop ), types_(_types), length_(_lengths)
    { }

    GeoIndicesUI32( GeoPTypesUI8&      _types,
		    GeoPLengthsUI32&   _lengths)
      : inherited_t(), types_(_types), length_(_lengths)
    { }

    virtual ~GeoIndicesUI32() 
    { }

  public: // ---------------------------------------- inherited
    
    void swap(size_t _i0, size_t _i1) { _swap( _i0, _i1, IsTriMesh() ); }
    virtual void reserve(size_t _n)   { _reserve( _n, IsTriMesh() ); }
    virtual void resize(size_t _n)    { _resize( _n, IsTriMesh() ); }

  protected: // ------------------------------------- swap

    void _swap(size_t _i0, size_t _i1, GenProg::False )
    {
      omerr() << "Unsupported mesh type!" << std::endl;
      assert(0);
    }
    
    void _swap(size_t _i0, size_t _i1, GenProg::True )
    {
      size_t j0 = _i0 + _i0 + _i0;
      size_t j1 = _i1 + _i1 + _i1;

      inherited_t::swap(   j0,   j1 );
      inherited_t::swap( ++j0, ++j1 );
      inherited_t::swap( ++j0, ++j1 );
    }

    virtual void _reserve(size_t _n, GenProg::True )
    { inherited_t::reserve( _n + _n + _n ); }

    virtual void _reserve(size_t _n, GenProg::False )
    { assert( false ); }

    virtual void _resize(size_t _n, GenProg::True )
    { inherited_t::resize( _n + _n + _n ); }

    virtual void _resize(size_t _n, GenProg::False )
    { assert( false ); }


  protected:

    GeoPTypesUI8    &types_;
    GeoPLengthsUI32 &length_;

  };

} // namespace FP


// ----------------------------------------------------------------------------

#ifndef DOXY_IGNORE_THIS

template <typename T> struct _t2vp;
template <> struct _t2vp< osg::Pnt2f > 
{ typedef osg::GeoPositions2f type; typedef VP::GeoPositions2f prop; };

template <> struct _t2vp< osg::Pnt3f > 
{ typedef osg::GeoPositions3f type; typedef VP::GeoPositions3f prop; };

template <> struct _t2vp< osg::Pnt4f >
{ typedef osg::GeoPositions4f type; typedef VP::GeoPositions4f prop; };

template <> struct _t2vp< osg::Pnt2d >
{ typedef osg::GeoPositions2d type; typedef VP::GeoPositions2d prop; };
template <> struct _t2vp< osg::Pnt3d >
{ typedef osg::GeoPositions3d type; typedef VP::GeoPositions3d prop; };
template <> struct _t2vp< osg::Pnt4d >
{ typedef osg::GeoPositions4d type; typedef VP::GeoPositions4d prop; };

template <typename T> struct _t2vn;
template <> struct _t2vn< osg::Vec3f > 
{ typedef osg::GeoNormals3f   type; typedef VP::GeoNormals3f   prop; };

template <typename T> struct _t2vc;
template <> struct _t2vc< osg::Color3f >  
{ typedef osg::GeoColors3f  type;   typedef VP::GeoColors3f    prop; };

template <> struct _t2vc< osg::Color4f >
{ typedef osg::GeoColors4f  type;   typedef VP::GeoColors4f    prop; };

template <> struct _t2vc< osg::Color3ub >
{ typedef osg::GeoColors3ub  type;   typedef VP::GeoColors3ub    prop; };

template <> struct _t2vc< osg::Color4ub >
{ typedef osg::GeoColors4ub  type;   typedef VP::GeoColors3ub    prop; };

template <typename T> struct _t2vtc;
template <> struct _t2vtc< osg::Vec2f > 
{ typedef osg::GeoTexCoords2f type;  typedef VP::GeoTexCoords2f  prop; };

template <> struct _t2vtc< osg::Vec3f >
{ typedef osg::GeoTexCoords3f type;  typedef VP::GeoTexCoords3f  prop; };

#endif

//=============================================================================
} // namespace Kernel_OSG
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_PROPERTYT_HH defined
//=============================================================================

