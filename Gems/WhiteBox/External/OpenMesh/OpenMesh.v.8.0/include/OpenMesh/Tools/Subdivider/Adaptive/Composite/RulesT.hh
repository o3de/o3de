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



/** \file RulesT.hh
    
 */


//=============================================================================
//
//  Composite Subdivision and Averaging Rules
//
//=============================================================================

#ifndef OPENMESH_SUBDIVIDER_ADAPTIVE_RULEST_HH
#define OPENMESH_SUBDIVIDER_ADAPTIVE_RULEST_HH


//== INCLUDES =================================================================

#include <OpenMesh/Core/System/config.hh>
#include <OpenMesh/Tools/Subdivider/Adaptive/Composite/RuleInterfaceT.hh>
// -------------------- STL
#include <vector>


#if defined(OM_CC_MIPS) // avoid warnings
#  define MIPS_WARN_WA( Item ) \
  void raise(typename M:: ## Item ## Handle &_h, state_t _target_state ) \
  { Inherited::raise(_h, _target_state); }
#else
#  define MIPS_WARN_WA( Item )
#endif

//== NAMESPACE ================================================================

namespace OpenMesh   { // BEGIN_NS_OPENMESH
namespace Subdivider { // BEGIN_NS_SUBDIVIDER
namespace Adaptive   { // BEGIN_NS_ADAPTIVE


//== CLASS DEFINITION =========================================================

/** Adaptive Composite Subdivision framework.
*/


//=============================================================================
  
/** Topological composite rule Tvv,3 doing a 1-3 split of a face.
 */
template <class M> class Tvv3 : public RuleInterfaceT<M>
{
  COMPOSITE_RULE( Tvv3, M );
private:
  typedef RuleInterfaceT<M>                 Base;
  
public:

  typedef RuleInterfaceT<M> Inherited;

  explicit Tvv3(M& _mesh) : Inherited(_mesh) { Base::set_subdiv_type(3); };

  void raise(typename M::FaceHandle&   _fh, state_t _target_state);
  void raise(typename M::VertexHandle& _vh, state_t _target_state);
  MIPS_WARN_WA(Edge) // avoid warning
};


//=============================================================================


/** Topological composite rule Tvv,4 doing a 1-4 split of a face
 */
template <class M> class Tvv4 : public RuleInterfaceT<M>
{
  COMPOSITE_RULE( Tvv4, M );

private:
  typedef RuleInterfaceT<M>                 Base;
public:
  typedef typename M::HalfedgeHandle HEH;
  typedef typename M::VertexHandle   VH;
   
  typedef RuleInterfaceT<M> Inherited;

  explicit Tvv4(M& _mesh) : Inherited(_mesh) { Base::set_subdiv_type(4); };

  void raise(typename M::FaceHandle&   _fh, state_t _target_state); 
  void raise(typename M::VertexHandle& _vh, state_t _target_state);
  void raise(typename M::EdgeHandle&   _eh, state_t _target_state);

private:

  void split_edge(HEH& _hh, VH& _vh, state_t _target_state);
  void check_edge(const typename M::HalfedgeHandle& _hh, 
                  state_t _target_state);
};


//=============================================================================


/** Composite rule VF
 */
template <class M> class VF : public RuleInterfaceT<M>
{
  COMPOSITE_RULE( VF, M );
private:
  typedef RuleInterfaceT<M>                 Base;

public:
  typedef RuleInterfaceT<M> Inherited;

  explicit VF(M& _mesh) : Inherited(_mesh) {}

  void raise(typename M::FaceHandle& _fh, state_t _target_state);
  MIPS_WARN_WA(Edge)
  MIPS_WARN_WA(Vertex)
};


//=============================================================================


/** Composite rule FF
 */
template <class M> class FF : public RuleInterfaceT<M>
{
  COMPOSITE_RULE( FF, M );
private:
  typedef RuleInterfaceT<M>                 Base;

public:
  typedef RuleInterfaceT<M> Inherited;

  explicit FF(M& _mesh) : Inherited(_mesh) {}

  void raise(typename M::FaceHandle& _fh, state_t _target_state);
  MIPS_WARN_WA(Vertex) // avoid warning
  MIPS_WARN_WA(Edge  ) // avoid warning
};


//=============================================================================


/** Composite rule FFc
 */
template <class M> class FFc : public RuleInterfaceT<M>
{
  COMPOSITE_RULE( FFc, M );
private:
  typedef RuleInterfaceT<M>                 Base;

public:
  typedef RuleInterfaceT<M> Inherited;

  explicit FFc(M& _mesh) : Inherited(_mesh) {}

  void raise(typename M::FaceHandle& _fh, state_t _target_state);
  MIPS_WARN_WA(Vertex) // avoid warning
  MIPS_WARN_WA(Edge  ) // avoid warning
};


//=============================================================================


/** Composite rule FV
 */
template <class M> class FV : public RuleInterfaceT<M>
{
  COMPOSITE_RULE( FV, M );
private:
  typedef RuleInterfaceT<M>                 Base;

public:
  typedef RuleInterfaceT<M> Inherited;

  explicit FV(M& _mesh) : Inherited(_mesh) {}

  void raise(typename M::VertexHandle& _vh, state_t _target_state);
  MIPS_WARN_WA(Face) // avoid warning
  MIPS_WARN_WA(Edge) // avoid warning
};


//=============================================================================


/** Composite rule FVc
 */
template <class M> class FVc : public RuleInterfaceT<M>
{
  COMPOSITE_RULE( FVc, M );
private:
  typedef RuleInterfaceT<M>                 Base;

public:
  typedef RuleInterfaceT<M> Inherited;

  explicit FVc(M& _mesh) : Inherited(_mesh) { init_coeffs(50); }

  void raise(typename M::VertexHandle& _vh, state_t _target_state);
  MIPS_WARN_WA(Face) // avoid warning
  MIPS_WARN_WA(Edge) // avoid warning

  static void init_coeffs(size_t _max_valence);
  static const std::vector<double>& coeffs() { return coeffs_; }

  double coeff( size_t _valence )
  {
    assert(_valence < coeffs_.size());
    return coeffs_[_valence];
  }

private:

  static std::vector<double> coeffs_;

};


//=============================================================================


/** Composite rule VV
 */
template <class M> class VV : public RuleInterfaceT<M>
{
  COMPOSITE_RULE( VV, M );
private:
  typedef RuleInterfaceT<M>                 Base;

public:

  typedef RuleInterfaceT<M> Inherited;

  explicit VV(M& _mesh) : Inherited(_mesh) {}

  void raise(typename M::VertexHandle& _vh, state_t _target_state);
  MIPS_WARN_WA(Face) // avoid warning
  MIPS_WARN_WA(Edge) // avoid warning
};


//=============================================================================


/** Composite rule VVc
 */
template <class M> class VVc : public RuleInterfaceT<M>
{
  COMPOSITE_RULE( VVc, M );
private:
  typedef RuleInterfaceT<M>                 Base;

public:
  typedef RuleInterfaceT<M> Inherited;

  explicit VVc(M& _mesh) : Inherited(_mesh) {}

  void raise(typename M::VertexHandle& _vh, state_t _target_state);
  MIPS_WARN_WA(Face) // avoid warning
  MIPS_WARN_WA(Edge) // avoid warning
};


//=============================================================================


/** Composite rule VE
 */
template <class M> class VE : public RuleInterfaceT<M>
{
  COMPOSITE_RULE( VE, M );
private:
  typedef RuleInterfaceT<M>                 Base;

public:
  typedef RuleInterfaceT<M> Inherited;

  explicit VE(M& _mesh) : Inherited(_mesh) {}

  void raise(typename M::EdgeHandle& _eh, state_t _target_state);
  MIPS_WARN_WA(Face  ) // avoid warning
  MIPS_WARN_WA(Vertex) // avoid warning
};


//=============================================================================


/** Composite rule VdE
 */
template <class M> class VdE : public RuleInterfaceT<M>
{
  COMPOSITE_RULE( VdE, M );
private:
  typedef RuleInterfaceT<M>                 Base;

public:
  typedef RuleInterfaceT<M> Inherited;

  explicit VdE(M& _mesh) : Inherited(_mesh) {}

  void raise(typename M::EdgeHandle& _eh, state_t _target_state);
  MIPS_WARN_WA(Face  ) // avoid warning
  MIPS_WARN_WA(Vertex) // avoid warning
};


//=============================================================================


/** Composite rule VdEc
 */
template <class M> class VdEc : public RuleInterfaceT<M>
{
  COMPOSITE_RULE( VdEc, M );
private:
  typedef RuleInterfaceT<M>                 Base;

public:
  typedef RuleInterfaceT<M> Inherited;

  explicit VdEc(M& _mesh) : Inherited(_mesh) {}

  void raise(typename M::EdgeHandle& _eh, state_t _target_state);
  MIPS_WARN_WA(Face  ) // avoid warning
  MIPS_WARN_WA(Vertex) // avoid warning
};


//=============================================================================


/** Composite rule EV
 */
template <class M> class EV : public RuleInterfaceT<M>
{
  COMPOSITE_RULE( EV, M );
private:
  typedef RuleInterfaceT<M>                 Base;

public:
  typedef RuleInterfaceT<M> Inherited;

  explicit EV(M& _mesh) : Inherited(_mesh) {}

  void raise(typename M::VertexHandle& _vh, state_t _target_state);
  MIPS_WARN_WA(Face) // avoid warning
  MIPS_WARN_WA(Edge) // avoid warning
};


//=============================================================================


/** Composite rule EVc
 */
template <class M> class EVc : public RuleInterfaceT<M>
{
  COMPOSITE_RULE( EVc, M );
private:
  typedef RuleInterfaceT<M>                 Base;

public:

  typedef RuleInterfaceT<M> Inherited;

  explicit EVc(M& _mesh) : Inherited(_mesh) { init_coeffs(50); }

  void raise(typename M::VertexHandle& _vh, state_t _target_state);
  MIPS_WARN_WA(Face) // avoid warning
  MIPS_WARN_WA(Edge) // avoid warning

  static void init_coeffs(size_t _max_valence);
  static const std::vector<double>& coeffs() { return coeffs_; }

  double coeff( size_t _valence )
  {
    assert(_valence < coeffs_.size());
    return coeffs_[_valence];
  }

private:

  static std::vector<double> coeffs_;
  
};


//=============================================================================


/** Composite rule EF
 */
template <class M> class EF : public RuleInterfaceT<M>
{
  COMPOSITE_RULE( EF, M );
private:
  typedef RuleInterfaceT<M>                 Base;

public:
  typedef RuleInterfaceT<M> Inherited;

  explicit EF(M& _mesh) : Inherited(_mesh) {}

  void raise(typename M::FaceHandle& _fh, state_t _target_state);
  MIPS_WARN_WA(Edge  ) // avoid warning
  MIPS_WARN_WA(Vertex) // avoid warning
};


//=============================================================================


/** Composite rule FE
 */
template <class M> class FE : public RuleInterfaceT<M>
{
  COMPOSITE_RULE( FE, M );
private:
  typedef RuleInterfaceT<M>                 Base;

public:
  typedef RuleInterfaceT<M> Inherited;

  explicit FE(M& _mesh) : Inherited(_mesh) {}

  void raise(typename M::EdgeHandle& _eh, state_t _target_state);
  MIPS_WARN_WA(Face  ) // avoid warning
  MIPS_WARN_WA(Vertex) // avoid warning
};


//=============================================================================


/** Composite rule EdE
 */
template <class M> class EdE : public RuleInterfaceT<M>
{
  COMPOSITE_RULE( EdE, M );
private:
  typedef RuleInterfaceT<M>                 Base;

public:
  typedef RuleInterfaceT<M> Inherited;

  explicit EdE(M& _mesh) : Inherited(_mesh) {}

  void raise(typename M::EdgeHandle& _eh, state_t _target_state);
  MIPS_WARN_WA(Face  ) // avoid warning
  MIPS_WARN_WA(Vertex) // avoid warning
};


//=============================================================================


/** Composite rule EdEc
 */
template <class M> class EdEc : public RuleInterfaceT<M>
{
  COMPOSITE_RULE( EdEc, M );
private:
  typedef RuleInterfaceT<M>                 Base;

public:
  typedef RuleInterfaceT<M> Inherited;

  explicit EdEc(M& _mesh) : Inherited(_mesh) {}

  void raise(typename M::EdgeHandle& _eh, state_t _target_state);
  MIPS_WARN_WA(Face  ) // avoid warning
  MIPS_WARN_WA(Vertex) // avoid warning
};

// ----------------------------------------------------------------------------

#undef MIPS_WARN_WA

//=============================================================================
} // END_NS_ADAPTIVE
} // END_NS_SUBDIVIDER
} // END_NS_OPENMESH
//=============================================================================
#if defined(OM_INCLUDE_TEMPLATES) && !defined(OPENMESH_SUBDIVIDER_ADAPTIVE_RULEST_CC)
#  define OPENMESH_SUBDIVIDER_TEMPLATES
#  include "RulesT_impl.hh"
#endif
//=============================================================================
#endif // OPENMESH_SUBDIVIDER_ADAPTIVE_RULEST_HH defined
//=============================================================================

