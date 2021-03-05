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

// Description : CPU side SVO


#include "SVOGI_precompiled.h"

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/lock.h>

#include "SvoTree.h"

#include "TextureBlockPacker.h"
#include "IRenderAuxGeom.h"

#include "FrameProfiler.h"

#include "MathConversion.h"

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

namespace SVOGI
{
    //Since the Job System has no priority or size scheduling hints we don't want to starve out other jobs with 
    //the longer running GI jobs.  This will keep us to a 'reasonable' number of jobs.
    static AZ::u32 s_svoMaxJobCount = (AZStd::thread::hardware_concurrency() / 4 > 0) ? AZStd::thread::hardware_concurrency() / 4 : 1;
    //Scratch working buffer for GI calculations.
    static DataBrick<GISubVoxels>* s_scratchData;
    static AZStd::atomic_bool* s_freeScratch;

    ////////////////////////////////////////////////////////////////////////////////////////////
    //                        SVO ENVIRONMENT
    ////////////////////////////////////////////////////////////////////////////////////////////    

    const AZ::s32 SvoEnvironment::s_uninitializedTexturePoolId = 0;

    AZ::s32 SvoEnvironment::m_currentPassFrameId = 0;
    AZ::s32 SvoEnvironment::m_brickTexturePoolDimXY = 128;
    AZ::s32 SvoEnvironment::m_brickTexturePoolDimZ = 256;

    SvoEnvironment::SvoEnvironment()
    {
        m_brickTextureFormat = eTF_R8G8B8A8; // eTF_BC3

        AllocateTexturePools();

        m_prevCheckVal = -1000000;

        m_svoRoot.reset();
        m_globalSpecularCM = nullptr;
        m_globalSpecularCM_Mult = 1;
        m_activeVoxels = 0;
        m_evictGpu = false;

        m_blockPacker = aznew TextureBlockPacker3D(nAtlasDimMaxXY, nAtlasDimMaxXY, nAtlasDimMaxZ, true);
        m_blockIndex = 0;
        s_scratchData = (DataBrick<GISubVoxels>*)azmalloc(sizeof(DataBrick<GISubVoxels>)*s_svoMaxJobCount);
        s_scratchData = new(s_scratchData) DataBrick<GISubVoxels>[s_svoMaxJobCount];
        s_freeScratch = new AZStd::atomic_bool[s_svoMaxJobCount];
        for (AZ::s32 i = 0; i < s_svoMaxJobCount; ++i)
        {
            s_freeScratch[i].store(true);
        }

        EBUS_EVENT_RESULT(m_jobContext, AZ::JobManagerBus, GetGlobalContext);
    }

    SvoEnvironment::~SvoEnvironment()
    {
        m_voxelJobsCompletion.StartAndWaitForCompletion();
        // To avoid potential memory issues during tear down, ensure that all voxels are destroyed
        // before destroying the block packer as they refer to memory owned by the block packer.
        m_processingQueue.clear();
        m_brickUpdateQueue.clear();
        m_svoRoot.reset();

        delete m_blockPacker;
        delete[] s_freeScratch;
        azfree(s_scratchData);

        SvoEnvironment::m_currentPassFrameId = 0;

        DeallocateTexturePools();
    }

    void SvoEnvironment::ReconstructTree()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
        if (gEnv->pConsole->GetCVar("e_svoTI_Active")->GetIVal())
        {
            m_brickUpdateQueue.clear();
            m_processingQueue.clear();

            m_svoRoot.reset();

            DeallocateTexturePools();
            AllocateTexturePools();

            // Keep pending insertions and include inserted meshes to it.
            // They will be reconsidered on the first update after the recreation of the root.
            m_globalInsertions.insert(m_globalInsertedMeshes.begin(), m_globalInsertedMeshes.end());
            m_globalInsertedMeshes.clear();
            m_globalRemovals.clear();

            AZ::Aabb terrainAabb = AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(terrainAabb, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);
            m_svoRoot = AZStd::make_shared<Voxel>(terrainAabb, nullptr, this, 0);
        }
    }

    //Push all nodes that need to be updated into the update queue.
    void SvoEnvironment::UpdateVoxels()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
        if (!m_svoRoot)
        {
            if (!gEnv->p3DEngine->LevelLoadingInProgress() || gEnv->IsEditor())
            {
                AZ::Aabb terrainAabb = AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
                AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(terrainAabb, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);
                m_svoRoot = AZStd::make_shared<Voxel>(terrainAabb, nullptr, this, 0);
            }
        }

        if (m_svoRoot)
        {
            float maxSize = gEnv->pConsole->GetCVar("e_svoMaxNodeSize")->GetFVal();
            float minSize = gEnv->pConsole->GetCVar("e_svoMinNodeSize")->GetFVal();

            bool hasNewInsertionsOrRemovals = !(m_globalInsertions.empty() && m_globalRemovals.empty());
            if (hasNewInsertionsOrRemovals)
            {
                m_svoRoot->EnqueueMeshes(m_globalInsertions, m_globalRemovals, maxSize);
                m_globalInsertedMeshes.insert(m_globalInsertions.begin(), m_globalInsertions.end());
                m_globalInsertions.clear();
                m_globalRemovals.clear();
            }

            m_svoRoot->Update(m_processingQueue, m_svoRoot, maxSize, minSize);
        }
    }

    //Process nodes that need updating and remove outdated nodes.
    void SvoEnvironment::ProcessVoxels()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
        AZ::s32 maxLoadedNodes = gEnv->pConsole->GetCVar("e_svoMaxBricksOnCPU")->GetIVal();
        AZ::s32 maxNodesPerJob = gEnv->pConsole->GetCVar("e_svoMaxVoxelUpdatesPerJob")->GetIVal();
        float maxSize = gEnv->pConsole->GetCVar("e_svoMaxNodeSize")->GetFVal();

        while (!m_processingQueue.empty())
        {
            //Compute Free Scratch Space location
            AZ::u32 offset = 0;
            for (; offset < s_svoMaxJobCount; ++offset)
            {
                if (s_freeScratch[offset])
                {
                    s_freeScratch[offset].store(false);
                    break;
                }
            }

            //No free scratch space stop processing this frame.
            if (offset == s_svoMaxJobCount)
            {
                return;
            }

            //Build working voxel set for job
            AZStd::vector<AZStd::shared_ptr<Voxel>> voxels;
            voxels.reserve(maxNodesPerJob);
            AZ::u32 count = 0;
            while (!m_processingQueue.empty() && count < maxNodesPerJob)
            {
                voxels.push_back(m_processingQueue.front());
                m_processingQueue.pop_front();
                ++count;
            }

            auto voxelJobFunc = [this, offset, voxels, maxSize, maxLoadedNodes]()
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Renderer, "SvoEnvironment::ProcessVoxels:JobFunc");
                for (auto voxel : voxels)
                {
                    voxel->UpdateBrickData(maxSize, maxLoadedNodes, s_scratchData[offset]);

                    voxel->m_cpuEnqueued.store(false);
                }

                s_freeScratch[offset] = true;
            };

            AZ::Job* job = AZ::CreateJobFunction(voxelJobFunc, true, m_jobContext);
            job->SetDependent(&m_voxelJobsCompletion);
            job->Start();
        }
    }

    void SvoEnvironment::EvictVoxels()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

        if (!m_svoRoot)
        {
            return;
        }

        AZ::s32 maxLoadedNodes = gEnv->pConsole->GetCVar("e_svoMaxBricksOnCPU")->GetIVal();

        if ((m_lastEvictionFrame + m_delayToEvictInFrames) < GetCurrPassMainFrameID() ||
            m_activeVoxels >= maxLoadedNodes)
        {
            m_lastEvictionFrame = GetCurrPassMainFrameID();

            float minSize = gEnv->pConsole->GetCVar("e_svoMinNodeSize")->GetFVal();

            // Evict voxels older than m_evictionDelay
            m_svoRoot->Evict(m_delayToEvictInFrames, false, minSize);

            // Warn the user that the value of e_svoMaxBricksOnCPU is not high enough.
            // NOTE: Because Evict() does not release nodes that are or will be processed, we will only
            // warn the user when it keeps happening after a reasonable amount of continuous frames.
            if (m_activeVoxels >= maxLoadedNodes)
            {
                ++m_numFramesReachingMaxBricksOnCPU;

                if (m_numFramesReachingMaxBricksOnCPU >= m_delayToWarnReachingMaxBricksOnCPUInFrames)
                {
                    AZ_Warning("SVOGI", false, "Maximum number of active voxels reached (%d). Increment the value of e_svoMaxBricksOnCPU.", maxLoadedNodes);
                    m_numFramesReachingMaxBricksOnCPU = 0; // Avoid warn every frame
                }
            }
            else
            {
                m_numFramesReachingMaxBricksOnCPU = 0;
            }
        }
    }

    void SvoEnvironment::EvictGpuData()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
        if (m_evictGpu)
        {
            if (m_svoRoot)
            {
                m_svoRoot->EvictGpuData(m_evictionDelayInFrames, false);
            }
            m_evictGpu = false;
        }
    }

    void SvoEnvironment::UploadVoxels(bool showVoxels)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
        AZ::s32 maxBricksPerFrame = gEnv->pConsole->GetCVar("e_svoMaxBrickUpdates")->GetIVal();
        AZ::s32 bricksUploaded = 0;
        {
            while (!m_brickUpdateQueue.empty() && bricksUploaded <= maxBricksPerFrame)
            {
                AZStd::shared_ptr<Voxel> voxel = m_brickUpdateQueue.front();
                voxel->UpdateBrickRenderData();
                voxel->UpdateTreeRenderData();
                //Mark block as needing to be processed.
                if (voxel->m_block)
                {
                    voxel->m_block->m_staticDirty = true;
                    voxel->m_block->m_dynamicDirty = true;
                }
                voxel->m_gpuEnqueued = false;
                m_brickUpdateQueue.pop_front();
                ++bricksUploaded;
            }
        }

        if (m_svoRoot)
        {
            float maxSize = gEnv->pConsole->GetCVar("e_svoMaxNodeSize")->GetFVal();
            m_svoRoot->ReserveGPUMemory(m_svoRoot, maxSize);
            m_svoRoot->UpdateGpuTree(m_svoRoot);
            if (showVoxels)
            {
                m_svoRoot->DrawVoxels();
            }
        }
    }

    void SvoEnvironment::GetSvoStaticTextures(I3DEngine::SSvoStaticTexInfo& svoInfo, PodArray<I3DEngine::SLightTI>* pLightsTI_S, PodArray<I3DEngine::SLightTI>* pLightsTI_D)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
        svoInfo.pTexTree = gEnv->pRenderer->EF_GetTextureByID(m_nTexNodePoolId);
        svoInfo.pTexOpac = gEnv->pRenderer->EF_GetTextureByID(m_nTexOpasPoolId);
        svoInfo.pTexRgb0 = gEnv->pRenderer->EF_GetTextureByID(m_nTexRgb0PoolId);
        svoInfo.pTexRgb1 = gEnv->pRenderer->EF_GetTextureByID(m_nTexRgb1PoolId);
        svoInfo.pTexDynl = gEnv->pRenderer->EF_GetTextureByID(m_nTexDynlPoolId);
        svoInfo.pTexRgb2 = gEnv->pRenderer->EF_GetTextureByID(m_nTexRgb2PoolId);
        svoInfo.pTexRgb3 = gEnv->pRenderer->EF_GetTextureByID(m_nTexRgb3PoolId);
        svoInfo.pTexNorm = gEnv->pRenderer->EF_GetTextureByID(m_nTexNormPoolId);
        svoInfo.pTexAldi = gEnv->pRenderer->EF_GetTextureByID(m_nTexAldiPoolId);
        svoInfo.pGlobalSpecCM = m_globalSpecularCM;

        svoInfo.fGlobalSpecCM_Mult = m_globalSpecularCM_Mult;

        svoInfo.nTexDimXY = m_brickTexturePoolDimXY;
        svoInfo.nTexDimZ = m_brickTexturePoolDimZ;
        svoInfo.nBrickSize = brickDimension;

        svoInfo.bSvoReady = true;

        *pLightsTI_S = m_lightsTI_S;
        *pLightsTI_D = m_lightsTI_D;
    }

    void SvoEnvironment::GetSvoBricksForUpdate(PodArray<I3DEngine::SSvoNodeInfo>& arrNodeInfo, bool getDynamic)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
        arrNodeInfo.Clear();

        if (!m_blockPacker)
        {
            return;
        }

        if (!gEnv->pConsole->GetCVar("e_svoTI_Active")->GetIVal())
        {
            return;
        }

        const AZ::u32 numberOfBlocks = m_blockPacker->GetNumBlocks();

        AZ::u32 maxUpdatesPerFrame = gEnv->pConsole->GetCVar("e_svoMaxBrickUpdates")->GetIVal();
        AZ::u32 blocksAdded = 0;
        AZ::u32 oldStart = m_blockIndex;
        for (; blocksAdded < maxUpdatesPerFrame;)
        {
            if (TextureBlock3D* block = m_blockPacker->GetBlockInfo(m_blockIndex))
            {
                if ((!getDynamic && block->m_staticDirty) || (getDynamic && block->m_dynamicDirty))
                {
                    I3DEngine::SSvoNodeInfo nodeInfo;

                    nodeInfo.wsBox = AZAabbToLyAABB(block->m_worldBox);
                    nodeInfo.tcBox = AZAabbToLyAABB(block->m_textureBox);
                    nodeInfo.nAtlasOffset = block->m_atlasOffset;
                    ++blocksAdded;
                    arrNodeInfo.Add(nodeInfo);

                    if (!getDynamic)
                    {
                        block->m_staticDirty = false;
                    }
                    else
                    {
                        block->m_dynamicDirty = false;
                    }
                    block->m_lastUpdatedFrame = GetCurrPassMainFrameID();
                }
            }
            m_blockIndex = (m_blockIndex + 1) % numberOfBlocks;
            //Walked full block list back to where we started.  Break for now.
            if (m_blockIndex == oldStart)
            {
                break;
            }
        }

        //Due to the fact that we are not tracking lighting changes
        //refresh the block if it hasn't been updated in a while.
        //Once we have a system for detecting if a light has changed 
        //with respect to a given voxel region we can remove this behavior.
        for (AZ::u32 blockIndex = 0; blockIndex < numberOfBlocks; blockIndex++)
        {
            if (TextureBlock3D* block = m_blockPacker->GetBlockInfo(blockIndex))
            {
                if (GetCurrPassMainFrameID() - block->m_lastUpdatedFrame > m_blockUpdateDelayInFrames)
                {
                    block->m_staticDirty = true;
                    block->m_dynamicDirty = true;
                }
            }
        }
    }

    static AZ::s32 SLightTI_Compare(const void* v1, const void* v2)
    {
        I3DEngine::SLightTI* p[2] = { (I3DEngine::SLightTI*)v1, (I3DEngine::SLightTI*)v2 };

        if (p[0]->fSortVal > p[1]->fSortVal)
        {
            return 1;
        }
        if (p[0]->fSortVal < p[1]->fSortVal)
        {
            return -1;
        }

        return 0;
    }

    void SvoEnvironment::CollectLights()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
        AABB nodeBox;
        nodeBox.Reset();

        nodeBox.Add(gEnv->pSystem->GetViewCamera().GetPosition());

        nodeBox.Expand(Vec3(256, 256, 256));

        m_lightsTI_S.Clear();
        m_lightsTI_D.Clear();

        if (AZ::s32 nCount = gEnv->p3DEngine->GetObjectsByTypeInBox(eERType_Light, nodeBox, (IRenderNode**)0))
        {
            AZStd::vector<IRenderNode*> arrObjects(nCount);
            nCount = gEnv->p3DEngine->GetObjectsByTypeInBox(eERType_Light, nodeBox, &arrObjects[0]);

            for (AZ::s32 nL = 0; nL < nCount; nL++)
            {
                ILightSource* pRN = (ILightSource*)arrObjects[nL];

                CDLight& rLight = pRN->GetLightProperties();

                I3DEngine::SLightTI lightTI;
                memset(&lightTI, 0, sizeof(lightTI));


                IRenderNode::EVoxelGIMode eVoxMode = pRN->GetVoxelGIMode();

                if (eVoxMode)
                {
                    lightTI.vPosR = Vec4(rLight.m_Origin, rLight.m_fRadius);

                    if ((rLight.m_Flags & DLF_PROJECT) && (rLight.m_fLightFrustumAngle < 90.f) && rLight.m_pLightImage)
                    {
                        lightTI.vDirF = Vec4(pRN->GetMatrix().GetColumn(0), rLight.m_fLightFrustumAngle * 2);
                    }
                    else
                    {
                        lightTI.vDirF = Vec4(0, 0, 0, 0);
                    }

                    if (eVoxMode == IRenderNode::VM_Dynamic)
                    {
                        lightTI.vCol = rLight.m_Color.toVec4();
                    }
                    else
                    {
                        lightTI.vCol = rLight.m_BaseColor.toVec4();
                    }

                    lightTI.vCol.w = (rLight.m_Flags & DLF_CASTSHADOW_MAPS) ? 1.f : 0.f;

                    if (rLight.m_Flags & DLF_SUN)
                    {
                        lightTI.fSortVal = -1;
                    }
                    else
                    {

                        Vec3 vCamPos = m_camera.GetPosition();
                        lightTI.fSortVal = vCamPos.GetDistance(rLight.m_Origin) / max(24.f, rLight.m_fRadius);
                    }

                    if (eVoxMode == IRenderNode::VM_Dynamic)
                    {
                        if ((pRN->GetDrawFrame(0) > 10) && (pRN->GetDrawFrame(0) >= (AZ::s32)GetCurrPassMainFrameID()))
                        {
                            m_lightsTI_D.Add(lightTI);
                        }
                    }
                    else
                    {
                        m_lightsTI_S.Add(lightTI);
                    }
                }
            }

            if (m_lightsTI_S.Count() > 1)
            {
                qsort(m_lightsTI_S.GetElements(), m_lightsTI_S.Count(), sizeof(m_lightsTI_S[0]), SLightTI_Compare);
            }

            if (m_lightsTI_D.Count() > 1)
            {
                qsort(m_lightsTI_D.GetElements(), m_lightsTI_D.Count(), sizeof(m_lightsTI_D[0]), SLightTI_Compare);
            }

            if (m_lightsTI_D.Count() > 8)
            {
                m_lightsTI_D.PreAllocate(8);
            }
        }

        m_globalSpecularCM = 0;
        m_globalSpecularCM_Mult = 0;
        AZ::Aabb terrainAabb = AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(terrainAabb, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);
        AABB areaBox = AZAabbToLyAABB(terrainAabb);

        if (AZ::s32 nCount = gEnv->p3DEngine->GetObjectsByTypeInBox(eERType_Light, areaBox, (IRenderNode**)0))
        {
            AZStd::vector<IRenderNode*> arrObjects(nCount, nullptr);
            nCount = gEnv->p3DEngine->GetObjectsByTypeInBox(eERType_Light, areaBox, &arrObjects[0]);

            float fMaxRadius = 999;

            for (AZ::s32 nL = 0; nL < nCount; nL++)
            {
                ILightSource* pRN = (ILightSource*)arrObjects[nL];

                CDLight& rLight = pRN->GetLightProperties();

                if (rLight.m_fRadius > fMaxRadius && rLight.m_Flags & DLF_DEFERRED_CUBEMAPS)
                {
                    fMaxRadius = rLight.m_fRadius;
                    m_globalSpecularCM = rLight.GetSpecularCubemap();
                    m_globalSpecularCM_Mult = rLight.m_SpecMult;
                }
            }
        }
    }

    void SvoEnvironment::UpsertMesh(AZ::EntityId entityId, AZ::Transform transform, AZ::Aabb worldAabb,
        AZ::Data::Asset<LmbrCentral::MeshAsset> meshAsset, _smart_ptr<IMaterial> material)
    {
        // Both cases insertion and update of a mesh are treated the same
        // way, as a new insertion, so remove the mesh first.
        RemoveMesh(entityId);

        AZStd::shared_ptr<MeshData> data = AZStd::make_shared<MeshData>(entityId, transform, worldAabb, meshAsset, material);

        bool inserted = m_globalInsertions.insert({ entityId, data }).second;
        AZ_Assert(inserted, "Tried to double insert mesh.");

        // NOTE: At this point the mesh will be added to:
        //    - m_globalInsertions
        //    - m_globalRemovals (if the mesh was already inserted, this is a mesh update)
    }

    void SvoEnvironment::RemoveMesh(AZ::EntityId entityId)
    {
        //Check if mesh is in the list of current objects
        auto searchInserted = m_globalInsertedMeshes.find(entityId);
        if (searchInserted != m_globalInsertedMeshes.end())
        {
            m_globalRemovals.insert(*searchInserted);
            m_globalInsertedMeshes.erase(searchInserted);
        }

        //Remove pending insertions.
        auto searchInsertions = m_globalInsertions.find(entityId);
        if (searchInsertions != m_globalInsertions.end())
        {
            m_globalInsertions.erase(searchInsertions);
        }
    }

    void SvoEnvironment::AllocateTexturePool(AZ::s32& texPoolId, AZ::s32 width, AZ::s32 height, AZ::s32 depth, ETEX_Format texFormat, AZ::s32 filter, AZ::s32 flags)
    {
        if (texPoolId == s_uninitializedTexturePoolId)
        {
            texPoolId = gEnv->pRenderer->DownLoadToVideoMemory3D(NULL, width, height, depth, texFormat, texFormat, 1, false, filter, 0, 0, flags);
        }
    }

    void SvoEnvironment::AllocateTexturePools()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
        AZ::s32 nFlagsReadOnly = FT_DONT_STREAM;
        AZ::s32 nFlagsReadWrite = FT_DONT_STREAM | FT_USAGE_UNORDERED_ACCESS | FT_USAGE_UAV_RWTEXTURE;

        AllocateTexturePool(m_nTexRgb0PoolId, m_brickTexturePoolDimXY, m_brickTexturePoolDimXY, m_brickTexturePoolDimZ, m_brickTextureFormat, FILTER_LINEAR, nFlagsReadWrite);

        if (gEnv->pConsole->GetCVar("e_svoTI_Active")->GetIVal() && 
            gEnv->pConsole->GetCVar("e_svoTI_IntegrationMode")->GetIVal())
        {
            // direct lighting
            AllocateTexturePool(m_nTexRgb1PoolId, m_brickTexturePoolDimXY, m_brickTexturePoolDimXY, m_brickTexturePoolDimZ, m_brickTextureFormat, FILTER_LINEAR, nFlagsReadWrite);

            // dyn direct lighting
            AllocateTexturePool(m_nTexDynlPoolId, m_brickTexturePoolDimXY, m_brickTexturePoolDimXY, m_brickTexturePoolDimZ, m_brickTextureFormat, FILTER_LINEAR, nFlagsReadWrite);

            // propagation
            AllocateTexturePool(m_nTexRgb2PoolId, m_brickTexturePoolDimXY, m_brickTexturePoolDimXY, m_brickTexturePoolDimZ, m_brickTextureFormat, FILTER_LINEAR, nFlagsReadWrite);

            // propagation
            AllocateTexturePool(m_nTexRgb3PoolId, m_brickTexturePoolDimXY, m_brickTexturePoolDimXY, m_brickTexturePoolDimZ, m_brickTextureFormat, FILTER_LINEAR, nFlagsReadWrite);
        }

        AllocateTexturePool(m_nTexNormPoolId, m_brickTexturePoolDimXY, m_brickTexturePoolDimXY, m_brickTexturePoolDimZ, m_brickTextureFormat, FILTER_LINEAR, nFlagsReadWrite);

        AllocateTexturePool(m_nTexAldiPoolId, m_brickTexturePoolDimXY, m_brickTexturePoolDimXY, m_brickTexturePoolDimZ, m_brickTextureFormat, FILTER_LINEAR, nFlagsReadWrite);

        AllocateTexturePool(m_nTexOpasPoolId, m_brickTexturePoolDimXY, m_brickTexturePoolDimXY, m_brickTexturePoolDimZ, m_brickTextureFormat, FILTER_LINEAR, nFlagsReadWrite);

        AllocateTexturePool(m_nTexNodePoolId, nVoxNodPoolDimXY, nVoxNodPoolDimXY, nVoxNodPoolDimZ, eTF_R32G32B32A32F, FILTER_POINT, nFlagsReadOnly);
    }

    void SvoEnvironment::DeallocateTexturePool(AZ::s32& texPoolId)
    {
        if (texPoolId != s_uninitializedTexturePoolId)
        {
            gEnv->pRenderer->RemoveTexture(texPoolId);
            texPoolId = s_uninitializedTexturePoolId;
        }
    }

    void SvoEnvironment::DeallocateTexturePools()
    {
        DeallocateTexturePool(m_nTexRgb0PoolId);
        DeallocateTexturePool(m_nTexRgb1PoolId);
        DeallocateTexturePool(m_nTexDynlPoolId);
        DeallocateTexturePool(m_nTexRgb2PoolId);
        DeallocateTexturePool(m_nTexRgb3PoolId);
        DeallocateTexturePool(m_nTexNormPoolId);
        DeallocateTexturePool(m_nTexAldiPoolId);
        DeallocateTexturePool(m_nTexOpasPoolId);
        DeallocateTexturePool(m_nTexNodePoolId);
    }

    void SvoEnvironment::SetCamera(const CCamera& newCam)
    {
        m_camera = newCam;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    //                       Voxels
    ////////////////////////////////////////////////////////////////////////////////////////////


    Voxel::Voxel(const AZ::Aabb& box, AZStd::shared_ptr<Voxel> parent, SvoEnvironment* env, AZ::u8 childIndex)
    {
        AZ_Assert(env, "Invalid SVO Environment");
        AZ_Assert(childIndex < s_numVoxelChildren, "Invalid child index: %d", childIndex);

        m_parentNode = parent;
        m_nodeBox = box;
        m_boxSize = box.GetZExtent();
        m_svoEnv = env;
        m_cpuEnqueued.store(false);
        m_gpuEnqueued = false;
        m_childIndex = childIndex;
        m_childOffsetsDirty = true;
        m_blockID = TextureBlockPacker3D::s_invalidBlockID;
        m_block = nullptr;
        m_brick = nullptr;

        // Use current frame to avoid store frame 0 when voxel is created.
        m_lastVisibleFrameId = GetCurrPassMainFrameID();
        m_lastUpdatedFrameId = m_lastVisibleFrameId;

        m_queueId = 0;
    }

    Voxel::~Voxel()
    {
        ReleaseBlock();

        if (m_brick)
        {
            if (m_brick->HasBrickData())
            {
                --m_svoEnv->m_activeVoxels;
            }

            delete m_brick;
            m_brick = nullptr;
        }
    }

    void Voxel::ReleaseBlock()
    {
        if (m_blockID != TextureBlockPacker3D::s_invalidBlockID)
        {
            m_svoEnv->m_blockPacker->RemoveBlock(m_blockID);
            m_blockID = TextureBlockPacker3D::s_invalidBlockID;
        }
        m_block = nullptr;
    }

    void Voxel::Update(AZStd::deque<AZStd::shared_ptr<Voxel>>& processingQueue, AZStd::shared_ptr<Voxel> self, float maxSize, float minSize)
    {
        //If the node is not visible stop updating.
        if (!m_svoEnv->m_camera.IsAABBVisible_E(AZAabbToLyAABB(m_nodeBox)))
        {
            return;
        }

        m_lastVisibleFrameId = GetCurrPassMainFrameID();

        //If voxel is "small" relative to camera distance do not upload data to GPU.
        {
            AZStd::shared_ptr<Voxel> parent = m_parentNode.lock();
            bool voxelIsSmall = GetLodRatio() > m_svoEnv->m_voxelLodCutoff;
            bool parentHasObjectData = parent && parent->m_brick && parent->m_brick->HasBrickData() && !parent->m_brick->m_terrainOnly;
            if (voxelIsSmall && !parentHasObjectData)
            {
                return;
            }
        }

        m_lastUpdatedFrameId = m_lastVisibleFrameId;

        //Enqueue the node for processing if necessary. 
        {
            //Lock to prevent buffer swapping during write
            AZStd::lock_guard<AZStd::mutex> queueLock(m_queueMutex);
            AZ::u32 queueId = m_queueId;
            //If the voxel is not equeued and has either never been processed before (!(m_brick || m_block)) or has entities 
            //to be processed enqueue it. 
            const bool voxelNotQueued = !m_cpuEnqueued;
            const bool voxelNeverProcessed = !(m_brick || m_block);
            const bool hasInsertionsOrRemovals = (!m_insertions[queueId].empty() || !m_removals[queueId].empty());

            if (voxelNotQueued && (hasInsertionsOrRemovals || voxelNeverProcessed))
            {
                processingQueue.push_back(self);
                m_cpuEnqueued.store(true);
            }
        }

        //If node has brick data generate children. 
        //If the node is bigger than max size then generate the children automatically.
        if ((m_brick && m_brick->HasBrickData()) || m_boxSize > maxSize)
        {
            AllocateChildren(self, maxSize, minSize);
        }

        for (AZ::u32 childIndex = 0; childIndex < s_numVoxelChildren; childIndex++)
        {
            AZStd::shared_ptr<Voxel> child = m_children[childIndex];
            if (child)
            {
                child->Update(processingQueue, child, maxSize, minSize);
            }
        }
    }

    void Voxel::Evict(AZ::u32 frameDelay, bool forceEvict, float minSize)
    {
        //This function assumes the root is not evictable. 
        //To evict the entire tree reset the root pointer.
        AZStd::shared_ptr<Voxel> parent = m_parentNode.lock();

        if (parent && (m_lastVisibleFrameId < (GetCurrPassMainFrameID() - frameDelay) || forceEvict))
        {
            // Remove voxel from the tree.
            //
            // When the last shared pointer is destroyed then this voxel will be destroyed and
            // therefore all its children as well. That includes all bricks' data and gpu blocks
            // of this voxel and all its children. 
            // The last shared pointer can be:
            //    - This function's caller.
            //    - If this voxel will be processed a shared pointer is queued in m_processingQueue or m_brickUpdateQueue.
            //    - If this Voxel is being processed by a job a shared pointer will be in vector "voxels" (see function SvoEnvironment::ProcessVoxels()).
            //
            // NOTE: It's possible to remove the voxel from m_processingQueue and m_brickUpdateQueue now
            // to get the memory back faster, but it would not be accurate unless walking its children to
            // remove them as well. The cost of walking the children and remove them from the queues is not
            // worth it, eventually they will be processed and destroyed in a few frames anyway.
            parent->m_children[m_childIndex].reset();

            parent->m_childOffsetsDirty = true;

            parent.reset();
        }
        else
        {
            parent.reset();

            for (AZ::u32 childIndex = 0; childIndex < s_numVoxelChildren; childIndex++)
            {
                if (m_children[childIndex])
                {
                    m_children[childIndex]->Evict(frameDelay, forceEvict, minSize);
                }
            }
        }
    }

    void Voxel::EvictGpuData(AZ::u32 frameDelay, bool forceEvict)
    {
        if (m_lastVisibleFrameId < (GetCurrPassMainFrameID() - frameDelay) || forceEvict)
        {
            if (m_block)
            {
                ReleaseBlock();

                AZStd::shared_ptr<Voxel> parent = m_parentNode.lock();
                if (parent)
                {
                    parent->m_childOffsetsDirty = true;
                }

                // Force evict all children's GPU data since current voxel's was evicted
                forceEvict = true; 
            }
        }

        for (AZ::u32 childIndex = 0; childIndex < s_numVoxelChildren; childIndex++)
        {
            if (m_children[childIndex])
            {
                m_children[childIndex]->EvictGpuData(frameDelay, forceEvict);
            }
        }
    }

    void Voxel::EnqueueMeshes(const EntityMeshDataMap& insertions, const EntityMeshDataMap& removals, float maxSize)
    {
        EntityMeshDataMap newInsertions;
        EntityMeshDataMap newRemovals;

        {
            AZStd::lock_guard<AZStd::mutex> writeQueueLock(m_queueMutex);
            AZ::u32 queueId = m_queueId;
            auto& insertionQueue = m_insertions[queueId];
            auto& removalQueue = m_removals[queueId];

            for (auto& meshToRemove : removals)
            {
                //Check if mesh is in the list of inserted or pending insertion meshes
                auto searchInserted = m_insertedAndPendingInsertionMeshes.find(meshToRemove.first);
                if (searchInserted != m_insertedAndPendingInsertionMeshes.end())
                {
                    newRemovals.insert(*searchInserted);
                    removalQueue.insert(*searchInserted);
                    m_insertedAndPendingInsertionMeshes.erase(searchInserted);
                }

                //Remove pending insertions from current queue.
                auto searchInsertions = insertionQueue.find(meshToRemove.first);
                if (searchInsertions != insertionQueue.end())
                {
                    insertionQueue.erase(searchInsertions);
                }
            }

            for (auto& meshToInsert : insertions)
            {
                //Check if the mesh overlaps the voxel
                if (meshToInsert.second->m_worldAabb.Overlaps(m_nodeBox))
                {
                    newInsertions.insert(meshToInsert);

                    bool inserted = insertionQueue.insert(meshToInsert).second;
                    AZ_Assert(inserted, "Tried to double insert mesh");

                    // Adding the mesh to the inserted + pending insertion.
                    inserted = m_insertedAndPendingInsertionMeshes.insert(meshToInsert).second;
                    AZ_Assert(inserted, "Tried to double insert mesh.");
                }
            }
        }

        // Enqueue to children new mesh insertions/removals that affected this voxel.
        bool hasNewInsertionsOrRemovals = !(newInsertions.empty() && newRemovals.empty());
        if (hasNewInsertionsOrRemovals)
        {
            for (AZ::u32 childIndex = 0; childIndex < s_numVoxelChildren; childIndex++)
            {
                if (m_children[childIndex])
                {
                    m_children[childIndex]->EnqueueMeshes(newInsertions, newRemovals, maxSize);
                }
            }
        }
    }

    void Voxel::UpdateBrickData(float maxSize, AZ::s32 maxLoadedNodes, DataBrick<GISubVoxels>& scratchData)
    {
        // If this voxel doesn't have brick data yet
        // do not allocate more memory if we reached the
        // maximum number of active voxels.
        if (m_boxSize <= maxSize)
        {
            if (!m_brick || !m_brick->HasBrickData())
            {
                if (m_svoEnv->m_activeVoxels >= maxLoadedNodes)
                {
                    return;
                }
            }
        }

        // Swap buffers.
        AZ::u32 queueId;
        {
            AZStd::lock_guard<AZStd::mutex> swapQueueLock(m_queueMutex);
            queueId = m_queueId;
            m_queueId = (m_queueId + 1) % s_bufferCount;
        }

        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

        auto& insertions = m_insertions[queueId];
        auto& removals = m_removals[queueId];
        if (m_boxSize <= maxSize)
        {
            if (!m_brick)
            {
                m_brick = aznew Brick();
            }

            bool hadBrickData = m_brick->HasBrickData();

            m_brick->m_brickAabb = m_nodeBox;

            //Convert bounding box to local coordinates
            AZ::Vector3 vCenter = m_nodeBox.GetCenter();
            m_brick->m_brickAabb.Translate(-vCenter);
            m_brick->m_brickOrigin = vCenter;
            m_brick->ProcessMeshes(insertions, removals, scratchData);
            ++(m_brick->m_lastUpdated);

            if (!hadBrickData && m_brick->HasBrickData())
            {
                ++m_svoEnv->m_activeVoxels;
            }
        }

        insertions.clear();
        removals.clear();
    }

    AZ::Aabb Voxel::GetChildBBox(AZ::u8 childIndex)
    {
        AZ::u8 x = (childIndex / 4);
        AZ::u8 y = (childIndex - x * 4) / 2;
        AZ::u8 z = (childIndex - x * 4 - y * 2);
        AZ::Vector3 vSize = m_nodeBox.GetExtents() * 0.5f;
        AZ::Vector3 vOffset = vSize;
        vOffset *= AZ::Vector3(x, y, z);
        AZ::Aabb childBox;
        childBox.SetMin(m_nodeBox.GetMin() + vOffset);
        childBox.SetMax(childBox.GetMin() + vSize);
        return childBox;
    }

    void Voxel::AllocateChildren(AZStd::shared_ptr<Voxel> self, float maxSize, float minSize)
    {
        //Do not allocate children if we are at minimal size. 
        if (m_nodeBox.GetZExtent() <= minSize)
        {
            return;
        }

        const EntityMeshDataMap noRemovals;

        for (AZ::u32 childIndex = 0; childIndex < s_numVoxelChildren; ++childIndex)
        {
            //Check if child needs to be allocated. 
            if (!m_children[childIndex])
            {
                AZ::Aabb childBox = GetChildBBox(childIndex);

                //If the child node is not visible skip its creation.
                if (!m_svoEnv->m_camera.IsAABBVisible_E(AZAabbToLyAABB(childBox)))
                {
                    continue;
                }

                m_children[childIndex] = AZStd::make_shared<Voxel>(childBox, self, m_svoEnv, childIndex);

                //Propagate meshes to the child
                if (!m_insertedAndPendingInsertionMeshes.empty())
                {
                    m_children[childIndex]->EnqueueMeshes(m_insertedAndPendingInsertionMeshes, noRemovals, maxSize);
                }
            }
        }
    }

    void Voxel::ReserveGPUMemory(AZStd::shared_ptr<Voxel> self, float maxSize)
    {
        if (!m_svoEnv->m_camera.IsAABBVisible_E(AZAabbToLyAABB(m_nodeBox)))
        {
            // If voxel is not visible, its children aren't either
            return;
        }

        //If the voxel has data but hasn't been uploaded we will check if it needs to be uploaded.

        //Note: due to how the gpu offsets are being calculated we have to allocate blocks to the larger nodes
        //This will need to be fixed later. 
        if (!m_block && ((m_brick && m_brick->HasBrickData()) || (m_boxSize > maxSize)))
        {
            AZ::s32 blockWidth = 1;
            AZ::s32 blockHeight = 1;
            AZ::s32 blockDepth = 1;

            m_blockID = m_svoEnv->m_blockPacker->AddBlock(blockWidth, blockHeight, blockDepth, m_nodeBox);
            if (m_blockID != TextureBlockPacker3D::s_invalidBlockID)
            {
                m_block = m_svoEnv->m_blockPacker->GetBlockInfo(m_blockID);
                AZ_Assert(m_block, "Invalid block ID %d", m_blockID);
            }
            else
            {
                m_svoEnv->m_evictGpu = true;
                return; //Unable to reserve a block. 
            }

            AZStd::shared_ptr<Voxel> parent = m_parentNode.lock();
            if (parent)
            {
                parent->m_childOffsetsDirty = true;
            }
        }

        if (m_block)
        {
            for (AZ::u32 childIndex = 0; childIndex < s_numVoxelChildren; childIndex++)
            {
                AZStd::shared_ptr<Voxel> child = m_children[childIndex];
                if (child)
                {
                    child->ReserveGPUMemory(child, maxSize);
                }
            }
        }
    }

    void Voxel::UpdateGpuTree(AZStd::shared_ptr<Voxel> self)
    {
        // if we don't have a block on the gpu memory we should do nothing.
        if (m_block)
        {
            // Check voxel is not already inside brick update queue
            // to prevent unnecesary texture block writes.
            if (!m_gpuEnqueued)
            {
                bool brickDataDirty = false;
                if (m_brick && m_brick->m_lastUpdated != m_brick->m_lastUploaded)
                {
                    brickDataDirty = true;
                    m_brick->m_lastUploaded.store(m_brick->m_lastUpdated);
                }
                if (m_childOffsetsDirty || brickDataDirty)
                {
                    m_childOffsetsDirty = false;
                    m_svoEnv->m_brickUpdateQueue.push_back(self);
                    m_gpuEnqueued = true;
                }
            }

            for (AZ::u32 childIndex = 0; childIndex < s_numVoxelChildren; childIndex++)
            {
                AZStd::shared_ptr<Voxel> child = m_children[childIndex];
                if (child)
                {
                    child->UpdateGpuTree(child);
                }
            }
        }
    }

    AZ::s32 Voxel::GetOffset()
    {
        return m_block ? m_block->m_atlasOffset : -2;
    }

    void Voxel::UpdateTreeRenderData()
    {
        if (!m_block)
        {
            return;
        }

        Vec3i vOffset(m_block->m_minX, m_block->m_minY, m_block->m_minZ);

        Vec4 treeData[nVoxNodMaxDim * nVoxNodMaxDim * nVoxNodMaxDim];
        memset(treeData, 0x00, sizeof(treeData));

        AZ::s32 childOffsets[s_numVoxelChildren] = { -2, -2, -2, -2, -2, -2, -2, -2 };

        for (AZ::u32 childIndex = 0; childIndex < s_numVoxelChildren; childIndex++)
        {
            if (m_children[childIndex])
            {
                childOffsets[childIndex] = m_children[childIndex]->GetOffset();
            }
        }

        treeData[0] = Vec4(AZVec3ToLYVec3(m_nodeBox.GetMin()), 0);
        treeData[1] = treeData[0] + Vec4(Vec3(1, 1, 1) * m_nodeBox.GetZExtent(), 0);
        treeData[0].w = m_nodeBox.GetZExtent();
        {
            AZStd::shared_ptr<Voxel> parent = m_parentNode.lock();
            treeData[1].w = parent ? (0.1f + (float)parent->GetOffset()) : -2.f;
        }

        for (AZ::s32 c = 0; c < 4; c++)
        {
            if (childOffsets[c + 0] >= 0)
            {
                treeData[2][c] = 0.1f + (float)childOffsets[c + 0];
            }
            else
            {
                treeData[2][c] = -0.1f + (float)childOffsets[c + 0];
            }

            if (childOffsets[c + 4] >= 0)
            {
                treeData[3][c] = 0.1f + (float)childOffsets[c + 4];
            }
            else
            {
                treeData[3][c] = -0.1f + (float)childOffsets[c + 4];
            }
        }

        treeData[4][0] = 0.1f + (float)gEnv->pRenderer->GetFrameID(false);

        gEnv->pRenderer->UpdateTextureInVideoMemory(
            m_svoEnv->m_nTexNodePoolId,
            (AZ::u8*)&treeData[0],
            vOffset.x * nVoxNodMaxDim,
            vOffset.y * nVoxNodMaxDim,
            nVoxNodMaxDim,
            nVoxNodMaxDim,
            eTF_R32G32B32A32F,
            vOffset.z * nVoxNodMaxDim,
            nVoxNodMaxDim);
    }

    void Voxel::UpdateBrickRenderData()
    {
        if (!m_block)
        {
            return;
        }

        if (!m_brick || !m_brick->HasBrickData())
        {
            return;
        }

        //Lock and read what data is currently there.  If a job is half way through processing then it could potentially
        //cause a partial upload, but the job will re-mark the data as dirty for the next possible frame. 
        //This is to avoid writing to the data while it is uploading. 
        AZStd::shared_lock<AZStd::shared_mutex> uploadLock(m_brick->m_brickDataMutex);

        Vec3i vOffset(m_block->m_minX, m_block->m_minY, m_block->m_minZ);

        const byte* pImgRgb = reinterpret_cast<const byte*>(m_brick->m_colors->m_data);
        const byte* pImgNor = reinterpret_cast<const byte*>(m_brick->m_normals->m_data);
        const byte* pImgOpa = reinterpret_cast<const byte*>(m_brick->m_opacities->m_data);

        Vec3i vSizeFin;
        vSizeFin.x = (brickDimension);
        vSizeFin.y = (brickDimension);
        vSizeFin.z = (brickDimension);

        gEnv->pRenderer->UpdateTextureInVideoMemory(
            m_svoEnv->m_nTexRgb0PoolId,
            pImgRgb,
            vOffset.x * nVoxBloMaxDim,
            vOffset.y * nVoxBloMaxDim,
            vSizeFin.x,
            vSizeFin.y,
            m_svoEnv->m_brickTextureFormat,
            vOffset.z * nVoxBloMaxDim,
            vSizeFin.z);

        gEnv->pRenderer->UpdateTextureInVideoMemory(
            m_svoEnv->m_nTexNormPoolId,
            pImgNor,
            vOffset.x * nVoxBloMaxDim,
            vOffset.y * nVoxBloMaxDim,
            vSizeFin.x,
            vSizeFin.y,
            m_svoEnv->m_brickTextureFormat,
            vOffset.z * nVoxBloMaxDim,
            vSizeFin.z);

        gEnv->pRenderer->UpdateTextureInVideoMemory(
            m_svoEnv->m_nTexOpasPoolId,
            pImgOpa,
            vOffset.x * nVoxBloMaxDim,
            vOffset.y * nVoxBloMaxDim,
            vSizeFin.x,
            vSizeFin.y,
            m_svoEnv->m_brickTextureFormat,
            vOffset.z * nVoxBloMaxDim,
            vSizeFin.z);
    }

    float Voxel::GetLodRatio()
    {
        const CCamera& cam = m_svoEnv->m_camera;
        const float dist = m_nodeBox.GetCenter().GetDistance(LYVec3ToAZVec3(cam.GetPosition()));
        return dist / m_boxSize;
    }

    //Debug drawing functionality. 
    void Voxel::DrawVoxels()
    {
        bool drawSelf = true;

        if (GetLodRatio() > m_svoEnv->m_voxelLodCutoff)
        {
            drawSelf = false;
        }

        for (AZ::u32 childIndex = 0; childIndex < s_numVoxelChildren; childIndex++)
        {
            if (m_children[childIndex])
            {
                m_children[childIndex]->DrawVoxels();
            }
        }

        if (drawSelf)
        {
            DrawBrickData();
        }
    }

    void Voxel::DrawBrickData()
    {
        const CCamera& cam = m_svoEnv->m_camera;
        if (cam.IsAABBVisible_F(AZAabbToLyAABB(m_nodeBox)))
        {
            if (m_brick && m_brick->HasBrickData())
            {
                AZStd::shared_lock<AZStd::shared_mutex> readLock(m_brick->m_brickDataMutex);

                AZ::u8 boxLog = static_cast<AZ::u8>(log2(m_boxSize));
                ColorF brickColor;
                ColorF voxelColor = Col_Black;

                switch ((boxLog % 3) + (m_block ? 3 : 0))
                {
                case 0:
                    brickColor = Col_Red;
                    break;
                case 1:
                    brickColor = Col_Lime;
                    break;
                case 2:
                    brickColor = Col_Blue;
                    break;
                case 3:
                    brickColor = Col_Magenta;
                    break;
                case 4:
                    brickColor = Col_Yellow;
                    break;
                case 5:
                    brickColor = Col_Cyan;
                    break;
                default:
                    brickColor = Col_DarkGrey;
                    break;
                }
                gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags(SAuxGeomRenderFlags());
                gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(AZAabbToLyAABB(m_nodeBox), false, voxelColor, eBBD_Faceted);

                for (int x = 0; x < brickDimension; x++)
                {
                    for (int y = 0; y < brickDimension; y++)
                    {
                        for (int z = 0; z < brickDimension; z++)
                        {
                            int id = z * brickDimension * brickDimension + y * brickDimension + x;

                            if ((*m_brick->m_counts)[id] > 0)
                            {
                                AZ::Vector3 vMin = m_nodeBox.GetMin() + (m_nodeBox.GetMax() - m_nodeBox.GetMin()) * AZ::Vector3((float)x / brickDimension, (float)y / brickDimension, (float)z / brickDimension);
                                AZ::Vector3 vMax = m_nodeBox.GetMin() + (m_nodeBox.GetMax() - m_nodeBox.GetMin()) * AZ::Vector3((float)(x + 1) / brickDimension, (float)(y + 1) / brickDimension, (float)(z + 1) / brickDimension);

                                AZ::Aabb brickBox;
                                brickBox.SetMin(vMin);
                                brickBox.SetMax(vMax);

                                bool doNotDraw = false;

                                for (AZ::u32 childIndex = 0; childIndex < s_numVoxelChildren; childIndex++)
                                {
                                    Voxel* child = m_children[childIndex].get();
                                    if (child && child->GetLodRatio() <= m_svoEnv->m_voxelLodCutoff)
                                    {
                                        if (Overlap::AABB_AABB(AZAabbToLyAABB(child->m_nodeBox), AZAabbToLyAABB(brickBox)))
                                        {
                                            doNotDraw = true;
                                        }
                                    }
                                }

                                if (doNotDraw)
                                {
                                    continue;
                                }

                                gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags(SAuxGeomRenderFlags());
                                gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(AZAabbToLyAABB(brickBox), false, brickColor, eBBD_Faceted);
                            }
                        }
                    }
                }
            }
        }
    }

} // namespace SVOGI
