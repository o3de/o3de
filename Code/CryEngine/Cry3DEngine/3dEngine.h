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

#ifndef CRYINCLUDE_CRY3DENGINE_3DENGINE_H
#define CRYINCLUDE_CRY3DENGINE_3DENGINE_H
#pragma once

#include <CryThreadSafeRendererContainer.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Math/Aabb.h>
#include <StatObjBus.h>

#ifdef DrawText
#undef DrawText
#endif //DrawText

struct IOctreeNode;
struct IDeferredPhysicsEventManager;
class COctreeNode;
class CCullBuffer;

struct StatInstGroup;


namespace LegacyProceduralVegetation
{
    class VegetationPoolManager;
}

class CMemoryBlock
    : public IMemoryBlock
{
public:
    virtual void* GetData() { return m_pData; }
    virtual int GetSize() { return m_nSize; }
    virtual ~CMemoryBlock() { delete[] m_pData; }

    CMemoryBlock() { m_pData = 0; m_nSize = 0; }
    CMemoryBlock(const void* pData, int nSize)
    {
        m_pData = 0;
        m_nSize = 0;
        SetData(pData, nSize);
    }
    void SetData(const void* pData, int nSize)
    {
        delete[] m_pData;
        m_pData = new uint8[nSize];
        memcpy(m_pData, pData, nSize);
        m_nSize = nSize;
    }
    void Free()
    {
        delete[] m_pData;
        m_pData = NULL;
        m_nSize = 0;
    }
    void Allocate(int nSize)
    {
        delete[] m_pData;
        m_pData = new uint8[nSize];
        memset(m_pData, 0, nSize);
        m_nSize = nSize;
    }

    static CMemoryBlock* CompressToMemBlock(void* pData, int nSize, ISystem* pSystem)
    {
        CMemoryBlock* pMemBlock = NULL;
        uint8* pTmp = new uint8[nSize + 4];
        size_t nComprSize = nSize;
        *(uint32*)pTmp = nSize;
        if (pSystem->CompressDataBlock(pData, nSize, pTmp + 4, nComprSize))
        {
            pMemBlock = new CMemoryBlock(pTmp, nComprSize + 4);
        }

        delete[] pTmp;
        return pMemBlock;
    }

    static CMemoryBlock* DecompressFromMemBlock(CMemoryBlock* pMemBlock, ISystem* pSystem)
    {
        size_t nUncompSize = *(uint32*)pMemBlock->GetData();
        SwapEndian(nUncompSize);
        CMemoryBlock* pResult = new CMemoryBlock;
        pResult->Allocate(nUncompSize);
        if (!pSystem->DecompressDataBlock((byte*)pMemBlock->GetData() + 4, pMemBlock->GetSize() - 4, pResult->GetData(), nUncompSize))
        {
            assert(!"CMemoryBlock::DecompressFromMemBlock failed");
            delete pResult;
            pResult = NULL;
        }

        return pResult;
    }

    uint8* m_pData;
    int m_nSize;
};

// Values to combine for phys area type selection
enum EAreaPhysics
{
    Area_Water = BIT(0),
    Area_Air = BIT(1),
    // Other physics media can be masked in as well

    Area_Gravity = BIT(14),
    Area_Other = BIT(15),
};

struct SAreaChangeRecord
{
    AABB boxAffected;                       // Area of change
    uint16 uPhysicsMask;                    // Types of mediums for this area
};

struct SPhysAreaNodeProxy
{
    void Reset()
    {
        pRenderNode = (IRenderNode*)(intptr_t)-1;
        bIsValid = false;
        bbox.Reset();
    }

    IRenderNode* pRenderNode;       // RenderNode
    uint16 uPhysicsMask;                // Bit mask of physics interested in
    bool bIsValid;                              // Does the proxy carry valid data
    AABB bbox;                                  // Bounding box of render node
};

struct SFrameInfo
{
    void Reset()
    {
        ppRNTmpData = (CRNTmpData**)(intptr_t)-1;
        bIsValid = false;
        nCreatedFrameId = nLastUsedFrameId = ~0;
    }

    uint32 nLastUsedFrameId;
    uint32 nCreatedFrameId;
    bool bIsValid;
    CRNTmpData** ppRNTmpData;
};

struct SNodeInfo;
class CStitchedImage;
struct DLightAmount
{
    CDLight* pLight;
    float fAmount;
};

template <class T, int nMaxElemsInChunk>
struct CPoolAllocator
{
    CPoolAllocator() { m_nCounter = 0; }

    ~CPoolAllocator()
    {
        Reset();
    }

    void Reset()
    {
        for (int i = 0; i < m_Pools.Count(); i++)
        {
            delete[](byte*)m_Pools[i];
            m_Pools[i] = NULL;
        }
        m_nCounter = 0;
    }

    void ReleaseElement(T* pElem)
    {
        if (pElem)
        {
            m_FreeElements.Add(pElem);
        }
    }

    T* GetNewElement()
    {
        if (m_FreeElements.Count())
        {
            T* pPtr = m_FreeElements.Last();
            m_FreeElements.DeleteLast();
            return pPtr;
        }

        int nPoolId = m_nCounter / nMaxElemsInChunk;
        int nElemId = m_nCounter - nPoolId * nMaxElemsInChunk;
        m_Pools.PreAllocate(nPoolId + 1, nPoolId + 1);
        if (!m_Pools[nPoolId])
        {
            m_Pools[nPoolId] = (T*)new byte[nMaxElemsInChunk * sizeof(T)];
        }
        m_nCounter++;
        return &m_Pools[nPoolId][nElemId];
    }

    int GetCount() { return m_nCounter - m_FreeElements.Count(); }
    int GetCapacity() { return m_Pools.Count() * nMaxElemsInChunk; }
    int GetCapacityBytes() { return GetCapacity() * sizeof(T); }

private:

    int m_nCounter;
    PodArray<T*> m_Pools;
    PodArray<T*> m_FreeElements;
};

struct SImageSubInfo
{
    SImageSubInfo() { memset(this, 0, sizeof(*this)); fTiling = fTilingIn = 1.f; }

    static const int nMipsNum = 4;

    union
    {
        byte* pImgMips[nMipsNum];
        int pImgMipsSizeKeeper[8];
    };

    float fAmount;
    int  nReady;
    int nDummy[4];

    _smart_ptr<IMaterial> pMat;
    int nDim;
    float fTilingIn;
    float fTiling;
    float fSpecularAmount;
    int nSortOrder;
    int nAlignFix;

    AUTO_STRUCT_INFO
};

struct SImageInfo
    : public Cry3DEngineBase
{
    SImageInfo()
    {
        szDetMatName[0] = szBaseTexName[0] = '\0';
        nPhysSurfaceType = 0;
        nLayerId = 0;
        fUseRemeshing = 0;
        fBr = 1.f;
        layerFilterColor = Col_White;
        nDetailSurfTypeId = 0;
        ZeroStruct(arrTextureId);
    }

    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const { /*nothing*/ }

    SImageSubInfo baseInfo;
    SImageSubInfo detailInfo;

    char szDetMatName[128 - 20];

    int arrTextureId[4];
    int nPhysSurfaceType;

    char szBaseTexName[128];

    float fUseRemeshing;
    ColorF layerFilterColor;
    int nLayerId;
    float fBr;
    int nDetailSurfTypeId;

    int GetMemoryUsage();

    AUTO_STRUCT_INFO
};

struct SSceneFrustum
{
    uint32* pRgbImage;
    uint32  nRgbWidth, nRgbHeight;

    float* pDepthImage;
    uint32  nDepthWidth, nDepthHeight;

    CCamera camera;

    IRenderMesh* pRM;
    _smart_ptr<IMaterial> pMaterial;

    float fDistance;
    int nId;

    static int Compare(const void* v1, const void* v2)
    {
        SSceneFrustum* p[2] = { (SSceneFrustum*)v1, (SSceneFrustum*)v2 };

        if (p[0]->fDistance > p[1]->fDistance)
        {
            return 1;
        }
        if (p[0]->fDistance < p[1]->fDistance)
        {
            return -1;
        }

        if (p[0]->nId > p[1]->nId)
        {
            return 1;
        }
        if (p[0]->nId < p[1]->nId)
        {
            return -1;
        }

        return 0;
    }
};

struct SPerObjectShadow
{
    IShadowCaster* pCaster;
    float fConstBias;
    float fSlopeBias;
    float fJitter;
    Vec3  vBBoxScale;
    uint nTexSize;
};
//////////////////////////////////////////////////////////////////////
#define LV_MAX_COUNT 256
#define LV_LIGHTS_MAX_COUNT 64

#define LV_WORLD_BUCKET_SIZE 512
#define LV_LIGHTS_WORLD_BUCKET_SIZE 512

#define LV_WORLD_SIZEX 128
#define LV_WORLD_SIZEY 128
#define LV_WORLD_SIZEZ 64

#define LV_CELL_SIZEX 4
#define LV_CELL_SIZEY 4
#define LV_CELL_SIZEZ 8

#define LV_CELL_RSIZEX (1.0f / (float)LV_CELL_SIZEX)
#define LV_CELL_RSIZEY (1.0f / (float)LV_CELL_SIZEY)
#define LV_CELL_RSIZEZ (1.0f / (float)LV_CELL_SIZEZ)

#define LV_LIGHT_CELL_SIZE 32
#define LV_LIGHT_CELL_R_SIZE (1.0f / (float)LV_LIGHT_CELL_SIZE)

#define LV_DLF_LIGHTVOLUMES_MASK (DLF_DISABLED | DLF_FAKE | DLF_AMBIENT | DLF_DEFERRED_CUBEMAPS)

class CLightVolumesMgr
    : public Cry3DEngineBase
{
public:
    CLightVolumesMgr()
    {
        Init();
    }

    void Init();
    void Reset();
    uint16 RegisterVolume(const Vec3& vPos, f32 fRadius, uint8 nClipVolumeRef, const SRenderingPassInfo& passInfo);
    void RegisterLight(const CDLight& pDL, uint32 nLightID, const SRenderingPassInfo& passInfo);
    void Update(const SRenderingPassInfo& passInfo);
    void Clear(const SRenderingPassInfo& passInfo);
    void GetLightVolumes(threadID nThreadID, SLightVolume*& pLightVols, uint32& nNumVols);
#ifndef _RELEASE
    void DrawDebug(const SRenderingPassInfo& passInfo);
#endif

private:
    struct SLightVolInfo
    {
        SLightVolInfo()
            : vVolume(ZERO, 0.0f)
            , nNextVolume(0)
            , nClipVolumeID(0)
        {
        };
        SLightVolInfo(const Vec3& pPos, float fRad, uint8 clipVolumeID)
            : vVolume(pPos, fRad)
            , nNextVolume(0)
            , nClipVolumeID(clipVolumeID)
        {
        };

        Vec4 vVolume; // xyz: position, w: radius
        uint16 nNextVolume; // index of next light volume for this hash bucket (0 if none)
        uint8 nClipVolumeID; // clip volume stencil ref
    };

    struct SLightCell
    {
        SLightCell()
            : nLightCount(0)
        {
        };

        uint16 nLightID[LV_LIGHTS_MAX_COUNT];
        uint8 nLightCount;
    };

    inline int32 GetIntHash(const int32 k, [[maybe_unused]] const int32 nBucketSize = LV_WORLD_BUCKET_SIZE) const
    {
        static const uint32 nHashBits = 9;
        static const uint32 nGoldenRatio32bits = 2654435761u; // (2^32) / (golden ratio)
        return (k * nGoldenRatio32bits) >> (32 - nHashBits);  // ref: knuths integer multiplicative hash function
    }

    inline uint16 GetWorldHashBucketKey(const int32 x, const int32 y, const int32 z, const int32 nBucketSize = LV_WORLD_BUCKET_SIZE) const
    {
        const uint32 nPrimeNum = 101;//0xd8163841;
        return aznumeric_caster((((GetIntHash(x) + nPrimeNum) * nPrimeNum + GetIntHash(y)) * nPrimeNum + GetIntHash(z)) & (nBucketSize - 1));
    }

    void AddLight(const SRenderLight& pLight, const SLightVolInfo* __restrict pVolInfo, SLightVolume& pVolume);

    typedef DynArray<SLightVolume> LightVolumeVector;

private:
    LightVolumeVector m_pLightVolumes[RT_COMMAND_BUF_COUNT];        // Final light volume list. <todo> move light list creation to renderer to avoid double-buffering this
    DynArray<SLightVolInfo*> m_pLightVolsInfo[RT_COMMAND_BUF_COUNT]; // World cells data
    SLightCell m_pWorldLightCells[LV_LIGHTS_WORLD_BUCKET_SIZE];     // 2D World cell buckets for light sources ids
    uint16 m_nWorldCells[LV_WORLD_BUCKET_SIZE];                     // World cell buckets for light volumes
    bool m_bUpdateLightVolumes : 1;
};

// onscreen infodebug for e_debugDraw >= 100
#ifndef _RELEASE
class CDebugDrawListMgr
{
    typedef CryFixedStringT<64> TMyStandardString;
    typedef CryFixedStringT<128> TFilenameString;

public:

    CDebugDrawListMgr();
    void Update();
    void AddObject(I3DEngine::SObjectInfoToAddToDebugDrawList& objInfo);
    void DumpLog();
    bool IsEnabled() const { return Cry3DEngineBase::GetCVars()->e_DebugDraw >= LM_BASENUMBER; }
    static void ConsoleCommand(IConsoleCmdArgs* args);

private:

    enum
    {
        UNDEFINED_ASSET_ID = 0xffffffff
    };

    struct TAssetInfo
    {
        TMyStandardString   name;
        TFilenameString fileName;
        uint32      numTris;
        uint32      numVerts;
        uint32      texMemory;
        uint32      meshMemory;
        uint32      drawCalls;
        uint32      numInstances;
        I3DEngine::EDebugDrawListAssetTypes type;
        uint32      ID; // to identify which drawBoxes belong to this asset

        TAssetInfo(const I3DEngine::SObjectInfoToAddToDebugDrawList& objInfo);
        bool operator<(const TAssetInfo& other) const;
    };

    struct TObjectDrawBoxInfo
    {
        Matrix34    mat;
        AABB            bbox;
        uint32      assetID;

        TObjectDrawBoxInfo(const I3DEngine::SObjectInfoToAddToDebugDrawList& objInfo);
    };

    void FindNewLeastValueAsset();
    void ClearFrameData();
    void ClearConsoleCommandRequestVars();
    static bool SortComparison(const TAssetInfo& A, const TAssetInfo& B) { return B < A; }
    const char* GetStrCurrMode();
    void GetStrCurrFilter(TMyStandardString& strOut);
    bool ShouldFilterOutObject(const I3DEngine::SObjectInfoToAddToDebugDrawList& newObject);
    void MemToString(uint32 memVal, TMyStandardString& outStr);
    static void PrintText(float x, float y, const ColorF& fColor, const char* label_text, ...);
    const char* GetAssetTypeName(I3DEngine::EDebugDrawListAssetTypes type);
    TAssetInfo* FindDuplicate(const TAssetInfo& object);
    void CheckFilterCVar();

    // to avoid any heap allocation
    static void MyStandardString_Concatenate(TMyStandardString& outStr, const char* str);
    static void MyFileNameString_Assign(TFilenameString& outStr, const char* pStr);

    template<class T>
    static void MyString_Assign(T& outStr, const char* pStr)
    {
        if (pStr)
        {
            outStr._Assign(pStr, min(strlen(pStr), outStr.capacity()));
        }
        else
        {
            outStr = "";
        }
    }



    enum EListModes
    {
        LM_BASENUMBER = 100,
        LM_TRI_COUNT = LM_BASENUMBER,
        LM_VERT_COUNT,
        LM_DRAWCALLS,
        LM_TEXTMEM,
        LM_MESHMEM
    };


    bool                                            m_isFrozen;
    uint32                                      m_counter;
    uint32                                      m_assetCounter;
    uint32                                      m_indexLeastValueAsset;
    std::vector<TAssetInfo>     m_assets;
    std::vector<TObjectDrawBoxInfo> m_drawBoxes;
    CryCriticalSection              m_lock;

    static bool                             m_dumpLogRequested;
    static bool                             m_freezeRequested;
    static bool                             m_unfreezeRequested;
    static uint32                           m_filter;
};
#endif //_RELEASE

//////////////////////////////////////////////////////////////////////
class C3DEngine
    : public I3DEngine
    , public Cry3DEngineBase
{
public:

    // I3DEngine interface implementation
    virtual bool Init();
    virtual void OnFrameStart();
    virtual void Update();
    virtual void RenderWorld(int nRenderFlags, const SRenderingPassInfo& passInfo, const char* szDebugName);
    virtual void PreWorldStreamUpdate(const CCamera& cam);
    virtual void WorldStreamUpdate();
    virtual void ShutDown();
    virtual void Release() { CryAlignedDelete(this); };
    virtual void SetLevelPath(const char* szFolderName);
    virtual bool LoadLevel(const char* szFolderName, const char* szMissionName);
    virtual void UnloadLevel();
    virtual void PostLoadLevel();
    virtual bool InitLevelForEditor(const char* szFolderName, const char* szMissionName);
    virtual bool LevelLoadingInProgress();
    virtual void DisplayInfo(float& fTextPosX, float& fTextPosY, float& fTextStepY, bool bEnhanced);
    virtual void DisplayMemoryStatistics();
    virtual void SetupDistanceFog();
    virtual IStatObj* LoadStatObjUnsafeManualRef(const char* fileName, const char* geomName = nullptr, /*[Out]*/ IStatObj::SSubObject** subObject = nullptr, 
        bool useStreaming = true, unsigned long loadingFlags = 0, const void* data = nullptr, int dataSize = 0) override;
    virtual _smart_ptr<IStatObj> LoadStatObjAutoRef(const char* fileName, const char* geomName = nullptr, /*[Out]*/ IStatObj::SSubObject** subObject = nullptr, 
        bool useStreaming = true, unsigned long loadingFlags = 0, const void* data = nullptr, int dataSize = 0) override;
    virtual const IObjManager* GetObjectManager() const;
    virtual IObjManager* GetObjectManager();

    virtual IStatObj* FindStatObjectByFilename(const char* filename);
    virtual void RegisterEntity(IRenderNode* pEnt, int nSID = -1, int nSIDConsideredSafe = -1);
    virtual bool IsSunShadows(){ return m_bSunShadows; };
    virtual void SelectEntity(IRenderNode* pEnt);
    virtual void LoadEmptyLevel() override;

    virtual void LoadStatObjAsync(LoadStaticObjectAsyncResult resultCallback, const char* szFileName, const char* szGeomName = nullptr, bool bUseStreaming = true, unsigned long nLoadingFlags = 0);
    virtual void ProcessAsyncStaticObjectLoadRequests() override;

#ifndef _RELEASE
    virtual void AddObjToDebugDrawList(SObjectInfoToAddToDebugDrawList& objInfo);
    virtual bool IsDebugDrawListEnabled() const { return m_DebugDrawListMgr.IsEnabled(); }
#endif

    virtual void UnRegisterEntityDirect(IRenderNode* pEnt);
    virtual void UnRegisterEntityAsJob(IRenderNode* pEnt);

    virtual bool IsUnderWater(const Vec3& vPos) const;
    virtual void SetOceanRenderFlags(uint8 nFlags);
    virtual uint8 GetOceanRenderFlags() const { return m_nOceanRenderFlags; }
    virtual uint32 GetOceanVisiblePixelsCount() const;
    virtual float GetBottomLevel(const Vec3& referencePos, float maxRelevantDepth, int objtypes);
    virtual float GetBottomLevel(const Vec3& referencePos, float maxRelevantDepth /* = 10.0f*/);
    virtual float GetBottomLevel(const Vec3& referencePos, int objflags);

#if defined(USE_GEOM_CACHES)
    virtual IGeomCache* LoadGeomCache(const char* szFileName);
    virtual IGeomCache* FindGeomCacheByFilename(const char* szFileName);
#endif

    virtual IStatObj* LoadDesignerObject(int nVersion, const char* szBinaryStream, int size);

    void AsyncOctreeUpdate(IRenderNode* pEnt, int nSID, int nSIDConsideredSafe, uint32 nFrameID, bool bUnRegisterOnly);
    bool UnRegisterEntityImpl(IRenderNode* pEnt);

    // Fast option - use if just ocean height required
    virtual float GetWaterLevel();
    // This will return ocean height or water volume height, optional for accurate water height query
    virtual float GetWaterLevel(const Vec3* pvPos, IPhysicalEntity* pent = NULL, bool bAccurate = false);
    // Only use for Accurate query - this will return exact ocean height
    virtual float GetAccurateOceanHeight(const Vec3& pCurrPos) const;

    virtual CausticsParams GetCausticsParams() const;
    virtual OceanAnimationData GetOceanAnimationParams() const override;
    virtual void GetHDRSetupParams(Vec4 pParams[5]) const;
    virtual void CreateDecal(const CryEngineDecalInfo& Decal);

    virtual void SetSunDir(const Vec3& newSunOffset);
    virtual Vec3 GetSunDir() const;
    virtual Vec3 GetSunDirNormalized() const;
    virtual Vec3 GetRealtimeSunDirNormalized() const;
    virtual void SetSunColor(Vec3 vColor);
    Vec3 GetSunAnimColor() override;
    void SetSunAnimColor(const Vec3& sunAnimColor) override;
    float GetSunAnimSpeed() override;
    void SetSunAnimSpeed(float sunAnimSpeed) override;
    AZ::u8 GetSunAnimPhase() override;
    void SetSunAnimPhase(AZ::u8 sunAnimPhase) override;
    AZ::u8 GetSunAnimIndex() override;
    void SetSunAnimIndex(AZ::u8 sunAnimIndex) override;
    virtual void SetSSAOAmount(float fMul);
    virtual void SetSSAOContrast(float fMul);
    virtual void SetRainParams(const SRainParams& rainParams);
    virtual bool GetRainParams(SRainParams& rainParams);
    virtual void SetSnowSurfaceParams(const Vec3& vCenter, float fRadius, float fSnowAmount, float fFrostAmount, float fSurfaceFreezing);
    virtual bool GetSnowSurfaceParams(Vec3& vCenter, float& fRadius, float& fSnowAmount, float& fFrostAmount, float& fSurfaceFreezing);
    virtual void SetSnowFallParams(int nSnowFlakeCount, float fSnowFlakeSize, float fSnowFallBrightness, float fSnowFallGravityScale, float fSnowFallWindScale, float fSnowFallTurbulence, float fSnowFallTurbulenceFreq);
    virtual bool GetSnowFallParams(int& nSnowFlakeCount, float& fSnowFlakeSize, float& fSnowFallBrightness, float& fSnowFallGravityScale, float& fSnowFallWindScale, float& fSnowFallTurbulence, float& fSnowFallTurbulenceFreq);
    virtual void OnExplosion(Vec3 vPos, float fRadius, bool bDeformTerrain = true);
    //! For editor
    virtual void RemoveAllStaticObjects(int nSID);

    virtual void SetPhysMaterialEnumerator(IPhysMaterialEnumerator* pPhysMaterialEnumerator);
    virtual IPhysMaterialEnumerator* GetPhysMaterialEnumerator();
    virtual void LoadMissionDataFromXMLNode(const char* szMissionName);

    void AddDynamicLightSource(const class CDLight& LSource, ILightSource* pEnt, int nEntityLightId, float fFadeout, const SRenderingPassInfo& passInfo);

    inline void AddLightToRenderer(const CDLight& pLight, float fMult, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter)
    {
        const uint32 nLightID = GetRenderer()->EF_GetDeferredLightsNum();
        GetRenderer()->EF_AddDeferredLight(pLight, fMult, passInfo, rendItemSorter);
        Get3DEngine()->m_LightVolumesMgr.RegisterLight(pLight, nLightID, passInfo);
        m_nDeferredLightsNum++;
    }

    virtual void SetMaxViewDistanceScale(float fScale) { m_fMaxViewDistScale = fScale; }
    virtual float GetMaxViewDistance(bool bScaled = true);
    virtual const SFrameLodInfo& GetFrameLodInfo() const { return m_frameLodInfo; }
    virtual void SetFrameLodInfo(const SFrameLodInfo& frameLodInfo);
    virtual void SetFogColor(const Vec3& vFogColor);
    virtual Vec3 GetFogColor();
    virtual float GetDistanceToSectorWithWater();

    virtual void GetSkyLightParameters(Vec3& sunDir, Vec3& sunIntensity, float& Km, float& Kr, float& g, Vec3& rgbWaveLengths);
    virtual void SetSkyLightParameters(const Vec3& sunDir, const Vec3& sunIntensity, float Km, float Kr, float g, const Vec3& rgbWaveLengths, bool forceImmediateUpdate = false);

    void SetLightsHDRDynamicPowerFactor(const float value);
    virtual float GetLightsHDRDynamicPowerFactor() const;

    // Return true if tessellation is allowed (by cvars) into currently set shadow map LOD
    bool IsTessellationAllowedForShadowMap(const SRenderingPassInfo& passInfo) const;
    // Return true if tessellation is allowed for given render object
    virtual bool IsTessellationAllowed(const CRenderObject* pObj, const SRenderingPassInfo& passInfo, bool bIgnoreShadowPass = false) const;

    virtual void SetRenderNodeMaterialAtPosition(EERType eNodeType, const Vec3& vPos, _smart_ptr<IMaterial> pMat);
    virtual void OverrideCameraPrecachePoint(const Vec3& vPos);
    virtual int AddPrecachePoint(const Vec3& vPos, const Vec3& vDir, float fTimeOut = 3.f, float fImportanceFactor = 1.0f);
    virtual void ClearPrecachePoint(int id);
    virtual void ClearAllPrecachePoints();
    virtual void GetPrecacheRoundIds(int pRoundIds[MAX_STREAM_PREDICTION_ZONES]);

    virtual void TraceFogVolumes(const Vec3& vPos, const AABB& objBBox, SFogVolumeData& fogVolData, const SRenderingPassInfo& passInfo, bool fogVolumeShadingQuality);

    virtual Vec3 GetSunColor() const;
    virtual float GetSSAOAmount() const;
    virtual float GetSSAOContrast() const;

    virtual void FreeRenderNodeState(IRenderNode* pEnt);
    virtual const char* GetLevelFilePath(const char* szFileName);

    bool LoadCompiledOctreeForEditor() override;
    virtual bool SetStatInstGroup(int nGroupId, const IStatInstGroup& siGroup, int nSID);
    virtual bool GetStatInstGroup(int nGroupId, IStatInstGroup& siGroup, int nSID);
    virtual void ActivatePortal(const Vec3& vPos, bool bActivate, const char* szEntityName);
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual void GetResourceMemoryUsage(ICrySizer* pSizer, const AABB& cstAABB);
    virtual IVisArea* CreateVisArea(uint64 visGUID);
    virtual void DeleteVisArea(IVisArea* pVisArea);
    virtual void UpdateVisArea(IVisArea* pArea, const Vec3* pPoints, int nCount, const char* szName,
        const SVisAreaInfo& info, bool bReregisterObjects);
    virtual IClipVolume* CreateClipVolume();
    virtual void DeleteClipVolume(IClipVolume* pClipVolume);
    virtual void UpdateClipVolume(IClipVolume* pClipVolume, _smart_ptr<IRenderMesh> pRenderMesh, IBSPTree3D* pBspTree, const Matrix34& worldTM, bool bActive, uint32 flags, const char* szName);
    virtual void ResetParticlesAndDecals();
    virtual IRenderNode* CreateRenderNode(EERType type);
    virtual void DeleteRenderNode(IRenderNode* pRenderNode);
    virtual Vec3 GetWind(const AABB& box, bool bIndoors) const;
    virtual Vec3 GetGlobalWind(bool bIndoors) const;
    virtual bool SampleWind(Vec3* pSamples, int nSamples, const AABB& volume, bool bIndoors) const;
    void SetupBending(CRenderObject*& pObj, const IRenderNode* pNode, const float fRadiusVert, const SRenderingPassInfo& passInfo, bool alreadyDuplicated = false);
    virtual IVisArea* GetVisAreaFromPos(const Vec3& vPos);
    virtual bool IntersectsVisAreas(const AABB& box, void** pNodeCache = 0);
    virtual bool ClipToVisAreas(IVisArea* pInside, Sphere& sphere, Vec3 const& vNormal, void* pNodeCache = 0);
    virtual bool IsVisAreasConnected(IVisArea* pArea1, IVisArea* pArea2, int nMaxReqursion, bool bSkipDisabledPortals);
    void EnableOceanRendering(bool bOcean); // todo: remove

    virtual void AddTextureLoadHandler(ITextureLoadHandler* pHandler);
    virtual void RemoveTextureLoadHandler(ITextureLoadHandler* pHandler);
    virtual ITextureLoadHandler* GetTextureLoadHandlerForImage(const char* ext);
    virtual struct ILightSource* CreateLightSource();
    virtual void DeleteLightSource(ILightSource* pLightSource);
    virtual bool RestoreTerrainFromDisk(int nSID);
    virtual void CheckMemoryHeap();

    virtual int GetLoadedObjectCount();
    virtual void GetLoadedStatObjArray(IStatObj** pObjectsArray, int& nCount);
    virtual void GetObjectsStreamingStatus(SObjectsStreamingStatus& outStatus);
    virtual void GetStreamingSubsystemData(int subsystem, SStremaingBandwidthData& outData);
    virtual void DeleteEntityDecals(IRenderNode* pEntity);
    virtual void DeleteDecalsInRange(AABB* pAreaBox, IRenderNode* pEntity);
    virtual void LockCGFResources();
    virtual void UnlockCGFResources();
    virtual void FreeUnusedCGFResources();

    virtual void SerializeState(TSerialize ser);
    virtual void PostSerialize(bool bReading);

    virtual void SetStreamableListener(IStreamedObjectListener* pListener);

    //////////////////////////////////////////////////////////////////////////
    // Materials access.
    virtual IMaterialHelpers& GetMaterialHelpers();
    virtual IMaterialManager* GetMaterialManager();
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // ObjManager access.
    virtual IObjManager* GetObjManager() override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // CGF Loader.
    //////////////////////////////////////////////////////////////////////////
    virtual CContentCGF* CreateChunkfileContent(const char* filename);
    virtual void ReleaseChunkfileContent(CContentCGF*);
    virtual bool LoadChunkFileContent(CContentCGF* pCGF, const char* filename, bool bNoWarningMode = false, bool bCopyChunkFile = true);
    virtual bool LoadChunkFileContentFromMem(CContentCGF* pCGF, const void* pData, size_t nDataLen, uint32 nLoadingFlags, bool bNoWarningMode = false, bool bCopyChunkFile = true);
    //////////////////////////////////////////////////////////////////////////
    virtual IChunkFile* CreateChunkFile(bool bReadOnly = false);

    //////////////////////////////////////////////////////////////////////////
    // Chunk file writer.
    //////////////////////////////////////////////////////////////////////////
    virtual ChunkFile::IChunkFileWriter* CreateChunkFileWriter(EChunkFileFormat eFormat, AZ::IO::IArchive* pPak, const char* filename) const;
    virtual void ReleaseChunkFileWriter(ChunkFile::IChunkFileWriter* p) const;

    //////////////////////////////////////////////////////////////////////////
    // Post processing effects interfaces

    class IPostEffectGroupManager* GetPostEffectGroups() const override { return m_postEffectGroups.get(); }
    class IPostEffectGroup* GetPostEffectBaseGroup() const override { return m_postEffectBaseGroup; }

    // Most code should use either GetPostEffectGroups() or GetPostEffectBaseGroup() instead of these
    virtual void SetPostEffectParam(const char* pParam, float fValue, bool bForceValue = false) const;
    virtual void SetPostEffectParamVec4(const char* pParam, const Vec4& pValue, bool bForceValue = false) const;
    virtual void SetPostEffectParamString(const char* pParam, const char* pszArg) const;

    virtual void GetPostEffectParam(const char* pParam, float& fValue) const;
    virtual void GetPostEffectParamVec4(const char* pParam, Vec4& pValue) const;
    virtual void GetPostEffectParamString(const char* pParam, const char*& pszArg) const;

    virtual int32 GetPostEffectID(const char* pPostEffectName);

    virtual void ResetPostEffects(bool bOnSpecChange = false);
    virtual void DisablePostEffects();

    virtual void SetShadowsGSMCache(bool bCache);
    virtual void SetCachedShadowBounds(const AABB& shadowBounds, float fAdditionalCascadesScale);
    virtual void SetRecomputeCachedShadows(uint nUpdateStrategy = 0);
    void SetShadowsCascadesBias(const float* pCascadeConstBias, const float* pCascadeSlopeBias);
    const float* GetShadowsCascadesConstBias() const { return m_pShadowCascadeConstBias; }
    const float* GetShadowsCascadesSlopeBias() const { return m_pShadowCascadeSlopeBias; }
    int GetShadowsCascadeCount(const CDLight* pLight) const;

    virtual uint32 GetObjectsByType(EERType objType, IRenderNode** pObjects);
    uint32 GetObjectsByTypeInBox(EERType objType, const AABB& bbox, IRenderNode** pObjects, ObjectTreeQueryFilterCallback filterCallback = nullptr) override;
    virtual uint32 GetObjectsInBox(const AABB& bbox, IRenderNode** pObjects = 0);
    void GetObjectsByTypeInBox(EERType objType, const AABB& bbox, PodArray<IRenderNode*>* pLstObjects, ObjectTreeQueryFilterCallback filterCallback = nullptr) override;
    virtual uint32 GetObjectsByFlags(uint dwFlags, IRenderNode** pObjects = 0);
    virtual void OnObjectModified(IRenderNode* pRenderNode, uint dwFlags);

    virtual void ActivateObjectsLayer(uint16 nLayerId, bool bActivate, bool bPhys, bool bObjects, bool bStaticLights, const char* pLayerName, IGeneralMemoryHeap* pHeap = NULL, bool bCheckLayerActivation = true);
    virtual void GetLayerMemoryUsage(uint16 nLayerId, ICrySizer* pSizer, int* pNumBrushes, int* pNumDecals) const;
    virtual void SkipLayerLoading(uint16 nLayerId, bool bClearList);
    bool IsLayerSkipped(uint16 nLayerId);

    //////////////////////////////////////////////////////////////////////////

    const char* GetLevelFolder() override { return m_szLevelFolder; }

    bool SaveCGF(std::vector<IStatObj*>& pObjs);

    virtual bool IsAreaActivationInUse() { return m_bAreaActivationInUse && GetCVars()->e_ObjectLayersActivation; }

    int GetCurrentLightSpec()
    {
        return CONFIG_VERYHIGH_SPEC; // very high spec.
    }

    bool CheckMinSpec(uint32 nMinSpec) override;

    void UpdateRenderingCamera(const char* szCallerName, const SRenderingPassInfo& passInfo);
    virtual void PrepareOcclusion(const CCamera& rCamera);
    virtual void EndOcclusion();
#ifndef _RELEASE
    void ProcessStreamingLatencyTest(const CCamera& camIn, CCamera& camOut, const SRenderingPassInfo& passInfo);
#endif

    void ScreenShotHighRes(CStitchedImage* pStitchedImage, const int nRenderFlags, const SRenderingPassInfo& passInfo, uint32 SliceCount, f32 fTransitionSize);

    // cylindrical mapping made by multiple slices rendered and distorted
    // Returns:
    //   true=mode is active, stop normal rendering, false=mode is not active
    bool ScreenShotPanorama(CStitchedImage* pStitchedImage, const int nRenderFlags, const SRenderingPassInfo& passInfo, uint32 SliceCount, f32 fTransitionSize);
    // Render simple top-down screenshot for map overviews
    bool ScreenShotMap(CStitchedImage* pStitchedImage, const int nRenderFlags, const SRenderingPassInfo& passInfo, uint32 SliceCount, f32 fTransitionSize);

    void ScreenshotDispatcher(const int nRenderFlags, const SRenderingPassInfo& passInfo);

    virtual void FillDebugFPSInfo(SDebugFPSInfo& info);

    void ClearDebugFPSInfo(bool bUnload = false)
    {
        m_fAverageFPS = 0.0f;
        m_fMinFPS = m_fMinFPSDecay = 999.f;
        m_fMaxFPS = m_fMaxFPSDecay = 0.0f;
        ClearPrecacheInfo();
        if (bUnload)
        {
            stl::free_container(arrFPSforSaveLevelStats);
        }
        else
        {
            arrFPSforSaveLevelStats.clear();
        }
    }

    void ClearPrecacheInfo()
    {
        m_nFramesSinceLevelStart = 0;
        m_nStreamingFramesSinceLevelStart = 0;
        m_bPreCacheEndEventSent = false;
        m_fTimeStateStarted = 0.0f;
    }

    virtual const CCamera& GetRenderingCamera() const       { return m_RenderingCamera; }
    virtual float GetZoomFactor() const                     { return m_fZoomFactor; }
    virtual void Tick();

    virtual void UpdateShaderItems();
    void GetCollisionClass(SCollisionClass& collclass, int tableIndex);

    C3DEngine(ISystem* pSystem);
    ~C3DEngine();

    const float GetGSMRange() override { return m_fGsmRange; }
    const float GetGSMRangeStep() override { return m_fGsmRangeStep; }

    void UpdatePreRender(const SRenderingPassInfo& passInfo);
    void UpdatePostRender(const SRenderingPassInfo& passInfo);

    virtual void RenderScene(const int nRenderFlags, const SRenderingPassInfo& passInfo);
    virtual void RenderSceneReflection(int nRenderFlags, const SRenderingPassInfo& passInfo);
    virtual void DebugDraw_UpdateDebugNode();

    void DebugDraw_Draw();
    bool IsOutdoorVisible();
    void RenderSkyBox(_smart_ptr<IMaterial> pMat, const SRenderingPassInfo& passInfo);

    int GetStreamingFramesSinceLevelStart() { return m_nStreamingFramesSinceLevelStart; }
    int GetRenderFramesSinceLevelStart() { return m_nFramesSinceLevelStart; }

    bool CreateDecalInstance(const CryEngineDecalInfo&DecalInfo, class CDecal* pCallerManagedDecal);
    Vec3 GetTerrainSurfaceNormal(Vec3 vPos);
    void LoadEnvironmentSettingsFromXML(XmlNodeRef pInputNode, int nSID);
#if defined(FEATURE_SVO_GI)
    void LoadTISettings(XmlNodeRef pInputNode);
#endif
    void LoadDefaultAssets();

    // access to components
    static CVars* GetCVars() { return m_pCVars; }
    ILINE CVisAreaManager* GetVisAreaManager() { return m_pVisAreaManager; }
    ILINE CClipVolumeManager* GetClipVolumeManager() { return m_pClipVolumeManager; }
    ILINE PodArray<ILightSource*>* GetLightEntities() { return &m_lstStaticLights; }

    ILINE IGeneralMemoryHeap* GetBreakableBrushHeap() { return m_pBreakableBrushHeap; }

    virtual void OnCameraTeleport();

    bool m_bAreaActivationInUse;

    // Level info
    float m_fSkyBoxAngle, m_fSkyBoxStretching;

    float m_fMaxViewDistScale;
    float m_fMaxViewDistHighSpec;
    float m_fMaxViewDistLowSpec;

    float m_volFogGlobalDensity;
    float m_volFogGlobalDensityMultiplierLDR;
    float m_volFogFinalDensityClamp;

    float m_fCloudShadingSunLightMultiplier;
    float m_fCloudShadingSkyLightMultiplier;
    Vec3 m_vCloudShadingCustomSunColor;
    Vec3 m_vCloudShadingCustomSkyColor;

    Vec3 m_vFogColor;
    Vec3 m_vDefFogColor;
    Vec3 m_vSunDir;
    Vec3 m_vSunDirNormalized;
    float m_fSunDirUpdateTime;
    Vec3 m_vSunDirRealtime;

    Vec3 m_volFogRamp;
    Vec3 m_volFogShadowRange;
    Vec3 m_volFogShadowDarkening;
    Vec3 m_volFogShadowEnable;

    Vec3 m_volFog2CtrlParams;
    Vec3 m_volFog2ScatteringParams;
    Vec3 m_volFog2Ramp;
    Vec3 m_volFog2Color;
    Vec3 m_volFog2GlobalDensity;
    Vec3 m_volFog2HeightDensity;
    Vec3 m_volFog2HeightDensity2;
    Vec3 m_volFog2Color1;
    Vec3 m_volFog2Color2;

    Vec3 m_nightSkyHorizonCol;
    Vec3 m_nightSkyZenithCol;
    float m_nightSkyZenithColShift;
    float m_nightSkyStarIntensity;
    Vec3 m_nightMoonCol;
    float m_nightMoonSize;
    Vec3 m_nightMoonInnerCoronaCol;
    float m_nightMoonInnerCoronaScale;
    Vec3 m_nightMoonOuterCoronaCol;
    float m_nightMoonOuterCoronaScale;

    float m_moonRotationLatitude;
    float m_moonRotationLongitude;
    Vec3 m_moonDirection;
    int m_nWaterBottomTexId;
    int m_nNightMoonTexId;
    float m_fSunClipPlaneRange;
    float m_fSunClipPlaneRangeShift;
    bool m_bSunShadows;

    int m_nCloudShadowTexId;

    float m_fGsmRange;
    float m_fGsmRangeStep;
    float m_fShadowsConstBias;
    float m_fShadowsSlopeBias;

    int m_nSunAdditionalCascades;
    int m_nGsmCache;
    Vec3 m_oceanFogColor;
    Vec3 m_oceanFogColorShallow;
    float m_oceanFogDensity;
    float m_skyboxMultiplier;
    float m_dayNightIndicator;
    bool m_bHeightMapAoEnabled;

    Vec3 m_fogColor2;
    Vec3 m_fogColorRadial;
    Vec3 m_volFogHeightDensity;
    Vec3 m_volFogHeightDensity2;
    Vec3 m_volFogGradientCtrl;

private:
    float m_oceanWindDirection;
    float m_oceanWindSpeed;
    float m_oceanWavesSpeed;
    float m_oceanWavesAmount;
    float m_oceanWavesSize;

public:
    float m_dawnStart;
    float m_dawnEnd;
    float m_duskStart;
    float m_duskEnd;

    float m_fParticlesAmbientMultiplier;
    float m_fParticlesLightMultiplier;
    // film characteristic curve tweakables
    Vec4 m_vHDRFilmCurveParams;
    Vec3 m_vHDREyeAdaptation;
    Vec3 m_vHDREyeAdaptationLegacy;
    float m_fHDRBloomAmount;

    // hdr color grading
    Vec3 m_vColorBalance;
    float m_fHDRSaturation;

    // default post effect group path
    const char* m_defaultPostEffectGroup = "Libs/PostEffectGroups/Default.xml";

#ifndef _RELEASE
    CDebugDrawListMgr m_DebugDrawListMgr;
#endif

#define MAX_SHADOW_CASCADES_NUM 20

    float m_pShadowCascadeConstBias[MAX_SHADOW_CASCADES_NUM];
    float m_pShadowCascadeSlopeBias[MAX_SHADOW_CASCADES_NUM];

    AABB m_CachedShadowsBounds;
    uint  m_nCachedShadowsUpdateStrategy;
    float m_fCachedShadowsCascadeScale;

    // special case for combat mode adjustments
    float m_fSaturation;
    Vec4 m_pPhotoFilterColor;
    float m_fPhotoFilterColorDensity;
    float m_fGrainAmount;
    float m_fSunSpecMult;

    // Level shaders
    _smart_ptr<IMaterial> m_pTerrainWaterMat;
    _smart_ptr<IMaterial> m_pSkyMat;
    _smart_ptr<IMaterial> m_pSkyLowSpecMat;
    _smart_ptr<IMaterial> m_pSunMat;

    // Fog Materials
    _smart_ptr< IMaterial > m_pMatFogVolEllipsoid;
    _smart_ptr< IMaterial > m_pMatFogVolBox;

    void CleanLevelShaders()
    {
        m_pTerrainWaterMat = 0;
        m_pSkyMat = 0;
        m_pSkyLowSpecMat = 0;
        m_pSunMat = 0;

        m_pMatFogVolEllipsoid = 0;
        m_pMatFogVolBox = 0;
    }

    // Render elements
    CRESky* m_pRESky;
    CREHDRSky* m_pREHDRSky;

    int m_nDeferredLightsNum;

    mutable Vec3* m_pWindSamplePositions;
    mutable size_t m_nWindSamplePositions;

    // functions SRenderingPass
    virtual CCamera* GetRenderingPassCamera(const CCamera& rCamera);

    virtual void GetSvoStaticTextures(I3DEngine::SSvoStaticTexInfo& svoInfo, PodArray<I3DEngine::SLightTI>* pLightsTI_S, PodArray<I3DEngine::SLightTI>* pLightsTI_D);
    virtual void GetSvoBricksForUpdate(PodArray<SSvoNodeInfo>& arrNodeInfo, bool getDynamic);

    bool IsShadersSyncLoad() { return m_bContentPrecacheRequested && GetCVars()->e_AutoPrecacheTexturesAndShaders; }
    bool IsStatObjSyncLoad() { return m_bContentPrecacheRequested && GetCVars()->e_AutoPrecacheCgf; }
    float GetAverageCameraSpeed() { return m_fAverageCameraSpeed; }
    Vec3 GetAverageCameraMoveDir() { return m_vAverageCameraMoveDir; }

    typedef std::map<uint64, int> ShadowFrustumListsCacheUsers;
    ShadowFrustumListsCacheUsers m_FrustumsCacheUsers[2];

    struct ILightSource* GetSunEntity();

    void OnCasterDeleted(IShadowCaster* pCaster) override;

    CCullBuffer* GetCoverageBuffer() { return m_pCoverageBuffer; }

    void InitShadowFrustums(const SRenderingPassInfo& passInfo);

    ///////////////////////////////////////////////////////////////////////////////

    virtual void GetLightVolumes(threadID nThreadID, SLightVolume*& pLightVols, uint32& nNumVols);
    virtual uint16 RegisterVolumeForLighting(const Vec3& vPos, f32 fRadius, uint8 nClipVolumeRef, const SRenderingPassInfo& passInfo);

    CLightVolumesMgr m_LightVolumesMgr;

    ///////////////////////////////////////////////////////////////////////////////

    void RemoveEntityLightSources(IRenderNode* pEntity);

    int GetRealLightsNum() { return m_nRealLightsNum; }
    void SetupClearColor();
    void CheckAddLight(CDLight* pLight, const SRenderingPassInfo& passInfo);

    void DrawTextRightAligned(const float x, const float y, const char* format, ...) PRINTF_PARAMS(4, 5);
    void DrawTextRightAligned(const float x, const float y, const float scale, const ColorF& color, const char* format, ...) PRINTF_PARAMS(6, 7);
    void DrawTextLeftAligned(const float x, const float y, const float scale, const ColorF& color, const char* format, ...) PRINTF_PARAMS(6, 7);
    void DrawTextAligned(int flags, const float x, const float y, const float scale, const ColorF& color, const char* format, ...) PRINTF_PARAMS(7, 8);

    void DrawBBoxHelper(const Vec3& vMin, const Vec3& vMax, ColorB col = Col_White) override { DrawBBox(vMin, vMax, col); }
    void DrawBBoxHelper(const AABB& box, ColorB col = Col_White) override { DrawBBox(box, col); }

    float GetLightAmount(CDLight* pLight, const AABB& objBox);

    IStatObj* CreateStatObj();
    virtual IStatObj* CreateStatObjOptionalIndexedMesh(bool createIndexedMesh);

    // Creates a new indexed mesh.
    IIndexedMesh* CreateIndexedMesh();

    void InitMaterialDefautMappingAxis(_smart_ptr<IMaterial> pMat) override;
    _smart_ptr<IMaterial> MakeSystemMaterialFromShaderHelper(const char* sShaderName, SInputShaderResources* Res = NULL) override
    {
        return MakeSystemMaterialFromShader(sShaderName, Res);
    }

    bool CheckMinSpecHelper(uint32 nMinSpec) override { return CheckMinSpec(nMinSpec); }

    virtual IVisAreaManager* GetIVisAreaManager() { return (IVisAreaManager*)m_pVisAreaManager; }
    bool CreateOcean(_smart_ptr<IMaterial> pTerrainWaterMat, float waterLevel) override;
    void DeleteOcean() override;
    void ChangeOceanMaterial(_smart_ptr<IMaterial> pMat) override;
    void ChangeOceanWaterLevel(float fWaterLevel) override;

    bool LoadVisAreas(std::vector<struct IStatObj*>** ppStatObjTable, std::vector<_smart_ptr<IMaterial> >** ppMatTable);
    bool LoadUsedShadersList();
    bool PrecreateDecals();
    void LoadFlaresData();

    void LoadCollisionClasses(XmlNodeRef node);

    virtual void PrecacheLevel(bool bPrecacheAllVisAreas, Vec3* pPrecachePoints, int nPrecachePointsNum);
    virtual void ProposeContentPrecache() { m_bContentPrecacheRequested = true; }
    bool IsContentPrecacheRequested() { return m_bContentPrecacheRequested; }

    virtual ITimeOfDay* GetTimeOfDay();
    virtual void SetSkyMaterialPath(const string& skyMaterialPath);
    virtual void SetSkyLowSpecMaterialPath(const string& skyMaterialPath);
    virtual void LoadSkyMaterial();
    virtual _smart_ptr<IMaterial> GetSkyMaterial();
    void SetSkyMaterial(_smart_ptr<IMaterial> pSkyMat) override;
    bool IsHDRSkyMaterial(_smart_ptr<IMaterial> pMat) const;

    using I3DEngine::SetGlobalParameter;
    virtual void SetGlobalParameter(E3DEngineParameter param, const Vec3& v);
    using I3DEngine::GetGlobalParameter;
    virtual void GetGlobalParameter(E3DEngineParameter param, Vec3& v);
    virtual void SetShadowMode(EShadowMode shadowMode) { m_eShadowMode = shadowMode; }
    virtual EShadowMode GetShadowMode() const { return m_eShadowMode; }
    virtual void AddPerObjectShadow(IShadowCaster* pCaster, float fConstBias, float fSlopeBias, float fJitter, const Vec3& vBBoxScale, uint nTexSize);
    virtual void RemovePerObjectShadow(IShadowCaster* pCaster);
    virtual struct SPerObjectShadow* GetPerObjectShadow(IShadowCaster* pCaster);
    virtual void GetCustomShadowMapFrustums(ShadowMapFrustum*& arrFrustums, int& nFrustumCount);
    virtual int SaveStatObj(IStatObj* pStatObj, TSerialize ser);
    virtual IStatObj* LoadStatObj(TSerialize ser);

    virtual bool CheckIntersectClouds(const Vec3& p1, const Vec3& p2);
    virtual void OnRenderMeshDeleted(IRenderMesh* pRenderMesh);
    virtual bool RayObjectsIntersection2D(Vec3 vStart, Vec3 vEnd, Vec3& vHitPoint, EERType eERType);
    virtual bool RenderMeshRayIntersection(IRenderMesh* pRenderMesh, SRayHitInfo& hitInfo, _smart_ptr<IMaterial> pCustomMtl = 0);

    virtual IOpticsManager* GetOpticsManager() { return m_pOpticsManager; }

    virtual void RegisterForStreaming(IStreamable* pObj);
    virtual void UnregisterForStreaming(IStreamable* pObj);

    virtual void PrecacheRenderNode(IRenderNode* pObj, float fEntDistanceReal);

    void MarkRNTmpDataPoolForReset() { m_bResetRNTmpDataPool = true; }
    SBending* GetBendingEntry(SBending*, const SRenderingPassInfo& passInfo);

    static void GetObjectsByTypeGlobal(PodArray<IRenderNode*>& lstObjects, EERType objType, const AABB* pBBox, ObjectTreeQueryFilterCallback filterCallback = nullptr);
    static void MoveObjectsIntoListGlobal(PodArray<SRNInfo>* plstResultEntities, const AABB* pAreaBox, bool bRemoveObjects = false, bool bSkipDecals = false, bool bSkip_ERF_NO_DECALNODE_DECALS = false, bool bSkipDynamicObjects = false, EERType eRNType = eERType_TypesNum);

    bool IsObjectTreeReady() override
    {
        return m_pObjectsTree != nullptr;
    }

    IOctreeNode* GetIObjectTree() override
    {
        return (IOctreeNode*)m_pObjectsTree;
    }

    inline COctreeNode* GetObjectTree()
    {
        return m_pObjectsTree;
    }

    inline void SetObjectTree(COctreeNode* tree)
    {
        m_pObjectsTree = tree;
    }

    int m_idMatLeaves; // for shooting foliages
    bool m_bResetRNTmpDataPool;

    float m_fRefreshSceneDataCVarsSumm;
    int m_nRenderTypeEnableCVarSum;

    PodArray<IRenderNode*> m_lstAlwaysVisible;
    PodArray<SPerObjectShadow> m_lstPerObjectShadows;
    PodArray<ShadowMapFrustum> m_lstCustomShadowFrustums;
    int m_nCustomShadowFrustumCount;

    CryCriticalSection m_checkCreateRNTmpData;
    CThreadSafeRendererContainer<SFrameInfo> m_elementFrameInfo;
    CRNTmpData m_LTPRootFree, m_LTPRootUsed;
    void CreateRNTmpData(CRNTmpData** ppInfo, IRenderNode* pRNode, const SRenderingPassInfo& passInfo);
    void CheckCreateRNTmpData(CRNTmpData** ppInfo, IRenderNode* pRNode, const SRenderingPassInfo& passInfo) override
    {
        // Lock to avoid a situation where two threads simultaneously find that *ppinfo is null,
        // which would result in two CRNTmpData objects for the same owner which eventually leads to a crash
        AUTO_LOCK(m_checkCreateRNTmpData);
        if (CRNTmpData* tmpData = (*ppInfo))
        {
            m_elementFrameInfo[tmpData->nFrameInfoId].nLastUsedFrameId = passInfo.GetMainFrameID();
        }
        else
        {
            CreateRNTmpData(ppInfo, pRNode, passInfo);
        }
    }

    uint32 GetFrameInfoId(CRNTmpData** ppRNTmpData, uint32 createdFrameID)
    {
        size_t index = 0;
        SFrameInfo* pFrameInfo = m_elementFrameInfo.push_back_new(index);
        pFrameInfo->nCreatedFrameId = createdFrameID;
        pFrameInfo->nLastUsedFrameId = createdFrameID;
        pFrameInfo->ppRNTmpData = ppRNTmpData;
        pFrameInfo->bIsValid = true;
        return index;
    }

    void FreeRNTmpData(CRNTmpData** ppInfo);

    void FreeRNTmpDataPool();
    void UpdateRNTmpDataPool(bool bFreeAll);

    //  CPoolAllocator<CRNTmpData, 512> m_RNTmpDataPools;

    void UpdateStatInstGroups();
    void UpdateRenderTypeEnableLookup();
    void ProcessOcean(const SRenderingPassInfo& passInfo);
    Vec3 GetEntityRegisterPoint(IRenderNode* pEnt);

    virtual void RenderRenderNode_ShadowPass(IShadowCaster* pRNode, const SRenderingPassInfo& passInfo, AZ::LegacyJobExecutor* pJobExecutor);
    void ProcessCVarsChange();
    ILINE int GetGeomDetailScreenRes()
    {
        return GetCVars()->e_ForceDetailLevelForScreenRes ? GetCVars()->e_ForceDetailLevelForScreenRes : GetRenderer()->GetWidth();
    }

    int GetBlackTexID() { return m_nBlackTexID; }
    int GetBlackCMTexID() { return m_nBlackCMTexID; }

    virtual void SyncProcessStreamingUpdate();

    virtual void SetScreenshotCallback(IScreenshotCallback* pCallback);

    virtual IDeferredPhysicsEventManager* GetDeferredPhysicsEventManager() { return m_pDeferredPhysicsEventManager; }

    void PrintDebugInfo(const SRenderingPassInfo& passInfo);

    SImageSubInfo* RegisterImageInfo(byte** pMips, int nDim, const char* pName);
    SImageSubInfo* GetImageInfo(const char* pName);
    std::map<string, SImageSubInfo*> m_imageInfos;
    byte** AllocateMips(byte* pImage, int nDim, byte** pImageMips);
    IScreenshotCallback* m_pScreenshotCallback;
    OcclusionTestClient m_OceanOcclTestVar;
    bool m_bInShutDown;
    bool m_bInUnload;
    bool m_bInLoad;

    IDeferredPhysicsEventManager* m_pDeferredPhysicsEventManager;

    std::set<uint16> m_skipedLayers;

    IGeneralMemoryHeap* m_pBreakableBrushHeap;

    AZStd::mutex m_statObjQueueLock;
    std::queue<StaticObjectAsyncLoadRequest> m_statObjLoadRequests;

    typedef std::list<ITextureLoadHandler*> TTextureLoadHandlers;
    TTextureLoadHandlers m_textureLoadHandlers;

    class PhysicsAreaUpdates
    {
    public:
        void SetAreaDirty(const SAreaChangeRecord& rec);

        uint32 CreateProxy(const IRenderNode* pRenderNode, uint16 uPhysicsMask);

        void UpdateProxy(const IRenderNode* pRenderNode, uint32 nProxyId);

        void ResetProxy(uint32 proxyId);

        void Update();

        void Reset();

        void GarbageCollect();

        CryCriticalSection m_Mutex;
    private:
        CThreadSafeRendererContainer<SPhysAreaNodeProxy> m_Proxies;
        PodArray<SAreaChangeRecord> m_DirtyAreas;
    };

    PhysicsAreaUpdates& GetPhysicsAreaUpdates()
    {
        return m_PhysicsAreaUpdates;
    }

    //I3DEngine Overrides START
    void GetStatObjAndMatTables(DynArray<IStatObj*>* pStatObjTable, DynArray<_smart_ptr<IMaterial> >* pMatTable, DynArray<IStatInstGroup*>* pStatInstGroupTable, uint32 nObjTypeMask) override;
    void WaitForCullingJobsCompletion() override;
    //I3DEngine Overrides END

private:

    // IProcess Implementation
    void    SetFlags(int flags) { m_nFlags = flags; }
    int     GetFlags(void) { return m_nFlags; }
    int     m_nFlags;

    COctreeNode* m_pObjectsTree;

    std::vector<byte> arrFPSforSaveLevelStats;
    PodArray<float> m_arrProcessStreamingLatencyTestResults;
    PodArray<int> m_arrProcessStreamingLatencyTexNum;

    // fields which are used by SRenderingPass to store over frame information
    CThreadSafeRendererContainer<CCamera>                                   m_RenderingPassCameras[2]; // camera storage for SRenderingPass, the cameras cannot be stored on stack to allow job execution

    float m_fZoomFactor;                                                                // zoom factor of m_RenderingCamera
    // cameras used by 3DEngine
    CCamera     m_RenderingCamera;                                              // Camera used for Rendering on 3DEngine Side, normally equal to the view camera, except if frozen with e_camerafreeze

    PodArray<IRenderNode*>  m_deferredRenderComponentStreamingPriorityUpdates;      // deferred streaming priority updates for newly seen CComponentRenders

    float m_fLightsHDRDynamicPowerFactor; // lights hdr exponent/exposure

    int m_nBlackTexID;
    int m_nBlackCMTexID;
    char m_sGetLevelFilePathTmpBuff[AZ_MAX_PATH_LEN];
    char m_szLevelFolder[_MAX_PATH];

    bool m_bOcean; // todo: remove
    
    // Ocean Caustics - Should be removed once the Ocean Gem is done and the feature toggle for it is removed.
    float m_oceanCausticsDistanceAtten;
    float m_oceanCausticsTiling;
    float m_oceanCausticDepth;
    float m_oceanCausticIntensity;

    Vec3 m_vSkyHightlightPos;
    Vec3 m_vSkyHightlightCol;
    float m_fSkyHighlightSize;
    Vec3 m_vAmbGroundCol;
    float m_fAmbMaxHeight;
    float m_fAmbMinHeight;
    uint8 m_nOceanRenderFlags;
    Vec3  m_vPrevMainFrameCamPos;
    float m_fAverageCameraSpeed;
    Vec3 m_vAverageCameraMoveDir;
    EShadowMode m_eShadowMode;
    bool m_bLayersActivated;
    bool m_bContentPrecacheRequested;
    bool m_bTerrainTextureStreamingInProgress;
    bool m_bSegmentOperationInProgress;

    // interfaces
    IPhysMaterialEnumerator* m_pPhysMaterialEnumerator;

    // data containers
    int m_nRealLightsNum;

    PodArray<ILightSource*> m_lstStaticLights;

    PodArray<SCollisionClass> m_collisionClasses;

#define MAX_LIGHTS_NUM 32
    PodArray<CCamera> m_arrLightProjFrustums;

    class CTimeOfDay*    m_pTimeOfDay;

    std::unique_ptr<class IPostEffectGroupManager> m_postEffectGroups;
    class IPostEffectGroup* m_postEffectBaseGroup;

    ICVar*                  m_pLightQuality;

    // FPS for savelevelstats

    float m_fAverageFPS;
    float m_fMinFPS, m_fMinFPSDecay;
    float m_fMaxFPS, m_fMaxFPSDecay;
    int  m_nFramesSinceLevelStart;
    int  m_nStreamingFramesSinceLevelStart;
    bool m_bPreCacheEndEventSent;
    float m_fTimeStateStarted;
    uint32 m_nRenderWorldUSecs;
    SFrameLodInfo m_frameLodInfo;

    ITexture*   m_ptexIconLowMemoryUsage;
    ITexture*   m_ptexIconAverageMemoryUsage;
    ITexture*   m_ptexIconHighMemoryUsage;
    ITexture*   m_ptexIconEditorConnectedToConsole;

    std::vector<IDecalRenderNode*> m_decalRenderNodes; // list of registered decal render nodes, used to clean up longer not drawn decals

    class PhysicsAreaUpdatesHandler;
    AZStd::unique_ptr<PhysicsAreaUpdatesHandler> m_physicsAreaUpdatesHandler;

    PhysicsAreaUpdates m_PhysicsAreaUpdates;

    string m_skyMatName;
    string m_skyLowSpecMatName;

    // Variable to keep track if the cvar e_SkyType has changed, which may cause the engine to load a different sky material
    int m_previousSkyType = -1;

    //Bending Pools contain the per frame Vegetation, Decals, etc. structures. Each Engine-frame the next buffer is
    // cleared and set as the current in a ring and page-allocated to fit the needs of the current frame.
    static const int NUM_BENDING_POOLS = 4;
    CThreadSafeRendererContainer<SBending> m_bendingPool[NUM_BENDING_POOLS];
    int m_bendingPoolIdx;

    bool m_levelLoaded;

    // not sorted

    void LoadTimeOfDaySettingsFromXML(XmlNodeRef node);
    char* GetXMLAttribText(XmlNodeRef pInputNode, const char* szLevel1, const char* szLevel2, const char* szDefaultValue);
    char* GetXMLAttribText(XmlNodeRef pInputNode, const char* szLevel1, const char* szLevel2, const char* szLevel3, const char* szDefaultValue);

    // without calling high level functions like panorama screenshot
    void RenderInternal(const int nRenderFlags, const SRenderingPassInfo& passInfo, const char* szDebugName);

    bool IsCameraAnd3DEngineInvalid(const SRenderingPassInfo& passInfo, const char* szCaller);

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    void ResetCasterCombinationsCache();

    void DeleteAllStaticLightSources();

    void UpdateSun(const SRenderingPassInfo& passInfo);
    void SubmitSun(const SRenderingPassInfo& passInfo);

    CCullBuffer* m_pCoverageBuffer;
    struct CLightEntity* m_pSun;

    void UpdateMoonDirection();

    // Copy objects from tree
    void CopyObjectsByType(EERType objType, const AABB* pBox, PodArray<IRenderNode*>* plistObjects, ObjectTreeQueryFilterCallback filterCallback = nullptr);
    void CopyObjects(const AABB* pBox, PodArray<IRenderNode*>* plistObjects);

    void CleanUpOldDecals();

    template<typename TReturn>
    using LoadStatObjFunc = TReturn(CObjManager::*)(const char* filename, const char* _szGeomName, IStatObj::SSubObject** ppSubObject, bool bUseStreaming, unsigned long nLoadingFlags, const void* pData, int nDataSize, const char* szBlockName);

    template<typename TReturn>
    TReturn LoadStatObjInternal(const char* fileName, const char* geomName, IStatObj::SSubObject** subObject, bool useStreaming, 
        unsigned long loadingFlags, LoadStatObjFunc<TReturn> loadStatObjFunc, const void* data = nullptr, int dataSize = 0);

    bool RemoveObjectsInArea(Vec3 vExploPos, float fExploRadius);

    ///////////////////////////////////////////////////////////////////////////
    // Octree Loading/Saving related START
    ///////////////////////////////////////////////////////////////////////////


    //! initialWorldSize: in Meters.
    bool CreateOctree(float initialWorldSize);
    
    void DestroyOctree();

    ///////////////////////////////////////////////////////////////////////////
    // Octree Loading/Saving related END
    ///////////////////////////////////////////////////////////////////////////
};

#endif // CRYINCLUDE_CRY3DENGINE_3DENGINE_H
