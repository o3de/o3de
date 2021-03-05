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
//  Implements the baseclass for IOManager file access modules
//
//=============================================================================


#ifndef __BASEREADER_HH__
#define __BASEREADER_HH__


//=== INCLUDES ================================================================


// STD C++
#include <iosfwd>
#include <string>
#include <cctype>
#include <functional>
#include <algorithm>

// OpenMesh
#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/IO/Options.hh>
#include <OpenMesh/Core/IO/importer/BaseImporter.hh>
#include <OpenMesh/Core/Utils/SingletonT.hh>


//== NAMESPACES ===============================================================


namespace OpenMesh {
namespace IO {


//=== IMPLEMENTATION ==========================================================


/**
   Base class for reader modules.
   Reader modules access persistent data and pass them to the desired
   data structure by the means of a BaseImporter derivative.
   All reader modules must be derived from this class.
*/
class OPENMESHDLLEXPORT BaseReader
{
public:

  /// Destructor
  virtual ~BaseReader() {};

  /// Returns a brief description of the file type that can be parsed.
  virtual std::string get_description() const = 0;
  
  /** Returns a string with the accepted file extensions separated by a 
      whitespace and in small caps.
  */
  virtual std::string get_extensions() const = 0;

  /// Return magic bits used to determine file format
  virtual std::string get_magic() const { return std::string(""); }


  /** Reads a mesh given by a filename. Usually this method opens a stream
      and passes it to stream read method. Acceptance checks by filename
      extension can be placed here.

      Options can be passed via _opt. After execution _opt contains the Options
      that were available
  */
  virtual bool read(const std::string& _filename, 
		    BaseImporter& _bi,
                    Options& _opt) = 0;
		
 /** Reads a mesh given by a std::stream. This method usually uses the same stream reading method
    that read uses. Options can be passed via _opt. After execution _opt contains the Options
      that were available.

      Please make sure that if _is is std::ifstream, the correct std::ios_base::openmode flags are set. 
  */
  virtual bool read(std::istream& _is, 
		    BaseImporter& _bi,
                    Options& _opt) = 0;


  /** \brief Returns true if writer can parse _filename (checks extension).
   * _filename can also provide an extension without a name for a file e.g. _filename == "om" checks, if the reader can read the "om" extension
   * @param _filename complete name of a file or just the extension
   * @result true, if reader can read data with the given extension
   */
  virtual bool can_u_read(const std::string& _filename) const;


protected:

  // case insensitive search for _ext in _fname.
  bool check_extension(const std::string& _fname, 
		       const std::string& _ext) const;
};


/** \brief Trim left whitespace
 *
 * Removes whitespace at the beginning of the string
 *
 * @param _string input string
 * @return trimmed string
 */
static inline std::string &left_trim(std::string &_string) {

  // Find out if the compiler supports CXX11
  #if ( __cplusplus >= 201103L || _MSVC_LANG >= 201103L )
    // as with CXX11 we can use lambda expressions
    _string.erase(_string.begin(), std::find_if(_string.begin(), _string.end(), [](int i)->int { return ! std::isspace(i); }));
  #else
    // we do what we did before
    _string.erase(_string.begin(), std::find_if(_string.begin(), _string.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
  #endif
  
  return _string;
}

/** \brief Trim right whitespace
 *
 * Removes whitespace at the end of the string
 *
 * @param _string input string
 * @return trimmed string
 */
static inline std::string &right_trim(std::string &_string) {

  // Find out if the compiler supports CXX11
  #if ( __cplusplus >= 201103L || _MSVC_LANG >= 201103L )
    // as with CXX11 we can use lambda expressions
    _string.erase(std::find_if(_string.rbegin(), _string.rend(), [](int i)->int { return ! std::isspace(i); } ).base(), _string.end());
  #else
    // we do what we did before
    _string.erase(std::find_if(_string.rbegin(), _string.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), _string.end());
  #endif



  return _string;
}

/** \brief Trim whitespace
 *
 * Removes whitespace at the beginning and end of the string
 *
 * @param _string input string
 * @return trimmed string
 */
static inline std::string &trim(std::string &_string) {
  return left_trim(right_trim(_string));
}



//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
#endif
//=============================================================================
