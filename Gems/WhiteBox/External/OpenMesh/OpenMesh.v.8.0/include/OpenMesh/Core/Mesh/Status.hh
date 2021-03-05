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
//  CLASS Status
//
//=============================================================================


#ifndef OPENMESH_ATTRIBUTE_STATUS_HH
#define OPENMESH_ATTRIBUTE_STATUS_HH


//== INCLUDES =================================================================

#include <OpenMesh/Core/System/config.h>


//== NAMESPACES ===============================================================


namespace OpenMesh {
namespace Attributes {

 
//== CLASS DEFINITION  ========================================================
  

/** Status bits used by the Status class. 
 *  \see OpenMesh::Attributes::StatusInfo
 */
enum StatusBits {

  DELETED               = 1,    ///< Item has been deleted
  LOCKED                = 2,    ///< Item is locked.
  SELECTED              = 4,    ///< Item is selected.
  HIDDEN                = 8,    ///< Item is hidden.
  FEATURE               = 16,   ///< Item is a feature or belongs to a feature.
  TAGGED                = 32,   ///< Item is tagged.
  TAGGED2               = 64,   ///< Alternate bit for tagging an item.
  FIXEDNONMANIFOLD      = 128,  ///< Item was non-two-manifold and had to be fixed
  UNUSED                = 256   ///< Unused
};


/** \class StatusInfo Status.hh <OpenMesh/Attributes/Status.hh>
 *
 *   Add status information to a base class.
 *
 *   \see StatusBits
 */
class StatusInfo
{
public:

  typedef unsigned int value_type;
    
  StatusInfo() : status_(0) {}

  /// is deleted ?
  bool deleted() const  { return is_bit_set(DELETED); }
  /// set deleted
  void set_deleted(bool _b) { change_bit(DELETED, _b); }


  /// is locked ?
  bool locked() const  { return is_bit_set(LOCKED); }
  /// set locked
  void set_locked(bool _b) { change_bit(LOCKED, _b); }


  /// is selected ?
  bool selected() const  { return is_bit_set(SELECTED); }
  /// set selected
  void set_selected(bool _b) { change_bit(SELECTED, _b); }


  /// is hidden ?
  bool hidden() const  { return is_bit_set(HIDDEN); }
  /// set hidden
  void set_hidden(bool _b) { change_bit(HIDDEN, _b); }


  /// is feature ?
  bool feature() const  { return is_bit_set(FEATURE); }
  /// set feature
  void set_feature(bool _b) { change_bit(FEATURE, _b); }


  /// is tagged ?
  bool tagged() const  { return is_bit_set(TAGGED); }
  /// set tagged
  void set_tagged(bool _b) { change_bit(TAGGED, _b); }


  /// is tagged2 ? This is just one more tag info.
  bool tagged2() const  { return is_bit_set(TAGGED2); }
  /// set tagged
  void set_tagged2(bool _b) { change_bit(TAGGED2, _b); }
  
  
  /// is fixed non-manifold ?
  bool fixed_nonmanifold() const  { return is_bit_set(FIXEDNONMANIFOLD); }
  /// set fixed non-manifold
  void set_fixed_nonmanifold(bool _b) { change_bit(FIXEDNONMANIFOLD, _b); }


  /// return whole status
  unsigned int bits() const { return status_; }
  /// set whole status at once
  void set_bits(unsigned int _bits) { status_ = _bits; }


  /// is a certain bit set ?
  bool is_bit_set(unsigned int _s) const { return (status_ & _s) > 0; }
  /// set a certain bit
  void set_bit(unsigned int _s) { status_ |= _s; }
  /// unset a certain bit
  void unset_bit(unsigned int _s) { status_ &= ~_s; }
  /// set or unset a certain bit
  void change_bit(unsigned int _s, bool _b) {  
    if (_b) status_ |= _s; else status_ &= ~_s; }


private: 

  value_type status_;
};


//=============================================================================
} // namespace Attributes
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_ATTRIBUTE_STATUS_HH defined
//=============================================================================
