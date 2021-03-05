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

// Description : Draws geometry caches


#ifndef CRYINCLUDE_CRY3DENGINE_GEOMCACHERENDERNODE_H
#define CRYINCLUDE_CRY3DENGINE_GEOMCACHERENDERNODE_H
#pragma once

#if defined(USE_GEOM_CACHES)

#include "GeomCache.h"
#include "GeomCacheDecoder.h"

struct SGeomCacheRenderMeshUpdateContext
{
    SGeomCacheRenderMeshUpdateContext()
        : m_meshId(0)
        , m_pRenderMesh(NULL)
        , m_pUpdateState(NULL)
        , m_pIndices(NULL) {}

    // Information needed to create the render mesh each frame
    uint m_meshId;

    // The render mesh
    _smart_ptr<IRenderMesh> m_pRenderMesh;

    // Locks the render mesh from rendering until it was filled
    volatile int* m_pUpdateState;

    // Previous positions for motion blur
    stl::aligned_vector<Vec3, 16> m_prevPositions;

    // Data pointers for updating
    vtx_idx* m_pIndices;
    strided_pointer<Vec3> m_pPositions;
    strided_pointer<UCol> m_pColors;
    strided_pointer<Vec2> m_pTexcoords;
    strided_pointer<SPipTangents> m_pTangents;
    strided_pointer<Vec3> m_pVelocities;
};

struct SGeomCacheRenderElementData
{
    CREGeomCache* m_pRenderElement;
    volatile int* m_pUpdateState;
    int m_threadId;
    DynArray<CREGeomCache::SMeshRenderData>* m_pCurrentFillData;
};

class CGeomCacheRenderNode
    : public IGeomCacheRenderNode
    , public IGeomCacheListener
    , public Cry3DEngineBase
{
public:
    CGeomCacheRenderNode();
    virtual ~CGeomCacheRenderNode();

    virtual const char* GetName() const;
    virtual const char* GetEntityClassName() const;
    virtual EERType GetRenderNodeType() { return eERType_GeomCache; }

    virtual Vec3 GetPos(bool bWorldOnly) const;
    virtual void SetBBox(const AABB& WSBBox);
    virtual const AABB GetBBox() const;
    virtual void GetLocalBounds(AABB& bbox);

    // Called before rendering to update to current frame bbox
    void UpdateBBox();

    virtual void Render(const struct SRendParams& entDrawParams, const SRenderingPassInfo& passInfo);

    void SetMatrix(const Matrix34& matrix);
    const Matrix34& GetMatrix() const { return m_matrix; }

    virtual void SetMaterial(_smart_ptr<IMaterial> pMat);
    virtual _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos);
    virtual _smart_ptr<IMaterial> GetMaterialOverride() { return m_pMaterial; }

    void SetBaseMaxViewDistance(float maxViewDistance) override { m_maxViewDist = maxViewDistance; }
    virtual float GetMaxViewDist();

    virtual void GetMemoryUsage(ICrySizer* pSizer) const;

    // Streaming
    float GetStreamingTime() const { return std::max(m_streamingTime, m_playbackTime); }

    // Called for starting the update job in CGeomCacheManager
    void StartAsyncUpdate();

    // Called by fill job if it didn't call FillFrameAsync because data wasn't available
    void SkipFrameFill();

    // Called from the update job in CGeomCacheManager
    bool FillFrameAsync(const char* const pFloorFrameData, const char* const pCeilFrameData, const float lerpFactor);

    // Called from FillFrameAsync
    void UpdateMesh_JobEntry(SGeomCacheRenderMeshUpdateContext *pUpdateContext, const SGeomCacheStaticMeshData *pStaticMeshData,
        const char* pFloorMeshData, const char* pCeilMeshData, float lerpFactor);

    // Called from CGeomCacheManager when playback stops
    void ClearFillData();

    // Called from CObjManager to update streaming
    void UpdateStreamableComponents(float fImportance, float fDistance, bool bFullUpdate, int nLod, const float fInvScale, bool bDrawNear);

    // IGeomCacheRenderNode
    virtual bool LoadGeomCache(const char* sGeomCacheFileName);
    void SetGeomCache(_smart_ptr<IGeomCache> geomCache) override;

    virtual void SetPlaybackTime(const float time);
    virtual float GetPlaybackTime() const { return m_playbackTime; }

    virtual bool IsStreaming() const;
    virtual void StartStreaming(const float time);
    virtual void StopStreaming();
    virtual bool IsLooping() const;
    virtual void SetLooping(const bool bEnable);
    virtual float GetPrecachedTime() const;

    virtual IGeomCache* GetGeomCache() const { return m_pGeomCache; }

    virtual bool DidBoundsChange();

    virtual void SetDrawing(bool bDrawing) { m_bDrawing = bDrawing; }

    // Set stand in CGFs and distance
    virtual void SetStandIn(const char* pFilePath, const char* pMaterial);
    IStatObj* GetStandIn() override { return m_pStandIn; }
    virtual void SetFirstFrameStandIn(const char* pFilePath, const char* pMaterial);
    IStatObj* GetFirstFrameStandIn() override { return m_pFirstFrameStandIn; }
    virtual void SetLastFrameStandIn(const char* pFilePath, const char* pMaterial);
    IStatObj* GetLastFrameStandIn() override { return m_pLastFrameStandIn; }
    virtual void SetStandInDistance(const float distance);
    float GetStandInDistance() override { return m_standInDistance; }

    // Set distance at which cache will start streaming automatically (0 means no auto streaming)
    virtual void SetStreamInDistance(const float distance);
    float GetStreamInDistance() override { return m_streamInDistance; }

    virtual void DebugDraw(const SGeometryDebugDrawInfo& info, float fExtrudeScale, uint nodeIndex) const;
    virtual bool RayIntersection(SRayHitInfo& hitInfo, _smart_ptr<IMaterial> pCustomMtl, uint* pHitNodeIndex) const;

    // Get node information
    virtual uint GetNodeCount() const;
    virtual Matrix34 GetNodeTransform(const uint nodeIndex) const;
    virtual const char* GetNodeName(const uint nodeIndex) const;
    virtual uint32 GetNodeNameHash(const uint nodeIndex) const;
    virtual bool IsNodeDataValid(const uint nodeIndex) const;

    // Physics
    virtual void InitPhysicalEntity(IPhysicalEntity* pPhysicalEntity, const pe_articgeomparams& params);

    void OffsetPosition([[maybe_unused]] const Vec3& delta) {}

#ifndef _RELEASE
    void DebugRender();
#endif

private:
    void CalcBBox();

    void FillRenderObject(const SRendParams& rendParams, const SRenderingPassInfo& passInfo, _smart_ptr<IMaterial> pMaterial, CRenderObject* pRenderObject);

    bool Initialize();
    bool InitializeRenderMeshes();
    _smart_ptr<IRenderMesh> SetupDynamicRenderMesh(SGeomCacheRenderMeshUpdateContext& updateContext);

    void Clear(bool bWaitForStreamingJobs);

    void InitTransformsRec(uint& currentNodeIndex, const std::vector<SGeomCacheStaticNodeData>& staticNodeData, const QuatTNS& currentTransform);

    void UpdateTransformsRec(uint& currentNodeIndex, uint& currentMeshIndex, const std::vector<SGeomCacheStaticNodeData>& staticNodeData,
        const std::vector<SGeomCacheStaticMeshData>& staticMeshData, uint& currentNodeDataOffset, const char* const pFloorNodeData,
        const char* const pCeilNodeData, const QuatTNS& currentTransform, const float lerpFactor);

    // IGeomCacheListener
    virtual void OnGeomCacheStaticDataLoaded();
    virtual void OnGeomCacheStaticDataUnloaded();

    void DebugDrawRec(const SGeometryDebugDrawInfo& info, float fExtrudeScale,
        uint& currentNodeIndex, const std::vector<SGeomCacheStaticNodeData>& staticNodeData) const;
    bool RayIntersectionRec(SRayHitInfo& hitInfo, _smart_ptr<IMaterial> pCustomMtl, uint* pHitNodeIndex,
        uint& currentNodeIndex, const std::vector<SGeomCacheStaticNodeData>& staticNodeData,
        SRayHitInfo& hitOut, float& fMinDistance) const;

#ifndef _RELEASE
    void InstancingDebugDrawRec(uint& currentNodeIndex, const std::vector<SGeomCacheStaticNodeData>& staticNodeData);
#endif

    enum EStandInType
    {
        eStandInType_None,
        eStandInType_Default,
        eStandInType_FirstFrame,
        eStandInType_LastFrame
    };

    EStandInType SelectStandIn() const;
    IStatObj* GetStandIn(const EStandInType type) const;

    void PrecacheStandIn(IStatObj* pStandIn, float fImportance, float fDistance, bool bFullUpdate, int nLod, const float fInvScale, bool bDrawNear);

    void UpdatePhysicalEntity(const pe_articgeomparams* pParams);
    void UpdatePhysicalMaterials();

    // Material ID -> render element data + update state pointer
    typedef AZStd::unordered_map<uint32, SGeomCacheRenderElementData> TRenderElementMap;
    TRenderElementMap m_pRenderElements;

    // Saved node transforms for motion blur and attachments
    std::vector<Matrix34> m_nodeMatrices;

    // All render meshes
    std::vector<_smart_ptr<IRenderMesh> > m_renderMeshes;

    // Update contexts for render meshes
    std::vector<SGeomCacheRenderMeshUpdateContext> m_renderMeshUpdateContexts;

    // Override material
    _smart_ptr<IMaterial> m_pMaterial;

    // The rendered cache
    _smart_ptr<CGeomCache> m_pGeomCache;

    // World space matrix
    Matrix34 m_matrix;

    // Playback
    volatile float m_playbackTime;

    // Streaming flag
    volatile float m_streamingTime;

    // Misc
    IPhysicalEntity* m_pPhysicalEntity;
    float m_maxViewDist;

    // World space bounding box
    AABB m_bBox;

    // AABB of current displayed frame and render buffer
    AABB m_currentAABB;
    AABB m_currentDisplayAABB;

    // Used for editor debug rendering & ray intersection
    mutable CryCriticalSection m_fillCS;

    // Transform ready sync
    mutable CryMutex m_bTransformsReadyCS;
    mutable CryConditionVariable m_bTransformReadyCV;

    // Stand in stat objects
    EStandInType m_standInVisible;
    _smart_ptr<IStatObj> m_pStandIn;
    _smart_ptr<IStatObj> m_pFirstFrameStandIn;
    _smart_ptr<IStatObj> m_pLastFrameStandIn;
    float m_standInDistance;

    // Distance at which render node will automatically start streaming
    float m_streamInDistance;

    // Flags
    volatile bool m_bInitialized;
    bool m_bLooping;
    volatile bool m_bIsStreaming;
    bool m_bFilledFrameOnce;
    bool m_bBoundsChanged;
    bool m_bDrawing;
    bool m_bTransformReady;
};

#endif
#endif // CRYINCLUDE_CRY3DENGINE_GEOMCACHERENDERNODE_H
