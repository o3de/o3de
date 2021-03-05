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
//  CLASS VectorT
//
//=============================================================================

// Don't parse this header file with doxygen since
// for some reason (obviously due to a bug in doxygen,
// bugreport: https://bugzilla.gnome.org/show_bug.cgi?id=629182)
// macro expansion and preprocessor defines
// don't work properly.

#if ((defined(_MSC_VER) && (_MSC_VER >= 1900)) || __cplusplus > 199711L || defined(__GXX_EXPERIMENTAL_CXX0X__)) && !defined(OPENMESH_VECTOR_LEGACY)
#include "Vector11T.hh"
#else
#ifndef DOXYGEN

#ifndef OPENMESH_VECTOR_HH
#define OPENMESH_VECTOR_HH


//== INCLUDES =================================================================

#include <OpenMesh/Core/System/config.h>
#include <ostream>
#include <cmath>
#include <cassert>
#include <cstring>

#if defined(__GNUC__) && defined(__SSE__)
#include <xmmintrin.h>
#endif

//== NAMESPACES ===============================================================


namespace OpenMesh {


//== CLASS DEFINITION =========================================================


/** The N values of the template Scalar type are the only data members
    of the class VectorT<Scalar,N>. This guarantees 100% compatibility
    with arrays of type Scalar and size N, allowing us to define the
    cast operators to and from arrays and array pointers.

    In addition, this class will be specialized for Vec4f to be 16 bit
    aligned, so that aligned SSE instructions can be used on these
    vectors.
*/
template<typename Scalar, int N> class VectorDataT {
    public:
        Scalar values_[N];
};


#if defined(__GNUC__) && defined(__SSE__)

/// This specialization enables us to use aligned SSE instructions.
template<> class VectorDataT<float, 4> {
    public:
        union {
            __m128 m128;
            float values_[4];
        };
};

#endif




//== CLASS DEFINITION =========================================================


#define DIM               N
#define TEMPLATE_HEADER   template <typename Scalar, int N>
#define CLASSNAME         VectorT
#define DERIVED           VectorDataT<Scalar,N>
#define unroll(expr)      for (int i=0; i<N; ++i) expr(i)

/** \class VectorT VectorT.hh <OpenMesh/Core/Math/VectorT.hh>
    A vector is an array of \<N\> values of type \<Scalar\>.
    The actual data is stored in an VectorDataT, this class just adds
    the necessary operators.
*/
#include "VectorT_inc.hh"

#undef  DIM
#undef  TEMPLATE_HEADER
#undef  CLASSNAME
#undef  DERIVED
#undef  unroll




//== PARTIAL TEMPLATE SPECIALIZATIONS =========================================
#if OM_PARTIAL_SPECIALIZATION


#define TEMPLATE_HEADER        template <typename Scalar>
#define CLASSNAME              VectorT<Scalar,DIM> 
#define DERIVED                VectorDataT<Scalar,DIM>


#define DIM                    2
#define unroll(expr)           expr(0) expr(1)
#define unroll_comb(expr, op)  expr(0) op expr(1)
#define unroll_csv(expr)       expr(0), expr(1)
#include "VectorT_inc.hh"
#undef  DIM
#undef  unroll
#undef  unroll_comb
#undef  unroll_csv


#define DIM                    3
#define unroll(expr)           expr(0) expr(1) expr(2)
#define unroll_comb(expr, op)  expr(0) op expr(1) op expr(2)
#define unroll_csv(expr)       expr(0), expr(1), expr(2)
#include "VectorT_inc.hh"
#undef  DIM
#undef  unroll
#undef  unroll_comb
#undef  unroll_csv


#define DIM                    4
#define unroll(expr)           expr(0) expr(1) expr(2) expr(3)
#define unroll_comb(expr, op)  expr(0) op expr(1) op expr(2) op expr(3)
#define unroll_csv(expr)       expr(0), expr(1), expr(2), expr(3)
#include "VectorT_inc.hh"
#undef  DIM
#undef  unroll
#undef  unroll_comb
#undef  unroll_csv
    
#define DIM                    5
#define unroll(expr)           expr(0) expr(1) expr(2) expr(3) expr(4)
#define unroll_comb(expr, op)  expr(0) op expr(1) op expr(2) op expr(3) op expr(4)
#define unroll_csv(expr)       expr(0), expr(1), expr(2), expr(3), expr(4)
#include "VectorT_inc.hh"
#undef  DIM
#undef  unroll
#undef  unroll_comb
#undef  unroll_csv

#define DIM                    6
#define unroll(expr)           expr(0) expr(1) expr(2) expr(3) expr(4) expr(5)
#define unroll_comb(expr, op)  expr(0) op expr(1) op expr(2) op expr(3) op expr(4) op expr(5)
#define unroll_csv(expr)       expr(0), expr(1), expr(2), expr(3), expr(4), expr(5)
#include "VectorT_inc.hh"
#undef  DIM
#undef  unroll
#undef  unroll_comb
#undef  unroll_csv
    

#undef  TEMPLATE_HEADER
#undef  CLASSNAME
#undef  DERIVED




//== FULL TEMPLATE SPECIALIZATIONS ============================================
#else

/// cross product for Vec3f
template<>
inline VectorT<float,3>
VectorT<float,3>::operator%(const VectorT<float,3>& _rhs) const 
{
   return 
     VectorT<float,3>(values_[1]*_rhs.values_[2]-values_[2]*_rhs.values_[1],
		      values_[2]*_rhs.values_[0]-values_[0]*_rhs.values_[2],
		      values_[0]*_rhs.values_[1]-values_[1]*_rhs.values_[0]);
}
  

/// cross product for Vec3d
template<>
inline VectorT<double,3>
VectorT<double,3>::operator%(const VectorT<double,3>& _rhs) const
{
 return 
   VectorT<double,3>(values_[1]*_rhs.values_[2]-values_[2]*_rhs.values_[1],
		     values_[2]*_rhs.values_[0]-values_[0]*_rhs.values_[2],
		     values_[0]*_rhs.values_[1]-values_[1]*_rhs.values_[0]);
}

#endif



//== GLOBAL FUNCTIONS =========================================================


/// \relates OpenMesh::VectorT
/// scalar * vector
template<typename Scalar1, typename Scalar2,int N>
inline VectorT<Scalar1,N> operator*(Scalar2 _s, const VectorT<Scalar1,N>& _v) {
  return _v*_s;
}


/// \relates OpenMesh::VectorT
/// symmetric version of the dot product
template<typename Scalar, int N>
inline Scalar 
dot(const VectorT<Scalar,N>& _v1, const VectorT<Scalar,N>& _v2) {
  return (_v1 | _v2); 
}


/// \relates OpenMesh::VectorT
/// symmetric version of the cross product
template<typename Scalar, int N>
inline VectorT<Scalar,N> 
cross(const VectorT<Scalar,N>& _v1, const VectorT<Scalar,N>& _v2) {
  return (_v1 % _v2);
}


/// \relates OpenMesh::VectorT
/// non-member norm
template<typename Scalar, int DIM>
Scalar norm(const VectorT<Scalar, DIM>& _v) {
    return _v.norm();
}


/// \relates OpenMesh::VectorT
/// non-member sqrnorm
template<typename Scalar, int DIM>
Scalar sqrnorm(const VectorT<Scalar, DIM>& _v) {
    return _v.sqrnorm();
}


/// \relates OpenMesh::VectorT
/// non-member vectorize
template<typename Scalar, int DIM, typename OtherScalar>
VectorT<Scalar, DIM>& vectorize(VectorT<Scalar, DIM>& _v, OtherScalar const& _val) {
    return _v.vectorize(_val);
}


/// \relates OpenMesh::VectorT
/// non-member normalize
template<typename Scalar, int DIM>
VectorT<Scalar, DIM>& normalize(VectorT<Scalar, DIM>& _v) {
    return _v.normalize();
}


/// \relates OpenMesh::VectorT
/// non-member maximize
template<typename Scalar, int DIM>
VectorT<Scalar, DIM>& maximize(VectorT<Scalar, DIM>& _v1, VectorT<Scalar, DIM>& _v2) {
    return _v1.maximize(_v2);
}


/// \relates OpenMesh::VectorT
/// non-member minimize
template<typename Scalar, int DIM>
VectorT<Scalar, DIM>& minimize(VectorT<Scalar, DIM>& _v1, VectorT<Scalar, DIM>& _v2) {
    return _v1.minimize(_v2);
}


//== TYPEDEFS =================================================================

/** 1-byte signed vector */
typedef VectorT<signed char,1> Vec1c;
/** 1-byte unsigned vector */
typedef VectorT<unsigned char,1> Vec1uc;
/** 1-short signed vector */
typedef VectorT<signed short int,1> Vec1s;
/** 1-short unsigned vector */
typedef VectorT<unsigned short int,1> Vec1us;
/** 1-int signed vector */
typedef VectorT<signed int,1> Vec1i;
/** 1-int unsigned vector */
typedef VectorT<unsigned int,1> Vec1ui;
/** 1-float vector */
typedef VectorT<float,1> Vec1f;
/** 1-double vector */
typedef VectorT<double,1> Vec1d;

/** 2-byte signed vector */
typedef VectorT<signed char,2> Vec2c;
/** 2-byte unsigned vector */
typedef VectorT<unsigned char,2> Vec2uc;
/** 2-short signed vector */
typedef VectorT<signed short int,2> Vec2s;
/** 2-short unsigned vector */
typedef VectorT<unsigned short int,2> Vec2us;
/** 2-int signed vector */
typedef VectorT<signed int,2> Vec2i;
/** 2-int unsigned vector */
typedef VectorT<unsigned int,2> Vec2ui;
/** 2-float vector */
typedef VectorT<float,2> Vec2f;
/** 2-double vector */
typedef VectorT<double,2> Vec2d;

/** 3-byte signed vector */
typedef VectorT<signed char,3> Vec3c;
/** 3-byte unsigned vector */
typedef VectorT<unsigned char,3> Vec3uc;
/** 3-short signed vector */
typedef VectorT<signed short int,3> Vec3s;
/** 3-short unsigned vector */
typedef VectorT<unsigned short int,3> Vec3us;
/** 3-int signed vector */
typedef VectorT<signed int,3> Vec3i;
/** 3-int unsigned vector */
typedef VectorT<unsigned int,3> Vec3ui;
/** 3-float vector */
typedef VectorT<float,3> Vec3f;
/** 3-double vector */
typedef VectorT<double,3> Vec3d;
/** 3-bool vector */
typedef VectorT<bool,3> Vec3b;

/** 4-byte signed vector */
typedef VectorT<signed char,4> Vec4c;
/** 4-byte unsigned vector */
typedef VectorT<unsigned char,4> Vec4uc;
/** 4-short signed vector */
typedef VectorT<signed short int,4> Vec4s;
/** 4-short unsigned vector */
typedef VectorT<unsigned short int,4> Vec4us;
/** 4-int signed vector */
typedef VectorT<signed int,4> Vec4i;
/** 4-int unsigned vector */
typedef VectorT<unsigned int,4> Vec4ui;
/** 4-float vector */
typedef VectorT<float,4> Vec4f;
/** 4-double vector */
typedef VectorT<double,4> Vec4d;

/** 5-byte signed vector */
typedef VectorT<signed char, 5> Vec5c;
/** 5-byte unsigned vector */
typedef VectorT<unsigned char, 5> Vec5uc;
/** 5-short signed vector */
typedef VectorT<signed short int, 5> Vec5s;
/** 5-short unsigned vector */
typedef VectorT<unsigned short int, 5> Vec5us;
/** 5-int signed vector */
typedef VectorT<signed int, 5> Vec5i;
/** 5-int unsigned vector */
typedef VectorT<unsigned int, 5> Vec5ui;
/** 5-float vector */
typedef VectorT<float, 5> Vec5f;
/** 5-double vector */
typedef VectorT<double, 5> Vec5d;

/** 6-byte signed vector */
typedef VectorT<signed char,6> Vec6c;
/** 6-byte unsigned vector */
typedef VectorT<unsigned char,6> Vec6uc;
/** 6-short signed vector */
typedef VectorT<signed short int,6> Vec6s;
/** 6-short unsigned vector */
typedef VectorT<unsigned short int,6> Vec6us;
/** 6-int signed vector */
typedef VectorT<signed int,6> Vec6i;
/** 6-int unsigned vector */
typedef VectorT<unsigned int,6> Vec6ui;
/** 6-float vector */
typedef VectorT<float,6> Vec6f;
/** 6-double vector */
typedef VectorT<double,6> Vec6d;


//=============================================================================
} // namespace OpenMesh
//=============================================================================


#endif // OPENMESH_VECTOR_HH defined
//=============================================================================
#endif // DOXYGEN
#endif // C++11
