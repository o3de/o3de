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



#ifndef TIMER_HH
#define TIMER_HH
// ----------------------------------------------------------------------------

/** \file Timer.hh
    A timer class
*/


// ----------------------------------------------------------------------------

#include <OpenMesh/Core/System/config.hh>
//
#include <ostream>
#include <string>
#if defined(OM_CC_MIPS)
#  include <assert.h>
#else
#  include <cassert>
#endif


// ------------------------------------------------------------- namespace ----

namespace OpenMesh {
namespace Utils {


// -------------------------------------------------------------- forwards ----


class TimerImpl;


// ----------------------------------------------------------------- class ----

/** Timer class
 */
class OPENMESHDLLEXPORT Timer
{
public:

  /// Formatting options for member Timer::as_string()
  enum Format {
    Automatic,
    Long,
    Hours,
    Minutes,
    Seconds,
    HSeconds,
    MSeconds,
    MicroSeconds,
    NanoSeconds
  };

  Timer(void);

  Timer(const Timer& _other) = delete;

  ~Timer(void);

  /// Returns true if self is in a valid state!
  bool is_valid() const { return state_!=Invalid; }

  bool is_stopped() const { return state_==Stopped; }

  /// Reset the timer
  void reset(void);

  /// Start measurement
  void start(void);

  /// Stop measurement
  void stop(void);

  /// Continue measurement
  void cont(void);

  /// Give resolution of timer. Depends on the underlying measurement method.
  float resolution() const;
    
  /// Returns measured time in seconds, if the timer is in state 'Stopped'
  double seconds(void) const;

  /// Returns measured time in hundredth seconds, if the timer is in state 'Stopped'
  double hseconds(void) const { return seconds()*1e2; }

  /// Returns measured time in milli seconds, if the timer is in state 'Stopped'
  double mseconds(void) const { return seconds()*1e3; }

  /// Returns measured time in micro seconds, if the timer is in state 'Stopped'
  double useconds(void) const { return seconds()*1e6; }
  
  /** Returns the measured time as a string. Use the format flags to specify
      a wanted resolution.
   */
  std::string as_string(Format format = Automatic);
  
  /** Returns a given measured time as a string. Use the format flags to 
      specify a wanted resolution.
   */
  static std::string as_string(double seconds, Format format = Automatic);

public:

  //@{
  /// Compare timer values
  bool operator < (const Timer& t2) const 
  { 
    assert( is_stopped() && t2.is_stopped() ); 
    return (seconds() < t2.seconds()); 
  }

  bool operator > (const Timer& t2) const
  { 
    assert( is_stopped() && t2.is_stopped() ); 
    return (seconds() > t2.seconds()); 
  }

  bool operator == (const Timer& t2) const
  { 
    assert( is_stopped() && t2.is_stopped() ); 
    return (seconds() == t2.seconds()); 
  }

  bool operator <= (const Timer& t2) const
  {
    assert( is_stopped() && t2.is_stopped() ); 
    return (seconds() <= t2.seconds()); 
  }

  bool operator >=(const Timer& t2) const
  { 
    assert( is_stopped() && t2.is_stopped() ); 
    return (seconds() >= t2.seconds()); 
  }
  //@}

protected:

  TimerImpl *impl_;

  enum {
    Invalid = -1,
    Stopped =  0,
    Running =  1
  } state_;

};


/** Write seconds to output stream. 
 *  Timer must be stopped before.
 *  \relates Timer
 */
inline std::ostream& operator << (std::ostream& _o, const Timer& _t)
{
   return (_o << _t.seconds());
}


// ============================================================================
} // END_NS_UTILS
} // END_NS_OPENMESH
// ============================================================================
#endif
// end of Timer.hh
// ===========================================================================

