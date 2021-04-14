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

#include "Cry3DEngine_precompiled.h"
#include "CCullThread.h"
#include "ObjMan.h"
#include "CCullRenderer.h"
#include <AzCore/Jobs/Job.h>
#include <AzCore/Jobs/JobFunction.h>

typedef NAsyncCull::CCullRenderer<CULL_SIZEX, CULL_SIZEY>    tdCullRasterizer;

volatile static                     NAsyncCull::tdVertexCache g_VertexCache;
uint8                           g_RasterizerBuffer[sizeof(tdCullRasterizer) + 16];
tdCullRasterizer*   g_Rasterizer;
#define RASTERIZER (*g_Rasterizer)

namespace NAsyncCull
{
    const NVMath::vec4 MaskNot3 =   NVMath::Vec4(~3u, ~0u, ~0u, ~0u);

    CCullThread::CCullThread()
        : m_Enabled(false)
        , m_Active(false)
        , m_nPrepareState(IDLE)
        , m_OCMMeshCount(0)
        , m_OCMInstCount(0)
        , m_OCMOffsetInstances(0)
    {
        size_t Buffer   =   reinterpret_cast<size_t>(g_RasterizerBuffer);
        Buffer  +=  127;
        Buffer  &=  ~127;
        g_Rasterizer        =   new(reinterpret_cast<void*>(Buffer))tdCullRasterizer();

        m_NearPlane = 0;
        m_FarPlane = 0;
        m_NearestMax = 0;
    }

    bool CCullThread::LoadLevel(const char* pFolderName)
    {
        m_OCMBuffer.resize(0);
        AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen((string(pFolderName) + "/occluder.ocm").c_str(), "rbx");
        if (fileHandle == AZ::IO::InvalidHandle)
        {
            //__debugbreak();
            return false;
        }
        gEnv->pCryPak->FSeek(fileHandle, 0, SEEK_END);
        const size_t Size = gEnv->pCryPak->FTell(fileHandle);
        gEnv->pCryPak->FSeek(fileHandle, 0L, SEEK_SET);
        m_OCMBuffer.reserve(Size + 144 * 3 + 16); //48tri*9byte padding for unrolled loop in rasterization without special case (not 144 algined poly count)
        //16 for alignment
        m_OCMBuffer.resize(Size);
        size_t  BufferOffset    =   reinterpret_cast<size_t>(&m_OCMBuffer[0]);
        BufferOffset    =   (BufferOffset + 15) & ~15;
        m_pOCMBufferAligned     =   reinterpret_cast<uint8*>(BufferOffset);

        gEnv->pCryPak->FRead(m_pOCMBufferAligned, Size, fileHandle, false);
        gEnv->pCryPak->FClose(fileHandle);

        const uint32 Version    =   Swap(*reinterpret_cast<uint32*>(&m_pOCMBufferAligned[0]));
        m_OCMMeshCount              =   *reinterpret_cast<uint32*>(&m_pOCMBufferAligned[4]);
        m_OCMInstCount              =   *reinterpret_cast<uint32*>(&m_pOCMBufferAligned[8]);
        m_OCMOffsetInstances    =   *reinterpret_cast<uint32*>(&m_pOCMBufferAligned[12]);

        if (Version != ~3u && Version != ~4u)
        {
            CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_ERROR, "Unsupported occlusion mesh format version. Please reexport the occluder mesh.");
            stl::free_container(m_OCMBuffer);
            return false;
        }

        if (m_OCMOffsetInstances & 3)
        {
            CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_ERROR, "The occluder mesh contains invalid data. Please reexport the occluder mesh.");
            stl::free_container(m_OCMBuffer);
            return false;
        }

        if (Version == ~3u) //bump to version ~4
        {
            m_OCMMeshCount              =   Swap(m_OCMMeshCount);
            m_OCMInstCount              =   Swap(m_OCMInstCount);
            m_OCMOffsetInstances    =   Swap(m_OCMOffsetInstances);
            PodArray<uint8> OCMBufferOut(Size * 8, Size * 8);
            uint8* pOut = &OCMBufferOut[0];
            *reinterpret_cast<uint32*>(&pOut[0])    =   ~4u;//version
            *reinterpret_cast<uint32*>(&pOut[4])    =   m_OCMMeshCount;
            *reinterpret_cast<uint32*>(&pOut[8])    =   m_OCMInstCount;
            *reinterpret_cast<uint32*>(&pOut[12])   =   m_OCMOffsetInstances;//needs to be patched at the end
            pOut += 16;
            uint8* pMeshes  =   &m_pOCMBufferAligned[0];//actually starts at 16, but MeshOffset is zero based
            uint8* pInstances   =   &m_pOCMBufferAligned[m_OCMOffsetInstances];
            std::map<uint32, uint32>  Offsets;//<old Offset, new Offset>
            for (size_t a = 0; a < m_OCMInstCount; a++)
            {
                Matrix44 World(IDENTITY);
                uint8*  pInstance   =   pInstances + a * (sizeof(int) + 12 * sizeof(float));//meshoffset+worldmatrix43
                uint32& MeshOffset = *reinterpret_cast<uint32*>(&pInstance[0]);
                float*  pWorldMat   =   reinterpret_cast<float*>(&pInstance[4]);
                Swap(MeshOffset);
                Swap(pWorldMat[0x0]);
                Swap(pWorldMat[0x1]);
                Swap(pWorldMat[0x2]);
                Swap(pWorldMat[0x3]);
                Swap(pWorldMat[0x4]);
                Swap(pWorldMat[0x5]);
                Swap(pWorldMat[0x6]);
                Swap(pWorldMat[0x7]);
                Swap(pWorldMat[0x8]);
                Swap(pWorldMat[0x9]);
                Swap(pWorldMat[0xA]);
                Swap(pWorldMat[0xB]);

                if (Offsets.find(MeshOffset) != Offsets.end())//already endian swapped?
                {
                    continue;
                }
                Offsets[MeshOffset] =   static_cast<uint32>(pOut - &OCMBufferOut[0]);//zero based offset

                uint8*  pMesh           =   pMeshes + MeshOffset;
                uint16& QuadCount   =   *reinterpret_cast<uint16*>(pMesh);
                uint16& TriCount    =   *reinterpret_cast<uint16*>(pMesh + 2);
                Swap(QuadCount);
                Swap(TriCount);
                *reinterpret_cast<uint32*>(pOut)    =   TriCount + QuadCount / 4 * 6;
                pOut += 16;//to keep 16byte alignment

                const size_t    Quads16     =   (reinterpret_cast<size_t>(pMesh + 4) + 15) & ~15;
                const size_t    Tris16      =   (Quads16 + QuadCount * 3 + 15) & ~15;
                const int8* pQuads          =   reinterpret_cast<const int8*>(Quads16);
                const int8* pTris               =   reinterpret_cast<const int8*>(Tris16);

                for (size_t b = 0, S = QuadCount; b < S; b += 4)
                {
                    const float x0 = *pQuads++;
                    const float y0 = *pQuads++;
                    const float z0 = *pQuads++;
                    const float x1 = *pQuads++;
                    const float y1 = *pQuads++;
                    const float z1 = *pQuads++;
                    const float x2 = *pQuads++;
                    const float y2 = *pQuads++;
                    const float z2 = *pQuads++;
                    const float x3 = *pQuads++;
                    const float y3 = *pQuads++;
                    const float z3 = *pQuads++;
                    reinterpret_cast<float*>(pOut)[0x00]    =   x0;
                    reinterpret_cast<float*>(pOut)[0x01]    =   y0;
                    reinterpret_cast<float*>(pOut)[0x02]    =   z0;
                    reinterpret_cast<float*>(pOut)[0x03]    =   1.f;
                    reinterpret_cast<float*>(pOut)[0x04]    =   x2;
                    reinterpret_cast<float*>(pOut)[0x05]    =   y2;
                    reinterpret_cast<float*>(pOut)[0x06]    =   z2;
                    reinterpret_cast<float*>(pOut)[0x07]    =   1.f;
                    reinterpret_cast<float*>(pOut)[0x08]    =   x3;
                    reinterpret_cast<float*>(pOut)[0x09]    =   y3;
                    reinterpret_cast<float*>(pOut)[0x0a]    =   z3;
                    reinterpret_cast<float*>(pOut)[0x0b]    =   1.f;

                    reinterpret_cast<float*>(pOut)[0x0c]    =   x2;
                    reinterpret_cast<float*>(pOut)[0x0d]    =   y2;
                    reinterpret_cast<float*>(pOut)[0x0e]    =   z2;
                    reinterpret_cast<float*>(pOut)[0x0f]    =   1.f;
                    reinterpret_cast<float*>(pOut)[0x10]    =   x0;
                    reinterpret_cast<float*>(pOut)[0x11]    =   y0;
                    reinterpret_cast<float*>(pOut)[0x12]    =   z0;
                    reinterpret_cast<float*>(pOut)[0x13]    =   1.f;
                    reinterpret_cast<float*>(pOut)[0x14]    =   x1;
                    reinterpret_cast<float*>(pOut)[0x15]    =   y1;
                    reinterpret_cast<float*>(pOut)[0x16]    =   z1;
                    reinterpret_cast<float*>(pOut)[0x17]    =   1.f;
                    pOut += 0x18 * sizeof(float);
                }
                for (size_t c = 0, S = TriCount; c < S; c++)
                {
                    const float x = *pTris++;
                    const float y = *pTris++;
                    const float z = *pTris++;
                    reinterpret_cast<float*>(pOut)[0x00]    =   x;
                    reinterpret_cast<float*>(pOut)[0x01]    =   y;
                    reinterpret_cast<float*>(pOut)[0x02]    =   z;
                    reinterpret_cast<float*>(pOut)[0x03]    =   1.f;
                    pOut += 4 * sizeof(float);
                }
            }
            m_OCMOffsetInstances    =   static_cast<uint32>(pOut - &OCMBufferOut[0]);
            const size_t InstanceSize   =   m_OCMInstCount * (sizeof(int) + 12 * sizeof(float));
            memcpy(pOut, pInstances, InstanceSize);
            for (size_t a = 0; a < m_OCMInstCount; a++)
            {
                uint8*  pInstance   =   pOut + a * (sizeof(int) + 12 * sizeof(float));//meshoffset+worldmatrix43
                uint32& MeshOffset = *reinterpret_cast<uint32*>(&pInstance[0]);
                MeshOffset  =   Offsets[MeshOffset];
            }
            pOut += InstanceSize;

            m_OCMBuffer.resize(pOut - &OCMBufferOut[0]);
            size_t  bufferOffset    =   reinterpret_cast<size_t>(&m_OCMBuffer[0]);
            bufferOffset    =   (bufferOffset + 15) & ~15;
            m_pOCMBufferAligned     =   reinterpret_cast<uint8*>(bufferOffset);
            memcpy(m_pOCMBufferAligned, &OCMBufferOut[0], m_OCMBuffer.size());
        }

        // Integrity check: each mesh data must be aligned to 4 bytes
        uint8* pInstances   =   &m_pOCMBufferAligned[m_OCMOffsetInstances];
        for (size_t a = 0; a < m_OCMInstCount; a++)
        {
            uint8*  pInstance   =   pInstances + a * (sizeof(int) + 12 * sizeof(float));//meshoffset+worldmatrix43
            uint32  MeshOffset = *reinterpret_cast<uint32*>(&pInstance[0]);
            if (MeshOffset & 3)
            {
                CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_ERROR, "The occluder mesh contains invalid data. Please reexport the occluder mesh.");
                stl::free_container(m_OCMBuffer);
                return false;
            }
        }

        return true;
    }

    void CCullThread::UnloadLevel()
    {
        stl::free_container(m_OCMBuffer);
        m_pOCMBufferAligned     =   NULL;

        m_OCMMeshCount              =   0;
        m_OCMInstCount              =   0;
        m_OCMOffsetInstances    =   0;
    }

    void CCullThread::PrepareCullbufferAsync(const CCamera& rCamera)
    {
        Matrix44                                    MatProj;
        Matrix44                                    MatView;
        Matrix44                                    MatViewProj;

#if !defined(_RELEASE) // debug code to catch double invocations of the prepare occlusion buffer job per frame
        static int _debug = -1;
        if (_debug == -1)
        {
            _debug = gEnv->pRenderer->GetFrameID(false);
        }
        else if (_debug == gEnv->pRenderer->GetFrameID(false))
        {
            __debugbreak();
        }
        else
        {
            _debug = gEnv->pRenderer->GetFrameID(false);
        }
#endif

        const CCamera& rCam = rCamera;

        CCamera tmp_cam = m_pRenderer->GetCamera();
        m_pRenderer->SetCamera(rCam);
        m_pRenderer->GetModelViewMatrix(reinterpret_cast<f32*>(&MatView));
        m_pRenderer->GetProjectionMatrix(reinterpret_cast<f32*>(&MatProj));
        m_pRenderer->SetCamera(tmp_cam);

        uint32 nReverseDepthEnabled = 0;
        m_pRenderer->EF_Query(EFQ_ReverseDepthEnabled, nReverseDepthEnabled);

        if (nReverseDepthEnabled) // Convert to regular depth again. TODO: make occlusion culler work with reverse depth
        {
            MatProj.m22 = -MatProj.m22 + MatProj.m23;
            MatProj.m32 = -MatProj.m32 + MatProj.m33;
        }

        m_ViewDir   =   rCam.GetViewdir();
        MatViewProj = MatView * MatProj;

        MatViewProj.Transpose();

        const float SCALEX      =   static_cast<float>(CULL_SIZEX / 2);
        const float SCALEY      =   static_cast<float>(CULL_SIZEY / 2);
        const Matrix44A MatScreen(SCALEX, 0.f, 0.f, SCALEX,
            0.f, -SCALEY, 0.f, SCALEY,
            0.f, 0.f, 1.f, 0.f,
            0.f, 0.f, 0.f, 1.f);
        m_MatScreenViewProj =   MatScreen * MatViewProj;
        m_MatScreenViewProjTransposed = m_MatScreenViewProj.GetTransposed();
        m_NearPlane =  rCam.GetNearPlane();
        m_FarPlane =  rCam.GetFarPlane();
        m_NearestMax = m_pRenderer->GetNearestRangeMax();

        m_Position  =   rCam.GetPosition();

        HWZBuffer.ZBufferSizeX  =   CULL_SIZEX;
        HWZBuffer.ZBufferSizeY    =   CULL_SIZEY;

        GetObjManager()->BeginCulling();

        m_nPrepareState = PREPARE_STARTED;
        m_Enabled = false;
        m_bCheckOcclusionRequested = 0;

        RASTERIZER.Prepare();

        m_PrepareBufferSync.PushCompletionFence();
        m_OcclusionJobExecutor.StartJob([this]()
        {
            this->PrepareOcclusion();
        }); // legacy: job.SetPriorityLevel(JobManager::eHighPriority); job.SetBlocking();
    }

    void CCullThread::CullStart(const SRenderingPassInfo& passInfo)
    {
        FUNCTION_PROFILER_3DENGINE;

        // signal rasterizer that it should stop
        m_bCheckOcclusionRequested = 1;

        // tell the job that the PPU is ready for occlusion culling, this call will
        // start the check occlusion job if the prepare step has finished, if not
        // the prepare job itself will start the culling job
        bool bNeedJobStart = false;
        {
            AUTO_LOCK(m_FollowUpLock);
            if (m_nPrepareState == PREPARE_DONE)
            {
                m_nPrepareState = CHECK_STARTED;
                bNeedJobStart = true;
            }
            else
            {
                m_nPrepareState = CHECK_REQUESTED;
                *((SRenderingPassInfo*)m_passInfoForCheckOcclusion) = passInfo;
            }
        }

        if (bNeedJobStart)
        {
            m_OcclusionJobExecutor.StartJob([this, passInfo]()
            {
                this->CheckOcclusion(passInfo);
            }); // legacy: job.SetPriorityLevel(JobManager::eHighPriority);
        }
    }

    void CCullThread::CullEnd(bool waitForOcclusionJobCompletion)
    {
        // If no frame was rendered, we need to remove the producer added in BeginCulling
        m_PrepareBufferSync.WaitForCompletion();

        bool bNeedRemoveProducer = false;
        {
            if (m_nPrepareState != CHECK_STARTED && m_nPrepareState != IDLE)
            {
                bNeedRemoveProducer = true;
            }
        }

        if (bNeedRemoveProducer)
        {
            GetObjManager()->RemoveCullJobProducer();
            m_nPrepareState = IDLE; // No producer so mark us as idle
        }

        if (waitForOcclusionJobCompletion)
        {
            m_OcclusionJobExecutor.WaitForCompletion();
        }

    }

    void CCullThread::OutputMeshList()
    {
    }


    float DistToBox(Vec3 Center, Vec3 Extends, Vec3 ViewPos)
    {
        Vec3 Delta  =   (ViewPos - Center).abs();
        Delta   =   (Delta - Extends);
        Delta.x =    max(Delta.x, 0.f);
        Delta.y =    max(Delta.y, 0.f);
        Delta.z =    max(Delta.z, 0.f);
        return Delta.x * Delta.x + Delta.y * Delta.y + Delta.z * Delta.z;
    }

    void CCullThread::RasterizeZBuffer(uint32 PolyLimit)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

        if (m_OCMInstCount == 0)
        {
            float fRed[4] = {1, 0, 0, 1};
            gEnv->pRenderer->Draw2dLabel(1.0f, 5.0f, 1.6f, fRed, false, "OCM file failed to load -> no occlusion checking possible!");
            return;
        }

        uint Tmp[16 * sizeof(float) * 2 + 16];
        const uint8* pMeshes    =   m_pOCMBufferAligned;//actually starts at 16, but MeshOffset is zero based
        uint8* pInstances   =   &m_pOCMBufferAligned[m_OCMOffsetInstances];

        Matrix44A& rTmp0    =   *reinterpret_cast<Matrix44A*>((reinterpret_cast<size_t>(Tmp) + 15) & ~15);
        Matrix44A& rTmp1    =   *reinterpret_cast<Matrix44A*>((reinterpret_cast<size_t>(Tmp) + 15 + 64) & ~15);
        rTmp0   =   m_MatScreenViewProj.GetTransposed();

        int Visible = 0;
        int Invisible = 0;
        uint32 Poly = 0;
        float LastDist;
        uint8*  pLastInstance = 0;

        bool    Swapped = true;
        for (size_t c = 0; c < 20 && Swapped; c++)//incrementally (max 20 rounds) bubblesort instances front to back
        {
            Swapped = false;
            LastDist = -1.f;
            for (size_t a = 0; a < m_OCMInstCount; a++)
            {
                Matrix44 World(IDENTITY);
                uint8*  pInstance   =   pInstances + a * (sizeof(int) + 12 * sizeof(float));//meshoffset+worldmatrix43
                const uint32    MeshOffset = *reinterpret_cast<const uint32*>(&pInstance[0]);
                const float*    pWorldMat   =   reinterpret_cast<const float*>(&pInstance[4]);
                memcpy(&World, (void*)pWorldMat, 12 * sizeof(float));

                //simple incremental bubblesort
                const float Dist    =   (World.GetTranslation() - m_Position).GetLength();
                if (Dist < LastDist)
                {
                    PREFAST_ASSUME(pLastInstance);
                    Swapped = true;
                    for (size_t b = 0; b < 13; b++)
                    {
                        std::swap(reinterpret_cast<uint32*>(pLastInstance)[b], reinterpret_cast<uint32*>(pInstance)[b]);
                    }
                }

                LastDist = Dist;
                pLastInstance = pInstance;
            }
        }

        const bool EarlyOut = GetCVars()->e_CoverageBufferEarlyOut == 1;
        const int64 MaxEarlyOutDelay = (int64)(GetCVars()->e_CoverageBufferEarlyOutDelay * 1000.0f);
        LastDist = -1.f;

        ITimer* pTimer = gEnv->pTimer;
        int64 StartTime = -1;

        for (size_t a = 0; a < m_OCMInstCount && (PolyLimit == 0 || Poly < PolyLimit); a++)
        {
            // stop if MT need to run check occlusion
            if (EarlyOut && *const_cast<volatile int*>(&m_bCheckOcclusionRequested))
            {
                if (StartTime < 0)
                {
                    StartTime = pTimer->GetAsyncTime().GetMicroSecondsAsInt64();
                }

                int64 CurTime = pTimer->GetAsyncTime().GetMicroSecondsAsInt64();
                if (CurTime - StartTime > MaxEarlyOutDelay)
                {
                    break;
                }
            }

            Matrix44 World(IDENTITY);
            uint8*  pInstance   =   pInstances + a * (sizeof(int) + 12 * sizeof(float));//meshoffset+worldmatrix43
            const uint32    MeshOffset = *reinterpret_cast<volatile const uint32*>(&pInstance[0]);
            const float*    pWorldMat   =   reinterpret_cast<const float*>(&pInstance[4]);
            memcpy(&World, (void*)pWorldMat, 12 * sizeof(float));

            Vec3 Pos    =       World.GetTranslation(), Extend;
            Extend.x = (fabsf(World.m00) + fabsf(World.m01) + fabsf(World.m02)) * (127.f);
            Extend.y = (fabsf(World.m10) + fabsf(World.m11) + fabsf(World.m12)) * (127.f);
            Extend.z = (fabsf(World.m20) + fabsf(World.m21) + fabsf(World.m22)) * (127.f);

            const int InFrustum =   RASTERIZER.AABBInFrustum(reinterpret_cast<NVMath::vec4*>(&rTmp0), Pos - Extend, Pos + Extend, m_Position);
            if (!InFrustum)
            {
                Invisible++;
                continue;
            }
            else
            {
                Visible++;
            }

            rTmp1   =   (m_MatScreenViewProj * World).GetTransposed();
            const uint8*    pMesh           =   pMeshes + MeshOffset;
            const size_t    TriCount    =   *reinterpret_cast<const uint32*>(pMesh);
            const size_t    Tris16      =   (reinterpret_cast<size_t>(pMesh + 4) + 15) & ~15;
            const int8* pTris               =   reinterpret_cast<const int8*>(Tris16);
            if (InFrustum & 2)
            {
                RASTERIZER.Rasterize<true>(reinterpret_cast<NVMath::vec4*>(&rTmp1), reinterpret_cast<const NVMath::vec4*>(pTris), TriCount);
            }
            else
            {
                RASTERIZER.Rasterize<false>(reinterpret_cast<NVMath::vec4*>(&rTmp1), reinterpret_cast<const NVMath::vec4*>(pTris), TriCount);
            }
            Poly += TriCount;
        }
    }

#if !defined(_RELEASE)
    void CCullThread::CoverageBufferDebugDraw()
    {
        RASTERIZER.DrawDebug(m_pRenderer, 1);
    }
#endif


    void CCullThread::PrepareOcclusion()
    {
        if (!GetCVars()->e_CameraFreeze)
        {
            FUNCTION_PROFILER_3DENGINE;
            using namespace NVMath;

            int bHWZBuffer  =   GetCVars()->e_CoverageBufferReproj;

            if (bHWZBuffer > 3 && m_OCMBuffer.empty())
            {
                bHWZBuffer  =   2;
            }

            if ((bHWZBuffer & 3) > 0)
            {
                m_Enabled = RASTERIZER.DownLoadHWDepthBuffer(m_NearPlane, m_FarPlane, m_NearestMax, GetCVars()->e_CoverageBufferBias);
            }
            else
            {
                RASTERIZER.Clear();
            }
        }

        m_OcclusionJobExecutor.StartJob([this]()
        {
            this->PrepareOcclusion_ReprojectZBuffer();
        }); // legacy: job.SetPriorityLevel(JobManager::eHighPriority);
    }

    void CCullThread::PrepareOcclusion_ReprojectZBuffer()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

        int bHWZBuffer  =   GetCVars()->e_CoverageBufferReproj;

        if (bHWZBuffer > 3 && m_OCMBuffer.empty())
        {
            bHWZBuffer  =   2;
        }

        if (!GetCVars()->e_CameraFreeze && (bHWZBuffer & 3) > 0 && m_Enabled)
        {
            enum
            {
                nLinesPerJob = 8
            };
            m_nRunningReprojJobs = tdCullRasterizer::RESOLUTION_Y / nLinesPerJob;
            m_nRunningReprojJobsAfterMerge = tdCullRasterizer::RESOLUTION_Y / nLinesPerJob;
            for (int i = 0; i < tdCullRasterizer::RESOLUTION_Y; i += nLinesPerJob)
            {
                m_OcclusionJobExecutor.StartJob([this, i]()
                {
                    this->PrepareOcclusion_ReprojectZBufferLine(i, nLinesPerJob);
                }); // legacy: job.SetPriorityLevel(JobManager::eHighPriority);
            }
        }
        else
        {
            m_OcclusionJobExecutor.StartJob([this]()
            {
                this->PrepareOcclusion_RasterizeZBuffer();
            }); // job.SetPriorityLevel(JobManager::eHighPriority);
        }
    }

    void CCullThread::PrepareOcclusion_ReprojectZBufferLine(int nStartLine, int nNumLines)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

        if (!GetCVars()->e_CameraFreeze)
        {
            uint Tmp[80];
            Matrix44A& rTmp =   *reinterpret_cast<Matrix44A*>((reinterpret_cast<size_t>(Tmp) + 15) & ~15);
            rTmp    =   m_MatScreenViewProjTransposed;
            RASTERIZER.ReprojectHWDepthBuffer(rTmp, m_NearPlane, m_FarPlane, m_NearestMax, GetCVars()->e_CoverageBufferBias, nStartLine, nNumLines);
        }

        uint32 nRemainingJobs = CryInterlockedDecrement((volatile int*)&m_nRunningReprojJobs);
        if (nRemainingJobs == 0)
        {
            enum
            {
                nLinesPerJob = 8
            };
            for (int i = 0; i < tdCullRasterizer::RESOLUTION_Y; i += nLinesPerJob)
            {
                m_OcclusionJobExecutor.StartJob([this, i]()
                {
                    this->PrepareOcclusion_ReprojectZBufferLineAfterMerge(i, nLinesPerJob);
                }); // job.SetPriorityLevel(JobManager::eHighPriority);
            }
        }
    }

    void CCullThread::PrepareOcclusion_ReprojectZBufferLineAfterMerge(int nStartLine, int nNumLines)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

        // merge the reprojected buffer bevore new jobs are started on it
        RASTERIZER.MergeReprojectHWDepthBuffer(nStartLine, nNumLines);

        if (!GetCVars()->e_CameraFreeze)
        {
            uint Tmp[80];
            Matrix44A& rTmp =   *reinterpret_cast<Matrix44A*>((reinterpret_cast<size_t>(Tmp) + 15) & ~15);
            rTmp    =   m_MatScreenViewProjTransposed;
            RASTERIZER.ReprojectHWDepthBufferAfterMerge(rTmp, m_NearPlane, m_FarPlane, m_NearestMax, GetCVars()->e_CoverageBufferBias, nStartLine, nNumLines);
        }

        uint32 nRemainingJobs = CryInterlockedDecrement((volatile int*)&m_nRunningReprojJobsAfterMerge);
        if (nRemainingJobs == 0)
        {
            m_OcclusionJobExecutor.StartJob([this]()
            {
                this->PrepareOcclusion_RasterizeZBuffer();
            }); //job.SetPriorityLevel(JobManager::eHighPriority);
        }
    }


    void CCullThread::PrepareOcclusion_RasterizeZBuffer()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

        m_Enabled = true;
        if (!GetCVars()->e_CameraFreeze)
        {
            int bHWZBuffer  =   GetCVars()->e_CoverageBufferReproj;
            int PolyLimit       =   GetCVars()->e_CoverageBufferRastPolyLimit;

            if (bHWZBuffer > 3 && m_OCMBuffer.empty())
            {
                bHWZBuffer  =   2;
            }

            bool rast_z_buff = (bHWZBuffer & 4) ? true : false;
            if (rast_z_buff)
            {
                m_Enabled   =   true;
                RasterizeZBuffer((uint32)PolyLimit);
            }
        }

        bool bNeedJobStart = false;
        {
            AUTO_LOCK(m_FollowUpLock);
            if (m_nPrepareState == CHECK_REQUESTED)
            {
                m_nPrepareState = CHECK_STARTED;
                bNeedJobStart = true;
            }
            else
            {
                m_nPrepareState = PREPARE_DONE;
            }
        }

        m_PrepareBufferSync.PopCompletionFence();
        if (bNeedJobStart)
        {
            m_OcclusionJobExecutor.StartJob([this]()
            {
                this->CheckOcclusion(*reinterpret_cast<SRenderingPassInfo*>(m_passInfoForCheckOcclusion));
            }); // legacy: job.SetPriorityLevel(JobManager::eHighPriority)
        }
    }

    void CCullThread::CheckOcclusion(SRenderingPassInfo passInfo)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

        uint8 AlignBuffer[2 * sizeof(Matrix44A) + 16];
        size_t pBuffer  =   (reinterpret_cast<size_t>(AlignBuffer) + 15) & ~15;
        Matrix44A& RESTRICT_REFERENCE rMatFinalT    =   reinterpret_cast<Matrix44A*>(pBuffer)[1];


        Vec3 localPostion;
        memcpy(&localPostion, &m_Position, sizeof(Vec3));

        const AABB PosAABB  =   AABB(m_Position, 0.5f);
        const float Bias    =   GetCVars()->e_CoverageBufferAABBExpand;
        rMatFinalT  =   m_MatScreenViewProj.GetTransposed();

        // Debugging stats in green to screen here with how many octree nodes pass/fail and how many terrain nodes pass/fail
        unsigned int octreeNodesCulled = 0;
        unsigned int octreeNodesVisible = 0;
        while (1)
        {
            SCheckOcclusionJobData jobData;
            GetObjManager()->PopFromCullQueue(&jobData);

            // stop processing when beeing told so
            if (jobData.type == SCheckOcclusionJobData::QUIT)
            {
                break;
            }

            if (jobData.type == SCheckOcclusionJobData::OCTREE_NODE)
            {
                AABB    rAABB;
                COctreeNode* pOctTreeNode = (COctreeNode*)jobData.octTreeData.pOctTreeNode;

                memcpy(&rAABB, &pOctTreeNode->GetObjectsBBox(), sizeof(AABB));
                float fDistance = sqrtf(Distance::Point_AABBSq(passInfo.GetCamera().GetPosition(), rAABB));

                // Test OctTree BoundingBox
                if (TestAABB(rAABB, fDistance))
                {
                    pOctTreeNode->COctreeNode::RenderContent(jobData.octTreeData.nRenderMask, passInfo, jobData.rendItemSorter, jobData.pCam);
                    octreeNodesVisible++;
                }
                else
                {
                    octreeNodesCulled++;
                }
            }
            else
            {
                __debugbreak(); // unknown culler job type
            }
        }

        if (GetCVars()->e_CoverageBufferDebug)
        {
            float fGreen[4] = {0, 1, 0, 1};
            gEnv->pRenderer->Draw2dLabel(16.0f, 32.0f, 1.6f, fGreen, false, AZStd::string::format("Octree Nodes Culled %i, Octree Nodes Visible %i",octreeNodesCulled, octreeNodesVisible).c_str());
        }

        GetObjManager()->RemoveCullJobProducer();
    }

    ///////////////////////////////////////////////////////////////////////////////
    bool CCullThread::TestAABB(const AABB& rAABB, float fEntDistance, float fVerticalExpand)
    {
        IF (GetCVars()->e_CheckOcclusion == 0, 0)
        {
            return true;
        }

        const AABB PosAABB  =   AABB(m_Position, 0.5f);
        const float Bias    =   GetCVars()->e_CoverageBufferAABBExpand;
        DEFINE_ALIGNED_DATA(Matrix44A, rMatFinalT(m_MatScreenViewProj.GetTransposed()), 16);
        AABB bbox(rAABB);

        if (Bias < 0.f)
        {
            bbox.Expand((bbox.max - bbox.min) * -Bias - Vec3(Bias, Bias, Bias));
        }
        else
        {
            bbox.Expand(Vec3(Bias * fEntDistance));
        }

        float fVerticalExpandScaled = fVerticalExpand * fEntDistance;
        bbox.min.z -= fVerticalExpandScaled;
        bbox.max.z += fVerticalExpandScaled;

        if (!m_Enabled)
        {
            return true;
        }

        if (bbox.IsIntersectBox(PosAABB))
        {
            return true;
        }

        if (RASTERIZER.TestAABB(reinterpret_cast<const NVMath::vec4*>(&rMatFinalT), bbox.min, bbox.max, m_Position))
        {
            return true;
        }

        return false;
    }

    bool CCullThread::TestQuad(const Vec3& vCenter, const Vec3& vAxisX, const Vec3& vAxisY)
    {
        IF (GetCVars()->e_CheckOcclusion == 0, 0)
        {
            return true;
        }

        if (!m_Enabled)
        {
            return true;
        }

        DEFINE_ALIGNED_DATA(Matrix44A, rMatFinalT(m_MatScreenViewProj.GetTransposed()), 16);
        if (RASTERIZER.TestQuad(reinterpret_cast<const NVMath::vec4*>(&rMatFinalT), vCenter, vAxisX, vAxisY))
        {
            return true;
        }

        return false;
    }
} // namespace NAsyncCull


