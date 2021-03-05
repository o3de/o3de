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
//  CLASS RuleInterfaceT
//
//=============================================================================

#ifndef OPENMESH_SUBDIVIDER_ADAPTIVE_RULEINTERFACET_HH
#define OPENMESH_SUBDIVIDER_ADAPTIVE_RULEINTERFACET_HH


//== INCLUDES =================================================================

#include <string>
#include <OpenMesh/Tools/Subdivider/Adaptive/Composite/CompositeTraits.hh>

//== NAMESPACE ================================================================

namespace OpenMesh   { // BEGIN_NS_OPENMESH
namespace Subdivider { // BEGIN_NS_SUBDIVIDER
namespace Adaptive   { // BEGIN_NS_ADAPTIVE


//== FORWARDS =================================================================

template <typename M> class CompositeT;
template <typename M> class RuleInterfaceT;

//== CLASS DEFINITION =========================================================


// ----------------------------------------------------------------------------

/** Handle template for adaptive composite subdividion rules
 *  \internal
 * 
 *  Use typed handle of a rule, e.g. Tvv3<MyMesh>::Handle.
 */
template < typename R > 
struct RuleHandleT : public BaseHandle
{
  explicit RuleHandleT(int _idx=-1) : BaseHandle(_idx) {}
  typedef R Rule;

  operator bool() const { return is_valid(); }

};

/** Defines the method type() (RuleInterfaceT::type()) and the
 *  typedefs Self and Handle.
 */
#define COMPOSITE_RULE( classname, mesh_type ) \
  protected:\
    friend class CompositeT<mesh_type>; \
  public: \
    const char *type() const { return #classname; } \
    typedef classname<mesh_type>     Self;          \
    typedef RuleHandleT< Self >      Handle


// ----------------------------------------------------------------------------
/** Base class for adaptive composite subdivision rules
 *  \see class CompositeT
 */
template <typename M> class RuleInterfaceT
{
public:

  typedef M                   Mesh;
  typedef RuleInterfaceT<M>   Self;
  typedef RuleHandleT< Self > Rule;

  typedef typename M::Scalar  scalar_t;

protected:

  /// Default constructor
  RuleInterfaceT(Mesh& _mesh) : mesh_(_mesh) {};

public:

  /// Destructor
  virtual ~RuleInterfaceT() {};


  /// Returns the name of the rule.
  /// Use define COMPOSITE_RULE to overload this function in a derived class.
  virtual const char *type() const = 0;

public:

  /// \name Raise item
  //@{
  /// Raise item to target state \c _target_state.
  virtual void raise(typename M::FaceHandle& _fh, state_t _target_state) 
  {
    if (mesh_.data(_fh).state() < _target_state) {
      update(_fh, _target_state);
      mesh_.data(_fh).inc_state();
    }
  }

  virtual void raise(typename M::EdgeHandle& _eh, state_t _target_state) 
  {
    if (mesh_.data(_eh).state() < _target_state) {
      update(_eh, _target_state);
      mesh_.data(_eh).inc_state();
    }
  }

  virtual void raise(typename M::VertexHandle& _vh, state_t _target_state) 
  {
    if (mesh_.data(_vh).state() < _target_state) {
      update(_vh, _target_state);
      mesh_.data(_vh).inc_state();
    }
  }
  //@}

  void update(typename M::FaceHandle& _fh, state_t _target_state)
  {
    typename M::FaceHandle opp_fh;

    while (mesh_.data(_fh).state() < _target_state - 1) {
      prev_rule()->raise(_fh, _target_state - 1);
    }

    // Don't use unflipped / unfinal faces!!!
    if (subdiv_type() == 3) {

      if (mesh_.face_handle(mesh_.opposite_halfedge_handle(mesh_.halfedge_handle(_fh))).is_valid()) {

	while (!mesh_.data(_fh).final()) {

	  opp_fh = mesh_.face_handle(mesh_.opposite_halfedge_handle(mesh_.halfedge_handle(_fh)));

	  assert (mesh_.data(_fh).state() >=
		  mesh_.data(opp_fh).state());

	  // different states: raise other face
	  if (mesh_.data(_fh).state() > mesh_.data(opp_fh).state()){

	    // raise opposite face
	    prev_rule()->raise(opp_fh, _target_state - 1);
	  }

	  else {

	    // equal states

	    // flip edge
	    // 	    typename M::EdgeHandle eh(mesh_.edge_handle(mesh_.halfedge_handle(_fh)));

	    // 	    if (mesh_.is_flip_ok(eh)) {

	    // 	      std::cout << "Flipping Edge...\n";

	    // 	      mesh_.flip(eh);

	    // 	      mesh_.data(_fh).set_final();
	    // 	      mesh_.data(opp_fh).set_final();
	    // 	    }

	    // 	    else {

	    // 	      std::cout << "Flip not okay.\n";
	    // 	    }
	  }
	}
      }

      else {

	// 	mesh_.data(_fh).set_final();
      }

      //     std::cout << "Raising Face   to Level " 
      // 	      << _target_state
      // 	      << " with "
      // 	      << type()
      // 	      << ".\n";

    }
    
    assert( subdiv_type() != 4       || 
            mesh_.data(_fh).final() ||
            _target_state%n_rules() == (subdiv_rule()->number() + 1)%n_rules() );

    typename M::FaceEdgeIter   fe_it;
    typename M::FaceVertexIter fv_it;
    typename M::EdgeHandle     eh;
    typename M::VertexHandle   vh;

    std::vector<typename M::FaceHandle> face_vector;
    face_vector.clear();

    if (_target_state > 1) {

      for (fe_it = mesh_.fe_iter(_fh); fe_it.is_valid(); ++fe_it) {

        eh = *fe_it;
        prev_rule()->raise(eh, _target_state - 1);
      }

      for (fv_it = mesh_.fv_iter(_fh); fv_it.is_valid(); ++fv_it) {

        vh = *fv_it;
        prev_rule()->raise(vh, _target_state - 1);
      }
    }
  }
  

  void update(typename M::EdgeHandle& _eh, state_t _target_state) 
  {
    state_t state(mesh_.data(_eh).state());

    // raise edge to correct state
    if (state + 1 < _target_state && _target_state > 0) {

      prev_rule()->raise(_eh, _target_state - 1);
    }

    typename M::VertexHandle vh;
    typename M::FaceHandle   fh;

    if (_target_state > 1)
    {
      vh = mesh_.to_vertex_handle(mesh_.halfedge_handle(_eh, 0));
      prev_rule()->raise(vh, _target_state - 1);

      vh = mesh_.to_vertex_handle(mesh_.halfedge_handle(_eh, 1));
      prev_rule()->raise(vh, _target_state - 1);

      fh = mesh_.face_handle(mesh_.halfedge_handle(_eh, 0));
      if (fh.is_valid())
	prev_rule()->raise(fh, _target_state - 1);

      fh = mesh_.face_handle(mesh_.halfedge_handle(_eh, 1));
      if (fh.is_valid())
	prev_rule()->raise(fh, _target_state - 1);
    }
  }


  void update(typename M::VertexHandle& _vh, state_t _target_state) {

    state_t state(mesh_.data(_vh).state());

    // raise vertex to correct state
    if (state + 1 < _target_state)
    {
      prev_rule()->raise(_vh, _target_state - 1);
    }

    std::vector<typename M::HalfedgeHandle> halfedge_vector;
    halfedge_vector.clear();

    typename M::VertexOHalfedgeIter voh_it;
    typename M::EdgeHandle eh;
    typename M::FaceHandle fh;

    if (_target_state > 1)
    {

      for (voh_it = mesh_.voh_iter(_vh); voh_it.is_valid(); ++voh_it) {
        halfedge_vector.push_back(*voh_it);
      }

      while ( !halfedge_vector.empty() ) {
        eh = mesh_.edge_handle(halfedge_vector.back());
        halfedge_vector.pop_back();

        prev_rule()->raise(eh, _target_state - 1);
      }

      for (voh_it = mesh_.voh_iter(_vh); voh_it.is_valid(); ++voh_it) {
        halfedge_vector.push_back(*voh_it);
      }

      while ( !halfedge_vector.empty() ) {
	fh = mesh_.face_handle(halfedge_vector.back());
	halfedge_vector.pop_back();

	if (fh.is_valid())
	  prev_rule()->raise(fh, _target_state - 1);
      }
    }
  }

public:


  /// Type of split operation, if it is a topological operator
  int  subdiv_type() const { return subdiv_type_; }


  /// Position in rule sequence
  int  number() const      { return number_;      }

  /// \name Parameterization of rule
  //@{

  /// Set coefficient - ignored by non-parameterized rules.
  virtual void set_coeff( scalar_t _coeff ) { coeff_ = _coeff; }

  /// Get coefficient - ignored by non-parameterized rules.
  scalar_t coeff() const { return coeff_; }

  //@}

protected:

  void  set_prev_rule(Self*& _p) { prev_rule_ = _p; }
  Self* prev_rule() { return prev_rule_; }

  void  set_subdiv_rule(Self*& _n) { subdiv_rule_ = _n; }
  Self* subdiv_rule() { return subdiv_rule_; }

  void set_number(int _n) { number_ = _n; }

  void set_n_rules(int _n) { n_rules_ = _n; }
  int  n_rules() { return n_rules_; }

  void set_subdiv_type(int _n) 
  { assert(_n == 3 || _n == 4); subdiv_type_ = _n; }

  friend class CompositeT<M>;

protected:

  Mesh& mesh_;

private:

  Self* prev_rule_;
  Self* subdiv_rule_;
  
  int   subdiv_type_;
  int   number_;
  int   n_rules_;

  scalar_t coeff_;

private: // Noncopyable

  RuleInterfaceT(const RuleInterfaceT&);
  RuleInterfaceT& operator=(const RuleInterfaceT&);

};

//=============================================================================
} // END_NS_ADAPTIVE
} // END_NS_SUBDIVIDER
} // END_NS_OPENMESH
//=============================================================================
#endif // OPENMESH_SUBDIVIDER_ADAPTIVE_RULEINTERFACET_HH defined
//=============================================================================

