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
#pragma once

struct IStreamable;
struct SCheckOcclusionJobData;
struct SCheckOcclusionOutput;
class COctreeNode;
struct IStatInstGroup;

namespace NAsyncCull
{
    class CCullThread;
}

// Inplace object for IStreamable* to cache StreamableMemoryContentSize
struct SStreamAbleObject
{
    explicit SStreamAbleObject(IStreamable* pObj, bool bUpdateMemUsage = true)
        : m_pObj(pObj)
        , fCurImportance(-1000.f)
    {
        if (pObj && bUpdateMemUsage)
        {
            m_nStreamableContentMemoryUsage = pObj->GetStreamableContentMemoryUsage();
        }
        else
        {
            m_nStreamableContentMemoryUsage = 0;
        }
    }

    bool operator==(const SStreamAbleObject& rOther) const
    {
        return m_pObj == rOther.m_pObj;
    }

    int GetStreamableContentMemoryUsage() const { return m_nStreamableContentMemoryUsage; }
    IStreamable* GetStreamAbleObject() const { return m_pObj; }
    uint32 GetLastDrawMainFrameId() const
    {
        return m_pObj->GetLastDrawMainFrameId();
    }
    float fCurImportance;
private:
    IStreamable* m_pObj;
    int m_nStreamableContentMemoryUsage;
};

struct SObjManPrecacheCamera
{
    SObjManPrecacheCamera()
        : vPosition(ZERO)
        , vDirection(ZERO)
        , bbox(AABB::RESET)
        , fImportanceFactor(1.0f)
    {
    }

    Vec3 vPosition;
    Vec3 vDirection;
    AABB bbox;
    float fImportanceFactor;
};

struct SObjManPrecachePoint
{
    SObjManPrecachePoint()
        : nId(0)
    {
    }

    int nId;
    CTimeValue expireTime;
};

struct IObjManager
{
    virtual ~IObjManager() {}

    virtual void PreloadLevelObjects() = 0;
    virtual void UnloadObjects(bool bDeleteAll) = 0;
    virtual void CheckTextureReadyFlag() = 0;
    virtual void FreeStatObj(IStatObj* pObj) = 0;
    virtual _smart_ptr<IStatObj> GetDefaultCGF() = 0;

    typedef std::vector< IDecalRenderNode* > DecalsToPrecreate;
    virtual DecalsToPrecreate& GetDecalsToPrecreate() = 0;
    virtual PodArray<SStreamAbleObject>& GetArrStreamableObjects() = 0;
    virtual PodArray<SObjManPrecacheCamera>& GetStreamPreCacheCameras() = 0;
    virtual PodArray<COctreeNode*>& GetArrStreamingNodeStack() = 0;
    virtual PodArray<SObjManPrecachePoint>& GetStreamPreCachePointDefs() = 0;

    typedef std::map<string, IStatObj*, stl::less_stricmp<string> > ObjectsMap;
    virtual ObjectsMap& GetNameToObjectMap() = 0;

    typedef std::set<IStatObj*> LoadedObjects;
    virtual LoadedObjects& GetLoadedObjects() = 0;

    virtual Vec3 GetSunColor() = 0;
    virtual void SetSunColor(const Vec3& color) = 0;

    virtual Vec3 GetSunAnimColor() = 0;
    virtual void SetSunAnimColor(const Vec3& color) = 0;

    virtual float GetSunAnimSpeed() = 0;
    virtual void SetSunAnimSpeed(float sunAnimSpeed) = 0;

    virtual AZ::u8 GetSunAnimPhase() = 0;
    virtual void SetSunAnimPhase(AZ::u8 sunAnimPhase) = 0;

    virtual AZ::u8 GetSunAnimIndex() = 0;
    virtual void SetSunAnimIndex(AZ::u8 sunAnimIndex) = 0;

    virtual float GetSSAOAmount() = 0;
    virtual void SetSSAOAmount(float amount) = 0;

    virtual float GetSSAOContrast() = 0;
    virtual void SetSSAOContrast(float amount) = 0;

    virtual SRainParams& GetRainParams() = 0;
    virtual SSnowParams& GetSnowParams() = 0;

    virtual bool IsCameraPrecacheOverridden() = 0;
    virtual void SetCameraPrecacheOverridden(bool state) = 0;

    virtual IStatObj* LoadNewCGF(IStatObj* pObject, int flagCloth, bool bUseStreaming, bool bForceBreakable, unsigned long nLoadingFlags, const char* normalizedFilename, const void* pData, int nDataSize, const char* originalFilename, const char* geomName, IStatObj::SSubObject** ppSubObject) = 0;
    virtual IStatObj* LoadFromCacheNoRef(IStatObj* pObject, bool bUseStreaming, unsigned long nLoadingFlags, const char* geomName, IStatObj::SSubObject** ppSubObject) = 0;

    virtual IStatObj* AllocateStatObj() = 0;
    virtual IStatObj* LoadStatObjUnsafeManualRef(const char* szFileName, const char* szGeomName = NULL, IStatObj::SSubObject** ppSubObject = NULL, bool bUseStreaming = true, unsigned long nLoadingFlags = 0, const void* m_pData = 0, int m_nDataSize = 0, const char* szBlockName = NULL) = 0;
    virtual _smart_ptr<IStatObj> LoadStatObjAutoRef(const char* szFileName, const char* szGeomName = NULL, IStatObj::SSubObject** ppSubObject = NULL, bool bUseStreaming = true, unsigned long nLoadingFlags = 0, const void* m_pData = 0, int m_nDataSize = 0, const char* szBlockName = NULL) = 0;
    virtual void GetLoadedStatObjArray(IStatObj** pObjectsArray, int& nCount) = 0;
    virtual bool InternalDeleteObject(IStatObj* pObject) = 0;
    virtual void MakeShadowCastersList(CVisArea* pReceiverArea, const AABB& aabbReceiver, int dwAllowedTypes, int32 nRenderNodeFlags, Vec3 vLightPos, CDLight* pLight, ShadowMapFrustum* pFr, PodArray<struct SPlaneObject>* pShadowHull, const SRenderingPassInfo& passInfo) = 0;
    virtual int MakeStaticShadowCastersList(IRenderNode* pIgnoreNode, ShadowMapFrustum* pFrustum, int renderNodeExcludeFlags, int nMaxNodes, const SRenderingPassInfo& passInfo) = 0;
    virtual void MakeDepthCubemapRenderItemList(CVisArea* pReceiverArea, const AABB& cubemapAABB, int renderNodeFlags, PodArray<struct IShadowCaster*>* objectsList, const SRenderingPassInfo& passInfo) = 0;
    virtual void PrecacheStatObjMaterial(_smart_ptr<IMaterial> pMaterial, const float fEntDistance, IStatObj* pStatObj, bool bFullUpdate, bool bDrawNear) = 0;
    virtual void PrecacheStatObj(IStatObj* pStatObj, int nLod, const Matrix34A& statObjMatrix, _smart_ptr<IMaterial> pMaterial, float fImportance, float fEntDistance, bool bFullUpdate, bool bHighPriority) = 0;

    virtual int GetLoadedObjectCount() = 0;

    virtual uint16 CheckCachedNearestCubeProbe(IRenderNode* pEnt) = 0;
    virtual int16 GetNearestCubeProbe(IVisArea* pVisArea, const AABB& objBox, bool bSpecular = true) = 0;

    virtual void RenderObject(IRenderNode* o, const AABB& objBox, float fEntDistance, EERType eERType, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter) = 0;
    virtual void RenderDecalAndRoad(IRenderNode* pEnt, const AABB& objBox, float fEntDistance, bool nCheckOcclusion, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter) = 0;
    virtual void RenderObjectDebugInfo(IRenderNode* pEnt, float fEntDistance, const SRenderingPassInfo& passInfo) = 0;
    virtual void RenderAllObjectDebugInfo() = 0;
    virtual void RenderObjectDebugInfo_Impl(IRenderNode* pEnt, float fEntDistance) = 0;
    virtual void RemoveFromRenderAllObjectDebugInfo(IRenderNode* pEnt) = 0;

    virtual float GetXYRadius(int nType, int nSID = DEFAULT_SID) = 0;
    virtual bool GetStaticObjectBBox(int nType, Vec3& vBoxMin, Vec3& vBoxMax, int nSID = DEFAULT_SID) = 0;

    virtual IStatObj* GetStaticObjectByTypeID(int nTypeID, int nSID = DEFAULT_SID) = 0;
    virtual IStatObj* FindStaticObjectByFilename(const char* filename) = 0;

    //virtual float GetBendingRandomFactor() = 0;
    virtual float GetGSMMaxDistance() const = 0;
    virtual void SetGSMMaxDistance(float value) = 0;

    virtual int GetUpdateStreamingPrioriryRoundIdFast() = 0;
    virtual int GetUpdateStreamingPrioriryRoundId() = 0;
    virtual void IncrementUpdateStreamingPrioriryRoundIdFast(int amount) = 0;
    virtual void IncrementUpdateStreamingPrioriryRoundId(int amount) = 0;

    virtual NAsyncCull::CCullThread& GetCullThread() = 0;

    virtual void SetLockCGFResources(bool state) = 0;
    virtual bool IsLockCGFResources() = 0;

    virtual bool IsBoxOccluded(const AABB& objBox, float fDistance, OcclusionTestClient* const __restrict pOcclTestVars, bool bIndoorOccludersOnly, EOcclusionObjectType eOcclusionObjectType, const SRenderingPassInfo& passInfo) = 0;

    virtual void AddDecalToRenderer(float fDistance, _smart_ptr<IMaterial> pMat, const uint8 sortPrio, Vec3 right, Vec3 up, const UCol& ucResCol, const uint8 uBlendType, const Vec3& vAmbientColor, Vec3 vPos, const int nAfterWater, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter) = 0;

    virtual void RegisterForStreaming(IStreamable* pObj) = 0;
    virtual void UnregisterForStreaming(IStreamable* pObj) = 0;
    virtual void UpdateRenderNodeStreamingPriority(IRenderNode* pObj, float fEntDistance, float fImportanceFactor, bool bFullUpdate, const SRenderingPassInfo& passInfo, bool bHighPriority = false) = 0;

    virtual void GetMemoryUsage(class ICrySizer* pSizer) const = 0;
    virtual void GetBandwidthStats(float* fBandwidthRequested) = 0;

    virtual void ReregisterEntitiesInArea(Vec3 vBoxMin, Vec3 vBoxMax) = 0;
    virtual void UpdateObjectsStreamingPriority(bool bSyncLoad, const SRenderingPassInfo& passInfo) = 0;
    virtual void ProcessObjectsStreaming(const SRenderingPassInfo& passInfo) = 0;

    virtual void ProcessObjectsStreaming_Impl(bool bSyncLoad, const SRenderingPassInfo& passInfo) = 0;
    virtual void ProcessObjectsStreaming_Sort(bool bSyncLoad, const SRenderingPassInfo& passInfo) = 0;
    virtual void ProcessObjectsStreaming_Release() = 0;
    virtual void ProcessObjectsStreaming_InitLoad(bool bSyncLoad) = 0;
    virtual void ProcessObjectsStreaming_Finish() = 0;

    // time counters
    virtual bool IsAfterWater(const Vec3& vPos, const SRenderingPassInfo& passInfo) = 0;
    virtual void FreeNotUsedCGFs() = 0;
    virtual void MakeUnitCube() = 0;

    virtual bool CheckOcclusion_TestAABB(const AABB& rAABB, float fEntDistance) = 0;
    virtual bool CheckOcclusion_TestQuad(const Vec3& vCenter, const Vec3& vAxisX, const Vec3& vAxisY) = 0;

    virtual void PushIntoCullQueue(const SCheckOcclusionJobData& rCheckOcclusionData) = 0;
    virtual void PopFromCullQueue(SCheckOcclusionJobData* pCheckOcclusionData) = 0;

    virtual void PushIntoCullOutputQueue(const SCheckOcclusionOutput& rCheckOcclusionOutput) = 0;
    virtual bool PopFromCullOutputQueue(SCheckOcclusionOutput* pCheckOcclusionOutput) = 0;

    virtual void BeginCulling() = 0;
    virtual void RemoveCullJobProducer() = 0;
    virtual void AddCullJobProducer() = 0;

#ifndef _RELEASE
    virtual void CoverageBufferDebugDraw() = 0;
#endif

    virtual bool LoadOcclusionMesh(const char* pFileName) = 0;

    virtual void ClearStatObjGarbage() = 0;
    virtual void CheckForGarbage(IStatObj* pObject) = 0;
    virtual void UnregisterForGarbage(IStatObj* pObject) = 0;

    virtual int GetObjectLOD(const IRenderNode* pObj, float fDistance) = 0;
    virtual bool RayStatObjIntersection(IStatObj* pStatObj, const Matrix34& objMat, _smart_ptr<IMaterial> pMat, Vec3 vStart, Vec3 vEnd, Vec3& vClosestHitPoint, float& fClosestHitDistance, bool bFastTest) = 0;
    virtual bool RayRenderMeshIntersection(IRenderMesh* pRenderMesh, const Vec3& vInPos, const Vec3& vInDir, Vec3& vOutPos, Vec3& vOutNormal, bool bFastTest, _smart_ptr<IMaterial> pMat) = 0;
    virtual bool SphereRenderMeshIntersection(IRenderMesh* pRenderMesh, const Vec3& vInPos, const float fRadius, _smart_ptr<IMaterial> pMat) = 0;

    virtual uint8 GetDissolveRef(float fDist, float fMaxViewDist) = 0;
    virtual float GetLodDistDissolveRef(SLodDistDissolveTransitionState* pState, float curDist, int nNewLod, const SRenderingPassInfo& passInfo) = 0;

    virtual void CleanStreamingData() = 0;
    virtual IRenderMesh* GetRenderMeshBox() = 0;

    virtual void PrepareCullbufferAsync(const CCamera& rCamera) = 0;
    virtual void BeginOcclusionCulling(const SRenderingPassInfo& passInfo) = 0;
    virtual void EndOcclusionCulling(bool waitForOcclusionJobCompletion = false) = 0;
    virtual void RenderBufferedRenderMeshes(const SRenderingPassInfo& passInfo) = 0;

    virtual int GetListStaticTypesCount() = 0;
    virtual int GetListStaticTypesGroupCount(int typeId) = 0;
    virtual IStatInstGroup* GetIStatInstGroup(int typeId, int groupId) = 0;

    virtual int IncrementNextPrecachePointId() = 0;
};
