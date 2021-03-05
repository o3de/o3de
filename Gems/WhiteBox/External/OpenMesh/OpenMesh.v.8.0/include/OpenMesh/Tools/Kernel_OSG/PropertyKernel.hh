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



#ifndef OPENMESH_KERNEL_OSG_PROPERTYKERNEL_HH
#define OPENMESH_KENREL_OSG_PROPERTYKERNEL_HH


//== INCLUDES =================================================================

#include <OpenMesh/Core/Utils/Property.hh>
#include <OpenMesh/Core/Mesh/BaseKernel.hh>
// --------------------
#include <OpenMesh/Tools/Kernel_OSG/PropertyT.hh>

//== NAMESPACES ===============================================================

namespace OpenMesh {
namespace Kernel_OSG {

//== CLASS DEFINITION =========================================================

/** Helper class, extending functionaliy of OpenMesh::BaseKernel to OpenSG
 *  specific property adaptors.
 *  \internal
 *  \todo Follow coding convention and rename class to PropertyKernelT
 */
template < typename IsTriMesh >
class PropertyKernel : public OpenMesh::BaseKernel
{
public:

  // --------------------------------------------------------------- item types

  typedef FPropHandleT<osg::UInt8>      FPTypesHandle;
  typedef FPropHandleT<osg::UInt32>     FPLengthsHandle;
  typedef FPropHandleT<osg::UInt32>     FIndicesHandle;

  typedef FP::GeoPTypesUI8              GeoPTypes;
  typedef FP::GeoPLengthsUI32           GeoPLengths;
  typedef FP::GeoIndicesUI32<IsTriMesh> GeoIndices;

  // ------------------------------------------------- constructor / destructor

  PropertyKernel() {}
  virtual ~PropertyKernel() { }


protected: // ---------------------------------------------- add osg properties

  // -------------------- vertex properties

  template < typename T >
  VPropHandleT<T> add_vpositions( const T& _t, const std::string& _n )
  { return VPropHandleT<T>(_add_vprop( new typename _t2vp<T>::prop(_n))); }

  template < typename T >
  VPropHandleT<T> add_vnormals( const T& _t, const std::string& _n )
  { return VPropHandleT<T>(_add_vprop( new typename _t2vn<T>::prop(_n) )); }

  template < typename T >
  VPropHandleT<T> add_vcolors( const T& _t, const std::string& _n )
  { return VPropHandleT<T>(_add_vprop( new typename _t2vc<T>::prop(_n) )); }

  template < typename T >
  VPropHandleT<T> add_vtexcoords( const T& _t, const std::string& _n )
  { return VPropHandleT<T>(_add_vprop( new typename _t2vtc<T>::prop(_n) )); }


  // -------------------- face properties

  FPTypesHandle add_fptypes( )
  { return FPTypesHandle(_add_fprop(new GeoPTypes)); }

  FPLengthsHandle add_fplengths( )
  { return FPLengthsHandle(_add_fprop(new GeoPLengths)); }

  FIndicesHandle add_findices( FPTypesHandle _pht, FPLengthsHandle _phl )
  { 
    GeoIndices *bp = new GeoIndices( fptypes(_pht), fplengths(_phl ) );
    return FIndicesHandle(_add_fprop( bp ) ); 
  }

protected: // ------------------------------------------- access osg properties
  
  template < typename T >
  typename _t2vp<T>::prop& vpositions( VPropHandleT<T> _ph ) 
  { return static_cast<typename _t2vp<T>::prop&>( _vprop( _ph ) ); }

  template < typename T >
  const typename _t2vp<T>::prop& vpositions( VPropHandleT<T> _ph) const
  { return static_cast<const typename _t2vp<T>::prop&>( _vprop( _ph ) ); }


  template < typename T >
  typename _t2vn<T>::prop& vnormals( VPropHandleT<T> _ph ) 
  { return static_cast<typename _t2vn<T>::prop&>( _vprop( _ph ) ); }

  template < typename T >
  const typename _t2vn<T>::prop& vnormals( VPropHandleT<T> _ph) const
  { return static_cast<const typename _t2vn<T>::prop&>( _vprop( _ph ) ); }


  template < typename T >
  typename _t2vc<T>::prop& vcolors( VPropHandleT<T> _ph )
  { return static_cast<typename _t2vc<T>::prop&>( _vprop( _ph ) ); }

  template < typename T >
  const typename _t2vc<T>::prop& vcolors( VPropHandleT<T> _ph ) const
  { return static_cast<const typename _t2vc<T>::prop&>( _vprop( _ph ) ); }


  template < typename T >
  typename _t2vtc<T>::prop& vtexcoords( VPropHandleT<T> _ph )
  { return static_cast<typename _t2vtc<T>::prop&>( _vprop( _ph ) ); }

  template < typename T >
  const typename _t2vtc<T>::prop& vtexcoords( VPropHandleT<T> _ph ) const
  { return static_cast<const typename _t2vtc<T>::prop&>( _vprop( _ph ) ); }


  //
  GeoPTypes& fptypes( FPTypesHandle _ph )
  { return static_cast<GeoPTypes&>( _fprop(_ph) ); }

  const GeoPTypes& fptypes( FPTypesHandle _ph ) const
  { return static_cast<const GeoPTypes&>( _fprop(_ph) ); }


  GeoPLengths& fplengths( FPLengthsHandle _ph )
  { return static_cast<GeoPLengths&>( _fprop(_ph) ); }

  const GeoPLengths& fplengths( FPLengthsHandle _ph ) const
  { return static_cast<const GeoPLengths&>( _fprop(_ph) ); }


  GeoIndices& findices( FIndicesHandle _ph )
  { return static_cast<GeoIndices&>( _fprop(_ph) ); }

  const GeoIndices& findices( FIndicesHandle _ph ) const
  { return static_cast<const GeoIndices&>( _fprop(_ph) ); }

    
protected: // ------------------------------------ access osg property elements

  template <typename T> 
  T& vpositions( VPropHandleT<T> _ph, VertexHandle _vh ) 
  { return vpositions(_ph)[_vh.idx()]; }

  template <class T>
  const T& vpositions( VPropHandleT<T> _ph, VertexHandle _vh ) const 
  { return vpositions(_ph)[_vh.idx()]; }


  template < typename T> 
  T& vnormals( VPropHandleT<T> _ph, VertexHandle _vh ) 
  { return vnormals(_ph)[_vh.idx()]; }

  template <class T>
  const T& vnormals( VPropHandleT<T> _ph, VertexHandle _vh ) const 
  { return vnormals(_ph)[_vh.idx()]; }


  template < typename T> 
  T& vcolors( VPropHandleT<T> _ph, VertexHandle _vh ) 
  { return vcolors(_ph)[_vh.idx()]; }

  template <class T>
  const T& vcolors( VPropHandleT<T> _ph, VertexHandle _vh ) const 
  { return vcolors(_ph)[_vh.idx()]; }


  template < typename T> 
  T& vtexcoords( VPropHandleT<T> _ph, VertexHandle _vh ) 
  { return vtexcoords(_ph)[_vh.idx()]; }

  template <class T>
  const T& vtexcoords( VPropHandleT<T> _ph, VertexHandle _vh ) const 
  { return vtexcoords(_ph)[_vh.idx()]; }


  // -------------------- access face property elements

  FPTypesHandle::value_type& 
  fptypes( FPTypesHandle _ph, FaceHandle _fh )
  { return fptypes( _ph )[ _fh.idx()]; }

  const FPTypesHandle::value_type& 
  fptypes( FPTypesHandle _ph, FaceHandle _fh ) const
  { return fptypes( _ph )[ _fh.idx()]; }


  FPLengthsHandle::value_type& 
  fplengths( FPLengthsHandle _ph, FaceHandle _fh )
  { return fplengths( _ph )[ _fh.idx()]; }

  const FPLengthsHandle::value_type& 
  fplengths( FPLengthsHandle _ph, FaceHandle _fh ) const
  { return fplengths( _ph )[ _fh.idx()]; }


  FIndicesHandle::value_type& 
  findices( FIndicesHandle _ph, FaceHandle _fh )
  { return findices( _ph )[ _fh.idx()]; }

  const FIndicesHandle::value_type& 
  findices( FIndicesHandle _ph, FaceHandle _fh ) const
  { return findices( _ph )[ _fh.idx()]; }

public:

  void stats(void)
  {
    std::cout << "#V : "  << n_vertices() << std::endl;
    std::cout << "#E : "  << n_edges() << std::endl;
    std::cout << "#F : "  << n_faces() << std::endl;
    property_stats();
  }
};


//=============================================================================
} // namespace Kernel_OSG
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_KERNEL_OSG_PROPERTYKERNEL_HH defined
//=============================================================================

