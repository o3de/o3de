/***************************************************************************
 * Copyright 1998-2018 by authors (see AUTHORS.txt)                        *
 *                                                                         *
 *   This file is part of LuxCoreRender.                                   *
 *                                                                         *
 * Licensed under the Apache License, Version 2.0 (the "License");         *
 * you may not use this file except in compliance with the License.        *
 * You may obtain a copy of the License at                                 *
 *                                                                         *
 *     http://www.apache.org/licenses/LICENSE-2.0                          *
 *                                                                         *
 * Unless required by applicable law or agreed to in writing, software     *
 * distributed under the License is distributed on an "AS IS" BASIS,       *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
 * See the License for the specific language governing permissions and     *
 * limitations under the License.                                          *
 ***************************************************************************/

// NOTE: this file is included in LuxCore so any external dependency must be
// avoided here

#ifndef _LUXRAYSEXPORT_DEFS_H
#define _LUXRAYSEXPORT_DEFS_H

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the CPP_API_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// CPP_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef WIN32
#ifdef LUXCORE_DLL
 #ifdef CPP_API_EXPORTS
  #define CPP_API __declspec(dllexport)
 #else
  #define CPP_API __declspec(dllimport)
 #endif
#else
 #define CPP_API
#endif
#else // linux/osx
 #ifdef CPP_API_EXPORTS
  #define CPP_API __attribute__ ((visibility ("default")))
 #else
  #define CPP_API
 #endif
#endif

#if (defined(__apple_build_version__) && ((__clang_major__ == 5) && (__clang_minor__ >= 1) || ((__clang_major__ == 6) && (__clang_minor__ < 1))))
 #define CPP_EXPORT extern "C"
#else
 #define CPP_EXPORT extern "C++"
#endif

#endif // _LUXRAYSEXPORT_DEFS_H
