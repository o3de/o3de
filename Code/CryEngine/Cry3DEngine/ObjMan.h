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

#ifndef CRYINCLUDE_CRY3DENGINE_OBJMAN_H
#define CRYINCLUDE_CRY3DENGINE_OBJMAN_H
#pragma once


#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>

#include "StatObj.h"
#include "../RenderDll/Common/Shadow_Renderer.h"

#include "StlUtils.h"
#include "cbuffer.h"
#include "CZBufferCuller.h"
#include "PoolAllocator.h"
#include "CCullThread.h"
#include <StatObjBus.h>
#include <IObjManager.h>

#include <map>
#include <vector>

#define ENTITY_MAX_DIST_FACTOR 100
#define MAX_VALID_OBJECT_VOLUME (10000000000.f)
#define DEFAULT_CGF_NAME ("engineassets/objects/default.cgf")

struct IStatObj;
struct IIndoorBase;
struct IRenderNode;
struct ISystem;
struct IDecalRenderNode;
struct SCheckOcclusionJobData;
struct SCheckOcclusionOutput;
struct CVisArea;

class CVegetation;

class C3DEngine;
struct IMaterial;

#define SMC_EXTEND_FRUSTUM 8
#define SMC_SHADOW_FRUSTUM_TEST 16

#define OCCL_TEST_HEIGHT_MAP    1
#define OCCL_TEST_CBUFFER           2
#define OCCL_TEST_INDOOR_OCCLUDERS_ONLY     4
#define POOL_STATOBJ_ALLOCS

//! contains stat obj instance group properties (vegetation object properties)
struct StatInstGroup
    : public IStatInstGroup
{
    StatInstGroup()
    {
        pStatObj = nullptr;
    }

    void Update(struct CVars* pCVars, int nGeomDetailScreenRes);
    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const{}
};

struct SExportedBrushMaterial
{
    int size;
    char material[64];
};

struct SRenderMeshInfoOutput
{
    SRenderMeshInfoOutput() { memset(this, 0, sizeof(*this)); }
    _smart_ptr<IRenderMesh> pMesh;
    _smart_ptr<IMaterial> pMat;
};

struct SObjManRenderDebugInfo
{
    SObjManRenderDebugInfo(IRenderNode* _pEnt, float _fEntDistance)
        : pEnt(_pEnt)
        , fEntDistance(_fEntDistance) {}

    IRenderNode* pEnt;
    float fEntDistance;
};

//////////////////////////////////////////////////////////////////////////
class CObjManager
    : public Cry3DEngineBase
    , public IObjManager
    , private StatInstGroupEventBus::Handler
{
public:
    enum
    {
        MaxPrecachePoints = 4,
    };
    //! The maximum number of objects pending garbage collection before cleanup is forced
    //! in the current frame instead of delayed until loading has completed.
    //! This helps reduce spikes when cleaning up render objects.
    static const size_t s_maxPendingGarbageObjects = 250;

public:
    CObjManager();
    virtual ~CObjManager();

    void PreloadLevelObjects();
    void UnloadObjects(bool bDeleteAll);

    void CheckTextureReadyFlag();

    IStatObj* AllocateStatObj();
    void FreeStatObj(IStatObj* pObj);
    virtual _smart_ptr<IStatObj> GetDefaultCGF() { return m_pDefaultCGF; }

    virtual SRainParams& GetRainParams() override { return m_rainParams; }
    virtual SSnowParams& GetSnowParams() override { return m_snowParams;  }

    template <class T>
    static int GetItemId(std::vector<T*>* pArray, T* pItem, [[maybe_unused]] bool bAssertIfNotFound = true)
    {
        for (uint32 i = 0, end = pArray->size(); i < end; ++i)
        {
            if ((*pArray)[i] == pItem)
            {
                return i;
            }
        }
        return -1;
    }

    template <class T>
    static T* GetItemPtr(std::vector<T*>* pArray, int nId)
    {
        if (nId < 0)
        {
            return NULL;
        }

        assert(nId < (int)pArray->size());

        if (nId < (int)pArray->size())
        {
            return (*pArray)[nId];
        }
        else
        {
            return NULL;
        }
    }


    template <class T>
    static int GetItemId(std::vector<_smart_ptr<T> >* pArray, _smart_ptr<T> pItem, [[maybe_unused]] bool bAssertIfNotFound = true)
    {
        for (uint32 i = 0, end = pArray->size(); i < end; ++i)
        {
            if ((*pArray)[i] == pItem)
            {
                return i;
            }
        }
        return -1;
    }

    template <class T>
    static _smart_ptr<T> GetItemPtr(std::vector<_smart_ptr<T> >* pArray, int nId)
    {
        if (nId < 0)
        {
            return NULL;
        }

        assert(nId < (int)pArray->size());

        if (nId < (int)pArray->size())
        {
            return (*pArray)[nId];
        }
        else
        {
            return NULL;
        }
    }

    //! Loads a static object from a CGF file.  Does not increment the static object's reference counter.  The reference returned is not guaranteed to be valid unless run on the same thread running the garbage collection.  Best used for priming the cache
    virtual IStatObj* LoadStatObjUnsafeManualRef(const char* szFileName, const char* szGeomName = NULL, IStatObj::SSubObject** ppSubObject = NULL, bool bUseStreaming = true, unsigned long nLoadingFlags = 0, const void* m_pData = 0, int m_nDataSize = 0, const char* szBlockName = NULL) override;

    //! Loads a static object from a CGF file.  Increments the static object's reference counter.  This method is threadsafe.  Not suitable for preloading
    _smart_ptr<IStatObj> LoadStatObjAutoRef(const char* szFileName, const char* szGeomName = NULL, IStatObj::SSubObject** ppSubObject = NULL, bool bUseStreaming = true, unsigned long nLoadingFlags = 0, const void* m_pData = 0, int m_nDataSize = 0, const char* szBlockName = NULL);

private:

    template<typename T>
    T LoadStatObjInternal(const char* filename, const char* _szGeomName, IStatObj::SSubObject** ppSubObject, bool bUseStreaming, unsigned long nLoadingFlags, const void* pData, int nDataSize, const char* szBlockName);

    template <size_t SIZE_IN_CHARS>
    void NormalizeLevelName(const char* filename, char(&normalizedFilename)[SIZE_IN_CHARS]);

    void LoadDefaultCGF(const char* filename, unsigned long nLoadingFlags);
    virtual IStatObj* LoadNewCGF(IStatObj* pObject, int flagCloth, bool bUseStreaming, bool bForceBreakable, unsigned long nLoadingFlags, const char* normalizedFilename, const void* pData, int nDataSize, const char* originalFilename, const char* geomName, IStatObj::SSubObject** ppSubObject) override;

    virtual IStatObj* LoadFromCacheNoRef(IStatObj* pObject, bool bUseStreaming, unsigned long nLoadingFlags, const char* geomName, IStatObj::SSubObject** ppSubObject) override;

    PodArray<PodArray<StatInstGroup> > m_lstStaticTypes;

public:

    void GetLoadedStatObjArray(IStatObj** pObjectsArray, int& nCount);

    // Deletes object.
    // Only should be called by Release function of IStatObj.
    virtual bool InternalDeleteObject(IStatObj* pObject) override;

    PodArray<PodArray<StatInstGroup> >& GetListStaticTypes() { return m_lstStaticTypes; }
    int GetListStaticTypesCount() override { return m_lstStaticTypes.Count(); }
    int GetListStaticTypesGroupCount(int typeId) override { return m_lstStaticTypes[typeId].Count(); }
    IStatInstGroup* GetIStatInstGroup(int typeId, int groupId) override { return &m_lstStaticTypes[typeId][groupId]; }

    void MakeShadowCastersList(CVisArea* pReceiverArea, const AABB& aabbReceiver,
        int dwAllowedTypes, int32 nRenderNodeFlags, Vec3 vLightPos, CDLight* pLight, ShadowMapFrustum* pFr, PodArray<struct SPlaneObject>* pShadowHull, const SRenderingPassInfo& passInfo);

    int MakeStaticShadowCastersList(IRenderNode* pIgnoreNode, ShadowMapFrustum* pFrustum, int renderNodeExcludeFlags, int nMaxNodes, const SRenderingPassInfo& passInfo);

    void MakeDepthCubemapRenderItemList(CVisArea* pReceiverArea, const AABB& cubemapAABB, int renderNodeFlags, PodArray<struct IShadowCaster*>* objectsList, const SRenderingPassInfo& passInfo);

    // decal pre-caching
    DecalsToPrecreate m_decalsToPrecreate;

    void PrecacheStatObjMaterial(_smart_ptr<IMaterial> pMaterial, const float fEntDistance, IStatObj* pStatObj, bool bFullUpdate, bool bDrawNear);
    virtual DecalsToPrecreate& GetDecalsToPrecreate() override { return m_decalsToPrecreate; }

    void PrecacheStatObj(IStatObj* pStatObj, int nLod, const Matrix34A& statObjMatrix, _smart_ptr<IMaterial> pMaterial, float fImportance, float fEntDistance, bool bFullUpdate, bool bHighPriority);

    virtual PodArray<SStreamAbleObject>& GetArrStreamableObjects() override { return m_arrStreamableObjects; }
    virtual PodArray<SObjManPrecacheCamera>& GetStreamPreCacheCameras() override { return m_vStreamPreCacheCameras; }

    virtual Vec3 GetSunColor() override { return m_vSunColor; }
    virtual void SetSunColor(const Vec3& color) override { m_vSunColor = color; }

    Vec3 GetSunAnimColor() override { return m_sunAnimColor; }
    void SetSunAnimColor(const Vec3& color) override { m_sunAnimColor = color; }

    float GetSunAnimSpeed() override { return m_sunAnimSpeed; }
    void SetSunAnimSpeed(float sunAnimSpeed) override { m_sunAnimSpeed = sunAnimSpeed; }

    AZ::u8 GetSunAnimPhase() override { return m_sunAnimPhase; }
    void SetSunAnimPhase(AZ::u8 sunAnimPhase) override { m_sunAnimPhase = sunAnimPhase; }

    AZ::u8 GetSunAnimIndex() override { return m_sunAnimIndex; }
    void SetSunAnimIndex(AZ::u8 sunAnimIndex) override { m_sunAnimIndex = sunAnimIndex; }

    virtual float GetSSAOAmount() override { return m_fSSAOAmount;  }
    virtual void SetSSAOAmount(float amount) override { m_fSSAOAmount = amount; }

    virtual float GetSSAOContrast() override { return m_fSSAOContrast; }
    virtual void SetSSAOContrast(float amount) override { m_fSSAOContrast = amount; }

    virtual bool IsCameraPrecacheOverridden() override { return m_bCameraPrecacheOverridden; }
    virtual void SetCameraPrecacheOverridden(bool state) override { m_bCameraPrecacheOverridden = state; }

    virtual ObjectsMap& GetNameToObjectMap() override { return m_nameToObjectMap; }
    virtual LoadedObjects& GetLoadedObjects() override { return m_lstLoadedObjects; }

    //////////////////////////////////////////////////////////////////////////

    ObjectsMap m_nameToObjectMap;
    LoadedObjects m_lstLoadedObjects;

    /// Thread-safety for async requests to CreateInstance.
    // Always take this lock before m_garbageMutex if taking both
    AZStd::recursive_mutex m_loadMutex;

public:
    int GetLoadedObjectCount() { return m_lstLoadedObjects.size(); }

    uint16 CheckCachedNearestCubeProbe(IRenderNode* pEnt)
    {
        if (pEnt->m_pRNTmpData)
        {
            CRNTmpData::SRNUserData& pUserDataRN = pEnt->m_pRNTmpData->userData;

            const uint16 nCacheClearThreshold = 32;
            ++pUserDataRN.nCubeMapIdCacheClearCounter;
            pUserDataRN.nCubeMapIdCacheClearCounter &= (nCacheClearThreshold - 1);

            if (pUserDataRN.nCubeMapId && pUserDataRN.nCubeMapIdCacheClearCounter)
            {
                return pUserDataRN.nCubeMapId;
            }
        }

        // cache miss
        return 0;
    }

    int16 GetNearestCubeProbe(IVisArea* pVisArea, const AABB& objBox, bool bSpecular = true);

    void RenderObject(IRenderNode* o,
        const AABB& objBox,
        float fEntDistance,
        EERType eERType,
        const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter);

    void RenderDecalAndRoad(IRenderNode* pEnt,
        const AABB& objBox, float fEntDistance,
        bool nCheckOcclusion, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter);

    void RenderObjectDebugInfo(IRenderNode* pEnt, float fEntDistance, const SRenderingPassInfo& passInfo);
    void RenderAllObjectDebugInfo();
    void RenderObjectDebugInfo_Impl(IRenderNode* pEnt, float fEntDistance);
    void RemoveFromRenderAllObjectDebugInfo(IRenderNode* pEnt);

    float GetXYRadius(int nType, int nSID = GetDefSID());
    bool GetStaticObjectBBox(int nType, Vec3& vBoxMin, Vec3& vBoxMax, int nSID = GetDefSID());

    IStatObj* GetStaticObjectByTypeID(int nTypeID, int nSID = GetDefSID());
    IStatObj* FindStaticObjectByFilename(const char* filename);

    //float GetBendingRandomFactor() override;
    int GetUpdateStreamingPrioriryRoundIdFast() override { return m_nUpdateStreamingPrioriryRoundIdFast; }
    int GetUpdateStreamingPrioriryRoundId() override { return m_nUpdateStreamingPrioriryRoundId; };
    virtual void IncrementUpdateStreamingPrioriryRoundIdFast(int amount) override { m_nUpdateStreamingPrioriryRoundIdFast += amount; }
    virtual void IncrementUpdateStreamingPrioriryRoundId(int amount) override { m_nUpdateStreamingPrioriryRoundId += amount; }

    virtual void SetLockCGFResources(bool value) override { m_bLockCGFResources = value; }
    virtual bool IsLockCGFResources() override { return m_bLockCGFResources != 0; }


    bool IsBoxOccluded(const AABB& objBox,
        float fDistance,
        OcclusionTestClient* const __restrict pOcclTestVars,
        bool bIndoorOccludersOnly,
        EOcclusionObjectType eOcclusionObjectType,
        const SRenderingPassInfo& passInfo);

    void AddDecalToRenderer(float fDistance,
        _smart_ptr<IMaterial> pMat,
        const uint8 sortPrio,
        Vec3 right,
        Vec3 up,
        const UCol& ucResCol,
        const uint8 uBlendType,
        const Vec3& vAmbientColor,
        Vec3 vPos,
        const int nAfterWater,
        const SRenderingPassInfo& passInfo,
        const SRendItemSorter& rendItemSorter);

    //////////////////////////////////////////////////////////////////////////

    void RegisterForStreaming(IStreamable* pObj);
    void UnregisterForStreaming(IStreamable* pObj);
    void UpdateRenderNodeStreamingPriority(IRenderNode* pObj, float fEntDistance, float fImportanceFactor, bool bFullUpdate, const SRenderingPassInfo& passInfo, bool bHighPriority = false);

    void GetMemoryUsage(class ICrySizer* pSizer) const;
    void GetBandwidthStats(float* fBandwidthRequested);

    void ReregisterEntitiesInArea(Vec3 vBoxMin, Vec3 vBoxMax);
    void UpdateObjectsStreamingPriority(bool bSyncLoad, const SRenderingPassInfo& passInfo);
    void ProcessObjectsStreaming(const SRenderingPassInfo& passInfo);

    // implementation parts of ProcessObjectsStreaming
    void ProcessObjectsStreaming_Impl(bool bSyncLoad, const SRenderingPassInfo& passInfo);
    void ProcessObjectsStreaming_Sort(bool bSyncLoad, const SRenderingPassInfo& passInfo);
    void ProcessObjectsStreaming_Release();
    void ProcessObjectsStreaming_InitLoad(bool bSyncLoad);
    void ProcessObjectsStreaming_Finish();

#ifdef OBJMAN_STREAM_STATS
    void ProcessObjectsStreaming_Stats(const SRenderingPassInfo& passInfo);
#endif

    // time counters

    virtual bool IsAfterWater(const Vec3& vPos, const SRenderingPassInfo& passInfo) override;

    void GetObjectsStreamingStatus(I3DEngine::SObjectsStreamingStatus& outStatus);

    void FreeNotUsedCGFs();

    void MakeUnitCube();

    //////////////////////////////////////////////////////////////////////////
    // CheckOcclusion functionality
    bool CheckOcclusion_TestAABB(const AABB& rAABB, float fEntDistance) override;
    bool CheckOcclusion_TestQuad(const Vec3& vCenter, const Vec3& vAxisX, const Vec3& vAxisY) override;

    void PushIntoCullQueue(const SCheckOcclusionJobData& rCheckOcclusionData) override;
    void PopFromCullQueue(SCheckOcclusionJobData* pCheckOcclusionData);

    void PushIntoCullOutputQueue(const SCheckOcclusionOutput& rCheckOcclusionOutput) override;
    bool PopFromCullOutputQueue(SCheckOcclusionOutput* pCheckOcclusionOutput) override;

    void BeginCulling() override;
    void RemoveCullJobProducer() override;
    void AddCullJobProducer() override;
    virtual NAsyncCull::CCullThread& GetCullThread() override { return m_CullThread;  };

#ifndef _RELEASE
    void CoverageBufferDebugDraw() override;
#endif

    bool LoadOcclusionMesh(const char* pFileName) override;

    //////////////////////////////////////////////////////////////////////////
    // Garbage collection for parent stat objects.
    // Returns number of deleted objects
    void ClearStatObjGarbage();
    void CheckForGarbage(IStatObj* pObject);
    void UnregisterForGarbage(IStatObj* pObject);

    int GetObjectLOD(const IRenderNode* pObj, float fDistance);
    bool RayStatObjIntersection(IStatObj* pStatObj, const Matrix34& objMat, _smart_ptr<IMaterial> pMat,
        Vec3 vStart, Vec3 vEnd, Vec3& vClosestHitPoint, float& fClosestHitDistance, bool bFastTest);
    bool RayRenderMeshIntersection(IRenderMesh* pRenderMesh, const Vec3& vInPos, const Vec3& vInDir, Vec3& vOutPos, Vec3& vOutNormal, bool bFastTest, _smart_ptr<IMaterial> pMat);
    bool SphereRenderMeshIntersection(IRenderMesh* pRenderMesh, const Vec3& vInPos, const float fRadius, _smart_ptr<IMaterial> pMat);
    PodArray<CVisArea*> m_tmpAreas0, m_tmpAreas1;

    uint8 GetDissolveRef(float fDist, float fMaxViewDist);
    float GetLodDistDissolveRef(SLodDistDissolveTransitionState* pState, float curDist, int nNewLod, const SRenderingPassInfo& passInfo);

    virtual PodArray<COctreeNode*>& GetArrStreamingNodeStack() override { return m_arrStreamingNodeStack; }
    virtual PodArray<SObjManPrecachePoint>& GetStreamPreCachePointDefs() override { return m_vStreamPreCachePointDefs; }

    void CleanStreamingData();
    IRenderMesh* GetRenderMeshBox();

    void PrepareCullbufferAsync(const CCamera& rCamera);
    void BeginOcclusionCulling(const SRenderingPassInfo& passInfo);
    void EndOcclusionCulling(bool waitForOcclusionJobCompletion = false);
    void RenderBufferedRenderMeshes(const SRenderingPassInfo& passInfo);

    virtual float GetGSMMaxDistance() const override { return m_fGSMMaxDistance; }
    virtual void SetGSMMaxDistance(float value) override { m_fGSMMaxDistance = value; }

    virtual int IncrementNextPrecachePointId() override { return m_nNextPrecachePointId++; }
private:
    std::vector<std::pair<_smart_ptr<IMaterial>, float> > m_collectedMaterials;

public:
    //////////////////////////////////////////////////////////////////////////
    // Public Member variables (need to be cleaned).
    //////////////////////////////////////////////////////////////////////////

    static int m_nUpdateStreamingPrioriryRoundId;
    static int m_nUpdateStreamingPrioriryRoundIdFast;
    static int s_nLastStreamingMemoryUsage;                 //For streaming tools in editor

    Vec3    m_vSunColor;    //Similar to CDLight's m_BaseColor
    Vec3    m_sunAnimColor; //Similar to CDLight's m_Color
    float   m_sunAnimSpeed;
    AZ::u8  m_sunAnimPhase;
    AZ::u8  m_sunAnimIndex;

    float                   m_fILMul;
    float                   m_fSSAOAmount;
    float                   m_fSSAOContrast;
    SRainParams     m_rainParams;
    SSnowParams     m_snowParams;

    int           m_bLockCGFResources;

    float m_fGSMMaxDistance;

public:
    //////////////////////////////////////////////////////////////////////////
    // Private Member variables.
    //////////////////////////////////////////////////////////////////////////
    PodArray<IStreamable*>  m_arrStreamableToRelease;
    PodArray<IStreamable*>  m_arrStreamableToLoad;
    PodArray<IStreamable*>  m_arrStreamableToDelete;
    bool m_bNeedProcessObjectsStreaming_Finish;

#ifdef SUPP_HWOBJ_OCCL
    IShader* m_pShaderOcclusionQuery;
#endif

    //  bool LoadStaticObjectsFromXML(XmlNodeRef xmlVegetation);
    _smart_ptr<IStatObj>        m_pDefaultCGF;
    _smart_ptr<IRenderMesh> m_pRMBox;

    //////////////////////////////////////////////////////////////////////////
    std::vector<_smart_ptr<IStatObj> > m_lockedObjects;

    //////////////////////////////////////////////////////////////////////////

    bool m_bGarbageCollectionEnabled;

    PodArray<SStreamAbleObject> m_arrStreamableObjects;
    PodArray<COctreeNode*> m_arrStreamingNodeStack;
    PodArray<SObjManPrecachePoint> m_vStreamPreCachePointDefs;
    PodArray<SObjManPrecacheCamera> m_vStreamPreCacheCameras;
    int m_nNextPrecachePointId;
    bool m_bCameraPrecacheOverridden;

#ifdef POOL_STATOBJ_ALLOCS
    stl::PoolAllocator<sizeof(CStatObj), stl::PSyncMultiThread, alignof(CStatObj)>* m_statObjPool;
#endif

    CThreadSafeRendererContainer<SObjManRenderDebugInfo> m_arrRenderDebugInfo;

    NAsyncCull::CCullThread m_CullThread;
    CryMT::SingleProducerSingleConsumerQueue<SCheckOcclusionJobData> m_CheckOcclusionQueue;
    CryMT::N_ProducerSingleConsumerQueue<SCheckOcclusionOutput> m_CheckOcclusionOutputQueue;

private:

    // Always take this lock after m_loadLock if taking both
    AZStd::recursive_mutex m_garbageMutex;
    AZStd::vector<IStatObj*> m_checkForGarbage;

private:
    // StatInstGroupEventBus
    AZStd::unordered_set<StatInstGroupId> m_usedIds;
    StatInstGroupId GenerateStatInstGroupId() override;
    void ReleaseStatInstGroupId(StatInstGroupId statInstGroupId) override;
    void ReleaseStatInstGroupIdSet(const AZStd::unordered_set<StatInstGroupId>& statInstGroupIdSet) override;
    void ReserveStatInstGroupIdRange(StatInstGroupId from, StatInstGroupId to) override;
};


#endif // CRYINCLUDE_CRY3DENGINE_OBJMAN_H
