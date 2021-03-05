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


#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Asset/AssetCommon.h>

#include <AzCore/Jobs/Job.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <LmbrCentral/Rendering/MeshAsset.h>
#include "IRenderer.h"//Because IShader.h won't compile without this.
#include "IShader.h"
#include "IStatObj.h"
#include "I3DEngine.h"
#include "ITexture.h"


#include "SvoBrick.h"


/*
  These clases represent a sparse voxel octree.
  The SvoEnvironment contains the tree root and associated meta data.  It handles the entry into the tree
  and provides the interface for the system module.
  The Voxel class represents the nodes of the sparse voxel octree.
  Each voxel contains a texture block structer and bricks of data to be used with cone tracing.
  This represents the basic brick map algorithm.

*/
namespace SVOGI
{


#define nAtlasDimMaxXY (SvoEnvironment::m_brickTexturePoolDimXY / nVoxBloMaxDim)
#define nAtlasDimMaxZ (SvoEnvironment::m_brickTexturePoolDimZ/ nVoxBloMaxDim)

#define nAtlasDimBriXY (SvoEnvironment::m_brickTexturePoolDimXY / brickDimension)
#define nAtlasDimBriZ (SvoEnvironment::m_brickTexturePoolDimZ/ brickDimension)

#define nVoxNodPoolDimXY (nVoxNodMaxDim * nAtlasDimMaxXY)
#define nVoxNodPoolDimZ (nVoxNodMaxDim * nAtlasDimMaxZ)

    const static AZ::u32 s_bufferCount = 2;
    const static AZ::u32 s_numVoxelChildren = 8;

    class Voxel
    {
    public:
        AZ_CLASS_ALLOCATOR(Voxel, AZ::SystemAllocator, 0);

        Voxel(const AZ::Aabb& box, AZStd::shared_ptr<Voxel> parent, class SvoEnvironment* env, AZ::u8 childIndex);
        ~Voxel();

        //Recursive functions.
        void Update(AZStd::deque<AZStd::shared_ptr<Voxel>>& processingQueue, AZStd::shared_ptr<Voxel> self, float maxSize, float minSize);
        void ReserveGPUMemory(AZStd::shared_ptr<Voxel> self, float maxSize);
        void UpdateGpuTree(AZStd::shared_ptr<Voxel> self);

        void Evict(AZ::u32 frameDelay, bool forceEvict, float minSize);
        void EvictGpuData(AZ::u32 frameDelay, bool forceEvict);

        //Debug drawing functionality.
        void DrawVoxels();

        void EnqueueMeshes(const EntityMeshDataMap& insertions, const EntityMeshDataMap& removals, float maxSize);

        //Non-recursive functions.
        void UpdateBrickData(float maxSize, AZ::s32 maxLoadedNodes, DataBrick<GISubVoxels>& scratchData);
        AZ::s32 GetOffset();
        AZ::Aabb GetChildBBox(AZ::u8 childIndex);
        void AllocateChildren(AZStd::shared_ptr<Voxel> self, float maxSize, float minSize);
        void UpdateBrickRenderData();
        void UpdateTreeRenderData();
        void DrawBrickData();
        float GetLodRatio();
        void ReleaseBlock();

        AZ::Aabb m_nodeBox;
        EntityMeshDataMap m_insertedAndPendingInsertionMeshes;
        EntityMeshDataMap m_insertions[s_bufferCount];
        EntityMeshDataMap m_removals[s_bufferCount];
        AZ::u32 m_queueId;
        AZStd::mutex m_queueMutex;

        AZStd::shared_ptr<Voxel> m_children[s_numVoxelChildren];
        AZStd::weak_ptr<Voxel> m_parentNode;

        class Brick* m_brick;
        AZ::s32 m_blockID;
        struct TextureBlock3D* m_block;
        class SvoEnvironment* m_svoEnv;

        float m_boxSize;

        AZ::s32 m_lastVisibleFrameId;
        AZ::s32 m_lastUpdatedFrameId;

        bool m_childOffsetsDirty; //This flag will be set when a child changes its gpu mapping.
        AZStd::atomic_bool m_cpuEnqueued; //This flag is set to prevent double enqueuing for voxel processing (m_processingQueue).
        bool m_gpuEnqueued; //This flag is set to prevent double enqueuing for voxel's brick update (m_brickUpdateQueue).
        AZ::u8 m_childIndex;
    };

    class SvoEnvironment
    {
    public:
        AZ_CLASS_ALLOCATOR(SvoEnvironment, AZ::SystemAllocator, 0);

        SvoEnvironment();
        ~SvoEnvironment();

        //These functions are called from the SVO renderer to get associated data for rendering.
        //Thus they will be invoked from the render thread currently.
        void GetSvoStaticTextures(I3DEngine::SSvoStaticTexInfo& svoInfo, PodArray<I3DEngine::SLightTI>* pLightsTI_S, PodArray<I3DEngine::SLightTI>* pLightsTI_D);
        void GetSvoBricksForUpdate(PodArray<I3DEngine::SSvoNodeInfo>& arrNodeInfo, bool getDynamic);

        //This function can be called from any thread as it will only upload data.
        //Currently it should be invoked from the Main thread. 
        void UploadVoxels(bool showVoxels);

        //These functions are intended to be called from the same thread 
        //These functions can modify the svo structure.  
        //Currently they should be invoked from the main thread. 
        void UpdateVoxels();
        void ProcessVoxels();
        void EvictVoxels();
        void EvictGpuData();

        void ReconstructTree();

        void SetCamera(const CCamera& newCam);
        void AllocateTexturePools();
        void DeallocateTexturePools();

        void CollectLights();

        void UpsertMesh(AZ::EntityId entityId, AZ::Transform transform, AZ::Aabb worldAabb,
            AZ::Data::Asset<LmbrCentral::MeshAsset> meshAsset, _smart_ptr<IMaterial> material);

        void RemoveMesh(AZ::EntityId entityId);

        //Camera for culling Voxels
        CCamera m_camera;
        //Class for handling data upload to GPU
        class TextureBlockPacker3D* m_blockPacker;

        PodArray<I3DEngine::SLightTI> m_lightsTI_S, m_lightsTI_D;
        AZStd::shared_ptr<Voxel> m_svoRoot;

        //Queue for voxels that need to have CPU data generated.
        AZStd::deque<AZStd::shared_ptr<Voxel>> m_processingQueue;

        //Queues for voxels that need to have GPU data modified.
        AZStd::deque<AZStd::shared_ptr<Voxel>> m_brickUpdateQueue;
        
        EntityMeshDataMap m_globalInsertedMeshes;
        EntityMeshDataMap m_globalInsertions;
        EntityMeshDataMap m_globalRemovals;

        AZ::JobContext* m_jobContext;
        AZ::JobCompletion m_voxelJobsCompletion;
        ITexture* m_globalSpecularCM;
        float m_globalSpecularCM_Mult;
        double m_prevCheckVal;

        float m_voxelLodCutoff;

        const static AZ::s32 s_uninitializedTexturePoolId;

        AZ::s32 m_nTexOpasPoolId = s_uninitializedTexturePoolId;
        AZ::s32 m_nTexNodePoolId = s_uninitializedTexturePoolId;
        AZ::s32 m_nTexRgb0PoolId = s_uninitializedTexturePoolId;
        AZ::s32 m_nTexRgb1PoolId = s_uninitializedTexturePoolId;
        AZ::s32 m_nTexDynlPoolId = s_uninitializedTexturePoolId;
        AZ::s32 m_nTexRgb2PoolId = s_uninitializedTexturePoolId;
        AZ::s32 m_nTexRgb3PoolId = s_uninitializedTexturePoolId;
        AZ::s32 m_nTexNormPoolId = s_uninitializedTexturePoolId;
        AZ::s32 m_nTexAldiPoolId = s_uninitializedTexturePoolId;

        AZ::u32 m_blockIndex;

        //Counter to control how many voxels are actively in flight and have data. 
        AZStd::atomic_int m_activeVoxels;

        //Delay to run eviction on the tree
        const AZ::s32 m_delayToEvictInFrames = 120;
        AZ::s32 m_lastEvictionFrame = 0; // Used to track when to run eviction on the tree and prune it by removing older nodes.

        //When evicting, toss anything older than this number of frames
        const AZ::s32 m_evictionDelayInFrames = 60;
        const AZ::s32 m_blockUpdateDelayInFrames = 60;

        //Delay to warn users that the maximum number of bricks on CPU is not high enough
        const AZ::s32 m_delayToWarnReachingMaxBricksOnCPUInFrames = 30;
        AZ::s32 m_numFramesReachingMaxBricksOnCPU = 0;

        //Currently set to just RGBA8 but should eventually be used to control compression.
        ETEX_Format m_brickTextureFormat;

        bool m_evictGpu;

        static AZ::s32 m_currentPassFrameId;
        static AZ::s32 m_brickTexturePoolDimXY;
        static AZ::s32 m_brickTexturePoolDimZ;

    protected:

        void AllocateTexturePool(AZ::s32& texPoolId, AZ::s32 width, AZ::s32 height, AZ::s32 depth, ETEX_Format texFormat, AZ::s32 filter, AZ::s32 flags = 0);
        void DeallocateTexturePool(AZ::s32& texPoolId);
    };

    inline AZ::s32 GetCurrPassMainFrameID() { return SvoEnvironment::m_currentPassFrameId; }
}