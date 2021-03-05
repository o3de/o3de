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



/** \file Adaptive/Composite/CompositeT.hh
    
 */

//=============================================================================
//
//  CLASS CompositeT
//
//=============================================================================

#ifndef OPENMESH_SUBDIVIDER_ADAPTIVE_COMPOSITET_HH
#define OPENMESH_SUBDIVIDER_ADAPTIVE_COMPOSITET_HH


//== INCLUDES =================================================================

#include <OpenMesh/Core/System/config.hh>
#include <OpenMesh/Tools/Subdivider/Adaptive/Composite/CompositeTraits.hh>
// --------------------
#include <vector>
#include <memory>
#include <string>


//== NAMESPACE ================================================================

namespace OpenMesh   { // BEGIN_NS_OPENMESH
namespace Subdivider { // BEGIN_NS_SUBDIVIDER
namespace Adaptive   { // BEGIN_NS_ADAPTIVE


//== CLASS DEFINITION =========================================================


template <typename R> struct RuleHandleT;
template <typename M> class  RuleInterfaceT;


//== CLASS DEFINITION =========================================================

/** Adaptive Composite Subdivision framework.
 *
 *  The adaptive composite subdivision framework is based on the work
 *  done by P. Oswald and P. Schroeder. This framework elevates the
 *  uniform case of the composite scheme to the adaptive
 *  setting.
 *
 *  For details on the composite scheme refer to
 *  - <a
 *  href="http://cm.bell-labs.com/who/poswald/sqrt3.pdf">P. Oswald,
 *  P. Schroeder "Composite primal/dual sqrt(3)-subdivision schemes",
 *  CAGD 20, 3, 2003, 135--164</a>
 *
 *  For details on the transition from uniform to adaptive composite
 *  subdivision please refer to
 *  - <a
 *  href='http://www.eg.org/EG/DL/PE/OPENSG03/04sovakar.pdf'>A. von Studnitz,
 *  A. Sovakar, L. Kobbelt "API Design for Adaptive Subdivision
 *  Schemes" OpenSG Symposium 2003</a>
 *
 *  In the composite scheme a subdivision operator is created by
 *  combining smaller "atomic" rules. Depending on the selection and
 *  ordering of the operator many known subdivision schemes can be
 *  created.
 *
 *  Every rule inherits from RuleInterfaceT and is represented out of
 *  the subdivider object by a RuleHandleT (as usual within
 *  %OpenMesh). You can add rules using the CompositeT::add()
 *  functions. The correct order of adding the rules is very
 *  important, and furthermore not all rules get along with each other
 *  very well. (Please read the given literature, especially the
 *  paper by Oswald and Schr�der.)
 *  
 *  To use a composite subdivider first define a rule sequence
 *  describing the order of execution of the rules. In the order the
 *  rules habe been added they will be executed. E.g. the rules given
 *  in operator notation have to added from right to left.  
 *
 *  After the rule sequence has been defined the subdivider has to be
 *  intialized using CompositeT::initialize(). If everything went well,
 *  use CompositeT::refine() to subdivide locally a face or vertex.
 *
 *  \note Not all (topological) operators have been implemented!
 *  \note Only triangle meshes are supported.
 *  \note The rule sequence must begin with a topological operator.
 *
 *  \see RuleInterfaceT, RuleHandleT
 *
 */ 
template <typename M> class CompositeT
{
public:

  typedef RuleInterfaceT<M>  Rule;
  typedef M                  Mesh;
  typedef std::vector<Rule*> RuleSequence;

  typedef typename M::VertexHandle   VH;
  typedef typename M::FaceHandle     FH;
  typedef typename M::EdgeHandle     EH;
  typedef typename M::HalfedgeHandle HH;

public:

  /// Constructor
  CompositeT(Mesh& _mesh) 
    : subdiv_type_(0), 
      subdiv_rule_(NULL), /*first_rule_(NULL), last_rule_(NULL),*/ mesh_(_mesh)
  { }

  ///
  virtual ~CompositeT() 
  { cleanup(); }


  /// Reset \c self to state after the default constructor except of
  /// the mesh.
  void cleanup(void)
  {
    subdiv_type_ = 0;
    subdiv_rule_ = NULL;

    std::for_each(rule_sequence_.begin(), 
                  rule_sequence_.end(), DeleteRule() );
    rule_sequence_.clear();
  }


  /// Initialize faces, edges, vertices, and rules
  bool initialize(void);

  
  /// Refine one face.
  void refine(typename M::FaceHandle& _fh);


  /// Raise one vertex to next final level.
  void refine(typename M::VertexHandle& _vh);


  /// Return subdivision split type (3 for 1-to-3 split, 4 for 1-to-4 split).
  int subdiv_type() { return subdiv_type_; }


  // Return subdivision rule.
  const Rule& subdiv_rule() const { return *subdiv_rule_; }

public:

  /// \name Managing composite rules
  //*@

  /** Add new rule to rule sequence by passing the type of the wanted
   *  rule as template argument to the method.
   *  \return Valid handle on success. Else it is invalid.
   */
  template < typename R >
  RuleHandleT<R> add()
  {
    size_t idx = rule_sequence_.size();
    rule_sequence_.push_back( new R( mesh_ ) );
    return RuleHandleT<R>( (idx < rule_sequence_.size()) ? idx : -1 );
  }

  /** Add new rule to rule sequence by passing an appropriate handle
   *  to the method.
   *  \return Valid handle on success. Else it is invalid.
   */
  template < typename R >
  RuleHandleT<R>& add( RuleHandleT<R>& _rh )
  {
    return _rh = add< R >();
  }

  /** Get rule in the rule sequence by a handle.  
   *
   * \return The wanted rule if the handle is valid. The return value
   * is undefined if the handle is invalid!
   */
  template < typename R >
  typename RuleHandleT<R>::Rule& rule( const RuleHandleT<R>& _rh )
  {
    typedef typename RuleHandleT<R>::Rule rule_t;
    assert( _rh.is_valid() );
    return *dynamic_cast<rule_t*>(rule_sequence_[ _rh.idx() ]);
  }


  /** Get rule (interface) by index
   *
   * \return The wanted rule if the handle is valid. The return value
   * is undefined if the handle is invalid!
   */
  RuleInterfaceT<M>& rule( size_t _idx )
  {
    assert( _idx < n_rules() );
    return *rule_sequence_[ _idx ];
  }

  /// Number of rules in the rule sequence
  size_t n_rules() const { return rule_sequence_.size(); }

  /// Return the sequence as string
  std::string rules_as_string(const std::string& _sep= " * ") const;

  //@}

protected:

  /// The rule sequence
  const RuleSequence& rules() const { return rule_sequence_; }

protected: // helper

  // get current generation from state
  state_t generation(state_t _s) { return _s-(_s % n_rules()); }
  state_t generation( VH _vh ) { return generation(mesh_.data(_vh).state()); }
  state_t generation( EH _eh ) { return generation(mesh_.data(_eh).state()); }
  state_t generation( FH _fh ) { return generation(mesh_.data(_fh).state()); }

private:

  // short cuts
  Rule* t_rule() { return subdiv_rule_;           }
  Rule* f_rule() { return rule_sequence_.front(); }
  Rule* l_rule() { return rule_sequence_.back();  }

private:

  // 
  RuleSequence rule_sequence_;

  // Split type
  int   subdiv_type_;

  Rule  *subdiv_rule_;
//   Rule  *first_rule_; 
//   Rule  *last_rule_;

  //
  Mesh  &mesh_;

private: // helper

#ifndef DOXY_IGNORE_THIS
  struct DeleteRule { void operator()( Rule* _r ) { delete _r; } };
#endif

private:

  CompositeT( const CompositeT& );
  CompositeT& operator = ( const CompositeT );

};

   
//=============================================================================
} // END_NS_ADAPTIVE
} // END_NS_SUBDIVIDER
} // END_NS_OPENMESH
//=============================================================================
#if defined(OM_INCLUDE_TEMPLATES) && !defined(OPENMESH_SUBDIVIDER_ADAPTIVE_COMPOSITET_CC)
#  define OPENMESH_SUBDIVIDER_TEMPLATES
#  include "CompositeT_impl.hh"
#endif
//=============================================================================
#endif // OPENMESH_SUBDIVIDER_ADAPTIVE_COMPOSITET_HH defined
//=============================================================================
