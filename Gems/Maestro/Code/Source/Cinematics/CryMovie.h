/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.


// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the CRYMOVIE_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// CRYMOVIE_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.

#ifndef CRYINCLUDE_CRYMOVIE_CRYMOVIE_H
#define CRYINCLUDE_CRYMOVIE_CRYMOVIE_H
#pragma once

#ifdef CRYMOVIE_EXPORTS
    #define CRYMOVIE_API DLL_EXPORT
#else
    #define CRYMOVIE_API DLL_IMPORT
#endif

struct ISystem;
struct IMovieSystem;

extern "C"
{
CRYMOVIE_API IMovieSystem* CreateMovieSystem(ISystem* pSystem);
CRYMOVIE_API void DeleteMovieSystem(IMovieSystem* pMM);
}
#endif // CRYINCLUDE_CRYMOVIE_CRYMOVIE_H
