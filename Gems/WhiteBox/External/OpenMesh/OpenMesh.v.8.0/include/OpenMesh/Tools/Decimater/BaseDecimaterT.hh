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



/** \file BaseDecimaterT.hh
 */

//=============================================================================
//
//  CLASS McDecimaterT
//
//=============================================================================

#ifndef OPENMESH_BASE_DECIMATER_DECIMATERT_HH
#define OPENMESH_BASE_DECIMATER_DECIMATERT_HH


//== INCLUDES =================================================================

#include <memory>

#include <OpenMesh/Core/Utils/Property.hh>
#include <OpenMesh/Tools/Decimater/ModBaseT.hh>
#include <OpenMesh/Core/Utils/Noncopyable.hh>
#include <OpenMesh/Tools/Decimater/Observer.hh>



//== NAMESPACE ================================================================

namespace OpenMesh  {
namespace Decimater {


//== CLASS DEFINITION =========================================================


/** base class decimater framework
    \see BaseDecimaterT, \ref decimater_docu
*/
class BaseDecimaterModule
{
};

template < typename MeshT >
class BaseDecimaterT : private Utils::Noncopyable
{
public: //-------------------------------------------------------- public types

  typedef BaseDecimaterT< MeshT >       Self;
  typedef MeshT                         Mesh;
  typedef CollapseInfoT<MeshT>          CollapseInfo;
  typedef ModBaseT<MeshT>                Module;
  typedef std::vector< Module* >        ModuleList;
  typedef typename ModuleList::iterator ModuleListIterator;

public: //------------------------------------------------------ public methods
  BaseDecimaterT(Mesh& _mesh);
  virtual ~BaseDecimaterT();

  /** Initialize decimater and decimating modules.

      Return values:
      true   ok
      false  No ore more than one non-binary module exist. In that case
             the decimater is uninitialized!
   */
  bool initialize();


  /// Returns whether decimater has been successfully initialized.
  bool is_initialized() const { return initialized_; }


  /// Print information about modules to _os
  void info( std::ostream& _os );

public: //--------------------------------------------------- module management

  /** \brief Add observer
   *
   * You can set an observer which is used as a callback to check the decimators progress and to
   * abort it if necessary.
   *
   * @param _o Observer to be used
   */
  void set_observer(Observer* _o)
  {
      observer_ = _o;
  }

  /// Get current observer of a decimater
  Observer* observer()
  {
      return observer_;
  }

  /// access mesh. used in modules.
  Mesh& mesh() { return mesh_; }

  /// add module to decimater
  template < typename _Module >
  bool add( ModHandleT<_Module>& _mh )
  {
    if (_mh.is_valid())
      return false;

    _mh.init( new _Module(mesh()) );
    all_modules_.push_back( _mh.module() );

    set_uninitialized();

    return true;
  }


  /// remove module
  template < typename _Module >
  bool remove( ModHandleT<_Module>& _mh )
  {
    if (!_mh.is_valid())
      return false;

    typename ModuleList::iterator it = std::find(all_modules_.begin(),
                                                 all_modules_.end(),
                                                 _mh.module() );

    if ( it == all_modules_.end() ) // module not found
      return false;

    delete *it;
    all_modules_.erase( it ); // finally remove from list
    _mh.clear();

    set_uninitialized();
    return true;
  }


  /// get module referenced by handle _mh
  template < typename Module >
  Module& module( ModHandleT<Module>& _mh )
  {
    assert( _mh.is_valid() );
    return *_mh.module();
  }


protected:

  /// returns false, if abort requested by observer
  bool notify_observer(size_t _n_collapses)
  {
    if (observer() && _n_collapses % observer()->get_interval() == 0)
    {
      observer()->notify(_n_collapses);
      return !observer()->abort();
    }
    return true;
  }

  /// Reset the initialized flag, and clear the bmodules_ and cmodule_
  void set_uninitialized() {
    initialized_ = false;
    cmodule_ = 0;
    bmodules_.clear();
  }

  void update_modules(CollapseInfo& _ci)
  {
    typename ModuleList::iterator m_it, m_end = bmodules_.end();
    for (m_it = bmodules_.begin(); m_it != m_end; ++m_it)
      (*m_it)->postprocess_collapse(_ci);
    cmodule_->postprocess_collapse(_ci);
  }


protected: //---------------------------------------------------- private methods

  /// Is an edge collapse legal?  Performs topological test only.
  /// The method evaluates the status bit Locked, Deleted, and Feature.
  /// \attention The method temporarily sets the bit Tagged. After usage
  ///            the bit will be disabled!
  bool is_collapse_legal(const CollapseInfo& _ci);

  /// Calculate priority of an halfedge collapse (using the modules)
  float collapse_priority(const CollapseInfo& _ci);

  /// Pre-process a collapse
  void preprocess_collapse(CollapseInfo& _ci);

  /// Post-process a collapse
  void postprocess_collapse(CollapseInfo& _ci);

  /**
   * This provides a function that allows the setting of a percentage
   * of the original constraint of the modules
   *
   * Note that some modules might re-initialize in their
   * set_error_tolerance_factor function as necessary
   * @param _factor has to be in the closed interval between 0.0 and 1.0
   */
  void set_error_tolerance_factor(double _factor);

  /** Reset the status of this class
   *
   * You have to call initialize again!!
   */
  void reset(){ initialized_ = false; };


private: //------------------------------------------------------- private data


  /// reference to mesh
  Mesh&      mesh_;

  /// list of binary modules
  ModuleList bmodules_;

  /// the current priority module
  Module*    cmodule_;

  /// list of all allocated modules (including cmodule_ and all of bmodules_)
  ModuleList all_modules_;

  /// Flag if all modules were initialized
  bool       initialized_;

  /// observer
  Observer* observer_;

};

//=============================================================================
} // END_NS_DECIMATER
} // END_NS_OPENMESH
//=============================================================================
#if defined(OM_INCLUDE_TEMPLATES) && !defined(OPENMESH_BASE_DECIMATER_DECIMATERT_CC)
#define OPENMESH_BASE_DECIMATER_TEMPLATES
#include "BaseDecimaterT_impl.hh"
#endif
//=============================================================================
#endif // OPENMESH_BASE_DECIMATER_DECIMATERT_HH defined
//=============================================================================
