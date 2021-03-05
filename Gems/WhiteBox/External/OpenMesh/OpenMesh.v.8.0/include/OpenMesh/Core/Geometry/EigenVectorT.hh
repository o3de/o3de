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

/** This file contains all code required to use Eigen3 vectors as Mesh
 *  vectors
 */
#pragma once

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Geometry>


namespace OpenMesh {
  template <typename _Scalar, int _Rows, int _Cols, int _Options>
  struct vector_traits<Eigen::Matrix<_Scalar, _Rows, _Cols, _Options>> {
      static_assert(_Rows != Eigen::Dynamic && _Cols != Eigen::Dynamic,
                    "Should not use dynamic vectors.");
      static_assert(_Rows == 1 || _Cols == 1, "Should not use matrices.");

      using vector_type = Eigen::Matrix<_Scalar, _Rows, _Cols, _Options>;
      using value_type = _Scalar;
      static const size_t size_ = _Rows * _Cols;
      static size_t size() { return size_; }
};

} // namespace OpenMesh

namespace Eigen {

  template <typename Derived>
  typename Derived::Scalar dot(const MatrixBase<Derived> &x,
                               const MatrixBase<Derived> &y) {
      return x.dot(y);
  }

  template <typename Derived>
  typename MatrixBase< Derived >::PlainObject cross(const MatrixBase<Derived> &x, const MatrixBase<Derived> &y) {
      return x.cross(y);
  }

  template <typename Derived>
  typename Derived::Scalar norm(const MatrixBase<Derived> &x) {
      return x.norm();
  }

  template <typename Derived>
  typename Derived::Scalar sqrnorm(const MatrixBase<Derived> &x) {
      return x.dot(x);
  }

  template <typename Derived>
  MatrixBase<Derived> normalize(MatrixBase<Derived> &x) {
      x /= x.norm();
      return x;
  }

  template <typename Derived>
  MatrixBase<Derived> &vectorize(MatrixBase<Derived> &x,
                                 typename Derived::Scalar const &val) {
      x.fill(val);
      return x;
  }

} // namespace Eigen

