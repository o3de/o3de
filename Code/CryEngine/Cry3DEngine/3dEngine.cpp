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

// Description : Implementation of I3DEngine interface methods


#include "Cry3DEngine_precompiled.h"

#include "3dEngine.h"

#include <MathConversion.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <Terrain/Bus/TerrainProviderBus.h>
#include "VisAreas.h"
#include "ObjMan.h"
#include "Ocean.h"
#include <CryPhysicsDeprecation.h>
#include "DecalManager.h"
#include "IndexedMesh.h"

#include "MatMan.h"

#include "CullBuffer.h"
#include "CGF/CGFLoader.h"
#include "CGF/ChunkFileWriters.h"
#include "CGF/ReadOnlyChunkFile.h"

#include "CloudRenderNode.h"
#include "CloudsManager.h"
#include "SkyLightManager.h"
#include "FogVolumeRenderNode.h"
#include "DecalRenderNode.h"
#include "TimeOfDay.h"
#include "LightEntity.h"
#include "FogVolumeRenderNode.h"
#include "ObjectsTree.h"
#include "WaterVolumeRenderNode.h"
#include "DistanceCloudRenderNode.h"
#include "VolumeObjectRenderNode.h"
#include "RenderMeshMerger.h"
#include "DeferredCollisionEvent.h"
#include "OpticsManager.h"
#include "GeomCacheRenderNode.h"
#include "GeomCacheManager.h"
#include "ClipVolumeManager.h"
#include <IRemoteCommand.h>
#include "PostEffectGroup.h"
#include "MainThreadRenderRequestBus.h"
#include "ObjMan.h"
#include "Environment/OceanEnvironmentBus.h"
#include <limits>
#include <CryPath.h>

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
#include "PrismRenderNode.h"
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

#include <AzFramework/IO/FileOperations.h>
#include <AzFramework/Physics/WindBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Jobs/LegacyJobExecutor.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/parallel/mutex.h>

#include <MathConversion.h>

// required for LARGE_INTEGER used by QueryPerformanceCounter
#ifdef WIN32
#include <CryWindows.h>
#endif

threadID Cry3DEngineBase::m_nMainThreadId = 0;
bool Cry3DEngineBase::m_bRenderTypeEnabled[eERType_TypesNum];
ISystem* Cry3DEngineBase::m_pSystem = 0;
IRenderer* Cry3DEngineBase::m_pRenderer = 0;
ITimer* Cry3DEngineBase::m_pTimer = 0;
ILog* Cry3DEngineBase::m_pLog = 0;

COcean* Cry3DEngineBase::m_pOcean = nullptr;
CObjManager* Cry3DEngineBase::m_pObjManager = 0;
IConsole* Cry3DEngineBase::m_pConsole = 0;
C3DEngine* Cry3DEngineBase::m_p3DEngine = 0;
CVars* Cry3DEngineBase::m_pCVars = 0;
AZ::IO::IArchive* Cry3DEngineBase::m_pCryPak = nullptr;
IOpticsManager* Cry3DEngineBase::m_pOpticsManager = 0;
CDecalManager* Cry3DEngineBase::m_pDecalManager = 0;
CSkyLightManager* Cry3DEngineBase::m_pSkyLightManager = 0;
CCloudsManager* Cry3DEngineBase::m_pCloudsManager = 0;
CVisAreaManager* Cry3DEngineBase::m_pVisAreaManager = 0;
CClipVolumeManager* Cry3DEngineBase::m_pClipVolumeManager = 0;
CRenderMeshMerger* Cry3DEngineBase::m_pRenderMeshMerger = 0;
CMatMan* Cry3DEngineBase::m_pMatMan = 0;
IStreamedObjectListener* Cry3DEngineBase::m_pStreamListener = 0;
#if defined(USE_GEOM_CACHES)
CGeomCacheManager* Cry3DEngineBase::m_pGeomCacheManager = 0;
#endif

bool Cry3DEngineBase::m_bLevelLoadingInProgress = false;
bool Cry3DEngineBase::m_bIsInRenderScene = false;

int Cry3DEngineBase::m_CpuFlags = 0;
#if !defined(CONSOLE)
bool Cry3DEngineBase::m_bEditor = false;
#endif //!defined(CONSOLE)
ESystemConfigSpec Cry3DEngineBase::m_LightConfigSpec = CONFIG_VERYHIGH_SPEC;
int Cry3DEngineBase::m_arrInstancesCounter[eERType_TypesNum];

#define LAST_POTENTIALLY_VISIBLE_TIME 2

namespace
{
    class CLoadLogListener
        : public ILoaderCGFListener
    {
    public:
        virtual ~CLoadLogListener(){}
        virtual void Warning(const char* format) {Cry3DEngineBase::Warning("%s", format); }
        virtual void Error(const char* format) {Cry3DEngineBase::Error("%s", format); }
        virtual bool IsValidationEnabled() { return Cry3DEngineBase::GetCVars()->e_StatObjValidate != 0; }
    };
}

namespace OceanGlobals
{
    float g_oceanLevel = 0.0f;
    float g_oceanStep = 1.0f; 
    AZStd::recursive_mutex g_oceanParamsMutex;
}

//////////////////////////////////////////////////////////////////////
C3DEngine::C3DEngine(ISystem* pSystem)
{
    //#if defined(_DEBUG) && defined(WIN32)
    //  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //#endif

    // Level info
    m_fSunSpecMult = 1.f;
    m_bAreaActivationInUse = false;

    Cry3DEngineBase::m_nMainThreadId = CryGetCurrentThreadId();
    Cry3DEngineBase::m_pSystem = pSystem;
    Cry3DEngineBase::m_pRenderer = gEnv->pRenderer;
    Cry3DEngineBase::m_pTimer = gEnv->pTimer;
    Cry3DEngineBase::m_pLog = gEnv->pLog;
    Cry3DEngineBase::m_pConsole = gEnv->pConsole;
    Cry3DEngineBase::m_p3DEngine = this;
    Cry3DEngineBase::m_pCryPak = gEnv->pCryPak;
    Cry3DEngineBase::m_pCVars = 0;
    Cry3DEngineBase::m_pRenderMeshMerger = new CRenderMeshMerger;
    Cry3DEngineBase::m_pMatMan = new CMatMan;
    Cry3DEngineBase::m_pStreamListener = NULL;
    Cry3DEngineBase::m_CpuFlags = pSystem->GetCPUFlags();

    memset(Cry3DEngineBase::m_arrInstancesCounter, 0, sizeof(Cry3DEngineBase::m_arrInstancesCounter));

#if !defined(CONSOLE)
    m_bEditor = gEnv->IsEditor();
#endif
    m_pObjectsTree = nullptr;
    m_pCVars            = new CVars();
    Cry3DEngineBase::m_pCVars = m_pCVars;

    m_pTimeOfDay = NULL;

    m_postEffectGroups = std::make_unique<PostEffectGroupManager>();
    m_postEffectBaseGroup = m_postEffectGroups->GetGroup("Base");
    if (IPostEffectGroup* pPostEffectsDefaultGroup = m_postEffectGroups->GetGroup(m_defaultPostEffectGroup))
    {
        pPostEffectsDefaultGroup->SetEnable(true);
    }

    m_szLevelFolder[0] = 0;

    m_pSun = 0;
    m_nFlags = 0;
    m_pSkyMat = 0;
    m_pSkyLowSpecMat = 0;
    m_pTerrainWaterMat = 0;
    m_nWaterBottomTexId = 0;
    m_vSunDir = Vec3(5.f, 5.f, DISTANCE_TO_THE_SUN);
    m_vSunDirRealtime = Vec3(5.f, 5.f, DISTANCE_TO_THE_SUN).GetNormalized();

    m_nBlackTexID = 0;

    // create components
    m_pObjManager = CryAlignedNew<CObjManager>();

    m_pDecalManager     = 0;//new CDecalManager   (m_pSystem, this);
    m_pCloudsManager    = new CCloudsManager;
    m_pOpticsManager        = 0;
    m_pVisAreaManager   = 0;
    m_pClipVolumeManager = new CClipVolumeManager();
    m_pSkyLightManager  = CryAlignedNew<CSkyLightManager>();

    // create REs
    m_pRESky              = 0;
    m_pREHDRSky         = 0;

    m_pPhysMaterialEnumerator = 0;

    m_fMaxViewDistHighSpec = 8000;
    m_fMaxViewDistLowSpec  = 1000;

    m_fSkyBoxAngle = 0;
    m_fSkyBoxStretching = 0;


    m_physicsAreaUpdatesHandler = AZStd::make_unique<class PhysicsAreaUpdatesHandler>(m_PhysicsAreaUpdates);

    m_bOcean = true;
    m_nOceanRenderFlags = 0;

    m_bSunShadows = true;
    m_nSunAdditionalCascades = 0;
    m_CachedShadowsBounds.Reset();
    m_nCachedShadowsUpdateStrategy = ShadowMapFrustum::ShadowCacheData::eFullUpdate;

    m_fSunClipPlaneRange = 256.0f;
    m_fSunClipPlaneRangeShift = 0.0f;

    m_nRealLightsNum = m_nDeferredLightsNum = 0;

    m_pCoverageBuffer = CryAlignedNew<CCullBuffer>();

    m_fLightsHDRDynamicPowerFactor = 0.0f;

    m_vHDRFilmCurveParams = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    m_vHDREyeAdaptation = Vec3(0.05f, 0.8f, 0.9f);
    m_fHDRBloomAmount = 0.0f;
    m_vColorBalance = Vec3(1.0f, 1.0f, 1.0f);
    m_fHDRSaturation = 1.0f;

    m_vSkyHightlightPos.Set(0, 0, 0);
    m_vSkyHightlightCol.Set(0, 0, 0);
    m_fSkyHighlightSize = 0;

    m_volFogGlobalDensity = 0.02f;
    m_volFogGlobalDensityMultiplierLDR = 1.0f;
    m_volFogFinalDensityClamp = 1.0f;

    m_idMatLeaves = -1;

    m_oceanFogColor = 0.2f * Vec3(29.0f, 102.0f, 141.0f) / 255.0f;
    m_oceanFogColorShallow = Vec3(0, 0, 0); //0.5f * Vec3( 206.0f, 249.0f, 253.0f ) / 255.0f;
    m_oceanFogDensity = 0; //0.2f;

    m_oceanCausticsDistanceAtten = 100.0f;

    m_oceanCausticDepth = 8.0f;
    m_oceanCausticIntensity = 1.0f;

    m_oceanWindDirection = 1;
    m_oceanWindSpeed = 4.0f;
    m_oceanWavesSpeed = 1.0f;
    m_oceanWavesAmount = 1.5f;
    m_oceanWavesSize = 0.75f;

    m_fParticlesAmbientMultiplier = m_fParticlesLightMultiplier = 1.f;
    m_fRefreshSceneDataCVarsSumm = -1;
    m_nRenderTypeEnableCVarSum = -1;

    if (!m_LTPRootFree.pNext)
    {
        m_LTPRootFree.pNext = &m_LTPRootFree;
        m_LTPRootFree.pPrev = &m_LTPRootFree;
    }

    if (!m_LTPRootUsed.pNext)
    {
        m_LTPRootUsed.pNext = &m_LTPRootUsed;
        m_LTPRootUsed.pPrev = &m_LTPRootUsed;
    }
    m_bResetRNTmpDataPool = false;

    m_fSunDirUpdateTime = 0;
    m_vSunDirNormalized.zero();

    m_volFogRamp = Vec3(0, 100.0f, 0);
    m_volFogShadowRange = Vec3(0.1f, 0, 0);
    m_volFogShadowDarkening = Vec3(0.25f, 1, 1);
    m_volFogShadowEnable = Vec3(0, 0, 0);
    m_volFog2CtrlParams = Vec3(64.0f, 0.0f, 1.0f);
    m_volFog2ScatteringParams = Vec3(1.0f, 0.3f, 0.6f);
    m_volFog2Ramp = Vec3(0.0f, 0.0f, 0.0f);
    m_volFog2Color = Vec3(1.0f, 1.0f, 1.0f);
    m_volFog2GlobalDensity = Vec3(0.1f, 1.0f, 0.4f);
    m_volFog2HeightDensity = Vec3(0.0f, 1.0f, 0.1f);
    m_volFog2HeightDensity2 = Vec3(4000.0f, 0.0001f, 0.95f);
    m_volFog2Color1 = Vec3(1.0f, 1.0f, 1.0f);
    m_volFog2Color2 = Vec3(1.0f, 1.0f, 1.0f);
    m_nightSkyHorizonCol = Vec3(0, 0, 0);
    m_nightSkyZenithCol = Vec3(0, 0, 0);
    m_nightSkyZenithColShift = 0;
    m_nightSkyStarIntensity = 0;
    m_moonDirection = Vec3(0, 0, 0);
    m_nightMoonCol = Vec3(0, 0, 0);
    m_nightMoonSize = 0;
    m_nightMoonInnerCoronaCol = Vec3(0, 0, 0);
    m_nightMoonInnerCoronaScale = 1.0f;
    m_nightMoonOuterCoronaCol = Vec3(0, 0, 0);
    m_nightMoonOuterCoronaScale = 1.0f;
    m_moonRotationLatitude = 0;
    m_moonRotationLongitude = 0;
    m_skyboxMultiplier = 1.0f;
    m_dayNightIndicator = 1.0f;
    m_fogColor2 = Vec3(0, 0, 0);
    m_fogColorRadial = Vec3(0, 0, 0);
    m_volFogHeightDensity = Vec3(0, 1, 0);
    m_volFogHeightDensity2 = Vec3(4000.0f, 0, 0);
    m_volFogGradientCtrl = Vec3(1, 1, 1);

    m_vFogColor = Vec3(1.0f, 1.0f, 1.0f);
    m_vAmbGroundCol = Vec3(0.0f, 0.0f, 0.0f);

    m_dawnStart = 350.0f / 60.0f;
    m_dawnEnd = 360.0f / 60.0f;
    m_duskStart = 12.0f + 360.0f / 60.0f;
    m_duskEnd = 12.0f + 370.0f / 60.0f;

    m_fCloudShadingSunLightMultiplier = 0;
    m_fCloudShadingSkyLightMultiplier = 0;
    m_vCloudShadingCustomSunColor = Vec3(0, 0, 0);
    m_vCloudShadingCustomSkyColor = Vec3(0, 0, 0);

    m_vPrevMainFrameCamPos.Set(-1000000.f, -1000000.f, -1000000.f);
    m_fAverageCameraSpeed = 0;
    m_vAverageCameraMoveDir = Vec3(0);
    m_bContentPrecacheRequested = false;
    m_bTerrainTextureStreamingInProgress = false;
    m_bLayersActivated = false;
    m_eShadowMode = ESM_NORMAL;

    ClearDebugFPSInfo();

    m_fMaxViewDistScale = 1.f;

    m_ptexIconLowMemoryUsage = NULL;
    m_ptexIconAverageMemoryUsage = NULL;
    m_ptexIconHighMemoryUsage = NULL;
    m_ptexIconEditorConnectedToConsole = NULL;
    m_pScreenshotCallback = 0;
    m_bInShutDown = false;
    m_bInUnload = false;
    m_bInLoad = false;

    m_nCloudShadowTexId = 0;

    m_nNightMoonTexId = 0;

    m_pDeferredPhysicsEventManager = new CDeferredPhysicsEventManager();

#if defined(USE_GEOM_CACHES)
    m_pGeomCacheManager = new CGeomCacheManager();
#endif

    m_LightVolumesMgr.Init();

    m_pBreakableBrushHeap = NULL;

    m_nWindSamplePositions = 0;
    m_pWindSamplePositions = NULL;

    m_fZoomFactor = 0.0f;

    m_fAmbMaxHeight = 0.0f;
    m_fAmbMinHeight = 0.0f;

    m_pLightQuality = NULL;
    m_fSaturation = 0.0f;

    m_fGsmRange = 0.0f;
    m_fGsmRangeStep = 0.0f;
    m_fShadowsConstBias = 0.0f;
    m_fShadowsSlopeBias = 0.0f;
    m_nCustomShadowFrustumCount = 0;
    m_bHeightMapAoEnabled = false;

    m_bendingPoolIdx = 0;
    m_levelLoaded = false;
}

//////////////////////////////////////////////////////////////////////
C3DEngine::~C3DEngine()
{
    m_bInShutDown = true;
    m_bInUnload = true;
    m_bInLoad = false;

    CheckMemoryHeap();

    ShutDown();

    delete m_pTimeOfDay;
    delete m_pDecalManager;
    delete m_pVisAreaManager;
    SAFE_DELETE(m_pClipVolumeManager);

    CryAlignedDelete(m_pCoverageBuffer);
    m_pCoverageBuffer = 0;
    CryAlignedDelete(m_pSkyLightManager);
    m_pSkyLightManager = 0;
    SAFE_DELETE(m_pObjectsTree);
    //  delete m_pSceneTree;
    delete m_pRenderMeshMerger;
    delete m_pMatMan;
    m_pMatMan = 0;
    delete m_pCloudsManager;

    delete m_pCVars;

    delete m_pDeferredPhysicsEventManager;
}

bool C3DEngine::CheckMinSpec(uint32 nMinSpec)
{
    return Cry3DEngineBase::CheckMinSpec(nMinSpec);
}

bool C3DEngine::Init()
{
    m_pOpticsManager = new COpticsManager;
    m_pSystem->SetIOpticsManager(m_pOpticsManager);

    for (int i = 0; i < eERType_TypesNum; i++)
    {
        m_bRenderTypeEnabled[i] = true;
    }

    UpdateRenderTypeEnableLookup();

    // Allocate the temporary pool used for allocations during streaming and loading
    const size_t tempPoolSize = static_cast<size_t>(GetCVars()->e_3dEngineTempPoolSize) << 10;
    AZ_Assert(tempPoolSize != 0, "Temp pool size should not be 0.");

    {
        if (!CTemporaryPool::Initialize(tempPoolSize))
        {
            AZ_Assert(false, "Could not initialize initialize temporary pool for 3D Engine startup.");
            return false;
        }
    }

    SFrameLodInfo frameLodInfo;
    frameLodInfo.fLodRatio = GetCVars()->e_LodRatio;

    frameLodInfo.fTargetSize = GetCVars()->e_LodFaceAreaTargetSize;
    AZ_Assert(frameLodInfo.fTargetSize > 0.f, "FrameLodInfo target size should be greater than 0.");
    if (frameLodInfo.fTargetSize <= 0.f)
    {
        frameLodInfo.fTargetSize = 1.f;
    }

    frameLodInfo.nMinLod = GetCVars()->e_LodMin;
    frameLodInfo.nMaxLod = GetCVars()->e_LodMax;
    if (GetCVars()->e_Lods == 0)
    {
        frameLodInfo.nMinLod = 0;
        frameLodInfo.nMaxLod = 0;
    }
    SetFrameLodInfo(frameLodInfo);

    return true;
}

bool C3DEngine::IsCameraAnd3DEngineInvalid(const SRenderingPassInfo& passInfo, const char* szCaller)
{
    const CCamera& rCamera = passInfo.GetCamera();
    const float MAX_M23_REPORTED = 3000000.f; // MAT => upped from 100,000 which spammed this message on spear and cityhall. Really should stop editor generating
    // water levels that trigger this message.

    if (!_finite(rCamera.GetMatrix().m03) || !_finite(rCamera.GetMatrix().m13) || !_finite(rCamera.GetMatrix().m23) || GetMaxViewDistance() <= 0 ||
        rCamera.GetMatrix().m23 < -MAX_M23_REPORTED || rCamera.GetMatrix().m23 > MAX_M23_REPORTED || rCamera.GetFov() < 0.0001f || rCamera.GetFov() > gf_PI)
    {
        Error("Bad camera passed to 3DEngine from %s: Pos=(%.1f, %.1f, %.1f), Fov=%.1f, MaxViewDist=%.1f. Maybe the water level is too extreme.",
            szCaller,
            rCamera.GetMatrix().m03, rCamera.GetMatrix().m13, rCamera.GetMatrix().m23,
            rCamera.GetFov(), _finite(rCamera.GetMatrix().m03) ? GetMaxViewDistance() : 0);
        return true;
    }

    return false;
}

void C3DEngine::OnFrameStart()
{
    FUNCTION_PROFILER_3DENGINE;

    m_nRenderWorldUSecs = 0;
    m_pDeferredPhysicsEventManager->Update();

    m_bendingPoolIdx = (m_bendingPoolIdx + 1) % NUM_BENDING_POOLS;
    m_bendingPool[m_bendingPoolIdx].resize(0);

#if defined(USE_GEOM_CACHES)
    if (!gEnv->IsDedicated())
    {
        if (m_pGeomCacheManager)
        {
            m_pGeomCacheManager->StreamingUpdate();
        }
    }
#endif
    //update texture load handlers
    for (TTextureLoadHandlers::iterator iter = m_textureLoadHandlers.begin(); iter != m_textureLoadHandlers.end(); iter++)
    {
        (*iter)->Update();
    }
}

float GetOceanLevelCallback(int ix, int iy)
{
    using namespace OceanGlobals;
    return OceanToggle::IsActive() ? OceanRequest::GetAccurateOceanHeight(Vec3(ix * g_oceanStep, iy * g_oceanStep, g_oceanLevel)) : gEnv->p3DEngine->GetAccurateOceanHeight(Vec3(ix * g_oceanStep, iy * g_oceanStep, g_oceanLevel));
}

unsigned char GetOceanSurfTypeCallback([[maybe_unused]] int ix, [[maybe_unused]] int iy)
{
    return 0;
}

void C3DEngine::Update()
{
    FUNCTION_PROFILER_3DENGINE_LEGACYONLY;
    AZ_TRACE_METHOD();

    ProcessAsyncStaticObjectLoadRequests();

    m_LightConfigSpec = (ESystemConfigSpec)GetCurrentLightSpec();

    if (GetObjManager())
    {
        GetObjManager()->ClearStatObjGarbage();
    }

    if (m_pDecalManager)
    {
        m_pDecalManager->Update(GetTimer()->GetFrameTime());
    }

    if (GetCVars()->e_PrecacheLevel == 3)
    {
        PrecacheLevel(true, 0, 0);
    }

    DebugDraw_Draw();

    ProcessCVarsChange();

    {
        using namespace OceanGlobals;
        AZStd::lock_guard<decltype(g_oceanParamsMutex)> lock(g_oceanParamsMutex);

        g_oceanLevel = OceanToggle::IsActive() ? OceanRequest::GetOceanLevel() : GetWaterLevel();
    }

    CRenderMeshUtils::ClearHitCache();

    CleanUpOldDecals();

    CDecalRenderNode::ResetDecalUpdatesCounter();


    if (m_pBreakableBrushHeap)
    {
        m_pBreakableBrushHeap->Cleanup();
    }

    // make sure all jobs from the previous frame have finished
    threadID nThreadID;
    gEnv->pRenderer->EF_Query(EFQ_RenderThreadList, nThreadID);
    gEnv->pRenderer->GetFinalizeRendItemJobExecutor(nThreadID)->WaitForCompletion();
    gEnv->pRenderer->GetFinalizeShadowRendItemJobExecutor(nThreadID)->WaitForCompletion();

    UpdateRNTmpDataPool(m_bResetRNTmpDataPool);
    m_bResetRNTmpDataPool = false;

    m_PhysicsAreaUpdates.GarbageCollect();
}

void C3DEngine::Tick()
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::System);

    // make sure all jobs from the previous frame have finished (also in Tick since Update is not called during loading)
    threadID nThreadID = 0;
    gEnv->pRenderer->EF_Query(EFQ_RenderThreadList, nThreadID);
    gEnv->pRenderer->GetFinalizeRendItemJobExecutor(nThreadID)->WaitForCompletion();
    gEnv->pRenderer->GetFinalizeShadowRendItemJobExecutor(nThreadID)->WaitForCompletion();

    AZ::MainThreadRenderRequestBus::ExecuteQueuedEvents();

    AZ::MaterialNotificationEventBus::ExecuteQueuedEvents();

    // clear stored cameras from last frame
    m_RenderingPassCameras[nThreadID].resize(0);
}


void C3DEngine::ProcessCVarsChange()
{
    static int nObjectLayersActivation = -1;

    if (nObjectLayersActivation != GetCVars()->e_ObjectLayersActivation)
    {
        if (GetCVars()->e_ObjectLayersActivation == 2)
        {
            ActivateObjectsLayer(~0, true, true, true, true, "ALL_OBJECTS");
        }
        if (GetCVars()->e_ObjectLayersActivation == 3)
        {
            ActivateObjectsLayer(~0, false, true, true, true, "ALL_OBJECTS");
        }

        nObjectLayersActivation = GetCVars()->e_ObjectLayersActivation;
    }

    float fNewCVarsSumm =
        GetCVars()->e_ShadowsCastViewDistRatio +
        GetCVars()->e_Dissolve +
        GetFloatCVar(e_DissolveDistMin) +
        GetFloatCVar(e_DissolveDistMax) +
        GetFloatCVar(e_DissolveDistband) +
        GetCVars()->e_ViewDistRatio +
        GetCVars()->e_ViewDistMin +
        GetCVars()->e_ViewDistRatioDetail +
        GetCVars()->e_DefaultMaterial +
        GetGeomDetailScreenRes() +
        GetCVars()->e_Portals +
        GetCVars()->e_DebugDraw +
        GetFloatCVar(e_ViewDistCompMaxSize) +
        GetCVars()->e_DecalsDefferedStatic +
        GetRenderer()->GetWidth();

    if (m_fRefreshSceneDataCVarsSumm != -1 && m_fRefreshSceneDataCVarsSumm != fNewCVarsSumm)
    {
        UpdateStatInstGroups();

        AZ::Aabb terrainAabb = AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(terrainAabb, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);
        float terrainSize = AZ::GetMax(terrainAabb.GetXExtent(), terrainAabb.GetYExtent());

        // re-register every instance in level
        constexpr float UNREASONABLY_SMALL_TERRAIN_SIZE = 1.0f;
        constexpr float VERY_LARGE_TERRAIN_SIZE = 16.0f * 1024.0f;
        if (terrainSize < UNREASONABLY_SMALL_TERRAIN_SIZE)
        {
            //Only happens when the runtime terrain system was excluded from this build.
            terrainSize = VERY_LARGE_TERRAIN_SIZE;
        }
        GetObjManager()->ReregisterEntitiesInArea(
            Vec3(-terrainSize, -terrainSize, -terrainSize),
            Vec3(terrainSize * 2.f, terrainSize * 2.f, terrainSize * 2.f));

        // refresh vegetation properties
        UpdateStatInstGroups();

        // force refresh of temporary data associated with visible objects
        MarkRNTmpDataPoolForReset();
    }

    m_fRefreshSceneDataCVarsSumm = fNewCVarsSumm;

    int nRenderTypeEnableCVarSum    =
        (GetCVars()->e_Entities     << 2);

    if (m_nRenderTypeEnableCVarSum != nRenderTypeEnableCVarSum)
    {
        m_nRenderTypeEnableCVarSum = nRenderTypeEnableCVarSum;

        UpdateRenderTypeEnableLookup();
    }

    {
        float fNewCVarsSumm2 =
            GetCVars()->e_LodRatio;

        static float fCVarsSumm2 = fNewCVarsSumm2;

        if (fCVarsSumm2 != fNewCVarsSumm2)
        {
            MarkRNTmpDataPoolForReset();

            fCVarsSumm2 = fNewCVarsSumm2;
        }
    }

    SFrameLodInfo frameLodInfo;
    frameLodInfo.fLodRatio = GetCVars()->e_LodRatio;

    frameLodInfo.fTargetSize = GetCVars()->e_LodFaceAreaTargetSize;
    CRY_ASSERT(frameLodInfo.fTargetSize > 0.f);
    if (frameLodInfo.fTargetSize <= 0.f)
    {
        frameLodInfo.fTargetSize = 1.f;
    }

    frameLodInfo.nMinLod = GetCVars()->e_LodMin;
    frameLodInfo.nMaxLod = GetCVars()->e_LodMax;
    if (GetCVars()->e_Lods == 0)
    {
        frameLodInfo.nMinLod = 0;
        frameLodInfo.nMaxLod = 0;
    }
    SetFrameLodInfo(frameLodInfo);
}

//////////////////////////////////////////////////////////////////////
void C3DEngine::ShutDown()
{
    if (GetRenderer() != GetSystem()->GetIRenderer())
    {
        CryFatalError("Renderer was deallocated before I3DEngine::ShutDown() call");
    }

    UnlockCGFResources();

    UnloadLevel();

#if defined(USE_GEOM_CACHES)
    delete m_pGeomCacheManager;
    m_pGeomCacheManager = 0;
#endif

    if (m_pOpticsManager)
    {
        delete m_pOpticsManager;
        m_pOpticsManager = 0;
        m_pSystem->SetIOpticsManager(m_pOpticsManager);
    }

    CryAlignedDelete(m_pObjManager);
    m_pObjManager = 0;


    // Free the temporary pool's underlying storage
    // and reset the pool
    if (!CTemporaryPool::Shutdown())
    {
        CryFatalError("C3DEngine::Shutdown() could not shutdown temporary pool");
    }

    COctreeNode::Shutdown();
}

#ifndef _RELEASE
void C3DEngine::ProcessStreamingLatencyTest(const CCamera& camIn, CCamera& camOut, const SRenderingPassInfo& passInfo)
{
    static float fSQTestOffset = 0;
    static PodArray<ITexture*> arrTestTextures;
    static ITexture* pTestTexture = 0;
    static ITexture* pLastNotReadyTexture = 0;
    static float fStartTime = 0;
    static float fDelayStartTime = 0;
    static size_t nMaxTexUsage = 0;

    static int nOpenRequestCount = 0;
    SStreamEngineOpenStats stats;
    gEnv->pSystem->GetStreamEngine()->GetStreamingOpenStatistics(stats);
    if (stats.nOpenRequestCount > nOpenRequestCount)
    {
        nOpenRequestCount = stats.nOpenRequestCount;
    }
    else
    {
        nOpenRequestCount = max(0, nOpenRequestCount + stats.nOpenRequestCount) / 2;
    }

    ICVar* pTSFlush = GetConsole()->GetCVar("r_TexturesStreamingDebug");

    if (GetCVars()->e_SQTestBegin == 1)
    { // Init waiting few seconds until streaming is stabilized and all required textures are loaded
        PrintMessage("======== Starting streaming latency test ========");
        fDelayStartTime = GetCurTimeSec();
        nMaxTexUsage = 0;
        GetCVars()->e_SQTestBegin = 2;
        PrintMessage("Waiting %.1f seconds and zero requests and no camera movement", GetCVars()->e_SQTestDelay);

        if (ICVar* pPart = GetConsole()->GetCVar("e_Particles"))
        {
            pPart->Set(0);
        }
        if (ICVar* pAI = GetConsole()->GetCVar("sys_AI"))
        {
            pAI->Set(0);
        }
    }
    else if (GetCVars()->e_SQTestBegin == 2)
    { // Perform waiting
        if (GetCurTimeSec() - fDelayStartTime > GetCVars()->e_SQTestDelay && !nOpenRequestCount && m_fAverageCameraSpeed < .01f)
        {
            pTSFlush->Set(0);
            GetCVars()->e_SQTestBegin = 3;
        }
        else
        {
            pTSFlush->Set(3);
        }
    }
    else if (GetCVars()->e_SQTestBegin == 3)
    { // Build a list of all important loaded textures
        PrintMessage("Collect information about loaded textures");

        fSQTestOffset = (float)GetCVars()->e_SQTestDistance;

        arrTestTextures.Clear();
        SRendererQueryGetAllTexturesParam param;

        GetRenderer()->EF_Query(EFQ_GetAllTextures, param);
        if (param.pTextures)
        {
            for (uint32 i = 0; i < param.numTextures; i++)
            {
                ITexture* pTexture = param.pTextures[i];
                if (pTexture->GetAccessFrameId() > (int)(passInfo.GetMainFrameID() - 4))
                {
                    if (pTexture->GetMinLoadedMip() <= GetCVars()->e_SQTestMip)
                    {
                        if (pTexture->IsStreamable())
                        {
                            if (pTexture->GetWidth() * pTexture->GetHeight() >= 256 * 256)
                            {
                                arrTestTextures.Add(pTexture);

                                if (strstr(pTexture->GetName(), GetCVars()->e_SQTestTextureName->GetString()))
                                {
                                    pTestTexture = pTexture;
                                    PrintMessage("Test texture name: %s", pTexture->GetName());
                                }
                            }
                        }
                    }
                }
            }
        }

        GetRenderer()->EF_Query(EFQ_GetAllTexturesRelease, param);

        PrintMessage("%d test textures found", arrTestTextures.Count());

        PrintMessage("Teleporting camera to offset position");

        GetCVars()->e_SQTestBegin = 4;
    }
    else if (GetCVars()->e_SQTestBegin == 4)
    { // Init waiting few seconds until streaming is stabilized and all required textures are loaded
        fDelayStartTime = GetCurTimeSec();
        GetCVars()->e_SQTestBegin = 5;
        PrintMessage("Waiting %.1f seconds and zero requests and no camera movement", GetCVars()->e_SQTestDelay);
    }
    else if (GetCVars()->e_SQTestBegin == 5)
    { // Move camera to offset position and perform waiting
        Matrix34 mat = camIn.GetMatrix();
        Vec3 vPos = camIn.GetPosition() - camIn.GetViewdir() * fSQTestOffset;
        mat.SetTranslation(vPos);
        camOut.SetMatrix(mat);

        if (GetCurTimeSec() - fDelayStartTime > GetCVars()->e_SQTestDelay && !nOpenRequestCount && m_fAverageCameraSpeed < .01f)
        {
            PrintMessage("Begin camera movement");
            GetCVars()->e_SQTestBegin = 6;
            pTSFlush->Set(0);
        }
        else
        {
            pTSFlush->Set(3);
        }
    }
    else if (GetCVars()->e_SQTestBegin == 6)
    { // Process camera movement from offset position to test point
        Matrix34 mat = camIn.GetMatrix();
        Vec3 vPos = camIn.GetPosition() - camIn.GetViewdir() * fSQTestOffset;
        mat.SetTranslation(vPos);
        camOut.SetMatrix(mat);

        fSQTestOffset -= GetTimer()->GetFrameTime() * (float)GetCVars()->e_SQTestMoveSpeed;

        STextureStreamingStats statsTex(true);
        m_pRenderer->EF_Query(EFQ_GetTexStreamingInfo, statsTex);
        nMaxTexUsage = max(nMaxTexUsage, statsTex.nRequiredStreamedTexturesSize);

        if (fSQTestOffset <= 0)
        {
            PrintMessage("Finished camera movement");
            fStartTime = GetCurTimeSec();
            PrintMessage("Waiting for %d textures to stream in ...", arrTestTextures.Count());

            GetCVars()->e_SQTestBegin = 7;
            pLastNotReadyTexture = 0;
        }
    }
    else if (GetCVars()->e_SQTestBegin == 7)
    { // Wait until test all needed textures are loaded again
        STextureStreamingStats statsTex(true);
        m_pRenderer->EF_Query(EFQ_GetTexStreamingInfo, statsTex);
        nMaxTexUsage = max(nMaxTexUsage, statsTex.nRequiredStreamedTexturesSize);

        if (pTestTexture)
        {
            if (pTestTexture->GetMinLoadedMip() <= GetCVars()->e_SQTestMip)
            {
                PrintMessage("BINGO: Selected test texture loaded in %.1f sec", GetCurTimeSec() - fStartTime);
                pTestTexture = NULL;
                if (!arrTestTextures.Count())
                {
                    GetCVars()->e_SQTestBegin = 0;
                    GetConsole()->GetCVar("e_SQTestBegin")->Set(0);
                }
            }
        }

        if (arrTestTextures.Count())
        {
            int nFinishedNum = 0;
            for (int i = 0; i < arrTestTextures.Count(); i++)
            {
                if (arrTestTextures[i]->GetMinLoadedMip() <= GetCVars()->e_SQTestMip)
                {
                    nFinishedNum++;
                }
                else
                {
                    pLastNotReadyTexture = arrTestTextures[i];
                }
            }

            if (nFinishedNum == arrTestTextures.Count())
            {
                PrintMessage("BINGO: %d of %d test texture loaded in %.1f sec", nFinishedNum, arrTestTextures.Count(), GetCurTimeSec() - fStartTime);
                if (pLastNotReadyTexture)
                {
                    PrintMessage("LastNotReadyTexture: %s [%d x %d]", pLastNotReadyTexture->GetName(), pLastNotReadyTexture->GetWidth(), pLastNotReadyTexture->GetHeight());
                }
                PrintMessage("MaxTexUsage: %" PRISIZE_T " MB", nMaxTexUsage / 1024 / 1024);
                arrTestTextures.Clear();

                GetCVars()->e_SQTestBegin = 0;
                GetConsole()->GetCVar("e_SQTestBegin")->Set(0);

                m_arrProcessStreamingLatencyTestResults.Add(GetCurTimeSec() - fStartTime);
                m_arrProcessStreamingLatencyTexNum.Add(nFinishedNum);

                if (GetCVars()->e_SQTestCount == 0)
                {
                    const char* testResultsFile = "@usercache@/TestResults/Streaming_Latency_Test.xml";

                    AZ::IO::HandleType resultsFile = gEnv->pCryPak->FOpen(testResultsFile, "wb");
                    if (resultsFile != AZ::IO::InvalidHandle)
                    {
                        float fAverTime = 0;
                        for (int i = 0; i < m_arrProcessStreamingLatencyTestResults.Count(); i++)
                        {
                            fAverTime += m_arrProcessStreamingLatencyTestResults[i];
                        }
                        fAverTime /= m_arrProcessStreamingLatencyTestResults.Count();

                        int nAverTexNum = 0;
                        for (int i = 0; i < m_arrProcessStreamingLatencyTexNum.Count(); i++)
                        {
                            nAverTexNum += m_arrProcessStreamingLatencyTexNum[i];
                        }
                        nAverTexNum /= m_arrProcessStreamingLatencyTexNum.Count();

                        AZ::IO::Print(resultsFile, "<phase name=\"Streaming_Latency_Test\">\n"
                            "<metrics name=\"Streaming\">\n"
                            "<metric name=\"AvrLatency\" value=\"%.1f\"/>\n"
                            "<metric name=\"AvrTexNum\" value=\"%d\"/>\n"
                            "</metrics>\n"
                            "</phase>\n",
                            fAverTime,
                            nAverTexNum);
                        gEnv->pCryPak->FClose(resultsFile);
                    }

                    if (GetCVars()->e_SQTestExitOnFinish)
                    {
                        GetSystem()->Quit();
                    }
                }
            }
            else if ((passInfo.GetMainFrameID() & 31) == 0)
            {
                PrintMessage("Waiting: %d of %d test texture loaded in %.1f sec", nFinishedNum, arrTestTextures.Count(), GetCurTimeSec() - fStartTime);
            }
        }
    }
}
#endif

//////////////////////////////////////////////////////////////////////
void C3DEngine::UpdateRenderingCamera([[maybe_unused]] const char* szCallerName, const SRenderingPassInfo& passInfo)
{
    CCamera newCam = passInfo.GetCamera();

    if (passInfo.IsGeneralPass())
    {
        SVOGILegacyRequestBus::Broadcast(&SVOGILegacyRequests::UpdateVoxelData);
    }

    if IsCVarConstAccess(constexpr) (bool(GetFloatCVar(e_CameraRotationSpeed)))
    {
        Matrix34 mat = passInfo.GetCamera().GetMatrix();
        Matrix33 matRot;
        matRot.SetRotationZ(-GetCurTimeSec() * GetFloatCVar(e_CameraRotationSpeed));
        newCam.SetMatrix(mat * matRot);
    }

#if !defined(_RELEASE)

    {
        //this feature move the camera along with the player to a certain position and sets the angle accordingly
        //  (does not work via goto)
        //u can switch it off again via e_CameraGoto 0
        const char* const pCamGoto = GetCVars()->e_CameraGoto->GetString();
        assert(pCamGoto);
        if (strlen(pCamGoto) > 1)
        {
            Ang3 aAngDeg;
            Vec3 vPos;
            int args = azsscanf(pCamGoto, "%f %f %f %f %f %f", &vPos.x, &vPos.y, &vPos.z, &aAngDeg.x, &aAngDeg.y, &aAngDeg.z);
            if (args >= 3)
            {
                Vec3 curPos = newCam.GetPosition();
                if (fabs(vPos.x - curPos.x) > 10.f || fabs(vPos.y - curPos.y) > 10.f || fabs(vPos.z - curPos.z) > 10.f)
                {
                    char buf[128];
                    sprintf_s(buf, "goto %f %f %f", vPos.x, vPos.y, vPos.z);
                    gEnv->pConsole->ExecuteString(buf);
                }
                if (args >= 6)
                {
                    Matrix34 mat = passInfo.GetCamera().GetMatrix();
                    mat.SetTranslation(vPos);
                    mat.SetRotation33(Matrix33::CreateRotationXYZ(DEG2RAD(aAngDeg)));
                    newCam.SetMatrix(mat);
                }
            }
        }
    }

    // Streaming latency test
    if (GetCVars()->e_SQTestCount && !GetCVars()->e_SQTestBegin)
    {
        GetConsole()->GetCVar("e_SQTestBegin")->Set(1);
        GetConsole()->GetCVar("e_SQTestCount")->Set(GetCVars()->e_SQTestCount - 1);
    }
    if (GetCVars()->e_SQTestBegin)
    {
        ProcessStreamingLatencyTest(passInfo.GetCamera(), newCam, passInfo);
    }

#endif

    // set the camera if e_cameraFreeze is not set
    if (GetCVars()->e_CameraFreeze || GetCVars()->e_CoverageBufferDebugFreeze)
    {
        DrawSphere(GetRenderingCamera().GetPosition(), .05f);

        // always set camera to request position for the renderer, allows debugging with e_camerafreeze
        GetRenderer()->SetCamera(gEnv->pSystem->GetViewCamera());
    }
    else
    {
        m_RenderingCamera = newCam;
        // always set camera to request position for the renderer, allows debugging with e_camerafreeze
        GetRenderer()->SetCamera(newCam);
    }

    // now we have a valid camera, we can start generation of the occlusion buffer
    // only needed for editor here, in game we spawn the job more early
    if (passInfo.IsGeneralPass() && GetCVars()->e_StatObjBufferRenderTasks)
    {
        if (gEnv->IsEditor())
        {
            GetObjManager()->PrepareCullbufferAsync(passInfo.GetCamera());
        }
        else
        {
            assert(IsEquivalent(passInfo.GetCamera().GetViewdir(), GetObjManager()->GetCullThread().GetViewDir())); // early set camera differs from current main camera - will cause occlusion errors
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    // update streaming priority of newly seen CComponentRenders (fix for streaming system issue)
    for (int i = 0, nSize = m_deferredRenderComponentStreamingPriorityUpdates.size(); i <  nSize; ++i)
    {
        IRenderNode* pRenderNode = m_deferredRenderComponentStreamingPriorityUpdates[i];
        AABB aabb = pRenderNode->GetBBox();
        const Vec3& vCamPos = GetRenderingCamera().GetPosition();
        float fEntDistance = sqrt_tpl(Distance::Point_AABBSq(vCamPos, aabb)) * passInfo.GetZoomFactor();

        GetObjManager()->UpdateRenderNodeStreamingPriority(pRenderNode, fEntDistance, 1.0f, false, passInfo);
        if (GetCVars()->e_StreamCgfDebug == 2)
        {
            PrintMessage("C3DEngine::RegisterEntity__GetObjManager()->UpdateRenderNodeStreamingPriority %s", pRenderNode->GetName());
        }
    }
    m_deferredRenderComponentStreamingPriorityUpdates.resize(0);
}

void C3DEngine::GetSvoStaticTextures(I3DEngine::SSvoStaticTexInfo& svoInfo, PodArray<I3DEngine::SLightTI>* pLightsTI_S, PodArray<I3DEngine::SLightTI>* pLightsTI_D)
{
    SVOGILegacyRequestBus::Broadcast(&SVOGILegacyRequests::GetSvoStaticTextures, svoInfo, pLightsTI_S, pLightsTI_D);
}

void C3DEngine::GetSvoBricksForUpdate(PodArray<SSvoNodeInfo>& arrNodeInfo, bool getDynamic)
{
    SVOGILegacyRequestBus::Broadcast(&SVOGILegacyRequests::GetSvoBricksForUpdate, arrNodeInfo, getDynamic);
}

#if defined(FEATURE_SVO_GI)
void C3DEngine::LoadTISettings(XmlNodeRef pInputNode)
{
    const char* szXmlNodeName = "Total_Illumination_v2";
    if (gEnv->pConsole->GetCVar("e_svoTI_Active"))
    {
        gEnv->pConsole->GetCVar("e_svoTI_Active")->Set(GetXMLAttribText(pInputNode, szXmlNodeName, "Active", "0"));

        gEnv->pConsole->GetCVar("e_svoTI_InjectionMultiplier")->Set(GetXMLAttribText(pInputNode, szXmlNodeName, "InjectionMultiplier", "0"));

        gEnv->pConsole->GetCVar("e_svoTI_NumberOfBounces")->Set(GetXMLAttribText(pInputNode, szXmlNodeName, "NumberOfBounces", "0"));

        gEnv->pConsole->GetCVar("e_svoTI_Saturation")->Set(GetXMLAttribText(pInputNode, szXmlNodeName, "Saturation", "0"));

        gEnv->pConsole->GetCVar("e_svoTI_ConeMaxLength")->Set(GetXMLAttribText(pInputNode, szXmlNodeName, "ConeMaxLength", "0"));

        gEnv->pConsole->GetCVar("e_svoTI_DiffuseConeWidth")->Set(GetXMLAttribText(pInputNode, szXmlNodeName, "DiffuseConeWidth", "0"));

        gEnv->pConsole->GetCVar("e_svoTI_SSAOAmount")->Set(GetXMLAttribText(pInputNode, szXmlNodeName, "SSAOAmount", "0"));
        gEnv->pConsole->GetCVar("e_svoTI_UseLightProbes")->Set(GetXMLAttribText(pInputNode, szXmlNodeName, "UseLightProbes", "0"));
        gEnv->pConsole->GetCVar("e_svoTI_AmbientOffsetRed")->Set(GetXMLAttribText(pInputNode, szXmlNodeName, "AmbientOffsetRed", "1"));
        gEnv->pConsole->GetCVar("e_svoTI_AmbientOffsetGreen")->Set(GetXMLAttribText(pInputNode, szXmlNodeName, "AmbientOffsetGreen", "1"));
        gEnv->pConsole->GetCVar("e_svoTI_AmbientOffsetBlue")->Set(GetXMLAttribText(pInputNode, szXmlNodeName, "AmbientOffsetBlue", "1"));
        gEnv->pConsole->GetCVar("e_svoTI_AmbientOffsetBias")->Set(GetXMLAttribText(pInputNode, szXmlNodeName, "AmbientOffsetBias", ".1"));

        gEnv->pConsole->GetCVar("e_svoTI_IntegrationMode")->Set(GetXMLAttribText(pInputNode, szXmlNodeName, "IntegrationMode", "0"));

        if (gEnv->pConsole->GetCVar("e_svoTI_IntegrationMode")->GetIVal() < 1) // AO
        {
            gEnv->pConsole->GetCVar("e_svoTI_NumberOfBounces")->Set("1");
        }
    }
}
#endif
void C3DEngine::PrepareOcclusion(const CCamera& rCamera)
{
    if (!gEnv->IsEditor() && GetCVars()->e_StatObjBufferRenderTasks && !gEnv->IsFMVPlaying() && (!IsEquivalent(rCamera.GetPosition(), Vec3(0, 0, 0), VEC_EPSILON) || GetRenderer()->IsPost3DRendererEnabled()))
    {
        GetObjManager()->PrepareCullbufferAsync(rCamera);
    }
}

void C3DEngine::EndOcclusion()
{
    GetObjManager()->EndOcclusionCulling();
}

IStatObj* C3DEngine::LoadStatObjUnsafeManualRef(const char* fileName, const char* geomName, IStatObj::SSubObject** subObject,
    bool useStreaming, unsigned long loadingFlags, const void* data, int dataSize)
{
    return LoadStatObjInternal(fileName, geomName, subObject, useStreaming, loadingFlags, &CObjManager::LoadStatObjUnsafeManualRef, data, dataSize);
}

_smart_ptr<IStatObj> C3DEngine::LoadStatObjAutoRef(const char* fileName, const char* geomName, IStatObj::SSubObject** subObject,
    bool useStreaming, unsigned long loadingFlags, const void* data, int dataSize)
{
    return LoadStatObjInternal(fileName, geomName, subObject, useStreaming, loadingFlags, &CObjManager::LoadStatObjAutoRef, data, dataSize);
}

template<typename TReturn>
TReturn C3DEngine::LoadStatObjInternal(const char* fileName, const char* geomName, IStatObj::SSubObject** subObject, bool useStreaming,
    unsigned long loadingFlags, LoadStatObjFunc<TReturn> loadStatObjFunc, const void* data, int dataSize)
{
    if (!fileName || !fileName[0])
    {
        m_pSystem->Warning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_ERROR, 0, 0, "I3DEngine::LoadStatObj: filename is not specified");
        return nullptr;
    }

    if (!m_pObjManager)
    {
        m_pObjManager = CryAlignedNew<CObjManager>();
    }

    CObjManager* pObjManager = static_cast<CObjManager*>(m_pObjManager);
    return (pObjManager->*loadStatObjFunc)(fileName, geomName, subObject, useStreaming, loadingFlags, data, dataSize, nullptr);
}

void C3DEngine::LoadStatObjAsync(I3DEngine::LoadStaticObjectAsyncResult resultCallback, const char* szFileName, const char* szGeomName, bool bUseStreaming, unsigned long nLoadingFlags)
{
    CRY_ASSERT_MESSAGE(szFileName && szFileName[0], "LoadStatObjAsync: Invalid filename");
    CRY_ASSERT_MESSAGE(m_pObjManager, "Object manager is not ready.");

    StaticObjectAsyncLoadRequest request;
    request.m_callback = resultCallback;
    request.m_filename = szFileName;
    request.m_geomName = szGeomName;
    request.m_useStreaming = bUseStreaming;
    request.m_loadingFlags = nLoadingFlags;

    {
        AZStd::lock_guard<AZStd::mutex> lock(m_statObjQueueLock);
        m_statObjLoadRequests.push(std::move(request));
    }
}

void C3DEngine::ProcessAsyncStaticObjectLoadRequests()
{
    // Same scheme as skinned meshes: CharacterManager::ProcessAsyncLoadRequests.

    enum
    {
        kMaxLoadsPerFrame = 20
    };
    size_t loadsThisFrame = 0;

    while (loadsThisFrame < kMaxLoadsPerFrame)
    {
        StaticObjectAsyncLoadRequest request;
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_statObjQueueLock);
            if (m_statObjLoadRequests.empty())
            {
                break;
            }
            request = std::move(m_statObjLoadRequests.front());
            m_statObjLoadRequests.pop();
        }

        _smart_ptr<IStatObj> object = LoadStatObjAutoRef(request.m_filename.c_str(), request.m_geomName.c_str(), nullptr, request.m_useStreaming, request.m_loadingFlags);
        request.m_callback(object);

        ++loadsThisFrame;
    }
}

IStatObj* C3DEngine::FindStatObjectByFilename(const char* filename)
{
    if (filename ==  NULL)
    {
        return NULL;
    }

    if (filename[0] == 0)
    {
        return NULL;
    }

    return m_pObjManager->FindStaticObjectByFilename(filename);
}

void C3DEngine::RegisterEntity(IRenderNode* pEnt, int nSID, int nSIDConsideredSafe)
{
    FUNCTION_PROFILER_3DENGINE;
    if (gEnv->mMainThreadId != CryGetCurrentThreadId())
    {
        CryFatalError("C3DEngine::RegisterEntity should only be called on main thread.");
    }

    uint32 nFrameID = GetRenderer()->GetFrameID();
    AsyncOctreeUpdate(pEnt, nSID, nSIDConsideredSafe, nFrameID, false);
}

void C3DEngine::UnRegisterEntityDirect(IRenderNode* pEnt)
{
    UnRegisterEntityImpl(pEnt);
}

void C3DEngine::UnRegisterEntityAsJob(IRenderNode* pEnt)
{
    AsyncOctreeUpdate(pEnt, (int)0, (int)0, (int)0, true);
}

bool C3DEngine::CreateDecalInstance(const CryEngineDecalInfo& decal, CDecal* pCallerManagedDecal)
{
    if (!GetCVars()->e_Decals && !pCallerManagedDecal)
    {
        return false;
    }

    return m_pDecalManager->Spawn(decal, pCallerManagedDecal);
}

void C3DEngine::SelectEntity(IRenderNode* pEntity)
{
    static IRenderNode* pSelectedNode;
    static float fLastTime;
    if (pEntity && GetCVars()->e_Decals == 3)
    {
        float fCurTime = gEnv->pTimer->GetAsyncCurTime();
        if (fCurTime - fLastTime < 1.0f)
        {
            return;
        }
        fLastTime = fCurTime;
        if (pSelectedNode)
        {
            pSelectedNode->SetRndFlags(ERF_SELECTED, false);
        }
        pEntity->SetRndFlags(ERF_SELECTED, true);
        pSelectedNode = pEntity;
    }
}

void C3DEngine::CreateDecal(const struct CryEngineDecalInfo& decal)
{
    IF (!GetCVars()->e_DecalsAllowGameDecals, 0)
    {
        return;
    }

    if (GetCVars()->e_Decals == 2)
    {
        IRenderNode* pRN = decal.ownerInfo.pRenderNode;
        PrintMessage("Debug: C3DEngine::CreateDecal: Pos=(%.1f,%.1f,%.1f) Size=%.2f DecalMaterial=%s HitObjectName=%s(%s)",
            decal.vPos.x, decal.vPos.y, decal.vPos.z, decal.fSize, decal.szMaterialName,
            pRN ? pRN->GetName() : "NULL", pRN ? pRN->GetEntityClassName() : "NULL");
    }

    assert(!decal.pExplicitRightUpFront); // only game-play decals come here

    static uint32 nGroupId = 0;
    nGroupId++;

    if ((GetCVars()->e_DecalsDefferedStatic == 1 && decal.pExplicitRightUpFront) ||
        (GetCVars()->e_DecalsDefferedDynamic == 1 && !decal.pExplicitRightUpFront &&
         (!decal.ownerInfo.pRenderNode || 
             decal.ownerInfo.pRenderNode->GetRenderNodeType() == eERType_StaticMeshRenderComponent ||
             decal.fGrowTimeAlpha || decal.fSize > GetFloatCVar(e_DecalsDefferedDynamicMinSize)))
        && !decal.bForceSingleOwner)
    {
        CryEngineDecalInfo decal_adjusted = decal;
        decal_adjusted.nGroupId = nGroupId;
        decal_adjusted.bDeferred = true;
        m_pDecalManager->SpawnHierarchical(decal_adjusted, NULL);
        return;
    }

    if (decal.ownerInfo.pRenderNode && decal.fSize > 0.5f && !decal.bForceSingleOwner)
    {
        PodArray<SRNInfo> lstEntities;
        Vec3 vRadius(decal.fSize, decal.fSize, decal.fSize);
        const AABB cExplosionBox(decal.vPos - vRadius, decal.vPos + vRadius);

        if (CVisArea* pArea = (CVisArea*)decal.ownerInfo.pRenderNode->GetEntityVisArea())
        {
            if (pArea->m_pObjectsTree)
            {
                pArea->m_pObjectsTree->MoveObjectsIntoList(&lstEntities, &cExplosionBox, false, true, true, true);
            }
        }
        else
        {
            Get3DEngine()->MoveObjectsIntoListGlobal(&lstEntities, &cExplosionBox, false, true, true, true);
        }

        for (int i = 0; i < lstEntities.Count(); i++)
        {
            // decals on statobj's of render node
            CryEngineDecalInfo decalOnRenderNode = decal;
            decalOnRenderNode.ownerInfo.pRenderNode = lstEntities[i].pNode;
            decalOnRenderNode.nGroupId = nGroupId;

            //      if(decalOnRenderNode.ownerInfo.pRenderNode->GetRenderNodeType() != decal.ownerInfo.pRenderNode->GetRenderNodeType())
            //      continue;

            if (decalOnRenderNode.ownerInfo.pRenderNode->GetRndFlags() & ERF_HIDDEN)
            {
                continue;
            }

            m_pDecalManager->SpawnHierarchical(decalOnRenderNode, NULL);
        }
    }
    else
    {
        CryEngineDecalInfo decalStatic = decal;
        decalStatic.nGroupId = nGroupId;
        m_pDecalManager->SpawnHierarchical(decalStatic, NULL);
    }
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::SetSunColor(Vec3 vColor)
{
    if (m_pObjManager)
    {
        m_pObjManager->SetSunColor(vColor);
        m_pObjManager->SetSunAnimColor(vColor);
    }
}

Vec3 C3DEngine::GetSunAnimColor()
{
    if (m_pObjManager)
    {
        return m_pObjManager->GetSunAnimColor();
    }

    return Vec3();
}

void C3DEngine::SetSunAnimColor(const Vec3& sunAnimColor)
{
    if (m_pObjManager)
    {
        m_pObjManager->SetSunAnimColor(sunAnimColor);
    }
}

float C3DEngine::GetSunAnimSpeed()
{
    if (m_pObjManager)
    {
        return m_pObjManager->GetSunAnimSpeed();
    }

    return 0.0f;
}

void C3DEngine::SetSunAnimSpeed(float sunAnimSpeed)
{
    if (m_pObjManager)
    {
        m_pObjManager->SetSunAnimSpeed(sunAnimSpeed);
    }
}

AZ::u8 C3DEngine::GetSunAnimPhase()
{
    if (m_pObjManager)
    {
        return m_pObjManager->GetSunAnimPhase();
    }

    return 0;
}

void C3DEngine::SetSunAnimPhase(AZ::u8 sunAnimPhase)
{
    if (m_pObjManager)
    {
        m_pObjManager->SetSunAnimPhase(sunAnimPhase);
    }
}

AZ::u8 C3DEngine::GetSunAnimIndex()
{
    if (m_pObjManager)
    {
        return m_pObjManager->GetSunAnimIndex();
    }

    return 0;
}

void C3DEngine::SetSunAnimIndex(AZ::u8 sunAnimIndex)
{
    if (m_pObjManager)
    {
        m_pObjManager->SetSunAnimIndex(sunAnimIndex);
    }
}

void C3DEngine::SetSSAOAmount(float fMul)
{
    if (m_pObjManager)
    {
        m_pObjManager->SetSSAOAmount(fMul);
    }
}

void C3DEngine::SetSSAOContrast(float fMul)
{
    if (m_pObjManager)
    {
        m_pObjManager->SetSSAOContrast(fMul);
    }
}

void C3DEngine::RemoveAllStaticObjects([[maybe_unused]] int nSID)
{
    if (!IsObjectTreeReady())
    {
        return;
    }

    PodArray<SRNInfo> lstObjects;

    // Don't remove objects from the octree, since our query will return more objects than just the ones we're looking for.
    const bool removeObjectsFromOctree = false;

    // Skip gathering decals, since we're only looking for vegetation.
    const bool skipDecals = true;
    const bool skipERFNoDecalNodeDecals = true;

    // Skip dynamic vegetation, since we're only looking to remove static vegetation.
    const bool skipDynamicObjects = true;

    GetObjectTree()->MoveObjectsIntoList(&lstObjects, nullptr, removeObjectsFromOctree, skipDecals, skipERFNoDecalNodeDecals, skipDynamicObjects);
}

void C3DEngine::OnExplosion(Vec3 vPos, float fRadius, [[maybe_unused]] bool bDeformTerrain)
{
    if (GetCVars()->e_Decals == 2)
    {
        PrintMessage("Debug: C3DEngine::OnExplosion: Pos=(%.1f,%.1f,%.1f) fRadius=%.2f", vPos.x, vPos.y, vPos.z, fRadius);
    }

    auto terrain = AzFramework::Terrain::TerrainDataRequestBus::FindFirstHandler();
    if (!terrain)
    {
        return;
    }

    AZ::Aabb terrainAabb = terrain->GetTerrainAabb();
    if (!terrainAabb.Contains(LYVec3ToAZVec3(vPos)) || fRadius <= 0)
    {
        return; // out of terrain
    }

    const AZ::Vector2 terrainGridResolution = terrain->GetTerrainGridResolution();
    const float unitSizeX = terrainGridResolution.GetX();
    const float unitSizeY = terrainGridResolution.GetY();

    // do not create decals near the terrain holes
    {
        for (float x = vPos.x - fRadius; x <= vPos.x + fRadius + 1.0f; x += unitSizeX)
        {
            for (float y = vPos.y - fRadius; y <= vPos.y + fRadius + 1.0f; y += unitSizeY)
            {
                if (terrain->GetIsHoleFromFloats(x, y))
                {
                    return;
                }
            }
        }
    }

    // reduce ground decals size depending on distance to the ground
    float terrainHeight = terrain->GetHeightFromFloats(vPos.x, vPos.y);
    float fExploHeight = vPos.z - terrainHeight;

    // delete decals what can not be correctly updated
    Vec3 vRadius(fRadius, fRadius, fRadius);
    AABB areaBox(vPos - vRadius, vPos + vRadius);
    Get3DEngine()->DeleteDecalsInRange(&areaBox, NULL);
}

float C3DEngine::GetMaxViewDistance(bool bScaled)
{
    // lerp between specs
    float fMaxViewDist;

    // camera height lerp factor
    if (OceanToggle::IsActive() && !OceanRequest::OceanIsEnabled())
    {
        fMaxViewDist = m_fMaxViewDistHighSpec;
    }
    else
    {
        // spec lerp factor
        float lerpSpec = clamp_tpl(GetCVars()->e_MaxViewDistSpecLerp, 0.0f, 1.0f);

        // lerp between specs
        fMaxViewDist = m_fMaxViewDistLowSpec * (1.f - lerpSpec) + m_fMaxViewDistHighSpec * lerpSpec;

        const float waterLevel = OceanToggle::IsActive() ? OceanRequest::GetOceanLevel() : GetWaterLevel();
        float lerpHeight = clamp_tpl(max(0.f, GetSystem()->GetViewCamera().GetPosition().z - waterLevel) / GetFloatCVar(e_MaxViewDistFullDistCamHeight), 0.0f, 1.0f);

        // lerp between prev result and high spec
        fMaxViewDist = fMaxViewDist * (1.f - lerpHeight) + m_fMaxViewDistHighSpec * lerpHeight;
    }

    if (bScaled)
    {
        fMaxViewDist *= m_fMaxViewDistScale;
    }

    // for debugging
    const float fMaxViewDistCVar = GetFloatCVar(e_MaxViewDistance);
    fMaxViewDist = (float)fsel(fMaxViewDistCVar, fMaxViewDistCVar, fMaxViewDist);

    fMaxViewDist = (float)fsel(fabsf(fMaxViewDist) - 0.100001f, fMaxViewDist, 0.100001f);

    // eliminate some floating point inconsistency here, there's no point in nitpicking 7999.9995 view distance vs 8000
    fMaxViewDist = AZ::ClampIfCloseMag<float>(fMaxViewDist, float(round(fMaxViewDist)), 0.01f);

    return fMaxViewDist;
}

void C3DEngine::SetFrameLodInfo(const SFrameLodInfo& frameLodInfo)
{
    if (frameLodInfo.fLodRatio != m_frameLodInfo.fLodRatio || frameLodInfo.fTargetSize != m_frameLodInfo.fTargetSize)
    {
        ++m_frameLodInfo.nID;
        m_frameLodInfo.fLodRatio = frameLodInfo.fLodRatio;
        m_frameLodInfo.fTargetSize = frameLodInfo.fTargetSize;
    }
    m_frameLodInfo.nMinLod = frameLodInfo.nMinLod;
    m_frameLodInfo.nMaxLod = frameLodInfo.nMaxLod;
}

void C3DEngine::SetFogColor(const Vec3& vFogColor)
{
    m_vFogColor    = vFogColor;
    GetRenderer()->SetClearColor(m_vFogColor);
}

Vec3 C3DEngine::GetFogColor()
{
    return m_vFogColor;
}

void C3DEngine::GetSkyLightParameters(Vec3& sunDir, Vec3& sunIntensity, float& Km, float& Kr, float& g, Vec3& rgbWaveLengths)
{
    CSkyLightManager::SSkyDomeCondition skyCond;
    m_pSkyLightManager->GetCurSkyDomeCondition(skyCond);

    g = skyCond.m_g;
    Km = skyCond.m_Km;
    Kr = skyCond.m_Kr;
    sunIntensity = skyCond.m_sunIntensity;
    rgbWaveLengths = skyCond.m_rgbWaveLengths;
    sunDir = skyCond.m_sunDirection;
}

void C3DEngine::SetSkyLightParameters(const Vec3& sunDir, const Vec3& sunIntensity, float Km, float Kr, float g, const Vec3& rgbWaveLengths, bool forceImmediateUpdate)
{
    CSkyLightManager::SSkyDomeCondition skyCond;

    skyCond.m_g = g;
    skyCond.m_Km = Km;
    skyCond.m_Kr = Kr;
    skyCond.m_sunIntensity = sunIntensity;
    skyCond.m_rgbWaveLengths = rgbWaveLengths;
    skyCond.m_sunDirection = sunDir;

    m_pSkyLightManager->SetSkyDomeCondition(skyCond);
    if (forceImmediateUpdate && IsHDRSkyMaterial(GetSkyMaterial()))
    {
        m_pSkyLightManager->FullUpdate();
    }
}

void C3DEngine::SetLightsHDRDynamicPowerFactor(const float value)
{
    m_fLightsHDRDynamicPowerFactor = value;
}

float C3DEngine::GetLightsHDRDynamicPowerFactor() const
{
    return m_fLightsHDRDynamicPowerFactor;
}

bool C3DEngine::IsTessellationAllowedForShadowMap(const SRenderingPassInfo& passInfo) const
{
#ifdef MESH_TESSELLATION_ENGINE
    SRenderingPassInfo::EShadowMapType shadowType = passInfo.GetShadowMapType();
    switch (shadowType)
    {
    case SRenderingPassInfo::SHADOW_MAP_GSM:
        return passInfo.ShadowFrustumLod() < GetCVars()->e_ShadowsTessellateCascades;
    case SRenderingPassInfo::SHADOW_MAP_LOCAL:
        return GetCVars()->e_ShadowsTessellateDLights != 0;
    case SRenderingPassInfo::SHADOW_MAP_NONE:
    default:
        return false;
    }
#endif //#ifdef MESH_TESSELLATION_ENGINE

    return false;
}

void C3DEngine::SetPhysMaterialEnumerator(IPhysMaterialEnumerator* pPhysMaterialEnumerator)
{
    m_pPhysMaterialEnumerator = pPhysMaterialEnumerator;
}

IPhysMaterialEnumerator* C3DEngine::GetPhysMaterialEnumerator()
{
    return m_pPhysMaterialEnumerator;
}

float C3DEngine::GetDistanceToSectorWithWater()
{
    const Vec3 camPosition = GetRenderingCamera().GetPosition();
    const float minDistance = 0.1f;
    const bool oceanActive = OceanToggle::IsActive();
    const bool oceanEnabled = OceanRequest::OceanIsEnabled();
    float distance = minDistance;

    if (oceanActive && !oceanEnabled)
    {
        distance = std::numeric_limits<float>::infinity();
    }
    else
    {
        if (oceanActive && oceanEnabled)
        {
            distance = camPosition.z - OceanRequest::GetOceanLevel();
        }
        else
        {
            distance = std::numeric_limits<float>::infinity();
        }
    }

    return max(distance, minDistance);
}

Vec3 C3DEngine::GetSunColor() const
{
    return m_pObjManager ? m_pObjManager->GetSunColor() : Vec3(0, 0, 0);
}

float C3DEngine::GetSSAOAmount() const
{
    return m_pObjManager ? m_pObjManager->GetSSAOAmount() : 1.0f;
}

float C3DEngine::GetSSAOContrast() const
{
    return m_pObjManager ? m_pObjManager->GetSSAOContrast() : 1.0f;
}

void C3DEngine::SetRainParams(const SRainParams& rainParams)
{
    if (m_pObjManager)
    {
        m_pObjManager->GetRainParams().bIgnoreVisareas = rainParams.bIgnoreVisareas;
        m_pObjManager->GetRainParams().bDisableOcclusion = rainParams.bDisableOcclusion;
        m_pObjManager->GetRainParams().qRainRotation = rainParams.qRainRotation;
        m_pObjManager->GetRainParams().vWorldPos = rainParams.vWorldPos;
        m_pObjManager->GetRainParams().vColor = rainParams.vColor;
        m_pObjManager->GetRainParams().fAmount = rainParams.fAmount;
        m_pObjManager->GetRainParams().fCurrentAmount = rainParams.fCurrentAmount;
        m_pObjManager->GetRainParams().fRadius = rainParams.fRadius;
        m_pObjManager->GetRainParams().fFakeGlossiness = rainParams.fFakeGlossiness;
        m_pObjManager->GetRainParams().fFakeReflectionAmount = rainParams.fFakeReflectionAmount;
        m_pObjManager->GetRainParams().fDiffuseDarkening = rainParams.fDiffuseDarkening;
        m_pObjManager->GetRainParams().fRainDropsAmount = rainParams.fRainDropsAmount;
        m_pObjManager->GetRainParams().fRainDropsSpeed = rainParams.fRainDropsSpeed;
        m_pObjManager->GetRainParams().fRainDropsLighting = rainParams.fRainDropsLighting;
        m_pObjManager->GetRainParams().fMistAmount = rainParams.fMistAmount;
        m_pObjManager->GetRainParams().fMistHeight = rainParams.fMistHeight;
        m_pObjManager->GetRainParams().fPuddlesAmount = rainParams.fPuddlesAmount;
        m_pObjManager->GetRainParams().fPuddlesMaskAmount = rainParams.fPuddlesMaskAmount;
        m_pObjManager->GetRainParams().fPuddlesRippleAmount = rainParams.fPuddlesRippleAmount;
        m_pObjManager->GetRainParams().fSplashesAmount = rainParams.fSplashesAmount;

        m_pObjManager->GetRainParams().nUpdateFrameID = GetRenderer()->GetFrameID();
    }
}

bool C3DEngine::GetRainParams(SRainParams& rainParams)
{
    bool bRet = false;
    const int nFrmID = GetRenderer()->GetFrameID();
    if (m_pObjManager)
    {
        // Copy shared rain data only
        rainParams.bIgnoreVisareas = m_pObjManager->GetRainParams().bIgnoreVisareas;
        rainParams.bDisableOcclusion = m_pObjManager->GetRainParams().bDisableOcclusion;
        rainParams.qRainRotation = m_pObjManager->GetRainParams().qRainRotation;
        rainParams.vWorldPos = m_pObjManager->GetRainParams().vWorldPos;
        rainParams.vColor = m_pObjManager->GetRainParams().vColor;
        rainParams.fAmount = m_pObjManager->GetRainParams().fAmount;
        rainParams.fCurrentAmount = m_pObjManager->GetRainParams().fCurrentAmount;
        rainParams.fRadius = m_pObjManager->GetRainParams().fRadius;
        rainParams.fFakeGlossiness = m_pObjManager->GetRainParams().fFakeGlossiness;
        rainParams.fFakeReflectionAmount = m_pObjManager->GetRainParams().fFakeReflectionAmount;
        rainParams.fDiffuseDarkening = m_pObjManager->GetRainParams().fDiffuseDarkening;
        rainParams.fRainDropsAmount = m_pObjManager->GetRainParams().fRainDropsAmount;
        rainParams.fRainDropsSpeed = m_pObjManager->GetRainParams().fRainDropsSpeed;
        rainParams.fRainDropsLighting = m_pObjManager->GetRainParams().fRainDropsLighting;
        rainParams.fMistAmount = m_pObjManager->GetRainParams().fMistAmount;
        rainParams.fMistHeight = m_pObjManager->GetRainParams().fMistHeight;
        rainParams.fPuddlesAmount = m_pObjManager->GetRainParams().fPuddlesAmount;
        rainParams.fPuddlesMaskAmount = m_pObjManager->GetRainParams().fPuddlesMaskAmount;
        rainParams.fPuddlesRippleAmount = m_pObjManager->GetRainParams().fPuddlesRippleAmount;
        rainParams.fSplashesAmount = m_pObjManager->GetRainParams().fSplashesAmount;

        if (!IsOutdoorVisible() && !rainParams.bIgnoreVisareas)
        {
            rainParams.fAmount = 0.f;
        }

        bRet = m_pObjManager->GetRainParams().nUpdateFrameID == nFrmID;
    }
    return bRet;
}

void C3DEngine::SetSnowSurfaceParams(const Vec3& vCenter, float fRadius, float fSnowAmount, float fFrostAmount, float fSurfaceFreezing)
{
    if (m_pObjManager)
    {
        m_pObjManager->GetSnowParams().m_vWorldPos = vCenter;
        m_pObjManager->GetSnowParams().m_fRadius = fRadius;
        m_pObjManager->GetSnowParams().m_fSnowAmount = fSnowAmount;
        m_pObjManager->GetSnowParams().m_fFrostAmount = fFrostAmount;
        m_pObjManager->GetSnowParams().m_fSurfaceFreezing = fSurfaceFreezing;
    }
}

bool C3DEngine::GetSnowSurfaceParams(Vec3& vCenter, float& fRadius, float& fSnowAmount, float& fFrostAmount, float& fSurfaceFreezing)
{
    bool bRet = false;
    if (m_pObjManager)
    {
        vCenter = m_pObjManager->GetSnowParams().m_vWorldPos;
        fRadius = m_pObjManager->GetSnowParams().m_fRadius;
        fSnowAmount = 0.f;
        fFrostAmount = 0.f;
        fSurfaceFreezing = 0.f;
        if (IsOutdoorVisible())
        {
            fSnowAmount = m_pObjManager->GetSnowParams().m_fSnowAmount;
            fFrostAmount = m_pObjManager->GetSnowParams().m_fFrostAmount;
            fSurfaceFreezing = m_pObjManager->GetSnowParams().m_fSurfaceFreezing;
        }
        bRet = true;
    }
    return bRet;
}

void C3DEngine::SetSnowFallParams(int nSnowFlakeCount, float fSnowFlakeSize, float fSnowFallBrightness, float fSnowFallGravityScale, float fSnowFallWindScale, float fSnowFallTurbulence, float fSnowFallTurbulenceFreq)
{
    if (m_pObjManager)
    {
        m_pObjManager->GetSnowParams().m_nSnowFlakeCount = nSnowFlakeCount;
        m_pObjManager->GetSnowParams().m_fSnowFlakeSize = fSnowFlakeSize;
        m_pObjManager->GetSnowParams().m_fSnowFallBrightness = fSnowFallBrightness;
        m_pObjManager->GetSnowParams().m_fSnowFallGravityScale = fSnowFallGravityScale;
        m_pObjManager->GetSnowParams().m_fSnowFallWindScale = fSnowFallWindScale;
        m_pObjManager->GetSnowParams().m_fSnowFallTurbulence = fSnowFallTurbulence;
        m_pObjManager->GetSnowParams().m_fSnowFallTurbulenceFreq = fSnowFallTurbulenceFreq;
    }
}

bool C3DEngine::GetSnowFallParams(int& nSnowFlakeCount, float& fSnowFlakeSize, float& fSnowFallBrightness, float& fSnowFallGravityScale, float& fSnowFallWindScale, float& fSnowFallTurbulence, float& fSnowFallTurbulenceFreq)
{
    bool bRet = false;
    if (m_pObjManager)
    {
        nSnowFlakeCount = 0;
        fSnowFlakeSize = 0.f;
        fSnowFallBrightness = 0.f;
        fSnowFallGravityScale = 0.f;
        fSnowFallWindScale = 0.f;
        fSnowFallTurbulence = 0.f;
        fSnowFallTurbulenceFreq = 0.f;
        if (IsOutdoorVisible())
        {
            nSnowFlakeCount = m_pObjManager->GetSnowParams().m_nSnowFlakeCount;
            fSnowFlakeSize = m_pObjManager->GetSnowParams().m_fSnowFlakeSize;
            fSnowFallBrightness = m_pObjManager->GetSnowParams().m_fSnowFallBrightness;
            fSnowFallGravityScale = m_pObjManager->GetSnowParams().m_fSnowFallGravityScale;
            fSnowFallWindScale = m_pObjManager->GetSnowParams().m_fSnowFallWindScale;
            fSnowFallTurbulence = m_pObjManager->GetSnowParams().m_fSnowFallTurbulence;
            fSnowFallTurbulenceFreq = m_pObjManager->GetSnowParams().m_fSnowFallTurbulenceFreq;
        }
        bRet = true;
    }
    return bRet;
}

void C3DEngine::SetSunDir(const Vec3& newSunDir)
{
    Vec3 vSunDirNormalized = newSunDir.normalized();
    m_vSunDirRealtime = vSunDirNormalized;
    if (vSunDirNormalized.Dot(m_vSunDirNormalized) < GetFloatCVar(e_SunAngleSnapDot) ||
        GetCurTimeSec() - m_fSunDirUpdateTime > GetFloatCVar(e_SunAngleSnapSec))
    {
        m_vSunDirNormalized = vSunDirNormalized;
        m_vSunDir = m_vSunDirNormalized * DISTANCE_TO_THE_SUN;

        m_fSunDirUpdateTime = GetCurTimeSec();
    }
}

Vec3 C3DEngine::GetSunDir()  const
{
    return m_vSunDir;
}

Vec3 C3DEngine::GetRealtimeSunDirNormalized() const
{
    return m_vSunDirRealtime;
}

void C3DEngine::FreeRenderNodeState(IRenderNode* pEnt)
{
    // make sure we don't try to update the streaming priority if an object
    // was added and removed in the same frame
    int nElementID = m_deferredRenderComponentStreamingPriorityUpdates.Find(pEnt);
    if (nElementID != -1)
    {
        m_deferredRenderComponentStreamingPriorityUpdates.DeleteFastUnsorted(nElementID);
    }

    m_pObjManager->RemoveFromRenderAllObjectDebugInfo(pEnt);


#if !defined(_RELEASE)
    if (!gEnv->IsDedicated())
    {
        //As render nodes can be deleted in many places, it's possible that the map of render nodes used by stats gathering (r_stats 6, perfHUD, debug gun) could get aliased.
        //Ensure that this node is removed from the map to prevent a dereference after deletion.
        gEnv->pRenderer->ForceRemoveNodeFromDrawCallsMap(pEnt);
    }
#endif

    m_lstAlwaysVisible.Delete(pEnt);

    if (m_pDecalManager && (pEnt->m_nInternalFlags & IRenderNode::DECAL_OWNER))
    {
        m_pDecalManager->OnEntityDeleted(pEnt);
    }

    if (pEnt->GetRenderNodeType() == eERType_Light)
    {
        GetRenderer()->OnEntityDeleted(pEnt);
    }

    if (pEnt->GetRndFlags() & (ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS))
    { // make sure pointer to object will not be used somewhere in the renderer
#if !defined(_RELEASE)
        if (!(pEnt->GetRndFlags() & ERF_HAS_CASTSHADOWMAPS))
        {
            Warning("IRenderNode has ERF_CASTSHADOWMAPS set but not ERF_HAS_CASTSHADOWMAPS, name: '%s', class: '%s'.", pEnt->GetName(), pEnt->GetEntityClassName());
        }
#endif
        Get3DEngine()->OnCasterDeleted(pEnt);
    }

    UnRegisterEntityImpl(pEnt);

    if (pEnt->m_pRNTmpData)
    {
        Get3DEngine()->FreeRNTmpData(&pEnt->m_pRNTmpData);
        assert(!pEnt->m_pRNTmpData);
    }
}

const char* C3DEngine::GetLevelFilePath(const char* szFileName)
{
    cry_strcpy(m_sGetLevelFilePathTmpBuff, m_szLevelFolder);
    if (*szFileName && (*szFileName == '/' || *szFileName == '\\'))
    {
        cry_strcat(m_sGetLevelFilePathTmpBuff, szFileName + 1);
    }
    else
    {
        cry_strcat(m_sGetLevelFilePathTmpBuff, szFileName);
    }
    return m_sGetLevelFilePathTmpBuff;
}

void C3DEngine::ActivatePortal(const Vec3& vPos, bool bActivate, const char* szEntityName)
{
    if (m_pVisAreaManager)
    {
        m_pVisAreaManager->ActivatePortal(vPos, bActivate, szEntityName);
    }
}

bool C3DEngine::SetStatInstGroup(int nGroupId, const IStatInstGroup& siGroup, int nSID)
{
    if (m_pObjManager->GetListStaticTypes().Count() == 0)
    {
        AZ_Warning("C3DEngine", false, "Trying to set a Stat instance without an initialized Object manager.  This might be caused by using the vegetation system without terrain.");
        return false;
    }
    if ((nSID < 0) || (nSID >= m_pObjManager->GetListStaticTypes().Count()))
    {
        AZ_Assert(false, "Invalid StatInst ID: %d (should be > 0 and < %d)", nSID, m_pObjManager->GetListStaticTypes().Count());
        return false;
    }

    m_fRefreshSceneDataCVarsSumm = -100;

    m_pObjManager->GetListStaticTypes()[nSID].resize(max(nGroupId + 1, m_pObjManager->GetListStaticTypes()[nSID].Count()));

    if (nGroupId < 0 || nGroupId >= m_pObjManager->GetListStaticTypes()[nSID].Count())
    {
        return false;
    }

    StatInstGroup& rGroup = m_pObjManager->GetListStaticTypes()[nSID][nGroupId];

    // If the object was changed in the editor, ResetActiveNodes will need to be called later
    // Keep track of the previous object so we can check for this later
    _smart_ptr<IStatObj> pPreviousObject = rGroup.pStatObj;
    rGroup.pStatObj = siGroup.pStatObj;

    if (rGroup.pStatObj)
    {
        cry_strcpy(rGroup.szFileName, siGroup.pStatObj->GetFilePath());
    }
    else
    {
        rGroup.szFileName[0] = 0;
    }

    rGroup.bHideability = siGroup.bHideability;
    rGroup.bHideabilitySecondary = siGroup.bHideabilitySecondary;
    rGroup.nPlayerHideable = siGroup.nPlayerHideable;
    rGroup.fBending = siGroup.fBending;
    rGroup.nCastShadowMinSpec = siGroup.nCastShadowMinSpec;
    rGroup.bRecvShadow = siGroup.bRecvShadow;
    rGroup.bDynamicDistanceShadows = siGroup.bDynamicDistanceShadows;

    rGroup.bUseAlphaBlending = siGroup.bUseAlphaBlending;
    rGroup.fSpriteDistRatio = siGroup.fSpriteDistRatio;
    rGroup.fLodDistRatio = siGroup.fLodDistRatio;
    rGroup.fShadowDistRatio = siGroup.fShadowDistRatio;
    rGroup.fMaxViewDistRatio = siGroup.fMaxViewDistRatio;

    rGroup.fBrightness = siGroup.fBrightness;

    _smart_ptr<IMaterial> pPreviousGroupMaterial = rGroup.pMaterial;
    rGroup.pMaterial = siGroup.pMaterial;

    rGroup.fDensity = siGroup.fDensity;
    rGroup.fElevationMax = siGroup.fElevationMax;
    rGroup.fElevationMin = siGroup.fElevationMin;
    rGroup.fSize = siGroup.fSize;
    rGroup.fSizeVar = siGroup.fSizeVar;
    rGroup.fSlopeMax = siGroup.fSlopeMax;
    rGroup.fSlopeMin = siGroup.fSlopeMin;
    rGroup.fStiffness = siGroup.fStiffness;
    rGroup.fDamping = siGroup.fDamping;
    rGroup.fVariance = siGroup.fVariance;
    rGroup.fAirResistance = siGroup.fAirResistance;

    rGroup.bRandomRotation = siGroup.bRandomRotation;
    rGroup.nRotationRangeToTerrainNormal = siGroup.nRotationRangeToTerrainNormal;
    rGroup.nMaterialLayers = siGroup.nMaterialLayers;

    rGroup.bAllowIndoor = siGroup.bAllowIndoor;
    rGroup.fAlignToTerrainCoefficient = siGroup.fAlignToTerrainCoefficient;

    bool previousAutoMerged = rGroup.bAutoMerged;
    rGroup.bAutoMerged = siGroup.bAutoMerged;
    rGroup.minConfigSpec = siGroup.minConfigSpec;

    rGroup.nID = siGroup.nID;

    rGroup.Update(GetCVars(), Get3DEngine()->GetGeomDetailScreenRes());

    MarkRNTmpDataPoolForReset();

    return true;
}

bool C3DEngine::GetStatInstGroup(int nGroupId, IStatInstGroup& siGroup, int nSID)
{
    assert(nSID >= 0 && nSID < m_pObjManager->GetListStaticTypes().Count());

    if (nGroupId < 0 || nGroupId >= m_pObjManager->GetListStaticTypes()[nSID].Count())
    {
        return false;
    }

    StatInstGroup& rGroup = m_pObjManager->GetListStaticTypes()[nSID][nGroupId];

    siGroup.pStatObj            = rGroup.pStatObj;
    if (siGroup.pStatObj)
    {
        cry_strcpy(siGroup.szFileName, rGroup.pStatObj->GetFilePath());
    }

    siGroup.bHideability    = rGroup.bHideability;
    siGroup.bHideabilitySecondary   = rGroup.bHideabilitySecondary;
    siGroup.nPlayerHideable = rGroup.nPlayerHideable;
    siGroup.fBending            = rGroup.fBending;
    siGroup.nCastShadowMinSpec   = rGroup.nCastShadowMinSpec;
    siGroup.bRecvShadow   = rGroup.bRecvShadow;
    siGroup.bDynamicDistanceShadows   = rGroup.bDynamicDistanceShadows;

    siGroup.bUseAlphaBlending                       = rGroup.bUseAlphaBlending;
    siGroup.fSpriteDistRatio                        = rGroup.fSpriteDistRatio;
    siGroup.fLodDistRatio               = rGroup.fLodDistRatio;
    siGroup.fShadowDistRatio                        = rGroup.fShadowDistRatio;
    siGroup.fMaxViewDistRatio                       = rGroup.fMaxViewDistRatio;

    siGroup.fBrightness                                 = rGroup.fBrightness;
    siGroup.pMaterial                                       = rGroup.pMaterial;

    siGroup.fDensity                                        = rGroup.fDensity;
    siGroup.fElevationMax                               = rGroup.fElevationMax;
    siGroup.fElevationMin                               = rGroup.fElevationMin;
    siGroup.fSize                                               = rGroup.fSize;
    siGroup.fSizeVar                                        = rGroup.fSizeVar;
    siGroup.fSlopeMax                                       = rGroup.fSlopeMax;
    siGroup.fSlopeMin                                       = rGroup.fSlopeMin;
    siGroup.bAutoMerged       = rGroup.bAutoMerged;

    siGroup.fStiffness = rGroup.fStiffness;
    siGroup.fDamping = rGroup.fDamping;
    siGroup.fVariance = rGroup.fVariance;
    siGroup.fAirResistance = rGroup.fAirResistance;

    siGroup.nID = rGroup.nID;

    return true;
}

void C3DEngine::UpdateStatInstGroups()
{
    if (!m_pObjManager)
    {
        return;
    }

    for (uint32 nSID = 0; nSID < m_pObjManager->GetListStaticTypes().size(); nSID++)
    {
        PodArray<StatInstGroup>& rGroupTable = m_pObjManager->GetListStaticTypes()[nSID];
        for (uint32 nGroupId = 0; nGroupId < rGroupTable.size(); nGroupId++)
        {
            StatInstGroup& rGroup = rGroupTable[nGroupId];
            rGroup.Update(GetCVars(), Get3DEngine()->GetGeomDetailScreenRes());
        }
    }
}

void C3DEngine::GetMemoryUsage(class ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this) + (GetCVars()->e_StreamCgfDebug ? 100 * 1024 * 1024 : 0));
    pSizer->AddObject(m_pCVars);

    pSizer->AddObject(m_lstStaticLights);
    pSizer->AddObject(m_arrLightProjFrustums);

    pSizer->AddObject(arrFPSforSaveLevelStats);
    pSizer->AddObject(m_lstAlwaysVisible);

    if (CTemporaryPool* pPool = CTemporaryPool::Get())
    {
        SIZER_COMPONENT_NAME(pSizer, "Temporary Pool");
        pPool->GetMemoryUsage(pSizer);
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "RenderMeshMerger");
        m_pRenderMeshMerger->GetMemoryUsage(pSizer);
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "Optics");
        pSizer->AddObject(m_pOpticsManager);
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "SkyLightManager");
        pSizer->AddObject(m_pSkyLightManager);
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "DecalManager");
        pSizer->AddObject(m_pDecalManager);
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "OutdoorObjectsTree");
        pSizer->AddObject(m_pObjectsTree);
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "ObjManager");
        pSizer->AddObject(m_pObjManager);
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "Ocean");
        pSizer->AddObject(m_pOcean);
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "VisAreas");
        pSizer->AddObject(m_pVisAreaManager);
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "ClipVolumes");
        pSizer->AddObject(m_pClipVolumeManager);
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "CoverageBuffer");
        pSizer->AddObject(m_pCoverageBuffer);
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "RNTmpDataPool");

        CRNTmpData* pNext = NULL;

        for (CRNTmpData* pElem = m_LTPRootFree.pNext; pElem != &m_LTPRootFree; pElem = pNext)
        {
            pSizer->AddObject(pElem, sizeof(*pElem));
            pNext = pElem->pNext;
        }

        for (CRNTmpData* pElem = m_LTPRootUsed.pNext; pElem != &m_LTPRootUsed; pElem = pNext)
        {
            pSizer->AddObject(pElem, sizeof(*pElem));
            pNext = pElem->pNext;
        }
    }
}

void C3DEngine::GetResourceMemoryUsage(ICrySizer* pSizer, const AABB& cstAABB)
{
    //////////////////////////////////////////////////////////////////////////
    std::vector<IRenderNode*>   cFoundRenderNodes;
    unsigned int                            nFoundObjects(0);

    nFoundObjects = GetObjectsInBox(cstAABB);
    cFoundRenderNodes.resize(nFoundObjects, NULL);
    GetObjectsInBox(cstAABB, &cFoundRenderNodes.front());

    size_t nCurrentRenderNode(0);
    size_t nTotalNumberOfRenderNodes(0);

    nTotalNumberOfRenderNodes = cFoundRenderNodes.size();
    for (nCurrentRenderNode = 0; nCurrentRenderNode < nTotalNumberOfRenderNodes; ++nCurrentRenderNode)
    {
        IRenderNode*& piRenderNode = cFoundRenderNodes[nCurrentRenderNode];

        _smart_ptr<IMaterial>  piMaterial(piRenderNode->GetMaterialOverride());
        if (!piMaterial)
        {
            piMaterial = piRenderNode->GetMaterial();
        }

        if (piMaterial)
        {
            piMaterial->GetResourceMemoryUsage(pSizer);
        }

        {
            IRenderMesh*    piMesh(NULL);
            size_t              nCount(0);

            piMesh = piRenderNode->GetRenderMesh(nCount);
            for (; piMesh; ++nCount, piMesh = piRenderNode->GetRenderMesh(nCount))
            {
                // Timur, RenderMesh may not be loaded due to streaming!
                piMesh->GetMemoryUsage(pSizer, IRenderMesh::MEM_USAGE_COMBINED);
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////
}

bool C3DEngine::IsUnderWater(const Vec3& vPos) const
{
    bool bUnderWater = false;
    // Check underwater
    AZ_UNUSED(vPos)
    CRY_PHYSICS_REPLACEMENT_ASSERT();
    return bUnderWater;
}

void C3DEngine::SetOceanRenderFlags(uint8 nFlags)
{
    m_nOceanRenderFlags = nFlags;
}

uint32 C3DEngine::GetOceanVisiblePixelsCount() const
{
    return COcean::GetVisiblePixelsCount();
}

float C3DEngine::GetBottomLevel(const Vec3& referencePos, float maxRelevantDepth, [[maybe_unused]] int objflags)
{
    FUNCTION_PROFILER_3DENGINE;

    float terrainWorldZ = AzFramework::Terrain::TerrainDataRequests::GetDefaultTerrainHeight();
    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(terrainWorldZ
        , &AzFramework::Terrain::TerrainDataRequests::GetHeightFromFloats
        , referencePos.x, referencePos.y, AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, nullptr);

    const float padding = 0.2f;
    float rayLength;

    // NOTE: Terrain is above referencePos, so referencePos is probably inside a voxel or something.
    if (terrainWorldZ <= referencePos.z)
    {
        rayLength = min(maxRelevantDepth, (referencePos.z - terrainWorldZ));
    }
    else
    {
        rayLength = maxRelevantDepth;
    }

    rayLength += padding * 2.0f;

    // Get bottom level
    CRY_PHYSICS_REPLACEMENT_ASSERT();

    // Terrain was above or too far below referencePos, and no solid object was close enough.
    return BOTTOM_LEVEL_UNKNOWN;
}

float C3DEngine::GetBottomLevel(const Vec3& referencePos, float maxRelevantDepth /* = 10.0f */)
{
    return GetBottomLevel (referencePos, maxRelevantDepth, ent_terrain | ent_static | ent_sleeping_rigid | ent_rigid);
}

float C3DEngine::GetBottomLevel(const Vec3& referencePos, int objflags)
{
    return GetBottomLevel (referencePos, 10.0f, objflags);
}

#if defined(USE_GEOM_CACHES)
IGeomCache* C3DEngine::LoadGeomCache(const char* szFileName)
{
    if (!szFileName || !szFileName[0])
    {
        m_pSystem->Warning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_ERROR, 0, 0, "I3DEngine::LoadGeomCache: filename is not specified");
        return 0;
    }

    return m_pGeomCacheManager->LoadGeomCache(szFileName);
}

IGeomCache* C3DEngine::FindGeomCacheByFilename(const char* szFileName)
{
    if (!szFileName || szFileName[0] == 0)
    {
        return NULL;
    }

    return m_pGeomCacheManager->FindGeomCacheByFilename(szFileName);
}
#endif

namespace
{
    // The return value is the next position of the source buffer.
    int ReadFromBuffer(const char* pSource, int nSourceSize, int nSourcePos, void* pDest, int nDestSize)
    {
        if (pDest == 0 || nDestSize == 0)
        {
            return 0;
        }

        if (nSourcePos < 0 || nSourcePos >= nSourceSize)
        {
            return 0;
        }

        memcpy(pDest, &pSource[nSourcePos], nDestSize);

        return nSourcePos + nDestSize;
    }
}

IStatObj* C3DEngine::LoadDesignerObject(int nVersion, const char* szBinaryStream, int size)
{
    if (nVersion < 0 || nVersion > 2)
    {
        return NULL;
    }

    int nBufferPos = 0;
    int32 nSubObjectCount = 0;
    nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, &nSubObjectCount, sizeof(int32));

    IStatObj* pStatObj = gEnv->p3DEngine->CreateStatObj();
    if (!pStatObj)
    {
        return NULL;
    }

    std::vector<IStatObj*> statObjList;
    if (nSubObjectCount == 2)
    {
        pStatObj->AddSubObject(gEnv->p3DEngine->CreateStatObj());
        pStatObj->AddSubObject(gEnv->p3DEngine->CreateStatObj());
        pStatObj->GetIndexedMesh()->FreeStreams();
        statObjList.push_back(pStatObj->GetSubObject(0)->pStatObj);
        statObjList.push_back(pStatObj->GetSubObject(1)->pStatObj);
    }
    else
    {
        statObjList.push_back(pStatObj);
    }

    if (nVersion == 2)
    {
        int32 nStaticObjFlags = 0;
        nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, &nStaticObjFlags, sizeof(int32));
        pStatObj->SetFlags(nStaticObjFlags);
    }

    for (int k = 0, iCount(statObjList.size()); k < iCount; ++k)
    {
        int32 nPositionCount = 0;
        int32 nTexCoordCount = 0;
        int32 nFaceCount = 0;
        int32 nIndexCount = 0;
        int32 nTangentCount = 0;
        int32 nSubsetCount = 0;

        nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, &nPositionCount, sizeof(int32));
        nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, &nTexCoordCount, sizeof(int32));
        if (nPositionCount <= 0 || nTexCoordCount <= 0)
        {
            return NULL;
        }

        if (nVersion == 0)
        {
            nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, &nFaceCount, sizeof(int32));
            if (nFaceCount <= 0)
            {
                return NULL;
            }
        }
        else if (nVersion >= 1)
        {
            nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, &nIndexCount, sizeof(int32));
            if (nIndexCount <= 0)
            {
                return NULL;
            }
            nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, &nTangentCount, sizeof(int32));
        }
        nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, &nSubsetCount, sizeof(int32));
        IIndexedMesh* pMesh = statObjList[k]->GetIndexedMesh();
        if (!pMesh)
        {
            return NULL;
        }

        pMesh->FreeStreams();
        pMesh->SetVertexCount((int)nPositionCount);
        pMesh->SetFaceCount((int)nFaceCount);
        pMesh->SetIndexCount((int)nIndexCount);
        pMesh->SetTexCoordCount((int)nTexCoordCount);

        Vec3* const positions = pMesh->GetMesh()->GetStreamPtr<Vec3>(CMesh::POSITIONS);
        Vec3* const normals = pMesh->GetMesh()->GetStreamPtr<Vec3>(CMesh::NORMALS);
        SMeshTexCoord* const texcoords = pMesh->GetMesh()->GetStreamPtr<SMeshTexCoord>(CMesh::TEXCOORDS);

        nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, positions, sizeof(Vec3) * nPositionCount);
        nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, normals, sizeof(Vec3) * nPositionCount);
        nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, texcoords, sizeof(SMeshTexCoord) * nTexCoordCount);
        if (nVersion == 0)
        {
            SMeshFace* const faces = pMesh->GetMesh()->GetStreamPtr<SMeshFace>(CMesh::FACES);
            nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, faces, sizeof(SMeshFace) * nFaceCount);
        }
        else if (nVersion >= 1)
        {
            vtx_idx* const indices = pMesh->GetMesh()->GetStreamPtr<vtx_idx>(CMesh::INDICES);
            if constexpr (sizeof(vtx_idx) == sizeof(uint16))
            {
                nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, indices, sizeof(uint16) * nIndexCount);
            }
            else
            {
                uint16* indices16 = new uint16[nIndexCount];
                nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, indices16, sizeof(uint16) * nIndexCount);
                for (int32 i = 0; i < nIndexCount; ++i)
                {
                    indices[i] = (vtx_idx)indices16[i];
                }
                delete [] indices16;
            }
            pMesh->SetTangentCount((int)nTangentCount);
            if (nTangentCount > 0)
            {
                SMeshTangents* const tangents = pMesh->GetMesh()->GetStreamPtr<SMeshTangents>(CMesh::TANGENTS);
                nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, tangents, sizeof(SMeshTangents) * nTangentCount);
            }
        }

        pMesh->SetSubSetCount(nSubsetCount);
        for (int i = 0; i < nSubsetCount; ++i)
        {
            SMeshSubset subset;
            nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, &subset.vCenter, sizeof(Vec3));
            nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, &subset.fRadius, sizeof(float));
            nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, &subset.fTexelDensity, sizeof(float));

            int32 nFirstIndexId = 0;
            int32 nNumIndices = 0;
            int32 nFirstVertId = 0;
            int32 nNumVerts = 0;
            int32 nMatID = 0;
            int32 nMatFlags = 0;
            int32 nPhysicalizeType = 0;

            nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, &nFirstIndexId, sizeof(int32));
            nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, &nNumIndices, sizeof(int32));
            nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, &nFirstVertId, sizeof(int32));
            nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, &nNumVerts, sizeof(int32));
            nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, &nMatID, sizeof(int32));
            nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, &nMatFlags, sizeof(int32));
            nBufferPos = ReadFromBuffer(szBinaryStream, size, nBufferPos, &nPhysicalizeType, sizeof(int32));

            pMesh->SetSubsetBounds(i, subset.vCenter, subset.fRadius);
            pMesh->SetSubsetIndexVertexRanges(i, (int)nFirstIndexId, (int)nNumIndices, (int)nFirstVertId, (int)nNumVerts);
            pMesh->SetSubsetMaterialId(i, nMatID == -1 ? 0 : (int)nMatID);
            pMesh->SetSubsetMaterialProperties(i, (int)nMatFlags, (int)nPhysicalizeType, eVF_P3F_C4B_T2F);
        }

        if (nVersion == 0)
        {
#if defined(WIN32) || defined(WIN64)
            pMesh->Optimize();
#endif
        }

        statObjList[k]->Invalidate(true);
    }

    return pStatObj;
}

float C3DEngine::GetWaterLevel(const Vec3* pvPos, IPhysicalEntity* pent, bool bAccurate)
{
    FUNCTION_PROFILER_3DENGINE;
    if (!OceanGlobals::g_oceanParamsMutex.try_lock())
    {
        return OceanGlobals::g_oceanLevel;
    }

    bool bInVisarea = m_pVisAreaManager && m_pVisAreaManager->GetVisAreaFromPos(*pvPos) != 0;


    AZ_UNUSED(pent);

    float max_level = (!bInVisarea) ? (bAccurate ? GetAccurateOceanHeight(*pvPos) : GetWaterLevel()) : WATER_LEVEL_UNKNOWN;


    OceanGlobals::g_oceanParamsMutex.unlock();
    return max(WATER_LEVEL_UNKNOWN, max_level);
}

void C3DEngine::SetShadowsGSMCache(bool bCache)
{
    if (bCache)
    {
        m_nGsmCache = m_pConsole->GetCVar("r_ShadowsCache")->GetIVal();
    }
    else
    {
        m_nGsmCache = 0;
    }
}

float C3DEngine::GetAccurateOceanHeight(const Vec3& pCurrPos) const
{
    FUNCTION_PROFILER_3DENGINE;

    static int nFrameID = -1;
    const int nEngineFrameID = GetRenderer()->GetFrameID();
    static float fWaterLevel = 0;
    if (nFrameID != nEngineFrameID && m_pOcean)
    {
        fWaterLevel = OceanToggle::IsActive() ? OceanRequest::GetOceanLevel() : m_pOcean->GetWaterLevel();
        nFrameID = nEngineFrameID;
    }

    const float waterLevel = fWaterLevel + COcean::GetWave(pCurrPos, GetRenderer()->GetFrameID());
    return waterLevel;
}

I3DEngine::CausticsParams C3DEngine::GetCausticsParams() const
{
    I3DEngine::CausticsParams params;
    if (OceanToggle::IsActive())
    {
        params.tiling = OceanRequest::GetCausticsTiling();
        params.distanceAttenuation = OceanRequest::GetCausticsDistanceAttenuation();
        params.depth = OceanRequest::GetCausticsDepth();
        params.intensity = OceanRequest::GetCausticsIntensity();
    }
    else
    {
        params.tiling = m_oceanCausticsTiling;
        params.distanceAttenuation = m_oceanCausticsDistanceAtten;
        params.depth = m_oceanCausticDepth;
        params.intensity = m_oceanCausticIntensity;
    }
    params.height = 0.0f; // @TODO: (@pruiksma) Parameter not currently accessible, need to decide whether to rip out or start using. [LY-62048]
    return params;
}

void C3DEngine::GetHDRSetupParams(Vec4 pParams[5]) const
{
    float hdrBloomAmount;
    GetPostEffectParam("Global_User_HDRBloom", hdrBloomAmount);
    pParams[0] = m_vHDRFilmCurveParams;
    pParams[1] = Vec4(hdrBloomAmount * 0.3f, hdrBloomAmount * 0.3f, hdrBloomAmount * 0.3f, m_fGrainAmount);
    pParams[2] = Vec4(m_vColorBalance, m_fHDRSaturation);
    pParams[3] = Vec4(m_vHDREyeAdaptation, 1.0f);
    pParams[4] = Vec4(m_vHDREyeAdaptationLegacy, 1.0f);
};


I3DEngine::OceanAnimationData C3DEngine::GetOceanAnimationParams() const
{
    I3DEngine::OceanAnimationData data;
    if (OceanToggle::IsActive())
    {
        data.fWavesAmount = OceanRequest::GetWavesAmount();
        data.fWavesSize = OceanRequest::GetWavesSize();
        data.fWavesSpeed = OceanRequest::GetWavesSpeed();
        data.fWindDirection = OceanRequest::GetWindDirection();
        data.fWindSpeed = OceanRequest::GetWindSpeed();
    }
    else
    {
        data.fWavesAmount = m_oceanWavesAmount;
        data.fWavesSize = m_oceanWavesSize;
        data.fWavesSpeed = m_oceanWavesSpeed;
        data.fWindDirection = m_oceanWindDirection;
        data.fWindSpeed = m_oceanWindSpeed;
    }

    sincos_tpl(data.fWindDirection, &data.fWindDirectionU, &data.fWindDirectionV);
    return data;
}

IVisArea* C3DEngine::CreateVisArea(uint64 visGUID)
{
    return m_pObjManager ? m_pVisAreaManager->CreateVisArea(visGUID) : 0;
}

void C3DEngine::DeleteVisArea(IVisArea* pVisArea)
{
    if (!m_pVisAreaManager->IsValidVisAreaPointer((CVisArea*)pVisArea))
    {
        Warning("I3DEngine::DeleteVisArea: Invalid VisArea pointer");
        return;
    }
    if (m_pObjManager)
    {
        CVisArea* pArea = (CVisArea*)pVisArea;

        PodArray<SRNInfo> lstEntitiesInArea;
        if (pArea->m_pObjectsTree)
        {
            pArea->m_pObjectsTree->MoveObjectsIntoList(&lstEntitiesInArea, NULL);
        }

        // unregister from indoor
        for (int i = 0; i < lstEntitiesInArea.Count(); i++)
        {
            Get3DEngine()->UnRegisterEntityDirect(lstEntitiesInArea[i].pNode);
        }

        if (pArea->m_pObjectsTree)
        {
            assert(pArea->m_pObjectsTree->GetObjectsCount(eMain) == 0);
        }

        m_pVisAreaManager->DeleteVisArea((CVisArea*)pVisArea);

        for (int i = 0; i < lstEntitiesInArea.Count(); i++)
        {
            Get3DEngine()->RegisterEntity(lstEntitiesInArea[i].pNode);
        }
    }
}

void C3DEngine::UpdateVisArea(IVisArea* pVisArea, const Vec3* pPoints, int nCount, const char* szName,
    const SVisAreaInfo& info, bool bReregisterObjects)
{
    if (!m_pObjManager)
    {
        return;
    }

    CVisArea* pArea = (CVisArea*)pVisArea;

    //  PrintMessage("C3DEngine::UpdateVisArea: %s", szName);

    Vec3 vTotalBoxMin = pArea->m_boxArea.min;
    Vec3 vTotalBoxMax = pArea->m_boxArea.max;

    m_pVisAreaManager->UpdateVisArea((CVisArea*)pVisArea, pPoints, nCount, szName, info);

    if (((CVisArea*)pVisArea)->m_pObjectsTree && ((CVisArea*)pVisArea)->m_pObjectsTree->GetObjectsCount(eMain))
    {
        // merge old and new bboxes
        vTotalBoxMin.CheckMin(pArea->m_boxArea.min);
        vTotalBoxMax.CheckMax(pArea->m_boxArea.max);
    }
    else
    {
        vTotalBoxMin = pArea->m_boxArea.min;
        vTotalBoxMax = pArea->m_boxArea.max;
    }

    if (bReregisterObjects)
    {
        m_pObjManager->ReregisterEntitiesInArea(vTotalBoxMin - Vec3(8, 8, 8), vTotalBoxMax + Vec3(8, 8, 8));
    }
}

IClipVolume* C3DEngine::CreateClipVolume()
{
    return m_pClipVolumeManager->CreateClipVolume();
}

void C3DEngine::DeleteClipVolume(IClipVolume* pClipVolume)
{
    m_pClipVolumeManager->DeleteClipVolume(pClipVolume);
}

void C3DEngine::UpdateClipVolume(IClipVolume* pClipVolume, _smart_ptr<IRenderMesh> pRenderMesh, IBSPTree3D* pBspTree, const Matrix34& worldTM, bool bActive, uint32 flags, const char* szName)
{
    m_pClipVolumeManager->UpdateClipVolume(pClipVolume, pRenderMesh, pBspTree, worldTM, bActive, flags, szName);
}

void C3DEngine::ResetParticlesAndDecals()
{
    if (m_pDecalManager)
    {
        m_pDecalManager->Reset();
    }
}

IRenderNode* C3DEngine::CreateRenderNode(EERType type)
{
    switch (type)
    {
    case eERType_Cloud:
    {
        CCloudRenderNode* pCloud = new CCloudRenderNode();
        return pCloud;
    }
    case eERType_FogVolume:
    {
        CFogVolumeRenderNode* pFogVolume = new CFogVolumeRenderNode();
        return pFogVolume;
    }
    case eERType_Decal:
    {
        CDecalRenderNode* pDecal = new CDecalRenderNode();
        return pDecal;
    }
    case eERType_WaterVolume:
    {
        CWaterVolumeRenderNode* pWaterVolume = new CWaterVolumeRenderNode();
        return pWaterVolume;
    }
    case eERType_DistanceCloud:
    {
        CDistanceCloudRenderNode* pRenderNode = new CDistanceCloudRenderNode();
        return pRenderNode;
    }
    case eERType_VolumeObject:
    {
        IVolumeObjectRenderNode* pRenderNode = new CVolumeObjectRenderNode();
        return pRenderNode;
    }
#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
    case eERType_PrismObject:
    {
        IPrismRenderNode* pRenderNode = new CPrismRenderNode();
        return pRenderNode;
    }
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

#if defined(USE_GEOM_CACHES)
    case eERType_GeomCache:
    {
        IRenderNode* pRenderNode = new CGeomCacheRenderNode();
        return pRenderNode;
    }
#endif
    }
    assert(!"C3DEngine::CreateRenderNode: Specified node type is not supported.");
    return 0;
}

void C3DEngine::DeleteRenderNode(IRenderNode* pRenderNode)
{
    UnRegisterEntityDirect(pRenderNode);
    delete pRenderNode;
}


Vec3 C3DEngine::GetWind(const AABB& box, bool bIndoors) const
{
    FUNCTION_PROFILER_3DENGINE;

#if defined(CONSOLE_CONST_CVAR_MODE)
    if constexpr (!CVars::e_Wind)
#else
    if (!m_pCVars->e_Wind)
#endif
    {
        return Vec3_Zero;
    }

    // Start with global wind.
    Vec3 vWind = GetGlobalWind(bIndoors);

#if defined(CONSOLE_CONST_CVAR_MODE)
    if constexpr (CVars::e_WindAreas)
#else
    if (m_pCVars->e_WindAreas)
#endif
    {
        const Physics::WindRequests* windRequests = AZ::Interface<Physics::WindRequests>::Get();
        if (windRequests)
        {
            const AZ::Aabb aabb = LyAABBToAZAabb(box);
            const AZ::Vector3 wind = windRequests->GetWind(aabb);
            vWind += AZVec3ToLYVec3(wind);
        }
    }

    return vWind;
}

Vec3 C3DEngine::GetGlobalWind(bool bIndoors) const
{
    FUNCTION_PROFILER_3DENGINE;

    // We assume indoor wind is zero.
    if (!m_pCVars->e_Wind || bIndoors)
    {
        return Vec3_Zero;
    }

    const Physics::WindRequests* windRequests = AZ::Interface<Physics::WindRequests>::Get();
    if (windRequests)
    {
        const AZ::Vector3 wind = windRequests->GetGlobalWind();
        return AZVec3ToLYVec3(wind);
    }
    return Vec3_Zero;
}

bool C3DEngine::SampleWind(Vec3* pSamples, int nSamples, const AABB& volume, bool bIndoors) const
{
    FUNCTION_PROFILER_3DENGINE;

    if (!m_pCVars->e_Wind || nSamples == 0)
    {
        return false;
    }

    IF (m_nWindSamplePositions < (size_t)nSamples, 0)
    {
        if (m_pWindSamplePositions)
        {
            CryModuleMemalignFree(m_pWindSamplePositions);
        }
        m_nWindSamplePositions = nSamples;
        m_pWindSamplePositions = (Vec3*)CryModuleMemalign(m_nWindSamplePositions * sizeof(Vec3), 128u);
    }

    Vec3* pPositions = m_pWindSamplePositions;
    memcpy(pPositions, pSamples, sizeof(Vec3) * nSamples);

    // Start with global wind.
    Vec3 vWind = GetGlobalWind(bIndoors);

    for (int i = 0; i < nSamples; ++i)
    {
        pSamples[i] = vWind;
    }

#if defined(CONSOLE_CONST_CVAR_MODE)
    if constexpr (CVars::e_WindAreas)
#else
    if (m_pCVars->e_WindAreas)
#endif
    {
        const Physics::WindRequests* windRequests = AZ::Interface<Physics::WindRequests>::Get();
        if (windRequests)
        {
            for (int i = 0; i < nSamples; ++i)
            {
                const AZ::Vector3 position = LYVec3ToAZVec3(pPositions[i]);
                const AZ::Vector3 wind = windRequests->GetWind(position);
                pSamples[i] += AZVec3ToLYVec3(wind);
            }
        }
    }
    AZ_UNUSED(volume);
    return true;
}

void C3DEngine::SetupBending(CRenderObject*& pObj, const IRenderNode* pNode, const float fRadiusVert, const SRenderingPassInfo& passInfo, bool alreadyDuplicated)
{
    FUNCTION_PROFILER_3DENGINE;
    if (!GetCVars()->e_VegetationBending)
    {
        return;
    }

    if (!pNode->m_pRNTmpData)
    {
        return;
    }

    // Get/Update PhysAreaChanged Proxy
    {
        // Lock to avoid a situation where two threads simultaneously find that nProxId is ~0,
        // which would result in two physics proxies for the same render node which eventually leads to a crash
        AUTO_LOCK(m_PhysicsAreaUpdates.m_Mutex);
        const uint32 nProxyId = pNode->m_pRNTmpData->nPhysAreaChangedProxyId;
        if (nProxyId != ~0)
        {
            m_PhysicsAreaUpdates.UpdateProxy(pNode, nProxyId);
        }
        else
        {
            pNode->m_pRNTmpData->nPhysAreaChangedProxyId = m_PhysicsAreaUpdates.CreateProxy(pNode, Area_Air);
        }
    }

    CRNTmpData::SRNUserData& userData = pNode->m_pRNTmpData->userData;
    const bool needsUpdate = userData.nBendingLastFrame != (passInfo.GetMainFrameID() & ~(3 << 29));
    const bool isFirstFrame = userData.nBendingLastFrame == 0;

    const Vec3& vObjPos = pObj->GetTranslation();
    const float fMaxViewDist = const_cast<IRenderNode*>(pNode)->GetMaxViewDist();
    const float fBendingAttenuation = 1.f - (pObj->m_fDistance / (fMaxViewDist * GetFloatCVar(e_WindBendingDistRatio)));
    uint32 bendingMaskOr = 0u;
    uint32 bendingMaskAnd = ~FOB_BENDED;

    if (fBendingAttenuation > 0.f && userData.m_Bending.m_fMainBendingScale > 0.f)
    {
        bendingMaskOr = FOB_BENDED;
        bendingMaskAnd = ~0u;

        userData.m_BendingPrev = userData.m_Bending;

        if (needsUpdate)
        {
            userData.nBendingLastFrame = passInfo.GetMainFrameID();

            static const float fBEND_RESPONSE = 0.25f;
            static const float fMAX_BENDING = 2.f;
            static const float fWAVE_PARALLEL = 0.008f;
            static const float fWAVE_TRANSVERSE = 0.002f;

            if (!userData.bWindCurrent)
            {
                userData.vCurrentWind = m_p3DEngine->GetWind(pNode->GetBBox(), !!pNode->GetEntityVisArea());
                userData.bWindCurrent = true;
            }

            // Soft clamp bending from wind amplitude.
            Vec2 vBending = Vec2(userData.vCurrentWind) * fBEND_RESPONSE;
            vBending *= fMAX_BENDING / (fMAX_BENDING + vBending.GetLength());
            vBending *= fBendingAttenuation;

            SBending* pBending = &userData.m_Bending;

            float fWaveFreq = 0.4f / (fRadiusVert + 1.f) + 0.2f;

            if (!userData.bBendingSet)
            {
                // First time shown, set full bending.
                pBending->m_vBending = vBending;
                userData.bBendingSet = true;
            }
            else
            {
                // Already visible, fade toward current value.
                float fInterp = min(gEnv->pTimer->GetFrameTime() * fWaveFreq * 0.5f, 1.f);
                pBending->m_vBending += (vBending - pBending->m_vBending) * fInterp;
            }

            pBending->m_Waves[0].m_Level  = 0.000f;
            pBending->m_Waves[0].m_Freq   = fWaveFreq;
            pBending->m_Waves[0].m_Phase  = vObjPos.x * 0.125f;
            pBending->m_Waves[0].m_Amp    = pBending->m_vBending.x * fWAVE_PARALLEL + pBending->m_vBending.y * fWAVE_TRANSVERSE;
            pBending->m_Waves[0].m_eWFType = eWF_Sin;

            pBending->m_Waves[1].m_Level  = 0.000f;
            pBending->m_Waves[1].m_Freq   = fWaveFreq * 1.125f;
            pBending->m_Waves[1].m_Phase  = vObjPos.y * 0.125f;
            pBending->m_Waves[1].m_Amp    = pBending->m_vBending.y * fWAVE_PARALLEL - pBending->m_vBending.x * fWAVE_TRANSVERSE;
            pBending->m_Waves[1].m_eWFType = eWF_Sin;
        }

        // When starting fresh, we use the same bend info for previous so
        // that we don't get crazy motion changes.
        if (isFirstFrame)
        {
            userData.m_BendingPrev = userData.m_Bending;
        }
    }

    if (!alreadyDuplicated)
    {
        pObj = GetRenderer()->EF_DuplicateRO(pObj, passInfo);
    }
    SRenderObjData* pOD = GetRenderer()->EF_GetObjData(pObj, true, passInfo.ThreadID());
    if (!pOD)
    {
        return;
    }

    pObj->m_ObjFlags |= bendingMaskOr | FOB_DYNAMIC_OBJECT | FOB_MOTION_BLUR;
    pObj->m_ObjFlags &= bendingMaskAnd;
    pOD->m_pBending = Get3DEngine()->GetBendingEntry(&userData.m_Bending, passInfo);
    pOD->m_BendingPrev = Get3DEngine()->GetBendingEntry(&userData.m_BendingPrev, passInfo);
}

IVisArea* C3DEngine::GetVisAreaFromPos(const Vec3& vPos)
{
    if (m_pObjManager && m_pVisAreaManager)
    {
        return m_pVisAreaManager->GetVisAreaFromPos(vPos);
    }

    return 0;
}

bool C3DEngine::IntersectsVisAreas(const AABB& box, void** pNodeCache)
{
    if (m_pObjManager && m_pVisAreaManager)
    {
        return m_pVisAreaManager->IntersectsVisAreas(box, pNodeCache);
    }
    return false;
}

bool C3DEngine::ClipToVisAreas(IVisArea* pInside, Sphere& sphere, Vec3 const& vNormal, void* pNodeCache)
{
    if (pInside)
    {
        return pInside->ClipToVisArea(true, sphere, vNormal);
    }
    else if (m_pVisAreaManager)
    {
        return m_pVisAreaManager->ClipOutsideVisAreas(sphere, vNormal, pNodeCache);
    }
    return false;
}

bool C3DEngine::IsVisAreasConnected(IVisArea* pArea1, IVisArea* pArea2, int nMaxRecursion, bool bSkipDisabledPortals)
{
    if (pArea1 == pArea2)
    {
        return (true);  // includes the case when both pArea1 & pArea2 are NULL (totally outside)
    }
    // not considered by the other checks
    if (!pArea1 || !pArea2)
    {
        return (false); // avoid a crash - better to put this check only
    }
    // here in one place than in all the places where this function is called

    nMaxRecursion *= 2; // include portals since portals are the areas

    if (m_pObjManager && m_pVisAreaManager)
    {
        return ((CVisArea*)pArea1)->FindVisArea((CVisArea*)pArea2, nMaxRecursion, bSkipDisabledPortals);
    }

    return false;
}

bool C3DEngine::IsOutdoorVisible()
{
    if (m_pObjManager && m_pVisAreaManager)
    {
        return m_pVisAreaManager->IsOutdoorAreasVisible();
    }

    return false;
}

void C3DEngine::EnableOceanRendering(bool bOcean)
{
    m_bOcean = bOcean;
}

IObjManager* C3DEngine::GetObjManager()
{
    return Cry3DEngineBase::GetObjManager();
}

//////////////////////////////////////////////////////////////////////////
IMaterialHelpers& C3DEngine::GetMaterialHelpers()
{
    return Cry3DEngineBase::GetMatMan()->s_materialHelpers;
}

IMaterialManager* C3DEngine::GetMaterialManager()
{
    return Cry3DEngineBase::GetMatMan();
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::AddTextureLoadHandler(ITextureLoadHandler* pHandler)
{
    stl::push_back_unique(m_textureLoadHandlers, pHandler);
}

void C3DEngine::RemoveTextureLoadHandler(ITextureLoadHandler* pHandler)
{
    stl::find_and_erase(m_textureLoadHandlers, pHandler);
}

ITextureLoadHandler* C3DEngine::GetTextureLoadHandlerForImage(const char* path)
{
    const char* ext = PathUtil::GetExt(path);

    for (TTextureLoadHandlers::iterator iter = m_textureLoadHandlers.begin(); iter != m_textureLoadHandlers.end(); iter++)
    {
        if ((*iter)->SupportsExtension(ext))
        {
            return *iter;
        }
    }

    return nullptr;
}

void C3DEngine::CheckMemoryHeap()
{
    assert (CryMemory::IsHeapValid());
}

//////////////////////////////////////////////////////////////////////////
int C3DEngine::GetLoadedObjectCount()
{
    int nObjectsLoaded = m_pObjManager ? m_pObjManager->GetLoadedObjectCount() : 0;
    return nObjectsLoaded;
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::GetLoadedStatObjArray(IStatObj** pObjectsArray, int& nCount)
{
    if (m_pObjManager)
    {
        m_pObjManager->GetLoadedStatObjArray(pObjectsArray, nCount);
    }
    else
    {
        nCount = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::GetObjectsStreamingStatus(SObjectsStreamingStatus& outStatus)
{
    if (m_pObjManager)
    {
        m_pObjManager->GetObjectsStreamingStatus(outStatus);
    }
    else
    {
        memset(&outStatus, 0, sizeof(outStatus));
    }
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::GetStreamingSubsystemData(int subsystem, SStremaingBandwidthData& outData)
{
    switch (subsystem)
    {
    case eStreamTaskTypeSound:
        // Audio: bandwidth stats
        break;
    case eStreamTaskTypeGeometry:
        m_pObjManager->GetBandwidthStats(&outData.fBandwidthRequested);
        break;
    case eStreamTaskTypeTexture:
        gEnv->pRenderer->GetBandwidthStats(&outData.fBandwidthRequested);
        break;
    }

#if defined(STREAMENGINE_ENABLE_STATS)
    gEnv->pSystem->GetStreamEngine()->GetBandwidthStats((EStreamTaskType)subsystem, &outData.fBandwidthActual);
#endif
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::DeleteEntityDecals(IRenderNode* pEntity)
{
    if (m_pDecalManager && pEntity && (pEntity->m_nInternalFlags & IRenderNode::DECAL_OWNER))
    {
        m_pDecalManager->OnEntityDeleted(pEntity);
    }
}

void C3DEngine::DeleteDecalsInRange(AABB* pAreaBox, IRenderNode* pEntity)
{
    if (m_pDecalManager)
    {
        m_pDecalManager->DeleteDecalsInRange(pAreaBox, pEntity);
    }
}

void C3DEngine::LockCGFResources()
{
    if (m_pObjManager)
    {
        m_pObjManager->SetLockCGFResources(true);
    }
}

void C3DEngine::UnlockCGFResources()
{
    if (m_pObjManager)
    {
        bool bNeedToFreeCGFs = m_pObjManager->IsLockCGFResources();
        m_pObjManager->SetLockCGFResources(false);
        if (bNeedToFreeCGFs)
        {
            m_pObjManager->FreeNotUsedCGFs();
        }
    }
}

void C3DEngine::FreeUnusedCGFResources()
{
    if (m_pObjManager)
    {
        m_pObjManager->FreeNotUsedCGFs();
    }
}

void CLightEntity::ShadowMapInfo::Release([[maybe_unused]] struct IRenderer* pRenderer)
{
    delete this;
}

//////////////////////////////////////////////////////////////////////////
IIndexedMesh* C3DEngine::CreateIndexedMesh()
{
    return new CIndexedMesh();
}

void C3DEngine::SerializeState(TSerialize ser)
{
    ser.Value("m_bOcean", m_bOcean);
    ser.Value("m_moonRotationLatitude", m_moonRotationLatitude);
    ser.Value("m_moonRotationLongitude", m_moonRotationLongitude);
    ser.Value("m_eShadowMode", *(alias_cast<int*>(&m_eShadowMode)));
    if (ser.IsReading())
    {
        UpdateMoonDirection();
    }

    if (m_pDecalManager)
    {
        m_pDecalManager->Serialize(ser);
    }

    m_pTimeOfDay->Serialize(ser);
}

void C3DEngine::PostSerialize(bool /*bReading*/)
{
}

void C3DEngine::InitMaterialDefautMappingAxis(_smart_ptr<IMaterial> pMat)
{
    uint8 arrProj[3] = {'X', 'Y', 'Z'};
    pMat->m_ucDefautMappingAxis = 'Z';
    pMat->m_fDefautMappingScale = 1.f;
    for (int c = 0; c < 3 && c < pMat->GetSubMtlCount(); c++)
    {
        _smart_ptr<IMaterial> pSubMat = (_smart_ptr<IMaterial>)pMat->GetSubMtl(c);
        pSubMat->m_ucDefautMappingAxis = arrProj[c];
        pSubMat->m_fDefautMappingScale = pMat->m_fDefautMappingScale;
    }
}

//////////////////////////////////////////////////////////////////////////
CContentCGF* C3DEngine::CreateChunkfileContent(const char* filename)
{
    return new CContentCGF(filename);
}
void C3DEngine::ReleaseChunkfileContent(CContentCGF* pCGF)
{
    delete pCGF;
}

bool C3DEngine::LoadChunkFileContent(CContentCGF* pCGF, const char* filename, bool bNoWarningMode, bool bCopyChunkFile)
{
    CLoadLogListener listener;

    if (!pCGF)
    {
        FileWarning(0, filename, "CGF Loading Failed: no content instance passed");
    }
    else
    {
        CLoaderCGF cgf;

        CReadOnlyChunkFile* pChunkFile = new CReadOnlyChunkFile(bCopyChunkFile, bNoWarningMode);

        if (cgf.LoadCGF(pCGF, filename, *pChunkFile, &listener))
        {
            pCGF->SetChunkFile(pChunkFile);
            return true;
        }

        CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "%s: Failed to load chunk file: '%s'", __FUNCTION__, cgf.GetLastError());
        delete pChunkFile;
    }

    return false;
}

bool C3DEngine::LoadChunkFileContentFromMem(CContentCGF* pCGF, const void* pData, size_t nDataLen, uint32 nLoadingFlags, bool bNoWarningMode, bool bCopyChunkFile)
{
    if (!pCGF)
    {
        FileWarning(0, "<memory>", "CGF Loading Failed: no content instance passed");
    }
    else
    {
        CLoadLogListener listener;

        CLoaderCGF cgf;
        CReadOnlyChunkFile* pChunkFile = new CReadOnlyChunkFile(bCopyChunkFile, bNoWarningMode);

        if (cgf.LoadCGFFromMem(pCGF, pData, nDataLen, *pChunkFile, &listener, nLoadingFlags))
        {
            pCGF->SetChunkFile(pChunkFile);
            return true;
        }

        CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "%s: Failed to load chunk file: '%s'", __FUNCTION__, cgf.GetLastError());
        delete pChunkFile;
    }

    return false;
}

IChunkFile* C3DEngine::CreateChunkFile(bool bReadOnly)
{
    if (bReadOnly)
    {
        return new CReadOnlyChunkFile(false);
    }
    else
    {
        return new CChunkFile;
    }
}

//////////////////////////////////////////////////////////////////////////

ChunkFile::IChunkFileWriter* C3DEngine::CreateChunkFileWriter(EChunkFileFormat eFormat, AZ::IO::IArchive* pPak, const char* filename) const
{
    ChunkFile::CryPakFileWriter* const p = new ChunkFile::CryPakFileWriter();

    if (!p)
    {
        return 0;
    }

    if (!p->Create(pPak, filename))
    {
        delete p;
        return 0;
    }

    const ChunkFile::MemorylessChunkFileWriter::EChunkFileFormat fmt =
        (eFormat == I3DEngine::eChunkFileFormat_0x745)
        ? ChunkFile::MemorylessChunkFileWriter::eChunkFileFormat_0x745
        : ChunkFile::MemorylessChunkFileWriter::eChunkFileFormat_0x746;

    return new ChunkFile::MemorylessChunkFileWriter(fmt, p);
}

void C3DEngine::ReleaseChunkFileWriter(ChunkFile::IChunkFileWriter* p) const
{
    if (p)
    {
        delete p->GetWriter();
        delete p;
    }
}

//////////////////////////////////////////////////////////////////////////

bool C3DEngine::CreateOcean(_smart_ptr<IMaterial> pTerrainWaterMat, float waterLevel)
{
    // make ocean surface
    SAFE_DELETE(m_pOcean);

    if (pTerrainWaterMat == nullptr)
    {
        if (!m_pTerrainWaterMat)
        {
            return false;
        }

        pTerrainWaterMat = m_pTerrainWaterMat;
    }

    if (OceanToggle::IsActive() ? OceanRequest::OceanIsEnabled() : (waterLevel > 0))
    {
        m_pOcean = new COcean(pTerrainWaterMat, waterLevel);
    }
    return m_pOcean != nullptr;
}

void C3DEngine::DeleteOcean()
{
    SAFE_DELETE(m_pOcean);
}

void C3DEngine::ChangeOceanMaterial(_smart_ptr<IMaterial> pMat)
{
    if (m_pOcean)
    {
        m_pOcean->SetMaterial(pMat);
    }
}

void C3DEngine::ChangeOceanWaterLevel(float fWaterLevel)
{
    COcean::SetWaterLevelInfo(fWaterLevel);

    if (m_pOcean)
    {
        m_pOcean->SetWaterLevel(fWaterLevel);
    }
}

void C3DEngine::SetStreamableListener(IStreamedObjectListener* pListener)
{
    m_pStreamListener = pListener;
}

void C3DEngine::PrecacheLevel(bool bPrecacheAllVisAreas, Vec3* pPrecachePoints, int nPrecachePointsNum)
{
    LOADING_TIME_PROFILE_SECTION;

    if (GetVisAreaManager())
    {
        GetVisAreaManager()->PrecacheLevel(bPrecacheAllVisAreas, pPrecachePoints, nPrecachePointsNum);
    }
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::SetGlobalParameter(E3DEngineParameter param, const Vec3& v)
{
    float fValue = v.x;
    switch (param)
    {
    case E3DPARAM_SUN_COLOR:
        SetSunColor(v);
        break;

    case E3DPARAM_SUN_SPECULAR_MULTIPLIER:
        m_fSunSpecMult = v.x;
        break;

    case E3DPARAM_AMBIENT_GROUND_COLOR:
        m_vAmbGroundCol = v;
        break;
    case E3DPARAM_AMBIENT_MIN_HEIGHT:
        m_fAmbMaxHeight = v.x;
        break;
    case E3DPARAM_AMBIENT_MAX_HEIGHT:
        m_fAmbMinHeight = v.x;
        break;

    case E3DPARAM_SKY_HIGHLIGHT_POS:
        m_vSkyHightlightPos = v;
        break;
    case E3DPARAM_SKY_HIGHLIGHT_COLOR:
        m_vSkyHightlightCol = v;
        break;
    case E3DPARAM_SKY_HIGHLIGHT_SIZE:
        m_fSkyHighlightSize = fValue > 0.0f ? fValue : 0.0f;
        break;
    case E3DPARAM_VOLFOG_RAMP:
        m_volFogRamp = v;
        break;
    case E3DPARAM_VOLFOG_SHADOW_RANGE:
        m_volFogShadowRange = v;
        break;
    case E3DPARAM_VOLFOG_SHADOW_DARKENING:
        m_volFogShadowDarkening = v;
        break;
    case E3DPARAM_VOLFOG_SHADOW_ENABLE:
        m_volFogShadowEnable = v;
        break;
    case E3DPARAM_VOLFOG2_CTRL_PARAMS:
        m_volFog2CtrlParams = v;
        break;
    case E3DPARAM_VOLFOG2_SCATTERING_PARAMS:
        m_volFog2ScatteringParams = v;
        break;
    case E3DPARAM_VOLFOG2_RAMP:
        m_volFog2Ramp = v;
        break;
    case E3DPARAM_VOLFOG2_COLOR:
        m_volFog2Color = v;
        break;
    case E3DPARAM_VOLFOG2_GLOBAL_DENSITY:
        m_volFog2GlobalDensity = v;
        break;
    case E3DPARAM_VOLFOG2_HEIGHT_DENSITY:
        m_volFog2HeightDensity = v;
        break;
    case E3DPARAM_VOLFOG2_HEIGHT_DENSITY2:
        m_volFog2HeightDensity2 = v;
        break;
    case E3DPARAM_VOLFOG2_COLOR1:
        m_volFog2Color1 = v;
        break;
    case E3DPARAM_VOLFOG2_COLOR2:
        m_volFog2Color2 = v;
        break;
    case E3DPARAM_NIGHSKY_HORIZON_COLOR:
        m_nightSkyHorizonCol = v;
        break;
    case E3DPARAM_NIGHSKY_ZENITH_COLOR:
        m_nightSkyZenithCol = v;
        break;
    case E3DPARAM_NIGHSKY_ZENITH_SHIFT:
        m_nightSkyZenithColShift = v.x;
        break;
    case E3DPARAM_NIGHSKY_STAR_INTENSITY:
        m_nightSkyStarIntensity = v.x;
        break;
    case E3DPARAM_NIGHSKY_MOON_COLOR:
        m_nightMoonCol = v;
        break;
    case E3DPARAM_NIGHSKY_MOON_SIZE:
        m_nightMoonSize = v.x;
        break;
    case E3DPARAM_NIGHSKY_MOON_INNERCORONA_COLOR:
        m_nightMoonInnerCoronaCol = v;
        break;
    case E3DPARAM_NIGHSKY_MOON_INNERCORONA_SCALE:
        m_nightMoonInnerCoronaScale = v.x;
        break;
    case E3DPARAM_NIGHSKY_MOON_OUTERCORONA_COLOR:
        m_nightMoonOuterCoronaCol = v;
        break;
    case E3DPARAM_NIGHSKY_MOON_OUTERCORONA_SCALE:
        m_nightMoonOuterCoronaScale = v.x;
        break;
    case E3DPARAM_OCEANFOG_COLOR:
        m_oceanFogColor = v;
        break;
    case E3DPARAM_OCEANFOG_DENSITY:
        m_oceanFogDensity = v.x;
        break;
    case E3DPARAM_SKY_MOONROTATION:
        m_moonRotationLatitude = v.x;
        m_moonRotationLongitude = v.y;
        UpdateMoonDirection();
        break;
    case E3DPARAM_SKYBOX_MULTIPLIER:
        m_skyboxMultiplier = v.x;
        break;
    case E3DPARAM_DAY_NIGHT_INDICATOR:
        m_dayNightIndicator = v.x;
        // Audio: Set daylight parameter
        break;
    case E3DPARAM_FOG_COLOR2:
        m_fogColor2 = v;
        break;
    case E3DPARAM_FOG_RADIAL_COLOR:
        m_fogColorRadial = v;
        break;
    case E3DPARAM_VOLFOG_HEIGHT_DENSITY:
        m_volFogHeightDensity = Vec3(v.x, v.y, 0);
        break;
    case E3DPARAM_VOLFOG_HEIGHT_DENSITY2:
        m_volFogHeightDensity2 = Vec3(v.x, v.y, 0);
        break;
    case E3DPARAM_VOLFOG_GRADIENT_CTRL:
        m_volFogGradientCtrl = v;
        break;
    case E3DPARAM_VOLFOG_GLOBAL_DENSITY:
        m_volFogGlobalDensity = v.x;
        m_volFogFinalDensityClamp = v.z;
        break;
    case E3DPARAM_COLORGRADING_FILTERS_PHOTOFILTER_COLOR:
        m_pPhotoFilterColor = Vec4(v.x, v.y, v.z, 1);
        GetPostEffectBaseGroup()->SetParam("clr_ColorGrading_PhotoFilterColor", m_pPhotoFilterColor);
        break;
    case E3DPARAM_COLORGRADING_FILTERS_PHOTOFILTER_DENSITY:
        m_fPhotoFilterColorDensity = fValue;
        GetPostEffectBaseGroup()->SetParam("ColorGrading_PhotoFilterColorDensity", m_fPhotoFilterColorDensity);
        break;
    case E3DPARAM_COLORGRADING_FILTERS_GRAIN:
        m_fGrainAmount = fValue;
        GetPostEffectBaseGroup()->SetParam("ColorGrading_GrainAmount", m_fGrainAmount);
        break;
    case E3DPARAM_SKY_SKYBOX_ANGLE: // sky box rotation
        m_fSkyBoxAngle = fValue;
        break;
    case E3DPARAM_SKY_SKYBOX_STRETCHING: // sky box stretching
        m_fSkyBoxStretching = fValue;
        break;
    case E3DPARAM_HDR_FILMCURVE_SHOULDER_SCALE:
        m_vHDRFilmCurveParams.x = v.x;
        break;
    case E3DPARAM_HDR_FILMCURVE_LINEAR_SCALE:
        m_vHDRFilmCurveParams.y = v.x;
        break;
    case E3DPARAM_HDR_FILMCURVE_TOE_SCALE:
        m_vHDRFilmCurveParams.z = v.x;
        break;
    case E3DPARAM_HDR_FILMCURVE_WHITEPOINT:
        m_vHDRFilmCurveParams.w = v.x;
        break;
    case E3DPARAM_HDR_EYEADAPTATION_PARAMS:
        m_vHDREyeAdaptation = v;
        break;
    case E3DPARAM_HDR_EYEADAPTATION_PARAMS_LEGACY:
        m_vHDREyeAdaptationLegacy = v;
        break;
    case E3DPARAM_HDR_BLOOM_AMOUNT:
        m_fHDRBloomAmount = v.x;
        break;
    case E3DPARAM_HDR_COLORGRADING_COLOR_SATURATION:
        m_fHDRSaturation = v.x;
        break;
    case E3DPARAM_HDR_COLORGRADING_COLOR_BALANCE:
        m_vColorBalance = v;
        break;
    case E3DPARAM_CLOUDSHADING_MULTIPLIERS:
        m_fCloudShadingSunLightMultiplier = (v.x >= 0) ? v.x : 0;
        m_fCloudShadingSkyLightMultiplier = (v.y >= 0) ? v.y : 0;
        break;
    case E3DPARAM_CLOUDSHADING_SUNCOLOR:
        m_vCloudShadingCustomSunColor = v;
        break;
    case E3DPARAM_CLOUDSHADING_SKYCOLOR:
        m_vCloudShadingCustomSkyColor = v;
        break;
    case E3DPARAM_NIGHSKY_MOON_DIRECTION: // moon direction is fixed per level or updated via FG node (E3DPARAM_SKY_MOONROTATION)
    default:
        assert(0);
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::GetGlobalParameter(E3DEngineParameter param, Vec3& v)
{
    switch (param)
    {
    case E3DPARAM_SUN_COLOR:
        v = GetSunColor();
        break;

    case E3DPARAM_SUN_SPECULAR_MULTIPLIER:
        v = Vec3(m_fSunSpecMult, 0, 0);
        break;

    case E3DPARAM_AMBIENT_GROUND_COLOR:
        v = m_vAmbGroundCol;
        break;
    case E3DPARAM_AMBIENT_MIN_HEIGHT:
        v = Vec3(m_fAmbMaxHeight, 0, 0);
        break;
    case E3DPARAM_AMBIENT_MAX_HEIGHT:
        v = Vec3(m_fAmbMinHeight, 0, 0);
        break;
    case E3DPARAM_SKY_HIGHLIGHT_POS:
        v = m_vSkyHightlightPos;
        break;
    case E3DPARAM_SKY_HIGHLIGHT_COLOR:
        v = m_vSkyHightlightCol;
        break;
    case E3DPARAM_SKY_HIGHLIGHT_SIZE:
        v = Vec3(m_fSkyHighlightSize, 0, 0);
        break;
    case E3DPARAM_VOLFOG_RAMP:
        v = m_volFogRamp;
        break;
    case E3DPARAM_VOLFOG_SHADOW_RANGE:
        v = m_volFogShadowRange;
        break;
    case E3DPARAM_VOLFOG_SHADOW_DARKENING:
        v = m_volFogShadowDarkening;
        break;
    case E3DPARAM_VOLFOG_SHADOW_ENABLE:
        v = m_volFogShadowEnable;
        break;
    case E3DPARAM_VOLFOG2_CTRL_PARAMS:
        v = m_volFog2CtrlParams;
        break;
    case E3DPARAM_VOLFOG2_SCATTERING_PARAMS:
        v = m_volFog2ScatteringParams;
        break;
    case E3DPARAM_VOLFOG2_RAMP:
        v = m_volFog2Ramp;
        break;
    case E3DPARAM_VOLFOG2_COLOR:
        v = m_volFog2Color;
        break;
    case E3DPARAM_VOLFOG2_GLOBAL_DENSITY:
        v = m_volFog2GlobalDensity;
        break;
    case E3DPARAM_VOLFOG2_HEIGHT_DENSITY:
        v = m_volFog2HeightDensity;
        break;
    case E3DPARAM_VOLFOG2_HEIGHT_DENSITY2:
        v = m_volFog2HeightDensity2;
        break;
    case E3DPARAM_VOLFOG2_COLOR1:
        v = m_volFog2Color1;
        break;
    case E3DPARAM_VOLFOG2_COLOR2:
        v = m_volFog2Color2;
        break;
    case E3DPARAM_NIGHSKY_HORIZON_COLOR:
        v = m_nightSkyHorizonCol;
        break;
    case E3DPARAM_NIGHSKY_ZENITH_COLOR:
        v = m_nightSkyZenithCol;
        break;
    case E3DPARAM_NIGHSKY_ZENITH_SHIFT:
        v = Vec3(m_nightSkyZenithColShift, 0, 0);
        break;
    case E3DPARAM_NIGHSKY_STAR_INTENSITY:
        v = Vec3(m_nightSkyStarIntensity, 0, 0);
        break;
    case E3DPARAM_NIGHSKY_MOON_DIRECTION:
        v = m_moonDirection;
        break;
    case E3DPARAM_NIGHSKY_MOON_COLOR:
        v = m_nightMoonCol;
        break;
    case E3DPARAM_NIGHSKY_MOON_SIZE:
        v = Vec3(m_nightMoonSize, 0, 0);
        break;
    case E3DPARAM_NIGHSKY_MOON_INNERCORONA_COLOR:
        v = m_nightMoonInnerCoronaCol;
        break;
    case E3DPARAM_NIGHSKY_MOON_INNERCORONA_SCALE:
        v = Vec3(m_nightMoonInnerCoronaScale, 0, 0);
        break;
    case E3DPARAM_NIGHSKY_MOON_OUTERCORONA_COLOR:
        v = m_nightMoonOuterCoronaCol;
        break;
    case E3DPARAM_NIGHSKY_MOON_OUTERCORONA_SCALE:
        v = Vec3(m_nightMoonOuterCoronaScale, 0, 0);
        break;
    case E3DPARAM_SKY_MOONROTATION:
        v = Vec3(m_moonRotationLatitude, m_moonRotationLongitude, 0);
        break;
    case E3DPARAM_OCEANFOG_COLOR:
        v = m_oceanFogColor;
        break;
    case E3DPARAM_OCEANFOG_DENSITY:
        v = Vec3(m_oceanFogDensity, 0, 0);
        break;
    case E3DPARAM_SKYBOX_MULTIPLIER:
        v = Vec3(m_skyboxMultiplier, 0, 0);
        break;
    case E3DPARAM_DAY_NIGHT_INDICATOR:
        v = Vec3(m_dayNightIndicator, 0, 0);
        break;
    case E3DPARAM_FOG_COLOR2:
        v = m_fogColor2;
        break;
    case E3DPARAM_FOG_RADIAL_COLOR:
        v = m_fogColorRadial;
        break;
    case E3DPARAM_VOLFOG_HEIGHT_DENSITY:
        v = m_volFogHeightDensity;
        break;
    case E3DPARAM_VOLFOG_HEIGHT_DENSITY2:
        v = m_volFogHeightDensity2;
        break;
    case E3DPARAM_VOLFOG_GRADIENT_CTRL:
        v = m_volFogGradientCtrl;
        break;
    case E3DPARAM_VOLFOG_GLOBAL_DENSITY:
        v = Vec3(m_volFogGlobalDensity, m_volFogGlobalDensityMultiplierLDR, m_volFogFinalDensityClamp);
        break;
    case E3DPARAM_COLORGRADING_FILTERS_PHOTOFILTER_COLOR:
        v = Vec3(m_pPhotoFilterColor.x, m_pPhotoFilterColor.y, m_pPhotoFilterColor.z);
        break;
    case E3DPARAM_COLORGRADING_FILTERS_PHOTOFILTER_DENSITY:
        v = Vec3(m_fPhotoFilterColorDensity, 0, 0);
        break;
    case E3DPARAM_COLORGRADING_FILTERS_GRAIN:
        v = Vec3(m_fGrainAmount, 0, 0);
        break;
    case E3DPARAM_HDR_FILMCURVE_SHOULDER_SCALE:
        v = Vec3(m_vHDRFilmCurveParams.x, 0, 0);
        break;
    case E3DPARAM_HDR_FILMCURVE_LINEAR_SCALE:
        v = Vec3(m_vHDRFilmCurveParams.y, 0, 0);
        break;
    case E3DPARAM_HDR_FILMCURVE_TOE_SCALE:
        v = Vec3(m_vHDRFilmCurveParams.z, 0, 0);
        break;
    case E3DPARAM_HDR_FILMCURVE_WHITEPOINT:
        v = Vec3(m_vHDRFilmCurveParams.w, 0, 0);
        break;
    case E3DPARAM_HDR_EYEADAPTATION_PARAMS:
        v = m_vHDREyeAdaptation;
        break;
    case E3DPARAM_HDR_EYEADAPTATION_PARAMS_LEGACY:
        v = m_vHDREyeAdaptationLegacy;
        break;
    case E3DPARAM_HDR_BLOOM_AMOUNT:
        v = Vec3(m_fHDRBloomAmount, 0, 0);
        break;
    case E3DPARAM_HDR_COLORGRADING_COLOR_SATURATION:
        v = Vec3(m_fHDRSaturation, 0, 0);
        break;
    case E3DPARAM_HDR_COLORGRADING_COLOR_BALANCE:
        v = m_vColorBalance;
        break;
    case E3DPARAM_CLOUDSHADING_MULTIPLIERS:
        v = Vec3(m_fCloudShadingSunLightMultiplier, m_fCloudShadingSkyLightMultiplier, 0);
        break;
    case E3DPARAM_CLOUDSHADING_SUNCOLOR:
        v = m_vCloudShadingCustomSunColor;
        break;
    case E3DPARAM_CLOUDSHADING_SKYCOLOR:
        v = m_vCloudShadingCustomSkyColor;
        break;
    default:
        assert(0);
        break;
    }
}

//////////////////////////////////////////////////////////////////////////

void C3DEngine::SetCachedShadowBounds(const AABB& shadowBounds, float fAdditionalCascadesScale)
{
    const Vec3 boxSize = shadowBounds.GetSize();
    const bool isValid = boxSize.x > 0 && boxSize.y > 0 && boxSize.z > 0;

    m_CachedShadowsBounds = isValid ? shadowBounds : AABB(AABB::RESET);
    m_fCachedShadowsCascadeScale = fAdditionalCascadesScale;

    m_nCachedShadowsUpdateStrategy = ShadowMapFrustum::ShadowCacheData::eFullUpdate;
}

//////////////////////////////////////////////////////////////////////////

void C3DEngine::SetRecomputeCachedShadows(uint nUpdateStrategy)
{
    m_nCachedShadowsUpdateStrategy = nUpdateStrategy;
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::SetShadowsCascadesBias(const float* pCascadeConstBias, const float* pCascadeSlopeBias)
{
    memcpy(m_pShadowCascadeConstBias, pCascadeConstBias, sizeof(float) * MAX_SHADOW_CASCADES_NUM);
    memcpy(m_pShadowCascadeSlopeBias, pCascadeSlopeBias, sizeof(float) * MAX_SHADOW_CASCADES_NUM);
}

int C3DEngine::GetShadowsCascadeCount([[maybe_unused]] const CDLight* pLight) const
{
    int nCascadeCount = m_eShadowMode == ESM_HIGHQUALITY ? MAX_GSM_LODS_NUM : GetCVars()->e_GsmLodsNum;
    return clamp_tpl(nCascadeCount, 0, MAX_GSM_LODS_NUM);
}

//////////////////////////////////////////////////////////////////////////
bool C3DEngine::CheckIntersectClouds(const Vec3& p1, const Vec3& p2)
{
    return m_pCloudsManager->CheckIntersectClouds(p1, p2);
}

void C3DEngine::OnRenderMeshDeleted(IRenderMesh* pRenderMesh)
{
    if (m_pDecalManager)
    {
        m_pDecalManager->OnRenderMeshDeleted(pRenderMesh);
    }
}

bool C3DEngine::RayObjectsIntersection2D([[maybe_unused]] Vec3 vStart, [[maybe_unused]] Vec3 vEnd, [[maybe_unused]] Vec3& vHitPoint, [[maybe_unused]] EERType eERType)
{
#ifdef SUPPORT_TERRAIN_AO_PRE_COMPUTATIONS

    float fClosestHitDistance = 1000000;

    AABB aabb;
    aabb.Reset();
    aabb.Add(vStart);
    aabb.Add(vEnd);
    if (IsObjectTreeReady())
    {
        if (Overlap::AABB_AABB2D(aabb, m_pObjectsTree->GetNodeBox())) //!!!mpeykov: 2D?
        {
            m_pObjectsTree->RayObjectsIntersection2D(vStart, vEnd, vHitPoint, fClosestHitDistance, eERType);
        }
    }

    return (fClosestHitDistance < 1000000);

#else

    assert(!"C3DEngine::RayObjectsIntersection2D not supported on consoles");
    return 0;

#endif
}

bool C3DEngine::RenderMeshRayIntersection(IRenderMesh* pRenderMesh, SRayHitInfo& hitInfo, _smart_ptr<IMaterial> pCustomMtl)
{
    return CRenderMeshUtils::RayIntersection(pRenderMesh, hitInfo, pCustomMtl);
}

static SBending sBendRemoved;

void C3DEngine::FreeRNTmpData(CRNTmpData** ppInfo)
{
    AUTO_LOCK(m_checkCreateRNTmpData);

    CRNTmpData* pTmpRNTData = (*ppInfo);

    assert(pTmpRNTData->pNext != &m_LTPRootFree);
    assert(pTmpRNTData->pPrev != &m_LTPRootFree);
    assert(pTmpRNTData != &m_LTPRootUsed);
    if (gEnv->mMainThreadId != CryGetCurrentThreadId())
    {
        CryFatalError("CRNTmpData should only be allocated and free'd on main thread.");
    }

    if (pTmpRNTData != &m_LTPRootUsed)
    {
        pTmpRNTData->Unlink();
    }

    // Mark for phys area changed proxy for deletion
    if (pTmpRNTData->nPhysAreaChangedProxyId != ~0)
    {
        m_PhysicsAreaUpdates.ResetProxy(pTmpRNTData->nPhysAreaChangedProxyId);
        pTmpRNTData->nPhysAreaChangedProxyId = ~0;
    }

    if (pTmpRNTData->nFrameInfoId != ~0)
    {
        m_elementFrameInfo[pTmpRNTData->nFrameInfoId].Reset();
        pTmpRNTData->nFrameInfoId = ~0;
    }

    {
#ifdef SUPP_HWOBJ_OCCL
        if (pTmpRNTData->userData.m_OcclState.pREOcclusionQuery)
        {
            pTmpRNTData->userData.m_OcclState.pREOcclusionQuery->Release(false);
            pTmpRNTData->userData.m_OcclState.pREOcclusionQuery = NULL;
        }
#endif
    }

    if (pTmpRNTData != &m_LTPRootUsed)
    {
        pTmpRNTData->Link(&m_LTPRootFree);
        *(pTmpRNTData->pOwnerRef) = NULL;
    }
}

void C3DEngine::UpdateRNTmpDataPool(bool bFreeAll)
{
    // if we are freeing the whole pool, make sure no jobs are still running which could use the RNTmpObjects
    if (bFreeAll)
    {
        threadID nThreadID;
        gEnv->pRenderer->EF_Query(EFQ_MainThreadList, nThreadID);
        gEnv->pRenderer->GetFinalizeRendItemJobExecutor(nThreadID)->WaitForCompletion();
        gEnv->pRenderer->GetFinalizeShadowRendItemJobExecutor(nThreadID)->WaitForCompletion();
    }
    //CryLogAlways("UpdateRNTmpDataPool bFreeAll 0x%x nMainFrameID 0X%x", bFreeAll, nMainFrameID );
    FUNCTION_PROFILER_3DENGINE;

    // Ensure continues memory
    m_elementFrameInfo.CoalesceMemory();

    const uint32 nSize = m_elementFrameInfo.size();
    if (nSize == 0)
    {
        return;
    }

    const uint32 nMainFrameID = GetRenderer()->GetFrameID(false);
    const uint32 nTmpDataMaxFrames = (uint32)GetCVars()->e_RNTmpDataPoolMaxFrames;

    if (!bFreeAll && nMainFrameID >= nTmpDataMaxFrames)
    {
        const uint32 nLastValidFrame = nMainFrameID - nTmpDataMaxFrames;

        uint32 nNumItemsToDelete = 0;

        SFrameInfo* pFrontIter = &m_elementFrameInfo[0];
        SFrameInfo* pBackIter = &m_elementFrameInfo[nSize - 1];
        SFrameInfo* pHead = pFrontIter;

        // Handle single element case
        IF (nSize == 1, 0)
        {
            if (pFrontIter->bIsValid && pFrontIter->nLastUsedFrameId >= nLastValidFrame)
            {
                if (pFrontIter->bIsValid)
                {
                    FreeRNTmpData(pFrontIter->ppRNTmpData);
                }

                ++nNumItemsToDelete;
            }
        }

        // Move invalid elements to back of array and free if timed out
        while (pFrontIter < pBackIter)
        {
            while (pFrontIter->bIsValid && pFrontIter->nLastUsedFrameId >= nLastValidFrame && pFrontIter < pBackIter)
            {
                ++pFrontIter;
            }                                                                                                                         // Find invalid element at front
            while (!(pBackIter->bIsValid && pBackIter->nLastUsedFrameId >= nLastValidFrame) && pFrontIter < pBackIter)      // Find valid element at back
            {
                // Element timed out
                if (pBackIter->bIsValid)
                {
                    FreeRNTmpData(pBackIter->ppRNTmpData);
                    pBackIter->bIsValid = false;
                }

                --pBackIter;
                ++nNumItemsToDelete;
            }

            if (pFrontIter < pBackIter)
            {
                // Element timed out
                if (pFrontIter->bIsValid)
                {
                    FreeRNTmpData(pFrontIter->ppRNTmpData);
                }

                // Replace invalid front element with back element
                // Note: No need to swap because we cut the data from the array at the end anyway
                memcpy(pFrontIter, pBackIter, sizeof(SFrameInfo));
                (*pFrontIter->ppRNTmpData)->nFrameInfoId = (uint32)(pFrontIter - pHead);

                pBackIter->bIsValid = false; // safety
                pBackIter->ppRNTmpData = 0;

                --pBackIter;
                ++pFrontIter;
                ++nNumItemsToDelete;
            }
        }

        assert(nSize == m_elementFrameInfo.size());
        m_elementFrameInfo.resize(nSize - nNumItemsToDelete);
    }
    else if (bFreeAll) // Free all
    {
        SFrameInfo* pFrontIter = &m_elementFrameInfo[0];
        SFrameInfo* pBackIter = &m_elementFrameInfo[nSize - 1];
        ++pBackIter; // Point to element after last element in array

        // Free all
        while (pFrontIter != pBackIter)
        {
            // Element timed out
            if (pFrontIter->bIsValid)
            {
                FreeRNTmpData(pFrontIter->ppRNTmpData);
            }

            ++pFrontIter;
        }
        m_elementFrameInfo.resize(0);
    }
}

void C3DEngine::FreeRNTmpDataPool()
{
    // move all into m_LTPRootFree
    UpdateRNTmpDataPool(true);

    AUTO_LOCK(m_checkCreateRNTmpData);
    if (gEnv->mMainThreadId != CryGetCurrentThreadId())
    {
        CryFatalError("CRNTmpData should only be allocated and free'd on main thread.");
    }

    // delete all elements of m_LTPRootFree
    CRNTmpData* pNext = NULL;
    for (CRNTmpData* pElem = m_LTPRootFree.pNext; pElem != &m_LTPRootFree; pElem = pNext)
    {
        pNext = pElem->pNext;
        pElem->Unlink();
        delete pElem;
    }
}

void C3DEngine::CopyObjectsByType(EERType objType, const AABB* pBox, PodArray<IRenderNode*>* plstObjects, ObjectTreeQueryFilterCallback filterCallback)
{
    GetObjectsByTypeGlobal(*plstObjects, objType, pBox, filterCallback);

    if (GetVisAreaManager())
    {
        GetVisAreaManager()->GetObjectsByType(*plstObjects, objType, pBox, filterCallback);
    }
}

void C3DEngine::CopyObjects(const AABB* pBox, PodArray<IRenderNode*>* plstObjects)
{
    if (IsObjectTreeReady())
    {
        m_pObjectsTree->GetObjects(*plstObjects, pBox);
    }

    if (GetVisAreaManager())
    {
        GetVisAreaManager()->GetObjects(*plstObjects, pBox);
    }
}

uint32 C3DEngine::GetObjectsByType(EERType objType, IRenderNode** pObjects)
{
    PodArray<IRenderNode*> lstObjects;
    CopyObjectsByType(objType, NULL, &lstObjects);
    if (pObjects && !lstObjects.IsEmpty())
    {
        memcpy(pObjects, &lstObjects[0], lstObjects.GetDataSize());
    }
    return lstObjects.Count();
}

uint32 C3DEngine::GetObjectsByTypeInBox(EERType objType, const AABB& bbox, IRenderNode** pObjects, ObjectTreeQueryFilterCallback filterCallback)
{
    PodArray<IRenderNode*> lstObjects;
    CopyObjectsByType(objType, &bbox, &lstObjects, filterCallback);
    if (pObjects && !lstObjects.IsEmpty())
    {
        memcpy(pObjects, &lstObjects[0], lstObjects.GetDataSize());
    }
    return lstObjects.Count();
}

void C3DEngine::GetObjectsByTypeInBox(EERType objType, const AABB& bbox, PodArray<IRenderNode*>* pLstObjects, ObjectTreeQueryFilterCallback filterCallback)
{
    CopyObjectsByType(objType, &bbox, pLstObjects, filterCallback);
}

uint32 C3DEngine::GetObjectsInBox(const AABB& bbox, IRenderNode** pObjects)
{
    PodArray<IRenderNode*> lstObjects;
    CopyObjects(&bbox, &lstObjects);
    if (pObjects && !lstObjects.IsEmpty())
    {
        memcpy(pObjects, &lstObjects[0], lstObjects.GetDataSize());
    }
    return lstObjects.Count();
}

uint32 C3DEngine::GetObjectsByFlags(uint dwFlags, IRenderNode** pObjects /* =0 */)
{
    PodArray<IRenderNode*> lstObjects;

    if (Get3DEngine()->IsObjectTreeReady())
    {
        Get3DEngine()->GetObjectTree()->GetObjectsByFlags(dwFlags, lstObjects);
    }

    if (GetVisAreaManager())
    {
        GetVisAreaManager()->GetObjectsByFlags(dwFlags, lstObjects);
    }

    if (pObjects && !lstObjects.IsEmpty())
    {
        memcpy(pObjects, &lstObjects[0], lstObjects.GetDataSize());
    }
    return lstObjects.Count();
}

void C3DEngine::OnObjectModified([[maybe_unused]] IRenderNode* pRenderNode, uint dwFlags)
{
    if ((dwFlags & (ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS)) != 0)
    {
        SetRecomputeCachedShadows(ShadowMapFrustum::ShadowCacheData::eFullUpdate);
    }
}

int SImageInfo::GetMemoryUsage()
{
    int nSize = 0;
    if (detailInfo.pImgMips[0])
    {
        nSize += (int)((float)(detailInfo.nDim * detailInfo.nDim * sizeof(ColorB)) * 1.3f);
    }
    if (baseInfo.pImgMips[0])
    {
        nSize += (int)((float)(baseInfo.nDim * baseInfo.nDim * sizeof(ColorB)) * 1.3f);
    }
    return nSize;
}

byte** C3DEngine::AllocateMips(byte* pImage, int nDim, byte** pImageMips)
{
    memset(pImageMips, 0, SImageSubInfo::nMipsNum * sizeof(pImageMips[0]));

    pImageMips[0] = new byte[nDim * nDim * sizeof(ColorB)];
    memcpy(pImageMips[0], pImage, nDim * nDim * sizeof(ColorB));

    ColorB* pMipMain = (ColorB*)pImageMips[0];

    for (int nMip = 1; (nDim >> nMip) && nMip < SImageSubInfo::nMipsNum; nMip++)
    {
        int nDimMip = nDim >> nMip;

        int nSubSize = 1 << nMip;

        pImageMips[nMip] = new byte[nDimMip * nDimMip * sizeof(ColorB)];

        ColorB* pMipThis = (ColorB*)pImageMips[nMip];

        for (int x = 0; x < nDimMip; x++)
        {
            for (int y = 0; y < nDimMip; y++)
            {
                ColorF colSumm(0, 0, 0, 0);
                float fCount = 0;
                for (int _x = x * nSubSize - nSubSize / 2; _x < x * nSubSize + nSubSize + nSubSize / 2; _x++)
                {
                    for (int _y = y * nSubSize - nSubSize / 2; _y < y * nSubSize + nSubSize + nSubSize / 2; _y++)
                    {
                        int nMask = nDim - 1;
                        int id = (_x & nMask) * nDim + (_y & nMask);
                        colSumm.r += 1.f / 255.f * pMipMain[id].r;
                        colSumm.g += 1.f / 255.f * pMipMain[id].g;
                        colSumm.b += 1.f / 255.f * pMipMain[id].b;
                        colSumm.a += 1.f / 255.f * pMipMain[id].a;
                        fCount++;
                    }
                }

                colSumm /= fCount;

                colSumm.Clamp(0, 1);

                pMipThis[x * nDimMip + y] = colSumm;
            }
        }
    }

    return pImageMips;
}

void C3DEngine::RegisterForStreaming(IStreamable* pObj)
{
    if (GetObjManager())
    {
        GetObjManager()->RegisterForStreaming(pObj);
    }
}

void C3DEngine::UnregisterForStreaming(IStreamable* pObj)
{
    if (GetObjManager())
    {
        GetObjManager()->UnregisterForStreaming(pObj);
    }
}

SImageSubInfo* C3DEngine::GetImageInfo(const char* pName)
{
    if (m_imageInfos.find(string(pName)) != m_imageInfos.end())
    {
        return m_imageInfos[string(pName)];
    }

    return NULL;
}

SImageSubInfo* C3DEngine::RegisterImageInfo(byte** pMips, int nDim, const char* pName)
{
    if (m_imageInfos.find(string(pName)) != m_imageInfos.end())
    {
        return m_imageInfos[string(pName)];
    }

    assert(pMips && pMips[0]);

    SImageSubInfo* pImgSubInfo = new SImageSubInfo;

    pImgSubInfo->nDim = nDim;

    int nMipDim = pImgSubInfo->nDim;

    for (int m = 0; m < SImageSubInfo::nMipsNum && nMipDim; m++)
    {
        pImgSubInfo->pImgMips[m] = new byte[nMipDim * nMipDim * 4];

        memcpy(pImgSubInfo->pImgMips[m], pMips[m], nMipDim * nMipDim * 4);

        nMipDim /= 2;
    }

    const string strFileName = pName;

    pImgSubInfo->nReady = 1;

    m_imageInfos[strFileName] = pImgSubInfo;

    return pImgSubInfo;
}

void C3DEngine::SyncProcessStreamingUpdate()
{
    if (m_pObjManager)
    {
        m_pObjManager->ProcessObjectsStreaming_Finish();
    }
}

void C3DEngine::SetScreenshotCallback(IScreenshotCallback* pCallback)
{
    m_pScreenshotCallback = pCallback;
}

void C3DEngine::ActivateObjectsLayer(uint16 nLayerId, bool bActivate, bool bPhys, bool bObjects, bool bStaticLights, const char* pLayerName, IGeneralMemoryHeap* pHeap, bool bCheckLayerActivation /*=true*/)
{
    if (bCheckLayerActivation && !IsAreaActivationInUse())
    {
        return;
    }

    if (bActivate)
    {
        m_bLayersActivated = true;
    }

    if (bActivate && m_nFramesSinceLevelStart <= 1)
    {
        m_vPrevMainFrameCamPos.Set(-1000000.f, -1000000.f, -1000000.f);
    }

    FUNCTION_PROFILER_3DENGINE;

    PrintMessage("%s object layer %s (Id = %d) (LevelFrameId = %d)", bActivate ? "Activating" : "Deactivating", pLayerName, nLayerId, m_nFramesSinceLevelStart);
    INDENT_LOG_DURING_SCOPE();

    if (bObjects)
    {
        if (IsObjectTreeReady())
        {
            m_pObjectsTree->ActivateObjectsLayer(nLayerId, bActivate, bPhys, pHeap);
        }

        if (m_pVisAreaManager)
        {
            m_pVisAreaManager->ActivateObjectsLayer(nLayerId, bActivate, bPhys, pHeap);
        }
    }

    if (bStaticLights)
    {
        for (size_t i = 0; i < m_lstStaticLights.size(); ++i)
        {
            ILightSource* pLight = m_lstStaticLights[i];
            if (pLight->GetLayerId() == nLayerId)
            {
                pLight->SetRndFlags(ERF_HIDDEN, !bActivate);
            }
        }
    }
}


void C3DEngine::GetLayerMemoryUsage(uint16 nLayerId, ICrySizer* pSizer, int* pNumBrushes, int* pNumDecals) const
{
    if (pNumBrushes)
    {
        *pNumBrushes = 0;
    }
    if (pNumDecals)
    {
        *pNumDecals = 0;
    }

    if (m_pObjectsTree != nullptr)
    {
        m_pObjectsTree->GetLayerMemoryUsage(nLayerId, pSizer, pNumBrushes, pNumDecals);
    }
}


void C3DEngine::SkipLayerLoading(uint16 nLayerId, bool bClearList)
{
    if (bClearList)
    {
        m_skipedLayers.clear();
    }
    m_skipedLayers.insert(nLayerId);
}


bool C3DEngine::IsLayerSkipped(uint16 nLayerId)
{
    return m_skipedLayers.find(nLayerId) != m_skipedLayers.end() ? true : false;
}

void C3DEngine::PrecacheRenderNode(IRenderNode* pObj, float fEntDistanceReal)
{
    FUNCTION_PROFILER_3DENGINE;

    //  PrintMessage("==== PrecacheRenderNodePrecacheRenderNode: %s ====", pObj->GetName());

    if (m_pObjManager)
    {
        int dwOldRndFlags = pObj->m_dwRndFlags;
        pObj->m_dwRndFlags &= ~ERF_HIDDEN;

        SRenderingPassInfo passInfo = SRenderingPassInfo::CreateGeneralPassRenderingInfo(gEnv->pSystem->GetViewCamera());

        m_pObjManager->UpdateRenderNodeStreamingPriority(pObj, fEntDistanceReal, 1.0f, fEntDistanceReal < GetFloatCVar(e_StreamCgfFastUpdateMaxDistance), passInfo, true);
        pObj->m_dwRndFlags = dwOldRndFlags;
    }
}


void C3DEngine::CleanUpOldDecals()
{
    FUNCTION_PROFILER_3DENGINE;
    static uint32 nLastIndex = 0;
    const uint32 nDECALS_PER_FRAME = 50;

    if (uint32 nNumDecalRenderNodes = m_decalRenderNodes.size())
    {
        for (uint32 i = 0, end = __min(nDECALS_PER_FRAME, nNumDecalRenderNodes); i < end; ++i, ++nLastIndex)
        {
            // wrap around at the end to restart at the beginning
            if (nLastIndex >= nNumDecalRenderNodes)
            {
                nLastIndex = 0;
            }

            m_decalRenderNodes[nLastIndex]->CleanUpOldDecals();
        }
    }
}

void C3DEngine::UpdateRenderTypeEnableLookup()
{
    SetRenderNodeTypeEnabled(eERType_RenderComponent, (GetCVars()->e_Entities != 0));
    SetRenderNodeTypeEnabled(eERType_StaticMeshRenderComponent, (GetCVars()->e_Entities != 0));
    SetRenderNodeTypeEnabled(eERType_DynamicMeshRenderComponent, (GetCVars()->e_Entities != 0));
    SetRenderNodeTypeEnabled(eERType_SkinnedMeshRenderComponent, (GetCVars()->e_Entities != 0));
}

void C3DEngine::SetRenderNodeMaterialAtPosition(EERType eNodeType, const Vec3& vPos, _smart_ptr<IMaterial> pMat)
{
    PodArray<IRenderNode*> lstObjects;

    AABB aabbPos(vPos - Vec3(.1f, .1f, .1f), vPos + Vec3(.1f, .1f, .1f));

    GetObjectsByTypeGlobal(lstObjects, eNodeType, &aabbPos);

    if (GetVisAreaManager())
    {
        GetVisAreaManager()->GetObjectsByType(lstObjects, eNodeType, &aabbPos);
    }

    for (int i = 0; i < lstObjects.Count(); i++)
    {
        PrintMessage("Game changed render node material: %s EERType:%d pos: (%d,%d,%d)",
            pMat ? pMat->GetName() : "NULL",
            (int)eNodeType,
            (int)vPos.x, (int)vPos.y, (int)vPos.z);

        lstObjects[i]->SetMaterial(pMat);
    }
}

void C3DEngine::OverrideCameraPrecachePoint(const Vec3& vPos)
{
    if (m_pObjManager)
    {
        m_pObjManager->GetStreamPreCacheCameras()[0].vPosition = vPos;
        m_pObjManager->SetCameraPrecacheOverridden(true);
    }
}

int C3DEngine::AddPrecachePoint(const Vec3& vPos, const Vec3& vDir, float fTimeOut, float fImportanceFactor)
{
    if (m_pObjManager)
    {
        if (m_pObjManager->GetStreamPreCachePointDefs().size() >= CObjManager::MaxPrecachePoints)
        {
            size_t nOldestIdx = 0;
            int nOldestId = INT_MAX;
            for (size_t i = 1, c = m_pObjManager->GetStreamPreCachePointDefs().size(); i < c; ++i)
            {
                if (m_pObjManager->GetStreamPreCachePointDefs()[i].nId < nOldestId)
                {
                    nOldestIdx = i;
                    nOldestId = m_pObjManager->GetStreamPreCachePointDefs()[i].nId;
                }
            }

            assert (nOldestIdx > 0);

            CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "Precache points full - evicting oldest (%f, %f, %f)",
                m_pObjManager->GetStreamPreCacheCameras()[nOldestIdx].vPosition.x,
                m_pObjManager->GetStreamPreCacheCameras()[nOldestIdx].vPosition.y,
                m_pObjManager->GetStreamPreCacheCameras()[nOldestIdx].vPosition.z);

            m_pObjManager->GetStreamPreCachePointDefs().DeleteFastUnsorted((int)nOldestIdx);
            m_pObjManager->GetStreamPreCacheCameras().DeleteFastUnsorted((int)nOldestIdx);
        }

        SObjManPrecachePoint pp;
        pp.nId = m_pObjManager->IncrementNextPrecachePointId();
        pp.expireTime = gEnv->pTimer->GetAsyncTime() + CTimeValue(fTimeOut);
        SObjManPrecacheCamera pc;
        pc.vPosition = vPos;
        pc.bbox = AABB(vPos, GetCVars()->e_StreamPredictionBoxRadius);
        pc.vDirection = vDir;
        pc.fImportanceFactor = fImportanceFactor;
        m_pObjManager->GetStreamPreCachePointDefs().Add(pp);
        m_pObjManager->GetStreamPreCacheCameras().Add(pc);

        return pp.nId;
    }

    return -1;
}

void C3DEngine::ClearPrecachePoint(int id)
{
    if (m_pObjManager)
    {
        for (size_t i = 1, c = m_pObjManager->GetStreamPreCachePointDefs().size(); i < c; ++i)
        {
            if (m_pObjManager->GetStreamPreCachePointDefs()[i].nId == id)
            {
                m_pObjManager->GetStreamPreCachePointDefs().DeleteFastUnsorted((int)i);
                m_pObjManager->GetStreamPreCacheCameras().DeleteFastUnsorted((int)i);
                break;
            }
        }
    }
}

void C3DEngine::ClearAllPrecachePoints()
{
    if (m_pObjManager)
    {
        m_pObjManager->GetStreamPreCachePointDefs().resize(1);
        m_pObjManager->GetStreamPreCacheCameras().resize(1);
    }
}

void C3DEngine::GetPrecacheRoundIds(int pRoundIds[MAX_STREAM_PREDICTION_ZONES])
{
    if (m_pObjManager)
    {
        pRoundIds[0] = m_pObjManager->GetUpdateStreamingPrioriryRoundIdFast();
        pRoundIds[1] = m_pObjManager->GetUpdateStreamingPrioriryRoundId();
    }
}

SBending* C3DEngine::GetBendingEntry(SBending* pSrc, [[maybe_unused]] const SRenderingPassInfo& passInfo)
{
    assert(pSrc);
    SBending* pStorage = m_bendingPool[m_bendingPoolIdx].push_back_new();
    *pStorage = *pSrc;
    return pStorage;
}

///////////////////////////////////////////////////////////////////////////////
CCamera* C3DEngine::GetRenderingPassCamera(const CCamera& rCamera)
{
    threadID nThreadID = 0;
    gEnv->pRenderer->EF_Query(EFQ_RenderThreadList, nThreadID);
    CCamera* pCamera = m_RenderingPassCameras[nThreadID].push_back_new();
    *pCamera = rCamera;
    return pCamera;
}

void C3DEngine::GetCollisionClass(SCollisionClass& collclass, int tableIndex)
{
    if ((unsigned)tableIndex < m_collisionClasses.size())
    {
        collclass = m_collisionClasses[tableIndex];
    }
    else
    {
        collclass = SCollisionClass(0, 0);
    }
}

class C3DEngine::PhysicsAreaUpdatesHandler
    : private Physics::WindNotificationsBus::Handler
{
public:
    explicit PhysicsAreaUpdatesHandler(C3DEngine::PhysicsAreaUpdates& physicsAreaUpdates)
        : m_physicsAreaUpdates(physicsAreaUpdates)
    {
        Physics::WindNotificationsBus::Handler::BusConnect();
    }

    ~PhysicsAreaUpdatesHandler()
    {
        Physics::WindNotificationsBus::Handler::BusDisconnect();
    }

private:
    // Physics::WindNotificationsBus::Handler
    void OnGlobalWindChanged() override
    {
        // Using same 'global wind' area size value CryPhysics code had
        const AZ::Vector3 globalWindHalfBound(1e7);
        AZ::Aabb globalWindAabb = AZ::Aabb::CreateFromMinMax(-globalWindHalfBound, globalWindHalfBound);

        OnWindChanged(globalWindAabb);
    }

    void OnWindChanged(const AZ::Aabb& aabb) override
    {
        SAreaChangeRecord record;
        record.boxAffected = AZAabbToLyAABB(aabb);
        record.uPhysicsMask = Area_Air;

        m_physicsAreaUpdates.SetAreaDirty(record);
    }

    C3DEngine::PhysicsAreaUpdates& m_physicsAreaUpdates;
};

void C3DEngine::PhysicsAreaUpdates::SetAreaDirty(const SAreaChangeRecord& rec)
{
    // Merge with existing bb if close enough and same medium
    AUTO_LOCK(m_Mutex);
    static const float fMERGE_THRESHOLD = 2.f;
    float fNewVolume = rec.boxAffected.GetVolume();
    for (uint i = 0; i < m_DirtyAreas.size(); i++)
    {
        if (m_DirtyAreas[i].uPhysicsMask == rec.uPhysicsMask)
        {
            AABB bbUnion = rec.boxAffected;
            bbUnion.Add(m_DirtyAreas[i].boxAffected);
            if (bbUnion.GetVolume() <= (fNewVolume + m_DirtyAreas[i].boxAffected.GetVolume()) * fMERGE_THRESHOLD)
            {
                m_DirtyAreas[i].boxAffected = bbUnion;
                return;
            }
        }
    }
    m_DirtyAreas.push_back(rec);
}

void C3DEngine::PhysicsAreaUpdates::Update()
{
    //
    // (bethelz) This whole class is only used for CParticleEffect right now.
    //

    FUNCTION_PROFILER_3DENGINE;
    AUTO_LOCK(m_Mutex);
    if (m_DirtyAreas.empty())
    {
        return;
    }

    // Check area against registered proxies
    int nSizeAreasChanged = (int)m_DirtyAreas.size();
    int nSizeProxies = m_Proxies.size();

    // Access elements via [i] as the thread safe list does not always safe its element in a continues array
    for (int i = 0; i < nSizeProxies; ++i)
    {
        const SPhysAreaNodeProxy& proxy = m_Proxies[i];

        IF (!proxy.bIsValid, 0)
        {
            continue;
        }

        for (int j = 0; j < nSizeAreasChanged; ++j)
        {
            const SAreaChangeRecord& rec = m_DirtyAreas[j];
            if ((proxy.uPhysicsMask & rec.uPhysicsMask) && Overlap::AABB_AABB(proxy.bbox, rec.boxAffected))
            {
                if (rec.uPhysicsMask & Area_Air)
                {
                    if (proxy.pRenderNode->m_pRNTmpData)
                    {
                        proxy.pRenderNode->m_pRNTmpData->userData.bWindCurrent = 0;
                    }
                }

                proxy.pRenderNode->OnPhysAreaChange();
                break;
            }
        }
    }

    m_DirtyAreas.resize(0);
}

void C3DEngine::PhysicsAreaUpdates::Reset()
{
    stl::free_container(m_DirtyAreas);
}

uint32 C3DEngine::PhysicsAreaUpdates::CreateProxy(const IRenderNode* pRenderNode, uint16 uPhysicsMask)
{
    size_t nIndex = ~0;
    SPhysAreaNodeProxy* proxy = m_Proxies.push_back_new(nIndex);

    proxy->pRenderNode      = (IRenderNode*)pRenderNode;
    proxy->uPhysicsMask     = uPhysicsMask;
    proxy->bIsValid             = true;
    proxy->bbox                     = pRenderNode->GetBBox();
    return nIndex;
}

void C3DEngine::PhysicsAreaUpdates::UpdateProxy(const IRenderNode* pRenderNode, uint32 nProxyId)
{
    m_Proxies[nProxyId].bbox = pRenderNode->GetBBox();
}

void C3DEngine::PhysicsAreaUpdates::ResetProxy(uint32 proxyId)
{
    m_Proxies[proxyId].Reset();
}

void C3DEngine::PhysicsAreaUpdates::GarbageCollect()
{
    // Ensure list is continues in memory
    m_Proxies.CoalesceMemory();

    const uint32 nSize = m_Proxies.size();
    if (nSize == 0)
    {
        return;
    }

    SPhysAreaNodeProxy* pFrontIter = &m_Proxies[0];
    SPhysAreaNodeProxy* pBackIter = &m_Proxies[nSize - 1];
    const SPhysAreaNodeProxy* pHead = pFrontIter;
    uint32 nNumItemsToDelete = 0;

    // Move invalid nodes to the back of the array
    do
    {
        while (pFrontIter->bIsValid && pFrontIter < pBackIter)
        {
            ++pFrontIter;
        }
        while (!pBackIter->bIsValid && pFrontIter < pBackIter)
        {
            --pBackIter;
            ++nNumItemsToDelete;
        }

        if (pFrontIter < pBackIter)
        {
            // Replace invalid front element with back element
            // Note: No need to swap because we cut the data from the array at the end anyway
            memcpy(pFrontIter, pBackIter, sizeof(SPhysAreaNodeProxy));
            AZ_Assert(pFrontIter->pRenderNode && pFrontIter->pRenderNode->m_pRNTmpData, "The front iterator should have a valid m_pRNTmpData after the data from the valid back iterator has been copied to it.");
            if (pFrontIter->pRenderNode && pFrontIter->pRenderNode->m_pRNTmpData)
            {
                pFrontIter->pRenderNode->m_pRNTmpData->nPhysAreaChangedProxyId = (uint32)(pFrontIter - pHead);
            }
            
            pBackIter->bIsValid = false;

            --pBackIter;
            ++pFrontIter;
            ++nNumItemsToDelete;
        }
    } while (pFrontIter < pBackIter);

    // Cut off invalid elements
    m_Proxies.resize(nSize - nNumItemsToDelete);
}

void C3DEngine::UpdateShaderItems()
{
    if (GetMatMan())
    {
        GetMatMan()->UpdateShaderItems();
    }
}

void C3DEngine::OnCameraTeleport()
{
    MarkRNTmpDataPoolForReset();
}

IObjManager* C3DEngine::GetObjectManager()
{
    return m_pObjManager;
}

const IObjManager* C3DEngine::GetObjectManager() const
{
    return m_pObjManager;
}

bool C3DEngine::RemoveObjectsInArea(Vec3 vExploPos, float fExploRadius)
{
    bool bEverythingDeleted = true;

    Vec3 vRadius(fExploRadius, fExploRadius, fExploRadius);

    PodArray<SRNInfo> lstEntities;
    const AABB cExplosionBox(vExploPos - vRadius, vExploPos + vRadius);
    MoveObjectsIntoListGlobal(&lstEntities, &cExplosionBox, false, true, true, true);

    // remove small objects around
    {
        for (int i = 0; i < lstEntities.Count(); i++)
        {
            IRenderNode* pRenderNode = lstEntities[i].pNode;
            AABB entBox = pRenderNode->GetBBox();
            float fEntRadius = entBox.GetRadius();
            Vec3 vEntCenter = pRenderNode->GetBBox().GetCenter();
            float fDist = vExploPos.GetDistance(vEntCenter);
            if (fDist < fExploRadius + fEntRadius &&
                Overlap::Sphere_AABB(Sphere(vExploPos, fExploRadius), entBox))
            {
                if (fDist >= fExploRadius)
                { //
                    Matrix34A objMat;
                    CStatObj* pStatObj = (CStatObj*)pRenderNode->GetEntityStatObj(0, 0, &objMat);
                    if (!pStatObj)
                    {
                        continue;
                    }
                    objMat.Invert();
                    //assert(0);
                    Vec3 vOSExploPos = objMat.TransformPoint(vExploPos);

                    Vec3 vScaleTest(0, 0, 1.f);
                    vScaleTest = objMat.TransformVector(vScaleTest);
                    float fObjScaleInv = vScaleTest.len();

                    if (!pStatObj->IsSphereOverlap(Sphere(vOSExploPos, fExploRadius * fObjScaleInv)))
                    {
                        continue;
                    }
                }
            }
        }
    }

    return bEverythingDeleted;
}

#ifndef _RELEASE

//////////////////////////////////////////////////////////////////////////
// onscreen infodebug code for e_debugDraw >= 100

//////////////////////////////////////////////////////////////////////////
void C3DEngine::AddObjToDebugDrawList(SObjectInfoToAddToDebugDrawList& objInfo)
{
    m_DebugDrawListMgr.AddObject(objInfo);
}


bool    CDebugDrawListMgr::m_dumpLogRequested = false;
bool    CDebugDrawListMgr::m_freezeRequested = false;
bool    CDebugDrawListMgr::m_unfreezeRequested = false;
uint32 CDebugDrawListMgr::m_filter = I3DEngine::DLOT_ALL;


//////////////////////////////////////////////////////////////////////////
CDebugDrawListMgr::CDebugDrawListMgr()
{
    m_isFrozen = false;
    ClearFrameData();
    ClearConsoleCommandRequestVars();
    m_assets.reserve(32);   // just a reasonable value
    m_drawBoxes.reserve(256);   // just a reasonable value
}


//////////////////////////////////////////////////////////////////////////
void CDebugDrawListMgr::ClearFrameData()
{
    m_counter = 0;
    m_assetCounter = 0;
    m_assets.clear();
    m_drawBoxes.clear();
    m_indexLeastValueAsset = 0;
    CheckFilterCVar();
}


//////////////////////////////////////////////////////////////////////////
void CDebugDrawListMgr::ClearConsoleCommandRequestVars()
{
    m_dumpLogRequested      = false;
    m_freezeRequested           = false;
    m_unfreezeRequested     = false;
}

//////////////////////////////////////////////////////////////////////////
void CDebugDrawListMgr::AddObject(I3DEngine::SObjectInfoToAddToDebugDrawList& newObjInfo)
{
    if (m_isFrozen)
    {
        return;
    }

    m_lock.Lock();

    m_counter++;

    if (!ShouldFilterOutObject(newObjInfo))
    {
        TAssetInfo newAsset(newObjInfo);
        TObjectDrawBoxInfo newDrawBox(newObjInfo);

        TAssetInfo* pAssetDuplicated = FindDuplicate(newAsset);
        if (pAssetDuplicated)
        {
            pAssetDuplicated->numInstances++;
            newDrawBox.assetID = pAssetDuplicated->ID;
            m_drawBoxes.push_back(newDrawBox);
            pAssetDuplicated->drawCalls = max(pAssetDuplicated->drawCalls, newAsset.drawCalls);
        }
        else
        {
            newAsset.ID = m_assetCounter;
            newDrawBox.assetID = newAsset.ID;
            bool used = false;

            // list not full, so we add
            if (m_assets.size() < uint(Cry3DEngineBase::GetCVars()->e_DebugDrawListSize))
            {
                used = true;
                m_assets.push_back(newAsset);
            }
            else // if is full, only use it if value is greater than the current minimum, and then it substitutes the lowest slot
            {
                const TAssetInfo& leastValueAsset = m_assets[ m_indexLeastValueAsset ];
                if (leastValueAsset < newAsset)
                {
                    used = true;
                    m_assets[ m_indexLeastValueAsset ] = newAsset;
                }
            }
            if (used)
            {
                m_assetCounter++;
                m_drawBoxes.push_back(newDrawBox);
                FindNewLeastValueAsset();
            }
        }
    }

    m_lock.Unlock();
}


//////////////////////////////////////////////////////////////////////////
CDebugDrawListMgr::TAssetInfo* CDebugDrawListMgr::FindDuplicate(const TAssetInfo& asset)
{
    std::vector<TAssetInfo>::iterator iter = m_assets.begin();
    std::vector<TAssetInfo>::const_iterator endArray = m_assets.end();

    while (iter != endArray)
    {
        TAssetInfo& currAsset = *iter;
        if (currAsset.fileName == asset.fileName)
        {
            return &currAsset;
        }
        ++iter;
    }
    return NULL;
}



//////////////////////////////////////////////////////////////////////////
bool CDebugDrawListMgr::ShouldFilterOutObject(const I3DEngine::SObjectInfoToAddToDebugDrawList& object)
{
    if (m_filter == I3DEngine::DLOT_ALL)
    {
        return false;
    }

    return (m_filter & object.type) == 0;
}


//////////////////////////////////////////////////////////////////////////
void CDebugDrawListMgr::FindNewLeastValueAsset()
{
    uint32 size = m_assets.size();
    for (uint32 i = 0; i < size; ++i)
    {
        const TAssetInfo& currAsset = m_assets[ i ];
        const TAssetInfo& leastValueAsset = m_assets[ m_indexLeastValueAsset ];

        if (currAsset < leastValueAsset)
        {
            m_indexLeastValueAsset = i;
        }
    }
}



//////////////////////////////////////////////////////////////////////////
void CDebugDrawListMgr::Update()
{
    m_lock.Lock();
    if (!m_isFrozen)
    {
        std::sort(m_assets.begin(), m_assets.end(), CDebugDrawListMgr::SortComparison);
    }
    gEnv->pRenderer->CollectDrawCallsInfoPerNode(true);
    // it displays values from the previous frame. This mean that if it is disabled, and then enabled again later on, it will display bogus values for 1 frame...but i dont care (yet)
    float X = 10.f;
    float Y = 100.f;
    if (m_isFrozen)
    {
        PrintText(X, Y, Col_Red, "FROZEN DEBUGINFO");
    }
    Y += 20.f;
    PrintText(X, Y, Col_White, "total assets: %d     Ordered by:                 Showing:", m_counter);
    PrintText(X + 240, Y, Col_Yellow, GetStrCurrMode());
    TMyStandardString filterStr;
    GetStrCurrFilter(filterStr);
    PrintText(X + 420, Y, Col_Yellow, filterStr.c_str());
    Y += 20.f;
    float XName = 270;

    const char* pHeaderStr = "";
    switch (Cry3DEngineBase::GetCVars()->e_DebugDraw)
    {
    case LM_TRI_COUNT:
        pHeaderStr = "   tris  " " meshMem  " "rep  " " type        ";
        break;
    case LM_VERT_COUNT:
        pHeaderStr = "  verts  " " meshMem  " "rep  " " type        ";
        break;
    case LM_DRAWCALLS:
        pHeaderStr = "draw Calls  " "   tris  " "rep  " " type        ";
        break;
    case LM_TEXTMEM:
        pHeaderStr = "  texMem  " " meshMem  " "rep  " " type        ";
        break;
    case LM_MESHMEM:
        pHeaderStr = " meshMem  " "  texMem  " "rep  " " type        ";
        break;
    }


    PrintText(X, Y, Col_White, pHeaderStr);
    const int standardNameSize = 48;
    PrintText(XName, Y, Col_White, "Entity (class)");
    PrintText(XName + (standardNameSize + 2) * 7, Y, Col_White, "File name");

    Y += 20.f;
    for (uint32 i = 0; i < m_assets.size(); ++i)
    {
        ColorF colorLine = Col_Cyan;
        if (Cry3DEngineBase::GetCVars()->e_DebugDrawListBBoxIndex - 1 == i)
        {
            colorLine = Col_Blue;
        }
        const TAssetInfo& currAsset = m_assets[ i ];
        TMyStandardString texMemoryStr;
        TMyStandardString meshMemoryStr;
        MemToString(currAsset.texMemory, texMemoryStr);
        MemToString(currAsset.meshMemory, meshMemoryStr);

        switch (Cry3DEngineBase::GetCVars()->e_DebugDraw)
        {
        case LM_TRI_COUNT:
            PrintText(X, Y + 20.f * i, colorLine, "%7d  " "%s  " "%3d  " "%s",
                currAsset.numTris, meshMemoryStr.c_str(), currAsset.numInstances, GetAssetTypeName(currAsset.type));
            break;
        case LM_VERT_COUNT:
            PrintText(X, Y + 20.f * i, colorLine, "%7d  " "%s  " "%3d  " "%s",
                currAsset.numVerts, meshMemoryStr.c_str(), currAsset.numInstances, GetAssetTypeName(currAsset.type));
            break;
        case LM_DRAWCALLS:
        {
            PrintText(X, Y + 20.f * i, colorLine, "  %5d     " "%7d  " "%3d  " "%s",
                currAsset.drawCalls, currAsset.numTris, currAsset.numInstances, GetAssetTypeName(currAsset.type));
        }
        break;
        case LM_TEXTMEM:
            PrintText(X, Y + 20.f * i, colorLine, "%s  " "%s  " "%3d  " "%s",
                texMemoryStr.c_str(), meshMemoryStr.c_str(), currAsset.numInstances, GetAssetTypeName(currAsset.type));
            break;
        case LM_MESHMEM:
            PrintText(X, Y + 20.f * i, colorLine, "%s  " "%s  " "%3d  " "%s",
                meshMemoryStr.c_str(), texMemoryStr.c_str(), currAsset.numInstances, GetAssetTypeName(currAsset.type));
            break;
        }

        int filenameSep = 7 * (max(standardNameSize, int(currAsset.name.length())) + 2);
        float XFileName = XName + filenameSep;

        PrintText(XName, Y + 20.f * i, colorLine, currAsset.name.c_str());
        PrintText(XFileName, Y + 20.f * i, colorLine, currAsset.fileName.c_str());
    }

    if (Cry3DEngineBase::GetCVars()->e_DebugDrawListBBoxIndex > 0 && uint(Cry3DEngineBase::GetCVars()->e_DebugDrawListBBoxIndex - 1) < m_assets.size())
    {
        const TAssetInfo& assetInfo = m_assets[ Cry3DEngineBase::GetCVars()->e_DebugDrawListBBoxIndex - 1 ];
        uint32 numBoxesDrawn = 0;
        for (uint32 i = 0; i < m_drawBoxes.size(); ++i)
        {
            const TObjectDrawBoxInfo& drawBox = m_drawBoxes[ i ];
            if (drawBox.assetID == assetInfo.ID)
            {
                gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(drawBox.bbox, drawBox.mat, true, ColorB(0, 0, 255, 100), eBBD_Faceted);
                numBoxesDrawn++;
                if (numBoxesDrawn >= assetInfo.numInstances)
                {
                    break;
                }
            }
        }
    }

    if (m_freezeRequested)
    {
        m_isFrozen = true;
    }
    if (m_unfreezeRequested)
    {
        m_isFrozen = false;
    }
    if (m_dumpLogRequested)
    {
        DumpLog();
    }

    ClearConsoleCommandRequestVars();

    if (!m_isFrozen)
    {
        ClearFrameData();
        m_assets.reserve(Cry3DEngineBase::GetCVars()->e_DebugDrawListSize);
    }
    m_lock.Unlock();
}

//////////////////////////////////////////////////////////////////////////
void CDebugDrawListMgr::PrintText(float x, float y, const ColorF& fColor, const char* label_text, ...)
{
    va_list args;
    va_start(args, label_text);
    SDrawTextInfo ti;
    ti.xscale = ti.yscale = 1.2f;
    ti.flags = eDrawText_2D | eDrawText_FixedSize | eDrawText_Monospace;
    {
        ti.color[0] = fColor[0];
        ti.color[1] = fColor[1];
        ti.color[2] = fColor[2];
        ti.color[3] = fColor[3];
    }
    gEnv->pRenderer->DrawTextQueued(Vec3(x, y, 0.5f), ti, label_text, args);
    va_end(args);
}


//////////////////////////////////////////////////////////////////////////
void CDebugDrawListMgr::MemToString(uint32 memVal, TMyStandardString& outStr)
{
    if (memVal < 1024 * 1024)
    {
        outStr.Format("%5.1f kb", memVal / (1024.f));
    }
    else
    {
        outStr.Format("%5.1f MB", memVal / (1024.f * 1024.f));
    }
}

//////////////////////////////////////////////////////////////////////////
bool CDebugDrawListMgr::TAssetInfo::operator<(const TAssetInfo& other) const
{
    switch (Cry3DEngineBase::GetCVars()->e_DebugDraw)
    {
    case LM_TRI_COUNT:
        return (numTris < other.numTris);

    case LM_VERT_COUNT:
        return (numVerts < other.numVerts);

    case LM_DRAWCALLS:
        return (drawCalls < other.drawCalls);

    case LM_TEXTMEM:
        return (texMemory < other.texMemory);

    case LM_MESHMEM:
        return (meshMemory < other.meshMemory);

    default:
        assert(false);
        return false;
    }
}


//////////////////////////////////////////////////////////////////////////
CDebugDrawListMgr::TAssetInfo::TAssetInfo(const I3DEngine::SObjectInfoToAddToDebugDrawList& objInfo)
{
    type = objInfo.type;
    if (!objInfo.pClassName)
    {
        // custom functions to avoid any heap allocation
        MyString_Assign(name, objInfo.pName);
        MyStandardString_Concatenate(name, "(");
        MyStandardString_Concatenate(name, objInfo.pClassName);
        MyStandardString_Concatenate(name, ")");
    }

    MyFileNameString_Assign(fileName, objInfo.pFileName);

    numTris = objInfo.numTris;
    numVerts = objInfo.numVerts;
    texMemory = objInfo.texMemory;
    meshMemory = objInfo.meshMemory;
    drawCalls = gEnv->pRenderer->GetDrawCallsPerNode(objInfo.pRenderNode);
    numInstances = 1;
    ID = UNDEFINED_ASSET_ID;
}


//////////////////////////////////////////////////////////////////////////
CDebugDrawListMgr::TObjectDrawBoxInfo::TObjectDrawBoxInfo(const I3DEngine::SObjectInfoToAddToDebugDrawList& objInfo)
{
    mat.SetIdentity();
    bbox.Reset();
    if (objInfo.pMat)
    {
        mat = *objInfo.pMat;
    }
    if (objInfo.pBox)
    {
        bbox = *objInfo.pBox;
    }
    assetID = UNDEFINED_ASSET_ID;
}



//////////////////////////////////////////////////////////////////////////
void CDebugDrawListMgr::MyStandardString_Concatenate(TMyStandardString& outStr, const char* pStr)
{
    if (pStr && outStr.length() < outStr.capacity())
    {
        outStr._ConcatenateInPlace(pStr, min(strlen(pStr), outStr.capacity() - outStr.length()));
    }
}


//////////////////////////////////////////////////////////////////////////
void CDebugDrawListMgr::MyFileNameString_Assign(TFilenameString& outStr, const char* pStr)
{
    char tempBuf[ outStr.MAX_SIZE + 1 ];

    uint32 outInd = 0;
    if (pStr)
    {
        while (*pStr != 0 && outInd < outStr.MAX_SIZE)
        {
            tempBuf[outInd] = *pStr;
            outInd++;
            if (*pStr == '%' &&   outInd < outStr.MAX_SIZE)
            {
                tempBuf[outInd] = '%';
                outInd++;
            }
            pStr++;
        }
    }
    tempBuf[ outInd ] = 0;
    outStr = tempBuf;
}


//////////////////////////////////////////////////////////////////////////
void CDebugDrawListMgr::ConsoleCommand(IConsoleCmdArgs* args)
{
    if (args->GetArgCount() > 1 && args->GetArg(1))
    {
        switch (toupper(args->GetArg(1)[0]))
        {
        case 'F':
            m_freezeRequested = true;
            break;
        case 'C':
            m_unfreezeRequested = true;
            break;
        case 'D':
            m_dumpLogRequested = true;
            break;
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CDebugDrawListMgr::CheckFilterCVar()
{
    ICVar* pCVar = gEnv->pConsole->GetCVar("e_debugdrawlistfilter");

    if (!pCVar)
    {
        return;
    }

    const char* pVal = pCVar->GetString();

    if (pVal && azstricmp(pVal, "all") == 0)
    {
        m_filter = I3DEngine::DLOT_ALL;
        return;
    }

    m_filter = 0;
    while (pVal && pVal[0])
    {
        switch (toupper(pVal[0]))
        {
        case 'C':
            m_filter |= I3DEngine::DLOT_CHARACTER;
            break;
        case 'S':
            m_filter |= I3DEngine::DLOT_STATOBJ;
            break;
        }
        ++pVal;
    }
}


//////////////////////////////////////////////////////////////////////////
void CDebugDrawListMgr::DumpLog()
{
    TMyStandardString filterStr;
    GetStrCurrFilter(filterStr);
    CryLog("--------------------------------------------------------------------------------");
    CryLog("                           DebugDrawList infodebug");
    CryLog("--------------------------------------------------------------------------------");
    CryLog(" total objects: %d    Ordered by: %s     Showing: %s", m_counter, GetStrCurrMode(), filterStr.c_str());
    CryLog(PRINTF_EMPTY_FORMAT);
    CryLog("   tris      verts   draw Calls   texMem     meshMem    type");
    CryLog(" -------   --------  ----------  --------   --------  ----------");
    for (uint32 i = 0; i < m_assets.size(); ++i)
    {
        const TAssetInfo& currAsset = m_assets[ i ];
        TMyStandardString texMemoryStr;
        TMyStandardString meshMemoryStr;
        MemToString(currAsset.texMemory, texMemoryStr);
        MemToString(currAsset.meshMemory, meshMemoryStr);
        CryLog("%8d  %8d     %5d     %s   %s  %s      %s    %s", currAsset.numTris, currAsset.numVerts, currAsset.drawCalls,
            texMemoryStr.c_str(), meshMemoryStr.c_str(), GetAssetTypeName(currAsset.type), currAsset.name.c_str(), currAsset.fileName.c_str());
    }
    CryLog("--------------------------------------------------------------------------------");
}



//////////////////////////////////////////////////////////////////////////
const char* CDebugDrawListMgr::GetStrCurrMode()
{
    const char* pModesNames[] =
    {
        "Tri count",
        "Vert count",
        "Draw calls",
        "Texture memory",
        "Mesh memory"
    };

    uint32 index = Cry3DEngineBase::GetCVars()->e_DebugDraw - LM_BASENUMBER;
    uint32 numElems = sizeof(pModesNames) / sizeof *(pModesNames);
    if (index < numElems)
    {
        return pModesNames[index];
    }
    else
    {
        return "<UNKNOWN>";
    }
}

//////////////////////////////////////////////////////////////////////////
void CDebugDrawListMgr::GetStrCurrFilter(TMyStandardString& strOut)
{
    const char* pFilterNames[] =
    {
        "",
        "Characters",
        "StatObjs"
    };

    uint32 numElems = sizeof(pFilterNames) / sizeof *(pFilterNames);
    for (uint32 i = 1, bitVal = 1; i < numElems; ++i)
    {
        if ((bitVal & m_filter) != 0)
        {
            if (strOut.size() > 0)
            {
                strOut += "+";
            }
            strOut += pFilterNames[i];
        }
        bitVal *= 2;
    }

    if (strOut.size() == 0)
    {
        strOut = "ALL";
    }
}


//////////////////////////////////////////////////////////////////////////
const char* CDebugDrawListMgr::GetAssetTypeName(I3DEngine::EDebugDrawListAssetTypes type)
{
    const char* pNames[] =
    {
        "",
        "Brush     ",
        "Vegetation",
        "Character ",
        "StatObj   "
    };

    uint32 numElems = sizeof(pNames) / sizeof *(pNames);
    for (uint32 i = 1, bitVal = 1; i < numElems; ++i)
    {
        if (bitVal == type)
        {
            return pNames[i];
        }
        bitVal *= 2;
    }

    return "<UNKNOWN>";
}



#endif //RELEASE

void C3DEngine::GetStatObjAndMatTables(DynArray<IStatObj*>* pStatObjTable, DynArray<_smart_ptr<IMaterial> >* pMatTable, DynArray<IStatInstGroup*>* pStatInstGroupTable, uint32 nObjTypeMask)
{
    SHotUpdateInfo exportInfo;
    exportInfo.nObjTypeMask = nObjTypeMask;

    std::vector<IStatObj*> statObjTable;
    std::vector<_smart_ptr<IMaterial> > matTable;
    std::vector<IStatInstGroup*> statInstGroupTable;

    if (Get3DEngine() && Get3DEngine()->IsObjectTreeReady())
    {
        Get3DEngine()->GetObjectTree()->GenerateStatObjAndMatTables((pStatObjTable != NULL) ? &statObjTable : NULL,
            (pMatTable != NULL) ? &matTable : NULL,
            (pStatInstGroupTable != NULL) ? &statInstGroupTable : NULL,
            &exportInfo);
    }

    if (GetVisAreaManager())
    {
        GetVisAreaManager()->GenerateStatObjAndMatTables((pStatObjTable != NULL) ? &statObjTable : NULL,
            (pMatTable != NULL) ? &matTable : NULL,
            (pStatInstGroupTable != NULL) ? &statInstGroupTable : NULL,
            &exportInfo);
    }

    if (pStatObjTable)
    {
        pStatObjTable->resize(statObjTable.size());
        for (size_t i = 0; i < statObjTable.size(); ++i)
        {
            (*pStatObjTable)[i] = statObjTable[i];
        }

        statObjTable.clear();
    }

    if (pMatTable)
    {
        pMatTable->resize(matTable.size());
        for (size_t i = 0; i < matTable.size(); ++i)
        {
            (*pMatTable)[i] = matTable[i];
        }

        matTable.clear();
    }

    if (pStatInstGroupTable)
    {
        pStatInstGroupTable->resize(statInstGroupTable.size());
        for (size_t i = 0; i < statInstGroupTable.size(); ++i)
        {
            (*pStatInstGroupTable)[i] = statInstGroupTable[i];
        }

        statInstGroupTable.clear();
    }
}

