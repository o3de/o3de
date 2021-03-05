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

/*===========================================================================*\
 *                                                                           *             
 *   $Revision$                                                         *
 *   $Date$                   *
 *                                                                           *
\*===========================================================================*/

/** \file config.h
 *  \todo Move content to config.hh and include it to be compatible with old
 *  source.
 */

//=============================================================================

#ifndef OPENMESH_CONFIG_H
#define OPENMESH_CONFIG_H

//=============================================================================

#include <assert.h>
#include <OpenMesh/Core/System/compiler.hh>
#include <OpenMesh/Core/System/OpenMeshDLLMacros.hh>

// ----------------------------------------------------------------------------


#define OM_VERSION 0x80000
//#define OM_VERSION 0x70200

#define OM_GET_VER ((OM_VERSION & 0xf0000) >> 16)
#define OM_GET_MAJ ((OM_VERSION & 0x0ff00) >> 8)
#define OM_GET_MIN  (OM_VERSION & 0x000ff)

#ifdef WIN32
#  ifdef min
#    pragma message("Detected min macro! OpenMesh does not compile with min/max macros active! Please add a define NOMINMAX to your compiler flags or add #undef min before including OpenMesh headers !")
#    error min macro active 
#  endif
#  ifdef max
#    pragma message("Detected max macro! OpenMesh does not compile with min/max macros active! Please add a define NOMINMAX to your compiler flags or add #undef max before including OpenMesh headers !")
#    error max macro active 
#  endif
#endif

#if defined(_MSC_VER)
#  define OM_DEPRECATED(msg) __declspec(deprecated(msg))
#elif defined(__GNUC__)
#  if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) >= 40500 /* Test for GCC >= 4.5.0 */
#    define OM_DEPRECATED(msg) __attribute__ ((deprecated(msg)))
#  else
#    define OM_DEPRECATED(msg) __attribute__ ((deprecated))
#  endif
#elif defined(__clang__)
#  define OM_DEPRECATED(msg) __attribute__ ((deprecated(msg)))
#else
#  define OM_DEPRECATED(msg)
#endif

typedef unsigned int uint;

#if ((defined(_MSC_VER) && (_MSC_VER >= 1800)) || __cplusplus > 199711L || defined(__GXX_EXPERIMENTAL_CXX0X__))
#define OM_HAS_HASH
#endif

//=============================================================================
#endif // OPENMESH_CONFIG_H defined
//=============================================================================
