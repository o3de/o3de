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

/** \file Observer.hh
 *
 * This file contains an observer class which is used to monitor the progress
 * of an decimater.
 *
 */

//=============================================================================
//
//  CLASS Observer
//
//=============================================================================

#pragma once

//== INCLUDES =================================================================

#include <cstddef>
#include <OpenMesh/Core/System/config.h>

//== NAMESPACE ================================================================

namespace OpenMesh  {
namespace Decimater {


//== CLASS DEFINITION =========================================================

/** \brief Observer class
 *
 * Observers can be used to monitor the progress of the decimation and to
 * abort it in between.
 */
class OPENMESHDLLEXPORT Observer
{
public:

  /** Create an observer
   *
   * @param _notificationInterval Interval of decimation steps between notifications.
   */
  explicit Observer(size_t _notificationInterval);
  
  /// Destructor
  virtual ~Observer();
  
  /// Get the interval between notification steps
  size_t get_interval() const;

  /// Set the interval between notification steps
  void set_interval(size_t _notificationInterval);
  
  /** \brief callback
   *
   * This function has to be overloaded. It will be called regularly during
   * the decimation process and will return the current step.
   *
   * @param _step Current step of the decimater
   */
  virtual void notify(size_t _step) = 0;

  /** \brief Abort callback
   *
   * After each notification, this function is called by the decimater. If the
   * function returns true, the decimater will stop at a consistent state. Otherwise
   * it will continue.
   *
   * @return abort Yes or No
   */
  virtual bool abort() const;
  
private:
  size_t notificationInterval_;
};


//=============================================================================
} // END_NS_DECIMATER
} // END_NS_OPENMESH
//=============================================================================
