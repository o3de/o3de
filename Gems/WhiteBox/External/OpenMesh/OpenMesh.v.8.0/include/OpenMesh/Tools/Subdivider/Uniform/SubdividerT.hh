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



/** \file SubdividerT.hh
    
 */

//=============================================================================
//
//  CLASS SubdividerT
//
//=============================================================================

#ifndef OPENMESH_SUBDIVIDER_UNIFORM_SUDIVIDERT_HH
#define OPENMESH_SUBDIVIDER_UNIFORM_SUDIVIDERT_HH

//== INCLUDE ==================================================================

#include <OpenMesh/Core/System/config.hh>
#include <OpenMesh/Core/Utils/Noncopyable.hh>
#if defined(_DEBUG) || defined(DEBUG)
// Makes life lot easier, when playing/messing around with low-level topology
// changing methods of OpenMesh
#  include <OpenMesh/Tools/Utils/MeshCheckerT.hh>
#  define ASSERT_CONSISTENCY( T, m ) \
     assert(OpenMesh::Utils::MeshCheckerT<T>(m).check())
#else
#  define ASSERT_CONSISTENCY( T, m )
#endif

//== NAMESPACE ================================================================

namespace OpenMesh   {
namespace Subdivider {
namespace Uniform    {

//== CLASS DEFINITION =========================================================

/** Abstract base class for uniform subdivision algorithms.
 *
 *  A derived class must overload the following functions:
 *  -# const char* name() const
 *  -# void prepare(MeshType&)
 *  -# void subdivide(MeshType&, size_t, bool)
 *  -# void cleanup(MeshType&)
 */
template <typename MeshType, typename RealType = double>
class SubdividerT : private Utils::Noncopyable
{
public:

  typedef MeshType mesh_t;
  typedef RealType real_t;

public:

  /// \name Constructors
  //@{
  /// Constructor to be used with interface 2
  /// \see attach(), operator()(size_t), detach()
  SubdividerT(void) : attached_() { }

  /// Constructor to be used with interface 1 (calls attach())
  /// \see operator()( MeshType&, size_t )
  explicit SubdividerT( MeshType &_m ) : attached_(NULL) {  attach(_m); }

  //@}

  /// Destructor (calls detach())
  virtual ~SubdividerT() 
  { detach(); }

  /// Return name of subdivision algorithm
  virtual const char *name( void ) const = 0;


public: /// \name Interface 1

  //@{
  /// Subdivide the mesh \c _m \c _n times.
  /// \see SubdividerT(MeshType&)
  bool operator () ( MeshType& _m, size_t _n , const bool _update_points  = true)
  {    
    return prepare(_m) && subdivide( _m, _n , _update_points ) && cleanup( _m );
  }
  //@}

public: /// \name Interface 2
  //@{
  /// Attach mesh \c _m to self
  /// \see SubdividerT(), operator()(size_t), detach()
  bool attach( MeshType& _m )
  {
    if ( attached_ == &_m )
      return true;

    detach();
    if (prepare( _m ))
    {
      attached_ = &_m;
      return true;
    }
    return false;
  }

  /// Subdivide the attached \c _n times.
  /// \see SubdividerT(), attach(), detach()
  bool operator()( size_t _n , const bool _update_points = true)
  {
    return attached_ ? subdivide( *attached_, _n , _update_points) : false;
  }

  /// Detach an eventually attached mesh.
  /// \see SubdividerT(), attach(), operator()(size_t)
  void detach(void)
  {
    if ( attached_ )
    {
      cleanup( *attached_ );
      attached_ = NULL;
    }
  }
  //@}

protected: 

  /// \name Overload theses methods
  //@{
  /// Prepare mesh, e.g. add properties
  virtual bool prepare( MeshType& _m ) = 0;

  /// Subdivide mesh \c _m \c _n times
  virtual bool subdivide( MeshType& _m, size_t _n, const bool _update_points = true) = 0;

  /// Cleanup mesh after usage, e.g. remove added properties
  virtual bool cleanup( MeshType& _m ) = 0;
  //@}

private:
 
  MeshType *attached_;

};

//=============================================================================
} // namespace Uniform
} // namespace Subdivider
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_SUBDIVIDER_UNIFORM_SUBDIVIDERT_HH
//=============================================================================
