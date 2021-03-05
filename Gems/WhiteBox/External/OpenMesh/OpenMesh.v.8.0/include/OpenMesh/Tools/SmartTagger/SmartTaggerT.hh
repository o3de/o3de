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

#pragma once


//== INCLUDES =================================================================

// OpenMesh
#include <OpenMesh/Core/Utils/Property.hh>
#include <OpenMesh/Core/System/omstream.hh>


//== DEFINES ==================================================================

#define STV_DEBUG_CHECKS

//== NAMESPACES ===============================================================

namespace OpenMesh {

//== FORWARD DECLARATIONS =====================================================

// Smarttagging for vertices
template< class Mesh> class SmartTaggerVT;
// Smarttagging for edges
template< class Mesh> class SmartTaggerET;
// Smarttagging for faces
template< class Mesh> class SmartTaggerFT;
// Smarttagging for halfedges
template< class Mesh> class SmartTaggerHT;


//== CLASS DEFINITION =========================================================


/** \page smarttagger_docu Smart Tagger

The smart tagger can be used to tag vertices/halfedges/edges/faces on the mesh. It provides
an O(1) reset function to untag all primitives at once.

Usage:

\code
SmartTaggerVT< MeshType > tagger(mesh);

// Reset tagged flag on all vertices
tagger.untag_all();

// Check if something is tagged
bool tag = tagger.is_tagged(vh);

// Set tagged:
tagger.set_tag(vh);
\endcode

For details see OpenMesh::SmartTaggerT

*/


/** \brief Smart Tagger
 *
 * A tagger class to be used on OpenMesh. It provides an O(1) reset function for the property.
 * - Smarttagging for vertices:  SmartTaggerVT;
 * - Smarttagging for edges:     SmartTaggerET;
 * - Smarttagging for faces:     SmartTaggerFT;
 * - Smarttagging for halfedges: SmartTaggerHT;
 *
 * Usage:
 *
 * \code
 * SmartTaggerVT< MeshType >* tagger = new SmartTaggerVT< MeshType > (mesh_);
 *
 * // Reset tagged flag on all vertices
 * tagger.untag_all();
 *
 * // Check if something is tagged
 * bool tag = tagger.is_tagged(vh);
 *
 * // Set tagged:
 * tagger.set_tag(vh);
 * \endcode
 */
template <class Mesh, class EHandle, class EPHandle>
class SmartTaggerT
{
public:
   
  /// Constructor
  SmartTaggerT(Mesh& _mesh, unsigned int _tag_range = 1);
 
  /// Destructor
  ~SmartTaggerT();

  /** \brief untag all elements
   *
   */
  inline void untag_all();

  /** \brief untag all elements and set new tag_range
   *
   * @param _new_tag_range  New tag range of the tagger
   */
  inline void untag_all( const unsigned int _new_tag_range);
  
  /** \brief set tag to a value in [0..tag_range]
   *
   * @param _eh  Edge handle for the tag
   * @param _tag Tag value
   */
  inline void set_tag  ( const EHandle _eh, unsigned int _tag = 1);

  /** \brief get tag value in range [0..tag_range]
   *
   * @param _eh  Edge handle for the tag
   * @return Current tag value at that edge
   */
  inline unsigned int get_tag  ( const EHandle _eh) const;

  /** \brief  overloaded member for boolean tags
   *
   * @param _eh Edge handle for the tag
   * @return Current tag value at that edge
   */
  inline bool is_tagged( const EHandle _eh) const;

  /** \brief set new tag range and untag_all
   *
   * Set new tag range and reset tagger
   *
   * @param _tag_range  New tag range of the tagger
   */
  inline void set_tag_range( const unsigned int _tag_range);
  
protected:

  inline void all_tags_to_zero();

protected:

  // Reference to Mesh
  Mesh& mesh_;

  // property which holds the current tags
  EPHandle ep_tag_;

  // current tags range is [current_base_+1...current_base_+tag_range_]
  unsigned int current_base_;

  // number of different tagvalues available
  unsigned int tag_range_;
};


//== SPECIALIZATION ===========================================================

// define standard Tagger
template< class Mesh>
class SmartTaggerVT
    : public SmartTaggerT< Mesh, typename Mesh::VertexHandle, OpenMesh::VPropHandleT<unsigned int> > 
{
public:
  typedef SmartTaggerT< Mesh, typename Mesh::VertexHandle, OpenMesh::VPropHandleT<unsigned int> > BaseType;
  SmartTaggerVT(Mesh& _mesh, unsigned int _tag_range = 1) : BaseType(_mesh, _tag_range) {}
};

template< class Mesh>
class SmartTaggerET
    : public SmartTaggerT< Mesh, typename Mesh::EdgeHandle, OpenMesh::EPropHandleT<unsigned int> > 
{
public:
  typedef SmartTaggerT< Mesh, typename Mesh::EdgeHandle, OpenMesh::EPropHandleT<unsigned int> > BaseType;
  SmartTaggerET(Mesh& _mesh, unsigned int _tag_range = 1) : BaseType(_mesh, _tag_range) {}
};

template< class Mesh>
class SmartTaggerFT
    : public SmartTaggerT< Mesh, typename Mesh::FaceHandle, OpenMesh::FPropHandleT<unsigned int> > 
{
public:
  typedef SmartTaggerT< Mesh, typename Mesh::FaceHandle, OpenMesh::FPropHandleT<unsigned int> > BaseType;
  SmartTaggerFT(Mesh& _mesh, unsigned int _tag_range = 1): BaseType(_mesh, _tag_range) {}
};

template< class Mesh>
class SmartTaggerHT
    : public SmartTaggerT< Mesh, typename Mesh::HalfedgeHandle, OpenMesh::HPropHandleT<unsigned int> >
{
public:
  typedef SmartTaggerT< Mesh, typename Mesh::HalfedgeHandle, OpenMesh::HPropHandleT<unsigned int> > BaseType;
  SmartTaggerHT(Mesh& _mesh, unsigned int _tag_range = 1): BaseType(_mesh, _tag_range){}
};


//=============================================================================
} // namespace OpenMesh
//=============================================================================
#if defined(OM_INCLUDE_TEMPLATES) && !defined(OPENMESH_SMARTTAGGERT_C)
#define OPENMESH_SMARTTAGGERT_TEMPLATES
#include "SmartTaggerT_impl.hh"
#endif

