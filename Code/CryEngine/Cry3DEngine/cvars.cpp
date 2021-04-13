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

// Description : console variables used in 3dengine

#include "Cry3DEngine_precompiled.h"

#include "3dEngine.h"
#include "IRenderer.h"
#include "IStatObj.h"  // MAX_STATOBJ_LODS_NUM
#include "ITimeOfDay.h"

#include "Environment/OceanEnvironmentBus.h"

//////////////////////////////////////////////////////////////////////////
void OnTimeOfDayVarChange([[maybe_unused]] ICVar* pArgs)
{
    gEnv->p3DEngine->GetTimeOfDay()->SetTime(Cry3DEngineBase::GetCVars()->e_TimeOfDay);
}

void OnTimeOfDaySpeedVarChange([[maybe_unused]] ICVar* pArgs)
{
    ITimeOfDay::SAdvancedInfo advInfo;
    gEnv->p3DEngine->GetTimeOfDay()->GetAdvancedInfo(advInfo);
    advInfo.fAnimSpeed = Cry3DEngineBase::GetCVars()->e_TimeOfDaySpeed;
    gEnv->p3DEngine->GetTimeOfDay()->SetAdvancedInfo(advInfo);
}

void OnCGFStreamingChange([[maybe_unused]] ICVar* pArgs)
{
    if (gEnv->IsEditor())
    {
        Cry3DEngineBase::GetCVars()->e_StreamCgf = 0;
    }
}

void OnGsmLodsNumChange(ICVar* pArgs)
{
    Cry3DEngineBase::GetRenderer()->UpdateCachedShadowsLodCount(pArgs->GetIVal());
}

void OnDynamicDistanceShadowsVarChange([[maybe_unused]] ICVar* pArgs)
{
    Cry3DEngineBase::Get3DEngine()->SetRecomputeCachedShadows(ShadowMapFrustum::ShadowCacheData::eFullUpdate);
}

void OnVolumetricFogChanged(ICVar* pArgs)
{
    const ICVar* deferredShadingCVar = gEnv->pConsole->GetCVar("r_DeferredShadingTiled");
    if (deferredShadingCVar->GetIVal() == 0 && pArgs->GetIVal() != 0)
    {
        gEnv->pLog->LogWarning("e_VolumetricFog is set to 0 when r_DeferredShadingTiled is 0.");
        Cry3DEngineBase::GetCVars()->e_VolumetricFog = 0;
    }
}

#if !defined(_RELEASE)
void OnTerrainPerformanceSecondsChanged([[maybe_unused]] ICVar* pArgs)
{
#ifdef LY_TERRAIN_LEGACY_RUNTIME
    AZ::Debug::TerrainProfiler::RefreshFrameProfilerStatus();
#endif
}
#endif

void OnDebugDrawChange(ICVar* pArgs)
{
    static bool collectingDrawCalls = false;

    int e_debugDraw = pArgs->GetIVal();
    if (e_debugDraw >= 24 && e_debugDraw <= 25)
    {
        gEnv->pRenderer->CollectDrawCallsInfo(true);
        gEnv->pRenderer->CollectDrawCallsInfoPerNode(true);
        collectingDrawCalls = true;
    }
    else if (collectingDrawCalls)
    {
        gEnv->pRenderer->CollectDrawCallsInfo(false);
        gEnv->pRenderer->CollectDrawCallsInfoPerNode(false);
        collectingDrawCalls = false;
    }
}

void CVars::Init()
{
    DefineConstIntCVar(e_Fog, 1, VF_CHEAT | VF_CHEAT_ALWAYS_CHECK,
        "Activates global height/distance based fog");
    DefineConstIntCVar(e_FogVolumes, 1, VF_CHEAT | VF_CHEAT_ALWAYS_CHECK,
        "Activates local height/distance based fog volumes");
    REGISTER_CVAR_CB(e_VolumetricFog, 0, VF_NULL,
        "Activates volumetric fog", OnVolumetricFogChanged);
    DefineConstIntCVar(e_FogVolumesTiledInjection, 1, VF_NULL,
        "Activates tiled FogVolume density injection");
    REGISTER_CVAR(e_Entities, 1, VF_CHEAT | VF_CHEAT_ALWAYS_CHECK,
        "Activates drawing of entities");
    DefineConstIntCVar(e_SkyBox, 1, VF_CHEAT,
        "Activates drawing of skybox and moving cloud layers");
    DefineConstIntCVar(e_WaterOcean, e_WaterOceanDefault, VF_CHEAT | VF_CHEAT_ALWAYS_CHECK,
        "Activates drawing of ocean. \n"
        "1: use usual rendering path\n"
        "2: use fast rendering path with merged fog");

    if (!OceanToggle::IsActive())
    {
        DefineConstIntCVar(e_WaterOceanBottom, 1, VF_CHEAT,
            "Activates drawing bottom of ocean");
    }

    REGISTER_CVAR(e_WaterOceanFFT, 0, VF_NULL,
        "Activates fft based ocean");

    DefineConstIntCVar(e_WaterRipplesDebug, 0, VF_CHEAT,
        "Draw water hits that affect water ripple simulation");

    DefineConstIntCVar(e_DebugDrawShowOnlyCompound, 0, VF_NULL,
        "e_DebugDraw shows only Compound (less efficient) static meshes");
    DefineConstIntCVar(e_DebugDrawShowOnlyLod, -1, VF_NULL,
        "e_DebugDraw shows only objects showing lod X");

#ifdef CONSOLE_CONST_CVAR_MODE
    // in console release builds the const cvars work differently (see ISystem.h where CONSOLE_CONST_CVAR_MODE is defined), 
    // so revert to the version of e_debugdraw that doesn't support mode 24 & 25 which require the OnDebugDrawChange callback
    DefineConstIntCVar(e_DebugDraw, 0, VF_CHEAT | VF_CHEAT_ALWAYS_CHECK,
        "Draw helpers with information for each object (same number negative hides the text)\n"
        " 1: Name of the used cgf, polycount, used LOD\n"
        " 2: Color coded polygon count\n"
        " 3: Show color coded LODs count, flashing color indicates no Lod\n"
        " 4: Display object texture memory usage\n"
        " 5: Display color coded number of render materials\n"
        " 6: Display ambient color\n"
        " 7: Display tri count, number of render materials, texture memory\n"
        " 8: Free slot\n"
        " 9: Free slot\n"
        "10: Render geometry with simple lines and triangles\n"
        "11: Free slot\n"
        "12: Free slot\n"
        "13: Display occlusion amount (used during AO computations). Warning: can take a long time to calculate, depending on level size! \n"
        "15: Display helpers\n"
        "16: Display debug gun\n"
        "17: Streaming info (buffer sizes)\n"
        "18: Free slot\n"
        "19: Physics proxy triangle count\n"
        "20: Display Character attachments texture memory usage\n"
        "21: Display animated object distance to camera\n"
        "22: Display object's current LOD vertex count\n"
        "23: Display shadow casters in red\n"
        "24: Disabled\n"
        "25: Disabled\n"
        "----------------debug draw list values. Any of them enable 2d on-screen listing type info debug. Specific values define the list sorting-----------\n"
        " 100: tri count\n"
        " 101: verts count\n"
        " 102: draw calls\n"
        " 103: texture memory\n"
        " 104: mesh memory"
        );
#else
    REGISTER_CVAR_CB(e_DebugDraw, 0, VF_CHEAT | VF_CHEAT_ALWAYS_CHECK | CONST_CVAR_FLAGS,
        "Draw helpers with information for each object (same number negative hides the text)\n"
        " 1: Name of the used cgf, polycount, used LOD\n"
        " 2: Color coded polygon count\n"
        " 3: Show color coded LODs count, flashing color indicates no Lod\n"
        " 4: Display object texture memory usage\n"
        " 5: Display color coded number of render materials\n"
        " 6: Display ambient color\n"
        " 7: Display tri count, number of render materials, texture memory\n"
        " 8: Free slot\n"
        " 9: Free slot\n"
        "10: Render geometry with simple lines and triangles\n"
        "11: Free slot\n"
        "12: Free slot\n"
        "13: Display occlusion amount (used during AO computations). Warning: can take a long time to calculate, depending on level size! \n"
        "15: Display helpers\n"
        "16: Display debug gun\n"
        "17: Streaming info (buffer sizes)\n"
        "18: Free slot\n"
        "19: Physics proxy triangle count\n"
        "20: Display Character attachments texture memory usage\n"
        "21: Display animated object distance to camera\n"
        "22: Display object's current LOD vertex count\n"
        "23: Display shadow casters in red\n"
        "24: Display meshes with no LODs\n"
        "25: Display meshes with no LODs, meshes with not enough LODs\n"
        "----------------debug draw list values. Any of them enable 2d on-screen listing type info debug. Specific values define the list sorting-----------\n"
        " 100: tri count\n"
        " 101: verts count\n"
        " 102: draw calls\n"
        " 103: texture memory\n"
        " 104: mesh memory",
        OnDebugDrawChange);
#endif

    REGISTER_CVAR(e_DebugDrawLodMinTriangles, 200, VF_CHEAT | VF_CHEAT_ALWAYS_CHECK | CONST_CVAR_FLAGS,
        "Minimum number of triangles (lod 0) to show in LOD debug draw");

#ifndef _RELEASE
    DefineConstIntCVar(e_DebugDrawListSize, 24, VF_DEV_ONLY,    "num objects in the list for e_DebugDraw list infodebug");
    REGISTER_STRING_CB_DEV_ONLY("e_DebugDrawListFilter", "",  VF_NULL,
            "filter for e_DebugDraw list. Combine object type letters to create the filter\n"
            "(example: e_DebugDrawListFilter BVC = shows Characters+StatObject). 'all' = no filter.\n"
            " C: Character\n"
            " S: StatObj (non characters)\n", NULL);
    DefineConstIntCVar(e_DebugDrawListBBoxIndex, 0, VF_DEV_ONLY,
        "enables BBOX drawing for the 'n' element of the DebugDrawList (starting by 1.   0 = no bbox drawing).");
    REGISTER_COMMAND("e_DebugDrawListCMD", CDebugDrawListMgr::ConsoleCommand, VF_DEV_ONLY,
        "Issue commands to control e_DebugDraw list debuginfo behaviour\n"
        "'Freeze' (F) - stops refreshing stats\n"
        "'Continue' (C) - unfreezes\n"
        "'DumpLog' (D) - dumps the current on-screen info into the log");
#endif

#if !defined(_RELEASE)
    e_pStatObjRenderFilterStr = NULL;
    REGISTER_CVAR2("e_StatObjRenderFilter", &e_pStatObjRenderFilterStr, "", VF_NULL, "Debug: Controls which cgfs are rendered, based on input string");
    e_statObjRenderFilterMode = 0;
    REGISTER_CVAR2("e_StatObjRenderFilterMode", &e_statObjRenderFilterMode, 0, VF_NULL, "Debug: Controls how e_StatObjRenderFilter is use. 0=disabled 1=include 2=exclude");
#endif

    DefineConstFloatCVar(e_SunAngleSnapSec, VF_NULL,
        "Sun dir snap control");
    DefineConstFloatCVar(e_SunAngleSnapDot, VF_NULL,
        "Sun dir snap control");

    DefineConstIntCVar(e_Roads, 1, VF_CHEAT | VF_CHEAT_ALWAYS_CHECK,
        "Activates drawing of road objects");

    REGISTER_CVAR(e_Decals, 1, VF_NULL | VF_CHEAT_ALWAYS_CHECK,
        "Activates drawing of decals (game decals and hand-placed)");
    REGISTER_CVAR(e_DecalsForceDeferred, 0, VF_NULL,
        "1 - force to convert all decals to use deferred ones");
    REGISTER_CVAR(e_DecalsDefferedStatic, 1, VF_NULL,
        "1 - switch all non-planar decals placed by level designer to deferred");
    REGISTER_CVAR(e_DecalsDefferedDynamic, 1, VF_NULL,
        "1 - make all game play decals deferred, 2 - make all game play decals non deferred");
    DefineConstFloatCVar(e_DecalsDefferedDynamicMinSize, VF_CHEAT,
        "Convert only dynamic decals bigger than X into deferred");
    DefineConstFloatCVar(e_DecalsDefferedDynamicDepthScale, VF_CHEAT,
        "Scale decal projection depth");
    DefineConstFloatCVar(e_DecalsPlacementTestAreaSize, VF_CHEAT,
        "Avoid spawning decals on the corners or edges of entity geometry");
    DefineConstFloatCVar(e_DecalsPlacementTestMinDepth, VF_CHEAT,
        "Avoid spawning decals on the corners or edges of entity geometry");
    REGISTER_CVAR(e_DecalsMaxTrisInObject, 8000, VF_NULL,
        "Do not create decals on objects having more than X triangles");
    REGISTER_CVAR(e_DecalsAllowGameDecals,    1, VF_NULL,
        "Allows creation of decals by game (like weapon bullets marks)");
    DefineConstIntCVar(e_DecalsHitCache, 1, VF_CHEAT,
        "Use smart hit caching for bullet hits (may cause no decals in some cases)");
    DefineConstIntCVar(e_DecalsMerge, 0, VF_NULL,
        "Combine pieces of decals into one render call");
    DefineConstIntCVar(e_DecalsPreCreate, 1, VF_NULL,
        "Pre-create decals at load time");
    DefineConstIntCVar(e_DecalsClip, 1, VF_NULL,
        "Clip decal geometry by decal bbox");
    DefineConstFloatCVar(e_DecalsRange, VF_NULL,
        "Less precision for decals outside this range");
    REGISTER_CVAR(e_DecalsLifeTimeScale, 1.f, VF_NULL,
        "Allows to increase or reduce decals life time for different specs");
    REGISTER_CVAR(e_DecalsNeighborMaxLifeTime, 4.f, VF_NULL,
        "If not zero - new decals will force old decals to fade in X seconds");
    REGISTER_CVAR(e_DecalsOverlapping, 0, VF_NULL,
        "If zero - new decals will not be spawned if the distance to nearest decals less than X");
    DefineConstIntCVar(e_DecalsMaxValidFrames, 600, VF_NULL,
        "Number of frames after which not visible static decals are removed");
    REGISTER_CVAR(e_DecalsMaxUpdatesPerFrame, 4, VF_NULL,
        "Maximum number of static decal render mesh updates per frame");
    DefineConstIntCVar(e_VegetationBending, 1, VF_NULL,
        "Enable vegetation bending (does not affect merged grass)");

    DefineConstIntCVar(e_ForceDetailLevelForScreenRes, 0, VF_DEPRECATED,
        "[DEPRECATED] Force sprite distance and other values used for some specific screen resolution, 0 means current");

    DefineConstIntCVar(e_Wind, 1, VF_CHEAT,
        "Enable global wind calculations, affects vegetations bending animations");
    DefineConstIntCVar(e_WindAreas, 1, VF_CHEAT,
        "Debug");
    DefineConstFloatCVar(e_WindBendingDistRatio, VF_CHEAT,
        "Wind cutoff distance for bending (linearly attentuated to that distance)");
    REGISTER_CVAR(e_Shadows, 1, VF_NULL,
        "Activates drawing of shadows");
    REGISTER_CVAR(e_ShadowsBlendCascades, 1, VF_NULL,
        "Blend between shadow cascades: 0=off, 1=on");
    REGISTER_CVAR(e_ShadowsBlendCascadesVal, 0.75f, VF_NULL,
        "Size of cascade blend region");
#if defined(WIN32)
    REGISTER_CVAR(e_ShadowsLodBiasFixed, 1, VF_NULL,
        "Simplifies mesh for shadow map generation by X LOD levels");
#else
    DefineConstIntCVar(e_ShadowsLodBiasFixed, 0, VF_NULL,
        "Simplifies mesh for shadow map generation by X LOD levels");
#endif
    DefineConstIntCVar(e_ShadowsLodBiasInvis, 0, VF_NULL,
        "Simplifies mesh for shadow map generation by X LOD levels, if object is not visible in main frame");

    REGISTER_CVAR(e_Tessellation, 1, VF_NULL,
        "HW geometry tessellation  0 = not allowed, 1 = allowed");
    REGISTER_CVAR(e_TessellationMaxDistance, 30.f, VF_NULL,
        "Maximum distance from camera in meters to allow tessellation, also affects distance-based displacement fadeout");
    DefineConstIntCVar(e_ShadowsTessellateCascades, 1, VF_NULL,
        "Maximum cascade number to render tessellated shadows (0 = no tessellation for sun shadows)");
    DefineConstIntCVar(e_ShadowsTessellateDLights, 0, VF_NULL,
        "Disable/enable tessellation for local lights shadows");
    REGISTER_CVAR(e_ShadowsOnAlphaBlend, 0, VF_NULL,
        "Enable shadows on alphablended ");
    DefineConstIntCVar(e_ShadowsFrustums, 0, VF_CHEAT,
        "Debug");
    DefineConstIntCVar(e_ShadowsDebug, 0, VF_CHEAT,
        "0=off, 2=visualize shadow maps on the screen");
    REGISTER_CVAR(e_ShadowsCache, 1, VF_NULL,
        "Activates drawing of static cached shadows");
    REGISTER_CVAR(e_ShadowsCacheUpdate, 0, VF_NULL,
        "Trigger updates of the shadow cache: 0=no update, 1=one update, 2=continuous updates");
    REGISTER_CVAR(e_ShadowsCacheObjectLod, 0, VF_NULL,
        "The lod used for rendering objects into the shadow cache. Set to -1 to disable");
    REGISTER_CVAR_CB(e_ShadowsCacheRenderCharacters, 0, VF_NULL,
        "Render characters into the shadow cache. 0=disabled, 1=enabled", OnDynamicDistanceShadowsVarChange);
    REGISTER_CVAR(e_ShadowsCacheRequireManualUpdate, 0, VF_NULL,
        "Sets whether levels must trigger manual updates of the cached shadow maps:\n"
        "0=Cached shadows default to Incremental updates. Each cached shadow frustum will traverse and cull the octree each frame (Potentially high CPU/GPU overhead)\n"
        "1=Levels must trigger cached shadow updates via script (Preferred: Lowest overhead)\n"
        "2=Levels may either trigger cached shadow updates via script or allow cached shadows to update if the user moves too close to the border of the shadowmap");
    REGISTER_CVAR_CB(e_DynamicDistanceShadows, 1, VF_NULL,
        "Enable dynamic distance shadows, 0=disable, 1=enable, -1=don't render dynamic distance shadows", OnDynamicDistanceShadowsVarChange);
    DefineConstIntCVar(e_ShadowsCascadesDebug, 0, VF_CHEAT,
        "0=off, 1=visualize sun shadow cascades on screen");
    REGISTER_CVAR(e_ShadowsPerObjectResolutionScale, 1, VF_NULL,
        "Global scale for per object shadow texture resolution\n"
        "NOTE: individual texture resolution is rounded to next power of two ");
    REGISTER_CVAR(e_ShadowsClouds, 1, VF_NULL,
        "Cloud shadows");   // no cheat var because this feature shouldn't be strong enough to affect gameplay a lot
    REGISTER_CVAR(e_ShadowsPoolSize, 2048, VF_NULL,
        "Set size of shadow pool (e_ShadowsPoolSize*e_ShadowsPoolSize)");
    REGISTER_CVAR(e_ShadowsMaxTexRes, 1024, VF_NULL,
        "Set maximum resolution of shadow map\n256(faster), 512(medium), 1024(better quality)");
    REGISTER_CVAR(e_ShadowsResScale, 2.8f, VF_NULL,
        "Shadows slope bias for shadowgen");
    REGISTER_CVAR(e_ShadowsAdaptScale, 2.72f, VF_NULL,
        "Shadows slope bias for shadowgen");
    REGISTER_CVAR(e_ShadowsSlopeBias, 1.0f, VF_NULL,
        "Shadows slope bias for shadowgen");
    REGISTER_CVAR(e_ShadowsSlopeBiasHQ, 0.25f, VF_NULL,
        "Shadows slope bias for shadowgen (for high quality mode)");
    REGISTER_CVAR(e_ShadowsConstBias, 1.0f, VF_NULL,
        "Shadows slope bias for shadowgen");
    REGISTER_CVAR(e_ShadowsConstBiasHQ, 0.05f, VF_NULL,
        "Shadows slope bias for shadowgen (high quality mode)");
    REGISTER_CVAR(e_ShadowsClearShowMaskAtLoad, 1, VF_NULL,
                  "Clears the shadow mask at level load to remove any bad shadow data from previous level.\n"
                  "0 = Better perf. It does not clear the shadow which will help set shadowmask texture to be memoryless. This will help reduce gpu bandwidth)\n"
                  "1 = This will disable the memoryless optimization as it would clear the shadow at level load. Only use this if you see residual shadows from previous level showing up in current level.\n");

    DefineConstIntCVar(e_ShadowsMasksLimit, 0, VF_NULL,
        "Maximum amount of allocated shadow mask textures\n"
        "This limits the number of shadow casting lights overlapping\n"
        "0=disable limit(unpredictable memory requirements)\n"
        "1=one texture (4 channels for 4 lights)\n"
        "2=two textures (8 channels for 8 lights), ...");

    REGISTER_CVAR(e_ShadowsUpdateViewDistRatio, 128, VF_NULL,
        "View dist ratio for shadow maps updating for shadowpool");
    DefineConstFloatCVar(e_ShadowsCastViewDistRatioLights, VF_NULL,
        "View dist ratio for shadow maps casting for light sources");
    REGISTER_CVAR(e_ShadowsCastViewDistRatio, 0.8f, VF_NULL,
        "View dist ratio for shadow maps casting from objects");
    REGISTER_CVAR(e_GsmRange, 3.0f, VF_NULL,
        "Size of LOD 0 GSM area (in meters)");
    REGISTER_CVAR(e_GsmRangeStep, 3.0f, VF_NULL,
        "Range of next GSM lod is previous range multiplied by step");
    REGISTER_CVAR_CB(e_GsmLodsNum, 5, VF_NULL,
        "Number of GSM lods (0..5)", OnGsmLodsNumChange);
    DefineConstIntCVar(e_GsmDepthBoundsDebug, 0, VF_NULL,
        "Debug GSM bounds regions calculation");
    DefineConstIntCVar(e_GsmStats, 0, VF_CHEAT,
        "Show GSM statistics 0=off, 1=enable debug to the screens");
    REGISTER_CVAR(e_RNTmpDataPoolMaxFrames, 16, VF_CHEAT,
        "Cache RNTmpData at least for X framres");

    DefineConstIntCVar(e_AutoPrecacheCameraJumpDist, 16, VF_CHEAT,
        "When not 0 - Force full pre-cache of textures, procedural vegetation and shaders\n"
        "if camera moved for more than X meters in one frame or on new cut scene start");
    DefineConstIntCVar(e_AutoPrecacheTexturesAndShaders, 0, VF_CHEAT,
        "Force auto pre-cache of general textures and shaders");
    DefineConstIntCVar(e_AutoPrecacheCgf, 1, VF_CHEAT,
        "Force auto pre-cache of CGF render meshes. 1=pre-cache all meshes around camera. 2=pre-cache only important ones (twice faster)");
    REGISTER_CVAR(e_AutoPrecacheCgfMaxTasks, 8, VF_NULL,
        "Maximum number of parallel streaming tasks during pre-caching");
    REGISTER_CVAR(e_TerrainOcclusionCullingMaxDist,   200.f, VF_NULL,
        "Max length of ray (for version 1)");
    REGISTER_CVAR(e_StreamPredictionUpdateTimeSlice, 0.4f, VF_NULL,
        "Maximum amount of time to spend for scene streaming priority update in milliseconds");
    REGISTER_CVAR(e_StreamAutoMipFactorSpeedThreshold, 0.f, VF_NULL,
        "Debug");
    REGISTER_CVAR(e_StreamAutoMipFactorMin, 0.5f, VF_NULL,
        "Debug");
    REGISTER_CVAR(e_StreamAutoMipFactorMax, 1.0f, VF_NULL,
        "Debug");
    REGISTER_CVAR(e_StreamAutoMipFactorMaxDVD, 0.5f, VF_NULL,
        "Debug");

    REGISTER_CVAR(e_OcclusionCullingViewDistRatio, 0.5f, VF_NULL,
        "Skip per object occlusion test for very far objects - culling on tree level will handle it");

    REGISTER_CVAR(e_Sun, 1, VF_CHEAT,
        "Activates sun light source");
    REGISTER_CVAR(e_CoverageBuffer, 1, VF_NULL,
        "Activates usage of software coverage buffer.\n"
        "1 - camera culling only\n"
        "2 - camera culling and light-to-object check");
    REGISTER_CVAR(e_CoverageBufferVersion, 2, VF_NULL,
        "1 Vladimir's, 2MichaelK's");
    DefineConstIntCVar(e_CoverageBufferDebug, 0, VF_CHEAT,
        "Display content of main camera coverage buffer");
    DefineConstIntCVar(e_CoverageBufferDebugFreeze, 0, VF_CHEAT,
        "Freezes view matrix/-frustum ");
    DefineConstIntCVar(e_CoverageBufferDrawOccluders, 0, VF_CHEAT,
        "Debug draw of occluders for coverage buffer");
    DefineConstIntCVar(e_CoverageBufferTestMode, 2, VF_CHEAT,
        "Debug");
    REGISTER_CVAR(e_CoverageBufferBias, 0.05f, VF_NULL,
        "Coverage buffer z-biasing");
    REGISTER_CVAR(e_CoverageBufferAABBExpand, 0.020f, VF_NULL,
        "expanding the AABB's of the objects to test to avoid z-fighting issues in the Coverage buffer");
    REGISTER_CVAR(e_CoverageBufferEarlyOut, 1, VF_NULL,
        "preempting occluder rasterization to avoid stalling in the main thread if rendering is faster");
    REGISTER_CVAR(e_CoverageBufferEarlyOutDelay, 3.0f, VF_NULL,
        "Time in ms that rasterizer is allowed to continue working after early out request");
    REGISTER_CVAR(e_CoverageBufferRotationSafeCheck, 0, VF_NULL,
        "Coverage buffer safe checking for rotation 0=disabled 1=enabled 2=enabled for out of frustum object");
    DefineConstIntCVar(e_CoverageBufferLightsDebugSide, -1, VF_CHEAT,
        "Debug");
    REGISTER_CVAR(e_CoverageBufferDebugDrawScale, 1, VF_CHEAT,
        "Debug");
    REGISTER_CVAR(e_CoverageBufferResolution, 128, VF_NULL,
        "Resolution of software coverage buffer");

    REGISTER_CVAR(e_CoverageBufferReproj, 0, VF_NULL,
        "Use re-projection technique on CBuffer, 1 simple reproject, 2 additional hole filling, 4 using ocm mesh for occlusion checking");
    REGISTER_CVAR(e_CoverageBufferRastPolyLimit, 500000, VF_NULL,
        "maximum amount of polys to rasterize cap, 0 means no limit\ndefault is 500000");
    REGISTER_CVAR(e_CoverageBufferShowOccluder, 0, VF_NULL,
        "1 show only meshes used as occluder, 2 show only meshes not used as occluder");
    REGISTER_CVAR(e_CoverageBufferAccurateOBBTest, 0, VF_NULL,
        "Checking of OBB boxes instead of AABB or bounding rects");
    DefineConstIntCVar(e_CoverageBufferTolerance, 0, VF_NULL,
        "amount of visible pixel that will still identify the object as covered");
    DefineConstIntCVar(e_CoverageBufferOccludersTestMinTrisNum, 0, VF_CHEAT,
        "Debug");
    REGISTER_CVAR(e_CoverageBufferOccludersViewDistRatio, 1.0f, VF_CHEAT,
        "Debug");
    DefineConstFloatCVar(e_CoverageBufferOccludersLodRatio, VF_CHEAT,
        "Debug");
    DefineConstIntCVar(e_CoverageBufferTreeDebug, 0, VF_CHEAT,
        "Debug");
    DefineConstIntCVar(e_CoverageBufferMaxAddRenderMeshTime, 2, VF_NULL,
        "Max time for unlimited AddRenderMesh");
    REGISTER_CVAR(e_CoverageBufferNumberFramesLatency, 2, VF_NULL,
        "Configures the number of frames of latency between the GPU write of the downsample Z-Target and CPU readback of that target.\n"
        "0 - Disable CPU readback (For debugging)"
        "1 - Coverage buffer uses previous frame's depth information. (Not recommended, CPU may stall waiting on GPU)\n"
        "2 - Coverage buffer uses two frame old depth. (Default)\n"
        "3 - Coverage buffer uses three frame old depth information.");

    DefineConstIntCVar(e_DynamicLightsMaxCount, 512, VF_CHEAT,
        "Sets maximum amount of dynamic light sources");

    DefineConstIntCVar(e_DynamicLights, 1, VF_CHEAT,
        "Activates dynamic light sources");
    DefineConstIntCVar(e_DynamicLightsForceDeferred, 1, VF_CHEAT,
        "Convert all lights to deferred (except sun)");
    REGISTER_CVAR(e_DynamicLightsFrameIdVisTest, 1, VF_NULL,
        "Use based on last draw frame visibility test");
    DefineConstIntCVar(e_DynamicLightsConsistentSortOrder, 1, VF_NULL,
        "Debug");

    DefineConstIntCVar(e_HwOcclusionCullingWater,   1, VF_NULL,
        "Activates usage of HW occlusion test for ocean");

    DefineConstIntCVar(e_Portals, 1, VF_CHEAT,
        "Activates drawing of visareas content (indoors), values 2,3,4 used for debugging");
    DefineConstIntCVar(e_PortalsBigEntitiesFix, 1, VF_CHEAT,
        "Enables special processing of big entities like vehicles intersecting portals");
    DefineConstIntCVar(e_PortalsBlend, 1, VF_CHEAT,
        "Blend lights and cubemaps of vis areas connected to portals 0=off, 1=on");
    REGISTER_CVAR(e_PortalsMaxRecursion, 8, VF_NULL,
        "Maximum number of visareas and portals to traverse for indoor rendering");
    REGISTER_CVAR(e_DynamicLightsMaxEntityLights, 16, VF_NULL,
        "Set maximum number of lights affecting object");
    DefineConstFloatCVar(e_MaxViewDistance, VF_CHEAT,
        "Far clipping plane distance");
    REGISTER_CVAR(e_MaxViewDistSpecLerp, 1, VF_NULL,
        "1 - use max view distance set by designer for very high spec\n0 - for very low spec\nValues between 0 and 1 - will lerp between high and low spec max view distances");
    DefineConstFloatCVar(e_MaxViewDistFullDistCamHeight, VF_CHEAT,
        "Debug");
    DefineConstIntCVar(e_WaterVolumes, e_WaterVolumesDefault, VF_CHEAT,
        "Activates drawing of water volumes\n"
        "1: use usual rendering path\n"
        "2: use fast rendering path with merged fog");
    DefineConstIntCVar(e_RenderTransparentUnderWater, e_RenderTransparentUnderWaterDefault, VF_NULL,
        "Determines how transparent/alphablended objects are rendered in WaterVolume\n"
        "0: they are not rendered under water (fast performance)\n"
        "1: they are rendered twice under water and above water (higher quality)");
    if (!OceanToggle::IsActive())
    {
        REGISTER_CVAR(e_WaterTessellationAmount, 200, VF_NULL,  // being deprecated by Water gem
            "Set tessellation amount");
    }

    REGISTER_CVAR(e_WaterTessellationSwathWidth, 12, VF_NULL,
        "Set the swath width for the boustrophedonic mesh stripping");
    DefineConstIntCVar(e_BBoxes, 0, VF_CHEAT,
        "Activates drawing of bounding boxes");

    DefineConstIntCVar(e_StreamSaveStartupResultsIntoXML, 0, VF_NULL,
        "Save basic information about streaming performance on level start into XML");
    REGISTER_CVAR(e_StreamCgfPoolSize, 24, VF_NULL,
        "Render mesh cache size in MB");
    REGISTER_CVAR(e_SQTestBegin, 0, VF_NULL,
        "If not zero - start streaming latency unit test");
    REGISTER_CVAR(e_SQTestCount, 0, VF_NULL,
        "If not zero - restart test X times");
    REGISTER_CVAR(e_SQTestExitOnFinish, 0, VF_NULL,
        "If not zero - shutdown when finished testing");
    REGISTER_CVAR(e_SQTestDistance, 80, VF_NULL,
        "Distance to travel");
    REGISTER_CVAR(e_SQTestMip, 1, VF_NULL,
        "Mip to wait during test");
    REGISTER_CVAR(e_SQTestMoveSpeed, 10, VF_NULL,
        "Camera speed during test (meters/sec)");

    // Small temp pool size for consoles, editor and pc have much larger capabilities
    DefineConstIntCVar(e_3dEngineTempPoolSize, 1024, VF_NULL,
        "pool size for temporary allocations in kb, requires app restart");

    DefineConstIntCVar(e_3dEngineLogAlways, 0, VF_NULL,
        "Set maximum verbosity to 3dengine.dll log messages");

    DefineConstIntCVar(e_CoverCgfDebug, 0, VF_NULL, "Shows the cover setups on cfg files");

    REGISTER_CVAR(e_StreamCgfMaxTasksInProgress, 32, VF_CHEAT,
        "Maximum number of files simultaneously requested from streaming system");
    REGISTER_CVAR(e_StreamCgfMaxNewTasksPerUpdate, 4, VF_CHEAT,
        "Maximum number of files requested from streaming system per update");
    REGISTER_CVAR(e_StreamPredictionMaxVisAreaRecursion, 9, VF_CHEAT,
        "Maximum number visareas and portals to traverse.");
    REGISTER_CVAR(e_StreamPredictionBoxRadius, 1, VF_CHEAT, "Radius of stream prediction box");
    REGISTER_CVAR(e_StreamPredictionTexelDensity, 1, VF_CHEAT,
        "Use mesh texture mapping density info for textures streaming");
    REGISTER_CVAR(e_StreamPredictionAlwaysIncludeOutside, 0, VF_CHEAT,
        "Always include outside octrees in streaming");
    DefineConstFloatCVar(e_StreamCgfFastUpdateMaxDistance, VF_CHEAT,
        "Update streaming priorities for near objects every second frame");
    DefineConstFloatCVar(e_StreamPredictionMinFarZoneDistance, VF_CHEAT,
        "Debug");
    DefineConstFloatCVar(e_StreamPredictionMinReportDistance, VF_CHEAT,
        "Debug");
    REGISTER_CVAR_CB(e_StreamCgf, 1, VF_REQUIRE_APP_RESTART,
        "Enable streaming of static render meshes", OnCGFStreamingChange);
    DefineConstIntCVar(e_StreamCgfDebug, 0, VF_NULL,
        "Draw helpers and other debug information about CGF streaming\n"
        " 1: Draw color coded boxes for objects taking more than e_StreamCgfDebugMinObjSize,\n"
        "    also shows are the LOD's stored in single CGF or were split into several CGF's\n"
        " 2: Trace into console every loading and unloading operation\n"
        " 3: Print list of currently active objects taking more than e_StreamCgfDebugMinObjSize KB");
    DefineConstIntCVar(e_StreamCgfDebugMinObjSize, 100, VF_CHEAT,
        "Threshold for objects debugging in KB");
    DefineConstIntCVar(e_StreamCgfDebugHeatMap, 0, VF_CHEAT,
        "Generate and show mesh streaming heat map\n"
        " 1: Generate heat map for entire level\n"
        " 2: Show last heat map");
    DefineConstFloatCVar(e_StreamPredictionDistanceFar, VF_CHEAT,
        "Prediction distance for streaming, affects far objects");
    DefineConstFloatCVar(e_StreamPredictionDistanceNear, VF_CHEAT,
        "Prediction distance for streaming, affects LOD of objects");
    DefineConstFloatCVar(e_StreamCgfVisObjPriority, VF_CHEAT,
        "Priority boost for visible objects\n"
        "0 - visible objects has no priority over invisible objects, camera direction does not affect streaming\n"
        "1 - visible objects has highest priority, in case of trashing will produce even more trashing");

    DefineConstFloatCVar(e_StreamCgfGridUpdateDistance, VF_CHEAT,
        "Update streaming priorities when camera moves more than this value");

    DefineConstFloatCVar(e_StreamPredictionAhead, VF_CHEAT,
        "Use predicted camera position for streaming priority updates");

    DefineConstFloatCVar(e_StreamPredictionAheadDebug, VF_CHEAT,
        "Draw ball at predicted position");

    DefineConstFloatCVar(e_DissolveDistMax, VF_CHEAT,
        "At most how near to object MVD dissolve effect triggers (10% of MVD, clamped to this)");

    DefineConstFloatCVar(e_DissolveDistMin, VF_CHEAT,
        "At least how near to object MVD dissolve effect triggers (10% of MVD, clamped to this)");

    DefineConstFloatCVar(e_DissolveDistband, VF_CHEAT,
        "Over how many meters transition takes place");


    DefineConstIntCVar(e_StreamCgfUpdatePerNodeDistance, 1, VF_CHEAT,
        "Use node distance as entity distance for far nodex ");

    DefineConstIntCVar(e_ScissorDebug, 0, VF_CHEAT,
        "Debug");

    REGISTER_CVAR(e_OnDemandPhysics, gEnv->IsEditor() ? 0 : 1, VF_NULL,
        "Turns on on-demand physicalization (0=off)");
    REGISTER_CVAR(e_OnDemandMaxSize, 20.0f, VF_NULL,
        "Specifies the maximum size of vegetation objects that are physicalized on-demand");
    DefineConstIntCVar(e_Sleep, 0, VF_CHEAT,
        "Sleep X in C3DEngine::Draw");
    REGISTER_CVAR(e_ObjectLayersActivation, (m_bEditor ? 0 : 1), VF_CHEAT, "Allow game to activate/deactivate object layers");
    DefineConstIntCVar(e_ObjectLayersActivationPhysics, 1, VF_CHEAT,
        "Allow game to create/free physics of objects: 0: Disable; 1: All; 2: Water only.");
    DefineConstIntCVar(e_Objects, 1, VF_CHEAT,
        "Render or not all objects");
    DefineConstIntCVar(e_Render, e_RenderDefault, VF_CHEAT,
        "Enable engine rendering");
    DefineConstIntCVar(e_ObjectsTreeBBoxes, 0, VF_CHEAT,
        "Debug draw of object tree bboxes");
    /*  REGISTER_CVAR(e_obj_tree_min_node_size, 0, VF_CHEAT,
            "Debug draw of object tree bboxes");
      REGISTER_CVAR(e_obj_tree_max_node_size, 0, VF_CHEAT,
            "Debug draw of object tree bboxes");*/
    REGISTER_CVAR(e_StatObjBufferRenderTasks, 1, VF_NULL,
        "1 - occlusion test on render node level, 2 - occlusion test on render mesh level");
    REGISTER_CVAR(e_CheckOcclusion, 1, VF_NULL, "Perform a visible check in check occlusion job");

    #define DEFAULT_CHECK_OCCLUSION_QUEUE_SIZE 1024
    #define DEFAULT_CHECK_OCCLUSION_OUTPUT_QUEUE_SIZE 4096

    REGISTER_CVAR(e_CheckOcclusionQueueSize, DEFAULT_CHECK_OCCLUSION_QUEUE_SIZE, VF_NULL,
        "Size of queue for data send to check occlusion job");
    REGISTER_CVAR(e_CheckOcclusionOutputQueueSize, DEFAULT_CHECK_OCCLUSION_OUTPUT_QUEUE_SIZE, VF_NULL,
        "Size of queue for data send from check occlusion job");
    REGISTER_CVAR(e_StatObjTessellationMaxEdgeLenght, 1.75f, VF_CHEAT,
        "Split edges longer than X meters");
    REGISTER_CVAR(e_StatObjTessellationMode, 1, VF_CHEAT,
        "Set they way pre-tessellated version of meshes is created: 0 = no pre-tessellation, 1 = load from disk, 2 = generate from normal mesh on loading");
    DefineConstIntCVar(e_StatObjTestOBB, 0, VF_CHEAT,
        "Use additional OBB check for culling");
    DefineConstIntCVar(e_ObjStats, 0, VF_CHEAT,
        "Show instances count");
    DefineConstIntCVar(e_ObjFastRegister, 1, VF_CHEAT,
        "Debug");

    DefineConstIntCVar(e_OcclusionLazyHideFrames, 0, VF_CHEAT,
        "Makes less occluson tests, but it takes more frames to detect invisible objects");
    DefineConstIntCVar(e_OcclusionVolumes, e_OcclusionVolumesDefault, VF_CHEAT,
        "Enable occlusion volumes(antiportals)");
    DefineConstFloatCVar(e_OcclusionVolumesViewDistRatio, VF_NULL,
        "Controls how far occlusion volumes starts to occlude objects");

    DefineConstIntCVar(e_PrecacheLevel, 0, VF_NULL,
        "Pre-render objects right after level loading");
    REGISTER_CVAR(e_Dissolve, 1, VF_NULL,
        "Objects alphatest_noise_fading out on distance and between lods");
    DefineConstIntCVar(e_Lods, 1, VF_NULL,
        "Load and use LOD models for static geometry");
    DefineConstIntCVar(e_LodFaceArea, 1, VF_NULL,
        "Use geometric mean of faces area to compute LOD");
    DefineConstIntCVar(e_LodsForceUse, 1, VF_NULL,
        "Force using LODs even if triangle count do not suit");
    DefineConstFloatCVar(e_LodBoundingBoxDistanceMultiplier, VF_CHEAT,
        "e_LodBoundingBoxDistanceMultiplier ");

    REGISTER_CVAR(e_SQTestDelay, 5.f, VF_NULL,
        "Time to stabilize the system before camera movements");

    DefineConstIntCVar(e_Recursion, 1, VF_NULL,
        "If 0 - will skip recursive render calls like render into texture");
    DefineConstIntCVar(e_RecursionOcclusionCulling, 0, VF_NULL,
        "If 0 - will disable occlusion tests for recursive render calls like render into texture");
    REGISTER_CVAR(e_RecursionViewDistRatio, 0.1f, VF_NULL,
        "Set all view distances shorter by factor of X");

    REGISTER_CVAR(e_Clouds, 1, VF_NULL,
        "Enable clouds rendering");

    REGISTER_CVAR(e_SkyUpdateRate, 0.12f, VF_NULL,
        "Percentage of a full dynamic sky update calculated per frame (0..100].");
    DefineConstIntCVar(e_SkyQuality, 1, VF_NULL,
        "Quality of dynamic sky: 1 (very high), 2 (high).");
    DefineConstIntCVar(e_SkyType, 1, VF_NULL,
        "Type of sky used: 0 (static), 1 (dynamic).");

    DefineConstIntCVar(e_DisplayMemoryUsageIcon, e_DisplayMemoryUsageIconDefault, VF_NULL,
        "Turns On/Off the memory usage icon rendering: 1 on, 0 off.");

    REGISTER_CVAR(e_LodRatio, 6.0f, VF_NULL,
        "LOD distance ratio for objects");
    REGISTER_CVAR(e_LodFaceAreaTargetSize, 0.005f, VF_NULL,
        "Threshold used for LOD computation.");
    REGISTER_CVAR(e_FogVolumeShadingQuality, 0, VF_NULL,
        "Fog Volume Shading Quality 0: standard, 1:high (better fog volume interaction)");
    DefineConstFloatCVar(e_LodCompMaxSize, VF_NULL,
        "Affects LOD selection for big objects, small number will switch more objects into lower LOD");
    REGISTER_CVAR(e_ViewDistRatio, 60.0f, VF_CVARGRP_IGNOREINREALVAL,
        "View distance ratio for objects");
    DefineConstFloatCVar(e_ViewDistCompMaxSize, VF_NULL,
        "Affects max view distance for big objects, small number will render less objects");
    DefineConstFloatCVar(e_ViewDistRatioPortals, VF_NULL,
        "View distance ratio for portals");
    REGISTER_CVAR(e_ViewDistRatioDetail, 30.0f, VF_NULL,
        "View distance ratio for detail objects");
    REGISTER_CVAR(e_ViewDistRatioLights, 50.0f, VF_NULL,
        "View distance ratio for light sources");
    REGISTER_CVAR(e_ViewDistRatioCustom, 60.0f, VF_NULL,
        "View distance ratio for special marked objects (Players,AI,Vehicles)");
    REGISTER_CVAR(e_ViewDistMin, 0.0f, VF_NULL,
        "Min distance on what far objects will be culled out");
    REGISTER_CVAR(e_LodMin, 0, VF_NULL,
        "Min LOD for objects");
    REGISTER_CVAR(e_CharLodMin, 0, VF_NULL,
        "Min LOD for character objects");
    REGISTER_CVAR(e_LodForceUpdate, 0, VF_NULL,
        "When active, recalculate object LOD when rendering instead of using LOD calculated during previous frame.");
    REGISTER_CVAR(e_LodMax, MAX_STATOBJ_LODS_NUM - 1, VF_CHEAT,
        "Max LOD for objects");
    DefineConstIntCVar(e_LodMinTtris, 300, VF_CHEAT,
        "LODs with less triangles will not be used");
    REGISTER_CVAR(e_PhysMinCellSize, 4, VF_NULL,
        "Min size of cell in physical entity grid");
    DefineConstIntCVar(e_PhysEntityGridSizeDefault, 4096, VF_NULL,
        "Default size of the physical entity grid when there's no terrain.");
    REGISTER_CVAR(e_PhysProxyTriLimit, 5000, VF_NULL,
        "Maximum allowed triangle count for phys proxies");
    DefineConstIntCVar(e_PhysFoliage, 2, VF_NULL,
        "Enables physicalized foliage\n"
        "1 - only for dynamic objects\n"
        "2 - for static and dynamic)");
    DefineConstIntCVar(e_RenderMeshUpdateAsync, 1, VF_NULL,
        "Enables async updating of dynamically updated rendermeshes\n"
        "0 - performs a synchronous update\n"
        "1 - performs the update in an async job (default))");
    REGISTER_CVAR(e_FoliageWindActivationDist,  0,  VF_NULL,
        "If the wind is sufficiently strong, visible foliage in this view dist will be forcefully activated");

    DefineConstIntCVar(e_DeformableObjects, e_DeformableObjectsDefault, VF_NULL,
        "Enable / Disable morph based deformable objects");

    REGISTER_CVAR(e_CullVegActivation, 200, VF_NULL,
        "Vegetation activation distance limit; 0 disables visibility-based culling (= unconditional activation)");

    REGISTER_CVAR(e_PhysOceanCell, e_PhysOceanCellDefault, VF_NULL,
        "Cell size for ocean approximation in physics, 0 assumes flat plane");

    DefineConstFloatCVar(e_JointStrengthScale, VF_NULL,
        "Scales the strength of prebroken objects\' joints (for tweaking)");

    DefineConstFloatCVar(e_VolObjShadowStrength, VF_NULL,
        "Self shadow intensity of volume objects [0..1].");

    REGISTER_CVAR(e_ScreenShot, 0, VF_NULL,
        "Make screenshot combined up of multiple rendered frames\n"
        "(negative values for multiple frames, positive for a a single frame)\n"
        " 1 highres\n"
        " 2 360 degree panorama\n"
        " 3 Map top-down view\n"
        "\n"
        "see:\n"
        "  e_ScreenShotWidth, e_ScreenShotHeight, e_ScreenShotQuality, e_ScreenShotMapCenterX,\n"
        "  e_ScreenShotMapCenterY, e_ScreenShotMapSize, e_ScreenShotMinSlices, e_ScreenShotDebug");

    REGISTER_CVAR(e_ScreenShotWidth, 2000, VF_NULL,
        "used for all type highres screenshots made by e_ScreenShot to define the\n"
        "width of the destination image, 2000 default");
    REGISTER_CVAR(e_ScreenShotHeight, 1500, VF_NULL,
        "used for all type highres screenshots made by e_ScreenShot to define the\n"
        "height of the destination image, 1500 default");
    REGISTER_CVAR(e_ScreenShotQuality, 30, VF_NULL,
        "used for all type highres screenshots made by e_ScreenShot to define the quality\n"
        "0=fast, 10 .. 30 .. 100 = extra border in percent (soften seams), negative value to debug");
    REGISTER_CVAR(e_ScreenShotMinSlices, 1, VF_NULL,
        "used for all type highres screenshots made by e_ScreenShot to define the amount\n"
        "of sub-screenshots for the width and height to generate the image,\n the min count\n"
        "will be automatically raised if not sufficient (per screenshot-based)");
    REGISTER_CVAR(e_ScreenShotMapCenterX, 0.0f, VF_NULL,
        "param for the centerX position of the camera, see e_ScreenShotMap\n"
        "defines the x position of the top left corner of the screenshot-area on the terrain,\n"
        "0.0 - 1.0 (0.0 is default)");
    REGISTER_CVAR(e_ScreenShotMapCenterY, 0.0f, VF_NULL,
        "param for the centerX position of the camera, see e_ScreenShotMap\n"
        "defines the y position of the top left corner of the screenshot-area on the terrain,\n"
        "0.0 - 1.0 (0.0 is default)");
    REGISTER_CVAR(e_ScreenShotMapSizeX, 1024.f, VF_NULL,
        "param for the size in worldunits of area to make map screenshot, see e_ScreenShotMap\n"
        "defines the x position of the bottom right corner of the screenshot-area on the terrain,\n"
        "0.0 - 1.0 (1.0 is default)");
    REGISTER_CVAR(e_ScreenShotMapSizeY, 1024.f, VF_NULL,
        "param for the size in worldunits of area to make map screenshot, see e_ScreenShotMap\n"
        "defines the x position of the bottom right corner of the screenshot-area on the terrain,\n"
        "0.0 - 1.0 (1.0 is default)");
    REGISTER_CVAR(e_ScreenShotMapCamHeight, 4000.f, VF_NULL,
        "param for top-down-view screenshot creation, defining the camera height for screenshots,\n"
        "see e_ScreenShotMap defines the y position of the bottom right corner of the\n"
        "screenshot-area on the terrain,\n"
        "0.0 - 1.0 (1.0 is default)");
    REGISTER_CVAR(e_ScreenShotMapOrientation, 0, VF_NULL,
        "param for rotating the orientation through 90 degrees so the screen shot width is along the X axis\n"
        "see e_ScreenShotMap\n"
        "0 - 1 (0 is default)");
    REGISTER_CVAR(e_ScreenShotDebug, 0, VF_NULL,
        "0 off\n1 show stitching borders\n2 show overlapping areas");

    DefineConstIntCVar(e_Ropes, 1, VF_CHEAT,
        "Turn Rendering of Ropes on/off");

    DefineConstIntCVar(e_StatObjValidate, e_StatObjValidateDefault, VF_NULL,
        "Enable CGF mesh validation during loading");

    DefineConstIntCVar(e_StatObjPreload, 1, VF_NULL,
        "Load level CGF's in efficient way");

    DefineConstIntCVar(e_PreloadMaterials, 1, VF_NULL,
        "Preload level materials from level cache pak and resources list");
    DefineConstIntCVar(e_PreloadDecals, 1, VF_NULL,
        "Preload all materials for decals");

    DefineConstIntCVar(e_StatObjMerge, 1, VF_NULL,
        "Enable CGF sub-objects meshes merging");
    DefineConstIntCVar(e_StatObjMergeUseThread, 1, VF_NULL,
        "Use a thread to perform sub-objects meshes merging");
    DefineConstIntCVar(e_StatObjMergeMaxTrisPerDrawCall, 500, VF_NULL,
        "Skip merging of meshes already having acceptable number of triangles per draw call");
    DefineConstIntCVar(e_StatObjStoreMesh, 0, VF_NULL,
        "Store the mesh if enabled, used for cheat detection purposes (they will be stored by default on the dedi server)");

    DefineConstIntCVar(e_DefaultMaterial, 0, VF_CHEAT,
        "use gray illumination as default");

    REGISTER_CVAR(e_ObjQuality, 0, VF_NULL,
        "Object detail quality");
    REGISTER_CVAR(e_LightQuality, 0, VF_NULL,
        "Light detail quality. Controls whether lights are created or casts shadows based on the minimum spec level set in the light configuration."
        "1: Creates or casts shadows from lights that have the minimum spec level set to low."
        "2: Creates or casts shadows from lights that have the minimum spec level set to low or medium."
        "3: Creates or casts shadows from lights that have the minimum spec level set to low, medium or high."
        "4: Creates or casts shadows from lights that have the minimum spec level set to low, medium, high or very high.");
    REGISTER_CVAR(e_ObjShadowCastSpec, 0, VF_NULL,
        "Object shadow casting spec. Only objects with Shadow Cast Spec <= e_ObjShadowCastSpec will cast shadows");

    DefineConstIntCVar(e_LightVolumes, e_LightVolumesDefault, VF_NULL,
        "Allows deferred lighting for registered alpha blended geometry\n"
        "0 = Off\n"
        "1 = Enabled\n"
        "2 = Enabled just for sun light\n");

    DefineConstIntCVar(e_LightVolumesDebug, 0, VF_NULL,
        "Display light volumes debug info\n"
        "0 = Off\n"
        "1 = Enabled\n");

    e_ScreenShotFileFormat = REGISTER_STRING("e_ScreenShotFileFormat", "tga",  VF_NULL,
            "Set output image file format for hires screen shots. Can be jpg or tga");

    e_ScreenShotFileName = REGISTER_STRING("e_ScreenShotFileName", "", VF_NULL, 
        "Sets the output screen shot name, can include relative directories to @user@/ScreenShots");

    e_SQTestTextureName = REGISTER_STRING("e_SQTestTextureName", "strfrn_advrt_boards_screen",  VF_NULL,
            "Reference texture name for streaming latency test");
    e_StreamCgfDebugFilter = REGISTER_STRING("e_StreamCgfDebugFilter", "",  VF_NULL,
            "Show only items containing specified text");

    e_CameraGoto = REGISTER_STRING("e_CameraGoto", "0",  VF_CHEAT,
            "Move cameras to a certain pos/angle");
    e_DebugDrawFilter = REGISTER_STRING("e_DebugDrawFilter", "",  VF_NULL,
            "Show a specified text on DebugDraw");

    REGISTER_CVAR_CB(e_TimeOfDay, 0.0f, VF_CHEAT | VF_CHEAT_NOCHECK, "Current Time of Day", OnTimeOfDayVarChange);
    REGISTER_CVAR_CB(e_TimeOfDaySpeed, 0.0f, VF_CHEAT | VF_CHEAT_NOCHECK, "Time of Day change speed", OnTimeOfDaySpeedVarChange);
    DefineConstIntCVar(e_TimeOfDayDebug, 0, VF_NULL,
        "Display time of day current values on screen");


    DefineConstFloatCVar(e_CameraRotationSpeed, VF_CHEAT,
        "Rotate camera around Z axis for debugging");
    DefineConstIntCVar(e_CameraFreeze, 0, VF_CHEAT,
        "Freeze 3dengine camera (good to debug object culling and LOD).\n"
        "The view frustum is drawn in write frame.\n"
        " 0 = off\n"
        " 1 = activated");

    REGISTER_CVAR(e_GI, 1, VF_NULL,
        "Enable/disable global illumination. Default: 1 - enabled");

    REGISTER_CVAR(e_RenderMeshCollisionTolerance, 0.3f, VF_NULL,
        "Min distance between physics-proxy and rendermesh before collision is considered a hole");

    REGISTER_CVAR(e_WorldSegmentationTest, 0, VF_CHEAT,
        "Debug only: simulates multi-segment behavior in the editor");

    DefineConstIntCVar(e_PrepareDeformableObjectsAtLoadTime, 0, VF_CHEAT,
        "Enable to Prepare deformable objects at load time instead on demand, prevents peaks but increases memory usage");

    DefineConstIntCVar(e_DeferredPhysicsEvents, 1, VF_CHEAT,
        "Enable to Perform some physics events deferred as a task/job");

    REGISTER_CVAR(e_levelStartupFrameNum, 0, VF_NULL,
        "Set to number of frames to capture for avg fps computation");

    REGISTER_CVAR(e_levelStartupFrameDelay, 0, VF_NULL,
        "Set to number of frames to wait after level load before beginning fps measuring");

    REGISTER_CVAR(e_CacheNearestCubePicking, 1, VF_NULL,
        "Enable caching nearest cube maps probe picking for alpha blended geometry");


    REGISTER_CVAR(e_CGFMaxFileSize, -1, VF_CHEAT,
        "will refuse to load any cgf larger than the given filesize (in kb)\n"
        "-1 - 1024 (<0 off (default), >0 filesize limit)");

    REGISTER_CVAR(e_MaxDrawCalls, 0, VF_CHEAT,
        "Will not render CGFs past the given amount of drawcalls\n"
        "(<=0 off (default), >0 draw calls limit)");

    REGISTER_CVAR(e_CheckOctreeObjectsBoxSize, 1, VF_NULL, "CryWarning for crazy sized COctreeNode m_objectsBoxes");
    REGISTER_CVAR(e_DebugGeomPrep, 0, VF_NULL,  "enable logging of Geom preparation");
    DefineConstIntCVar(e_GeomCaches, 1, VF_NULL, "Activates drawing of geometry caches");
    REGISTER_CVAR(e_GeomCacheBufferSize, 128, VF_CHEAT, "Geometry cache stream buffer upper limit size in MB. Default: 128");
    REGISTER_CVAR(e_GeomCacheMaxPlaybackFromMemorySize, 16, VF_CHEAT,
        "Maximum size of geometry cache animated data in MB before always streaming from disk ignoring the memory playback flag. Default: 16");
    REGISTER_CVAR(e_GeomCachePreferredDiskRequestSize, 1024, VF_CHEAT,
        "Preferred disk request size for geometry cache streaming in KB. Default: 1024");
    REGISTER_CVAR(e_GeomCacheMinBufferAheadTime, 2.0f, VF_CHEAT,
        "Time in seconds minimum that data will be buffered ahead for geom cache streaming. Default: 2.0");
    REGISTER_CVAR(e_GeomCacheMaxBufferAheadTime, 5.0f, VF_CHEAT,
        "Time in seconds maximum that data will be buffered ahead for geom cache streaming. Default: 5.0");
    REGISTER_CVAR(e_GeomCacheDecodeAheadTime, 0.5f, VF_CHEAT,
        "Time in seconds that data will be decoded ahead for geom cache streaming. Default: 0.5");
#ifndef _RELEASE
    DefineConstIntCVar(e_GeomCacheDebug, 0, VF_CHEAT, "Show geometry cache debug overlay. Default: 0");
    e_GeomCacheDebugFilter = REGISTER_STRING("e_GeomCacheDebugFilter", "", VF_CHEAT, "Set name filter for e_geomCacheDebug");
    DefineConstIntCVar(e_GeomCacheDebugDrawMode, 0, VF_CHEAT, "Geometry cache debug draw mode\n"
        " 0 = normal\n"
        " 1 = only animated meshes\n"
        " 2 = only static meshes\n"
        " 3 = debug instancing");
#endif
    DefineConstIntCVar(e_GeomCacheLerpBetweenFrames, 1, VF_CHEAT, "Interpolate between geometry cache frames. Default: 1");

    REGISTER_CVAR(e_PermanentRenderObjects, 0, VF_NULL, "Creates permanent render objects for each render node");
    REGISTER_CVAR(e_StaticInstancing, 0, VF_NULL, "Enables instancing of static objects");
    REGISTER_CVAR(e_StaticInstancingMinInstNum, 10, VF_NULL, "Minimum number of common static objects in a tree node before hardware instancing is used.");

    DefineConstIntCVar(e_MemoryProfiling, 0, VF_DEV_ONLY, "Toggle displaying memory usage statistics");
}
