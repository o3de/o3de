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
//  Implements a simple singleton template
//
//=============================================================================


#ifndef __SINGLETON_HH__
#define __SINGLETON_HH__


//=== INCLUDES ================================================================

// OpenMesh
#include <OpenMesh/Core/System/config.h>

// STL
#include <stdexcept>


//== NAMESPACES ===============================================================


namespace OpenMesh {


//=== IMPLEMENTATION ==========================================================


/** A simple singleton template.
    Encapsulates an arbitrary class and enforces its uniqueness.
*/

template <typename T>
class SingletonT
{
public:

  /** Singleton access function.
      Use this function to obtain a reference to the instance of the 
      encapsulated class. Note that this instance is unique and created
      on the first call to Instance().
  */

  static T& Instance()
  {
    if (!pInstance__)
    {
      // check if singleton alive
      if (destroyed__)
      {
	OnDeadReference();
      }
      // first time request -> initialize
      else
      {
	Create();
      }
    }
    return *pInstance__;
  }


private:

  // Disable constructors/assignment to enforce uniqueness
  SingletonT();
  SingletonT(const SingletonT&);
  SingletonT& operator=(const SingletonT&);

  // Create a new singleton and store its pointer
  static void Create()
  {
    static T theInstance;
    pInstance__ = &theInstance;
  }
  
  // Will be called if instance is accessed after its lifetime has expired
  static void OnDeadReference()
  {
    throw std::runtime_error("[Singelton error] - Dead reference detected!\n");
  }

  virtual ~SingletonT()
  {
    pInstance__ = 0;
    destroyed__ = true;
  }
  
  static T*     pInstance__;
  static bool   destroyed__;
};



//=============================================================================
} // namespace OpenMesh
//=============================================================================
#if defined(OM_INCLUDE_TEMPLATES) && !defined(OPENMESH_SINGLETON_C)
#  define OPENMESH_SINGLETON_TEMPLATES
#  include "SingletonT_impl.hh"
#endif
//=============================================================================
#endif // __SINGLETON_HH__
//=============================================================================
