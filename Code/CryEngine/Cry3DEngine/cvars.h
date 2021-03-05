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

#if defined(CONSOLE_CONST_CVAR_MODE)
# define GetFloatCVar(name) name ## Default
#else
# define GetFloatCVar(name) (Cry3DEngineBase::GetCVars())->name
#endif

// console variables
struct CVars
    : public Cry3DEngineBase
{
    CVars()
    { Init(); }

    void Init();

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
    //default values used for const cvars
#ifdef _RELEASE
    enum
    {
        e_StatObjValidateDefault = 0
    };                                      // Validate meshes in all but release builds.
#else
    enum
    {
        e_StatObjValidateDefault = 1
    };                                      // Validate meshes in all but release builds.
#endif
#ifdef CONSOLE_CONST_CVAR_MODE
    enum
    {
        e_DisplayMemoryUsageIconDefault = 0
    };
#else
    enum
    {
        e_DisplayMemoryUsageIconDefault = 1
    };
#endif
#define e_PhysOceanCellDefault (0.f)
#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(cvars_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    enum
    {
        e_DeformableObjectsDefault = 1
    };
    enum
    {
        e_OcclusionVolumesDefault = 1
    };
    enum
    {
        e_WaterOceanDefault = 1
    };
    enum
    {
        e_WaterVolumesDefault = 1
    };
    enum
    {
        e_LightVolumesDefault = 1
    };
#endif


#define e_RenderTransparentUnderWaterDefault (0)
#define e_DecalsDefferedDynamicMinSizeDefault (0.35f)
#define e_DecalsPlacementTestAreaSizeDefault (0.08f)
#define e_DecalsPlacementTestMinDepthDefault (0.05f)
#define e_StreamPredictionDistanceFarDefault (16.f)
#define e_StreamPredictionDistanceNearDefault (0.f)
#define e_StreamCgfVisObjPriorityDefault (0.5f)
#define e_WindBendingDistRatioDefault (0.5f)
#define e_MaxViewDistFullDistCamHeightDefault (1000.f)
#define e_CoverageBufferOccludersLodRatioDefault (0.25f)
#define e_LodCompMaxSizeDefault (6.f)
#define e_LodBoundingBoxDistanceMultiplierDefault (0.1f)
#define e_MaxViewDistanceDefault (-1.f)
#define e_ViewDistCompMaxSizeDefault (64.f)
#define e_ViewDistRatioPortalsDefault (60.f)
#define e_WindDefault (0.1f)
#define e_ShadowsCastViewDistRatioLightsDefault (1.f)
#define e_DecalsRangeDefault (20.f)
#define e_GsmRangeStepExtendedDefault (8.f)
#define e_SunAngleSnapSecDefault (0.1f)
#define e_SunAngleSnapDotDefault (0.999999f)
#define e_OcclusionVolumesViewDistRatioDefault (0.05f)
#define e_JointStrengthScaleDefault (1.f)
#define e_VolObjShadowStrengthDefault (.4f)
#define e_CameraRotationSpeedDefault (0.f)
#define e_DecalsDefferedDynamicDepthScaleDefault (4.0f)
#define e_StreamCgfFastUpdateMaxDistanceDefault (16.f)
#define e_StreamPredictionMinFarZoneDistanceDefault (16.f)
#define e_StreamPredictionMinReportDistanceDefault (0.75f)
#define e_StreamCgfGridUpdateDistanceDefault (0.f)
#define e_StreamPredictionAheadDefault (0.5f)
#define e_StreamPredictionAheadDebugDefault (0.f)
#define e_DissolveDistMaxDefault (8.0f)
#define e_DissolveDistMinDefault (2.0f)
#define e_DissolveDistbandDefault (3.0f)
#define e_RenderMeshCollisionToleranceDefault (0.3f)

#if _RELEASE
#define e_RenderDefault (1)
#else
#define e_RenderDefault (gEnv->IsDedicated() ? 0 : 1)
#endif

    int e_Decals;
    int e_DecalsAllowGameDecals;
    int e_CoverageBufferVersion;
    DeclareConstFloatCVar(e_FoliageBrokenBranchesDamping);
    float e_ShadowsCastViewDistRatio;
    float e_OnDemandMaxSize;
    float e_MaxViewDistSpecLerp;
    float e_StreamAutoMipFactorSpeedThreshold;
    DeclareConstFloatCVar(e_DecalsDefferedDynamicMinSize);
    DeclareConstIntCVar(e_Objects, 1);
    float e_ViewDistRatioCustom;
    float e_StreamPredictionUpdateTimeSlice;
    DeclareConstIntCVar(e_DisplayMemoryUsageIcon, e_DisplayMemoryUsageIconDefault);
    int e_ScreenShotWidth;
    DeclareConstIntCVar(e_CoverageBufferTolerance, 0);
    int e_ScreenShotDebug;
#if defined(WIN32)
    int e_ShadowsLodBiasFixed;
#else
    DeclareConstIntCVar(e_ShadowsLodBiasFixed, 0);
#endif
    DeclareConstIntCVar(e_FogVolumes, 1);
    int e_VolumetricFog;
    DeclareConstIntCVar(e_FogVolumesTiledInjection, 1);
    DeclareConstIntCVar(e_Render, e_RenderDefault);
    int e_Tessellation;
    float e_TessellationMaxDistance;
    DeclareConstIntCVar(e_ShadowsTessellateCascades, 1);
    DeclareConstIntCVar(e_ShadowsTessellateDLights, 0);
    int e_CoverageBufferReproj;
    int e_CoverageBufferRastPolyLimit;
    int e_CoverageBufferShowOccluder;
    int e_CoverageBufferNumberFramesLatency;
    DeclareConstFloatCVar(e_ViewDistRatioPortals);
    DeclareConstFloatCVar(e_CoverageBufferOccludersLodRatio);
    DeclareConstIntCVar(e_ObjFastRegister, 1);
    float e_ViewDistRatioLights;
    DeclareConstIntCVar(e_DebugDraw, 0);
    int e_DebugDrawLodMinTriangles; // cvar for min number of triangles in object before displaying lod warnings
    ICVar* e_DebugDrawFilter;
    DeclareConstIntCVar(e_DebugDrawListSize, 16);
    DeclareConstIntCVar(e_DebugDrawListBBoxIndex, 0);
#if !defined(_RELEASE)
    const char* e_pStatObjRenderFilterStr;
    int e_statObjRenderFilterMode;
#endif
    DeclareConstIntCVar(e_AutoPrecacheTexturesAndShaders, 0);
    int e_StreamPredictionMaxVisAreaRecursion;
    float e_StreamPredictionBoxRadius;
    int e_Clouds;
    int e_DecalsMaxTrisInObject;
    DeclareConstFloatCVar(e_OcclusionVolumesViewDistRatio);
    DeclareConstFloatCVar(e_SunAngleSnapDot);
    DeclareConstIntCVar(e_PreloadDecals, 1);
    int e_WorldSegmentationTest;
    float e_DecalsLifeTimeScale;
    int e_DecalsForceDeferred;
    DeclareConstIntCVar(e_CoverageBufferDebugFreeze, 0);
    int e_PhysProxyTriLimit;
    float e_FoliageWindActivationDist;
    ICVar* e_SQTestTextureName;
    int e_ShadowsClouds;
    int e_levelStartupFrameDelay;
    float e_SkyUpdateRate;
    float e_RecursionViewDistRatio;
    DeclareConstIntCVar(e_StreamCgfDebugMinObjSize, 100);
    int e_CullVegActivation;
    int e_StreamPredictionTexelDensity;
    int e_StreamPredictionAlwaysIncludeOutside;
    DeclareConstIntCVar(e_DynamicLights, 1);
    int e_DynamicLightsFrameIdVisTest;
    DeclareConstIntCVar(e_ShadowsLodBiasInvis, 0);
    float e_CoverageBufferBias;
    int e_DynamicLightsMaxEntityLights;
    int e_SQTestMoveSpeed;
    float e_StreamAutoMipFactorMax;
    int e_CoverageBufferAccurateOBBTest;
    int e_ObjQuality;
    int e_LightQuality;
    int e_RNTmpDataPoolMaxFrames;
    DeclareConstIntCVar(e_DynamicLightsMaxCount, 512);
    int e_StreamCgfPoolSize;
    DeclareConstIntCVar(e_StatObjPreload, 1);
    DeclareConstIntCVar(e_ShadowsDebug, 0);
    DeclareConstIntCVar(e_ShadowsCascadesDebug, 0);
    DeclareConstFloatCVar(e_StreamPredictionDistanceNear);
    float e_CoverageBufferDebugDrawScale;
    DeclareConstIntCVar(e_GsmStats, 0);
    DeclareConstIntCVar(e_DynamicLightsForceDeferred, 1);
    DeclareConstIntCVar(e_Fog, 1);
    float e_TimeOfDay;
    DeclareConstIntCVar(e_SkyBox, 1);
    float e_CoverageBufferAABBExpand;
    int e_CoverageBufferEarlyOut;
    float e_CoverageBufferEarlyOutDelay;
    int e_Dissolve;
    int e_StatObjBufferRenderTasks;
    DeclareConstIntCVar(e_StreamCgfUpdatePerNodeDistance, 1);
    DeclareConstFloatCVar(e_DecalsDefferedDynamicDepthScale);
    DeclareConstIntCVar(e_LightVolumes, e_LightVolumesDefault);
    DeclareConstIntCVar(e_LightVolumesDebug, 0);
    DeclareConstIntCVar(e_Portals, 1);
    DeclareConstIntCVar(e_PortalsBlend, 1);
    int e_PortalsMaxRecursion;
    float e_StreamAutoMipFactorMaxDVD;
    DeclareConstIntCVar(e_CameraFreeze, 0);
    DeclareConstFloatCVar(e_StreamPredictionAhead);
    DeclareConstFloatCVar(e_FoliageBranchesStiffness);
    DeclareConstFloatCVar(e_StreamPredictionMinFarZoneDistance);
    int e_StreamCgf;
    int e_CheckOcclusion;
    int e_CheckOcclusionQueueSize;
    int e_CheckOcclusionOutputQueueSize;
    DeclareConstIntCVar(e_WaterVolumes, e_WaterVolumesDefault);
    DeclareConstIntCVar(e_RenderTransparentUnderWater, e_RenderTransparentUnderWaterDefault);
    float e_ScreenShotMapCamHeight;
    DeclareConstIntCVar(e_CoverageBufferOccludersTestMinTrisNum, 0);
    DeclareConstIntCVar(e_DeformableObjects, e_DeformableObjectsDefault);

    DeclareConstFloatCVar(e_StreamCgfFastUpdateMaxDistance);
    DeclareConstIntCVar(e_DecalsClip, 1);
    ICVar* e_ScreenShotFileFormat;
    ICVar* e_ScreenShotFileName;
    int e_CharLodMin;
    float e_PhysOceanCell;
    DeclareConstIntCVar(e_WindAreas, 1);
    DeclareConstFloatCVar(e_WindBendingDistRatio);
    float e_SQTestDelay;
    int e_PhysMinCellSize;
    DeclareConstIntCVar(e_PhysEntityGridSizeDefault, 4096);
    int e_StreamCgfMaxTasksInProgress;
    int e_StreamCgfMaxNewTasksPerUpdate;
    int e_CoverageBufferResolution;
    DeclareConstFloatCVar(e_DecalsPlacementTestAreaSize);
    DeclareConstFloatCVar(e_DecalsPlacementTestMinDepth);
    DeclareConstFloatCVar(e_CameraRotationSpeed);
    float e_ScreenShotMapSizeY;
    int e_GI;
    DeclareConstIntCVar(e_CoverageBufferLightsDebugSide, -1);
    DeclareConstIntCVar(e_PortalsBigEntitiesFix, 1);
    int e_SQTestBegin;
    ICVar* e_CameraGoto;

    DeclareConstFloatCVar(e_StreamPredictionMinReportDistance);
    int e_WaterTessellationSwathWidth;
    DeclareConstIntCVar(e_RecursionOcclusionCulling, 0);
    DeclareConstIntCVar(e_StreamSaveStartupResultsIntoXML, 0);
    DeclareConstIntCVar(e_PhysFoliage, 2);
    DeclareConstIntCVar(e_RenderMeshUpdateAsync, 1);
    DeclareConstIntCVar(e_CoverageBufferTreeDebug, 0);
    float e_CoverageBufferOccludersViewDistRatio;
    int e_DecalsDefferedDynamic;
    DeclareConstIntCVar(e_DefaultMaterial, 0);
    int e_ShadowsOcclusionCulling;
    int e_LodMin;
    DeclareConstIntCVar(e_PreloadMaterials, 1);
    DeclareConstIntCVar(e_ObjStats, 0);
    DeclareConstIntCVar(e_ShadowsFrustums, 0);
    DeclareConstIntCVar(e_OcclusionVolumes, e_OcclusionVolumesDefault);
    int e_DecalsDefferedStatic;
    DeclareConstIntCVar(e_Roads, 1);
    DeclareConstIntCVar(e_DebugDrawShowOnlyCompound, 0);
    DeclareConstIntCVar(e_StatObjMergeUseThread, 1);
    DeclareConstFloatCVar(e_SunAngleSnapSec);
    float e_GsmRangeStep;
    float e_LodRatio;
    float e_LodFaceAreaTargetSize;
    DeclareConstIntCVar(e_CoverageBufferDrawOccluders, 0);
    DeclareConstIntCVar(e_ObjectsTreeBBoxes, 0);
    DeclareConstIntCVar(e_PrepareDeformableObjectsAtLoadTime, 0);
    DeclareConstIntCVar(e_3dEngineTempPoolSize, 1024);
    DeclareConstFloatCVar(e_MaxViewDistFullDistCamHeight);
    DeclareConstIntCVar(e_VegetationBending, 1);
    DeclareConstFloatCVar(e_StreamPredictionAheadDebug);
    float e_ShadowsSlopeBias;
    float e_ShadowsSlopeBiasHQ;
    DeclareConstIntCVar(e_GsmDepthBoundsDebug, 0);
    DeclareConstIntCVar(e_TimeOfDayDebug, 0);
    int e_WaterTessellationAmount;  // being deprecated by Water gem
    int e_Entities;
    int e_CoverageBuffer;
    int e_FogVolumeShadingQuality;
    int e_ScreenShotQuality;
    int e_levelStartupFrameNum;
    DeclareConstIntCVar(e_DecalsPreCreate, 1);
    int e_SQTestCount;
    float e_GsmRange;
    int e_ScreenShotMapOrientation;
    int e_ScreenShotHeight;
    int e_WaterOceanFFT;
    DeclareConstFloatCVar(e_MaxViewDistance);
    DeclareConstIntCVar(e_AutoPrecacheCameraJumpDist, 16);
    DeclareConstIntCVar(e_LodsForceUse, 1);
    DeclareConstIntCVar(e_ForceDetailLevelForScreenRes, 0);
    DeclareConstIntCVar(e_3dEngineLogAlways, 0);
    DeclareConstIntCVar(e_DecalsHitCache, 1);
    DeclareConstIntCVar(e_BBoxes, 0);
    float e_TimeOfDaySpeed;
    int e_LodMax;
    int e_LodForceUpdate;
    DeclareConstFloatCVar(e_ViewDistCompMaxSize);
    float e_ShadowsAdaptScale;
    float e_ScreenShotMapSizeX;
    float e_OcclusionCullingViewDistRatio;
    DeclareConstIntCVar(e_WaterOceanBottom, 1);
    DeclareConstIntCVar(e_WaterRipplesDebug, 0);
    int e_OnDemandPhysics;
    float e_ShadowsResScale;
    DeclareConstIntCVar(e_Recursion, 1);
    DeclareConstIntCVar(e_CoverageBufferMaxAddRenderMeshTime, 2);
    int e_CoverageBufferRotationSafeCheck;
    DeclareConstIntCVar(e_StatObjTestOBB, 0);
    DeclareConstIntCVar(e_StatObjValidate, e_StatObjValidateDefault);
    DeclareConstIntCVar(e_DecalsMaxValidFrames, 600);
    DeclareConstIntCVar(e_DecalsMerge, 0);
    int e_SQTestDistance;
    float e_ViewDistMin;
    float e_StreamAutoMipFactorMin;
    DeclareConstIntCVar(e_LodMinTtris, 300);
    DeclareConstIntCVar(e_SkyQuality, 1);
    DeclareConstIntCVar(e_ScissorDebug, 0);
    DeclareConstIntCVar(e_StatObjMergeMaxTrisPerDrawCall, 500);
    DeclareConstIntCVar(e_DynamicLightsConsistentSortOrder, 1);
    DeclareConstIntCVar(e_StreamCgfDebug, 0);
    float e_TerrainOcclusionCullingMaxDist;
    float e_StatObjTessellationMaxEdgeLenght;
    int e_StatObjTessellationMode;
    DeclareConstIntCVar(e_OcclusionLazyHideFrames, 0);
    float e_RenderMeshCollisionTolerance;
    DeclareConstIntCVar(e_ShadowsMasksLimit, 0);
    int e_ShadowsCache;
    int e_ShadowsCacheUpdate;
    int e_ShadowsCacheObjectLod;
    int e_ShadowsCacheRenderCharacters;
    int e_ShadowsCacheRequireManualUpdate;
    int e_ShadowsPerObject;
    int e_DynamicDistanceShadows;
    float e_ShadowsPerObjectResolutionScale;
    int e_ObjShadowCastSpec;
    DeclareConstFloatCVar(e_JointStrengthScale);
    int e_AutoPrecacheCgfMaxTasks;
    float e_DecalsNeighborMaxLifeTime;
    DeclareConstFloatCVar(e_StreamCgfVisObjPriority);
    int e_ObjectLayersActivation;
    DeclareConstFloatCVar(e_DissolveDistMax);
    DeclareConstFloatCVar(e_DissolveDistMin);
    DeclareConstFloatCVar(e_DissolveDistband);
    int e_ScreenShotMinSlices;
    int e_DecalsMaxUpdatesPerFrame;
    DeclareConstIntCVar(e_SkyType, 1);
    int e_GsmLodsNum;
    DeclareConstIntCVar(e_AutoPrecacheCgf, 1);
    DeclareConstIntCVar(e_HwOcclusionCullingWater, 1);
    DeclareConstIntCVar(e_CoverageBufferTestMode, 2);
    DeclareConstIntCVar(e_DeferredPhysicsEvents, 1);
    DeclareConstFloatCVar(e_ShadowsCastViewDistRatioLights);
    int e_ShadowsUpdateViewDistRatio;
    DeclareConstIntCVar(e_Lods, 1);
    DeclareConstIntCVar(e_LodFaceArea, 1);
    DeclareConstFloatCVar(e_LodBoundingBoxDistanceMultiplier);
    float e_ShadowsConstBias;
    float e_ShadowsConstBiasHQ;
    int e_ShadowsClearShowMaskAtLoad;
    DeclareConstIntCVar(e_Ropes, 1);
    int e_ShadowsPoolSize;
    int e_ShadowsMaxTexRes;
    int e_Sun;
    DeclareConstFloatCVar(e_DecalsRange);
    float e_ScreenShotMapCenterY;
    int e_CacheNearestCubePicking;
    DeclareConstIntCVar(e_CoverCgfDebug, 0);
    DeclareConstFloatCVar(e_StreamCgfGridUpdateDistance);
    DeclareConstFloatCVar(e_LodCompMaxSize);
    float e_ViewDistRatioDetail;
    DeclareConstIntCVar(e_Sleep, 0);
    DeclareConstIntCVar(e_Wind, 1);
    int e_SQTestMip;
    int e_Shadows;
    int e_ShadowsBlendCascades;
    float e_ShadowsBlendCascadesVal;
    DeclareConstIntCVar(e_DebugDrawShowOnlyLod, -1);
    int e_ScreenShot;
    DeclareConstIntCVar(e_PrecacheLevel, 0);
    float e_ScreenShotMapCenterX;
    DeclareConstIntCVar(e_CoverageBufferDebug, 0);
    DeclareConstIntCVar(e_StatObjMerge, 1);
    DeclareConstIntCVar(e_StatObjStoreMesh, 0);
    ICVar* e_StreamCgfDebugFilter;
    int e_ShadowsOnAlphaBlend;
    DeclareConstFloatCVar(e_VolObjShadowStrength);
    DeclareConstIntCVar(e_WaterOcean, e_WaterOceanDefault);
    float e_ViewDistRatio;
    DeclareConstIntCVar(e_ObjectLayersActivationPhysics, 1);
    DeclareConstIntCVar(e_StreamCgfDebugHeatMap, 0);
    DeclareConstFloatCVar(e_StreamPredictionDistanceFar);
    int e_SQTestExitOnFinish;
    int e_DecalsOverlapping;
    int e_CGFMaxFileSize;
    int e_MaxDrawCalls;
#if !defined(_RELEASE)
#endif

    int e_DebugGeomPrep;

    int e_CheckOctreeObjectsBoxSize;
    DeclareConstIntCVar(e_GeomCaches, 1);
    int e_GeomCacheBufferSize;
    int e_GeomCacheMaxPlaybackFromMemorySize;
    int e_GeomCachePreferredDiskRequestSize;
    float e_GeomCacheMinBufferAheadTime;
    float e_GeomCacheMaxBufferAheadTime;
    float e_GeomCacheDecodeAheadTime;
    DeclareConstIntCVar(e_GeomCacheDebug, 0);
    ICVar* e_GeomCacheDebugFilter;
    DeclareConstIntCVar(e_GeomCacheDebugDrawMode, 0);
    DeclareConstIntCVar(e_GeomCacheLerpBetweenFrames, 1);

    int e_PermanentRenderObjects;
    int e_StaticInstancing;
    int e_StaticInstancingMinInstNum;

    DeclareConstIntCVar(e_MemoryProfiling, 0);


};
