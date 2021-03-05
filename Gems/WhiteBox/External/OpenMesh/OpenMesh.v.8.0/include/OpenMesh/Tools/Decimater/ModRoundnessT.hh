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



/** \file ModRoundnessT.hh

 */

//=============================================================================
//
//  CLASS ModRoundnessT
//
//=============================================================================

#ifndef OPENMESH_DECIMATER_MODROUNDNESST_HH
#define OPENMESH_DECIMATER_MODROUNDNESST_HH


//== INCLUDES =================================================================

#include <OpenMesh/Tools/Decimater/ModBaseT.hh>
#include <math.h>

#if defined(OM_CC_MSVC)
#  define OM_ENABLE_WARNINGS 4244
#  pragma warning(disable : OM_ENABLE_WARNINGS )
#endif

//== NAMESPACE ================================================================

namespace OpenMesh { // BEGIN_NS_OPENMESH
namespace Decimater { // BEGIN_NS_DECIMATER


//== CLASS DEFINITION =========================================================


/** \brief Use Roundness of triangles to control decimation
  *
  *
  * In binary and mode, the collapse is legal if:
  *  - The roundness after the collapse is greater than the given value
  *
  * In continuous mode the roundness after the collapse is returned
  */
template <class MeshT>
class ModRoundnessT : public ModBaseT<MeshT>
{
  public:
  DECIMATING_MODULE( ModRoundnessT, MeshT, Roundness );

  public:

  // typedefs
  typedef typename MeshT::Point                      Point;
  typedef typename vector_traits<Point>::value_type value_type;

  public:

  /// Constructor
  ModRoundnessT( MeshT &_dec ) :
    Base(_dec, false),
    min_r_(-1.0)
  { }

  /// Destructor
  ~ModRoundnessT() { }

  public: // inherited

  /** Compute collapse priority due to roundness of triangle.
   *
   *  The roundness is computed by dividing the radius of the
   *  circumference by the length of the shortest edge. The result is
   *  normalized.
   *
   * \return [0:1] or ILLEGAL_COLLAPSE in non-binary mode
   * \return LEGAL_COLLAPSE or ILLEGAL_COLLAPSE in binary mode
   * \see set_min_roundness()
   */
  float collapse_priority(const CollapseInfo& _ci)
  {
    //     using namespace OpenMesh;

    typename Mesh::ConstVertexOHalfedgeIter voh_it(Base::mesh(), _ci.v0);
    double                                  r;
    double                                  priority = 0.0; //==LEGAL_COLLAPSE
    typename Mesh::FaceHandle               fhC, fhB;
    Vec3f                                   B,C;

    if ( min_r_ < 0.0f ) // continues mode
    {
      C   = vector_cast<Vec3f>(Base::mesh().point( Base::mesh().to_vertex_handle(*voh_it)));
      fhC = Base::mesh().face_handle( *voh_it );

      for (++voh_it; voh_it.is_valid(); ++voh_it)
      {
        B   = C;
        fhB = fhC;
        C   = vector_cast<Vec3f>(Base::mesh().point(Base::mesh().to_vertex_handle(*voh_it)));
        fhC = Base::mesh().face_handle( *voh_it );

        if ( fhB == _ci.fl || fhB == _ci.fr )
          continue;

        // simulate collapse using position of v1
        r = roundness( vector_cast<Vec3f>(_ci.p1), B, C );

        // return the maximum non-roundness
        priority = std::max( priority, (1.0-r) );

      }
    }
    else // binary mode
    {
      C   = vector_cast<Vec3f>(Base::mesh().point( Base::mesh().to_vertex_handle(*voh_it)));
      fhC = Base::mesh().face_handle( *voh_it );

      for (++voh_it; voh_it.is_valid() && (priority==Base::LEGAL_COLLAPSE); ++voh_it)
      {
        B   = C;
        fhB = fhC;
        C   = vector_cast<Vec3f>(Base::mesh().point(Base::mesh().to_vertex_handle(*voh_it)));
        fhC = Base::mesh().face_handle( *voh_it );

        if ( fhB == _ci.fl || fhB == _ci.fr )
          continue;

        priority = ( (r=roundness( vector_cast<Vec3f>(_ci.p1), B, C )) < min_r_)
	          ? Base::ILLEGAL_COLLAPSE : Base::LEGAL_COLLAPSE;
      }
    }

    return (float) priority;
  }

  /// set the percentage of minimum roundness
  void set_error_tolerance_factor(double _factor) {
    if (this->is_binary()) {
      if (_factor >= 0.0 && _factor <= 1.0) {
        // the smaller the factor, the smaller min_r_ gets
        // thus creating a stricter constraint
        // division by error_tolerance_factor_ is for normalization
        value_type min_roundness = min_r_ * static_cast<value_type>(_factor / this->error_tolerance_factor_);
        set_min_roundness(min_roundness);
        this->error_tolerance_factor_ = _factor;
      }
}
  }


public: // specific methods

  void set_min_angle( float _angle, bool /* _binary=true */ )
  {
    assert( _angle > 0 && _angle < 60 );

    _angle = float(M_PI * _angle /180.0);

    Vec3f A,B,C;

    A = Vec3f(               0.0f, 0.0f,           0.0f);
    B = Vec3f( 2.0f * cos(_angle), 0.0f,           0.0f);
    C = Vec3f(        cos(_angle), sin(_angle),    0.0f);

    double r1 = roundness(A,B,C);

    _angle = float(0.5 * ( M_PI - _angle ));

    A = Vec3f(             0.0f, 0.0f,           0.0f);
    B = Vec3f( 2.0f*cos(_angle), 0.0f,           0.0f);
    C = Vec3f(      cos(_angle), sin(_angle),    0.0f);

    double r2 = roundness(A,B,C);

    set_min_roundness( value_type(std::min(r1,r2)), true );
  }

  /** Set a minimum roundness value.
   *  \param _min_roundness in range (0,1)
   *  \param _binary Set true, if the binary mode should be enabled,
   *                 else false. In latter case the collapse_priority()
   *                 returns a float value if the constraint does not apply
   *                 and ILLEGAL_COLLAPSE else.
   */
  void set_min_roundness( value_type _min_roundness, bool _binary=true )
  {
    assert( 0.0 <= _min_roundness && _min_roundness <= 1.0 );
    min_r_  = _min_roundness;
    Base::set_binary(_binary);
  }

  /// Unset minimum value constraint and enable non-binary mode.
  void unset_min_roundness()
  {
    min_r_  = -1.0;
    Base::set_binary(false);
  }

  // Compute a normalized roundness of a triangle ABC
  //
  // Having
  //   A,B,C corner points of triangle
  //   a,b,c the vectors BC,CA,AB
  //   Area  area of triangle
  //
  // then define
  //
  //      radius of circumference
  // R := -----------------------
  //      length of shortest edge
  //
  //       ||a|| * ||b|| * ||c||
  //       ---------------------
  //             4 * Area                 ||a|| * ||b|| * ||c||
  //    = ----------------------- = -----------------------------------
  //      min( ||a||,||b||,||c||)   4 * Area * min( ||a||,||b||,||c|| )
  //
  //                      ||a|| * ||b|| * ||c||
  //    = -------------------------------------------------------
  //      4 *  1/2 * ||cross(B-A,C-A)||  * min( ||a||,||b||,||c|| )
  //
  //                         a'a * b'b * c'c
  // R� = ----------------------------------------------------------
  //       4 * cross(B-A,C-A)'cross(B-A,C-A) * min( a'a, b'b, c'c )
  //
  //                      a'a * b'b * c'c
  // R = 1/2 * sqrt(---------------------------)
  //                 AA * min( a'a, b'b, c'c )
  //
  // At angle 60� R has it's minimum for all edge lengths = sqrt(1/3)
  //
  // Define normalized roundness
  //
  // nR := sqrt(1/3) / R
  //
  //                         AA * min( a'a, b'b, c'c )
  //     = sqrt(4/3) * sqrt(---------------------------)
  //                              a'a * b'b * c'c
  //
  double roundness( const Vec3f& A, const Vec3f& B, const Vec3f &C )
  {
    const value_type epsilon = value_type(1e-15);

    static const value_type sqrt43 = value_type(sqrt(4.0/3.0)); // 60�,a=b=c, **)

    Vec3f vecAC     = C-A;
    Vec3f vecAB     = B-A;

    // compute squared values to avoid sqrt-computations
    value_type aa = (B-C).sqrnorm();
    value_type bb = vecAC.sqrnorm();
    value_type cc = vecAB.sqrnorm();
    value_type AA = cross(vecAC,vecAB).sqrnorm(); // without factor 1/4   **)

    if ( AA < epsilon )
      return 0.0;

    double nom   = AA * std::min( std::min(aa,bb),cc );
    double denom = aa * bb * cc;
    double nR    = sqrt43 * sqrt(nom/denom);

    return nR;
  }

  private:

  value_type min_r_;
};


//=============================================================================
} // END_NS_DECIMATER
} // END_NS_OPENMESH
//=============================================================================
#if defined(OM_CC_MSVC) && defined(OM_ENABLE_WARNINGS)
#  pragma warning(default : OM_ENABLE_WARNINGS)
#  undef OM_ENABLE_WARNINGS
#endif
//=============================================================================
#endif // OPENMESH_DECIMATER_MODROUNDNESST_HH defined
//=============================================================================

