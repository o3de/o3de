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
//  Implements an reader module for STL files
//
//=============================================================================


#ifndef __STLREADER_HH__
#define __STLREADER_HH__


//=== INCLUDES ================================================================


#include <stdio.h>
#include <string>

#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Utils/SingletonT.hh>
#include <OpenMesh/Core/IO/reader/BaseReader.hh>


//== NAMESPACES ===============================================================


namespace OpenMesh {
namespace IO {

//== FORWARDS =================================================================

class BaseImporter;

//== IMPLEMENTATION ===========================================================


/**
    Implementation of the STL format reader. This class is singleton'ed by
    SingletonT to STLReader.
*/
class OPENMESHDLLEXPORT _STLReader_ : public BaseReader
{
public:

  // constructor
  _STLReader_();

  /// Destructor
  virtual ~_STLReader_() {};


  std::string get_description() const
  { return "Stereolithography Interface Format"; }
  std::string get_extensions() const { return "stl stla stlb"; }

  bool read(const std::string& _filename,
	    BaseImporter& _bi,
            Options& _opt);

  bool read(std::istream& _in,
		    BaseImporter& _bi,
            Options& _opt);

  /** Set the threshold to be used for considering two point to be equal.
      Can be used to merge small gaps */
  void set_epsilon(float _eps) { eps_=_eps; }

  /// Returns the threshold to be used for considering two point to be equal.
  float epsilon() const { return eps_; }



private:

  enum STL_Type { STLA, STLB, NONE };
  STL_Type check_stl_type(const std::string& _filename) const;

  bool read_stla(const std::string& _filename, BaseImporter& _bi, Options& _opt) const;
  bool read_stla(std::istream& _in, BaseImporter& _bi, Options& _opt) const;
  bool read_stlb(const std::string& _filename, BaseImporter& _bi, Options& _opt) const;
  bool read_stlb(std::istream& _in, BaseImporter& _bi, Options& _opt) const;


private:

  float eps_;
};


//== TYPE DEFINITION ==========================================================


/// Declare the single entity of the STL reader
extern _STLReader_  __STLReaderInstance;
OPENMESHDLLEXPORT _STLReader_&  STLReader();


//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
#endif
//=============================================================================
