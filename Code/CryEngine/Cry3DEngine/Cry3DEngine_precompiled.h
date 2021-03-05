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

#pragma once

const int nThreadsNum = 3;

#include <platform.h>

#include <vector>
//#define DEFINE_MODULE_NAME "Cry3DEngine"
//#define FORCE_STANDARD_ASSERT // fix edit and continue

#if defined(WIN64)
#define CRY_INTEGRATE_DX12
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
// Highlevel defines

// deferred cull queue handling - currently disabled
// #define USE_CULL_QUEUE

// Compilation (Export to Engine) not needed on consoles
#if defined(CONSOLE)
# define ENGINE_ENABLE_COMPILATION 0
#else
# define ENGINE_ENABLE_COMPILATION 1
#endif

#include <stdio.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Casting/lossy_cast.h>

#define MAX_PATH_LENGTH 512

#include <ITimer.h>
#include <IProcess.h>
#include <Cry_Math.h>
#include <Cry_Camera.h>
#include <Cry_XOptimise.h>
#include <Cry_Geo.h>
#include <ILog.h>
#include <ISystem.h>
#include <IConsole.h>
#include <IPhysics.h>
#include <IRenderer.h>
#include <IRenderAuxGeom.h>
#include <IEntityRenderState.h>
#include <StackContainer.h>
#include <I3DEngine.h>
#include <CryFile.h>
#include <smartptr.h>
#include <CryArray.h>
#include <CryHeaders.h>
#include "Cry3DEngineBase.h"
#include <float.h>
#include "CryArray.h"
#include "cvars.h"
#include <CrySizer.h>
#include <StlUtils.h>
#include <CryArray2d.h>
#include "Material.h"
#include "3dEngine.h"
#include "ObjMan.h"

#include <ISerialize.h>
#include "BasicArea.h"
#include "Environment/OceanEnvironmentBus.h"

#include "ObjectsTree.h"

inline int snprintf(char* buf, int size, const char* format, ...)
{
    va_list arglist;
    va_start(arglist, format);
    int res = azvsnprintf(buf, size, format, arglist);
    va_end(arglist);
    return res;
}

template <class T>
void AddToPtr(byte*& pPtr, T& rObj, EEndian eEndian)
{
    PREFAST_SUPPRESS_WARNING(6326) COMPILE_TIME_ASSERT(((sizeof(T) % 4) == 0));
    assert(!((INT_PTR)pPtr & 3));
    memcpy(pPtr, &rObj, sizeof(rObj));
    SwapEndian(*(T*)pPtr, eEndian);
    pPtr += sizeof(rObj);
    assert(!((INT_PTR)pPtr & 3));
}

template <class T>
void AddToPtr(byte*& pPtr, int& nDataSize, T& rObj, EEndian eEndian)
{
    PREFAST_SUPPRESS_WARNING(6326) COMPILE_TIME_ASSERT(((sizeof(T) % 4) == 0));
    assert(!((INT_PTR)pPtr & 3));
    memcpy(pPtr, &rObj, sizeof(rObj));
    SwapEndian(*(T*)pPtr, eEndian);
    pPtr += sizeof(rObj);
    nDataSize -= sizeof(rObj);
    assert(nDataSize >= 0);
    assert(!((INT_PTR)pPtr & 3));
}


inline void FixAlignment(byte*& pPtr, int& nDataSize)
{
    while ((UINT_PTR)pPtr & 3)
    {
        *pPtr = 222;
        pPtr++;
        nDataSize--;
    }
}

inline void FixAlignment(byte*& pPtr)
{
    while ((UINT_PTR)pPtr & 3)
    {
        *pPtr = 222;
        pPtr++;
    }
}

template <class T>
void AddToPtr(byte*& pPtr, int& nDataSize, const T* pArray, int nElemNum, EEndian eEndian, bool bFixAlignment = false)
{
    assert(!((INT_PTR)pPtr & 3));
    memcpy(pPtr, pArray, nElemNum * sizeof(T));
    SwapEndian((T*)pPtr, nElemNum, eEndian);
    pPtr += nElemNum * sizeof(T);
    nDataSize -= nElemNum * sizeof(T);
    assert(nDataSize >= 0);

    if (bFixAlignment)
    {
        FixAlignment(pPtr, nDataSize);
    }
    else
    {
        assert(!((INT_PTR)pPtr & 3));
    }
}

template <class T>
void AddToPtr(byte*& pPtr, const T* pArray, int nElemNum, EEndian eEndian, bool bFixAlignment = false)
{
    assert(!((INT_PTR)pPtr & 3));
    memcpy(pPtr, pArray, nElemNum * sizeof(T));
    SwapEndian((T*)pPtr, nElemNum, eEndian);
    pPtr += nElemNum * sizeof(T);

    if (bFixAlignment)
    {
        FixAlignment(pPtr);
    }
    else
    {
        assert(!((INT_PTR)pPtr & 3));
    }
}

struct TriangleIndex
{
    TriangleIndex() { ZeroStruct(*this); }
    uint16& operator [] (const int& n) { assert(n >= 0 && n < 3); return idx[n]; }
    const uint16& operator [] (const int& n) const { assert(n >= 0 && n < 3); return idx[n]; }
    uint16 idx[3];
    uint16 nCull;
};

    #define FUNCTION_PROFILER_3DENGINE FUNCTION_PROFILER(gEnv->pSystem, PROFILE_3DENGINE)
    #define FUNCTION_PROFILER_3DENGINE_LEGACYONLY FUNCTION_PROFILER_LEGACYONLY(gEnv->pSystem, PROFILE_3DENGINE)


