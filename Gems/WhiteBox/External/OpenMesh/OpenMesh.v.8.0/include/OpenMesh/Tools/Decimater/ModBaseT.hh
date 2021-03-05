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



/** \file ModBaseT.hh
    Base class for all decimation modules.
 */

//=============================================================================
//
//  CLASS ModBaseT
//
//=============================================================================

#ifndef OPENMESH_DECIMATER_MODBASET_HH
#define OPENMESH_DECIMATER_MODBASET_HH


//== INCLUDES =================================================================

#include <OpenMesh/Core/Utils/Noncopyable.hh>
#include <OpenMesh/Tools/Decimater/CollapseInfoT.hh>
#include <string>


//== NAMESPACE ================================================================

namespace OpenMesh  {
namespace Decimater {


//== FORWARD DECLARATIONS =====================================================

template <typename Mesh> class BaseDecimaterT;


//== CLASS DEFINITION =========================================================

/** Handle for mesh decimation modules
    \internal
 */

template <typename Module>
class ModHandleT : private Utils::Noncopyable
{
public:

  typedef ModHandleT<Module> Self;
  typedef Module module_type;

public:

  /// Default constructor
  ModHandleT() : mod_(NULL) {}

  /// Destructor
  ~ModHandleT() { /* don't delete mod_, since handle is not owner! */ }

  /// Check handle status
  /// \return \c true, if handle is valid, else \c false.
  bool is_valid() const { return mod_ != NULL; }

private:

#if defined(OM_CC_MSVC)
  friend class BaseDecimaterT;
#else
  template <typename Mesh> friend class BaseDecimaterT;
#endif

  void     clear()           { mod_ = NULL; }
  void     init(Module* _m)  { mod_ = _m;   }
  Module*  module()          { return mod_; }


private:

  Module* mod_;

};




//== CLASS DEFINITION =========================================================



/// Macro that sets up the name() function
/// \internal
#define DECIMATER_MODNAME(_mod_name) \
 virtual const std::string& name() const { \
  static std::string _s_modname_(#_mod_name); return _s_modname_; \
}


/** Convenience macro, to be used in derived modules
 *  The macro defines the types
 *  - \c Handle, type of the module's handle.
 *  - \c Base,   type of ModBaseT<>.
 *  - \c Mesh,   type of the associated mesh passed by the decimater type.
 *  - \c CollapseInfo,  to your convenience
 *  and uses DECIMATER_MODNAME() to define the name of the module.
 *
 *  \param Classname  The name of the derived class.
 *  \param MeshT      Pass here the mesh type, which is the
 *                    template parameter passed to ModBaseT.
 *  \param Name       Give the module a name.
 */
#define DECIMATING_MODULE(Classname, MeshT, Name)	\
  typedef Classname < MeshT >    Self;		\
  typedef OpenMesh::Decimater::ModHandleT< Self >     Handle; \
  typedef OpenMesh::Decimater::ModBaseT< MeshT > Base;   \
  typedef typename Base::Mesh         Mesh;		\
  typedef typename Base::CollapseInfo CollapseInfo;	\
  DECIMATER_MODNAME( Name )



//== CLASS DEFINITION =========================================================


/** Base class for all decimation modules.

    Each module has to implement this interface.
    To build your own module you have to
    -# derive from this class.
    -# create the basic settings with DECIMATING_MODULE().
    -# override collapse_priority(), if necessary.
    -# override initialize(), if necessary.
    -# override postprocess_collapse(), if necessary.

    A module has two major working modes:
    -# binary mode
    -# non-binary mode

    In the binary mode collapse_priority() checks a constraint and
    returns LEGAL_COLLAPSE or ILLEGAL_COLLAPSE.

    In the non-binary mode the module computes a float error value in
    the range [0, inf) and returns it. In the case a constraint has
    been set, e.g. the error must be lower than a upper bound, and the
    constraint is violated, collapse_priority() must return
    ILLEGAL_COLLAPSE.

    \see collapse_priority()

    \todo "Tutorial on building a custom decimation module."

*/

template <typename MeshT>
class ModBaseT
{
public:
  typedef MeshT Mesh;
  typedef CollapseInfoT<MeshT>                 CollapseInfo;

  enum {
    ILLEGAL_COLLAPSE = -1, ///< indicates an illegal collapse
    LEGAL_COLLAPSE   = 0   ///< indicates a legal collapse
  };

protected:

  /// Default constructor
  /// \see \ref decimater_docu
  ModBaseT(MeshT& _mesh, bool _is_binary)
    : error_tolerance_factor_(1.0), mesh_(_mesh), is_binary_(_is_binary) {}

public:

  /// Virtual desctructor
  virtual ~ModBaseT() { }

  /// Set module's name (using DECIMATER_MODNAME macro)
  DECIMATER_MODNAME(ModBase);


  /// Returns true if criteria returns a binary value.
  bool is_binary(void) const { return is_binary_; }

  /// Set whether module is binary or not.
  void set_binary(bool _b)   { is_binary_ = _b; }


public: // common interface

   /// Initialize module-internal stuff
   virtual void initialize() { }

   /** Return collapse priority.
    *
    *  In the binary mode collapse_priority() checks a constraint and
    *  returns LEGAL_COLLAPSE or ILLEGAL_COLLAPSE.
    *
    *  In the non-binary mode the module computes a float error value in
    *  the range [0, inf) and returns it. In the case a constraint has
    *  been set, e.g. the error must be lower than a upper bound, and the
    *  constraint is violated, collapse_priority() must return
    *  ILLEGAL_COLLAPSE.
    *
    *  \return Collapse priority in the range [0,inf),
    *          \c LEGAL_COLLAPSE or \c ILLEGAL_COLLAPSE.
    */
   virtual float collapse_priority(const CollapseInfoT<MeshT>& /* _ci */)
   { return LEGAL_COLLAPSE; }

   /** Before _from_vh has been collapsed into _to_vh, this method
       will be called.
    */
   virtual void preprocess_collapse(const CollapseInfoT<MeshT>& /* _ci */)
   {}

   /** After _from_vh has been collapsed into _to_vh, this method
       will be called.
    */
   virtual void postprocess_collapse(const CollapseInfoT<MeshT>& /* _ci */)
   {}

   /**
    * This provides a function that allows the setting of a percentage
    * of the original contraint.
    *
    * Note that the module might need to be re-initialized again after
    * setting the percentage
    * @param _factor has to be in the closed interval between 0.0 and 1.0
    */
   virtual void set_error_tolerance_factor(double _factor) {
     if (_factor >= 0.0 && _factor <= 1.0)
       error_tolerance_factor_ = _factor;
   }


protected:

  /// Access the mesh associated with the decimater.
  MeshT& mesh() { return mesh_; }

  // current percentage of the original constraint
  double error_tolerance_factor_;

private:

  // hide copy constructor & assignemnt
  ModBaseT(const ModBaseT& _cpy);
  ModBaseT& operator=(const ModBaseT& );

  MeshT& mesh_;

  bool is_binary_;
};


//=============================================================================
} // namespace Decimater
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_DECIMATER_MODBASE_HH defined
//=============================================================================

