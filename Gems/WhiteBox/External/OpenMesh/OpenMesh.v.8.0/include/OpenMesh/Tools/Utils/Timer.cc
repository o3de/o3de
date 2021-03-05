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



#ifndef DOXY_IGNORE_THIS
// ----------------------------------------------------------------------------
#include <OpenMesh/Core/System/config.h>
#if defined(OM_CC_MIPS)
#  include <math.h>
#  include <stdio.h>
#else
#  include <cmath>
#  include <cstdio>
#endif
#include "Timer.hh"
// ----------------------------------------------------------------------------

// ------------------------------------------------------------- namespace ----

namespace OpenMesh {
namespace Utils {


// ----------------------------------------------------------------------------

using namespace std;

// -------------------------------------------------------------- TimerImpl ----
// just a base class for the implementation
class TimerImpl
{
protected:
public:
  TimerImpl()  { ; }
  virtual ~TimerImpl() { ; }

  virtual void   reset(void) = 0;
  virtual void   start(void) = 0;
  virtual void   stop(void)  = 0;
  virtual void   cont(void)  = 0;
  virtual double seconds(void) const = 0;
};

// compiler and os dependent implementation

// ------------------------------------------------------------- windows 32 ----
#if defined(WIN32) && (defined(_MSC_VER) || defined(__INTEL_COMPILER) || defined (__MINGW32__) )

#ifndef DOXY_IGNORE_THIS
#include <windows.h>
#endif

class TimerImplWin32 : public TimerImpl
{
protected:
   LARGE_INTEGER freq_;
   LARGE_INTEGER count_;
   LARGE_INTEGER start_;

public:
   TimerImplWin32(void);
   ~TimerImplWin32(void) { ; }

   virtual void   reset(void);
   virtual void   start(void);
   virtual void   stop(void);
   virtual void   cont(void);
   virtual double seconds(void) const;
};

TimerImplWin32::TimerImplWin32(void)
{
   if (QueryPerformanceFrequency(&freq_)==FALSE)
     throw std::runtime_error("Performance counter of of stock!");
   reset();
}

void TimerImplWin32::reset(void)
{
   memset(&count_,0,sizeof(count_));
   memset(&start_,0,sizeof(count_));
}

void TimerImplWin32::start(void)
{
   reset();
   QueryPerformanceCounter(&start_);
}

void TimerImplWin32::stop(void)
{
   LARGE_INTEGER stop_;

   QueryPerformanceCounter(&stop_);
   count_.QuadPart += stop_.QuadPart - start_.QuadPart;
}

void TimerImplWin32::cont(void)
{
   QueryPerformanceCounter(&start_);
}

double TimerImplWin32::seconds(void) const
{
   return (double)count_.QuadPart/(double)freq_.QuadPart;
}

// ------------------------------------------------------------- posix time ----
#elif defined(__GNUC__) && defined(__POSIX__) 

#ifndef DOXY_IGNORE_THIS
#  include <time.h>
#endif

template <clockid_t N>
class TimerImplPosix : public TimerImpl
{
public:
    TimerImplPosix() : id_(N), seconds_(0.0)
    { }

    ~TimerImplPosix()
    { }

    virtual void reset(void) { seconds_ = 0.0; }

    virtual void start(void) { seconds_ = 0.0; clock_gettime( id_, &start_ ); }
    virtual void stop(void)
    {
        timespec stop;
        clock_gettime( id_, &stop );
        seconds_ += ( stop.tv_sec - start_.tv_sec );
        seconds_ += ( (double(stop.tv_nsec-start_.tv_nsec)*1e-9) );
    }

    virtual void cont(void)  { clock_gettime( id_, &start_ ); }

    virtual double seconds() const { return seconds_; }

protected:
   clockid_t id_;
   double    seconds_;
   timespec  start_;
};

// ----------------------------------------------------------- gettimeofday ----
#elif (defined(__GNUC__) || (defined(__INTEL_COMPILER) && !defined(WIN32))) && !defined(__MINGW32__)

#  include <sys/time.h>
#  include <sys/resource.h>
#  include <unistd.h>

class TimerImplGToD: public TimerImpl
{
public:
    TimerImplGToD() : seconds_(0.0)
    { }

    ~TimerImplGToD()
    { }

    virtual void reset(void) { seconds_ = 0.0; }
    virtual void start(void) { seconds_ = 0.0; gettimeofday( &start_, &tz_ ); }

    virtual void stop(void)
    {
      gettimeofday( &stop_, &tz_ );

      seconds_ +=  (double)(stop_.tv_sec - start_.tv_sec);
      seconds_ +=  (double)(stop_.tv_usec- start_.tv_usec)*1e-6;
    }

    virtual void cont(void)  { gettimeofday( &start_, &tz_); }

    virtual double seconds() const { return seconds_; }

private:
  
  struct timeval start_, stop_;
  struct timezone tz_;

  double seconds_;
};


#else // ---------------------------------------- standard implementation ----

#include <time.h>

static const unsigned long clockticks = CLOCKS_PER_SEC;

class TimerImplStd : public TimerImpl
{
public:
   TimerImplStd() : freq_(clockticks),count_(0),start_(0) { reset(); }
   ~TimerImplStd() { ; }

   virtual void   reset(void) { count_ = 0; }
   virtual void   start(void) { count_ = 0; start_ = clock(); }
   virtual void   stop(void);
   virtual void   cont(void)  { start_ = clock(); }
   virtual double seconds(void) const { return (double)count_/(double)freq_; }

protected:
   unsigned long freq_;
   unsigned long count_;
   unsigned long start_;
};

void TimerImplStd::stop(void)
{
   unsigned long stop_ = clock();
   count_ += stop_-start_;
}

#endif

// ----------------------------------------------------------------- Timer ----

Timer::Timer(void) :
    state_(Stopped)
{
#if defined(WIN32) && (defined(_MSC_VER) || defined(__INTEL_COMPILER) || defined(__MINGW32__)) 
  impl_       = new TimerImplWin32;
#elif defined(__GNUC__) && defined(__POSIX__)
// CLOCK_REALTIME
// CLOCK_MONOTONIC     - ?
// CLOCK_REALTIME_HR   - RTlinux
// CLOCK_MONOTONIC_HR  - ?
#  if defined(CLOCK_REALTIME_HR)
  impl_      = new TimerImplPosix<CLOCK_REALTIME_HR>;
#  else
  impl_      = new TimerImplPosix<CLOCK_REALTIME>;
#  endif
#elif (defined(__GNUC__) || (defined(__INTEL_COMPILER) && !defined(WIN32)) ) && !defined(__MINGW32__)
  impl_      = new TimerImplGToD;
#else
  impl_       = new TimerImplStd;
#endif

}

Timer::~Timer(void)
{
   delete impl_;
   state_ = Stopped;
}

void Timer::reset(void)
{
   state_ = Stopped;
   impl_->reset();
}

void Timer::start(void)
{
   state_ = Running;
   impl_->start();
}

void Timer::stop(void)
{
   impl_->stop();
   state_ = Stopped;
}

void Timer::cont(void)
{
   impl_->cont();
   state_ = Running;
}

double Timer::seconds(void) const
{
   return state_==Stopped ? impl_->seconds() : 0.0;
}

std::string Timer::as_string(Timer::Format format)
{
   if (state_ == Running)
       return "Running";
   return as_string(impl_->seconds(),format);
}

std::string Timer::as_string(double seconds, Timer::Format format)
{
   char string[32];

   double fraction;
   double integer;
   unsigned long t;
//    double rest;
   short  hour,min,sec;
   bool   negative = false;
   
   if ( seconds < 0 )
   {
      negative = true;
      seconds *= -1;
   }
   
   fraction = modf(seconds,&integer);

   t = (unsigned long)integer;

   hour = short( t / 3600L );
   t   %= 3600L;
   min  = short( t / 60L );
   t   %= 60L;
   sec  = short( t );
//    rest = (double)t + fraction;

   char *ptr = string;
   if (negative)
      *ptr++ = '-';
   
   switch(format)
   {
   case Timer::Automatic:     
      if (hour)
         ptr += sprintf(ptr,"%02dh:",hour);

      if (min)
         ptr += sprintf(ptr,"%02dm:",min);
      else if (ptr>string && hour)
         ptr += sprintf(ptr,"00m:");

      if (sec)
         ptr += sprintf(ptr,"%02d",sec);
      else if (ptr>string && min)
         ptr += sprintf(ptr,"00");

      if (!hour && !min) // higher resolution necessary
      {
         if (ptr > string && sec)
         {
            sprintf(ptr,".%.3fs",fraction);
            ptr++;
            while(*(ptr+2))
            { 
               *ptr = *(ptr+2);
               ptr++;
            }
            *ptr = '\0';
         }
         else if ( fraction * 1e2 > 0.1)
            sprintf(ptr,"%.3fcs",fraction*1.e2);
         else if ( fraction * 1e3 > 0.1)
            sprintf(ptr,"%.3fms",fraction*1.e3);
         else if ( fraction * 1e6 > 0.1)
            sprintf(ptr,"%.1f\xb5s",fraction*1.e6);
         else if ( fraction * 1e9 > 0.1)
            sprintf(ptr,"%.1fns",fraction*1.e9);
         else
            sprintf(ptr,"%.1fps",fraction*1.e12);            
      } else // append a 's' for seconds!
      {
         ptr[0] = 's';
         ptr[1] = '\0';
      }
      break;
   case Timer::Long:
      ptr += sprintf(ptr,"%02dh:%02dm:%02d",hour,min,sec);
      sprintf(ptr,".%.12fs",fraction);
      ptr++;
      while(*(ptr+2))
      { 
         *ptr = *(ptr+2);
         ptr++;
      }
      *ptr = '\0';
      break;
   case Timer::Hours:
      sprintf(ptr,"%02dh:%02dm:%02ds",hour,min,sec); break;
   case Timer::Minutes:
      ptr += sprintf(ptr,"%02dm:%02d", min, sec);
      sprintf(ptr,".%.2fs",fraction);
      ptr++;
      while(*(ptr+2))
      { 
         *ptr = *(ptr+2);
         ptr++;
      }
      *ptr = '\0';
      break;
   case Timer::Seconds: sprintf(ptr,"%.3fs",seconds); break;
   case Timer::HSeconds: sprintf(ptr,"%.3fcs",seconds*1e2); break;
   case Timer::MSeconds:  sprintf(ptr,"%.3fms",seconds*1e3); break;
   case Timer::MicroSeconds: sprintf(ptr,"%.1f\xb5s",seconds*1e6); break;
   case Timer::NanoSeconds: sprintf(ptr,"%.1fns",seconds*1e9); break;
   }
   return string;
}

// ============================================================================
} // END_NS_UTILS
} // END_NS_OPENMESH
// ----------------------------------------------------------------------------
#endif // DOXY_IGNORE_THIS
// ============================================================================
// end of file Timer.cc
// ============================================================================

