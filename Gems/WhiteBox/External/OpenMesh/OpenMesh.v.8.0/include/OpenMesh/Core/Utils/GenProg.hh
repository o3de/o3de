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
//  Utils for generic/generative programming
//
//=============================================================================

#ifndef OPENMESH_GENPROG_HH
#define OPENMESH_GENPROG_HH


//== INCLUDES =================================================================

#include <OpenMesh/Core/System/config.h>


//== NAMESPACES ===============================================================

namespace OpenMesh {

namespace GenProg  {
#ifndef DOXY_IGNORE_THIS

//== IMPLEMENTATION ===========================================================


/// This type maps \c true or \c false to different types.
template <bool b> struct Bool2Type { enum { my_bool = b }; };

/// This class generates different types from different \c int 's.
template <int i>  struct Int2Type  { enum { my_int = i }; };

/// Handy typedef for Bool2Type<true> classes
typedef Bool2Type<true> TrueType;

/// Handy typedef for Bool2Type<false> classes
typedef Bool2Type<false> FalseType;

//-----------------------------------------------------------------------------
/// compile time assertions 
template <bool Expr> struct AssertCompile;
template <> struct AssertCompile<true> {};



//--- Template "if" w/ partial specialization ---------------------------------
#if OM_PARTIAL_SPECIALIZATION


template <bool condition, class Then, class Else>
struct IF { typedef Then Result; };

/** Template \c IF w/ partial specialization
\code
typedef IF<bool, Then, Else>::Result  ResultType;
\endcode    
*/
template <class Then, class Else>
struct IF<false, Then, Else> { typedef Else Result; };





//--- Template "if" w/o partial specialization --------------------------------
#else


struct SelectThen 
{
  template <class Then, class Else> struct Select {
    typedef Then Result;
  };
};

struct SelectElse
{
  template <class Then, class Else> struct Select {
    typedef Else Result;
  };
};

template <bool condition> struct ChooseSelector {
  typedef SelectThen Result;
};

template <> struct ChooseSelector<false> {
  typedef SelectElse Result;
};


/** Template \c IF w/o partial specialization. Use it like
\code
typedef IF<bool, Then, Else>::Result  ResultType;
\endcode    
*/

template <bool condition, class Then, class Else>
class IF 
{ 
  typedef typename ChooseSelector<condition>::Result  Selector;
public:
  typedef typename Selector::template Select<Then, Else>::Result  Result;
};

#endif

//=============================================================================
#endif
} // namespace GenProg
} // namespace OpenMesh

#define assert_compile(EXPR)                GenProg::AssertCompile<(EXPR)>();

//=============================================================================
#endif // OPENMESH_GENPROG_HH defined
//=============================================================================
