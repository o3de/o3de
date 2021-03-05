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
//  multiplex streams & ultilities
//
//=============================================================================

#ifndef OPENMESH_MOSTREAM_HH
#define OPENMESH_MOSTREAM_HH


//== INCLUDES =================================================================

#include <OpenMesh/Core/System/config.h>
#include <ostream>
#if defined( OM_CC_GCC ) && OM_CC_VERSION < 30000
#  include <streambuf.h>
#else
#  include <streambuf>
#endif
#include <vector>
#include <map>
#include <string>
#include <algorithm>

#if (defined(_MSC_VER) && (_MSC_VER >= 1800)) || __cplusplus > 199711L || defined( __GXX_EXPERIMENTAL_CXX0X__ )
  #include <mutex>
#endif


//== NAMESPACES ===============================================================

namespace OpenMesh {
#ifndef DOXY_IGNORE_THIS


//== CLASS DEFINITION =========================================================


class basic_multiplex_target
{
public:
  virtual ~basic_multiplex_target() {}
  virtual void operator<<(const std::string& _s) = 0;
};


template <class T>
class multiplex_target : public basic_multiplex_target
{
public:
  explicit multiplex_target(T& _t) : target_(_t) {}
  virtual void operator<<(const std::string& _s) { target_ << _s; }
private:
  T& target_;
};



//== CLASS DEFINITION =========================================================


#if defined( OM_CC_GCC ) && OM_CC_VERSION < 30000
#  define STREAMBUF   streambuf
#  define INT_TYPE    int
#  define TRAITS_TYPE 
#else
#  define STREAMBUF std::basic_streambuf<char>
#endif

class multiplex_streambuf : public STREAMBUF
{
public:

  typedef STREAMBUF                   base_type;
#if defined( OM_CC_GCC ) && OM_CC_VERSION < 30000
  typedef int                         int_type;
  struct traits_type
  {
    static int_type eof() { return -1; }
    static char to_char_type(int_type c) { return char(c); }
  };
#else
  typedef base_type::int_type         int_type;
  typedef base_type::traits_type      traits_type;
#endif

  // Constructor
  multiplex_streambuf() : enabled_(true) { buffer_.reserve(100); }

  // Destructor
  ~multiplex_streambuf()
  {
    tmap_iter t_it(target_map_.begin()), t_end(target_map_.end());
    for (; t_it!=t_end; ++t_it)
      delete t_it->second;
  }


  // buffer enable/disable
  bool is_enabled() const { return enabled_; }
  void enable() { enabled_ = true; }
  void disable() { enabled_ = false; }


  // construct multiplex_target<T> and add it to targets
  template <class T> bool connect(T& _target) 
  {
    void* key = (void*) &_target;

    if (target_map_.find(key) != target_map_.end())
      return false;

    target_type*  mtarget = new multiplex_target<T>(_target);
    target_map_[key] = mtarget;

    __connect(mtarget); 
    return true;
  }


  // disconnect target from multiplexer
  template <class T> bool disconnect(T& _target) 
  {
    void*      key  = (void*) &_target;
    tmap_iter  t_it = target_map_.find(key);

    if (t_it != target_map_.end())
    {
      __disconnect(t_it->second);
      target_map_.erase(t_it);
      return true;
    }

    return false;
  }


protected:

  // output what's in buffer_
  virtual int sync() 
  {
    // If working on multiple threads, we need to serialize the output correctly (requires c++11 headers)
    #if (defined(_MSC_VER) && (_MSC_VER >= 1800)) || __cplusplus > 199711L || defined( __GXX_EXPERIMENTAL_CXX0X__ )
       std::lock_guard<std::mutex> lck (serializer_);
    #endif

    if (!buffer_.empty())
    {
      if (enabled_) multiplex();
#if defined( OM_CC_GCC ) && OM_CC_VERSION < 30000
      buffer_ = ""; // member clear() not available!
#else
      buffer_.clear();
#endif
    }
    return base_type::sync();
  }


  // take on char and add it to buffer_
  // if '\n' is encountered, trigger a sync()
  virtual 
  int_type overflow(int_type _c = multiplex_streambuf::traits_type::eof())
  {
    char c = traits_type::to_char_type(_c);

    // If working on multiple threads, we need to serialize the output correctly (requires c++11 headers)
    #if (defined(_MSC_VER) && (_MSC_VER >= 1800)) || __cplusplus > 199711L || defined( __GXX_EXPERIMENTAL_CXX0X__ )
       {
         std::lock_guard<std::mutex> lck (serializer_);
         buffer_.push_back(c);
       }
    #else
      buffer_.push_back(c);
    #endif

    if (c == '\n') sync();
    return 0;
  }


private:

  typedef basic_multiplex_target          target_type;
  typedef std::vector<target_type*>       target_list;
  typedef target_list::iterator           tlist_iter;
  typedef std::map<void*, target_type*>   target_map;
  typedef target_map::iterator            tmap_iter;


  // add _target to list of multiplex targets
  void __connect(target_type* _target) { targets_.push_back(_target); }


  // remove _target from list of multiplex targets
  void __disconnect(target_type* _target) { 
    targets_.erase(std::find(targets_.begin(), targets_.end(), _target));
  }


  // multiplex output of buffer_ to all targets
  void multiplex()
  {
    tlist_iter t_it(targets_.begin()), t_end(targets_.end());
    for (; t_it!=t_end; ++t_it)
      **t_it << buffer_;
  }


private:

  target_list  targets_;
  target_map   target_map_;
  std::string  buffer_;
  bool         enabled_;

  // If working on multiple threads, we need to serialize the output correctly (requires c++11 headers)
  #if (defined(_MSC_VER) && (_MSC_VER >= 1800)) || __cplusplus > 199711L || defined( __GXX_EXPERIMENTAL_CXX0X__ )
     std::mutex serializer_;
  #endif

};

#undef STREAMBUF


//== CLASS DEFINITION =========================================================


/** \class mostream mostream.hh <OpenMesh/Core/System/mostream.hh>

   This class provides streams that can easily be multiplexed (using
   the connect() method) and toggled on/off (using enable() /
   disable()).

    \see omlog, omout, omerr
*/

class mostream : public std::ostream
{
public:

  /// Explicit constructor
  explicit mostream() : std::ostream(NULL) { init(&streambuffer_); }


  /// Connect target to multiplexer
  template <class T> bool connect(T& _target) 
  { 
    return streambuffer_.connect(_target); 
  }


  /// Disconnect target from multiplexer
  template <class T> bool disconnect(T& _target) 
  {
    return streambuffer_.disconnect(_target);
  }


  /// is buffer enabled
  bool is_enabled() const { return streambuffer_.is_enabled(); }

  /// enable this buffer
  void enable() { streambuffer_.enable(); }

  /// disable this buffer
  void disable() { streambuffer_.disable(); }


private:
  multiplex_streambuf  streambuffer_;
};


//=============================================================================
#endif
} // namespace OpenMesh
//=============================================================================
#endif // OPENMESH_MOSTREAM_HH defined
//=============================================================================
