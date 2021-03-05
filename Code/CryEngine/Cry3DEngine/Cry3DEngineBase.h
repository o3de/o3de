
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

// Description : Access to external stuff used by 3d engine. Most 3d engine classes
//               are derived from this base class to access other interfaces


#ifndef CRYINCLUDE_CRY3DENGINE_CRY3DENGINEBASE_H
#define CRYINCLUDE_CRY3DENGINE_CRY3DENGINEBASE_H
#pragma once

#include "3DEngineMemory.h"

struct ISystem;
struct IRenderer;
struct ILog;
struct ITimer;
struct IConsole;
struct I3DEngine;
struct IObjManager;
struct CVars;
struct CVisAreaManager;

class COcean;
class C3DEngine;
class CParticleManager;
class CDecalManager;
class CRainManager;
class CCloudsManager;
class CSkyLightManager;
class CRenderMeshMerger;
class CMergedMeshesManager;
class CGeomCacheManager;
class CBreezeGenerator;
class CMatMan;
class CClipVolumeManager;

namespace AZ::IO
{
    struct IArchive;
}

#define DISTANCE_TO_THE_SUN 1000000

#if !defined(_RELEASE)
#define OBJMAN_STREAM_STATS
#endif

struct Cry3DEngineBase
{
    static ISystem* m_pSystem;
    static IRenderer* m_pRenderer;
    static ITimer* m_pTimer;
    static ILog* m_pLog;
    static ::IConsole* m_pConsole;
    static C3DEngine* m_p3DEngine;
    static CVars* m_pCVars;
    static AZ::IO::IArchive* m_pCryPak;
    static CObjManager* m_pObjManager;
    static COcean* m_pOcean;
    static IOpticsManager* m_pOpticsManager;
    static CDecalManager* m_pDecalManager;
    static CCloudsManager* m_pCloudsManager;
    static CVisAreaManager* m_pVisAreaManager;
    static CClipVolumeManager* m_pClipVolumeManager;
    static CMatMan* m_pMatMan;
    static CSkyLightManager* m_pSkyLightManager;
    static CRenderMeshMerger* m_pRenderMeshMerger;
    static IStreamedObjectListener* m_pStreamListener;
#if defined(USE_GEOM_CACHES)
    static CGeomCacheManager* m_pGeomCacheManager;
#endif

    static float m_fInvDissolveDistBand;
    static threadID m_nMainThreadId;
    static bool m_bLevelLoadingInProgress;
    static bool m_bIsInRenderScene;
    static bool m_bAsyncOctreeUpdates;
    static bool m_bRenderTypeEnabled[eERType_TypesNum];

    static int m_CpuFlags;
    static ESystemConfigSpec m_LightConfigSpec;
#if defined(CONSOLE)
    static const bool m_bEditor = false;
#else
    static bool m_bEditor;
#endif
    static int m_arrInstancesCounter[eERType_TypesNum];

    // components access
    ILINE static ISystem* GetSystem() { return m_pSystem; }
    ILINE static IRenderer* GetRenderer() { return m_pRenderer; }
    ILINE static ITimer* GetTimer() { return m_pTimer; }
    ILINE static ILog* GetLog() { return m_pLog; }

    inline static ::IConsole* GetConsole() { return m_pConsole; }
    inline static C3DEngine* Get3DEngine() { return m_p3DEngine; }
    inline static CObjManager* GetObjManager() { return m_pObjManager; };

    inline static COcean* GetOcean() { return m_pOcean; };
    inline static CVars* GetCVars() { return m_pCVars; }
    inline static CVisAreaManager* GetVisAreaManager() { return m_pVisAreaManager; }
    inline static AZ::IO::IArchive* GetPak() { return m_pCryPak; }
    inline static CMatMan* GetMatMan() { return m_pMatMan; }
    inline static CCloudsManager* GetCloudsManager() { return m_pCloudsManager; }
    inline static CRenderMeshMerger* GetSharedRenderMeshMerger() { return m_pRenderMeshMerger; };
    inline static CTemporaryPool* GetTemporaryPool() { return CTemporaryPool::Get(); };

#if defined(USE_GEOM_CACHES)
    inline static CGeomCacheManager* GetGeomCacheManager() { return m_pGeomCacheManager; };
#endif

    ILINE static bool                               IsRenderNodeTypeEnabled(EERType rnType) { return m_bRenderTypeEnabled[(int)rnType]; }
    ILINE static void                               SetRenderNodeTypeEnabled(EERType rnType, bool bEnabled) {m_bRenderTypeEnabled[(int)rnType] = bEnabled; }

    inline static int GetDefSID() { return DEFAULT_SID; };

    float GetCurTimeSec();
    float GetCurAsyncTimeSec();

    static void PrintMessage(const char* szText, ...) PRINTF_PARAMS(1, 2);
    static void PrintMessagePlus(const char* szText, ...) PRINTF_PARAMS(1, 2);
    static void PrintComment(const char* szText, ...) PRINTF_PARAMS(1, 2);

    // Validator warning.
    static void Warning(const char* format, ...) PRINTF_PARAMS(1, 2);
    static void Error(const char* format, ...) PRINTF_PARAMS(1, 2);
    static void FileWarning(int flags, const char* file, const char* format, ...)
    PRINTF_PARAMS(3, 4);

    CRenderObject* GetIdentityCRenderObject(int nThreadID)
    {
        CRenderObject* pCRenderObject = GetRenderer()->EF_GetObject_Temp(nThreadID);
        if (!pCRenderObject)
        {
            return NULL;
        }
        pCRenderObject->m_II.m_Matrix.SetIdentity();
        return pCRenderObject;
    }

    static bool IsValidFile(const char* sFilename);
    static bool IsResourceLocked(const char* sFilename);

    static bool IsPreloadEnabled();

    _smart_ptr<IMaterial> MakeSystemMaterialFromShader(const char* sShaderName, SInputShaderResources* Res = NULL);
    void DrawBBoxLabeled(const AABB& aabb, const Matrix34& m34, const ColorB& col, const char* format, ...) PRINTF_PARAMS(5, 6);
    void DrawBBox(const Vec3& vMin, const Vec3& vMax, ColorB col = Col_White);
    void DrawBBox(const AABB& box, ColorB col = Col_White);
    void DrawLine(const Vec3& vMin, const Vec3& vMax, ColorB col = Col_White);
    void DrawSphere(const Vec3& vPos, float fRadius, ColorB color = ColorB(255, 255, 255, 255));
    void DrawQuad(const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& v3, ColorB color);

    int& GetInstCount(EERType eType) { return m_arrInstancesCounter[eType]; }

    uint32 GetMinSpecFromRenderNodeFlags(uint32 dwRndFlags) const { return (dwRndFlags & ERF_SPEC_BITS_MASK) >> ERF_SPEC_BITS_SHIFT; }
    static bool CheckMinSpec(uint32 nMinSpec);

    static bool IsEscapePressed();

    size_t fread(
        void* buffer,
        size_t elementSize,
        size_t count,
        FILE* stream)
    {
        size_t res = ::fread(buffer, elementSize, count, stream);
        if (res != count)
        {
            Error("fread() failed");
        }
        return res;
    }

    int fseek (
        FILE* stream,
        long offset,
        int whence
        )
    {
        int res = ::fseek(stream, offset, whence);
        if (res != 0)
        {
            Error("fseek() failed");
        }
        return res;
    }
};

#endif // CRYINCLUDE_CRY3DENGINE_CRY3DENGINEBASE_H
