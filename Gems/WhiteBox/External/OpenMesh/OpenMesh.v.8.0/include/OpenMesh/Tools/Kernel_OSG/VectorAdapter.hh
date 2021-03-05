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



// ----------------------------------------------------------------------------

#ifndef OPENMESH_KERNEL_OSG_VECTORADAPTER_HH
#define OPENMESH_KERNEL_OSG_VECTORADAPTER_HH


//== INCLUDES =================================================================

#include <osg/Geometry>
#include <OpenMesh/Core/Utils/vector_cast.hh>

//== NAMESPACES ===============================================================

namespace OpenMesh {

//== CLASS DEFINITION =========================================================

// ----------------------------------------------------------------- class ----

#define OSG_VECTOR_TRAITS( VecType ) \
  template <> struct vector_traits< VecType > { \
    typedef VecType                vector_type; \
    typedef vector_type::ValueType value_type;  \
    typedef GenProg::Int2Type< vector_type::_iSize > typed_size; \
    \
    static const size_t size_ = vector_type::_iSize; \
    static size_t size() { return size_; } \
  }

/// Vector traits for OpenSG vector type
OSG_VECTOR_TRAITS( osg::Pnt4f );
/// Vector traits for OpenSG vector type
OSG_VECTOR_TRAITS( osg::Pnt3f );
/// Vector traits for OpenSG vector type
OSG_VECTOR_TRAITS( osg::Pnt2f );

/// Vector traits for OpenSG vector type
OSG_VECTOR_TRAITS( osg::Vec4f );
/// Vector traits for OpenSG vector type
OSG_VECTOR_TRAITS( osg::Vec3f );
/// Vector traits for OpenSG vector type
OSG_VECTOR_TRAITS( osg::Vec2f );

/// Vector traits for OpenSG vector type
OSG_VECTOR_TRAITS( osg::Pnt4d );
/// Vector traits for OpenSG vector type
OSG_VECTOR_TRAITS( osg::Pnt3d );
/// Vector traits for OpenSG vector type
OSG_VECTOR_TRAITS( osg::Pnt2d );

/// Vector traits for OpenSG vector type
OSG_VECTOR_TRAITS( osg::Vec4d );
/// Vector traits for OpenSG vector type
OSG_VECTOR_TRAITS( osg::Vec3d );

/// Vector traits for OpenSG vector type
OSG_VECTOR_TRAITS( osg::Vec4ub );


// ----------------------------------------------------------------------------


#define OSG_COLOR_TRAITS( VecType, N ) \
  template <> struct vector_traits< VecType > { \
    typedef VecType                vector_type; \
    typedef vector_type::ValueType value_type;  \
    typedef GenProg::Int2Type< N > typed_size; \
    \
    static const size_t size_ = N; \
    static size_t size() { return size_; } \
  }


/// Vector traits for OpenSG color type
OSG_COLOR_TRAITS( osg::Color3ub, 3 );
/// Vector traits for OpenSG color type
OSG_COLOR_TRAITS( osg::Color4ub, 4 );
/// Vector traits for OpenSG color type
OSG_COLOR_TRAITS( osg::Color3f,  3 );
/// Vector traits for OpenSG color type
OSG_COLOR_TRAITS( osg::Color4f,  4 );

#undef OSG_VECTOR_TRAITS


// ----------------------------------------
#if 1
#define PNT2VEC_CASTER( DST, SRC ) \
  template <> struct vector_caster< DST, SRC > { \
    typedef DST   dst_t; \
    typedef SRC   src_t; \
    typedef const dst_t& return_type; \
    inline static return_type cast( const src_t& _src ) {\
      return _src.subZero(); \
    } \
  }

/// convert Pnt3f to Vec3f
PNT2VEC_CASTER( osg::Vec3f, osg::Pnt3f );

/// convert Pnt4f to Vec4f
PNT2VEC_CASTER( osg::Vec4f, osg::Pnt4f );

/// convert Pnt3d to Vec3d
PNT2VEC_CASTER( osg::Vec3d, osg::Pnt3d );

/// convert Pnt4d to Vec4d
PNT2VEC_CASTER( osg::Vec4d, osg::Pnt4d );

#undef PNT2VEC
#else
 
  template <> 
  struct vector_caster< osg::Vec3f, osg::Pnt3f > 
  {
    typedef osg::Vec3f   dst_t;
    typedef osg::Pnt3f   src_t;

    typedef const dst_t& return_type;
    inline static return_type cast( const src_t& _src ) 
    {
      std::cout << "casting Pnt3f to Vec3f\n";
      return _src.subZero();
    }
  };

#endif
// ----------------------------------------

//@{
/// Adapter for osg vector member computing a scalar product
inline
osg::Vec3f::ValueType dot( const osg::Vec3f &_v1, const osg::Vec3f &_v2 )
{ return _v1.dot(_v2); }


inline
osg::Vec3f::ValueType dot( const osg::Vec3f &_v1, const osg::Pnt3f &_v2 )
{ return _v1.dot(_v2); }


inline
osg::Vec2f::ValueType dot( const osg::Vec2f &_v1, const osg::Vec2f &_v2 )
{ return _v1.dot(_v2); }


inline
osg::Vec3f cross( const osg::Vec3f &_v1, const osg::Vec3f &_v2 )
{ return _v1.cross(_v2); }
//@}

//=============================================================================
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_VECTORADAPTER_HH defined
//=============================================================================

