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



/** \file ModProgMeshT.hh

 */

//=============================================================================
//
//  CLASS ModProgMeshT
//
//=============================================================================

#ifndef OPENMESH_TOOLS_MODPROGMESHT_HH
#define OPENMESH_TOOLS_MODPROGMESHT_HH


//== INCLUDES =================================================================

#include <OpenMesh/Tools/Decimater/ModBaseT.hh>
#include <OpenMesh/Core/Utils/Property.hh>


//== NAMESPACE ================================================================

namespace OpenMesh  {
namespace Decimater {


//== CLASS DEFINITION =========================================================


/** Collect progressive mesh information while decimating.
 *
 *  The progressive mesh data is stored in an internal structure, which
 *  can be evaluated after the decimation process and (!) before calling
 *  the garbage collection of the decimated mesh.
 */
template <class MeshT>
class ModProgMeshT : public ModBaseT<MeshT>
{
public:

  DECIMATING_MODULE( ModProgMeshT, MeshT, ProgMesh );

  /** Struct storing progressive mesh information
   *  \see CollapseInfoT, ModProgMeshT
   */
  struct Info
  {
    /// Initializing constructor copies appropriate handles from
    /// collapse information \c _ci.
    Info( const CollapseInfo& _ci )
      : v0(_ci.v0), v1(_ci.v1), vl(_ci.vl),vr(_ci.vr)
    {}

    typename Mesh::VertexHandle v0; ///< See CollapseInfoT::v0
    typename Mesh::VertexHandle v1; ///< See CollapseInfoT::v1
    typename Mesh::VertexHandle vl; ///< See CollapseInfoT::vl
    typename Mesh::VertexHandle vr; ///< See CollapseInfoT::vr

  };

  /// Type of the list storing the progressive mesh info Info.
  typedef std::vector<Info>           InfoList;


public:

   /// Constructor
  ModProgMeshT( MeshT &_mesh ) : Base(_mesh, true)
  {
    Base::mesh().add_property( idx_ );
  }


  /// Destructor
  ~ModProgMeshT()
  {
    Base::mesh().remove_property( idx_ );
  }

  const InfoList&                            pmi() const
  {
    return pmi_;
  }

public: // inherited


  /// Stores collapse information in a queue.
  /// \see infolist()
  void postprocess_collapse(const CollapseInfo& _ci)
  {
    pmi_.push_back( Info( _ci ) );
  }


  bool is_binary(void) const { return true; }


public: // specific methods

  /** Write progressive mesh data to a file in proprietary binary format .pm.
   *
   *  The methods uses the collected data to write a progressive mesh
   *  file. It's a binary format with little endian byte ordering:
   *
   *  - The first 8 bytes contain the word "ProgMesh".
   *  - 32-bit int for the number of vertices \c NV in the base mesh.
   *  - 32-bit int for the number of faces in the base mesh.
   *  - 32-bit int for the number of halfedge collapses (now vertex splits)
   *  - Positions of vertices of the base mesh (32-bit float triplets).<br>
   *    \c [x,y,z][x,y,z]...
   *  - Triplets of indices (32-bit int) for each triangle (index in the
   *    list of vertices of the base mesh defined by the positions.<br>
   *    \c [v0,v1,v2][v0,v1,v2]...
   *  - For each collapse/split a detail information package made of
   *    3 32-bit floats for the positions of vertex \c v0, and 3 32-bit
   *    int indices for \c v1, \c vl, and \c vr.
   *    The index for \c vl or \c vr might be -1, if the face on this side
   *    of the edge does not exists.
   *
   *  \remark Write file before calling the garbage collection of the mesh.
   *  \param _ofname Name of the file, where to write the progressive mesh
   *  \return \c true on success of the operation, else \c false.
   */
  bool write( const std::string& _ofname );
  /// Reference to collected information
  const InfoList& infolist() const { return pmi_; }

private:

  // hide this method form user
  void set_binary(bool _b) {}

  InfoList          pmi_;
  VPropHandleT<size_t> idx_;
};


//=============================================================================
} // END_NS_DECIMATER
} // END_NS_OPENMESH
//=============================================================================
#if defined(OM_INCLUDE_TEMPLATES) && !defined(OPENMESH_DECIMATER_MODPROGMESH_CC)
#define OSG_MODPROGMESH_TEMPLATES
#include "ModProgMeshT_impl.hh"
#endif
//=============================================================================
#endif // OPENMESH_TOOLS_PROGMESHT_HH defined
//=============================================================================

