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

#include <AzCore/Math/Aabb.h>

#include <I3DEngine.h>
#include <ISerialize.h>
#include <gmock/gmock.h>

// the following was generated using google's python script to autogenerate mocks.
// however, it needed some hand-editing to make it work, so if you add functions to I3DEngine,
// it will probably be better to just manually add them here than try to run the script again.

class I3DEngineMock 
    : public I3DEngine 
{
public:
    // From IProcess
    MOCK_METHOD1(SetFlags,
        void(int));
    MOCK_METHOD0(GetFlags,
        int(void));

    // From I3DEngine
    MOCK_METHOD0(Init,
        bool());
    MOCK_METHOD1(SetLevelPath,
        void(const char* szFolderName));
    MOCK_METHOD1(CheckMinSpec,
        bool(uint32 nMinSpec));
    MOCK_METHOD1(PrepareOcclusion,
        void(const CCamera& rCamera));
    MOCK_METHOD0(EndOcclusion,
        void());
    MOCK_METHOD2(LoadLevel,
        bool(const char* szFolderName, const char* szMissionName));
    MOCK_METHOD2(InitLevelForEditor,
        bool(const char* szFolderName, const char* szMissionName));
    MOCK_METHOD0(LevelLoadingInProgress,
        bool());
    MOCK_METHOD0(OnFrameStart,
        void());
    MOCK_METHOD0(PostLoadLevel,
        void());
    MOCK_METHOD0(LoadEmptyLevel,
        void());
    MOCK_METHOD0(UnloadLevel,
        void());
    MOCK_METHOD0(Update,
        void());
    MOCK_CONST_METHOD0(GetRenderingCamera,
        const CCamera&());
    MOCK_CONST_METHOD0(GetZoomFactor,
        float());
    MOCK_METHOD0(Tick,
        void());
    MOCK_METHOD0(UpdateShaderItems,
        void());
    MOCK_METHOD0(Release,
        void());
    MOCK_METHOD3(RenderWorld,
        void(int nRenderFlags, const SRenderingPassInfo& passInfo, const char* szDebugName));
    MOCK_METHOD2(RenderSceneReflection,
        void(int nRenderFlags, const SRenderingPassInfo& passInfo));
    MOCK_METHOD1(PreWorldStreamUpdate,
        void(const CCamera& cam));
    MOCK_METHOD0(WorldStreamUpdate,
        void());
    MOCK_METHOD0(ShutDown,
        void());
    MOCK_METHOD7(LoadStatObjUnsafeManualRef,
        IStatObj*(const char* fileName, const char* geomName, IStatObj::SSubObject** subObject,
            bool useStreaming, unsigned long loadingFlags, const void* data, int dataSize));
    MOCK_METHOD7(LoadStatObjAutoRef,
        _smart_ptr<IStatObj>(const char* fileName, const char* geomName, IStatObj::SSubObject** subObject,
            bool useStreaming, unsigned long loadingFlags, const void* data, int dataSize));
    MOCK_METHOD0(ProcessAsyncStaticObjectLoadRequests,
        void());
    MOCK_METHOD5(LoadStatObjAsync,
        void(LoadStaticObjectAsyncResult resultCallback, const char* szFileName, const char* szGeomName, bool bUseStreaming, unsigned long nLoadingFlags));
    MOCK_METHOD1(FindStatObjectByFilename,
        IStatObj*(const char* filename));
    MOCK_METHOD0(GetGSMRange,
        const float());
    MOCK_METHOD0(GetGSMRangeStep,
        const float());
    MOCK_METHOD0(GetLoadedObjectCount,
        int());
    MOCK_METHOD2(GetLoadedStatObjArray,
        void(IStatObj** pObjectsArray, int& nCount));
    MOCK_METHOD1(GetObjectsStreamingStatus,
        void(SObjectsStreamingStatus& outStatus));
    MOCK_METHOD2(GetStreamingSubsystemData,
        void(int subsystem, SStremaingBandwidthData& outData));
    MOCK_METHOD3(RegisterEntity,
        void(IRenderNode* pEntity, int nSID, int nSIDConsideredSafe));
    MOCK_METHOD1(SelectEntity,
        void(IRenderNode* pEntity));
    MOCK_METHOD0(IsSunShadows,
        bool());
    MOCK_METHOD2(MakeSystemMaterialFromShaderHelper
        , _smart_ptr<IMaterial>(const char* sShaderName, SInputShaderResources* Res));
    MOCK_METHOD1(CheckMinSpecHelper
        , bool(uint32 nMinSpec));
    MOCK_METHOD1(OnCasterDeleted
        , void(IShadowCaster* pCaster));
    MOCK_METHOD4(GetStatObjAndMatTables,
        void(DynArray<IStatObj*>* pStatObjTable, DynArray<_smart_ptr<IMaterial>>* pMatTable, DynArray<IStatInstGroup*>* pStatInstGroupTable, uint32 nObjTypeMask));
#ifndef _RELEASE
    MOCK_METHOD1(AddObjToDebugDrawList,
        void(SObjectInfoToAddToDebugDrawList& objInfo));
    MOCK_CONST_METHOD0(IsDebugDrawListEnabled,
        bool());
#endif
    MOCK_METHOD1(UnRegisterEntityDirect,
        void(IRenderNode* pEntity));
    MOCK_METHOD1(UnRegisterEntityAsJob,
        void(IRenderNode* pEnt));
    MOCK_CONST_METHOD1(IsUnderWater,
        bool(const Vec3& vPos));
    MOCK_METHOD1(SetOceanRenderFlags,
        void(uint8 nFlags));
    MOCK_CONST_METHOD0(GetOceanRenderFlags,
        uint8());
    MOCK_CONST_METHOD0(GetOceanVisiblePixelsCount,
        uint32());
    MOCK_METHOD3(GetBottomLevel,
        float(const Vec3& referencePos, float maxRelevantDepth, int objtypes));
    MOCK_METHOD2(GetBottomLevel,
        float(const Vec3& referencePos, float maxRelevantDepth));
    MOCK_METHOD2(GetBottomLevel,
        float(const Vec3& referencePos, int objflags));
    MOCK_METHOD0(GetWaterLevel,
        float());
    MOCK_METHOD3(GetWaterLevel,
        float(const Vec3* pvPos, IPhysicalEntity* pent, bool bAccurate));
    MOCK_CONST_METHOD1(GetAccurateOceanHeight,
        float(const Vec3& pCurrPos));
    MOCK_CONST_METHOD0(GetCausticsParams,
        CausticsParams());
    MOCK_CONST_METHOD0(GetOceanAnimationParams,
        OceanAnimationData());
    MOCK_CONST_METHOD1(GetHDRSetupParams,
        void(Vec4 pParams[5]));
    MOCK_METHOD0(ResetParticlesAndDecals,
        void());
    MOCK_METHOD1(CreateDecal,
        void(const CryEngineDecalInfo& Decal));
    MOCK_METHOD2(DeleteDecalsInRange,
        void(AABB* pAreaBox, IRenderNode* pEntity));
    MOCK_METHOD1(SetSunColor,
        void(Vec3 vColor));
    MOCK_METHOD0(GetSunAnimColor,
        Vec3());
    MOCK_METHOD1(SetSunAnimColor,
        void(const Vec3& color));
    MOCK_METHOD0(GetSunAnimSpeed,
        float());
    MOCK_METHOD1(SetSunAnimSpeed,
        void(float sunAnimSpeed));
    MOCK_METHOD0(GetSunAnimPhase,
        AZ::u8());
    MOCK_METHOD1(SetSunAnimPhase,
        void(AZ::u8 sunAnimPhase));
    MOCK_METHOD0(GetSunAnimIndex,
        AZ::u8());
    MOCK_METHOD1(SetSunAnimIndex,
        void(AZ::u8 sunAnimIndex));
    MOCK_METHOD1(SetRainParams,
        void(const SRainParams& rainParams));
    MOCK_METHOD1(GetRainParams,
        bool(SRainParams& rainParams));
    MOCK_METHOD5(SetSnowSurfaceParams,
        void(const Vec3& vCenter, float fRadius, float fSnowAmount, float fFrostAmount, float fSurfaceFreezing));
    MOCK_METHOD5(GetSnowSurfaceParams,
        bool(Vec3& vCenter, float& fRadius, float& fSnowAmount, float& fFrostAmount, float& fSurfaceFreezing));
    MOCK_METHOD7(SetSnowFallParams,
        void(int nSnowFlakeCount, float fSnowFlakeSize, float fSnowFallBrightness, float fSnowFallGravityScale, float fSnowFallWindScale, float fSnowFallTurbulence, float fSnowFallTurbulenceFreq));
    MOCK_METHOD7(GetSnowFallParams,
        bool(int& nSnowFlakeCount, float& fSnowFlakeSize, float& fSnowFallBrightness, float& fSnowFallGravityScale, float& fSnowFallWindScale, float& fSnowFallTurbulence, float& fSnowFallTurbulenceFreq));
    MOCK_METHOD1(SetMaxViewDistanceScale,
        void(float fScale));
    MOCK_METHOD1(GetMaxViewDistance,
        float(bool));
    MOCK_CONST_METHOD0(GetFrameLodInfo,
        const SFrameLodInfo&());
    MOCK_METHOD1(SetFrameLodInfo,
        void(const SFrameLodInfo& frameLodInfo));
    MOCK_METHOD1(SetFogColor,
        void(const Vec3& vFogColor));
    MOCK_METHOD0(GetFogColor,
        Vec3());
    MOCK_METHOD6(GetSkyLightParameters,
        void(Vec3& sunDir, Vec3& sunIntensity, float& Km, float& Kr, float& g, Vec3& rgbWaveLengths));
    MOCK_METHOD7(SetSkyLightParameters,
        void(const Vec3& sunDir, const Vec3& sunIntensity, float Km, float Kr, float g, const Vec3& rgbWaveLengths, bool forceImmediateUpdate));
    MOCK_CONST_METHOD0(GetLightsHDRDynamicPowerFactor,
        float());
    MOCK_CONST_METHOD3(IsTessellationAllowed,
        bool(const CRenderObject* pObj, const SRenderingPassInfo& passInfo, bool bIgnoreShadowPass));
    MOCK_METHOD3(SetRenderNodeMaterialAtPosition,
        void(EERType eNodeType, const Vec3& vPos, _smart_ptr<IMaterial> pMat));
    MOCK_METHOD1(OverrideCameraPrecachePoint,
        void(const Vec3& vPos));
    MOCK_METHOD4(AddPrecachePoint,
        int(const Vec3& vPos, const Vec3& vDir, float fTimeOut, float fImportanceFactor));
    MOCK_METHOD1(ClearPrecachePoint,
        void(int id));
    MOCK_METHOD0(ClearAllPrecachePoints,
        void());
    MOCK_METHOD1(GetPrecacheRoundIds,
        void(int pRoundIds[MAX_STREAM_PREDICTION_ZONES]));
    MOCK_METHOD5(TraceFogVolumes,
        void(const Vec3& vPos, const AABB& objBBox, SFogVolumeData& fogVolData, const SRenderingPassInfo& passInfo, bool fogVolumeShadingQuality));

    MOCK_METHOD1(RemoveAllStaticObjects,
        void(int));
    MOCK_METHOD3(SetStatInstGroup,
        bool(int nGroupId, const IStatInstGroup& siGroup, int nSID));
    MOCK_METHOD3(GetStatInstGroup,
        bool(int, IStatInstGroup&, int));
    MOCK_METHOD3(OnExplosion,
        void(Vec3, float, bool));
    MOCK_METHOD1(SetPhysMaterialEnumerator,
        void(IPhysMaterialEnumerator* pPhysMaterialEnumerator));
    MOCK_METHOD0(GetPhysMaterialEnumerator,
        IPhysMaterialEnumerator*());
    MOCK_METHOD0(SetupDistanceFog,
        void());
    MOCK_METHOD1(LoadMissionDataFromXMLNode,
        void(const char* szMissionName));
    MOCK_METHOD2(LoadEnvironmentSettingsFromXML,
        void(XmlNodeRef, int));
    MOCK_METHOD0(LoadCompiledOctreeForEditor,
        bool());
    MOCK_CONST_METHOD0(GetSunDir,
        Vec3());
    MOCK_CONST_METHOD0(GetSunDirNormalized,
        Vec3());
    MOCK_CONST_METHOD0(GetRealtimeSunDirNormalized,
        Vec3());
    MOCK_METHOD0(GetDistanceToSectorWithWater,
        float());
    MOCK_CONST_METHOD0(GetSunColor,
        Vec3());
    MOCK_CONST_METHOD0(GetSSAOAmount,
        float());
    MOCK_CONST_METHOD0(GetSSAOContrast,
        float());
    MOCK_METHOD1(FreeRenderNodeState,
        void(IRenderNode* pEntity));
    MOCK_METHOD1(GetLevelFilePath,
        const char*(const char* szFileName));
    MOCK_METHOD4(DisplayInfo,
        void(float& fTextPosX, float& fTextPosY, float& fTextStepY, bool bEnhanced));
    MOCK_METHOD0(DisplayMemoryStatistics,
        void());

    // Can't mock methods with variable parameters so just create empty bodies for them.
    void DrawTextRightAligned([[maybe_unused]] const float x, [[maybe_unused]] const float y, [[maybe_unused]] const char* format, ...) override {}
    void DrawTextRightAligned([[maybe_unused]] const float x, [[maybe_unused]] const float y, [[maybe_unused]] const float scale, [[maybe_unused]] const ColorF& color, [[maybe_unused]] const char* format, ...) override {}

    MOCK_METHOD3(DrawBBoxHelper
        , void (const Vec3& vMin, const Vec3& vMax, ColorB col));
    MOCK_METHOD2(DrawBBoxHelper
        , void (const AABB& box, ColorB col));
    
    MOCK_METHOD3(ActivatePortal,
        void(const Vec3& vPos, bool bActivate, const char* szEntityName));
    MOCK_CONST_METHOD1(GetMemoryUsage,
        void(ICrySizer* pSizer));
    MOCK_METHOD2(GetResourceMemoryUsage,
        void(ICrySizer* pSizer, const AABB& cstAABB));
    MOCK_METHOD1(CreateVisArea,
        IVisArea*(uint64 visGUID));
    MOCK_METHOD1(DeleteVisArea,
        void(IVisArea* pVisArea));
    MOCK_METHOD6(UpdateVisArea,
        void(IVisArea* pArea, const Vec3* pPoints, int nCount, const char* szName, const SVisAreaInfo& info, bool bReregisterObjects));
    MOCK_METHOD4(IsVisAreasConnected,
        bool(IVisArea* pArea1, IVisArea* pArea2, int nMaxRecursion, bool bSkipDisabledPortals));
    MOCK_METHOD0(CreateClipVolume,
        IClipVolume*());
    MOCK_METHOD1(DeleteClipVolume,
        void(IClipVolume* pClipVolume));
    MOCK_METHOD7(UpdateClipVolume,
        void(IClipVolume* pClipVolume, _smart_ptr<IRenderMesh> pRenderMesh, IBSPTree3D* pBspTree, const Matrix34& worldTM, bool bActive, uint32 flags, const char* szName));
    MOCK_METHOD1(CreateRenderNode,
        IRenderNode*(EERType type));
    MOCK_METHOD1(DeleteRenderNode,
        void(IRenderNode* pRenderNode));
    MOCK_METHOD1(SetWind,
        void(const Vec3& vWind));
    MOCK_CONST_METHOD2(GetWind,
        Vec3(const AABB& box, bool bIndoors));
    MOCK_CONST_METHOD1(GetGlobalWind,
        Vec3(bool bIndoors));
    MOCK_CONST_METHOD4(SampleWind,
        bool(Vec3* pSamples, int nSamples, const AABB& volume, bool bIndoors));
    MOCK_METHOD1(GetVisAreaFromPos,
        IVisArea*(const Vec3& vPos));
    MOCK_METHOD2(IntersectsVisAreas,
        bool(const AABB& box, void** pNodeCache));
    MOCK_METHOD4(ClipToVisAreas,
        bool(IVisArea* pInside, Sphere& sphere, Vec3 const& vNormal, void* pNodeCache));
    MOCK_METHOD1(EnableOceanRendering,
        void(bool bOcean));
    MOCK_METHOD1(AddTextureLoadHandler,
        void(ITextureLoadHandler* pHandler));
    MOCK_METHOD1(RemoveTextureLoadHandler,
        void(ITextureLoadHandler* pHandler));
    MOCK_METHOD1(GetTextureLoadHandlerForImage,
        ITextureLoadHandler*(const char* ext));
    MOCK_METHOD0(CreateLightSource,
        struct ILightSource*());
    MOCK_METHOD1(DeleteLightSource,
        void(ILightSource* pLightSource));
    MOCK_METHOD0(GetLightEntities,
        const PodArray<ILightSource*>* ());
    MOCK_METHOD3(GetLightVolumes,
        void(threadID nThreadID, SLightVolume*& pLightVols, uint32& nNumVols));
    MOCK_METHOD4(RegisterVolumeForLighting,
        uint16(const Vec3& vPos, f32 fRadius, uint8 nClipVolumeRef, const SRenderingPassInfo& passInfo));
    MOCK_METHOD1(RestoreTerrainFromDisk,
        bool(int));
    MOCK_METHOD1(GetFilePath,
        const char*(const char* szFileName));
    MOCK_CONST_METHOD0(GetPostEffectGroups,
        class IPostEffectGroupManager*());
    MOCK_CONST_METHOD0(GetPostEffectBaseGroup,
        class IPostEffectGroup*());
    MOCK_CONST_METHOD3(SetPostEffectParam,
        void(const char* pParam, float fValue, bool bForceValue));
    MOCK_CONST_METHOD3(SetPostEffectParamVec4,
        void(const char* pParam, const Vec4& pValue, bool bForceValue));
    MOCK_CONST_METHOD2(SetPostEffectParamString,
        void(const char* pParam, const char* pszArg));
    MOCK_CONST_METHOD2(GetPostEffectParam,
        void(const char* pParam, float& fValue));
    MOCK_CONST_METHOD2(GetPostEffectParamVec4,
        void(const char* pParam, Vec4& pValue));
    MOCK_CONST_METHOD2(GetPostEffectParamString,
        void(const char* pParam, const char*& pszArg));
    MOCK_METHOD1(GetPostEffectID,
        int32(const char* pPostEffectName));
    MOCK_METHOD1(ResetPostEffects,
        void(bool));
    MOCK_METHOD0(DisablePostEffects,
        void());
    MOCK_METHOD1(SetShadowsGSMCache,
        void(bool bCache));
    MOCK_METHOD2(SetCachedShadowBounds,
        void(const AABB& shadowBounds, float fAdditionalCascadesScale));
    MOCK_METHOD1(SetRecomputeCachedShadows,
        void(uint));
    MOCK_METHOD0(CheckMemoryHeap,
        void());
    MOCK_METHOD1(DeleteEntityDecals,
        void(IRenderNode* pEntity));
    MOCK_METHOD0(LockCGFResources,
        void());
    MOCK_METHOD0(UnlockCGFResources,
        void());
    MOCK_METHOD0(FreeUnusedCGFResources,
        void());
    MOCK_METHOD0(CreateStatObj,
        IStatObj*());
    MOCK_METHOD1(CreateStatObjOptionalIndexedMesh,
        IStatObj*(bool createIndexedMesh));
    MOCK_METHOD0(CreateIndexedMesh,
        IIndexedMesh*());
    MOCK_METHOD1(SerializeState,
        void(TSerialize ser));
    MOCK_METHOD1(PostSerialize,
        void(bool bReading));
    MOCK_METHOD0(GetMaterialHelpers,
        IMaterialHelpers&());
    MOCK_METHOD0(GetMaterialManager,
        IMaterialManager*());
    MOCK_METHOD0(GetObjManager,
        IObjManager*());
    MOCK_METHOD1(CreateChunkfileContent,
        CContentCGF*(const char* filename));
    MOCK_METHOD1(ReleaseChunkfileContent,
        void(CContentCGF*));
    MOCK_METHOD4(LoadChunkFileContent,
        bool(CContentCGF* pCGF, const char* filename, bool bNoWarningMode, bool bCopyChunkFile));
    MOCK_METHOD6(LoadChunkFileContentFromMem,
        bool(CContentCGF* pCGF, const void* pData, size_t nDataLen, uint32 nLoadingFlags, bool bNoWarningMode, bool bCopyChunkFile));
    MOCK_METHOD1(CreateChunkFile,
        IChunkFile*(bool));
    MOCK_CONST_METHOD3(CreateChunkFileWriter,
        ChunkFile::IChunkFileWriter*(EChunkFileFormat eFormat, AZ::IO::IArchive* pPak, const char* filename));
    MOCK_CONST_METHOD1(ReleaseChunkFileWriter,
        void(ChunkFile::IChunkFileWriter* p));
    MOCK_METHOD2(CreateOcean,
        bool(_smart_ptr<IMaterial> pTerrainWaterMat, float waterLevel));
    MOCK_METHOD0(DeleteOcean,
        void());
    MOCK_METHOD1(ChangeOceanMaterial,
        void(_smart_ptr<IMaterial> pMat));
    MOCK_METHOD1(ChangeOceanWaterLevel,
        void(float fWaterLevel));
    MOCK_METHOD1(InitMaterialDefautMappingAxis
        , void(_smart_ptr<IMaterial> pMat));
    MOCK_METHOD0(GetIVisAreaManager,
        IVisAreaManager*());
    MOCK_METHOD3(PrecacheLevel,
        void(bool bPrecacheAllVisAreas, Vec3* pPrecachePoints, int nPrecachePointsNum));
    MOCK_METHOD0(ProposeContentPrecache,
        void());
    MOCK_METHOD0(GetTimeOfDay,
        ITimeOfDay*());
    MOCK_METHOD1(SetSkyMaterialPath,
        void(const string& skyMaterialPath));
    MOCK_METHOD1(SetSkyLowSpecMaterialPath,
        void(const string& skyMaterialPath));
    MOCK_METHOD0(LoadSkyMaterial,
        void());
    MOCK_METHOD0(GetSkyMaterial,
        _smart_ptr<IMaterial>());
    MOCK_METHOD1(SetSkyMaterial,
        void(_smart_ptr<IMaterial> pSkyMat));
    MOCK_METHOD2(SetGlobalParameter,
        void(E3DEngineParameter param, const Vec3& v));
    MOCK_METHOD2(GetGlobalParameter,
        void(E3DEngineParameter param, Vec3& v));
    MOCK_METHOD1(SetShadowMode,
        void(EShadowMode shadowMode));
    MOCK_CONST_METHOD0(GetShadowMode,
        EShadowMode());
    MOCK_METHOD6(AddPerObjectShadow,
        void(IShadowCaster* pCaster, float fConstBias, float fSlopeBias, float fJitter, const Vec3& vBBoxScale, uint nTexSize));
    MOCK_METHOD1(RemovePerObjectShadow,
        void(IShadowCaster* pCaster));
    MOCK_METHOD1(GetPerObjectShadow,
        struct SPerObjectShadow*(IShadowCaster* pCaster));
    MOCK_METHOD2(GetCustomShadowMapFrustums,
        void(struct ShadowMapFrustum*& arrFrustums, int& nFrustumCount));
    MOCK_METHOD2(SaveStatObj,
        int(IStatObj* pStatObj, TSerialize ser));
    MOCK_METHOD1(LoadStatObj,
        IStatObj*(TSerialize ser));
    MOCK_METHOD2(CheckIntersectClouds,
        bool(const Vec3& p1, const Vec3& p2));
    MOCK_METHOD1(OnRenderMeshDeleted,
        void(IRenderMesh* pRenderMesh));
    MOCK_METHOD0(DebugDraw_UpdateDebugNode,
        void());
    MOCK_METHOD4(RayObjectsIntersection2D,
        bool(Vec3 vStart, Vec3 vEnd, Vec3& vHitPoint, EERType eERType));
    MOCK_METHOD3(RenderMeshRayIntersection,
        bool(IRenderMesh* pRenderMesh, SRayHitInfo& hitInfo, _smart_ptr<IMaterial> pCustomMtl));
    MOCK_METHOD3(CheckCreateRNTmpData
        , void(CRNTmpData** ppInfo, IRenderNode* pRNode, const SRenderingPassInfo& passInfo));
    MOCK_METHOD1(FreeRNTmpData,
        void(CRNTmpData** ppInfo));
    MOCK_METHOD0(IsObjectTreeReady
        , bool());
    MOCK_METHOD0(GetIObjectTree
        , IOctreeNode*());
    MOCK_METHOD2(GetObjectsByType,
        uint32(EERType, IRenderNode**));
    MOCK_METHOD4(GetObjectsByTypeInBox,
        uint32(EERType objType, const AABB& bbox, IRenderNode** pObjects, ObjectTreeQueryFilterCallback filterCallback));
    MOCK_METHOD2(GetObjectsInBox,
        uint32(const AABB& bbox, IRenderNode** pObjects));
    MOCK_METHOD2(GetObjectsByFlags,
        uint32(uint, IRenderNode**));
    MOCK_METHOD4(GetObjectsByTypeInBox,
        void(EERType objType, const AABB& bbox, PodArray<IRenderNode*>* pLstObjects, ObjectTreeQueryFilterCallback filterCallback));
    MOCK_METHOD2(OnObjectModified,
        void(IRenderNode* pRenderNode, uint dwFlags));
    MOCK_METHOD1(FillDebugFPSInfo,
        void(SDebugFPSInfo&));
    MOCK_METHOD0(GetLevelFolder,
        const char*());
    MOCK_METHOD0(IsAreaActivationInUse,
        bool());
    MOCK_METHOD3(RenderRenderNode_ShadowPass,
        void(IShadowCaster* pRNode, const SRenderingPassInfo& passInfo, AZ::LegacyJobExecutor* pJobExecutor));
    MOCK_METHOD0(GetOpticsManager,
        IOpticsManager*());
    MOCK_METHOD0(SyncProcessStreamingUpdate,
        void());
    MOCK_METHOD1(SetScreenshotCallback,
        void(IScreenshotCallback* pCallback));
    MOCK_METHOD8(ActivateObjectsLayer,
        void(uint16 nLayerId, bool bActivate, bool bPhys, bool bObjects, bool bStaticLights, const char* pLayerName, IGeneralMemoryHeap* pHeap, bool bCheckLayerActivation));
    MOCK_CONST_METHOD4(GetLayerMemoryUsage,
        void(uint16 nLayerId, ICrySizer* pSizer, int* pNumBrushes, int* pNumDecals));
    MOCK_METHOD2(SkipLayerLoading,
        void(uint16 nLayerId, bool bClearList));
    MOCK_METHOD2(PrecacheRenderNode,
        void(IRenderNode* pObj, float fEntDistanceReal));
    MOCK_METHOD0(GetDeferredPhysicsEventManager,
        IDeferredPhysicsEventManager*());
    MOCK_METHOD1(SetStreamableListener,
        void(IStreamedObjectListener* pListener));
    MOCK_METHOD1(GetRenderingPassCamera,
        CCamera*(const CCamera& rCamera));
    MOCK_METHOD3(GetSvoStaticTextures,
        void(I3DEngine::SSvoStaticTexInfo& svoInfo, PodArray<I3DEngine::SLightTI>* pLightsTI_S, PodArray<I3DEngine::SLightTI>* pLightsTI_D));
    MOCK_METHOD2(GetSvoBricksForUpdate,
        void(PodArray<SSvoNodeInfo>& arrNodeInfo, bool getDynamic));
#if defined(USE_GEOM_CACHES)
    MOCK_METHOD1(LoadGeomCache,
        IGeomCache*(const char* szFileName));
    MOCK_METHOD1(FindGeomCacheByFilename,
        IGeomCache*(const char* szFileName));
#endif
    MOCK_METHOD3(LoadDesignerObject,
        IStatObj*(int nVersion, const char* szBinaryStream, int size));

    MOCK_METHOD0(WaitForCullingJobsCompletion,
        void());
};

