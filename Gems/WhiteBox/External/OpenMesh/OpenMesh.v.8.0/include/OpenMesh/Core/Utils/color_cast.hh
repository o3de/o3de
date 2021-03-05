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
//  Helper Functions for binary reading / writing
//
//=============================================================================


#ifndef OPENMESH_COLOR_CAST_HH
#define OPENMESH_COLOR_CAST_HH


//== INCLUDES =================================================================


#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Utils/vector_cast.hh>

//== NAMESPACES ===============================================================


namespace OpenMesh {


//=============================================================================


/** \name Cast vector type to another vector type.
*/
//@{

//-----------------------------------------------------------------------------
#ifndef DOXY_IGNORE_THIS

/// Cast one color vector to another.
template <typename dst_t, typename src_t>
struct color_caster
{
  typedef dst_t  return_type;

  inline static return_type cast(const src_t& _src)
  {
    dst_t dst;
    vector_cast(_src, dst, GenProg::Int2Type<vector_traits<dst_t>::size_>());
    return dst;
  }
};


template <>
struct color_caster<Vec3uc,Vec3f>
{
  typedef Vec3uc return_type;

  inline static return_type cast(const Vec3f& _src)
  {
    return Vec3uc( (unsigned char)(_src[0]* 255.0f + 0.5f),
		               (unsigned char)(_src[1]* 255.0f + 0.5f),
		               (unsigned char)(_src[2]* 255.0f + 0.5f) );
  }
};

template <>
struct color_caster<Vec3uc,Vec4f>
{
  typedef Vec3uc return_type;

  inline static return_type cast(const Vec4f& _src)
  {
    return Vec3uc( (unsigned char)(_src[0]* 255.0f + 0.5f),
                   (unsigned char)(_src[1]* 255.0f + 0.5f),
                   (unsigned char)(_src[2]* 255.0f + 0.5f) );
  }
};

template <>
struct color_caster<Vec3i,Vec3f>
{
  typedef Vec3i return_type;

  inline static return_type cast(const Vec3f& _src)
  {
    return Vec3i( (int)(_src[0]* 255.0f + 0.5f),
                  (int)(_src[1]* 255.0f + 0.5f),
                  (int)(_src[2]* 255.0f + 0.5f) );
  }
};

template <>
struct color_caster<Vec3i,Vec4f>
{
  typedef Vec3i return_type;

  inline static return_type cast(const Vec4f& _src)
  {
    return Vec3i( (int)(_src[0]* 255.0f + 0.5f),
                  (int)(_src[1]* 255.0f + 0.5f),
                  (int)(_src[2]* 255.0f + 0.5f) );
  }
};

template <>
struct color_caster<Vec4i,Vec4f>
{
  typedef Vec4i return_type;

  inline static return_type cast(const Vec4f& _src)
  {
    return Vec4i( (int)(_src[0]* 255.0f + 0.5f),
                  (int)(_src[1]* 255.0f + 0.5f),
                  (int)(_src[2]* 255.0f + 0.5f),
                  (int)(_src[3]* 255.0f + 0.5f) );
  }
};

template <>
struct color_caster<Vec3ui,Vec3f>
{
  typedef Vec3ui return_type;

  inline static return_type cast(const Vec3f& _src)
  {
    return Vec3ui( (unsigned int)(_src[0]* 255.0f + 0.5f),
                   (unsigned int)(_src[1]* 255.0f + 0.5f),
                   (unsigned int)(_src[2]* 255.0f + 0.5f) );
  }
};

template <>
struct color_caster<Vec3ui,Vec4f>
{
  typedef Vec3ui return_type;

  inline static return_type cast(const Vec4f& _src)
  {
    return Vec3ui( (unsigned int)(_src[0]* 255.0f + 0.5f),
                   (unsigned int)(_src[1]* 255.0f + 0.5f),
                   (unsigned int)(_src[2]* 255.0f + 0.5f) );
  }
};

template <>
struct color_caster<Vec4ui,Vec4f>
{
  typedef Vec4ui return_type;

  inline static return_type cast(const Vec4f& _src)
  {
    return Vec4ui( (unsigned int)(_src[0]* 255.0f + 0.5f),
                   (unsigned int)(_src[1]* 255.0f + 0.5f),
                   (unsigned int)(_src[2]* 255.0f + 0.5f),
                   (unsigned int)(_src[3]* 255.0f + 0.5f) );
  }
};

template <>
struct color_caster<Vec4uc,Vec3f>
{
  typedef Vec4uc return_type;

  inline static return_type cast(const Vec3f& _src)
  {
    return Vec4uc( (unsigned char)(_src[0]* 255.0f + 0.5f),
                   (unsigned char)(_src[1]* 255.0f + 0.5f),
                   (unsigned char)(_src[2]* 255.0f + 0.5f),
                   (unsigned char)(255) );
  }
};

template <>
struct color_caster<Vec4f,Vec3f>
{
  typedef Vec4f return_type;

  inline static return_type cast(const Vec3f& _src)
  {
    return Vec4f( _src[0],
                  _src[1],
                  _src[2],
                  1.0f );
  }
};

template <>
struct color_caster<Vec4ui,Vec3uc>
{
  typedef Vec4ui return_type;

  inline static return_type cast(const Vec3uc& _src)
  {
    return Vec4ui(_src[0],
                  _src[1],
                  _src[2],
                  255 );
  }
};

template <>
struct color_caster<Vec4f,Vec3i>
{
  typedef Vec4f return_type;

  inline static return_type cast(const Vec3i& _src)
  {
    const float f = 1.0f / 255.0f;
    return Vec4f(_src[0]*f, _src[1]*f, _src[2]*f, 1.0f );
  }
};

template <>
struct color_caster<Vec4uc,Vec4f>
{
  typedef Vec4uc return_type;

  inline static return_type cast(const Vec4f& _src)
  {
    return Vec4uc( (unsigned char)(_src[0]* 255.0f + 0.5f),
                   (unsigned char)(_src[1]* 255.0f + 0.5f),
                   (unsigned char)(_src[2]* 255.0f + 0.5f),
                   (unsigned char)(_src[3]* 255.0f + 0.5f) );
  }
};

template <>
struct color_caster<Vec4f,Vec4i>
{
  typedef Vec4f return_type;

  inline static return_type cast(const Vec4i& _src)
  {
    const float f = 1.0f / 255.0f;
    return Vec4f( _src[0] * f, _src[1] *  f, _src[2] * f , _src[3] * f  );
  }
};

template <>
struct color_caster<Vec4uc,Vec3uc>
{
  typedef Vec4uc return_type;

  inline static return_type cast(const Vec3uc& _src)
  {
    return Vec4uc( _src[0], _src[1], _src[2], 255 );
  }
};

template <>
struct color_caster<Vec3f, Vec3uc>
{
  typedef Vec3f return_type;

  inline static return_type cast(const Vec3uc& _src)
  {
    const float f = 1.0f / 255.0f;
    return Vec3f(_src[0] * f, _src[1] *  f, _src[2] * f );
  }
};

template <>
struct color_caster<Vec3f, Vec4uc>
{
  typedef Vec3f return_type;

  inline static return_type cast(const Vec4uc& _src)
  {
    const float f = 1.0f / 255.0f;
    return Vec3f(_src[0] * f, _src[1] *  f, _src[2] * f );
  }
};

template <>
struct color_caster<Vec4f, Vec3uc>
{
  typedef Vec4f return_type;

  inline static return_type cast(const Vec3uc& _src)
  {
    const float f = 1.0f / 255.0f;
    return Vec4f(_src[0] * f, _src[1] *  f, _src[2] * f, 1.0f );
  }
};

template <>
struct color_caster<Vec4f, Vec4uc>
{
  typedef Vec4f return_type;

  inline static return_type cast(const Vec4uc& _src)
  {
    const float f = 1.0f / 255.0f;
    return Vec4f(_src[0] * f, _src[1] *  f, _src[2] * f, _src[3] * f );
  }
};

// ----------------------------------------------------------------------------


#ifndef DOXY_IGNORE_THIS

#if !defined(OM_CC_MSVC)
template <typename dst_t>
struct color_caster<dst_t,dst_t>
{
  typedef const dst_t&  return_type;

  inline static return_type cast(const dst_t& _src)
  {
    return _src;
  }
};
#endif

#endif

//-----------------------------------------------------------------------------


template <typename dst_t, typename src_t>
inline
typename color_caster<dst_t, src_t>::return_type
color_cast(const src_t& _src )
{
  return color_caster<dst_t, src_t>::cast(_src);
}

#endif
//-----------------------------------------------------------------------------

//@}


//=============================================================================
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_COLOR_CAST_HH defined
//=============================================================================

