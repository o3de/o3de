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





// Set template keywords and class names properly when
// parsing with doxygen. This only seems to work this way since
// the scope of preprocessor defines is limited to one file in doxy.
#ifdef DOXYGEN

// Only used for correct doxygen parsing
#define OPENMESH_VECTOR_HH

#define DIM               N
#define TEMPLATE_HEADER   template <typename Scalar, int N>
#define CLASSNAME         VectorT
#define DERIVED           VectorDataT<Scalar,N>
#define unroll(expr)      for (int i=0; i<N; ++i) expr(i)

#endif

#if defined( OPENMESH_VECTOR_HH )

// ----------------------------------------------------------------------------

TEMPLATE_HEADER
class CLASSNAME : public DERIVED
{
private:
  typedef DERIVED                           Base;
public:

  //---------------------------------------------------------------- class info

  /// the type of the scalar used in this template
  typedef Scalar value_type;

  /// type of this vector
  typedef VectorT<Scalar,DIM>  vector_type;

  /// returns dimension of the vector (deprecated)
  static inline int dim() { return DIM; }

  /// returns dimension of the vector
  static inline size_t size() { return DIM; }

  static const size_t size_ = DIM;


  //-------------------------------------------------------------- constructors

  /// default constructor creates uninitialized values.
  inline VectorT() {}

  /// special constructor for 1D vectors
  explicit inline VectorT(const Scalar& v) {
//     assert(DIM==1);
//     values_[0] = v0;
    vectorize(v);
  }

#if DIM == 2
  /// special constructor for 2D vectors
  inline VectorT(const Scalar v0, const Scalar v1) {
    Base::values_[0] = v0; Base::values_[1] = v1;
  }
#endif

#if DIM == 3
  /// special constructor for 3D vectors
  inline VectorT(const Scalar v0, const Scalar v1, const Scalar v2) {
    Base::values_[0]=v0; Base::values_[1]=v1; Base::values_[2]=v2;
  }
#endif

#if DIM == 4
  /// special constructor for 4D vectors
  inline VectorT(const Scalar v0, const Scalar v1,
     const Scalar v2, const Scalar v3) {
    Base::values_[0]=v0; Base::values_[1]=v1; Base::values_[2]=v2; Base::values_[3]=v3;
  }

  VectorT homogenized() const { return VectorT(Base::values_[0]/Base::values_[3], Base::values_[1]/Base::values_[3], Base::values_[2]/Base::values_[3], 1); }
#endif

#if DIM == 5
  /// special constructor for 5D vectors
  inline VectorT(const Scalar v0, const Scalar v1, const Scalar v2,
     const Scalar v3, const Scalar v4) {
    Base::values_[0]=v0; Base::values_[1]=v1;Base::values_[2]=v2; Base::values_[3]=v3; Base::values_[4]=v4;
  }
#endif

#if DIM == 6
  /// special constructor for 6D vectors
  inline VectorT(const Scalar v0, const Scalar v1, const Scalar v2,
     const Scalar v3, const Scalar v4, const Scalar v5) {
    Base::values_[0]=v0; Base::values_[1]=v1; Base::values_[2]=v2;
    Base::values_[3]=v3; Base::values_[4]=v4; Base::values_[5]=v5;
  }
#endif

  /// construct from a value array (explicit)
  explicit inline VectorT(const Scalar _values[DIM]) {
    memcpy(data(), _values, DIM*sizeof(Scalar));
  }


#ifdef OM_CC_MIPS
  /// assignment from a vector of the same kind
  // mipspro need this method
  inline vector_type& operator=(const vector_type& _rhs) {
    memcpy(Base::values_, _rhs.Base::values_, DIM*sizeof(Scalar));
    return *this;
  }
#endif


  /// copy & cast constructor (explicit)
  template<typename otherScalarType>
  explicit inline VectorT(const VectorT<otherScalarType,DIM>& _rhs) {
    operator=(_rhs);
  }




  //--------------------------------------------------------------------- casts

  /// cast from vector with a different scalar type
  template<typename otherScalarType>
  inline vector_type& operator=(const VectorT<otherScalarType,DIM>& _rhs) {
#define expr(i)  Base::values_[i] = (Scalar)_rhs[i];
    unroll(expr);
#undef expr
    return *this;
  }

//   /// cast to Scalar array
//   inline operator Scalar*() { return Base::values_; }

//   /// cast to const Scalar array
//   inline operator const Scalar*() const { return Base::values_; }

  /// access to Scalar array
  inline Scalar* data() { return Base::values_; }

  /// access to const Scalar array
  inline const Scalar*data() const { return Base::values_; }


   //----------------------------------------------------------- element access

//    /// get i'th element read-write
//   inline Scalar& operator[](int _i) {
//     assert(_i>=0 && _i<DIM); return Base::values_[_i];
//   }

//   /// get i'th element read-only
//   inline const Scalar& operator[](int _i) const {
//     assert(_i>=0 && _i<DIM); return Base::values_[_i];
//   }

   /// get i'th element read-write
  inline Scalar& operator[](size_t _i) {
    assert(_i<DIM); return Base::values_[_i];
  }

  /// get i'th element read-only
  inline const Scalar& operator[](size_t _i) const {
    assert(_i<DIM); return Base::values_[_i];
  }




  //---------------------------------------------------------------- comparsion

  /// component-wise comparison
  inline bool operator==(const vector_type& _rhs) const {
#define expr(i) if(Base::values_[i]!=_rhs.Base::values_[i]) return false;
    unroll(expr);
#undef expr
    return true;
  }

  /// component-wise comparison
  inline bool operator!=(const vector_type& _rhs) const {
    return !(*this == _rhs);
  }




  //---------------------------------------------------------- scalar operators

  /// component-wise self-multiplication with scalar
  inline vector_type& operator*=(const Scalar& _s) {
#define expr(i) Base::values_[i] *= _s;
    unroll(expr);
#undef expr
    return *this;
  }

  /** component-wise self-division by scalar
      \attention v *= (1/_s) is much faster than this  */
  inline vector_type& operator/=(const Scalar& _s) {
#define expr(i) Base::values_[i] /= _s;
    unroll(expr);
#undef expr
    return *this;
  }


  /// component-wise multiplication with scalar
  inline vector_type operator*(const Scalar& _s) const {
#if DIM==N
    return vector_type(*this) *= _s;
#else
#define expr(i) Base::values_[i] * _s
    return vector_type(unroll_csv(expr));
#undef expr
#endif
  }


  /// component-wise division by with scalar
  inline vector_type operator/(const Scalar& _s) const {
#if DIM==N
    return vector_type(*this) /= _s;
#else
#define expr(i) Base::values_[i] / _s
    return vector_type(unroll_csv(expr));
#undef expr
#endif
  }






  //---------------------------------------------------------- vector operators

  /// component-wise self-multiplication
  inline vector_type& operator*=(const vector_type& _rhs) {
#define expr(i) Base::values_[i] *= _rhs[i];
    unroll(expr);
#undef expr
    return *this;
  }

  /// component-wise self-division
  inline vector_type& operator/=(const vector_type& _rhs) {
#define expr(i) Base::values_[i] /= _rhs[i];
    unroll(expr);
#undef expr
    return *this;
  }

  /// vector difference from this
  inline vector_type& operator-=(const vector_type& _rhs) {
#define expr(i) Base::values_[i] -= _rhs[i];
    unroll(expr);
#undef expr
    return *this;
  }

  /// vector self-addition
  inline vector_type& operator+=(const vector_type& _rhs) {
#define expr(i) Base::values_[i] += _rhs[i];
    unroll(expr);
#undef expr
    return *this;
  }


  /// component-wise vector multiplication
  inline vector_type operator*(const vector_type& _v) const {
#if DIM==N
    return vector_type(*this) *= _v;
#else
#define expr(i) Base::values_[i] * _v.Base::values_[i]
    return vector_type(unroll_csv(expr));
#undef expr
#endif
  }


  /// component-wise vector division
  inline vector_type operator/(const vector_type& _v) const {
#if DIM==N
    return vector_type(*this) /= _v;
#else
#define expr(i) Base::values_[i] / _v.Base::values_[i]
    return vector_type(unroll_csv(expr));
#undef expr
#endif
  }


  /// component-wise vector addition
  inline vector_type operator+(const vector_type& _v) const {
#if DIM==N
    return vector_type(*this) += _v;
#else
#define expr(i) Base::values_[i] + _v.Base::values_[i]
    return vector_type(unroll_csv(expr));
#undef expr
#endif
  }


  /// component-wise vector difference
  inline vector_type operator-(const vector_type& _v) const {
#if DIM==N
    return vector_type(*this) -= _v;
#else
#define expr(i) Base::values_[i] - _v.Base::values_[i]
    return vector_type(unroll_csv(expr));
#undef expr
#endif
  }


  /// unary minus
  inline vector_type operator-(void) const {
    vector_type v;
#define expr(i) v.Base::values_[i] = -Base::values_[i];
    unroll(expr);
#undef expr
    return v;
  }


  /// cross product: only defined for Vec3* as specialization
  /// \see OpenMesh::cross
  inline VectorT<Scalar,3> operator%(const VectorT<Scalar,3>& _rhs) const
#if DIM==3
  {
    return
      VectorT<Scalar,3>(Base::values_[1]*_rhs.Base::values_[2]-Base::values_[2]*_rhs.Base::values_[1],
      Base::values_[2]*_rhs.Base::values_[0]-Base::values_[0]*_rhs.Base::values_[2],
      Base::values_[0]*_rhs.Base::values_[1]-Base::values_[1]*_rhs.Base::values_[0]);
  }
#else
  ;
#endif


  /// compute scalar product
  /// \see OpenMesh::dot
  inline Scalar operator|(const vector_type& _rhs) const {
  Scalar p(0);
#define expr(i) p += Base::values_[i] * _rhs.Base::values_[i];
  unroll(expr);
#undef expr
    return p;
  }





  //------------------------------------------------------------ euclidean norm

  /// \name Euclidean norm calculations
  //@{
  /// compute euclidean norm
  inline Scalar norm() const { return (Scalar)sqrt(sqrnorm()); }
  inline Scalar length() const { return norm(); } // OpenSG interface

  /// compute squared euclidean norm
  inline Scalar sqrnorm() const 
  {
#if DIM==N
    Scalar s(0);
#define expr(i) s += Base::values_[i] * Base::values_[i];
    unroll(expr);
#undef expr
    return s;
#else
#define expr(i) Base::values_[i]*Base::values_[i]
    return (unroll_comb(expr, +));
#undef expr
#endif
  }

  /** normalize vector, return normalized vector
  */

  inline vector_type& normalize() 
  {
    *this /= norm();
    return *this;
  }
  
  /** return normalized vector
  */

  inline const vector_type normalized() const
  {
    return *this / norm();
  }

  /** normalize vector, return normalized vector and avoids div by zero 
  */
  inline vector_type& normalize_cond() 
  {
    Scalar n = norm();
    if (n != (Scalar)0.0)
    {
      *this /= n;
    }
    return *this;
  }
  
  //@}

  //------------------------------------------------------------ euclidean norm

  /// \name Non-Euclidean norm calculations
  //@{
  
  /// compute L1 (Manhattan) norm
  inline Scalar l1_norm() const
  {
#if DIM==N
    Scalar s(0);
#define expr(i) s += std::abs(Base::values_[i]);
    unroll(expr);
#undef expr
    return s;
#else
#define expr(i) std::abs(Base::values_[i])
    return (unroll_comb(expr, +));
#undef expr
#endif
  }

  /// compute l8_norm
  inline Scalar l8_norm() const
  {
    return max_abs();
  }

  //@}

  //------------------------------------------------------------ max, min, mean

  /// \name Minimum maximum and mean
  //@{

  /// return the maximal component
  inline Scalar max() const 
  {
    Scalar m(Base::values_[0]);
    for(int i=1; i<DIM; ++i) if(Base::values_[i]>m) m=Base::values_[i];
    return m;
  }

  /// return the maximal absolute component
  inline Scalar max_abs() const
  {
    Scalar m(std::abs(Base::values_[0]));
    for(int i=1; i<DIM; ++i) 
      if(std::abs(Base::values_[i])>m)
        m=std::abs(Base::values_[i]);
    return m;
  }


  /// return the minimal component
  inline Scalar min() const 
  {
    Scalar m(Base::values_[0]);
    for(int i=1; i<DIM; ++i) if(Base::values_[i]<m) m=Base::values_[i];
    return m;
  }

  /// return the minimal absolute component
  inline Scalar min_abs() const 
  {
    Scalar m(std::abs(Base::values_[0]));
    for(int i=1; i<DIM; ++i) 
      if(std::abs(Base::values_[i])<m)
        m=std::abs(Base::values_[i]);
    return m;
  }

  /// return arithmetic mean
  inline Scalar mean() const {
    Scalar m(Base::values_[0]);
    for(int i=1; i<DIM; ++i) m+=Base::values_[i];
    return m/Scalar(DIM);
  }

  /// return absolute arithmetic mean
  inline Scalar mean_abs() const {
    Scalar m(std::abs(Base::values_[0]));
    for(int i=1; i<DIM; ++i) m+=std::abs(Base::values_[i]);
    return m/Scalar(DIM);
  }


  /// minimize values: same as *this = min(*this, _rhs), but faster
  inline vector_type& minimize(const vector_type& _rhs) {
#define expr(i) if (_rhs[i] < Base::values_[i]) Base::values_[i] = _rhs[i];
    unroll(expr);
#undef expr
    return *this;
  }

  /// minimize values and signalize coordinate minimization
  inline bool minimized(const vector_type& _rhs) {
    bool result(false);
#define expr(i) if (_rhs[i] < Base::values_[i]) { Base::values_[i] = _rhs[i]; result = true; }
    unroll(expr);
#undef expr
    return result;
  }

  /// maximize values: same as *this = max(*this, _rhs), but faster
  inline vector_type& maximize(const vector_type& _rhs) {
#define expr(i) if (_rhs[i] > Base::values_[i]) Base::values_[i] = _rhs[i];
    unroll(expr);
#undef expr
    return *this;
  }

  /// maximize values and signalize coordinate maximization
  inline bool maximized(const vector_type& _rhs) {
    bool result(false);
#define expr(i) if (_rhs[i] > Base::values_[i]) { Base::values_[i] =_rhs[i]; result = true; }
    unroll(expr);
#undef expr
    return result;
  }

  /// component-wise min
  inline vector_type min(const vector_type& _rhs) const {
    return vector_type(*this).minimize(_rhs);
  }

  /// component-wise max
  inline vector_type max(const vector_type& _rhs) const {
    return vector_type(*this).maximize(_rhs);
  }

  //@}

  //------------------------------------------------------------ misc functions

  /// component-wise apply function object with Scalar operator()(Scalar).
  template<typename Functor>
  inline vector_type apply(const Functor& _func) const {
    vector_type result;
#define expr(i) result[i] = _func(Base::values_[i]);
    unroll(expr);
#undef expr
    return result;
  }

  /// store the same value in each component (e.g. to clear all entries)
  vector_type& vectorize(const Scalar& _s) {
#define expr(i) Base::values_[i] = _s;
    unroll(expr);
#undef expr
    return *this;
  }


  /// store the same value in each component
  static vector_type vectorized(const Scalar& _s) {
    return vector_type().vectorize(_s);
  }


  /// lexicographical comparison
  bool operator<(const vector_type& _rhs) const {
#define expr(i) if (Base::values_[i] != _rhs.Base::values_[i]) \
                   return (Base::values_[i] < _rhs.Base::values_[i]);
    unroll(expr);
#undef expr
    return false;
   }
};



/// read the space-separated components of a vector from a stream
TEMPLATE_HEADER
inline std::istream&
operator>>(std::istream& is, VectorT<Scalar,DIM>& vec)
{
#define expr(i) is >> vec[i];
  unroll(expr);
#undef expr
  return is;
}


/// output a vector by printing its space-separated compontens
TEMPLATE_HEADER
inline std::ostream&
operator<<(std::ostream& os, const VectorT<Scalar,DIM>& vec)
{
#if DIM==N
  for(int i=0; i<N-1; ++i) os << vec[i] << " ";
  os << vec[N-1];
#else
#define expr(i) vec[i]
  os << unroll_comb(expr, << " " <<);
#undef expr
#endif

  return os;
}


// ----------------------------------------------------------------------------
#endif // included by VectorT.hh
//=============================================================================
