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




#ifndef OPENMESH_IO_OPTIONS_HH
#define OPENMESH_IO_OPTIONS_HH


//=== INCLUDES ================================================================


// OpenMesh
#include <OpenMesh/Core/System/config.h>


//== NAMESPACES ==============================================================


namespace OpenMesh {
namespace IO   {


//=== IMPLEMENTATION ==========================================================


/** \name Mesh Reading / Writing
    Option for reader and writer modules.
*/
//@{


//-----------------------------------------------------------------------------

/** \brief Set options for reader/writer modules.
 *
 *  The class is used in a twofold way.
 *  -# In combination with reader modules the class is used
 *     - to pass hints to the reading module, whether the input is
 *     binary and what byte ordering the binary data has
 *     - to retrieve information about the file contents after
 *     succesful reading.
 *  -# In combination with write modules the class gives directions to
 *     the writer module, whether to
 *     - use binary mode or not and what byte order to use
 *     - store one of the standard properties.
 *
 *  The option are defined in \c Options::Flag as bit values and stored in
 *  an \c int value as a bitset.
 */
class Options
{
public:
  typedef int       enum_type;
  typedef enum_type value_type;

  /// Definitions of %Options for reading and writing. The options can be
  /// or'ed.
  enum Flag {
      Default        = 0x0000, ///< No options
      Binary         = 0x0001, ///< Set binary mode for r/w
      MSB            = 0x0002, ///< Assume big endian byte ordering
      LSB            = 0x0004, ///< Assume little endian byte ordering
      Swap           = 0x0008, ///< Swap byte order in binary mode
      VertexNormal   = 0x0010, ///< Has (r) / store (w) vertex normals
      VertexColor    = 0x0020, ///< Has (r) / store (w) vertex colors
      VertexTexCoord = 0x0040, ///< Has (r) / store (w) texture coordinates
      EdgeColor      = 0x0080, ///< Has (r) / store (w) edge colors
      FaceNormal     = 0x0100, ///< Has (r) / store (w) face normals
      FaceColor      = 0x0200, ///< Has (r) / store (w) face colors
      FaceTexCoord   = 0x0400, ///< Has (r) / store (w) face texture coordinates
      ColorAlpha     = 0x0800, ///< Has (r) / store (w) alpha values for colors
      ColorFloat     = 0x1000, ///< Has (r) / store (w) float values for colors (currently only implemented for PLY and OFF files)
      Custom         = 0x2000, ///< Has (r)             custom properties (currently only implemented in PLY Reader ASCII version)
      Status         = 0x4000  ///< Has (r) / store (w) status properties
  };

public:

  /// Default constructor
  Options() : flags_( Default )
  { }


  /// Copy constructor
  Options(const Options& _opt) : flags_(_opt.flags_)
  { }


  /// Initializing constructor setting a single option
  Options(Flag _flg) : flags_( _flg)
  { }


  /// Initializing constructor setting multiple options
  Options(const value_type _flgs) : flags_( _flgs)
  { }


  ~Options()
  { }

  /// Restore state after default constructor.
  void cleanup(void)
  { flags_ = Default; }

  /// Clear all bits.
  void clear(void)
  { flags_ = 0; }

  /// Returns true if all bits are zero.
  bool is_empty(void) const { return !flags_; }

public:


  //@{
  /// Copy options defined in _rhs.

  Options& operator = ( const Options& _rhs )
  { flags_ = _rhs.flags_; return *this; }

  Options& operator = ( const value_type _rhs )
  { flags_ = _rhs; return *this; }

  //@}


  //@{
  /// Unset options defined in _rhs.

  Options& operator -= ( const value_type _rhs )
  { flags_ &= ~_rhs; return *this; }

  Options& unset( const value_type _rhs)
  { return (*this -= _rhs); }

  //@}



  //@{
  /// Set options defined in _rhs

  Options& operator += ( const value_type _rhs )
  { flags_ |= _rhs; return *this; }

  Options& set( const value_type _rhs)
  { return (*this += _rhs); }

  //@}

public:


  // Check if an option or several options are set.
  bool check(const value_type _rhs) const
  {
    return (flags_ & _rhs)==_rhs;
  }

  bool is_binary()           const { return check(Binary); }
  bool vertex_has_normal()   const { return check(VertexNormal); }
  bool vertex_has_color()    const { return check(VertexColor); }
  bool vertex_has_texcoord() const { return check(VertexTexCoord); }
  bool vertex_has_status()   const { return check(Status); }
  bool edge_has_color()      const { return check(EdgeColor); }
  bool edge_has_status()     const { return check(Status); }
  bool halfedge_has_status() const { return check(Status); }
  bool face_has_normal()     const { return check(FaceNormal); }
  bool face_has_color()      const { return check(FaceColor); }
  bool face_has_texcoord()   const { return check(FaceTexCoord); }
  bool face_has_status()     const { return check(Status); }
  bool color_has_alpha()     const { return check(ColorAlpha); }
  bool color_is_float()      const { return check(ColorFloat); }


  /// Returns true if _rhs has the same options enabled.
  bool operator == (const value_type _rhs) const
  { return flags_ == _rhs; }


  /// Returns true if _rhs does not have the same options enabled.
  bool operator != (const value_type _rhs) const
  { return flags_ != _rhs; }


  /// Returns the option set.
  operator value_type ()     const { return flags_; }

private:

  bool operator && (const value_type _rhs) const;

  value_type flags_;
};

//-----------------------------------------------------------------------------




//@}


//=============================================================================
} // namespace IO
} // namespace OpenMesh
//=============================================================================
#endif
//=============================================================================
