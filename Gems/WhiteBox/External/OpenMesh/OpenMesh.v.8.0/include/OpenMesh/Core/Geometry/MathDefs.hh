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




#ifndef MATHDEFS_HH
#define MATHDEFS_HH

#include <cmath>
#include <cfloat>

#ifndef M_PI
  #define M_PI      3.14159265359
#endif

namespace OpenMesh
{

/** comparison operators with user-selected precision control
*/
template <class T, typename Real>
inline bool is_zero(const T& _a, Real _eps)
{ return fabs(_a) < _eps; }

template <class T1, class T2, typename Real>
inline bool is_eq(const T1& a, const T2& b, Real _eps)
{ return is_zero(a-b, _eps); }

template <class T1, class T2, typename Real>
inline bool is_gt(const T1& a, const T2& b, Real _eps)
{ return (a > b) && !is_eq(a,b,_eps); }

template <class T1, class T2, typename Real>
inline bool is_ge(const T1& a, const T2& b, Real _eps)
{ return (a > b) || is_eq(a,b,_eps); }

template <class T1, class T2, typename Real>
inline bool is_lt(const T1& a, const T2& b, Real _eps)
{ return (a < b) && !is_eq(a,b,_eps); }

template <class T1, class T2, typename Real>
inline bool is_le(const T1& a, const T2& b, Real _eps)
{ return (a < b) || is_eq(a,b,_eps); }

/*const float flt_eps__ = 10*FLT_EPSILON;
const double dbl_eps__ = 10*DBL_EPSILON;*/
const float flt_eps__ = (float)1e-05;
const double dbl_eps__ = 1e-09;

inline float eps__(float) 
{ return flt_eps__; }

inline double eps__(double)
{ return dbl_eps__; }

template <class T>
inline bool is_zero(const T& a)
{ return is_zero(a, eps__(a)); }

template <class T1, class T2>
inline bool is_eq(const T1& a, const T2& b)
{ return is_zero(a-b); }

template <class T1, class T2>
inline bool is_gt(const T1& a, const T2& b)
{ return (a > b) && !is_eq(a,b); }

template <class T1, class T2>
inline bool is_ge(const T1& a, const T2& b)
{ return (a > b) || is_eq(a,b); }

template <class T1, class T2>
inline bool is_lt(const T1& a, const T2& b)
{ return (a < b) && !is_eq(a,b); }

template <class T1, class T2>
inline bool is_le(const T1& a, const T2& b)
{ return (a < b) || is_eq(a,b); }

/// Trigonometry/angles - related

template <class T>
inline T sane_aarg(T _aarg)
{
  if (_aarg < -1)
  {
    _aarg = -1;
  }
  else if (_aarg >  1)
  {
    _aarg = 1;
  }
  return _aarg;
}

/** returns the angle determined by its cos and the sign of its sin
    result is positive if the angle is in [0:pi]
    and negative if it is in [pi:2pi]
*/
template <class T>
T angle(T _cos_angle, T _sin_angle)
{//sanity checks - otherwise acos will return nan
  _cos_angle = sane_aarg(_cos_angle);
  return (T) _sin_angle >= 0 ? acos(_cos_angle) : -acos(_cos_angle);
}

template <class T>
inline T positive_angle(T _angle)
{ return _angle < 0 ? (2*M_PI + _angle) : _angle; }

template <class T>
inline T positive_angle(T _cos_angle, T _sin_angle)
{ return positive_angle(angle(_cos_angle, _sin_angle)); }

template <class T>
inline T deg_to_rad(const T& _angle)
{ return M_PI*(_angle/180); }

template <class T>
inline T rad_to_deg(const T& _angle)
{ return 180*(_angle/M_PI); }

inline double log_(double _value)
{ return log(_value); }

}//namespace OpenMesh

#endif//MATHDEFS_HH
