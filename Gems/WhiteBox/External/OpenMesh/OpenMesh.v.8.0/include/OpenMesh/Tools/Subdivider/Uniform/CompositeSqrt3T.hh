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



/** \file CompositeSqrt3T.hh
    
 */

//=============================================================================
//
//  CLASS SQRT3T
//
//=============================================================================

#ifndef OPENMESH_SUBDIVIDER_UNIFORM_COMPOSITESQRT3T_HH
#define OPENMESH_SUBDIVIDER_UNIFORM_COMPOSITESQRT3T_HH


//== INCLUDES =================================================================

#include "Composite/CompositeT.hh"
#include "Composite/CompositeTraits.hh"


//== NAMESPACE ================================================================

namespace OpenMesh   { // BEGIN_NS_OPENMESH
namespace Subdivider { // BEGIN_NS_DECIMATER
namespace Uniform    { // BEGIN_NS_UNIFORM


//== CLASS DEFINITION =========================================================

/** Uniform composite sqrt(3) subdivision algorithm
 */
template <typename MeshType, typename RealType = double>
class CompositeSqrt3T : public CompositeT<MeshType, RealType>
{
public:

   typedef CompositeT<MeshType, RealType> Inherited;

public:

  CompositeSqrt3T() : Inherited() {};
  CompositeSqrt3T(MeshType& _mesh) : Inherited(_mesh) {};
  ~CompositeSqrt3T() {}

public:
   
  const char *name() const { return "Uniform Composite Sqrt3"; }

protected: // inherited interface

  void apply_rules(void)  
  {
    Inherited::Tvv3(); 
    Inherited::VF(); 
    Inherited::FF(); 
    Inherited::FVc(coeffs_); 
  }

protected:
   
  typedef typename Inherited::Coeff Coeff;

  /** Helper class
   *  \internal
   */
  struct FVCoeff : public Coeff 
  {
    FVCoeff() : Coeff() { init(50); }

    void init(size_t _max_valence)
    {
      weights_.resize(_max_valence);
      std::generate(weights_.begin(), 
                    weights_.end(), compute_weight() );
    }
    
    double operator()(size_t _valence) { return weights_[_valence]; }

  /** \internal
   */
    struct compute_weight 
    {
      compute_weight() : val_(0) { }

      double operator()(void) // sqrt(3) weights for non-boundary vertices
      { 
        return 2.0/3.0 * (cos(2.0*M_PI/val_++)+1.0); 
      }
      size_t val_;
    };

    std::vector<double> weights_;
    
  } coeffs_;

};


//=============================================================================
} // END_NS_UNIFORM
} // END_NS_SUBDIVIDER
} // END_NS_OPENMESH
//=============================================================================
#endif // OPENMESH_SUBDIVIDER_UNIFORM_COMPOSITESQRT3T_HH defined
//=============================================================================
