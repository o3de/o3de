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


#ifndef OPENMESH_VECTORCAST_HH
#define OPENMESH_VECTORCAST_HH


//== INCLUDES =================================================================


#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Utils/vector_traits.hh>
#include <OpenMesh/Core/Utils/GenProg.hh>
#include <OpenMesh/Core/Geometry/VectorT.hh>


//== NAMESPACES ===============================================================


namespace OpenMesh {


//=============================================================================


/** \name Cast vector type to another vector type.
*/
//@{

//-----------------------------------------------------------------------------

template <typename src_t, typename dst_t, int n>
inline void vector_cast( const src_t &_src, dst_t &_dst, GenProg::Int2Type<n> )
{
  assert_compile(vector_traits<dst_t>::size_ <= vector_traits<src_t>::size_)
  vector_cast(_src,_dst, GenProg::Int2Type<n-1>());
  _dst[n-1] = static_cast<typename vector_traits<dst_t>::value_type >(_src[n-1]);
}

template <typename src_t, typename dst_t>
inline void vector_cast( const src_t & /*_src*/, dst_t & /*_dst*/, GenProg::Int2Type<0> )
{
}

template <typename src_t, typename dst_t, int n>
inline void vector_copy( const src_t &_src, dst_t &_dst, GenProg::Int2Type<n> )
{
  assert_compile(vector_traits<dst_t>::size_ <= vector_traits<src_t>::size_)
  vector_copy(_src,_dst, GenProg::Int2Type<n-1>());
  _dst[n-1] = _src[n-1];
}

template <typename src_t, typename dst_t>
inline void vector_copy( const src_t & /*_src*/, dst_t & /*_dst*/ , GenProg::Int2Type<0> )
{
}



//-----------------------------------------------------------------------------
#ifndef DOXY_IGNORE_THIS

template <typename dst_t, typename src_t>
struct vector_caster
{
  typedef dst_t  return_type;

  inline static return_type cast(const src_t& _src)
  {
    dst_t dst;
    vector_cast(_src, dst, GenProg::Int2Type<vector_traits<dst_t>::size_>());
    return dst;
  }
};

#if !defined(OM_CC_MSVC)
template <typename dst_t>
struct vector_caster<dst_t,dst_t>
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


/// Cast vector type to another vector type by copying the vector elements
template <typename dst_t, typename src_t>
inline
typename vector_caster<dst_t, src_t>::return_type
vector_cast(const src_t& _src )
{
  return vector_caster<dst_t, src_t>::cast(_src);
}


//@}


//=============================================================================
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_MESHREADER_HH defined
//=============================================================================
