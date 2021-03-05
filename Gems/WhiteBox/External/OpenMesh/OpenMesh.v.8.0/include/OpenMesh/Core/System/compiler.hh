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




#ifndef OPENMESH_COMPILER_H
#define OPENMESH_COMPILER_H

//=============================================================================

#if defined(ACGMAKE_STATIC_BUILD)
#  define OM_STATIC_BUILD 1
#endif

//=============================================================================

#if defined(_DEBUG) || defined(DEBUG)
#  define OM_DEBUG
#endif

//=============================================================================

// Workaround for Intel Compiler with MS VC++ 6
#if defined(_MSC_VER) && \
   ( defined(__ICL) || defined(__INTEL_COMPILER) || defined(__ICC) )
#  if !defined(__INTEL_COMPILER)
#    define __INTEL_COMPILER __ICL
#  endif
#  define OM_USE_INTEL_COMPILER 1
#endif

// --------------------------------------------------------- MS Visual C++ ----
// Compiler _MSC_VER
// .NET 2002 1300 
// .NET 2003 1310
// .NET 2005 1400
#if defined(_MSC_VER) && !defined(OM_USE_INTEL_COMPILER)
#  if (_MSC_VER == 1300)
#    define OM_CC_MSVC
#    define OM_TYPENAME
#    define OM_OUT_OF_CLASS_TEMPLATE       0
#    define OM_PARTIAL_SPECIALIZATION      0
#    define OM_INCLUDE_TEMPLATES           1
#  elif (_MSC_VER == 1310)
#    define OM_CC_MSVC
#    define OM_TYPENAME
#    define OM_OUT_OF_CLASS_TEMPLATE       1
#    define OM_PARTIAL_SPECIALIZATION      1
#    define OM_INCLUDE_TEMPLATES           1
#  elif (_MSC_VER >= 1400) // settings for .NET 2005 (NOTE: not fully tested)
#    define OM_TYPENAME
#    define OM_OUT_OF_CLASS_TEMPLATE       1
#    define OM_PARTIAL_SPECIALIZATION      1
#    define OM_INCLUDE_TEMPLATES           1
#  else
#    error "Version 7 (.NET 2002) or higher of the MS VC++ is required!"
#  endif
//   currently no windows dll supported
#  define OM_STATIC_BUILD 1
#  if defined(_MT)
#    define OM_REENTRANT 1
#  endif
#  define OM_CC "MSVC++"
#  define OM_CC_VERSION _MSC_VER
// Does not work stable because the define _CPPRTTI sometimes does not exist,
// though the option /GR is set!? 
#  if defined(__cplusplus) && !defined(_CPPRTTI)
#    error "Enable Runtime Type Information (Compiler Option /GR)!"
#  endif
#  if !defined(_USE_MATH_DEFINES)
#    error "You have to define _USE_MATH_DEFINES in the compiler settings!"
#  endif
// ------------------------------------------------------------- Borland C ----
#elif defined(__BORLANDC__)
#  error "Borland Compiler are not supported yet!"
// ------------------------------------------------------------- GNU C/C++ ----
#elif defined(__GNUC__) && !defined(__ICC)
#  define OM_CC_GCC
#  define OM_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 )
#  define OM_GCC_MAJOR                __GNUC__
#  define OM_GCC_MINOR                __GNUC_MINOR__
#  if (OM_GCC_VERSION >= 30200)
#    define OM_TYPENAME typename
#    define OM_OUT_OF_CLASS_TEMPLATE  1
#    define OM_PARTIAL_SPECIALIZATION 1
#    define OM_INCLUDE_TEMPLATES      1
#  else
#    error "Version 3.2.0 or better of the GNU Compiler is required!"
#  endif
#  if defined(_REENTRANT)
#    define OM_REENTRANT 1
#  endif
#  define OM_CC "GCC"
#  define OM_CC_VERSION OM_GCC_VERSION
// ------------------------------------------------------------- Intel icc ----
#elif defined(__ICC) || defined(__INTEL_COMPILER)
#  define OM_CC_ICC
#  define OM_TYPENAME typename
#  define OM_OUT_OF_CLASS_TEMPLATE  1
#  define OM_PARTIAL_SPECIALIZATION 1
#  define OM_INCLUDE_TEMPLATES      1
#  if defined(_REENTRANT) || defined(_MT)
#    define OM_REENTRANT 1
#  endif
#  define OM_CC "ICC"
#  define OM_CC_VERSION __INTEL_COMPILER
//   currently no windows dll supported
#  if defined(_MSC_VER) || defined(WIN32)
#    define OM_STATIC_BUILD 1
#  endif
// ------------------------------------------------------ MIPSpro Compiler ----
#elif defined(__MIPS_ISA) || defined(__mips)
// _MIPS_ISA                    
// _COMPILER_VERSION            e.g. 730, 7 major, 3 minor
// _MIPS_FPSET                  32|64
// _MIPS_SZINT                  32|64
// _MIPS_SZLONG                 32|64
// _MIPS_SZPTR                  32|64
#  define OM_CC_MIPS
#  define OM_TYPENAME typename
#  define OM_OUT_OF_CLASS_TEMPLATE    1
#  define OM_PARTIAL_SPECIALIZATION   1
#  define OM_INCLUDE_TEMPLATES        0
#  define OM_CC "MIPS"
#  define OM_CC_VERSION _COMPILER_VERSION
// ------------------------------------------------------------------ ???? ----
#else
#  error "You're using an unsupported compiler!"
#endif

//=============================================================================
#endif // OPENMESH_COMPILER_H defined
//=============================================================================

