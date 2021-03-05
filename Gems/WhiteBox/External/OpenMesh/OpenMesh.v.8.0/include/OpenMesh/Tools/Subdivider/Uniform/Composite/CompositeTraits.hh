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



/** \file Uniform/Composite/CompositeTraits.hh
    Mesh traits for uniform composite subdivision.
 */

//=============================================================================
//
//  CLASS Traits
//
//=============================================================================

#ifndef OPENMESH_SUBDIVIDER_UNIFORM_COMPOSITETRAITS_HH
#define OPENMESH_SUBDIVIDER_UNIFORM_COMPOSITETRAITS_HH


//== INCLUDES =================================================================

//#include "Config.hh"
// --------------------
#include <OpenMesh/Core/Mesh/Traits.hh>
#include <OpenMesh/Core/Mesh/Attributes.hh>


//== NAMESPACE ================================================================

namespace OpenMesh   { // BEGIN_NS_OPENMESH
namespace Subdivider { // BEGIN_NS_DECIMATER
namespace Uniform    { // BEGIN_NS_UNIFORM


//== CLASS DEFINITION =========================================================
	      

/** Uniform Composite Subdivision framework.
*/

struct CompositeTraits : public OpenMesh::DefaultTraits
{
  FaceAttributes( OpenMesh::Attributes::Normal );
  
  VertexAttributes( OpenMesh::Attributes::Normal );
  
  //HalfedgeAttributes( OpenMesh::Attributes::PrevHalfedge );

  FaceTraits
  {
  private:
    typedef typename Refs::HalfedgeHandle HalfedgeHandle;
    typedef typename Refs::Scalar Scalar;
    typedef typename Refs::Point Point;
    HalfedgeHandle red_halfedge_handle_;
    unsigned int generation_;
    bool red_;
    Scalar quality_;
    Point midpoint_;
    Point position_;
    
  public:
    const unsigned int& generation() { return generation_; }
    void set_generation(const unsigned int& _g) { generation_ = _g; }
    void inc_generation() { ++generation_; }
    void set_red() { red_ = 1; }
    void set_green() {red_ = 0; }
    bool is_red() { return red_; }
    bool is_green() { return !red_; }
    void set_red_halfedge_handle(HalfedgeHandle& _heh) 
      { red_halfedge_handle_ = _heh; }
    HalfedgeHandle& red_halfedge_handle() { return red_halfedge_handle_; }
    void set_quality(Scalar& _q) { quality_ = _q; }
    Scalar& quality() { return quality_; }
    const Point& midpoint() const { return midpoint_; }
    void set_midpoint(const Point& _p) { midpoint_ = _p; }
    const Point& position() const { return position_; }
    void set_position(const Point& _p) { position_ = _p; }
  };

  EdgeTraits
  {
  private:
    typedef typename Refs::Point Point;
    typedef typename Refs::Scalar Scalar;
    Point midpoint_;
    Scalar length_;
    Point position_;
  public: 
    const Point& midpoint() const { return midpoint_; }
    void set_midpoint(const Point& _vh) { midpoint_ = _vh; }
    const Scalar& length() const { return length_; }
    void set_length(const Scalar& _s) { length_ = _s; }
    const Point& position() const { return position_; }
    void set_position(const Point& _p) { position_ = _p; }
  };

  VertexTraits
  {
  private:
    typedef typename Refs::Point Point;
    Point new_pos_;
    Point orig_pos_;
    Point position_;
    unsigned int generation_;
  public:
    const Point& new_pos() const { return new_pos_; }
    void set_new_pos(const Point& _p) { new_pos_ = _p; }
    const unsigned int& generation() const { return generation_; }
    void set_generation(const unsigned int& _i) { generation_ = _i; }
    const Point& orig_pos() const { return orig_pos_; }
    void set_orig_pos(const Point& _p) { orig_pos_ = _p; }
    const Point& position() const { return position_; }
    void set_position(const Point& _p) { position_ = _p; }
  };
};

//=============================================================================
} // END_NS_UNIFORM
} // END_NS_SUBDIVIDER
} // END_NS_OPENMESH
//=============================================================================
#endif // OPENMESH_SUBDIVIDER_UNIFORM_COMPOSITETRAITS_HH defined
//=============================================================================

