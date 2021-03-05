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



/** \file Tools/Utils/NumLimitsT.hh
    Temporary solution until std::numeric_limits is standard.
 */

//=============================================================================
//
//  CLASS NumLimitsT
//
//=============================================================================

#ifndef OPENMESH_UTILS_NUMLIMITS_HH
#define OPENMESH_UTILS_NUMLIMITS_HH


//== INCLUDES =================================================================

#include "Config.hh"
#include <limits.h>
#include <float.h>


//== NAMESPEACES ==============================================================

namespace OpenMesh { // BEGIN_NS_OPENMESH
namespace Utils { // BEGIN_NS_UTILS


//== CLASS DEFINITION =========================================================


/** \class NumLimitsT Tools/Utils/NumLimitsT.hh 

    This class provides the maximum and minimum values a certain
    scalar type (\c int, \c float, or \c double) can store. You can
    use it like this:
    \code
    #include <OpenMesh/Utils/NumLimitsT.hh>

    int   float_min   = OpenMesh::NumLimitsT<float>::min();
    float double_max  = OpenMesh::NumLimitsT<double>::max();
    \endcode
    
    \note This functionality should be provided by
    std::numeric_limits.  This template does not exist on gcc <=
    2.95.3. The class template NumLimitsT is just a workaround.
**/
template <typename Scalar>
class NumLimitsT
{
public:
  /// Return the smallest \em absolte value a scalar type can store.
  static inline Scalar min() { return 0; }
  /// Return the maximum \em absolte value a scalar type can store.
  static inline Scalar max() { return 0; }

  static inline bool   is_float()   { return false; }
  static inline bool   is_integer() { return !NumLimitsT<Scalar>::is_float(); }
  static inline bool   is_signed()  { return true; }
};

  // is_float

template<> 
inline bool NumLimitsT<float>::is_float() { return true; }

template<> 
inline bool NumLimitsT<double>::is_float() { return true; }

template<> 
inline bool NumLimitsT<long double>::is_float() { return true; }

  // is_signed

template<> 
inline bool NumLimitsT<unsigned char>::is_signed() { return false; }

template<> 
inline bool NumLimitsT<unsigned short>::is_signed() { return false; }

template<> 
inline bool NumLimitsT<unsigned int>::is_signed() { return false; }

template<> 
inline bool NumLimitsT<unsigned long>::is_signed() { return false; }

template<> 
inline bool NumLimitsT<unsigned long long>::is_signed() { return false; }

  // min/max
template<> inline int  NumLimitsT<int>::min() { return INT_MIN; }
template<> inline int  NumLimitsT<int>::max() { return INT_MAX; }

template<> inline float NumLimitsT<float>::min() { return FLT_MIN; }
template<> inline float NumLimitsT<float>::max() { return FLT_MAX; }

template<> inline double NumLimitsT<double>::min() { return DBL_MIN; }
template<> inline double NumLimitsT<double>::max() { return DBL_MAX; }


//=============================================================================
} // END_NS_UTILS
} // END_NS_OPENMESH
//=============================================================================
#endif // OPENMESH_NUMLIMITS_HH defined
//=============================================================================

