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

#ifndef CRYINCLUDE_CRY3DENGINE_CCULLTHREAD_H
#define CRYINCLUDE_CRY3DENGINE_CCULLTHREAD_H
#pragma once

#include "CryThread.h"
#include <AzCore/Jobs/LegacyJobExecutor.h>

namespace NAsyncCull
{
    class CCullThread
        : public Cry3DEngineBase
    {
        bool m_Enabled;
        bool m_Active; // used to verify that the cull job is running and no new jobs are added after the job has finished

    public:
        enum PrepareStateT
        {
            IDLE, PREPARE_STARTED, PREPARE_DONE, CHECK_REQUESTED, CHECK_STARTED
        };

        PrepareStateT         m_nPrepareState;
        CryCriticalSection    m_FollowUpLock;
        char                  m_passInfoForCheckOcclusion[sizeof(SRenderingPassInfo)];
        uint32                m_nRunningReprojJobs;
        uint32                m_nRunningReprojJobsAfterMerge;
        int                   m_bCheckOcclusionRequested;

    private:
        AZ::LegacyJobExecutor m_OcclusionJobExecutor; // All jobs pushed against this instance to gurantee a wait on all jobs before exiting ~CCullThread
        AZ::LegacyJobExecutor m_PrepareBufferSync;
        Matrix44A             m_MatScreenViewProj _ALIGN(16);
        Matrix44A             m_MatScreenViewProjTransposed;
        Vec3                  m_ViewDir;
        Vec3                  m_Position;
        float                 m_NearPlane;
        float                 m_FarPlane;
        float                 m_NearestMax;

        PodArray<uint8>       m_OCMBuffer;
        uint8*                m_pOCMBufferAligned;
        uint32                m_OCMMeshCount;
        uint32                m_OCMInstCount;
        uint32                m_OCMOffsetInstances;

        template<class T>
        T Swap(T& rData)
        {
            // #if IS_LOCAL_MACHINE_BIG_ENDIAN
            PREFAST_SUPPRESS_WARNING(6326)
            switch (sizeof(T))
            {
            case 1:
                break;
            case 2:
                SwapEndianBase(reinterpret_cast<uint16*>(&rData));
                break;
            case 4:
                SwapEndianBase(reinterpret_cast<uint32*>(&rData));
                break;
            case 8:
                SwapEndianBase(reinterpret_cast<uint64*>(&rData));
                break;
            default:
#if defined(__clang__) || defined(__GNUC__)
                __builtin_unreachable();
#else
                __assume(0);
#endif
            }
            //#endif
            return rData;
        }

        void RasterizeZBuffer(uint32 PolyLimit);
        void OutputMeshList();

    public:

        void CheckOcclusion(SRenderingPassInfo passInfo);
        void PrepareOcclusion();

        void PrepareOcclusion_RasterizeZBuffer();
        void PrepareOcclusion_ReprojectZBuffer();
        void PrepareOcclusion_ReprojectZBufferLine(int nStartLine, int nNumLines);
        void PrepareOcclusion_ReprojectZBufferLineAfterMerge(int nStartLine, int nNumLines);
        void Init();

        bool LoadLevel(const char* pFolderName);
        void UnloadLevel();

        bool TestAABB(const AABB& rAABB, float fEntDistance, float fVerticalExpand = 0.0f);
        bool TestQuad(const Vec3& vCenter, const Vec3& vAxisX, const Vec3& vAxisY);

        CCullThread();
        ~CCullThread() = default;


#ifndef _RELEASE
        void CoverageBufferDebugDraw();
#endif

        void PrepareCullbufferAsync(const CCamera& rCamera);
        void CullStart(const SRenderingPassInfo& passInfo);
        void CullEnd(bool waitForOcclusionJobCompletion = false);

        bool IsActive() const { return m_Active; }
        void SetActive(bool bActive) { m_Active = bActive; }

        Vec3 GetViewDir() { return m_ViewDir; };
    } _ALIGN(128);
}

#endif // CRYINCLUDE_CRY3DENGINE_CCULLTHREAD_H

