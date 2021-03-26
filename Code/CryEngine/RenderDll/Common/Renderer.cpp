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

// Description : Abstract renderer API


#include "RenderDll_precompiled.h"

#include <AzCore/Debug/Profiler.h>
#include <AzCore/PlatformId/PlatformId.h>
#include "Shadow_Renderer.h"
#include "IStatObj.h"
#include "I3DEngine.h"
#include "IMovieSystem.h"
#include "IIndexedMesh.h"
#include "BitFiddling.h"                                                    // IntegerLog2()
#include "ImageExtensionHelper.h"                                           // CImageExtensionHelper
#include "Textures/Image/CImage.h"
#include "Textures/TextureManager.h"
#include "Textures/TextureStreamPool.h"
#include "PostProcess/PostEffects.h"
#include "RendElements/CRELensOptics.h"
#include "IStereoRenderer.h"
#include "../../Cry3DEngine/Environment/OceanEnvironmentBus.h"
#include <Pak/CryPakUtils.h>

#include <LoadScreenBus.h>
#include <IVideoRenderer.h>

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED && !defined(NULL_RENDERER)
#include <RTT/RTTContextManager.h>
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED && !defined(NULL_RENDERER)

#include "RendElements/OpticsFactory.h"

#include "GraphicsPipeline/FurBendData.h"

#include "IGeomCache.h"
#include "ITimeOfDay.h"
#include <AzCore/Math/Crc.h>

#include "StatObjBus.h"
#include "RenderBus.h"

#include "RenderCapabilities.h"
#include "RenderView.h"

#if !defined(NULL_RENDERER)
#include "DriverD3D.h"  //Needed for eGT_256bpp_PATH
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define RENDERER_CPP_SECTION_1 1
#define RENDERER_CPP_SECTION_2 2
#define RENDERER_CPP_SECTION_4 4
#define RENDERER_CPP_SECTION_5 5
#define RENDERER_CPP_SECTION_7 7
#define RENDERER_CPP_SECTION_8 8
#define RENDERER_CPP_SECTION_9 9
#define RENDERER_CPP_SECTION_10 10
#define RENDERER_CPP_SECTION_11 11
#define RENDERER_CPP_SECTION_12 12
#define RENDERER_CPP_SECTION_13 13
#define RENDERER_CPP_SECTION_14 14
#define RENDERER_CPP_SECTION_15 15
#define RENDERER_CPP_SECTION_16 16
#define RENDERER_CPP_SECTION_17 17
#endif

#if defined(LINUX)
#include "ILog.h"
#endif
#include "IShader.h"
#include "Renderer.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <Common/Memory/VRAMDriller.h>
#include "Maestro/Types/AnimParamType.h"
#include <MainThreadRenderRequestBus.h>

#if defined(AZ_PLATFORM_WINDOWS) // Scrubber friendly define negation
    #if defined(OPENGL) // Scrubber friendly define negation
    #else
        #ifdef ENABLE_FRAME_PROFILER_LABELS
        // This is need for D3DPERF_ functions
        __pragma(comment(lib, "d3d9.lib"))
        #endif
    #endif
#endif

namespace
{
    class CConditonalLock
    {
        CryCriticalSection& _lock;
        bool _bActive;
    public:
        CConditonalLock(CryCriticalSection& lock, bool bActive)
            : _lock(lock)
            , _bActive(bActive)
        {
            if (_bActive)
            {
                _lock.Lock();
            }
        }
        ~CConditonalLock()
        {
            if (_bActive)
            {
                _lock.Unlock();
            }
        }
    };
}


bool QueryIsFullscreen();

#if defined(SUPPORT_D3D_DEBUG_RUNTIME)
string D3DDebug_GetLastMessage();
#endif

//////////////////////////////////////////////////////////////////////////
// Globals.
//////////////////////////////////////////////////////////////////////////
CRenderer* gRenDev = nullptr;

#define RENDERER_DEFAULT_FONT "Fonts/default.xml"

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(Renderer_cpp)
#endif
#if !defined(RENDERER_DEFAULT_MESHPOOLSIZE)
# define RENDERER_DEFAULT_MESHPOOLSIZE (0U)
#endif
#if !defined(RENDERER_DEFAULT_MESHINSTANCEPOOLSIZE)
# define RENDERER_DEFAULT_MESHINSTANCEPOOLSIZE (0U)
#endif

int g_CpuFlags = 0;

CNameTableR* CCryNameR::ms_table = nullptr;

//////////////////////////////////////////////////////////////////////////
// Pool allocators.
//////////////////////////////////////////////////////////////////////////
SDynTexture_PoolAlloc* g_pSDynTexture_PoolAlloc = 0;
//////////////////////////////////////////////////////////////////////////

// per-frame profilers: collect the information for each frame for
// displaying statistics at the beginning of each frame
//#define PROFILER(ID,NAME) DEFINE_FRAME_PROFILER(ID,NAME)
//#include "FrameProfilers-list.h"
//#undef PROFILER

/// Used to delete none pre-allocated RenderObject pool elements
struct SDeleteNonePoolRenderObjs
{
    SDeleteNonePoolRenderObjs(CRenderObject* pPoolStart, CRenderObject* pPoolEnd)
        : m_pPoolStart(pPoolStart)
        , m_pPoolEnd(pPoolEnd)   {}

    void operator()(CRenderObject** pData) const
    {
        // Delete elements outside of pool range
        if (*pData && (*pData < m_pPoolStart || *pData > m_pPoolEnd))
        {
            delete *pData;
        }
    }

    CRenderObject* m_pPoolStart;
    CRenderObject* m_pPoolEnd;
};

/// Used by console auto completion.
struct STextureNameAutoComplete
    : public IConsoleArgumentAutoComplete
{
    virtual int GetCount() const
    {
        SResourceContainer* pRC = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
        if (!pRC)
        {
            return 0;
        }
        return pRC->m_RMap.size();
    }
    virtual const char* GetValue(int nIndex) const
    {
        SResourceContainer* pRC = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
        if (!pRC || pRC->m_RMap.empty())
        {
            return "";
        }
        nIndex %= pRC->m_RMap.size();
        ResourcesMap::const_iterator it = pRC->m_RMap.begin();
        std::advance(it, nIndex);
        CTexture* pTex = (CTexture*)it->second;
        if (pTex)
        {
            return pTex->GetSourceName();
        }
        return "";
    }
} g_TextureNameAutoComplete;

// Common render console variables
int CRenderer::CV_r_ApplyToonShading;
int CRenderer::CV_r_GraphicsPipeline;
int CRenderer::CV_r_PostProcess_CB;
int CRenderer::CV_r_PostProcess;
float CRenderer::CV_r_dofMinZ;
float CRenderer::CV_r_dofMinZScale;
float CRenderer::CV_r_dofMinZBlendMult;
int CRenderer::CV_r_vsync;
int CRenderer::CV_r_OldBackendSkip;
#if defined(SUPPORT_DEVICE_INFO_USER_DISPLAY_OVERRIDES)
float CRenderer::CV_r_overrideRefreshRate = 0;
int CRenderer::CV_r_overrideScanlineOrder = 0;
int CRenderer::CV_r_overrideDXGIOutput = 0;
int CRenderer::CV_r_overrideDXGIOutputFS = 0;
#endif
#if defined(WIN32) || defined(WIN64)
int CRenderer::CV_r_FullscreenPreemption = 1;
#endif
AllocateConstIntCVar(CRenderer, CV_e_DebugTexelDensity);
int CRenderer::CV_r_flush;
int CRenderer::CV_r_minimizeLatency = 0;
AllocateConstIntCVar(CRenderer, CV_r_statsMinDrawcalls);
AllocateConstIntCVar(CRenderer, CV_r_stats);
AllocateConstIntCVar(CRenderer, CV_r_profiler);
float CRenderer::CV_r_profilerTargetFPS;
int CRenderer::CV_r_log = 0;
AllocateConstIntCVar(CRenderer, CV_r_logTexStreaming);
AllocateConstIntCVar(CRenderer, CV_r_logShaders);
int CRenderer::CV_r_logVBuffers;
AllocateConstIntCVar(CRenderer, CV_r_logVidMem);
AllocateConstIntCVar(CRenderer, CV_r_predicatedtiling);
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_CPP_SECTION_2
    #include AZ_RESTRICTED_FILE(Renderer_cpp)
#endif
int CRenderer::CV_r_DeferredShadingSortLights;
int CRenderer::CV_r_DeferredShadingAmbientSClear;
int CRenderer::CV_r_msaa;
int CRenderer::CV_r_msaa_samples;
int CRenderer::CV_r_msaa_quality;
int CRenderer::CV_r_msaa_debug;
float CRenderer::CV_r_msaa_threshold_normal;
float CRenderer::CV_r_msaa_threshold_depth;

int CRenderer::CV_r_BreakOnError;
int CRenderer::CV_r_D3D12SubmissionThread;
int CRenderer::CV_r_ReprojectOnlyStaticObjects;
int CRenderer::CV_r_ReverseDepth;

int CRenderer::CV_r_EnableDebugLayer;
int CRenderer::CV_r_NoDraw;

AllocateConstIntCVar(CRenderer, CV_r_multithreaded);
// CRY DX12
AllocateConstIntCVar(CRenderer, CV_r_multithreadedDrawing);
AllocateConstIntCVar(CRenderer, CV_r_multithreadedDrawingActiveThreshold);

int CRenderer::CV_r_multigpu;
AllocateConstIntCVar(CRenderer, CV_r_texturecompiling);
AllocateConstIntCVar(CRenderer, CV_r_texturecompilingIndicator);
AllocateConstIntCVar(CRenderer, CV_r_TexturesDebugBandwidth);
AllocateConstIntCVar(CRenderer, CV_r_texturesstreaming);
AllocateConstIntCVar(CRenderer, CV_r_TexturesStreamingDebug);
AllocateConstIntCVar(CRenderer, CV_r_texturesstreamingnoupload);
AllocateConstIntCVar(CRenderer, CV_r_texturesstreamingonlyvideo);
int CRenderer::CV_r_texturesstreamingsync;
AllocateConstIntCVar(CRenderer, CV_r_texturesstreamingResidencyEnabled);
AllocateConstIntCVar(CRenderer, CV_r_texturesstreamingUpdateType);
AllocateConstIntCVar(CRenderer, CV_r_texturesstreamingPrecacheRounds);
AllocateConstIntCVar(CRenderer, CV_r_texturesstreamingSuppress);
AllocateConstIntCVar(CRenderer, CV_r_texturesstreamingPostponeMips);
AllocateConstIntCVar(CRenderer, CV_r_texturesstreamingPostponeThresholdKB);
AllocateConstIntCVar(CRenderer, CV_r_texturesstreamingPostponeThresholdMip);
AllocateConstIntCVar(CRenderer, CV_r_texturesstreamingMinReadSizeKB);
int CRenderer::CV_r_texturesstreamingSkipMips;
int CRenderer::CV_r_texturesstreamingMinUsableMips;
int CRenderer::CV_r_texturesstreamingJobUpdate;
#if defined(TEXSTRM_DEFERRED_UPLOAD)
int CRenderer::CV_r_texturesstreamingDeferred;
#endif
#if defined(SUPPORTS_INPLACE_TEXTURE_STREAMING)
int CRenderer::CV_r_texturesstreamingInPlace;
#endif
float CRenderer::CV_r_texturesstreamingResidencyTimeTestLimit;
float CRenderer::CV_r_rain_maxviewdist;
float CRenderer::CV_r_rain_maxviewdist_deferred;
float CRenderer::CV_r_measureoverdrawscale;
float CRenderer::CV_r_texturesstreamingResidencyTime;
float CRenderer::CV_r_texturesstreamingResidencyThrottle;
AllocateConstIntCVar(CRenderer, CV_r_texturesstreamingmipfading);
int CRenderer::CV_r_TexturesStreamPoolSize;
int CRenderer::CV_r_TexturesStreamPoolSecondarySize;
int CRenderer::CV_r_texturesskiplowermips;
int CRenderer::CV_r_rendertargetpoolsize;
float CRenderer::CV_r_TexturesStreamingMaxRequestedMB;
int CRenderer::CV_r_TexturesStreamingMaxRequestedJobs;
float CRenderer::CV_r_TexturesStreamingMipBias;
int CRenderer::CV_r_TexturesStreamingMipClampDVD;
int CRenderer::CV_r_TexturesStreamingDisableNoStreamDuringLoad;
float CRenderer::CV_r_TextureLodDistanceRatio;


int CRenderer::CV_r_buffer_banksize;
int CRenderer::CV_r_constantbuffer_banksize;
int CRenderer::CV_r_constantbuffer_watermark;
int CRenderer::CV_r_buffer_sli_workaround;
int CRenderer::CV_r_transient_pool_size;
AllocateConstIntCVar(CRenderer, CV_r_buffer_enable_lockless_updates);
AllocateConstIntCVar(CRenderer, CV_r_enable_full_gpu_sync);
int CRenderer::CV_r_buffer_pool_max_allocs;
int CRenderer::CV_r_buffer_pool_defrag_static;
int CRenderer::CV_r_buffer_pool_defrag_dynamic;
int CRenderer::CV_r_buffer_pool_defrag_max_moves;

int CRenderer::CV_r_dyntexmaxsize;
int CRenderer::CV_r_dyntexatlascloudsmaxsize;

AllocateConstIntCVar(CRenderer, CV_r_texpostponeloading);
AllocateConstIntCVar(CRenderer, CV_r_texpreallocateatlases);
int CRenderer::CV_r_texatlassize;
int CRenderer::CV_r_texminanisotropy;
int CRenderer::CV_r_texmaxanisotropy;
AllocateConstIntCVar(CRenderer, CV_r_texlog);
AllocateConstIntCVar(CRenderer, CV_r_texnoload);
AllocateConstIntCVar(CRenderer, CV_r_texBlockOnLoad);

AllocateConstIntCVar(CRenderer, CV_r_debugrendermode);
AllocateConstIntCVar(CRenderer, CV_r_debugrefraction);

int CRenderer::CV_r_VRAMDebug;
int CRenderer::CV_r_DebugLightLayers;

int CRenderer::CV_r_DeferredShadingTiled;
int CRenderer::CV_r_DeferredShadingTiledHairQuality;
int CRenderer::CV_r_DeferredShadingTiledDebugDirect;
int CRenderer::CV_r_DeferredShadingTiledDebugIndirect;
int CRenderer::CV_r_DeferredShadingTiledDebugAccumulation;
int CRenderer::CV_r_DeferredShadingTiledDebugAlbedo;
int CRenderer::CV_r_DeferredShadingSSS;
int CRenderer::CV_r_DeferredShadingFilterGBuffer;

int CRenderer::CV_r_UsePersistentRTForModelHUD;

AllocateConstIntCVar(CRenderer, CV_r_deferredshadingLightVolumes);
AllocateConstIntCVar(CRenderer, CV_r_deferredDecals);
AllocateConstIntCVar(CRenderer, CV_r_deferredDecalsDebug);
AllocateConstIntCVar(CRenderer, CV_r_deferredDecalsOnDynamicObjects);

AllocateConstIntCVar(CRenderer, CV_r_deferredshadingDBTstencil);
AllocateConstIntCVar(CRenderer, CV_r_DeferredShadingScissor);
AllocateConstIntCVar(CRenderer, CV_r_DeferredShadingLBuffersFmt);
AllocateConstIntCVar(CRenderer, CV_r_DeferredShadingDepthBoundsTest);
AllocateConstIntCVar(CRenderer, CV_r_DeferredShadingDebug);
AllocateConstIntCVar(CRenderer, CV_r_DeferredShadingDebugGBuffer);
AllocateConstIntCVar(CRenderer, CV_r_DeferredShadingAmbient);
AllocateConstIntCVar(CRenderer, CV_r_DeferredShadingEnvProbes);
AllocateConstIntCVar(CRenderer, CV_r_DeferredShadingAmbientLights);
AllocateConstIntCVar(CRenderer, CV_r_DeferredShadingLights);
AllocateConstIntCVar(CRenderer, CV_r_DeferredShadingAreaLights);
AllocateConstIntCVar(CRenderer, CV_r_DeferredShadingStencilPrepass);
int CRenderer::CV_r_CBufferUseNativeDepth;


float CRenderer::CV_r_DeferredShadingLightLodRatio;
float CRenderer::CV_r_DeferredShadingLightStencilRatio;

int CRenderer::CV_r_HDRDebug;
int CRenderer::CV_r_HDRBloom;
int CRenderer::CV_r_HDRBloomQuality;

int CRenderer::CV_r_ToneMapTechnique;
int CRenderer::CV_r_ColorSpace;
int CRenderer::CV_r_ToneMapExposureType;
float CRenderer::CV_r_ToneMapManualExposureValue;

AllocateConstIntCVar(CRenderer, CV_r_HDRVignetting);
AllocateConstIntCVar(CRenderer, CV_r_HDRTexFormat);

int CRenderer::CV_r_HDRDolbyDynamicMetadata;

int CRenderer::CV_r_HDRDolbyScurve;
float CRenderer::CV_r_HDRDolbyScurveSourceMin;
float CRenderer::CV_r_HDRDolbyScurveSourceMid;
float CRenderer::CV_r_HDRDolbyScurveSourceMax;
float CRenderer::CV_r_HDRDolbyScurveSlope;
float CRenderer::CV_r_HDRDolbyScurveScale;
float CRenderer::CV_r_HDRDolbyScurveRGBPQTargetMin;
float CRenderer::CV_r_HDRDolbyScurveRGBPQTargetMid;
float CRenderer::CV_r_HDRDolbyScurveRGBPQTargetMax;
float CRenderer::CV_r_HDRDolbyScurveVisionTargetMin;
float CRenderer::CV_r_HDRDolbyScurveVisionTargetMid;
float CRenderer::CV_r_HDRDolbyScurveVisionTargetMax;

float CRenderer::CV_r_HDREyeAdaptationSpeed;
int CRenderer::CV_r_HDREyeAdaptationMode;
float CRenderer::CV_r_HDRGrainAmount;

float CRenderer::CV_r_Sharpening;
float CRenderer::CV_r_ChromaticAberration;

AllocateConstIntCVar(CRenderer, CV_r_geominstancing);
AllocateConstIntCVar(CRenderer, CV_r_geominstancingdebug);
AllocateConstIntCVar(CRenderer, CV_r_materialsbatching);
#if defined(WIN32) || defined(WIN64) || defined(APPLE) || defined(LINUX) || defined(USE_SILHOUETTEPOM_CVAR)
int CRenderer::CV_r_SilhouettePOM;
#endif
#ifdef WATER_TESSELLATION_RENDERER
int CRenderer::CV_r_WaterTessellationHW;
#endif
int CRenderer::CV_r_tessellationdebug;
float CRenderer::CV_r_tessellationtrianglesize;
float CRenderer::CV_r_displacementfactor;

int CRenderer::CV_r_batchtype;
int CRenderer::CV_r_geominstancingthreshold;
int CRenderer::m_iGeomInstancingThreshold = 0;        // 0 means not set yet

int CRenderer::CV_r_beams;

AllocateConstIntCVar(CRenderer, CV_r_DebugLightVolumes);

AllocateConstIntCVar(CRenderer, CV_r_UseShadowsPool);

float CRenderer::CV_r_ShadowsBias;
float CRenderer::CV_r_ShadowsAdaptionRangeClamp;
float CRenderer::CV_r_ShadowsAdaptionSize;
float CRenderer::CV_r_ShadowsAdaptionMin;
float CRenderer::CV_r_ShadowsParticleKernelSize;
float CRenderer::CV_r_ShadowsParticleJitterAmount;
float CRenderer::CV_r_ShadowsParticleAnimJitterAmount;
float CRenderer::CV_r_ShadowsParticleNormalEffect;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_CPP_SECTION_16
    #include AZ_RESTRICTED_FILE(Renderer_cpp)
#endif

AllocateConstIntCVar(CRenderer, CV_r_ShadowGenMode);

AllocateConstIntCVar(CRenderer, CV_r_ShadowsUseClipVolume);
AllocateConstIntCVar(CRenderer, CV_r_shadowtexformat);
AllocateConstIntCVar(CRenderer, CV_r_ShadowsMaskResolution);
AllocateConstIntCVar(CRenderer, CV_r_ShadowsMaskDownScale);
AllocateConstIntCVar(CRenderer, CV_r_ShadowsStencilPrePass);
int CRenderer::CV_r_ShadowsDepthBoundNV;
int CRenderer::CV_r_ShadowsPCFiltering;
float CRenderer::CV_r_shadow_jittering;
int CRenderer::CV_r_ShadowPoolMaxTimeslicedUpdatesPerFrame;
int CRenderer::CV_r_ShadowCastingLightsMaxCount;
AllocateConstIntCVar(CRenderer, CV_r_ShadowsGridAligned);
AllocateConstIntCVar(CRenderer, CV_r_ShadowPass);
AllocateConstIntCVar(CRenderer, CV_r_ShadowGen);
AllocateConstIntCVar(CRenderer, CV_r_ShadowPoolMaxFrames);
int CRenderer::CV_r_ShadowsCache;
int CRenderer::CV_r_ShadowsCacheFormat;
int CRenderer::CV_r_ShadowsScreenSpace;
int CRenderer::CV_r_ShadowsNearestMapResolution;
int CRenderer::CV_r_HeightMapAO;
float CRenderer::CV_r_HeightMapAOAmount;
float CRenderer::CV_r_HeightMapAORange;
float CRenderer::CV_r_HeightMapAOResolution;
float   CRenderer::CV_r_RenderMeshHashGridUnitSize;

AllocateConstIntCVar(CRenderer, CV_r_TerrainAO);
AllocateConstIntCVar(CRenderer, CV_r_TerrainAO_FadeDist);

AllocateConstIntCVar(CRenderer, CV_r_debuglights);
AllocateConstIntCVar(CRenderer, CV_r_lightssinglepass);

AllocateConstIntCVar(CRenderer, CV_r_impostersdraw);
float CRenderer::CV_r_imposterratio;
int CRenderer::CV_r_impostersupdateperframe;
AllocateConstIntCVar(CRenderer, CV_r_shaderslazyunload);
AllocateConstIntCVar(CRenderer, CV_r_shadersdebug);
#if !defined(CONSOLE)
int CRenderer::CV_r_shadersorbis;
int CRenderer::CV_r_shadersdx10;
int CRenderer::CV_r_shadersdx11;
int CRenderer::CV_r_shadersGL4;
int CRenderer::CV_r_shadersGLES3;
int CRenderer::CV_r_shadersMETAL;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_CPP_SECTION_14
    #include AZ_RESTRICTED_FILE(Renderer_cpp)
#endif

int CRenderer::CV_r_shadersPlatform;
#endif
AllocateConstIntCVar(CRenderer, CV_r_shadersignoreincludeschanging);
int CRenderer::CV_r_shaderspreactivate;
int CRenderer::CV_r_shadersAllowCompilation;
AllocateConstIntCVar(CRenderer, CV_r_shadersediting);
AllocateConstIntCVar(CRenderer, CV_r_shaderscompileautoactivate);
AllocateConstIntCVar(CRenderer, CV_r_shadersremotecompiler);
int CRenderer::CV_r_shadersasynccompiling;
int CRenderer::CV_r_shadersasyncactivation;
int CRenderer::CV_r_shadersasyncmaxthreads;
int CRenderer::CV_r_shaderscachedeterministic;
AllocateConstIntCVar(CRenderer, CV_r_shadersprecachealllights);
AllocateConstIntCVar(CRenderer, CV_r_ReflectTextureSlots);
int CRenderer::CV_r_shaderssubmitrequestline;
int CRenderer::CV_r_shadersuseinstancelookuptable;
int CRenderer::CV_r_shaderslogcachemisses;
int CRenderer::CV_r_shadersImport;
int CRenderer::CV_r_shadersExport;
int CRenderer::CV_r_shadersCacheUnavailableShaders;
AllocateConstIntCVar(CRenderer, CV_r_ShadersUseLLVMDirectXCompiler);

AllocateConstIntCVar(CRenderer, CV_r_meshprecache);
int CRenderer::CV_r_meshpoolsize;
int CRenderer::CV_r_meshinstancepoolsize;

AllocateConstIntCVar(CRenderer, CV_r_ZPassDepthSorting);
float CRenderer::CV_r_ZPrepassMaxDist;
int CRenderer::CV_r_usezpass;

AllocateConstIntCVar(CRenderer, CV_r_TransparentPasses);
AllocateConstIntCVar(CRenderer, CV_r_TranspDepthFixup);
AllocateConstIntCVar(CRenderer, CV_r_SoftAlphaTest);
AllocateConstIntCVar(CRenderer, CV_r_usehwskinning);
AllocateConstIntCVar(CRenderer, CV_r_usemateriallayers);
AllocateConstIntCVar(CRenderer, CV_r_ParticlesSoftIsec);
AllocateConstIntCVar(CRenderer, CV_r_ParticlesRefraction);
int CRenderer::CV_r_ParticlesHalfRes;
AllocateConstIntCVar(CRenderer, CV_r_ParticlesHalfResAmount);
AllocateConstIntCVar(CRenderer, CV_r_ParticlesHalfResBlendMode);
AllocateConstIntCVar(CRenderer, CV_r_ParticlesInstanceVertices);
float CRenderer::CV_r_ParticlesAmountGI;
int CRenderer::CV_r_ParticlesGpuMaxEmitCount;

int CRenderer::CV_r_AntialiasingMode_CB;
int CRenderer::CV_r_AntialiasingMode;
float CRenderer::CV_r_AntialiasingNonTAASharpening;
int CRenderer::CV_r_AntialiasingTAAJitterPattern;
float CRenderer::CV_r_AntialiasingTAAClampingFactor;
float CRenderer::CV_r_AntialiasingTAANewFrameWeight;
float CRenderer::CV_r_AntialiasingTAASharpening;
AllocateConstIntCVar(CRenderer, CV_r_AntialiasingTAAUseAntiFlickerFilter);
AllocateConstIntCVar(CRenderer, CV_r_AntialiasingTAAUseJitterMipBias);
AllocateConstIntCVar(CRenderer, CV_r_AntialiasingTAAUseVarianceClamping);
AllocateConstIntCVar(CRenderer, CV_r_AntialiasingModeDebug);
AllocateConstIntCVar(CRenderer, CV_r_AntialiasingModeEditor);


Vec2 CRenderer::s_overscanBorders(0, 0);

AllocateConstIntCVar(CRenderer, CV_r_MotionVectors);
AllocateConstIntCVar(CRenderer, CV_r_MotionVectorsTransparency);
AllocateConstIntCVar(CRenderer, CV_r_MotionVectorsDebug);
float CRenderer::CV_r_MotionVectorsTransparencyAlphaThreshold;
int CRenderer::CV_r_MotionBlur;
int CRenderer::CV_r_RenderMotionBlurAfterHDR;
int CRenderer::CV_r_MotionBlurScreenShot;
int CRenderer::CV_r_MotionBlurQuality;
int CRenderer::CV_r_MotionBlurGBufferVelocity;
float CRenderer::CV_r_MotionBlurThreshold;
float CRenderer::CV_r_MotionBlurShutterSpeed;
float CRenderer::CV_r_MotionBlurCameraMotionScale;
float CRenderer::CV_r_MotionBlurMaxViewDist;

AllocateConstIntCVar(CRenderer, CV_r_customvisions);

AllocateConstIntCVar(CRenderer, CV_r_snow);
AllocateConstIntCVar(CRenderer, CV_r_snow_halfres);
AllocateConstIntCVar(CRenderer, CV_r_snow_displacement);
AllocateConstIntCVar(CRenderer, CV_r_snowFlakeClusters);

AllocateConstIntCVar(CRenderer, CV_r_rain);
AllocateConstIntCVar(CRenderer, CV_r_rain_ignore_nearest);
float CRenderer::CV_r_rainamount;
float CRenderer::CV_r_rainDistMultiplier;
float CRenderer::CV_r_rainOccluderSizeTreshold;

int CRenderer::CV_r_SSReflections;
int CRenderer::CV_r_SSReflHalfRes;
int CRenderer::CV_r_ssdo;
int CRenderer::CV_r_ssdoHalfRes;
int CRenderer::CV_r_ssdoColorBleeding;
float CRenderer::CV_r_ssdoRadius;
float CRenderer::CV_r_ssdoRadiusMin;
float CRenderer::CV_r_ssdoRadiusMax;
float CRenderer::CV_r_ssdoAmountDirect;
float CRenderer::CV_r_ssdoAmountAmbient;
float CRenderer::CV_r_ssdoAmountReflection;

AllocateConstIntCVar(CRenderer, CV_r_dof);

AllocateConstIntCVar(CRenderer, CV_r_measureoverdraw);
AllocateConstIntCVar(CRenderer, CV_r_printmemoryleaks);
AllocateConstIntCVar(CRenderer, CV_r_releaseallresourcesonexit);

int   CRenderer::CV_r_rc_autoinvoke;

int CRenderer::CV_r_Refraction;
int CRenderer::CV_r_sunshafts;

AllocateConstIntCVar(CRenderer, CV_r_MergeShadowDrawcalls);

int CRenderer::CV_r_PostProcessReset;
AllocateConstIntCVar(CRenderer, CV_r_PostProcessFilters);
AllocateConstIntCVar(CRenderer, CV_r_PostProcessGameFx);

int CRenderer::CV_r_colorRangeCompression;

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
int CRenderer::CV_r_FinalOutputsRGB;
int CRenderer::CV_r_FinalOutputAlpha;
int CRenderer::CV_r_RTT;
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

int CRenderer::CV_r_colorgrading;
int CRenderer::CV_r_colorgrading_selectivecolor;
AllocateConstIntCVar(CRenderer, CV_r_colorgrading_levels);
AllocateConstIntCVar(CRenderer, CV_r_colorgrading_filters);
int CRenderer::CV_r_colorgrading_charts;
int CRenderer::CV_r_ColorgradingChartsCache;

AllocateConstIntCVar(CRenderer, CV_r_cloudsupdatealways);
AllocateConstIntCVar(CRenderer, CV_r_cloudsdebug);

AllocateConstIntCVar(CRenderer, CV_r_showdyntextures);
int CRenderer::CV_r_ShowDynTexturesMaxCount;
ICVar* CRenderer::CV_r_ShowDynTexturesFilter;
AllocateConstIntCVar(CRenderer, CV_r_shownormals);
AllocateConstIntCVar(CRenderer, CV_r_showlines);
float CRenderer::CV_r_normalslength;
AllocateConstIntCVar(CRenderer, CV_r_showtangents);
AllocateConstIntCVar(CRenderer, CV_r_showtimegraph);
AllocateConstIntCVar(CRenderer, CV_r_DebugFontRendering);
AllocateConstIntCVar(CRenderer, CV_profileStreaming);
AllocateConstIntCVar(CRenderer, CV_r_graphstyle);
AllocateConstIntCVar(CRenderer, CV_r_showbufferusage);

ICVar* CRenderer::CV_r_ShaderCompilerServer;
ICVar* CRenderer::CV_r_ShaderCompilerFolderSuffix;
ICVar* CRenderer::CV_r_ShaderEmailTags;
ICVar* CRenderer::CV_r_ShaderEmailCCs;
int CRenderer::CV_r_ShaderCompilerPort;
int CRenderer::CV_r_ShaderCompilerDontCache;
int CRenderer::CV_r_AssetProcessorShaderCompiler = 0;

int CRenderer::CV_r_flares = FLARES_DEFAULT_VAL;
AllocateConstIntCVar(CRenderer, CV_r_flareHqShafts);
float CRenderer::CV_r_FlaresChromaShift;
int CRenderer::CV_r_FlaresIrisShaftMaxPolyNum;
float CRenderer::CV_r_FlaresTessellationRatio;

int CRenderer::CV_r_envcmresolution;
int CRenderer::CV_r_envtexresolution;
float CRenderer::CV_r_waterupdateFactor;
float CRenderer::CV_r_waterupdateDistance;
float CRenderer::CV_r_envcmupdateinterval;
float CRenderer::CV_r_envtexupdateinterval;
int CRenderer::CV_r_SlimGBuffer;
AllocateConstIntCVar(CRenderer, CV_r_waterreflections);
AllocateConstIntCVar(CRenderer, CV_r_waterreflections_quality);
float CRenderer::CV_r_waterreflections_min_visible_pixels_update;
float CRenderer::CV_r_waterreflections_minvis_updatefactormul;
float CRenderer::CV_r_waterreflections_minvis_updatedistancemul;
int CRenderer::CV_r_watercaustics; //@NOTE: CV_r_watercaustics will be removed when the infinite ocean component feature toggle is removed.
float CRenderer::CV_r_watercausticsdistance;
int CRenderer::CV_r_watervolumecaustics;
int CRenderer::CV_r_watervolumecausticsdensity;
int CRenderer::CV_r_watervolumecausticsresolution;
float CRenderer::CV_r_watervolumecausticssnapfactor;
float CRenderer::CV_r_watervolumecausticsmaxdistance;
AllocateConstIntCVar(CRenderer, CV_r_water_godrays);
float CRenderer::CV_r_water_godrays_distortion;
AllocateConstIntCVar(CRenderer, CV_r_texNoAnisoAlphaTest);
AllocateConstIntCVar(CRenderer, CV_r_reflections);
AllocateConstIntCVar(CRenderer, CV_r_reflections_quality);
float CRenderer::CV_r_waterreflections_offset;
AllocateConstIntCVar(CRenderer, CV_r_reloadshaders);
AllocateConstIntCVar(CRenderer, CV_r_detailtextures);
float CRenderer::CV_r_detaildistance;
AllocateConstIntCVar(CRenderer, CV_r_texbindmode);
AllocateConstIntCVar(CRenderer, CV_r_nodrawshaders);
int CRenderer::CV_r_nodrawnear;
float CRenderer::CV_r_DrawNearZRange;
float CRenderer::CV_r_DrawNearFarPlane;
float CRenderer::CV_r_drawnearfov;
int CRenderer::CV_r_DrawNearShadows;
AllocateConstIntCVar(CRenderer, CV_r_profileshaders);
AllocateConstIntCVar(CRenderer, CV_r_ProfileShadersSmooth);
AllocateConstIntCVar(CRenderer, CV_r_ProfileShadersGroupByName);
ICVar* CRenderer::CV_r_excludeshader;
ICVar* CRenderer::CV_r_excludemesh;

float CRenderer::CV_r_gamma;
float CRenderer::CV_r_contrast;
float CRenderer::CV_r_brightness;

AllocateConstIntCVar(CRenderer, CV_r_nohwgamma);

int CRenderer::CV_r_scissor;

AllocateConstIntCVar(CRenderer, CV_r_wireframe);
int CRenderer::CV_r_GetScreenShot;

AllocateConstIntCVar(CRenderer, CV_r_character_nodeform);

AllocateConstIntCVar(CRenderer, CV_r_ZPassOnly);

int CRenderer::CV_r_ShowVideoMemoryStats;

ICVar* CRenderer::CV_r_ShowTexture = NULL;
ICVar* CRenderer::CV_r_TexturesStreamingDebugfilter = NULL;
int CRenderer::CV_r_TexturesStreamingDebugMinSize;
int CRenderer::CV_r_TexturesStreamingDebugMinMip;
AllocateConstIntCVar(CRenderer, CV_r_TexturesStreamingDebugDumpIntoLog);

AllocateConstIntCVar(CRenderer, CV_r_ShowLightBounds);

AllocateConstIntCVar(CRenderer, CV_r_MergeRenderChunks);
AllocateConstIntCVar(CRenderer, CV_r_TextureCompressor);

int CRenderer::CV_r_ParticlesTessellation;
int CRenderer::CV_r_ParticlesTessellationTriSize;

float CRenderer::CV_r_ZFightingDepthScale;
float CRenderer::CV_r_ZFightingExtrude;

float CRenderer::CV_r_TexelsPerMeter;
float CRenderer::s_previousTexelsPerMeter = -1.0f;

int CRenderer::CV_r_ConditionalRendering;
int CRenderer::CV_r_enableAltTab;
int CRenderer::CV_r_StereoDevice;
int CRenderer::CV_r_StereoMode;
int CRenderer::CV_r_StereoOutput;
int CRenderer::CV_r_StereoFlipEyes;
float CRenderer::CV_r_StereoStrength;
float CRenderer::CV_r_StereoEyeDist;
float CRenderer::CV_r_StereoScreenDist;
float CRenderer::CV_r_StereoNearGeoScale;
float CRenderer::CV_r_StereoHudScreenDist;
float CRenderer::CV_r_StereoGammaAdjustment;
int CRenderer::CV_r_ConsoleBackbufferWidth;
int CRenderer::CV_r_ConsoleBackbufferHeight;

// constant used to indicate that CustomResMax should be set to the maximum allowable by device resources
const int CRenderer::s_CustomResMaxSize_USE_MAX_RESOURCES = -1;
int CRenderer::CV_r_CustomResMaxSize;
int CRenderer::CV_r_CustomResWidth;
int CRenderer::CV_r_CustomResHeight;
int CRenderer::CV_r_CustomResPreview;
int CRenderer::CV_r_Supersampling;
int CRenderer::CV_r_SupersamplingFilter;

float CRenderer::CV_r_FogDepthTest;
#if defined(VOLUMETRIC_FOG_SHADOWS)
int CRenderer::CV_r_FogShadows;
int CRenderer::CV_r_FogShadowsMode;
#endif
int CRenderer::CV_r_FogShadowsWater;

AllocateConstIntCVar(CRenderer, CV_r_RainDropsEffect);

AllocateConstIntCVar(CRenderer, CV_r_RefractionPartialResolves);
AllocateConstIntCVar(CRenderer, CV_r_RefractionPartialResolvesDebug);

AllocateConstIntCVar(CRenderer, CV_r_Batching);

AllocateConstIntCVar(CRenderer, CV_r_Unlit);
AllocateConstIntCVar(CRenderer, CV_r_HideSunInCubemaps);

AllocateConstIntCVar(CRenderer, CV_r_ParticlesDebug);

// Confetti David Srour: Upscaling Quality for Metal
AllocateConstIntCVar(CRenderer, CV_r_UpscalingQuality);

//Clears GMEM G-Buffer
AllocateConstIntCVar(CRenderer, CV_r_ClearGMEMGBuffer);

// Enables fast math for metal shaders
AllocateConstIntCVar(CRenderer, CV_r_MetalShadersFastMath);

int CRenderer::CV_r_CubeDepthMapResolution;

int CRenderer::CV_r_SkipNativeUpscale;
int CRenderer::CV_r_SkipRenderComposites;

// Confetti David Srour: Global VisArea/Portals blend weight for GMEM path
float CRenderer::CV_r_GMEMVisAreasBlendWeight;

// Confetti David Srour: 0 = disable, 1= 256bpp GMEM path, 2 = 128bpp GMEM path
int CRenderer::CV_r_EnableGMEMPath;

// Confetti David Srour: 0 = regular postproc, 1 = mobile pipeline with compute
int CRenderer::CV_r_EnableGMEMPostProcCS;

// Confetti David Srour: Used to reduce draw calls during DOF's gathers
int CRenderer::CV_r_GMEM_DOF_Gather1_Quality;
int CRenderer::CV_r_GMEM_DOF_Gather2_Quality;

int CRenderer::CV_r_RainUseStencilMasking;

// Confetti Thomas Zeng: 0 = diable, 1 = enable
int CRenderer::CV_r_EnableComputeDownSampling;

// Confetti Vera
float CRenderer::CV_r_CubeDepthMapFarPlane;

int CRenderer::CV_r_ForceFixedPointRenderTargets;

// Fur control parameters
int CRenderer::CV_r_Fur;
int CRenderer::CV_r_FurShellPassCount;
int CRenderer::CV_r_FurShowBending;
int CRenderer::CV_r_FurDebug;
int CRenderer::CV_r_FurDebugOneShell;
int CRenderer::CV_r_FurFinPass;
int CRenderer::CV_r_FurFinShadowPass;
float CRenderer::CV_r_FurMovementBendingBias;
float CRenderer::CV_r_FurMaxViewDist;

#if defined(ENABLE_RENDER_AUX_GEOM)
int CRenderer::CV_r_enableauxgeom;
#endif

int CRenderer::CV_r_ParticleVerticePoolSize;
int CRenderer::CV_r_GeomCacheInstanceThreshold;
int CRenderer::CV_r_VisAreaClipLightsPerPixel;

int CRenderer::CV_r_VolumetricFog = 0;
int CRenderer::CV_r_VolumetricFogTexScale;
int CRenderer::CV_r_VolumetricFogTexDepth;
float CRenderer::CV_r_VolumetricFogReprojectionBlendFactor;
int CRenderer::CV_r_VolumetricFogSample;
int CRenderer::CV_r_VolumetricFogShadow;
int CRenderer::CV_r_VolumetricFogDownscaledSunShadow;
int CRenderer::CV_r_VolumetricFogDownscaledSunShadowRatio;
int CRenderer::CV_r_VolumetricFogReprojectionMode;
float CRenderer::CV_r_VolumetricFogMinimumLightBulbSize;
float CRenderer::CV_r_ResolutionScale = 1.0f;
int CRenderer::CV_r_OutputShaderSourceFiles = 0;

// Specular antialiasing
int CRenderer::CV_r_SpecularAntialiasing = 1;

// Console
float CRenderer::CV_r_minConsoleFontSize;
float CRenderer::CV_r_maxConsoleFontSize;

// Linux
int CRenderer::CV_r_linuxSkipWindowCreation = 0;

// Graphics programmers: Use these in your code for local tests/debugging.
// Delete all references in your code before you submit
int CRenderer::CV_r_GraphicsTest00;
int CRenderer::CV_r_GraphicsTest01;
int CRenderer::CV_r_GraphicsTest02;
int CRenderer::CV_r_GraphicsTest03;
int CRenderer::CV_r_GraphicsTest04;
int CRenderer::CV_r_GraphicsTest05;
int CRenderer::CV_r_GraphicsTest06;
int CRenderer::CV_r_GraphicsTest07;
int CRenderer::CV_r_GraphicsTest08;
int CRenderer::CV_r_GraphicsTest09;

//////////////////////////////////////////////////////////////////////

#if !defined(CONSOLE) && !defined(NULL_RENDERER)

static void ShadersPrecacheList([[maybe_unused]] IConsoleCmdArgs* Cmd)
{
    gRenDev->m_cEF.mfPrecacheShaders(false);
}
static void ShadersStatsList([[maybe_unused]] IConsoleCmdArgs* Cmd)
{
    gRenDev->m_cEF.mfPrecacheShaders(true);
}
static void GetShaderList([[maybe_unused]] IConsoleCmdArgs* Cmd)
{
    gRenDev->m_cEF.mfGetShaderList();
}

template<typename CallableT>
void ShadersOptimizeHelper(CallableT setupParserBin, const char* logString)
{
    setupParserBin();
    CryLogAlways("\nStarting shaders optimizing for %s...", logString);
    AZStd::string str = "@usercache@/" + gRenDev->m_cEF.m_ShadersCache;
    iLog->Log("Optimize shader cache folder: '%s'", gRenDev->m_cEF.m_ShadersCache.c_str());
    gRenDev->m_cEF.mfOptimiseShaders(str.c_str(), false);
}

static void ShadersOptimise([[maybe_unused]] IConsoleCmdArgs* Cmd)
{
    if (CRenderer::CV_r_shadersdx11)
    {
        ShadersOptimizeHelper(CParserBin::SetupForD3D11, "DX11");
    }
    if (CRenderer::CV_r_shadersGL4)
    {
        ShadersOptimizeHelper(CParserBin::SetupForGL4, "GLSL 4");
    }
    if (CRenderer::CV_r_shadersGLES3)
    {
        ShadersOptimizeHelper(CParserBin::SetupForGLES3, " GLSL-ES 3");
    }
    if (CRenderer::CV_r_shadersorbis)
    {
        ShadersOptimizeHelper(CParserBin::SetupForOrbis, "Orbis");
    }
    if (CRenderer::CV_r_shadersMETAL)
    {
        ShadersOptimizeHelper(CParserBin::SetupForMETAL, "METAL");
    }
}

#endif

static void OnChange_CV_r_PostProcess(ICVar* pCVar)
{
    if (!pCVar)
    {
        return;
    }

    if (gRenDev->m_pRT && !gRenDev->m_pRT->IsRenderThread())
    {
        gRenDev->m_pRT->FlushAndWait();
    }

    gRenDev->CV_r_PostProcess = pCVar->GetIVal();
}

// Track all AA conditions/dependencies in one place. Set corresponding cvars.
static void OnChange_CV_r_AntialiasingMode(ICVar* pCVar)
{
    if (!pCVar)
    {
        return;
    }

    if (gRenDev->m_pRT && !gRenDev->m_pRT->IsRenderThread())
    {
        gRenDev->m_pRT->FlushAndWait();
    }


    int32 nVal = pCVar->GetIVal();
    nVal = min(eAT_AAMODES_COUNT - 1, nVal);
#if defined(OPENGL_ES)
    if ((nVal == static_cast<int32>(eAT_SMAA1TX)) || (nVal == static_cast<int32>(eAT_TAA)))
    {
        AZ_Warning("Rendering", false, "SMAA and TAA are not supported on this platform. Fallback to FXAA");
        nVal = eAT_FXAA;
    }
#endif

#if defined (CRY_USE_METAL) || defined (OPENGL_ES)
    // We don't support switching to 128bpp after initialization of the gmem path.
    if (CD3D9Renderer::EGmemPath::eGT_256bpp_PATH == gcpRendD3D->FX_GetEnabledGmemPath(nullptr) && nVal == static_cast<int32>(eAT_TAA))
    {
        AZ_Warning("Rendering", pCVar->GetIVal() == 0, "TAA is not supported on 256bpp mode. Either switch to 128bpp or enable TAA at init so that the correct gmem mode is picked during initialization");
        nVal = eAT_FXAA;
    }
#endif

    ICVar* pMSAA = gEnv->pConsole->GetCVar("r_MSAA");
    ICVar* pMSAASamples = gEnv->pConsole->GetCVar("r_MSAA_samples");
    AZ_Assert(pMSAA, "r_MSAA is not a valid cvar");
    AZ_Assert(pMSAASamples, "r_MSAA_samples is not a valid cvar");
    pMSAA->Set(0);
    pMSAASamples->Set(0);

    pCVar->Set(nVal);
    gRenDev->CV_r_AntialiasingMode = nVal;
}

static const char* showRenderTargetHelp =
    "Displays render targets - for debug purpose\n"
    "[Usage]\n"
    "r_ShowRenderTarget -l : list all available render targets\n"
    "r_ShowRenderTarget -l hdr : list all available render targets whose name contain 'hdr'\n"
    "r_ShowRenderTarget -nf zpass : show any render targets whose name contain 'zpass' with no filtering in 2x2(default) table\n"
    "r_ShowRenderTarget -c:3 pass : show any render targets whose name contain 'pass' in 3x3 table\n"
    "r_ShowRenderTarget z hdr : show any render targets whose name contain either 'z' or 'hdr'\n"
    "r_ShowRenderTarget scene:rg scene:b : show any render targets whose name contain 'scene' first with red-green channels only and then with a blue channel only\n"
    "r_ShowRenderTarget scenetarget:rgba:2 : show any render targets whose name contain 'scenetarget' with all channels multiplied by 2\n"
    "r_ShowRenderTarget scene:b hdr:a : show any render targets whose name contain 'scene' with a blue channel only and ones whose name contain 'hdr' with an alpha channel only\n"
    "r_ShowRenderTarget -e $ztarget : show a render target whose name exactly matches '$ztarget'\n"
    "r_ShowRenderTarget -s scene : separately shows each channel of any 'scene' render targets\n"
    "r_ShowRenderTarget -k scene : shows any 'scene' render targets with RGBK decoding\n"
    "r_ShowRenderTarget -a scene : shows any 'scene' render targets with 101110/8888 aliasing";

void CRenderer::Cmd_ShowRenderTarget(IConsoleCmdArgs* pArgs)
{
    int argCount = pArgs->GetArgCount();

    gRenDev->m_showRenderTargetInfo.Reset();

    if (argCount <= 1)
    {
        string help = showRenderTargetHelp;
        int curPos = 0;
        string line = help.Tokenize("\n", curPos);
        while (false == line.empty())
        {
            gEnv->pLog->Log(line);
            line = help.Tokenize("\n", curPos);
        }
        return;
    }

    // Check for '-l'.
    for (int i = 1; i < argCount; ++i)
    {
        if (strcmp(pArgs->GetArg(i), "-l") == 0)
        {
            gRenDev->m_showRenderTargetInfo.bShowList = true;
            break;
        }
    }

    // Check for '-c:*'.
    for (int i = 1; i < argCount; ++i)
    {
        if (strlen(pArgs->GetArg(i)) > 3 && strncmp(pArgs->GetArg(i), "-c:", 3) == 0)
        {
            gRenDev->m_showRenderTargetInfo.col = atoi(pArgs->GetArg(i) + 3);
            if (gRenDev->m_showRenderTargetInfo.col <= 0)
            {
                gRenDev->m_showRenderTargetInfo.col = 2;
            }
        }
    }

    // Now gather all render targets.
    std::vector<CTexture*> allRTs;
    SResourceContainer* pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
    ResourcesMapItor it;
    for (it = pRL->m_RMap.begin(); it != pRL->m_RMap.end(); ++it)
    {
        CTexture* tp = (CTexture*)it->second;
        if (tp && !tp->IsNoTexture())
        {
            if ((tp->GetFlags() & (FT_USAGE_RENDERTARGET | FT_USAGE_DYNAMIC)) && tp->GetDevTexture())
            {
                allRTs.push_back(tp);
            }
        }
    }

    // Process actual arguments with possible '-nf', '-f', '-e' options.
    bool bNoRegularArgs = true;
    bool bFiltered = true;
    bool bExactMatch = false;
    bool bRGBKEncoded = false;
    bool bAliased = false;
    bool bWeightedChannels = false;
    bool bSplitChannels = false;

    for (int i = 1; i < argCount; ++i)
    {
        const char* pCurArg = pArgs->GetArg(i);

        bool bColOption = strlen(pCurArg) > 3 && strncmp(pCurArg, "-c:", 3) == 0;
        if (strcmp(pCurArg, "-l") == 0 || bColOption)
        {
            continue;
        }

        if (strcmp(pCurArg, "-nf") == 0)
        {
            bFiltered = false;
        }
        else if (strcmp(pCurArg, "-f") == 0)
        {
            bFiltered = true;
        }
        else if (strcmp(pCurArg, "-e") == 0)
        {
            bExactMatch = true;
        }
        else if (strcmp(pCurArg, "-k") == 0)
        {
            bRGBKEncoded = true;
        }
        else if (strcmp(pCurArg, "-a") == 0)
        {
            bAliased = true;
        }
        else if (strcmp(pCurArg, "-s") == 0)
        {
            bSplitChannels = true;
        }
        else
        {
            bNoRegularArgs = false;
            string argTxt = pCurArg, nameTxt, channelTxt, mulTxt;
            argTxt.MakeLower();
            float multiplier = 1.0f;
            size_t pos = argTxt.find(':');
            if (pos == string::npos)
            {
                nameTxt = argTxt;
                channelTxt = "rgba";
            }
            else
            {
                nameTxt = argTxt.substr(0, pos);
                channelTxt = argTxt.substr(pos + 1, string::npos);
                pos = channelTxt.find(':');
                if (pos != string::npos)
                {
                    mulTxt = channelTxt.substr(pos + 1, string::npos);
                    multiplier = static_cast<float>(atof(mulTxt.c_str()));
                    if (multiplier <= 0)
                    {
                        multiplier = 1.0f;
                    }
                }
                bWeightedChannels = true;
            }

            Vec4 channelWeight(0, 0, 0, 0);
            if (channelTxt.find('r') != string::npos)
            {
                channelWeight.x = 1.0f;
            }
            if (channelTxt.find('g') != string::npos)
            {
                channelWeight.y = 1.0f;
            }
            if (channelTxt.find('b') != string::npos)
            {
                channelWeight.z = 1.0f;
            }
            if (channelTxt.find('a') != string::npos)
            {
                channelWeight.w = 1.0f;
            }

            channelWeight *= multiplier;

            for (size_t k = 0; k < allRTs.size(); ++k)
            {
                string texName = allRTs[k]->GetName();
                texName.MakeLower();
                bool bMatch = false;
                if (bExactMatch)
                {
                    bMatch = texName == nameTxt;
                }
                else
                {
                    bMatch = texName.find(nameTxt.c_str()) != string::npos;
                }
                if (bMatch)
                {
                    SShowRenderTargetInfo::RT rt;
                    rt.bFiltered = bFiltered;
                    rt.bRGBKEncoded = bRGBKEncoded;
                    rt.bAliased = bAliased;
                    rt.textureID = allRTs[k]->GetID();
                    rt.channelWeight = channelWeight;

                    if (bSplitChannels)
                    {
                        const Vec4 channels[4] = { Vec4(1, 0, 0, 0), Vec4(0, 1, 0, 0), Vec4(0, 0, 1, 0), Vec4(0, 0, 0, 1) };

                        for (int j = 0; j < 4; ++j)
                        {
                            rt.channelWeight = bWeightedChannels ? channelWeight : Vec4(1, 1, 1, 1);
                            rt.channelWeight.x *= channels[j].x;
                            rt.channelWeight.y *= channels[j].y;
                            rt.channelWeight.z *= channels[j].z;
                            rt.channelWeight.w *= channels[j].w;

                            if (rt.channelWeight[j] > 0.0f)
                            {
                                gRenDev->m_showRenderTargetInfo.rtList.push_back(rt);
                            }
                        }
                    }
                    else
                    {
                        gRenDev->m_showRenderTargetInfo.rtList.push_back(rt);
                    }
                }
            }
        }
    }

    if (bNoRegularArgs && gRenDev->m_showRenderTargetInfo.bShowList) // This means showing all items.
    {
        for (size_t k = 0; k < allRTs.size(); ++k)
        {
            SShowRenderTargetInfo::RT rt;
            rt.bFiltered = true; // Doesn't matter, actually.
            rt.textureID = allRTs[k]->GetID();
            gRenDev->m_showRenderTargetInfo.rtList.push_back(rt);
        }
    }
}

static void cmd_OverscanBorders(IConsoleCmdArgs* pParams)
{
    int argCount = pParams->GetArgCount();

    if (argCount > 1)
    {
        CRenderer::s_overscanBorders.x = clamp_tpl((float)atof(pParams->GetArg(1)), 0.0f, 25.0f) * 0.01f;

        if (argCount > 2)
        {
            CRenderer::s_overscanBorders.y = clamp_tpl((float)atof(pParams->GetArg(2)), 0.0f, 25.0f) * 0.01f;
        }
        else
        {
            CRenderer::s_overscanBorders.y = CRenderer::s_overscanBorders.x;
        }
    }
    else
    {
        gEnv->pLog->LogWithType(ILog::eInputResponse, "Overscan Borders: Left/Right %G %% , Top/Bottom %G %%",
            CRenderer::s_overscanBorders.x * 100.0f, CRenderer::s_overscanBorders.y * 100.0f);
    }
}

static void OnChange_r_OverscanBorderScale([[maybe_unused]] ICVar* pCVar)
{
    const float maxOverscanBorderScale = 0.5f;
    CRenderer::s_overscanBorders.x = clamp_tpl(CRenderer::s_overscanBorders.x, 0.0f, maxOverscanBorderScale);
    CRenderer::s_overscanBorders.y = clamp_tpl(CRenderer::s_overscanBorders.y, 0.0f, maxOverscanBorderScale);
}

static void OnChange_CV_r_CubeDepthMapResolution(ICVar* /*pCVar*/)
{
}

static void OnChange_CV_r_SkipRenderComposites(ICVar* pCVar)
{
    int value = pCVar->GetIVal();

    AZ_Warning("Rendering", value == 0 || (value == 1 && CRenderer::CV_r_flares == 0), "r_SkipRenderComposites was set to 1 while r_Flares was enabled, setting r_Flares to 0.");
    CRenderer::CV_r_flares = 0;
}

static void OnChange_CV_r_DebugLightLayers(ICVar* pCVar)
{
    int value = pCVar->GetIVal();

    CRenderer::CV_r_DeferredShadingTiledDebugDirect = 0;
    CRenderer::CV_r_DeferredShadingTiledDebugIndirect = 0;
    CRenderer::CV_r_DeferredShadingTiledDebugAlbedo = 0;
    CRenderer::CV_r_DeferredShadingTiledDebugAccumulation = 0;

    // Reset HDR to defaults
    CRenderer::CV_r_HDRDebug = 0;

    ICVar* pFogVar = gEnv->pConsole->GetCVar("e_Fog");
    AZ_Assert(pFogVar, "Fog CVar is missing");
    pFogVar->Set(1);

    enum TiledDebugIndirect
    {
        TILED_DEBUG_INDIRECT_NONE = 3,
        TILED_DEBUG_INDIRECT_DIFF = 2,
        TILED_DEBUG_INDIRECT_DIFF_SPEC = 1
    };

    enum TiledDebugAccum
    {
        TILED_DEBUG_ACCUM_DIFF = 1
    };

    enum DebugLayer
    {
        DEBUG_LAYER_DIRECT_DIFFUSE = 1,
        DEBUG_LAYER_INDIRECT_DIFFUSE,
        DEBUG_LAYER_SPECULAR,
        DEBUG_LAYER_AO,
        DEBUG_LAYER_TEXTURES,
        DEBUG_LAYER_FOG,
        DEBUG_LAYER_HDR
    };

    if (value >= DEBUG_LAYER_DIRECT_DIFFUSE)
    {
        CRenderer::CV_r_DeferredShadingTiledDebugIndirect = TILED_DEBUG_INDIRECT_NONE;
        CRenderer::CV_r_DeferredShadingTiledDebugAlbedo = 1;
        CRenderer::CV_r_DeferredShadingTiledDebugAccumulation = TILED_DEBUG_ACCUM_DIFF;

        pFogVar->Set(0);

        CRenderer::CV_r_HDRDebug = 1;
        CRenderer::CV_r_HDREyeAdaptationMode = 1;
    }
    if (value >= DEBUG_LAYER_INDIRECT_DIFFUSE)
    {
        CRenderer::CV_r_DeferredShadingTiledDebugIndirect = TILED_DEBUG_INDIRECT_DIFF;
    }
    if (value >= DEBUG_LAYER_SPECULAR)
    {
        CRenderer::CV_r_DeferredShadingTiledDebugIndirect = TILED_DEBUG_INDIRECT_DIFF_SPEC;
        CRenderer::CV_r_DeferredShadingTiledDebugAccumulation = 0;
    }
    if (value >= DEBUG_LAYER_AO)
    {
        CRenderer::CV_r_DeferredShadingTiledDebugIndirect = 0;
    }
    if (value >= DEBUG_LAYER_TEXTURES)
    {
        CRenderer::CV_r_DeferredShadingTiledDebugAlbedo = 0;
    }
    if (value >= DEBUG_LAYER_FOG)
    {
        pFogVar->Set(1);
    }
    if (value >= DEBUG_LAYER_HDR)
    {
        CRenderer::CV_r_HDRDebug = 0;
    }
}

static void OnChange_CV_r_DeferredShadingTiled([[maybe_unused]] ICVar* pCVar)
{
#if defined (AZ_PLATFORM_MAC)
    // We don't support deferred shading tiled on macOS yet so always force the cvar to 0
    AZ_Warning("Rendering", pCVar->GetIVal() == 0, "Deferred Shading Tiled is not supported on macOS");
    CRenderer::CV_r_DeferredShadingTiled = 0;
#elif defined(OPENGL)
    // We don't support deferred shading tiled on any OpenGL targets yet so always force the cvar to 0
    AZ_Warning("Rendering", pCVar->GetIVal() == 0, "Deferred Shading Tiled is not supported when using OpenGL");
    CRenderer::CV_r_DeferredShadingTiled = 0;
#endif
}

static void OnChange_CV_r_Fur([[maybe_unused]] ICVar* pCVar)
{

#if defined (CRY_USE_METAL) || defined (OPENGL_ES)
    // We don't support fur on gmem/pls path yet so always force the cvar to 0
    if (CD3D9Renderer::EGmemPath::eGT_REGULAR_PATH != gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        AZ_Warning("Rendering", pCVar->GetIVal() == 0, "Fur is not supported on gmem/pls for mobile");
        CRenderer::CV_r_Fur = 0;
    }
#endif
}

static void OnChange_CV_r_SunShafts([[maybe_unused]] ICVar* pCVar)
{
#if defined (AZ_PLATFORM_MAC)
    // We don't support sunshaft settings greater than 1 on macOS yet so always force the cvar to 1
    AZ_Warning("Rendering", pCVar->GetIVal() > 1, "Sunshaft value settings above 1 are not supported on macOS");
    if (pCVar->GetIVal() >= 1)
    {
        CRenderer::CV_r_sunshafts = 1;
    }
    else
    {
        CRenderer::CV_r_sunshafts = 0;
    }
#endif
}

static void OnChange_CV_r_SSDO([[maybe_unused]] ICVar* pCVar)
{
    
#if defined (CRY_USE_METAL) || defined (OPENGL_ES)
    // We don't support switching to 128bpp after initialization of the gmem path.
    if (CD3D9Renderer::EGmemPath::eGT_256bpp_PATH == gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        AZ_Warning("Rendering", pCVar->GetIVal() == 0, "SSDO is not supported on 256bpp mode. Either switch to 128bpp or enable r_ssdo at init so that the correct gmem mode is picked during initialization");
        CRenderer::CV_r_ssdo = 0;
    }
#endif
}

static void OnChange_CV_r_SSReflections([[maybe_unused]] ICVar* pCVar)
{

#if defined (CRY_USE_METAL) || defined (OPENGL_ES)
    // We don't support switching to 128bpp after initialization of the gmem path.
    if (CD3D9Renderer::EGmemPath::eGT_256bpp_PATH == gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        AZ_Warning("Rendering", pCVar->GetIVal() == 0, "SSReflections are not supported on 256bpp mode. Either switch to 128bpp or enable r_SSReflections at init so that the correct gmem mode is picked during initialization");
        CRenderer::CV_r_SSReflections = 0;
    }
#endif
}

static void OnChange_CV_r_MotionBlur([[maybe_unused]] ICVar* pCVar)
{

#if defined (CRY_USE_METAL) || defined (OPENGL_ES)
    // We don't support switching to 128bpp after initialization of the gmem path.
    if (CD3D9Renderer::EGmemPath::eGT_256bpp_PATH == gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        AZ_Warning("Rendering", pCVar->GetIVal() == 0, "MotionBlur is not supported on 256bpp mode. Either switch to 128bpp or enable r_MotionBlur at init so that the correct gmem mode is picked during initialization");
        CRenderer::CV_r_MotionBlur = 0;
    }
#endif
}

static void OnChange_CV_r_TexelsPerMeter(ICVar* pCVar)
{
    if (pCVar && pCVar->GetFVal() == CRenderer::s_previousTexelsPerMeter)
    {
        CRenderer::CV_r_TexelsPerMeter = 0.0f;
    }

    ICVar* pCV_e_sketch_mode = gEnv->pConsole->GetCVar("e_sketch_mode");
    if (pCV_e_sketch_mode)
    {
        pCV_e_sketch_mode->Set(CRenderer::CV_r_TexelsPerMeter > 0.0f ? 4 : 0);
    }
    CRenderer::s_previousTexelsPerMeter = CRenderer::CV_r_TexelsPerMeter;
}

static void OnChange_CV_r_ShadersAllowCompiliation([[maybe_unused]] ICVar* pCVar)
{
    // disable async activation. Can be a problem though if some shader cache files were opened async/streamed
    // before this.
    CRenderer::CV_r_shadersasyncactivation = 0;
    CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, "Changing r_ShadersAllowCompilation at runtime can cause problems. Please set it in your system.cfg or user.cfg instead.");
}

static void OnChange_CV_r_flares([[maybe_unused]] ICVar* pCVar)
{
    AZ_Warning("Rendering", pCVar->GetIVal() == 0 || (pCVar->GetIVal() == 1 && CRenderer::CV_r_SkipRenderComposites == 0), "r_SkipRenderComposites is set to 1, r_flares will have no effect.");
}

static void OnChange_CV_r_FlaresTessellationRatio([[maybe_unused]] ICVar* pCVar)
{
    gEnv->pOpticsManager->Invalidate();
}

static void GetLogVBuffersStatic([[maybe_unused]] ICVar* pCVar)
{
    gRenDev->GetLogVBuffers();
}

static void OnChangeShadowJitteringCVar(ICVar* pCVar)
{
    gRenDev->SetShadowJittering(pCVar->GetFVal());
}

static void OnChange_CachedShadows([[maybe_unused]] ICVar* pCVar)
{
    CTexture::GenerateCachedShadowMaps();
    if (gEnv && gEnv->p3DEngine)
    {
        gEnv->p3DEngine->SetShadowsGSMCache(true);
        gEnv->p3DEngine->SetRecomputeCachedShadows(ShadowMapFrustum::ShadowCacheData::eFullUpdate);
    }
}

void CRenderer::ChangeGeomInstancingThreshold([[maybe_unused]] ICVar* pVar)
{
    // get user value
    m_iGeomInstancingThreshold = CV_r_geominstancingthreshold;

    // auto
    if (m_iGeomInstancingThreshold < 0)
    {
        int nGPU = gRenDev->GetFeatures() & RFT_HW_MASK;

        if (nGPU == RFT_HW_ATI)
        {
            CRenderer::m_iGeomInstancingThreshold = 2;              // seems to help in performance on all cards
        }
        else
        if (nGPU == RFT_HW_NVIDIA)
        {
            CRenderer::m_iGeomInstancingThreshold = 8;                  //
        }
        else
        {
            CRenderer::m_iGeomInstancingThreshold = 7;                  // might be a good start - can be tweaked further
        }
    }

    iLog->Log(" Used GeomInstancingThreshold is %d", m_iGeomInstancingThreshold);
}

RendererAssetListener::RendererAssetListener(IRenderer* renderer)
    : m_renderer(renderer)
{
}

void RendererAssetListener::Connect()
{
    BusConnect(AZ_CRC("dds", 0x780234cb));
    BusConnect(AZ_CRC("cgf", 0x3bbd9566));
    BusConnect(AZ_CRC("cfx", 0xd8a99944));
    BusConnect(AZ_CRC("cfi", 0xb219b9b6));
}

void RendererAssetListener::Disconnect()
{
    BusDisconnect();
}

void RendererAssetListener::OnFileChanged(AZStd::string assetName)
{
    // Do not pass on resource updates until the engine is up and running
    if (gEnv->pSystem && gEnv->p3DEngine)
    {
        m_renderer->EF_ReloadFile_Request(assetName.c_str());
    }
}

CRenderer::CRenderer()
    : m_assetListener(this)
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED && !defined(NULL_RENDERER)
    , m_contextManager(new AzRTT::RenderContextManager())
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED && !defined(NULL_RENDERER)
{
    static_assert(LegacyInternal::JobExecutorPool::NumPools == AZ_ARRAY_SIZE(CRenderer::m_SkinningDataPool), "JobExecutorPool and Skinning data pool size mismatch");
    CCryNameR::CreateNameTable();
}

void CRenderer::InitRenderer()
{
    if (!gRenDev)
    {
        gRenDev = this;
    }

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_CPP_SECTION_4
    #include AZ_RESTRICTED_FILE(Renderer_cpp)
#endif

    m_cEF.m_Bin.m_pCEF = &m_cEF;

    m_bDualStereoSupport = false;

    m_bShaderCacheGen = false;
    m_bSystemResourcesInit = 0;

    m_bSystemTargetsInit = 0;
    m_bIsWindowActive = true;

    m_bShadowsEnabled = true;
    m_bCloudShadowsEnabled = true;

#if defined(VOLUMETRIC_FOG_SHADOWS)
    m_bVolFogShadowsEnabled = false;
    m_bVolFogCloudShadowsEnabled = false;
#endif

    m_nDisableTemporalEffects = 0;

    m_nPoolIndex = 0;
    m_nPoolIndexRT = 0;

    m_ReqViewportScale = m_CurViewportScale = m_PrevViewportScale = Vec2(1, 1);

    m_UseGlobalMipBias = 0;
    m_nCurMinAniso = 1;
    m_nCurMaxAniso = 16;
    m_wireframe_mode = R_SOLID_MODE;
    m_wireframe_mode_prev = R_SOLID_MODE;
    m_RP.m_StateOr = 0;
    m_RP.m_StateAnd = -1;

    m_pSpriteVerts = NULL;
    m_pSpriteInds = NULL;

    m_nativeWidth = 0;
    m_nativeHeight = 0;
    m_backbufferWidth = 0;
    m_backbufferHeight = 0;
    m_numSSAASamples = 1;

    m_screenShotType = 0;

    REGISTER_CVAR3("r_ApplyToonShading", CV_r_ApplyToonShading, 0, VF_NULL,
        "Disable/Enable Toon Shading render mode\n"
        "  0: Off\n"
        "  1: Toon Shading on\n");

    REGISTER_CVAR3("r_GraphicsPipeline", CV_r_GraphicsPipeline, 0, VF_NULL,
        "Toggles new optimized graphics pipeline\n"
        "  0: Off\n"
        "  1: Just fullscreen passes\n"
        "  2: Just scene passes\n"
        "  3: All passes\n");

    REGISTER_CVAR3_CB("r_DebugLightLayers", CV_r_DebugLightLayers, 0, VF_DUMPTODISK,
        "1 - Direct lighting, diffuse only.\n"
        "2 - Add environment ambient lighting, diffuse only.\n"
        "3 - Add specular term.\n"
        "4 - Add AO.\n"
        "5 - Add textures.\n"
        "6 - Add fog.\n"
        "7 - Add tone mapping / bloom / color grading.\n",
        OnChange_CV_r_DebugLightLayers);

    REGISTER_CVAR3_CB("r_DeferredShadingTiled", CV_r_DeferredShadingTiled, 0, VF_DUMPTODISK,
        "Toggles tiled shading using a compute shader\n"
        "1 - Tiled forward shading for transparent objects\n"
        "2 - Tiled deferred and forward shading\n"
        "3 - Tiled deferred and forward shading with debug info\n"
        "4 - Light coverage visualization\n",
        OnChange_CV_r_DeferredShadingTiled);

    REGISTER_CVAR3("r_DeferredShadingTiledHairQuality", CV_r_DeferredShadingTiledHairQuality, 2, VF_DUMPTODISK,
        "Tiled shading hair quality\n"
        "0 - Regular forward shading\n"
        "1 - Tiled shading on selected assets and more accurate probe blending\n"
        "2 - Full tiled shading with high quality shadow filter\n");

    REGISTER_CVAR3("r_DeferredShadingTiledDebugDirect", CV_r_DeferredShadingTiledDebugDirect, 0, VF_DUMPTODISK,
        "1 - Disables translucent BRDF.\n"
        "2 - Disables all direct lighting.\n");

    REGISTER_CVAR3("r_DeferredShadingTiledDebugIndirect", CV_r_DeferredShadingTiledDebugIndirect, 0, VF_DUMPTODISK,
        "Incrementally disables stages of the indirect lighting pipeline.\n"
        "3 - Disables Ambient Diffuse\n"
        "2 - Disables Ambient Specular\n"
        "1 - Disables AO and SSR\n");

    REGISTER_CVAR3("r_DeferredShadingTiledDebugAccumulation", CV_r_DeferredShadingTiledDebugAccumulation, 0, VF_DUMPTODISK,
        "Toggles layered debug visualization of deferred lighting contributions\n"
        "1 - Show Only Accumulated Diffuse\n"
        "2 - Show Only Accumulated Specular\n");

    REGISTER_CVAR3("r_DeferredShadingTiledDebugAlbedo", CV_r_DeferredShadingTiledDebugAlbedo, 0, VF_DUMPTODISK,
        "Toggles layered debug visualization of deferred lighting contributions\n"
        "1 - Force white albedo value\n");

    REGISTER_CVAR3("r_DeferredShadingSSS", CV_r_DeferredShadingSSS, DEF_SHAD_SSS_DEFAULT_VAL, VF_DUMPTODISK,
        "Toggles deferred subsurface scattering (requires full deferred shading)");

    REGISTER_CVAR3("r_DeferredShadingFilterGBuffer", CV_r_DeferredShadingFilterGBuffer, 0, VF_DUMPTODISK,
        "Enables filtering of GBuffer to reduce specular aliasing.\n");

    DefineConstIntCVar3("r_DeferredShadingLightVolumes", CV_r_deferredshadingLightVolumes, 1, VF_DUMPTODISK,
        "Toggles Light volumes for deferred shading.\n"
        "Usage: r_DeferredShadingLightVolumes [0/1]\n"
        "Default is 1 (enabled)");

    DefineConstIntCVar3("r_DeferredDecals", CV_r_deferredDecals, 1, VF_DUMPTODISK,
        "Toggles deferred decals.\n"
        "Usage: r_DeferredDecals [0/1]\n"
        "Default is 1 (enabled), 0 Disabled. ");

    DefineConstIntCVar3("r_deferredDecalsDebug", CV_r_deferredDecalsDebug, 0, VF_DUMPTODISK,
        "Display decals debug info.\n"
        "Usage: r_deferredDecalsDebug [0/1]");

    DefineConstIntCVar3("r_deferredDecalsOnDynamicObjects", CV_r_deferredDecalsOnDynamicObjects, 0, VF_DUMPTODISK,
        "Render deferred decals on dynamic objects.\n"
        "Usage: r_deferredDecalsOnDynamicObjects [0/1]");

    DefineConstIntCVar3("r_DeferredShadingEnvProbes", CV_r_DeferredShadingEnvProbes, 1, VF_DUMPTODISK,
        "Toggles deferred environment probes rendering.\n"
        "Usage: r_DeferredShadingEnvProbes [0/1]\n"
        "Default is 1 (enabled)");

    DefineConstIntCVar3("r_DeferredShadingStencilPrepass", CV_r_DeferredShadingStencilPrepass, 1, VF_DUMPTODISK,
        "Toggles deferred shading stencil pre pass.\n"
        "Usage: r_DeferredShadingStencilPrepass [0/1]\n"
        "Default is 1 (enabled)");

    DefineConstIntCVar3("r_DeferredShadingScissor", CV_r_DeferredShadingScissor, 1, VF_DUMPTODISK,
        "Toggles deferred shading scissor test.\n"
        "Usage: r_DeferredShadingScissor [0/1]\n"
        "Default is 1 (enabled)");

    DefineConstIntCVar3("r_DeferredShadingLBuffersFmt", CV_r_DeferredShadingLBuffersFmt, 1, VF_NULL,
        "Toggles light buffers format.\n"
        "Usage: r_DeferredShadingLBuffersFmt [0/1/2] \n"
        "Default is 1 (R11G11B10F), 0: R16G16B16A16F 2: Use optimized format for gmem : diffuseAcc 8 (R8) bits instead of 64 and SpeculaAcc 32 bits (R10G10B10A2) instead of 64.");


    DefineConstIntCVar3("r_DeferredShadingDepthBoundsTest", CV_r_DeferredShadingDepthBoundsTest, DEF_SHAD_DBT_DEFAULT_VAL,
        VF_DUMPTODISK,
        "Toggles deferred shading depth bounds test.\n"
        "Usage: r_DeferredShadingDepthBoundsTest [0/1]\n"
        "Default is 1 (enabled)");

    DefineConstIntCVar3("r_DeferredShadingDBTstencil", CV_r_deferredshadingDBTstencil, DEF_SHAD_DBT_STENCIL_DEFAULT_VAL,
        VF_DUMPTODISK,
        "Toggles deferred shading combined depth bounds test + stencil test.\n"
        "Usage: r_DeferredShadingDBTstencil [0/1]\n"
        "Default is 1 (enabled)");

    DefineConstIntCVar3("r_DeferredShadingDebug", CV_r_DeferredShadingDebug, 0, VF_DUMPTODISK,
        "Toggles deferred shading debug.\n"
        "Usage: r_DeferredShadingDebug [0/1]\n"
        "  0 disabled (Default)\n"
        "  1: Visualize g-buffer and l-buffers\n"
        "  2: Debug deferred lighting fillrate (brighter colors means more expensive)\n");

    DefineConstIntCVar3("r_DebugGBuffer", CV_r_DeferredShadingDebugGBuffer, 0, VF_DEV_ONLY,
        "Debug view for gbuffer attributes\n"
        "  0 - Disabled\n"
        "  1 - Normals\n"
        "  2 - Smoothness\n"
        "  3 - Reflectance\n"
        "  4 - Albedo\n"
        "  5 - Lighting model\n"
        "  6 - Translucency\n"
        "  7 - Sun self-shadowing\n"
        "  8 - Subsurface scattering\n"
        "  9 - Specular validation overlay\n"
        );

    DefineConstIntCVar3("r_DeferredShadingLights", CV_r_DeferredShadingLights, 1, VF_DUMPTODISK,
        "Enables/Disables lights processing.\n"
        "Usage: r_DeferredShadingLights [0/1]\n"
        "Default is 1 (enabled)");

    DefineConstIntCVar3("r_DeferredShadingAmbientLights", CV_r_DeferredShadingAmbientLights, 1, VF_DUMPTODISK,
        "Enables/Disables ambient lights.\n"
        "Usage: r_DeferredShadingAmbientLights [0/1]\n"
        "Default is 1 (enabled)");

    DefineConstIntCVar3("r_DeferredShadingAreaLights", CV_r_DeferredShadingAreaLights, 1, VF_DUMPTODISK,
        "Enables/Disables more complex area lights processing.\n"
        "Usage: r_DeferredShadingAreaLights [0/1]\n"
        "Default is 0 (disabled)");

    DefineConstIntCVar3("r_DeferredShadingAmbient", CV_r_DeferredShadingAmbient, 1, VF_DUMPTODISK,
        "Enables/Disables ambient processing.\n"
        "Usage: r_DeferredShadingAmbient [0/1/2]\n"
        "  0: no ambient passes (disabled)\n"
        "  1: vis areas and outdoor ambient  (default)\n"
        "  2: only outdoor (debug vis areas mode)\n");

    REGISTER_CVAR3("r_DeferredShadingLightLodRatio", CV_r_DeferredShadingLightLodRatio, 1.0f, VF_DUMPTODISK,
        "Sets deferred shading light intensity threshold.\n"
        "Usage: r_DeferredShadingLightLodRatio [value]\n"
        "Default is 0.1");

    REGISTER_CVAR3("r_DeferredShadingLightStencilRatio", CV_r_DeferredShadingLightStencilRatio, 0.21f, VF_DUMPTODISK,
        "Sets screen ratio for deferred lights to use stencil (eg: 0.2 - 20% of screen).\n"
        "Usage: r_DeferredShadingLightStencilRatio [value]\n"
        "Default is 0.2");


    REGISTER_CVAR3("r_DeferredShadingSortLights", CV_r_DeferredShadingSortLights, 0, VF_CHEAT,
        "Sorts deferred lights\n"
        "Usage: r_DeferredShadingSortLights [0/1/2/3]\n"
        " 0: no sorting\n"
        " 1: sort by screen space influence area\n"
        " 2: lights that are already in the shadowmap pool are processed first\n"
        " 3: first sort by presence in the shadowmap pool and then by screen space influence area\n"
        "Default is 0 (off)");

    REGISTER_CVAR3("r_DeferredShadingAmbientSClear", CV_r_DeferredShadingAmbientSClear, 1, VF_NULL,
        "Clear stencil buffer after ambient pass (prevents artifacts on Nvidia hw)\n");

    ICVar* hdrDebug = REGISTER_CVAR3("r_HDRDebug", CV_r_HDRDebug, 0, VF_NULL,
            "Toggles HDR debugging info (to debug HDR/eye adaptation)\n"
            "Usage: r_HDRDebug\n"
            "0 off (default)\n"
            "1 show gamma-corrected scene target without HDR processing\n"
            "2 identify illegal colors (grey=normal, red=NotANumber, green=negative)\n"
            "3 display internal HDR textures\n"
            "4 display HDR range adaptation\n"
            "5 debug merged posts composition mask\n");
    if (hdrDebug)
    {
        hdrDebug->SetLimits(0.0f, 5.0f);
    }

    REGISTER_CVAR3("r_ToneMapTechnique", CV_r_ToneMapTechnique, 0, VF_NULL,
        "Toggles Tonemapping technique\n"
        "Usage: r_ToneMapTechnique\n"        
        "0 Uncharted 2 Filmic curve by J Hable (default)\n"
        "1 Linear operator\n"
        "2 Exponential operator\n"
        "3 Reinhard operator\n"
        "4 Cheap ALU based filmic curve from John Hable\n");
   
    REGISTER_CVAR3("r_ColorSpace", CV_r_ColorSpace, 0, VF_NULL,
        "Toggles Color Space conversion\n"
        "Usage: r_ColorSpace\n"
        "0 sRGB0 - Most accurate (default)\n"
        "1 sRGB1 - Cheap approximation\n"
        "2 sRGB2 - Cheapest approximation\n");
    
    REGISTER_CVAR3("r_ToneMapManualExposureValue", CV_r_ToneMapManualExposureValue, 1, VF_NULL,
        "Set the manual exposure value for cheaper tonemap techniques\n"
        "Usage: r_ToneMapManualExposureValue\n"
        "Default is 1.0\n");

    REGISTER_CVAR3("r_ToneMapExposureType", CV_r_ToneMapExposureType, 0, VF_NULL,
        "Set the type of exposure to be used by tonemap operators\n"
        "Usage: r_ToneMapExposureType\n"
        "Default is 0\n"
        "0 Auto exposure\n"
        "1 Manual exposure\n");

    REGISTER_CVAR3("r_HDRBloom", CV_r_HDRBloom, 1, VF_NULL,
        "Enables bloom/glare.\n"
        "Usage: r_HDRBloom [0/1]\n");

    REGISTER_CVAR3("r_HDRBloomQuality", CV_r_HDRBloomQuality, 2, VF_NULL,
        "Set bloom quality (0: low, 1: medium, 2: high)\n");

    DefineConstIntCVar3("r_HDRVignetting", CV_r_HDRVignetting, 1, VF_DUMPTODISK,
        "HDR viggneting\n"
        "Usage: r_HDRVignetting [Value]\n"
        "Default is 1 (enabled)");

    DefineConstIntCVar3("r_HDRTexFormat", CV_r_HDRTexFormat, 0, VF_DUMPTODISK,
        "HDR texture format. \n"
        "Usage: r_HDRTexFormat [Value] 0:(low precision - cheaper/faster), 1:(high precision)\n"
        "Default is 0");

    // Dolby Parameters
    REGISTER_CVAR3("r_HDRDolbyDynamicMetadata", CV_r_HDRDolbyDynamicMetadata, 1, VF_DUMPTODISK, "Enable Dolby Dynamic Metadata (provides Dolby Vision screen with min/max/mid of the current image, in order to improve image quality)");
    REGISTER_CVAR3("r_HDRDolbyScurve", CV_r_HDRDolbyScurve, 1, VF_DUMPTODISK, "Enable Dolby S-Curve (transformation from source intensity range to cd/m^2).");
    REGISTER_CVAR3("r_HDRDolbyScurveSourceMin", CV_r_HDRDolbyScurveSourceMin, 0.001f, VF_DUMPTODISK, "Set Dolby S-Curve Source minimum intensity (in source units).");
    REGISTER_CVAR3("r_HDRDolbyScurveSourceMid", CV_r_HDRDolbyScurveSourceMid, 0.4f, VF_DUMPTODISK, "Set Dolby S-Curve Source midpoint intensity (in source units).");
    REGISTER_CVAR3("r_HDRDolbyScurveSourceMax", CV_r_HDRDolbyScurveSourceMax, 10000.0f,  VF_DUMPTODISK, "Set Dolby S-Curve Source maximum intensity (in source units).");
    REGISTER_CVAR3("r_HDRDolbyScurveSlope", CV_r_HDRDolbyScurveSlope, 1.0f, VF_DUMPTODISK, "Set Dolby S-Curve Slope (similar to gamma).");
    REGISTER_CVAR3("r_HDRDolbyScurveScale", CV_r_HDRDolbyScurveScale, 1.0f, VF_DUMPTODISK, "Set Dolby S-Curve Multiplier (similar to brightness).");
    REGISTER_CVAR3("r_HDRDolbyScurveRGBPQTargetMin", CV_r_HDRDolbyScurveRGBPQTargetMin, 0.001f, VF_DUMPTODISK, "Set Dolby S-Curve RGBPQ (e.g. Maui) Target minimum intensity (in cd/m^2).");
    REGISTER_CVAR3("r_HDRDolbyScurveRGBPQTargetMid", CV_r_HDRDolbyScurveRGBPQTargetMid, 50.0f, VF_DUMPTODISK, "Set Dolby S-Curve RGBPQ (e.g. Maui) Target midpoint intensity (in cd/m^2).");
    REGISTER_CVAR3("r_HDRDolbyScurveRGBPQTargetMax", CV_r_HDRDolbyScurveRGBPQTargetMax, 2000.0f, VF_DUMPTODISK, "Set Dolby S-Curve RGBPQ (e.g. Maui) Target midpoint (average) intensity (in cd/m^2).");
    REGISTER_CVAR3("r_HDRDolbyScurveVisionTargetMin", CV_r_HDRDolbyScurveVisionTargetMin, 0.001f, VF_DUMPTODISK, "Set Dolby S-Curve Vision (e.g. Vizio) Target minimum intensity (in cd/m^2).");
    REGISTER_CVAR3("r_HDRDolbyScurveVisionTargetMid", CV_r_HDRDolbyScurveVisionTargetMid, 50.0f, VF_DUMPTODISK, "Set Dolby S-Curve Vision (e.g. Vizio) Target midpoint intensity (in cd/m^2).");
    REGISTER_CVAR3("r_HDRDolbyScurveVisionTargetMax", CV_r_HDRDolbyScurveVisionTargetMax, 800.0f, VF_DUMPTODISK, "Set Dolby S-Curve Vision (e.g. Vizio) Target maximum intensity (in cd/m^2).");

    // Eye Adaptation
    REGISTER_CVAR3("r_HDREyeAdaptationSpeed", CV_r_HDREyeAdaptationSpeed, 1.0f, VF_NULL,
        "HDR rendering eye adaptation speed\n"
        "Usage: r_EyeAdaptationSpeed [Value]");

    REGISTER_CVAR3("r_HDREyeAdaptationMode", CV_r_HDREyeAdaptationMode, 2, VF_DUMPTODISK,
        "Set the eye adaptation (auto exposure) mode.\n"
        "  1: Standard auto exposure (using EV values)\n"
        "  2: Legacy auto exposure for backwards compatibility (CE 3.6 to 3.8.1)");

    REGISTER_CVAR3("r_HDRGrainAmount", CV_r_HDRGrainAmount, 0.0f, VF_NULL,
        "HDR camera grain amount\n"
        "Usage: r_HDRGrainAmount [Value]");

    REGISTER_CVAR3("r_ChromaticAberration", CV_r_ChromaticAberration, 0.0f, VF_NULL,
        "Chromatic aberration amount\n"
        "Usage: r_ChromaticAberration [Value]");

    REGISTER_CVAR3("r_Sharpening", CV_r_Sharpening, 0.0f, VF_NULL,
        "Image sharpening amount\n"
        "Usage: r_Sharpening [Value]");

    REGISTER_CVAR3("r_Beams", CV_r_beams, 1, VF_NULL,
        "Toggles volumetric light beams.\n"
        "Usage: r_Beams [0/1]\n");

    REGISTER_CVAR3_CB("r_GeomInstancingThreshold", CV_r_geominstancingthreshold, -1, VF_NULL,
        "If the instance count gets bigger than the specified value the instancing feature is used.\n"
        "Usage: r_GeomInstancingThreshold [Num]\n"
        "Default is -1 (automatic depending on hardware, used value can be found in the log)",
        ChangeGeomInstancingThreshold);

    REGISTER_CVAR3("r_BatchType", CV_r_batchtype, 0, VF_NULL,
        "0 - CPU friendly.\n"
        "1 - GPU friendly.\n"
        "2 - Automatic.\n");

#if defined(WIN32) || defined(WIN64) || defined(APPLE) || defined(LINUX) || defined(USE_SILHOUETTEPOM_CVAR)
    REGISTER_CVAR3("r_SilhouettePOM", CV_r_SilhouettePOM, 0, VF_NULL,
        "Enables use of silhouette parallax occlusion mapping.\n"
        "Usage: r_SilhouettePOM [0/1]");
#endif
#ifdef WATER_TESSELLATION_RENDERER
    REGISTER_CVAR3("r_WaterTessellationHW", CV_r_WaterTessellationHW, 0, VF_NULL,
        "Enables hw water tessellation.\n"
        "Usage: r_WaterTessellationHW [0/1]");
#endif

    REGISTER_CVAR3("r_TessellationDebug", CV_r_tessellationdebug, 0, VF_NULL,
        "1 - Factor visualizing.\n"
        "Default is 0");
    REGISTER_CVAR3("r_TessellationTriangleSize", CV_r_tessellationtrianglesize, 8.0f, VF_NULL,
        "Desired triangle size for screen-space tessellation.\n"
        "Default is 10.");
    REGISTER_CVAR3("r_UseDisplacementFactor", CV_r_displacementfactor, 0.2f, VF_NULL,
        "Global displacement amount.\n"
        "Default is 0.4f.");

    DefineConstIntCVar3("r_GeomInstancing", CV_r_geominstancing, GEOM_INSTANCING_DEFAULT_VAL, VF_NULL,
        "Toggles HW geometry instancing.\n"
        "Usage: r_GeomInstancing [0/1]\n"
        "Default is 1 (on). Set to 0 to disable geom. instancing.");

    DefineConstIntCVar3("r_GeomInstancingDebug", CV_r_geominstancingdebug, 0, VF_NULL,
        "Toggles HW geometry instancing debug display.\n"
        "Usage: r_GeomInstancingDebug [0/1/2]\n"
        "Default is 0 (off). Set to 1 to add GPU markers around instanced objects. 2 will visually highlight them as well.");

    DefineConstIntCVar3("r_MaterialsBatching", CV_r_materialsbatching, 1, VF_NULL,
        "Toggles materials batching.\n"
        "Usage: r_MaterialsBatching [0/1]\n"
        "Default is 1 (on). Set to 0 to disable.");

    DefineConstIntCVar3("r_ImpostersDraw", CV_r_impostersdraw, 1, VF_NULL,
        "Toggles imposters drawing.\n"
        "Usage: r_ImpostersDraw [0/1]\n"
        "Default is 1 (on). Set to 0 to disable imposters.");
    REGISTER_CVAR3("r_ImposterRatio", CV_r_imposterratio, 1, VF_NULL,
        "Allows to scale the texture resolution of imposters (clouds)\n"
        "Usage: r_ImposterRatio [1..]\n"
        "Default is 1 (1:1 normal). Bigger values can help to save texture space\n"
        "(e.g. value 2 results in 1/4 texture memory usage)");
    REGISTER_CVAR3("r_ImpostersUpdatePerFrame", CV_r_impostersupdateperframe, 6000, VF_NULL,
        "How many kilobytes to update per-frame.\n"
        "Usage: r_ImpostersUpdatePerFrame [1000-30000]\n"
        "Default is 6000 (6 megabytes)");

    DefineConstIntCVar3("r_ZPassDepthSorting", CV_r_ZPassDepthSorting, ZPASS_DEPTH_SORT_DEFAULT_VAL, VF_NULL,
        "Toggles Z pass depth sorting.\n"
        "Usage: r_ZPassDepthSorting [0/1/2]\n"
        "0: No depth sorting\n"
        "1: Sort by depth layers (default)\n"
        "2: Sort by distance\n");

    REGISTER_CVAR3("r_ZPrepassMaxDist", CV_r_ZPrepassMaxDist, 16.0f, VF_NULL,
        "Set ZPrepass max dist.\n"
        "Usage: r_ZPrepassMaxDist (16.0f default) [distance in meters]\n");

    REGISTER_CVAR3("r_UseZPass", CV_r_usezpass, 2, VF_RENDERER_CVAR,
        "Toggles g-buffer pass.\n"
        "Usage: r_UseZPass [0/1/2]\n"
        "0: Disable Z-pass (not recommended, this disables any g-buffer rendering)\n"
        "1: Enable Z-pass (g-buffer only)\n"
        "2: Enable Z-pass (g-buffer and additional Z-prepass)");

    DefineConstIntCVar3("r_TransparentPasses", CV_r_TransparentPasses, 1, VF_NULL,
        "Toggles rendering of transparent/alpha blended objects.\n");

    DefineConstIntCVar3("r_TranspDepthFixup", CV_r_TranspDepthFixup, 1, VF_NULL,
        "Write approximate depth for certain transparent objects before post effects\n"
        "Usage: r_TranspDepthFixup [0/1]\n"
        "Default is 1 (enabled)\n");

    DefineConstIntCVar3("r_SoftAlphaTest", CV_r_SoftAlphaTest, 1, VF_NULL,
        "Toggles post processed soft alpha test for shaders supporting this\n"
        "Usage: r_SoftAlphaTest [0/1]\n"
        "Default is 1 (enabled)\n");

    DefineConstIntCVar3("r_UseHWSkinning", CV_r_usehwskinning, 1, VF_NULL,
        "Toggles HW skinning.\n"
        "Usage: r_UseHWSkinning [0/1]\n"
        "Default is 1 (on). Set to 0 to disable HW-skinning.");
    DefineConstIntCVar3("r_UseMaterialLayers", CV_r_usemateriallayers, 2, VF_NULL,
        "Enables material layers rendering.\n"
        "Usage: r_UseMaterialLayers [0/1/2]\n"
        "Default is 2 (optimized). Set to 1 for enabling but with optimization disabled (for debug).");

    DefineConstIntCVar3("r_ParticlesSoftIsec", CV_r_ParticlesSoftIsec, 1, VF_NULL,
        "Enables particles soft intersections.\n"
        "Usage: r_ParticlesSoftIsec [0/1]");

    DefineConstIntCVar3("r_ParticlesRefraction", CV_r_ParticlesRefraction, 1, VF_NULL,
        "Enables refractive particles.\n"
        "Usage: r_ParticlesRefraction [0/1]");

    REGISTER_CVAR3("r_ParticlesHalfRes", CV_r_ParticlesHalfRes, 0, VF_NULL,
        "Enables (1) or forces (2) rendering of particles in a half-resolution buffer.\n"
        "Usage: r_ParticlesHalfRes [0/1/2]");

    DefineConstIntCVar3("r_ParticlesHalfResBlendMode", CV_r_ParticlesHalfResBlendMode, 0, VF_NULL,
        "Specifies which particles can be rendered in half resolution.\n"
        "Usage: r_ParticlesHalfResBlendMode [0=alpha / 1=additive]");

    DefineConstIntCVar3("r_ParticlesHalfResAmount", CV_r_ParticlesHalfResAmount, 0, VF_NULL,
        "Sets particle half-res buffer to half (0) or quarter (1) screen size.\n"
        "Usage: r_ParticlesHalfResForce [0/1]");

    DefineConstIntCVar3("r_ParticlesInstanceVertices", CV_r_ParticlesInstanceVertices, 1, VF_NULL,
        "Enable instanced-vertex rendering.\n"
        "Usage: r_ParticlesInstanceVertices [0/1]");

    REGISTER_CVAR3("r_ParticlesAmountGI", CV_r_ParticlesAmountGI, 0.15f, VF_NULL,
        "Global illumination amount for particles without material.\n"
        "Usage: r_ParticlesAmountGI [n]");

    REGISTER_CVAR3("r_MSAA", CV_r_msaa, 0, VF_NULL,
        "Enables hw multisampling antialiasing.\n"
        "Usage: r_MSAA [0/1]\n"
        "Default: 0 (off).\n"
        "1: enabled + default reference quality mode\n");
    REGISTER_CVAR3("r_MSAA_samples", CV_r_msaa_samples, 0, VF_NULL,
        "Number of subsamples used when hw multisampled antialiasing is enabled.\n"
        "Usage: r_MSAA_samples N (where N is a number >= 0). Attention, N must be supported by given video hardware!\n"
        "Default: 0. Please note that various hardware implements special MSAA modes via certain combinations of\n"
        "r_MSAA_quality and r_MSAA_samples.");
    REGISTER_CVAR3("r_MSAA_quality", CV_r_msaa_quality, 0, VF_NULL,
        "Quality level used when multisampled antialiasing is enabled.\n"
        "Usage: r_MSAA_quality N (where N is a number >= 0). Attention, N must be supported by given video hardware!\n"
        "Default: 0. Please note that various hardware implements special MSAA modes via certain combinations of\n"
        "r_MSAA_quality and r_MSAA_samples.");
    REGISTER_CVAR3("r_MSAA_debug", CV_r_msaa_debug, 0, VF_NULL,
        "Enable debugging mode for msaa.\n"
        "Usage: r_MSAA_debug N (where N is debug mode > 0)\n"
        "Default: 0. disabled. Note debug modes share target with post processing, disable post processing for correct visualization. \n"
        "1 disable sample frequency pass\n"
        "2 visualize sample frequency mask\n");

    // This values seem a good performance/quality balance for a Crysis style level (note that they might need re-adjust for different projects)
    REGISTER_CVAR3("r_MSAA_threshold_depth", CV_r_msaa_threshold_depth, 0.1f, VF_NULL,
        "Set depth threshold to be used for custom resolve sub-samples masking\n");
    REGISTER_CVAR3("r_MSAA_threshold_normal", CV_r_msaa_threshold_normal, 0.9f, VF_NULL,
        "Set normals threshold to be used for custom resolve sub-samples masking\n");

    REGISTER_CVAR3("r_UseSpecularAntialiasing", CV_r_SpecularAntialiasing, 1, VF_NULL,
        "Enable specular antialiasing.\n"
        "Usage: r_UseSpecularAntialiasing [0/1]");

    static string aaModesDesc = "Enables post process based anti-aliasing modes.\nUsage: r_AntialiasingMode [n]\n";

    for (int i = 0; i < eAT_AAMODES_COUNT; ++i)
    {
        string mode;
        mode.Format("%d: %s\n", i, s_pszAAModes[i]);
        aaModesDesc.append(mode);
    }

    REGISTER_CVAR3_CB("r_AntialiasingMode", CV_r_AntialiasingMode_CB, eAT_DEFAULT_AA, VF_NULL, aaModesDesc.c_str(), OnChange_CV_r_AntialiasingMode);
    CV_r_AntialiasingMode = CV_r_AntialiasingMode_CB;

    REGISTER_CVAR3("r_AntialiasingNonTAASharpening", CV_r_AntialiasingNonTAASharpening, 0.f, VF_NULL,
        "Enables non-TAA sharpening.\n");

    REGISTER_CVAR3("r_AntialiasingTAAJitterPattern", CV_r_AntialiasingTAAJitterPattern, 7, VF_NULL,
        "Selects TAA sampling pattern.\n"
        "  0: no subsamples\n  1: 2x\n  2: 3x\n  3: 4x\n  4: 8x\n  5: sparse grid 8x8\n  6: random\n  7: Halton 8x\n  8: Halton random");

    DefineConstIntCVar3("r_AntialiasingTAAUseJitterMipBias", CV_r_AntialiasingTAAUseJitterMipBias, 1, VF_NULL,
        "Allows mip map biasing on textures when jitter is enabled\n");

    DefineConstIntCVar3("r_AntialiasingTAAUseVarianceClamping", CV_r_AntialiasingTAAUseVarianceClamping, 0, VF_NULL,
        "Allows variance-based color clipping. Decreases ghosting but may increase flickering artifacts.\n");

    DefineConstIntCVar3("r_AntialiasingModeDebug", CV_r_AntialiasingModeDebug, 0, VF_NULL,
        "Enables AA debugging views\n"
        "Usage: r_AntialiasingModeDebug [n]"
        "1: Display edge detection"
        "2: Zoom image 2x"
        "3: Zoom image 2x + display edge detection"
        "4: Zoom image 4x, etc");

    DefineConstIntCVar3("r_AntialiasingTAAUseAntiFlickerFilter", CV_r_AntialiasingTAAUseAntiFlickerFilter, 1, VF_NULL,
        "Enables TAA anti-flicker filtering.\n");

    REGISTER_CVAR3("r_AntialiasingTAAClampingFactor", CV_r_AntialiasingTAAClampingFactor, 1.25f, VF_NULL,
        "Controls the history clamping factor for TAA. Higher values will cause more ghosting but less flickering. Acceptable values between 0.75 and 2.0\n");

    REGISTER_CVAR3("r_AntialiasingTAANewFrameWeight", CV_r_AntialiasingTAANewFrameWeight, 0.05f, VF_NULL,
        "The weight controlling how much of the current frame is used when integrating with the exponential history buffer.\n");

    REGISTER_CVAR3("r_AntialiasingTAASharpening", CV_r_AntialiasingTAASharpening, 0.1f, VF_NULL,
        "Enables TAA sharpening.\n");

    DefineConstIntCVar3("CV_r_AntialiasingModeEditor", CV_r_AntialiasingModeEditor, 1, VF_NULL,
        "Sets antialiasing modes to editing mode (disables jitter on modes using camera jitter which can cause flickering of helper objects)\n"
        "Usage: CV_r_AntialiasingModeEditor [0/1]");

    DefineConstIntCVar3("r_MotionVectors", CV_r_MotionVectors, 1, VF_NULL,
        "Enables generation of motion vectors for dynamic objects\n");

    DefineConstIntCVar3("r_MotionVectorsTransparency", CV_r_MotionVectorsTransparency, 1, VF_NULL,
        "Enables generation of motion vectors for transparent objects\n");

    REGISTER_CVAR3("r_MotionVectorsTransparencyAlphaThreshold", CV_r_MotionVectorsTransparencyAlphaThreshold, 0.25f, VF_NULL,
        "Transparent object alpha threshold. If the alpha is above this threshold the object will generate motion vectors.\n"
        "Usage: r_MotionVectorsTransparencyAlphaThreshold (val)\n"
        "Default is 0.25.  0 - disabled\n");

    DefineConstIntCVar3("r_MotionVectorsDebug", CV_r_MotionVectorsDebug, 0, VF_NULL,
        "Enables motion vector debug visualization.\n");

    REGISTER_CVAR3_CB("r_MotionBlur", CV_r_MotionBlur, 2, VF_NULL,
        "Enables per object and camera motion blur.\n"
        "Usage: r_MotionBlur [0/1/2/3]\n"
        "Default is 1 (camera motion blur on).\n"
        "1: camera motion blur\n"
        "2: camera and object motion blur\n"
        "3: debug mode\n", OnChange_CV_r_MotionBlur);

    REGISTER_CVAR3("r_RenderMotionBlurAfterHDR", CV_r_RenderMotionBlurAfterHDR, 0, VF_NULL,
        "Forces Motion Blur To Render After HDR processing.\n"
        "Usage: r_MotionBlur [0/1]\n"
        "Default is 0 (Motion Blur Before HDR).\n"
        "0: Motion Blur Applied Before HDR Processing (Luminance Measurement, Bloom, Tonemapping)\n"
        "1: Motion Blur Applied After HDR Processing (Luminance Measurement, Bloom, Tonemapping)\n");

    REGISTER_CVAR3("r_MotionBlurScreenShot", CV_r_MotionBlurScreenShot, 0, VF_NULL,
        "Enables motion blur during high res screen captures"
        "Usage: r_MotionBlur [0/1]\n"
        "0: motion blur disabled for screen shot (default)\n"
        "1: motion blur enabled for screen shot\n");

    REGISTER_CVAR3("r_MotionBlurQuality", CV_r_MotionBlurQuality, 1, VF_NULL,
        "Set motion blur sample count.\n"
        "Usage: r_MotionBlurQuality [0/1]\n"
        "0 - low quality, 1 - medium quality, 2 - high quality\n");

    REGISTER_CVAR3("r_MotionBlurGBufferVelocity", CV_r_MotionBlurGBufferVelocity, 1, VF_NULL,
        "Pack velocity output in g-buffer.\n"
        "Usage: r_MotionBlurGBufferVelocity [0/1]\n"
        "Default is 1 (enabled). 0 - disabled\n");

    REGISTER_CVAR3("r_MotionBlurThreshold", CV_r_MotionBlurThreshold, 0.0001f, VF_NULL,
        "Object motion blur velocity threshold.\n"
        "Usage: r_MotionBlurThreshold (val)\n"
        "Default is 0.0001.  0 - disabled\n");

    REGISTER_CVAR3("r_MotionBlurShutterSpeed", CV_r_MotionBlurShutterSpeed, 250.0f, 0,
        "Sets camera exposure time for motion blur as 1/x seconds.\n"
        "Default: 250 (1/250 of a second)");

    REGISTER_CVAR3("r_MotionBlurCameraMotionScale", CV_r_MotionBlurCameraMotionScale, 0.2f, 0,
        "Reduces motion blur for camera movements to account for a fixed focus point of the viewer.\n"
        "Default: 0.2");

    REGISTER_CVAR3("r_MotionBlurMaxViewDist", CV_r_MotionBlurMaxViewDist, 16.0f, 0,
        "Sets motion blur max view distance for objects.\n"
        "Usage: r_MotionBlurMaxViewDist [0...1]\n"
        "Default is 16 meters");

    DefineConstIntCVar3("r_CustomVisions", CV_r_customvisions, CUSTOMVISIONS_DEFAULT_VAL, VF_NULL,
        "Enables custom visions, like heatvision, binocular view, etc.\n"
        "Usage: r_CustomVisions [0/1/2/3]\n"
        "Default is 0 (disabled). 1 enables. 2 - cheaper version, no post processing. 3 - cheaper post version");

    DefineConstIntCVar3("r_Snow", CV_r_snow, 2, VF_NULL,
        "Enables snow rendering\n"
        "Usage: r_Snow [0/1/2]\n"
        "0 - disabled\n"
        "1 - enabled\n"
        "2 - enabled with snow occlusion");

    DefineConstIntCVar3("r_SnowHalfRes", CV_r_snow_halfres, 0, VF_NULL,
        "When enabled, snow renders at half resolution to conserve fill rate.\n"
        "Usage: r_SnowHalfRes [0/1]\n"
        "0 - disabled\n"
        "1 - enabled\n");

    DefineConstIntCVar3("r_SnowDisplacement", CV_r_snow_displacement, 0, VF_NULL,
        "Enables displacement for snow accumulation\n"
        "Usage: r_SnowDisplacement [0/1]\n"
        "0 - disabled\n"
        "1 - enabled");

    DefineConstIntCVar3("r_SnowFlakeClusters", CV_r_snowFlakeClusters, 100, VF_NULL,
        "Number of snow flake clusters.\n"
        "Usage: r_SnowFlakeClusters [n]");

    DefineConstIntCVar3("r_Rain", CV_r_rain, 2, VF_NULL,
        "Enables rain rendering\n"
        "Usage: r_Rain [0/1/2]\n"
        "0 - disabled"
        "1 - enabled"
        "2 - enabled with rain occlusion");

    REGISTER_CVAR3("r_RainAmount", CV_r_rainamount, 1.0f, VF_NULL,
        "Sets rain amount\n"
        "Usage: r_RainAmount");

    REGISTER_CVAR3("r_RainMaxViewDist", CV_r_rain_maxviewdist, 32.0f, VF_NULL,
        "Sets rain max view distance\n"
        "Usage: r_RainMaxViewDist");

    REGISTER_CVAR3("r_RainMaxViewDist_Deferred", CV_r_rain_maxviewdist_deferred, 40.f, VF_NULL,
        "Sets maximum view distance (in meters) for deferred rain reflection layer\n"
        "Usage: r_RainMaxViewDist_Deferred [n]");

    REGISTER_CVAR3("r_RainDistMultiplier", CV_r_rainDistMultiplier, 2.f, VF_NULL, "Rain layer distance from camera multiplier");

    REGISTER_CVAR3("r_RainOccluderSizeTreshold", CV_r_rainOccluderSizeTreshold, 25.f, VF_NULL, "Only objects bigger than this size will occlude rain");

    REGISTER_CVAR3_CB("r_SSReflections", CV_r_SSReflections, 0, VF_NULL,
        "Glossy screen space reflections [0/1]\n", OnChange_CV_r_SSReflections);
    REGISTER_CVAR3("r_SSReflHalfRes", CV_r_SSReflHalfRes, 1, VF_NULL,
        "Toggles rendering reflections in half resolution\n");
    REGISTER_CVAR3_CB("r_ssdo", CV_r_ssdo, 1, VF_NULL, "Screen Space Directional Occlusion [0/1]\n", OnChange_CV_r_SSDO);
    REGISTER_CVAR3("r_ssdoHalfRes", CV_r_ssdoHalfRes, 2, VF_NULL,
        "Apply SSDO bandwidth optimizations\n"
        "0 - Full resolution (not recommended)\n"
        "1 - Use lower resolution depth\n"
        "2 - Low res depth except for small camera FOVs to avoid artifacts\n"
        "3 - Half resolution output");
    REGISTER_CVAR3("r_ssdoColorBleeding", CV_r_ssdoColorBleeding, 1, VF_NULL,
        "Enables AO color bleeding to avoid overly dark occlusion on bright surfaces (requires tiled deferred shading)\n"
        "Usage: r_ssdoColorBleeding [0/1]\n");
    REGISTER_CVAR3("r_ssdoRadius", CV_r_ssdoRadius, 1.2f, VF_NULL, "SSDO radius");
    REGISTER_CVAR3("r_ssdoRadiusMin", CV_r_ssdoRadiusMin, 0.04f, VF_NULL, "Min clamped SSDO radius");
    REGISTER_CVAR3("r_ssdoRadiusMax", CV_r_ssdoRadiusMax, 0.20f, VF_NULL, "Max clamped SSDO radius");
    REGISTER_CVAR3("r_ssdoAmountDirect", CV_r_ssdoAmountDirect, 2.0f, VF_NULL, "Strength of occlusion applied to light sources");
    REGISTER_CVAR3("r_ssdoAmountAmbient", CV_r_ssdoAmountAmbient, 1.0f, VF_NULL, "Strength of occlusion applied to probe irradiance");
    REGISTER_CVAR3("r_ssdoAmountReflection", CV_r_ssdoAmountReflection, 1.5f, VF_NULL, "Strength of occlusion applied to probe specular");

    DefineConstIntCVar3("r_RainIgnoreNearest", CV_r_rain_ignore_nearest, 1, VF_NULL,
        "Disables rain wet/reflection layer for nearest objects\n"
        "Usage: r_RainIgnoreNearest [0/1]\n");

    DefineConstIntCVar3("r_DepthOfField", CV_r_dof, DOF_DEFAULT_VAL, VF_NULL,
        "Enables depth of field.\n"
        "Usage: r_DepthOfField [0/1/2]\n"
        "Default is 0 (disabled). 1 enables, 2 hdr time of day dof enabled");

    DefineConstIntCVar3("r_DebugLightVolumes", CV_r_DebugLightVolumes, 0, VF_NULL,
        "0=Disable\n"
        "1=Enable\n"
        "Usage: r_DebugLightVolumes[0/1]");

    DefineConstIntCVar3("r_UseShadowsPool", CV_r_UseShadowsPool, SHADOWS_POOL_DEFAULT_VAL, VF_NULL,
        "0=Disable\n"
        "1=Enable\n"
        "Usage: r_UseShadowsPool[0/1]");

    REGISTER_CVAR3("r_ShadowsBias", CV_r_ShadowsBias, 0.00008f, VF_DUMPTODISK, //-0.00002
        "Select shadow map blurriness if r_ShadowsBias is activated.\n"
        "Usage: r_ShadowsBias [0.1 - 16]");

    REGISTER_CVAR3("r_ShadowsAdaptionRangeClamp", CV_r_ShadowsAdaptionRangeClamp, 0.02f, VF_DUMPTODISK, //-0.00002
        "maximum range between caster and reciever to take into account.\n"
        "Usage: r_ShadowsAdaptionRangeClamp [0.0 - 1.0], default 0.01");

    REGISTER_CVAR3("r_ShadowsAdaptionSize", CV_r_ShadowsAdaptionSize, 0.3f, VF_DUMPTODISK, //-0.00002
        "Select shadow map blurriness if r_ShadowsBias is activated.\n"
        "Usage: r_ShadowsAdaptoinSize [0 for none - 10 for rapidly changing]");

    REGISTER_CVAR3("r_ShadowsAdaptionMin", CV_r_ShadowsAdaptionMin, 0.35f, VF_DUMPTODISK, //-0.00002
        "starting kernel size, to avoid blocky shadows.\n"
        "Usage: r_ShadowsAdaptionMin [0.0 for blocky - 1.0 for blury], 0.35 is default");

    REGISTER_CVAR3("r_ShadowsParticleKernelSize", CV_r_ShadowsParticleKernelSize, 1.f, VF_DUMPTODISK,
        "Blur kernel size for particles shadows.\n"
        "Usage: r_ShadowsParticleKernelSize [0.0 hard edge - x for blur], 1. is default");

    REGISTER_CVAR3("r_ShadowsParticleJitterAmount", CV_r_ShadowsParticleJitterAmount, 0.5f, VF_DUMPTODISK,
        "Amount of jittering for particles shadows.\n"
        "Usage: r_ShadowsParticleJitterAmount [x], 0.5 is default");

    REGISTER_CVAR3("r_ShadowsParticleAnimJitterAmount", CV_r_ShadowsParticleAnimJitterAmount, 1.f, VF_DUMPTODISK,
        "Amount of animated jittering for particles shadows.\n"
        "Usage: r_ShadowsParticleJitterAmount [x], 1. is default");

    REGISTER_CVAR3("r_ShadowsParticleNormalEffect", CV_r_ShadowsParticleNormalEffect, 1.f, VF_DUMPTODISK,
        "Shadow taps on particles affected by normal and intensity (breaks lines and uniformity of shadows).\n"
        "Usage: r_ShadowsParticleNormalEffect [x], 1. is default");

    DefineConstIntCVar3("r_ShadowGenMode", CV_r_ShadowGenMode, 1, VF_NULL,
        "0=Use Frustums Mask\n"
        "1=Regenerate all sides\n"
        "Usage: r_ShadowGenMode [0/1]");

    REGISTER_CVAR3_CB("r_ShadowsCache", CV_r_ShadowsCache, 0, VF_NULL,
        "Replace all sun cascades above cvar value with cached (static) shadow map: 0=no cached shadows, 1=replace first cascade and up, 2=replace second cascade and up,...",
        OnChange_CachedShadows);

    REGISTER_CVAR3_CB("r_ShadowsCacheFormat", CV_r_ShadowsCacheFormat, 1, VF_NULL,
        "0=use D32 texture format for shadow cache\n"
        "1=use D16 texture format for shadow cache\n",
        OnChange_CachedShadows);

    REGISTER_STRING_CB("r_ShadowsCacheResolutions", "", VF_RENDERER_CVAR, "Shadow cache resolution per cascade. ", OnChange_CachedShadows);

    REGISTER_CVAR3("r_ShadowsNearestMapResolution", CV_r_ShadowsNearestMapResolution, 4096, VF_REQUIRE_APP_RESTART,
        "Nearest shadow map resolution. Default: 4096");

    REGISTER_CVAR3("r_ShadowsScreenSpace", CV_r_ShadowsScreenSpace, 0, VF_NULL,
        "Include screen space tracing into shadow computations\n"
        "Helps reducing artifacts caused by limited shadow map resolution and biasing\n"
        "Applied only in the near range and supposed to be used mostly in the cutscenes for better shadows on character faces");

    DefineConstIntCVar3("r_ShadowsUseClipVolume", CV_r_ShadowsUseClipVolume, SHADOWS_CLIP_VOL_DEFAULT_VAL, VF_DUMPTODISK,
        ".\n"
        "Usage: r_ShadowsUseClipVolume [0=Disable/1=Enable");

    DefineConstIntCVar3("r_ShadowTexFormat", CV_r_shadowtexformat, 0, VF_NULL,
        "0=use D32 texture format for depth map\n"
        "1=use D16 texture format for depth map\n"
        "2=use D24S8 texture format for depth map\n"
        "Usage: r_ShadowTexFormat [0-2]");

    DefineConstIntCVar3("r_ShadowsMaskResolution", CV_r_ShadowsMaskResolution, 0, VF_NULL,
        "0=per pixel shadow mask\n"
        "1=horizontal half resolution shadow mask\n"
        "2=horizontal and vertical half resolution shadow mask\n"
        "Usage: r_ShadowsMaskResolution [0/1/2]");
    DefineConstIntCVar3("r_ShadowsMaskDownScale", CV_r_ShadowsMaskDownScale, 0, VF_NULL,
        "Saves video memory by using lower resolution for shadow masks except first one\n"
        "0=per pixel shadow mask\n"
        "1=half resolution shadow mask\n"
        "Usage: r_ShadowsMaskDownScale [0/1]");
    REGISTER_CVAR3("r_CBufferUseNativeDepth", CV_r_CBufferUseNativeDepth,   CBUFFER_NATIVE_DEPTH_DEAFULT_VAL, VF_NULL,
        "1= enable, 0 = disable\n"
        "Usage: r_CBufferUseNativeDepth [0/1]");
    DefineConstIntCVar3("r_ShadowsStencilPrePass", CV_r_ShadowsStencilPrePass, 1, VF_NULL,
        "1=Use Stencil pre-pass for shadows\n"
        "Usage: r_ShadowsStencilPrePass [0/1]");
    REGISTER_CVAR3("r_ShadowsDepthBoundNV", CV_r_ShadowsDepthBoundNV, 0, VF_NULL,
        "1=use NV Depth Bound extension\n"
        "Usage: r_ShadowsDepthBoundNV [0/1]");
    REGISTER_CVAR3("r_ShadowsPCFiltering", CV_r_ShadowsPCFiltering, 1, VF_NULL,
        "1=use PCF for shadows\n"
        "Usage: r_ShadowsPCFiltering [0/1]");
    REGISTER_CVAR3_CB("r_ShadowJittering", CV_r_shadow_jittering, 3.4f, VF_NULL,
        "Shadow map jittering radius.\n"
        "In PC the only use of this cvar is to instantly see the effects of diferent jittering values,\n"
        "because any value set here will be overwritten by ToD animation (only in PC) as soon as ToD changes.\n"
        "Usage: r_ShadowJittering [0=off]", OnChangeShadowJitteringCVar);
    SetShadowJittering(CV_r_shadow_jittering);   // need to do this because the registering process can modify the default value (getting it from the .cfg) and will not notify the call back
    DefineConstIntCVar3("r_DebugLights", CV_r_debuglights, 0, VF_CHEAT,
        "Display dynamic lights for debugging.\n"
        "Usage: r_DebugLights [0/1/2/3]\n"
        "Default is 0 (off). Set to 1 to display centers of light sources,\n"
        "or set to 2 to display light centers and attenuation spheres, 3 to get light properties to the screen");
    DefineConstIntCVar3("r_ShadowsGridAligned", CV_r_ShadowsGridAligned, 1, VF_DUMPTODISK,
        "Selects algorithm to use for shadow mask generation:\n"
        "0 - Disable shadows snapping\n"
        "1 - Enable shadows snapping");
    DefineConstIntCVar3("r_ShadowPass", CV_r_ShadowPass, 1, VF_NULL,
        "Process shadow pass");
    DefineConstIntCVar3("r_ShadowGen", CV_r_ShadowGen, 1, VF_NULL,
        "0=disable shadow map updates, 1=enable shadow map updates");
    DefineConstIntCVar3("r_ShadowPoolMaxFrames", CV_r_ShadowPoolMaxFrames, 30, VF_NULL,
        "Maximum number of frames a shadow can exist in the pool");
    REGISTER_CVAR3("r_ShadowPoolMaxTimeslicedUpdatesPerFrame", CV_r_ShadowPoolMaxTimeslicedUpdatesPerFrame, 1, VF_NULL,
        "Max number of time sliced shadow pool updates allowed per frame");
    REGISTER_CVAR3("r_ShadowCastingLightsMaxCount", CV_r_ShadowCastingLightsMaxCount, 32, VF_REQUIRE_APP_RESTART,
        "Maximum number of simultaneously visible shadow casting lights");

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_CPP_SECTION_17
    #include AZ_RESTRICTED_FILE(Renderer_cpp)
#endif

    REGISTER_CVAR3_CB("r_HeightMapAO", CV_r_HeightMapAO, 1, VF_NULL,
        "Large Scale Ambient Occlusion based on height map approximation of the scene\n"
        "0=off, 1=quarter resolution, 2=half resolution, 3=full resolution",
        OnChange_CachedShadows);
    REGISTER_CVAR3("r_HeightMapAOAmount", CV_r_HeightMapAOAmount, 1.0f, VF_NULL,
        "Height Map Ambient Occlusion Amount");
    REGISTER_CVAR3_CB("r_HeightMapAOResolution", CV_r_HeightMapAOResolution, 2048, VF_NULL,
        "Texture resolution of the height map used for HeightMapAO", OnChange_CachedShadows);
    REGISTER_CVAR3_CB("r_HeightMapAORange", CV_r_HeightMapAORange, 1000.f, VF_NULL,
        "Range of height map AO around viewer in meters", OnChange_CachedShadows);

    REGISTER_CVAR3("r_RenderMeshHashGridUnitSize", CV_r_RenderMeshHashGridUnitSize, .5f, VF_NULL, "Controls density of render mesh triangle indexing structures");

    DefineConstIntCVar3("r_TerrainAO", CV_r_TerrainAO, 7, 0, "7=Activate terrain AO deferred passes");
    DefineConstIntCVar3("r_TerrainAO_FadeDist", CV_r_TerrainAO_FadeDist, 8, 0, "Controls sky light fading in tree canopy in Z direction");

    DefineConstIntCVar3("r_LightsSinglePass", CV_r_lightssinglepass, 1, VF_NULL, "");

    DefineConstIntCVar3("r_ShowDynTextures", CV_r_showdyntextures, 0, VF_NULL,
        "Display a dyn. textures, filtered by r_ShowDynTexturesFilter\n"
        "Usage: r_ShowDynTextures 0/1/2\n"
        "Default is 0. Set to 1 to show all dynamic textures or 2 to display only the ones used in this frame\n"
        "Textures are sorted by memory usage");

    REGISTER_CVAR3("r_ShowDynTexturesMaxCount", CV_r_ShowDynTexturesMaxCount, 36, VF_NULL,
        "Allows to adjust number of textures shown on the screen\n"
        "Usage: r_ShowDynTexturesMaxCount [1...36]\n"
        "Default is 36");

    CV_r_ShowDynTexturesFilter = REGISTER_STRING("r_ShowDynTexturesFilter", "*", VF_NULL,
            "Usage: r_ShowDynTexturesFilter *end\n"
            "Usage: r_ShowDynTexturesFilter *mid*\n"
            "Usage: r_ShowDynTexturesFilter start*\n"
            "Default is *. Set to 'pattern' to show only specific textures (activate r_ShowDynTextures)");

    CV_r_ShaderCompilerServer = REGISTER_STRING("r_ShaderCompilerServer", "127.0.0.1", VF_NULL,
            "Usage: r_ShaderCompilerServer 127.0.0.1 \n"
            "Default is 127.0.0.1 ");

    CV_r_ShaderCompilerFolderSuffix = REGISTER_STRING("r_ShaderCompilerFolderSuffix", "", VF_NULL,
            "Usage: r_ShaderCompilerFolderSuffix suffix \n"
            "Default is empty. Set to some other value to append this suffix to the project name when compiling shaders");

    {
        const SFileVersion& ver = gEnv->pSystem->GetFileVersion();

        char versionString[128];
        memset(versionString, 0, sizeof(versionString));
        azsprintf(versionString, "Build Version: %d.%d.%d.%d", ver.v[3], ver.v[2], ver.v[1], ver.v[0]);

        CV_r_ShaderEmailTags = REGISTER_STRING("r_ShaderEmailTags", versionString, VF_NULL,
                "Adds optional tags to shader error emails e.g. own name or build run\n"
                "Usage: r_ShaderEmailTags \"some set of tags or text\" \n"
                "Default is build version ");
    }

    {
        CV_r_ShaderEmailCCs = REGISTER_STRING("r_ShaderEmailCCs", "", VF_NULL,
                "Adds optional CC addresses to shader error emails\n"
                "Usage: r_ShaderEmailCCs \"email1@your_domain.com;email2@your_domain.com\" \n"
                "Default is empty ");
    }

    REGISTER_CVAR3("r_ShaderCompilerPort", CV_r_ShaderCompilerPort, 61453, VF_NULL,
        "set user defined port of the shader compile server.\n"
        "Usage: r_ShaderCompilerPort 61453 #\n"
        "Default is 61453");

    REGISTER_CVAR3("r_ShaderCompilerDontCache", CV_r_ShaderCompilerDontCache, 0, VF_NULL,
        "Disables caching on server side.\n"
        "Usage: r_ShaderCompilerDontCache 0 #\n"
        "Default is 0");

    REGISTER_CVAR3("r_RC_AutoInvoke", CV_r_rc_autoinvoke, (gEnv->pSystem->IsDevMode() ? 1 : 0), VF_NULL,
        "Enable calling the resource compiler (rc.exe) to compile assets at run-time if the date check\n"
        "shows that the destination is older or does not exist.\n"
        "Usage: r_RC_AutoInvoke 0 (default is 1)");

    REGISTER_CVAR3("r_dofMinZ", CV_r_dofMinZ, 0.0f, VF_NULL,
        "Set dof min z distance, anything behind this distance will get out focus. (good default value 0.4) \n");

    REGISTER_CVAR3("r_dofMinZScale", CV_r_dofMinZScale, 0.0f, VF_NULL,
        "Set dof min z out of focus strenght (good default value - 1.0f)\n");

    REGISTER_CVAR3("r_dofMinZBlendMult", CV_r_dofMinZBlendMult, 1.0f, VF_NULL,
        "Set dof min z blend multiplier (bigger value means faster blendind transition)\n");

    REGISTER_CVAR3("r_Refraction", CV_r_Refraction, 1, VF_NULL,
        "Enables refraction.\n"
        "Usage: r_Refraction [0/1]\n"
        "Default is 1 (on). Set to 0 to disable.");

    REGISTER_CVAR3_CB("r_sunshafts", CV_r_sunshafts, SUNSHAFTS_DEFAULT_VAL, VF_NULL,
        "Enables sun shafts.\n"
        "Usage: r_sunshafts [0/1/2]\n"
        "Usage: r_sunshafts = 2: enabled with occlusion\n"
        "Default is 1 (on). Set to 0 to disable.",
        OnChange_CV_r_SunShafts);

    REGISTER_CVAR3_CB("r_PostProcessEffects", CV_r_PostProcess_CB, 1, VF_NULL,
        "Enables post processing special effects.\n"
        "Usage: r_PostProcessEffects [0/1/2]\n"
        "Default is 1 (enabled). 2 enables and displays active effects",
        OnChange_CV_r_PostProcess);
    CV_r_PostProcess = CV_r_PostProcess_CB;

    DefineConstIntCVar3("r_PostProcessFilters", CV_r_PostProcessFilters, 1, VF_CHEAT,
        "Enables post processing special effects filters.\n"
        "Usage: r_PostProcessEffectsFilters [0/1]\n"
        "Default is 1 (enabled). 0 disabled");

    DefineConstIntCVar3("r_PostProcessGameFx", CV_r_PostProcessGameFx, 1, VF_CHEAT,
        "Enables post processing special effects game fx.\n"
        "Usage: r_PostProcessEffectsGameFx [0/1]\n"
        "Default is 1 (enabled). 0 disabled");

    REGISTER_CVAR3("r_PostProcessReset", CV_r_PostProcessReset, 0, VF_CHEAT,
        "Enables post processing special effects reset.\n"
        "Usage: r_PostProcessEffectsReset [0/1]\n"
        "Default is 0 (disabled). 1 enabled");


    DefineConstIntCVar3("r_MergeShadowDrawcalls", CV_r_MergeShadowDrawcalls, 1, VF_NULL,
        "Enabled Merging of RenderChunks for ShadowRendering\n"
        "Default is 1 (enabled). 0 disabled");

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    REGISTER_CVAR3("r_FinalOutputsRGB", CV_r_FinalOutputsRGB, 1, VF_NULL,
        "Enables sRGB final output.\n"
        "Usage: r_FinalOutputsRGB [0/1]");

    REGISTER_CVAR3("r_FinalOutputAlpha", CV_r_FinalOutputAlpha, 0, VF_NULL,
        "Enables alpha in final output. 0\n"
        "Usage: r_FinalOutputAlpha [0/1/2]\n"
        "Usage: r_FinalOutputAlpha = 0: no alpha (default)\n"
        "Usage: r_FinalOutputAlpha = 1: full opaque\n"
        "Usage: r_FinalOutputAlpha = 2: depth-based\n");

    REGISTER_CVAR3("r_RTT", CV_r_RTT, 1, VF_NULL,
        "Enables render scene to texture. \n"
        "Usage: r_RTT [0/1]\n");
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

    REGISTER_CVAR3("r_ColorRangeCompression", CV_r_colorRangeCompression, 0, VF_NULL,
        "Enables color range compression to account for the limited RGB range of TVs.\n"
        "  0: Disabled (full extended range)\n"
        "  1: Range 16-235\n");

    REGISTER_CVAR3("r_ColorGrading", CV_r_colorgrading, COLOR_GRADING_DEFAULT_VAL, VF_NULL,
        "Enables color grading.\n"
        "Usage: r_ColorGrading [0/1]");

    REGISTER_CVAR3("r_ColorGradingSelectiveColor", CV_r_colorgrading_selectivecolor, 1, VF_NULL,
        "Enables color grading.\n"
        "Usage: r_ColorGradingSelectiveColor [0/1]");

    DefineConstIntCVar3("r_ColorGradingLevels", CV_r_colorgrading_levels, 1, VF_NULL,
        "Enables color grading.\n"
        "Usage: r_ColorGradingLevels [0/1]");

    DefineConstIntCVar3("r_ColorGradingFilters", CV_r_colorgrading_filters, 1, VF_NULL,
        "Enables color grading.\n"
        "Usage: r_ColorGradingFilters [0/1]");

    REGISTER_CVAR3("r_ColorGradingCharts", CV_r_colorgrading_charts, 1, VF_NULL,
        "Enables color grading via color charts.\n"
        "Usage: r_ColorGradingCharts [0/1]");

    REGISTER_CVAR3("r_ColorGradingChartsCache", CV_r_ColorgradingChartsCache, 4, VF_CVARGRP_IGNOREINREALVAL, // Needs to be ignored from cvar group state otherwise some adv graphic options will show as custom when using multi GPU due to mgpu.cfg being reloaded after changing syspec and graphic options have changed
        "Enables color grading charts update caching.\n"
        "Usage: r_ColorGradingCharts [0/1/2/etc]\n"
        "Default is 4 (update every 4 frames), 0 - always update, 1- update every other frame");

    DefineConstIntCVar3("r_CloudsUpdateAlways", CV_r_cloudsupdatealways, 0, VF_NULL,
        "Toggles updating of clouds each frame.\n"
        "Usage: r_CloudsUpdateAlways [0/1]\n"
        "Default is 0 (off)");
    DefineConstIntCVar3("r_CloudsDebug", CV_r_cloudsdebug, 0, VF_NULL,
        "Toggles debugging mode for clouds."
        "Usage: r_CloudsDebug [0/1/2]\n"
        "Usage: r_CloudsDebug = 1: render just screen imposters\n"
        "Usage: r_CloudsDebug = 2: render just non-screen imposters\n"
        "Default is 0 (off)");

    REGISTER_CVAR3("r_DynTexMaxSize", CV_r_dyntexmaxsize, 48, VF_NULL, ""); // 48 Mb
    DefineConstIntCVar3("r_TexPreallocateAtlases", CV_r_texpreallocateatlases, TEXPREALLOCATLAS_DEFAULT_VAL, VF_NULL, "");
    REGISTER_CVAR3("r_TexAtlasSize", CV_r_texatlassize, 1024, VF_NULL, ""); // 1024x1024

    DefineConstIntCVar3("r_TexPostponeLoading", CV_r_texpostponeloading, 1, VF_NULL, "");
    REGISTER_CVAR3("r_DynTexAtlasCloudsMaxSize", CV_r_dyntexatlascloudsmaxsize, 32, VF_NULL, ""); // 32 Mb

    const int defaultbuffer_bank_size = 4;
    const int default_transient_bank_size = 4;
    const int default_cb_bank_size = 4;
    const int defaultt_cb_watermark = 64;
    const int defaultbuffer_pool_max_allocs = 0xfff0;
    const int defaultbuffer_pool_defrag = 0;

    REGISTER_CVAR3("r_buffer_banksize", CV_r_buffer_banksize, defaultbuffer_bank_size, VF_CHEAT, "the bank size in MB for buffer pooling");
    REGISTER_CVAR3("r_constantbuffer_banksize", CV_r_constantbuffer_banksize, default_cb_bank_size, VF_CHEAT, "the bank size in MB for constant buffers pooling");
    REGISTER_CVAR3("r_constantbuffer_watermarm", CV_r_constantbuffer_watermark, defaultt_cb_watermark, VF_CHEAT, "the threshold aftyer which constants buffers will reclaim memory");
    REGISTER_CVAR3("r_buffer_sli_workaround", CV_r_buffer_sli_workaround, 0, VF_NULL, "enable SLI workaround for buffer pooling");
    REGISTER_CVAR3("r_transient_pool_size", CV_r_transient_pool_size, default_transient_bank_size, VF_CHEAT, "the bank size in MB for the transient pool");
    DefineConstIntCVar3("r_buffer_enable_lockless_updates", CV_r_buffer_enable_lockless_updates, 1, VF_CHEAT, "enable/disable lockless buffer updates on platforms that support them");
    DefineConstIntCVar3("r_enable_full_gpu_sync", CV_r_enable_full_gpu_sync, 0, VF_CHEAT, "enable full gpu synchronization for debugging purposes on the every buffer I/O operation (debugging only)");
    REGISTER_CVAR3("r_buffer_pool_max_allocs", CV_r_buffer_pool_max_allocs, defaultbuffer_pool_max_allocs, VF_CHEAT, "the maximum number of allocations per buffer pool if defragmentation is enabled");
    REGISTER_CVAR3("r_buffer_pool_defrag_static", CV_r_buffer_pool_defrag_static, defaultbuffer_pool_defrag, VF_CHEAT, "enable/disable runtime defragmentation of static buffers");
    REGISTER_CVAR3("r_buffer_pool_defrag_dynamic", CV_r_buffer_pool_defrag_dynamic, defaultbuffer_pool_defrag, VF_CHEAT, "enable/disable runtime defragmentation of dynamic buffers");
    REGISTER_CVAR3("r_buffer_pool_defrag_max_moves", CV_r_buffer_pool_defrag_max_moves, 64, VF_CHEAT, "maximum number of moves the defragmentation system is allowed to perform per frame");

    REGISTER_CVAR3("r_TexMinAnisotropy", CV_r_texminanisotropy, 0, VF_REQUIRE_LEVEL_RELOAD,
        "Specifies the minimum level allowed for anisotropic texture filtering.\n"
        "0(default) means abiding by the filtering setting in each material, except possibly being capped by r_TexMaxAnisotropy.");
    REGISTER_CVAR3("r_TexMaxAnisotropy", CV_r_texmaxanisotropy, TEXMAXANISOTROPY_DEFAULT_VAL, VF_REQUIRE_LEVEL_RELOAD,
        "Specifies the maximum level allowed for anisotropic texture filtering.");
    DefineConstIntCVar3("r_TexNoAnisoAlphaTest", CV_r_texNoAnisoAlphaTest, TEXNOANISOALPHATEST_DEFAULT_VAL, VF_DUMPTODISK,
        "Disables anisotropic filtering on alpha-tested geometry like vegetation.\n");
    DefineConstIntCVar3("r_TexLog", CV_r_texlog, 0, VF_NULL,
        "Configures texture information logging.\n"
        "Usage: r_TexLog #\n"
        "where # represents:\n"
        " 0: Texture logging off\n"
        " 1: Texture information logged to screen\n"
        " 2: All loaded textures logged to 'UsedTextures.txt'\n"
        " 3: Missing textures logged to 'MissingTextures.txt");
    DefineConstIntCVar3("r_TexNoLoad", CV_r_texnoload, 0, VF_NULL,
        "Disables loading of textures.\n"
        "Usage: r_TexNoLoad [0/1]\n"
        "When 1 texture loading is disabled.");
    DefineConstIntCVar3("r_TexBlockOnLoad", CV_r_texBlockOnLoad, 0, VF_NULL,
        "When loading a texture, block until resource compiler has finished compiling it.\n"
        "Usage: r_TexBlockOnLoad [0/1]\n"
        "When 1 renderer will block and wait on the resource compiler.");

    REGISTER_CVAR3("r_RenderTargetPoolSize", CV_r_rendertargetpoolsize, 0, VF_NULL,
        "Size of pool for render targets in MB.\n"
        "Default is 50(MB).");

    REGISTER_CVAR3("r_texturesskiplowermips", CV_r_texturesskiplowermips, 0, VF_NULL,
        "Enabled skipping lower mips for deprecated platform.\n");

    int nDefaultTexPoolSize = 512;

    REGISTER_CVAR3("r_TexturesStreamPoolSize", CV_r_TexturesStreamPoolSize, nDefaultTexPoolSize, VF_NULL,
        "Size of texture streaming pool in MB.\n");

    REGISTER_CVAR3("r_TexturesStreamPoolSecondarySize", CV_r_TexturesStreamPoolSecondarySize, 0, VF_NULL,
        "Size of secondary pool for textures in MB.");

    REGISTER_CVAR3("r_TexturesStreamingSync", CV_r_texturesstreamingsync, 0, VF_RENDERER_CVAR,
        "Force only synchronous texture streaming.\n"
        "All textures will be streamed in the main thread. Useful for debug purposes.\n"
        "Usage: r_TexturesStreamingSync [0/1]\n"
        "Default is 0 (off).");
    DefineConstIntCVar3("r_TexturesStreamingResidencyEnabled", CV_r_texturesstreamingResidencyEnabled, 1, VF_NULL,
        "Toggle for resident textures streaming support.\n"
        "Usage: r_TexturesStreamingResidencyEnabled [toggle]"
        "Default is 0, 1 for enabled");
    REGISTER_CVAR3("r_TexturesStreamingResidencyTimeTestLimit", CV_r_texturesstreamingResidencyTimeTestLimit, 5.0f, VF_NULL,
        "Time limit to use for mip thrashing calculation in seconds.\n"
        "Usage: r_TexturesStreamingResidencyTimeTestLimit [time]"
        "Default is 5 seconds");
    REGISTER_CVAR3("r_TexturesStreamingResidencyTime", CV_r_texturesstreamingResidencyTime, 10.0f, VF_NULL,
        "Time to keep textures resident for before allowing them to be removed from memory.\n"
        "Usage: r_TexturesStreamingResidencyTime [Time]\n"
        "Default is 10 seconds");
    REGISTER_CVAR3("r_TexturesStreamingResidencyThrottle", CV_r_texturesstreamingResidencyThrottle, 0.5f, VF_NULL,
        "Ratio for textures to become resident.\n"
        "Usage: r_TexturesStreamingResidencyThrottle [ratio]"
        "Default is 0.5"
        "Max is 1.0 means textures will become resident sooner, Min 0.0 means textures will not become resident");
    REGISTER_CVAR3("r_TexturesStreamingMaxRequestedMB", CV_r_TexturesStreamingMaxRequestedMB, 2.f, VF_NULL,
        "Maximum amount of texture data requested from streaming system in MB.\n"
        "Usage: r_TexturesStreamingMaxRequestedMB [size]\n"
        "Default is 2.0(MB)");

    DefineConstIntCVar3("r_TexturesStreamingPostponeMips", CV_r_texturesstreamingPostponeMips, 0, VF_NULL,
        "Postpone loading of high res mipmaps to improve resolution ballance of texture streaming.\n"
        "Usage: r_TexturesStreamingPostponeMips [0/1]\n"
        "Default is 1 (on).");

    DefineConstIntCVar3("r_TexturesStreamingPostponeThresholdKB", CV_r_texturesstreamingPostponeThresholdKB, 1024, VF_NULL,
        "Threshold used to postpone high resolution mipmap loads in KB.\n"
        "Usage: r_TexturesStreamingPostponeThresholdKB [size]\n"
        "Default is 1024(KB)");
    DefineConstIntCVar3("r_texturesstreamingPostponeThresholdMip", CV_r_texturesstreamingPostponeThresholdMip, 1, VF_NULL,
        "Threshold used to postpone high resolution mipmaps.\n"
        "Usage: r_texturesstreamingPostponeThresholdMip [count]\n"
        "Default is 1");
    DefineConstIntCVar3("r_TexturesStreamingMinReadSizeKB", CV_r_texturesstreamingMinReadSizeKB, 64, VF_NULL,
        "Minimal read portion in KB.\n"
        "Usage: r_TexturesStreamingMinReadSizeKB [size]\n"
        "Default is 32(KB)");
    REGISTER_CVAR3("r_texturesstreamingSkipMips", CV_r_texturesstreamingSkipMips, 0, VF_NULL,
        "Number of top mips to ignore when streaming.\n");
    REGISTER_CVAR3("r_texturesstreamingMinUsableMips", CV_r_texturesstreamingMinUsableMips, 7, VF_NULL,
        "Minimum number of mips a texture should be able to use after applying r_texturesstreamingSkipMips.\n");
    REGISTER_CVAR3("r_texturesstreamingJobUpdate", CV_r_texturesstreamingJobUpdate, 1, VF_NULL,
        "Enable texture streaming update job");
#if defined(TEXSTRM_DEFERRED_UPLOAD)
    REGISTER_CVAR3("r_texturesstreamingDeferred", CV_r_texturesstreamingDeferred, 1, VF_NULL,
        "When enabled textures will be uploaded through a deferred context.\n");
#endif
#if defined(SUPPORTS_INPLACE_TEXTURE_STREAMING)
    REGISTER_CVAR3("r_texturesstreamingInPlace", CV_r_texturesstreamingInPlace, 1, VF_NULL,
        "When enabled textures will stream directly into video memory.\n");
#endif
    REGISTER_CVAR3("r_TexturesStreamingMaxRequestedJobs", CV_r_TexturesStreamingMaxRequestedJobs, 32, VF_NULL,
        "Maximum number of tasks submitted to streaming system.\n"
        "Usage: r_TexturesStreamingMaxRequestedJobs [jobs number]\n"
        "Default is 32 jobs");
    DefineConstIntCVar3("r_TexturesStreamingUpdateType", CV_r_texturesstreamingUpdateType, TEXSTREAMING_UPDATETYPE_DEFAULT_VAL, VF_NULL,
        "Texture streaming update type.\n"
        "Default is 0");
    DefineConstIntCVar3("r_TexturesStreamingPrecacheRounds", CV_r_texturesstreamingPrecacheRounds, 1, VF_NULL,
        "Number of precache rounds to include in active streamed texture lists.\n"
        "Default is 1");
    DefineConstIntCVar3("r_TexturesStreamingSuppress", CV_r_texturesstreamingSuppress, 0, VF_NULL,
        "Force unloading of all textures and suppress new stream tasks.\n"
        "Default is 0");
    REGISTER_CVAR3("r_TexturesStreamingMipBias", CV_r_TexturesStreamingMipBias, 0, VF_NULL,
        "Controls how texture LOD depends from distance to the objects.\n"
        "Increasing this value will reduce amount of memory required for textures.\n"
        "Usage: r_TexturesStreamingMipBias [-4..0..4]\n"
        "Default is 0.");
    REGISTER_CVAR3("r_TexturesStreamingMipClampDVD", CV_r_TexturesStreamingMipClampDVD, 1, VF_NULL,
        "Clamp the texture mip level to certain value when streaming from DVD. 1 will never allow highest mips to be loaded for example.\n"
        "Usage: r_TexturesStreamingMipClampDVD [0..4]\n"
        "Default is 1.");
    REGISTER_CVAR3("r_TexturesStreamingDisableNoStreamDuringLoad", CV_r_TexturesStreamingDisableNoStreamDuringLoad, 0, VF_NULL,
        "Load time optimisation. When enabled, textures flagged as non-streaming will still be streamed during level load, but will have a "
        "high priority stream request added in RT_Precache. Once streamed in, the texture will remain resident\n");
    DefineConstIntCVar3("r_TexturesStreamingMipFading", CV_r_texturesstreamingmipfading, 1, VF_NULL,
        "Controls how the new texture MIP appears after being streamed in.\n"
        "This variable influences only a visual quality of appearing texture details.\n"
        "Usage: r_TexturesStreamingMipFading [0/1]\n"
        "Default is 1 (enabled).");
    DefineConstIntCVar3("r_TexturesStreamingNoUpload", CV_r_texturesstreamingnoupload, 0, VF_NULL,
        "Disable uploading data into texture from system memory. Useful for debug purposes.\n"
        "Usage: r_TexturesStreamingNoUpload [0/1]\n"
        "Default is 0 (off).");
    DefineConstIntCVar3("r_TexturesStreamingOnlyVideo", CV_r_texturesstreamingonlyvideo, 0, VF_NULL,
        "Don't store system memory copy of texture. Applicable only for PC.\n"
        "Usage: r_TexturesStreamingOnlyVideo [0/1]\n"
        "Default is 0 (off).");

    DefineConstIntCVar3("r_TexturesDebugBandwidth", CV_r_TexturesDebugBandwidth, 0, VF_CHEAT,
        "Replaces all material textures with a small white texture to debug texture bandwidth utilization\n");

    DefineConstIntCVar3("r_TexturesStreaming", CV_r_texturesstreaming, TEXSTREAMING_DEFAULT_VAL, VF_REQUIRE_APP_RESTART,
        "Enables direct streaming of textures from disk during game.\n"
        "Usage: r_TexturesStreaming [0/1/2]\n"
        "Default is 0 (off). All textures save in native format with mips in a\n"
        "cache file. Textures are then loaded into texture memory from the cache.");

    DefineConstIntCVar3("r_TexturesStreamingDebug", CV_r_TexturesStreamingDebug, 0, VF_CHEAT,
        "Enables textures streaming debug mode. (Log uploads and remove unnecessary mip levels)\n"
        "Usage: r_TexturesStreamingDebug [0/1/2]\n"
        "Default is 0 (off)."
        "1 - texture streaming log."
        "2 - Show textures hit-parade based on streaming priorities"
        "3 - Show textures hit-parade based on the memory consumed");
    CV_r_TexturesStreamingDebugfilter = REGISTER_STRING("r_TexturesStreamingDebugFilter", "", VF_CHEAT, "Filters displayed textures by name in texture streaming debug mode\n");
    REGISTER_CVAR3("r_TexturesStreamingDebugMinSize", CV_r_TexturesStreamingDebugMinSize, 100, VF_NULL,
        "Filters displayed textures by size in texture streaming debug mode");
    REGISTER_CVAR3("r_TexturesStreamingDebugMinMip", CV_r_TexturesStreamingDebugMinMip, 100, VF_NULL,
        "Filters displayed textures by loaded mip in texture streaming debug mode");
    DefineConstIntCVar3("r_TexturesStreamingDebugDumpIntoLog", CV_r_TexturesStreamingDebugDumpIntoLog, 0, VF_NULL,
        "Dump content of current texture streaming debug screen into log");
    REGISTER_CVAR3("r_TextureLodDistanceRatio", CV_r_TextureLodDistanceRatio, -1, VF_NULL,
        "Controls dynamic LOD system for textures used in materials.\n"
        "Usage: r_TextureLodDistanceRatio [-1, 0 and bigger]\n"
        "Default is -1 (completely off). Value 0 will set full LOD to all textures used in frame.\n"
        "Values bigger than 0 will activate texture LOD selection depending on distance to the objects.");

    DefineConstIntCVar3("r_TextureCompiling", CV_r_texturecompiling, 1, VF_NULL,
        "Enables Run-time compilation and subsequent injection of changed textures from disk during editing.\n"
        "Usage: r_TextureCompiling [0/1]\n"
        "Default is 1 (on). Changes are tracked and passed through to the rendering.\n"
        "Compilation can also be muted by the r_RC_AutoInvoke config.");
    DefineConstIntCVar3("r_TextureCompilingIndicator", CV_r_texturecompilingIndicator, 0, VF_NULL,
        "Replaces the textures which are currently compiled by a violet indicator-texture.\n"
        "Usage: r_TextureCompilingIndicator [-1/0/1]\n"
        "Default is 0 (off). Textures are silently replaced by their updated versions without any indication.\n"
        "Negative values will also stop show indicators for compilation errors.\n"
        "Positive values will show indicators whenever a texture is subject to imminent changes.\n");

#ifndef STRIP_RENDER_THREAD
    DefineConstIntCVar3("r_MultiThreaded", CV_r_multithreaded, MULTITHREADED_DEFAULT_VAL, VF_NULL,
        "0=disabled, 1=enabling rendering in separate thread,\n"
        "2(default)=automatic detection\n"
        "should be activated before rendering");
#endif
    DefineConstIntCVar3("r_MultiThreadedDrawing", CV_r_multithreadedDrawing, 0, VF_NULL,
        "  0=disabled,\n"
        "  N=number of concurrent draw recording jobs,\n"
        " -1=Number is as large as the number of available worker threads");
    DefineConstIntCVar3("r_MultiThreadedDrawingActiveThreshold", CV_r_multithreadedDrawingActiveThreshold, 0, VF_NULL,
        "  0=disabled,\n"
        "  N=minimum number of draws per job,\n"
        "If there are not enough draws for all jobs it will dial down the number of jobs.");
    REGISTER_CVAR3("r_MultiGPU", CV_r_multigpu, 1, VF_NULL,
        "Toggles MGPU support. Should usually be set before startup.\n"
        "  0: force off\n"
        "  1: automatic detection (reliable with SLI, does not respect driver app profiles with Crossfire)\n");

    DefineConstIntCVar3("r_ShowNormals", CV_r_shownormals, 0, VF_CHEAT,
        "Toggles visibility of normal vectors.\n"
        "Usage: r_ShowNormals [0/1]"
        "Default is 0 (off).");
    DefineConstIntCVar3("r_ShowLines", CV_r_showlines, 0, VF_CHEAT,
        "Toggles visibility of wireframe overlay.\n"
        "Usage: r_ShowLines [0/1]\n"
        "Default is 0 (off).");
    REGISTER_CVAR3("r_NormalsLength", CV_r_normalslength, 0.2f, VF_CHEAT,
        "Sets the length of displayed vectors.\n"
        "r_NormalsLength 0.2\n"
        "Default is 0.2 (meters). Used with r_ShowTangents and r_ShowNormals.");
    DefineConstIntCVar3("r_ShowTangents", CV_r_showtangents, 0, VF_CHEAT,
        "Toggles visibility of three tangent space vectors.\n"
        "Usage: r_ShowTangents [0/1]\n"
        "Default is 0 (off).");
    DefineConstIntCVar3("r_ShowTimeGraph", CV_r_showtimegraph, 0, VF_NULL,
        "Configures graphic display of frame-times.\n"
        "Usage: r_ShowTimeGraph [0/1/2]\n"
        "\t1: Graph displayed as points."
        "\t2: Graph displayed as lines."
        "Default is 0 (off).");
#ifndef EXCLUDE_DOCUMENTATION_PURPOSE
    DefineConstIntCVar3("r_DebugFontRendering", CV_r_DebugFontRendering, 0, VF_CHEAT,
        "0=off, 1=display various features of the font rendering to verify function and to document usage");
#endif // EXCLUDE_DOCUMENTATION_PURPOSE
    DefineConstIntCVar3("profileStreaming", CV_profileStreaming, 0, VF_NULL,
        "Profiles streaming of different assets.\n"
        "Usage: profileStreaming [0/1/2]\n"
        "\t1: Graph displayed as points."
        "\t2: Graph displayed as lines."
        "Default is 0 (off).");
    DefineConstIntCVar3("r_GraphStyle", CV_r_graphstyle, 0, VF_NULL, "");
    DefineConstIntCVar3("r_ShowBufferUsage", CV_r_showbufferusage, 0, VF_NULL,
        "Shows usage of statically allocated buffers.\n"
        "Usage: r_ShowBufferUSage [0/1]\n"
        "Default is 0 (off).");
    REGISTER_CVAR3_CB("r_LogVBuffers", CV_r_logVBuffers, 0, VF_CHEAT | VF_CONST_CVAR,
        "Logs vertex buffers in memory to 'LogVBuffers.txt'.\n"
        "Usage: r_LogVBuffers [0/1]\n"
        "Default is 0 (off).",
        GetLogVBuffersStatic);
    DefineConstIntCVar3("r_LogTexStreaming", CV_r_logTexStreaming, 0, VF_CHEAT,
        "Logs streaming info to Direct3DLogStreaming.txt\n"
        "0: off\n"
        "1: normal\n"
        "2: extended");
    DefineConstIntCVar3("r_LogShaders", CV_r_logShaders, 0, VF_CHEAT,
        "Logs shaders info to Direct3DLogShaders.txt\n"
        "0: off\n"
        "1: normal\n"
        "2: extended");

#if defined(WIN32) || defined(WIN64)
    const int r_flush_default = 0;
#else
    const int r_flush_default = 1;  // TODO: Check if r_flush should be disabled on other platforms as well
#endif
    REGISTER_CVAR3("r_Flush", CV_r_flush, r_flush_default, VF_NULL, "");

    REGISTER_CVAR3("r_minimizeLatency", CV_r_minimizeLatency, 0, VF_REQUIRE_APP_RESTART,
        "Initializes and drives renderer to minimize display latency as much as possible.\n"
        "As such only a double buffer swap chain will be created.\n"
        "Maximum frame latency will be set to 1 on DXGI-supporting platforms\n"
        "as well as frames flushed after Present() if r_Flush is enabled.");

    DefineConstIntCVar3("r_ShadersDebug", CV_r_shadersdebug, 0, VF_DUMPTODISK,
        "Enable special logging when shaders become compiled\n"
        "Usage: r_ShadersDebug [0/1/2/3/4]\n"
        " 1 = assembly into directory Main/{Game}/shaders/cache/d3d9\n"
        " 2 = compiler input into directory Main/{Game}/testcg\n"
        " 3 = compiler input with debug information (useful for PIX etc./{Game}/testcg_1pass\n"
        " 4 = compiler input with debug information, but optimized shaders\n"
        "Default is 0 (off)");

#if !defined(CONSOLE)
    REGISTER_CVAR3("r_ShadersOrbis", CV_r_shadersorbis, 0, VF_NULL, "");
    REGISTER_CVAR3("r_ShadersDX11", CV_r_shadersdx11, 0, VF_NULL, "");
    REGISTER_CVAR3("r_ShadersGL4", CV_r_shadersGL4, 0, VF_NULL, "");
    REGISTER_CVAR3("r_ShadersGLES3", CV_r_shadersGLES3, 0, VF_NULL, "");
    REGISTER_CVAR3("r_ShadersMETAL", CV_r_shadersMETAL, 0, VF_NULL, "");
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_CPP_SECTION_15
    #include AZ_RESTRICTED_FILE(Renderer_cpp)
#endif
    REGISTER_CVAR3("r_ShadersPlatform", CV_r_shadersPlatform, static_cast<int>(AZ::PlatformID::PLATFORM_MAX), VF_NULL, "");
#endif

    DefineConstIntCVar3("r_ShadersIgnoreIncludesChanging", CV_r_shadersignoreincludeschanging, 0, VF_NULL, "");
    DefineConstIntCVar3("r_ShadersLazyUnload", CV_r_shaderslazyunload, 0, VF_NULL, "");

    REGISTER_CVAR3("r_ShadersPreactivate", CV_r_shaderspreactivate, SHADERS_PREACTIVATE_DEFAULT_VAL, VF_DUMPTODISK, "");

    REGISTER_CVAR3_CB("r_ShadersAllowCompilation", CV_r_shadersAllowCompilation, SHADERS_ALLOW_COMPILATION_DEFAULT_VAL, VF_NULL, "", OnChange_CV_r_ShadersAllowCompiliation);

    DefineConstIntCVar3("r_ShadersRemoteCompiler", CV_r_shadersremotecompiler, 0, VF_DUMPTODISK, "Enables remote shader compilation on dedicated machine");
    REGISTER_CVAR3("r_ShadersAsyncCompiling", CV_r_shadersasynccompiling, 1, VF_NULL,
        "Enable asynchronous shader compiling\n"
        "Usage: r_ShadersAsyncCompiling [0/1/2/3]\n"
        " 0 = off, (stalling) shaders compiling\n"
        " 1 = on, shaders are compiled in parallel, missing shaders are rendered in yellow\n"
        " 2 = on, shaders are compiled in parallel, missing shaders are not rendered\n"
        " 3 = on, shaders are compiled in parallel in precache mode");
    REGISTER_CVAR3("r_ShadersAsyncActivation", CV_r_shadersasyncactivation, 1, VF_NULL,
        "Enable asynchronous shader activation\n"
        "Usage: r_ShadersAsyncActivation [0/1]\n"
        " 0 = off, (stalling) synchronous shaders activation\n"
        " 1 = on, shaders are activated/streamed asynchronously\n");

    DefineConstIntCVar3("r_ShadersEditing", CV_r_shadersediting, 0, VF_NULL, "Force all cvars to settings, which allow shader editing");

    DefineConstIntCVar3("r_ShadersCompileAutoActivate", CV_r_shaderscompileautoactivate, 0, VF_NULL, "Automatically reenable shader compilation if outdated shader is detected");

    REGISTER_CVAR3("r_AssetProcessorShaderCompiler", CV_r_AssetProcessorShaderCompiler, 0, VF_NULL,
        "Enables using the Asset Processor as a proxy for the shader compiler if its not reachable directly.\n"
        "Usage: r_AssetProcessorShaderCompiler 1\n"
        "Default is 0 (disabled)");

    DefineConstIntCVar3("r_ReflectTextureSlots", CV_r_ReflectTextureSlots, 1, VF_NULL, "Reflect texture slot information from shader");

    REGISTER_CVAR3("r_ShadersAsyncMaxThreads", CV_r_shadersasyncmaxthreads, 1, VF_DUMPTODISK, "");
    REGISTER_CVAR3("r_ShadersCacheDeterministic", CV_r_shaderscachedeterministic, 1, VF_NULL, "Ensures that 2 shaderCaches built from the same source are binary equal");
    DefineConstIntCVar3("r_ShadersPrecacheAllLights", CV_r_shadersprecachealllights, 1, VF_NULL, "");
    REGISTER_CVAR3("r_ShadersSubmitRequestline", CV_r_shaderssubmitrequestline, 1, VF_NULL, "");
    REGISTER_CVAR3("r_ShadersUseInstanceLookUpTable", CV_r_shadersuseinstancelookuptable, 0, VF_NULL,
        "Use lookup table to search for shader instances. Speeds up the process, but uses more memory. Handy for shader generation.");

    bool bShaderLogCacheMisses = true;
#if defined(_RELEASE)
    bShaderLogCacheMisses = false;
#endif
    REGISTER_CVAR3("r_ShadersLogCacheMisses", CV_r_shaderslogcachemisses, bShaderLogCacheMisses ? 2 : 0, VF_NULL,
        "Log all shader caches misses on HD (both level and global shader cache misses).\n" 
        "0 = No logging to disk or TTY\n"
        "1 = Logging to disk only\n"
        "2 = Logging to disk and TTY (default)");

    REGISTER_CVAR3("r_ShadersImport", CV_r_shadersImport, 0, VF_NULL,
        "0 = Off\n"
        "1 = Import pre-parsed shader reflection information from .fxb files if they exist for a related .cfx which skips expensive parsing of .cfx files in RT_ParseShader. If a .fxb exists for a shader but an individual permutation is missing, then fallback to the slow .cfx parsing for that permutation."
        "2 = Import from the .fxb files, but do not fallback if import fails.  Missing shader permutations from .fxb files will be ignored."
        "3 = Same behavior as 1, but only when running Performance/Release configurations.  Debug/Profile builds will disable this and set it to 0 (for an improved development experience).  This allows us to continue compiling shaders in Debug/Profile configurations and run optimally in Performance/Release");

    REGISTER_CVAR3("r_ShadersExport", CV_r_shadersExport, 1, VF_NULL,
        "0 off, 1 allow export of .fxb files during shader cache generation.");

    REGISTER_CVAR3("r_ShadersCacheUnavailableShaders", CV_r_shadersCacheUnavailableShaders, 0, VF_NULL,
        "0 off (default), 1 cache unavailable shaders to avoid requesting their compilation in future executions.");

    DefineConstIntCVar3("r_ShadersUseLLVMDirectXCompiler", CV_r_ShadersUseLLVMDirectXCompiler, 0, VF_NULL,
        "Shaders will be compiled using the LLVM DirectX Shader Compiler (GL4, GLES3 and METAL).\n"
        "Usage: r_ShadersUseLLVMDirectXCompiler 1\n"
        "Default is 0 (disabled)");

    DefineConstIntCVar3("r_DebugRenderMode", CV_r_debugrendermode, 0, VF_CHEAT, "");
    DefineConstIntCVar3("r_DebugRefraction", CV_r_debugrefraction, 0, VF_CHEAT,
        "Debug refraction usage. Displays red instead of refraction\n"
        "Usage: r_DebugRefraction\n"
        "Default is 0 (off)");

    DefineConstIntCVar3("r_MeshPrecache", CV_r_meshprecache, 1, VF_NULL, "");
    REGISTER_CVAR3("r_MeshPoolSize", CV_r_meshpoolsize, RENDERER_DEFAULT_MESHPOOLSIZE, VF_NULL,
        "The size of the pool for render data in kilobytes. "
        "Disabled by default on PC (mesh data allocated on heap)."
        "Enabled by default on consoles. Requires app restart to change.");
    REGISTER_CVAR3("r_MeshInstancePoolSize", CV_r_meshinstancepoolsize, RENDERER_DEFAULT_MESHINSTANCEPOOLSIZE, VF_NULL,
        "The size of the pool for volatile render data in kilobytes. "
        "Disabled by default on PC (mesh data allocated on heap)."
        "Enabled by default on consoles. Requires app restart to change.");

    CV_r_excludeshader = REGISTER_STRING("r_ExcludeShader", "0", VF_CHEAT,
            "Exclude the named shader from the render list.\n"
            "Usage: r_ExcludeShader ShaderName\n"
            "Sometimes this is useful when debugging.");

    CV_r_excludemesh = REGISTER_STRING("r_ExcludeMesh", "", VF_CHEAT,
            "Exclude or ShowOnly the named mesh from the render list.\n"
            "Usage: r_ExcludeShader Name\n"
            "Usage: r_ExcludeShader !Name\n"
            "Sometimes this is useful when debugging.");

    DefineConstIntCVar3("r_ProfileShaders", CV_r_profileshaders, 0, VF_CHEAT,
        "Enables display of render profiling information.\n"
        "Usage: r_ProfileShaders [0/1]\n"
        "Default is 0 (off). Set to 1 to display profiling\n"
        "of rendered shaders.");
    DefineConstIntCVar3("r_ProfileShadersSmooth", CV_r_ProfileShadersSmooth, 4, VF_CHEAT,
        "Smooth time information.\n"
        "Usage: r_ProfileShadersSmooth [0-10]");
    DefineConstIntCVar3("r_ProfileShadersGroupByName", CV_r_ProfileShadersGroupByName, 1, VF_CHEAT,
        "Group items by name ignoring RT flags.\n"
        "Usage: r_ProfileShaders [0/1]");

    /*  REGISTER_CVAR3("r_EnvCMWrite", CV_r_envcmwrite, 0, VF_NULL,
            "Writes cube-map textures to disk.\n"
            "Usage: r_EnvCMWrite [0/1]\n"
            "Default is 0 (off). The textures are written to 'Cube_posx.jpg'\n"
            "'Cube_negx.jpg',...,'Cube_negz.jpg'. At least one of the real-time\n"
            "cube-map shaders should be present in the current scene.");
            */
    REGISTER_CVAR3("r_EnvCMResolution", CV_r_envcmresolution, 1, VF_DUMPTODISK,
        "Sets resolution for target environment cubemap, in pixels.\n"
        "Usage: r_EnvCMResolution #\n"
        "where # represents:\n"
        "\t0: 64\n"
        "\t1: 128\n"
        "\t2: 256\n"
        "Default is 2 (256 by 256 pixels).");

    REGISTER_CVAR3("r_EnvTexResolution", CV_r_envtexresolution, ENVTEXRES_DEFAULT_VAL, VF_DUMPTODISK,
        "Sets resolution for 2d target environment texture, in pixels.\n"
        "Usage: r_EnvTexResolution #\n"
        "where # represents:\n"
        " 0: 64\n"
        " 1: 128\n"
        " 2: 256\n"
        " 3: 512\n"
        "Default is 3 (512 by 512 pixels).");

    REGISTER_CVAR3("r_WaterUpdateDistance", CV_r_waterupdateDistance, 2.0f, VF_NULL, "");

    REGISTER_CVAR3("r_WaterUpdateFactor", CV_r_waterupdateFactor, 0.01f, VF_DUMPTODISK | VF_CVARGRP_IGNOREINREALVAL, // Needs to be ignored from cvar group state otherwise some adv graphic options will show as custom when using multi GPU due to mgpu.cfg being reloaded after changing syspec and graphic options have changed,
        "Distance factor for water reflected texture updating.\n"
        "Usage: r_WaterUpdateFactor 0.01\n"
        "Default is 0.01. 0 means update every frame");

    REGISTER_CVAR3("r_EnvCMupdateInterval", CV_r_envcmupdateinterval, 0.04f, VF_DUMPTODISK,
        "Sets the interval between environmental cube map texture updates.\n"
        "Usage: r_EnvCMupdateInterval #\n"
        "Default is 0.1.");
    REGISTER_CVAR3("r_EnvTexUpdateInterval", CV_r_envtexupdateinterval, 0.001f, VF_DUMPTODISK,
        "Sets the interval between environmental 2d texture updates.\n"
        "Usage: r_EnvTexUpdateInterval 0.001\n"
        "Default is 0.001.");
    
    // Slimming of GBuffers by encoding full RGB channels into more efficient YCbCr channels which require
    // less storage for the CbCr channels (ie. 24(8+8+8) bits to 16(8+4+4) bits).
    // This allows the packing of different component channels into the G-Buffers, saving the cost of 3 extra channels.
    // 4 + 4 + 4 = 12 bytes of saving per pixel in the G-Buffer, assuming RGBA8 format
    // Slimmed down GBuffer encoding scheme:
    // Texture Channels:            R               G               B                   A   
    //
    // Normal Map Texture           Normal.x        Normal.y        Specular Y (YCrCb)  Smoothness (6bit) + Light (2bit)
    // Diffuse Texture              Albedo.x        Albedo.y        Albedo.z            Specular CrCb (4+4 bit)
    // Specular (One Channel Only)  Occlusion       N/A             N/A                 N/A
    REGISTER_CVAR3("r_SlimGBuffer", CV_r_SlimGBuffer,0, VF_REQUIRE_APP_RESTART,
        "Optimize the gbuffer render targets use.\n"
        "Usage:r_SlimGBuffer 1\n");
      
    DefineConstIntCVar3("r_WaterReflections", CV_r_waterreflections, 1, VF_DUMPTODISK,
        "Toggles water reflections.\n"
        "Usage: r_WaterReflections [0/1]\n"
        "Default is 1 (water reflects).");

    DefineConstIntCVar3("r_WaterReflectionsQuality", CV_r_waterreflections_quality, WATERREFLQUAL_DEFAULT_VAL, VF_DUMPTODISK,
        "Activates water reflections quality setting.\n"
        "Usage: r_WaterReflectionsQuality [0/1/2/3]\n"
        "Default is 0 (terrain only), 1 (terrain + particles), 2 (terrain + particles + brushes), 3 (everything)");

    REGISTER_CVAR3("r_WaterReflectionsMinVisiblePixelsUpdate", CV_r_waterreflections_min_visible_pixels_update, 0.05f, VF_DUMPTODISK,
        "Activates water reflections if visible pixels above a certain threshold.");

    REGISTER_CVAR3("r_WaterReflectionsMinVisUpdateFactorMul", CV_r_waterreflections_minvis_updatefactormul, 20.0f, VF_DUMPTODISK,
        "Activates update factor multiplier when water mostly occluded.");
    REGISTER_CVAR3("r_WaterReflectionsMinVisUpdateDistanceMul", CV_r_waterreflections_minvis_updatedistancemul, 10.0f, VF_DUMPTODISK,
        "Activates update distance multiplier when water mostly occluded.");

    REGISTER_CVAR3("r_WaterCaustics", CV_r_watercaustics, 1, VF_RENDERER_CVAR, // deprecating soon, moving to the Water Gem
        "Toggles under water caustics.\n"
        "Usage: r_WaterCaustics [0/1]\n"
        "Default is 1 (enabled).");

    REGISTER_CVAR3("r_WaterCausticsDistance", CV_r_watercausticsdistance, 100.0f, VF_NULL,
        "Toggles under water caustics max distance.\n"
        "Usage: r_WaterCausticsDistance\n"
        "Default is 100.0 meters");

    REGISTER_CVAR3("r_WaterVolumeCaustics", CV_r_watervolumecaustics, WATERVOLCAUSTICS_DEFAULT_VAL, VF_NULL,
        "Toggles advanced water caustics for watervolumes.\n"
        "Usage: r_WaterVolumeCaustics [0/1]\n"
        "Default is 0 (disabled). 1 - enables.");

    REGISTER_CVAR3("r_WaterVolumeCausticsDensity", CV_r_watervolumecausticsdensity, 128, VF_NULL,
        "Density/resolution of watervolume caustic grid.\n"
        "Usage: r_WaterVolumeCausticsDensity [16/256]\n"
        "Default is 256");

    REGISTER_CVAR3("r_WaterVolumeCausticsRes", CV_r_watervolumecausticsresolution, 512, VF_NULL,
        "Resolution of watervoluem caustics texture.\n"
        "Usage: r_WaterVolumeCausticsRes [n]\n"
        "Default is 1024");

    REGISTER_CVAR3("r_WaterVolumeCausticsSnapFactor", CV_r_watervolumecausticssnapfactor, 1.0f, VF_NULL,
        "Distance in which to snap the vertex grid/projection (to avoid aliasing).\n"
        "Usage: r_WaterVolumeCausticsSnapFactor [n]\n"
        "Default is 1.0");

    REGISTER_CVAR3("r_WaterVolumeCausticsMaxDist", CV_r_watervolumecausticsmaxdistance, 35.0f, VF_NULL,
        "Maximum distance in which caustics are visible.\n"
        "Usage: r_WaterVolumeCausticsMaxDist [n]\n"
        "Default is 35");

    if (!OceanToggle::IsActive())
    {
        DefineConstIntCVar3("r_WaterGodRays", CV_r_water_godrays, 1, VF_NULL,
            "Enables under water god rays.\n"
            "Usage: r_WaterGodRays [0/1]\n"
            "Default is 1 (enabled).");

        REGISTER_CVAR3("r_WaterGodRaysDistortion", CV_r_water_godrays_distortion, 1, VF_NULL,
            "Set the amount of distortion when underwater.\n"
            "Usage: r_WaterGodRaysDistortion [n]\n"
            "Default is 1.");
    }


    DefineConstIntCVar3("r_Reflections", CV_r_reflections, 1, VF_DUMPTODISK,
        "Toggles reflections.\n"
        "Usage: r_Reflections [0/1]\n"
        "Default is 1 (reflects).");

    REGISTER_CVAR3("r_ReflectionsOffset", CV_r_waterreflections_offset, 0.0f, VF_NULL, "");

    DefineConstIntCVar3("r_ReflectionsQuality", CV_r_reflections_quality, 3, VF_DUMPTODISK,
        "Toggles reflections quality.\n"
        "Usage: r_ReflectionsQuality [0/1/2/3]\n"
        "Default is 0 (terrain only), 1 (terrain + particles), 2 (terrain + particles + brushes), 3 (everything)");

    DefineConstIntCVar3("r_DetailTextures", CV_r_detailtextures, 1, VF_DUMPTODISK,
        "Toggles detail texture overlays.\n"
        "Usage: r_DetailTextures [0/1]\n"
        "Default is 1 (detail textures on).");

    DefineConstIntCVar3("r_ReloadShaders", CV_r_reloadshaders, 0, VF_CHEAT,
        "Reloads shaders.\n"
        "Usage: r_ReloadShaders [0/1]\n"
        "Default is 0. Set to 1 to reload shaders.");

    REGISTER_CVAR3("r_DetailDistance", CV_r_detaildistance, 6.0f, VF_DUMPTODISK,
        "Distance used for per-pixel detail layers blending.\n"
        "Usage: r_DetailDistance (1-20)\n"
        "Default is 6.");

    DefineConstIntCVar3("r_TexBindMode", CV_r_texbindmode, 0, VF_CHEAT,
        "Enable texture overrides.\n"
        "Usage: r_TexBindMode [0/1/2/4/5/6/7/8/9/10/11]\n"
        "\t1 - Force gray non-font maps\n"
        "\t5 - Force flat normal maps\n"
        "\t6 - Force white diffuse maps\n"
        "\t7 - Force diffuse maps to use mipmapdebug texture\n"
        "\t8 - Colour code diffuse maps to show minimum uploaded mip [0:green,1:cyan,2:blue,3:purple,4:magenta,5:yellow,6:orange,7:red,higher:white]\n"
        "\t9 - Colour code diffuse maps to show textures streaming in in green and out in red\n"
        "\t10 - Colour code diffuse maps that have requested a lower mip than the lowest available [-3: red, -2: yellow, -1: green]\n"
        "\t11 - Force white diffuse map and flat normal map\n"
        "\t12 - Visualise textures that have more or less mips in memory than needed\n"
        "Default is 0 (disabled).");
    DefineConstIntCVar3("r_NoDrawShaders", CV_r_nodrawshaders, 0, VF_CHEAT,
        "Disable entire render pipeline.\n"
        "Usage: r_NoDrawShaders [0/1]\n"
        "Default is 0 (render pipeline enabled). Used for debugging and profiling.");
    REGISTER_CVAR3("r_DrawNearShadows", CV_r_DrawNearShadows, 0, VF_NULL,
        "Enable shadows for near objects.\n"
        "Usage: r_DrawNearShadows [0/1]\n");
    REGISTER_CVAR3("r_NoDrawNear", CV_r_nodrawnear, 0, VF_RENDERER_CVAR,
        "Disable drawing of near objects.\n"
        "Usage: r_NoDrawNear [0/1]\n"
        "Default is 0 (near objects are drawn).");
    REGISTER_CVAR3("r_DrawNearZRange", CV_r_DrawNearZRange, 0.12f, VF_NULL,
        "Default is 0.1.");
    REGISTER_CVAR3("r_DrawNearFarPlane", CV_r_DrawNearFarPlane, 40.0f, VF_NULL,
        "Default is 40.");
    REGISTER_CVAR3("r_DrawNearFoV", CV_r_drawnearfov, 60.0f, VF_NULL,
        "Sets the FoV for drawing of near objects.\n"
        "Usage: r_DrawNearFoV [n]\n"
        "Default is 60.");

    REGISTER_CVAR3_CB("r_Flares", CV_r_flares, FLARES_DEFAULT_VAL, VF_DUMPTODISK,
        "Toggles lens flare effect.\n"
        "Usage: r_Flares [0/1]\n"
        "Default is 1 (on).",
        OnChange_CV_r_flares);

    DefineConstIntCVar3("r_FlareHqShafts", CV_r_flareHqShafts, FLARES_HQSHAFTS_DEFAULT_VAL, VF_DUMPTODISK,
        "Toggles high quality mode for point light shafts.\n"
        "Usage: r_FlareHqShafts [0/1]\n"
        "Default is 1 (on).");

    REGISTER_CVAR3("r_FlaresChromaShift", CV_r_FlaresChromaShift, 6.0f, VF_NULL,
        "Set flares chroma shift amount.\n"
        "Usage: r_FlaresChromaShift [n]\n"
        "Default is 6\n"
        "0 Disables");

    REGISTER_CVAR3("r_FlaresIrisShaftMaxPolyNum", CV_r_FlaresIrisShaftMaxPolyNum, 200, VF_NULL,
        "Set the maximum number of polygon of IrisShaft.\n"
        "Usage : r_FlaresIrisShaftMaxPolyNum [n]\n"
        "Default is 200\n"
        "0 Infinite");

    REGISTER_CVAR3_CB("r_FlaresTessellationRatio", CV_r_FlaresTessellationRatio, 1, VF_NULL,
        "Set the tessellation rate of flares. 1 is the original mesh.\n"
        "Usage : r_FlaresTessellationRatio 0.5\n"
        "Default is 1.0\n"
        "Range is from 0 to 1", OnChange_CV_r_FlaresTessellationRatio);

    REGISTER_CVAR3("r_Gamma", CV_r_gamma, 1.0f, VF_DUMPTODISK,
        "Adjusts the graphics card gamma correction (fast, needs hardware support, also affects HUD and desktop)\n"
        "r_NoHWGamma must be set to 0 for this to have an effect.\n"
        "Usage: r_Gamma 1.0\n"
        "1 off (default)");
    REGISTER_CVAR3("r_Brightness", CV_r_brightness, 0.5f, VF_DUMPTODISK,
        "Sets the display brightness (fast, needs hardware support, also affects HUD and desktop)\n"
        "r_NoHWGamma must be set to 0 for this to have an effect.\n"
        "Usage: r_Brightness 0.5\n"
        "Default is 0.5.");
    REGISTER_CVAR3("r_Contrast", CV_r_contrast, 0.5f, VF_DUMPTODISK,
        "Sets the display contrast (fast, needs hardware support, also affects HUD and desktop)\n"
        "r_NoHWGamma must be set to 0 for this to have an effect.\n"
        "Usage: r_Contrast 0.5\n"
        "Default is 0.5.");

    DefineConstIntCVar3("r_NoHWGamma", CV_r_nohwgamma, 2, VF_DUMPTODISK,
        "Sets renderer to ignore hardware gamma correction.\n"
        "Usage: r_NoHWGamma [0/1/2]\n"
        "0 - allow hardware gamma correction\n"
        "1 - disable hardware gamma correction\n"
        "2 - disable hardware gamma correction in Editor\n");

    REGISTER_CVAR3("r_Scissor", CV_r_scissor, 1, VF_RENDERER_CVAR, "Enables scissor test");

    DefineConstIntCVar3("r_wireframe", CV_r_wireframe, R_SOLID_MODE, VF_CHEAT, "Toggles wireframe rendering mode");

    REGISTER_CVAR3_CB("r_GetScreenShot", CV_r_GetScreenShot, 0, VF_NULL,
        AZStd::string::format
        (
            "To capture one screenshot (variable is set to 0 after capturing)\n"
            "%d = do not take a screenshot (default)\n"
            "%d = take a screenshot and another HDR screenshot if HDR is enabled\n"
            "%d = take a screenshot\n",
            static_cast<int>(ScreenshotType::None),
            static_cast<int>(ScreenshotType::HdrAndNormal),
            static_cast<int>(ScreenshotType::Normal)
        ).c_str(),
        []([[maybe_unused]] ICVar* pArgs)
        {
            // Do not accept other values, since ScreenshotType::NormalWithFilepath = 3 is reserved for internal use.
            if (CV_r_GetScreenShot != static_cast<int>(ScreenshotType::None) &&
                CV_r_GetScreenShot != static_cast<int>(ScreenshotType::HdrAndNormal) &&
                CV_r_GetScreenShot != static_cast<int>(ScreenshotType::Normal))
            {
                CV_r_GetScreenShot = static_cast<int>(ScreenshotType::None);
                iLog->LogWarning("Screenshot type not supported!");
            }
        }
    );

    DefineConstIntCVar3("r_Character_NoDeform", CV_r_character_nodeform, 0, VF_NULL, "");

    REGISTER_CVAR3("r_Log", CV_r_log, 0, VF_CHEAT,
        "Logs rendering information to Direct3DLog.txt.\n"
        "Use negative values to log a single frame.\n"
        "Usage: r_Log +/-[0/1/2/3/4]\n"
        "\t1: Logs a list of all shaders without profile info.\n"
        "\t2: Log contains a list of all shaders with profile info.\n"
        "\t3: Logs all API function calls.\n"
        "\t4: Highly detailed pipeline log, including all passes,\n"
        "\t\t\tstates, lights and pixel/vertex shaders.\n"
        "Default is 0 (off). Use this function carefully, because\n"
        "log files grow very quickly.");

    DefineConstIntCVar3("r_LogVidMem", CV_r_logVidMem, 0, VF_CHEAT,
        "Logs vid mem information to VidMemLog.txt.");

    DefineConstIntCVar3("r_Stats", CV_r_stats, 0, VF_CHEAT,
        "Toggles render statistics.\n"
        "0=disabled,\n"
        "1=global render stats,\n"
        "2=print shaders for selected object,\n"
        "3=CPU times of render passes and video memory usage,\n"
        "4=CPU times of render passes,\n"
        "5=Occlusion query calls (calls to mfDraw/mfReadResult_Now/mfReadResult_Try),\n"
        "6=display per-instance drawcall count,\n"
        "8=Info about instanced DIPs,\n"
        "13=print info about cleared RT's,\n"
        "Usage: r_Stats [0/1/n]");

    DefineConstIntCVar3("r_statsMinDrawCalls", CV_r_statsMinDrawcalls, 0, VF_CHEAT,
        "Minimum drawcall amount to display for use with r_Stats 6");

    DefineConstIntCVar3("r_profiler", CV_r_profiler, 0, VF_NULL,
        "Display render pipeline profiler.\n"
        "  0: Disabled\n"
        "  1: Basic overview\n"
        "  2: Detailed pass stats\n");

    REGISTER_CVAR3("r_profilerTargetFPS", CV_r_profilerTargetFPS, 30.0f, VF_NULL,
        "Target framerate for application.");

    REGISTER_CVAR3("r_VSync", CV_r_vsync, 1, VF_RESTRICTEDMODE | VF_DUMPTODISK,
        "Toggles vertical sync.\n"
        "0: Disabled\n"
        "1: Enabled\n"
        "2: Enabled, use asynchronous swaps on deprecated platform");

    REGISTER_CVAR3("r_OldBackendSkip", CV_r_OldBackendSkip, 0, VF_RESTRICTEDMODE | VF_DUMPTODISK,
        "Ignores old backend processing.\n"
        "0: Old backend is on\n"
        "1: Old backend is skipped\n"
        "2: Old backend shadows are skipped\n");

#if defined(SUPPORT_DEVICE_INFO_USER_DISPLAY_OVERRIDES)
    REGISTER_CVAR3("r_overrideRefreshRate", CV_r_overrideRefreshRate, 0, VF_RESTRICTEDMODE | VF_DUMPTODISK,
        "Enforces specified refresh rate when running in fullscreen (0=off).");
    REGISTER_CVAR3("r_overrideScanlineOrder", CV_r_overrideScanlineOrder, 0, VF_RESTRICTEDMODE | VF_DUMPTODISK,
        "Enforces specified scanline order when running in fullscreen.\n"
        "0=off,\n"
        "1=progressive,\n"
        "2=interlaced (upper field first),\n"
        "3=interlaced (lower field first)\n"
        "Usage: r_overrideScanlineOrder [0/1/2/3]");
    REGISTER_CVAR3("r_overrideDXGIOutput", CV_r_overrideDXGIOutput, 0, VF_REQUIRE_APP_RESTART,
        "Specifies index of display to use for output (0=primary display).");
    REGISTER_CVAR3("r_overrideDXGIOutputFS", CV_r_overrideDXGIOutputFS, 0, VF_NULL,
        "Specifies index of display to use for full screen output (0=primary display).");
#endif
#if defined(WIN32) || defined(WIN64)
    REGISTER_CVAR3("r_FullscreenPreemption", CV_r_FullscreenPreemption, 1, VF_NULL,
        "While in fullscreen activities like notification pop ups of other applications won't cause a mode switch back into windowed mode.");
#endif
    DefineConstIntCVar3("r_PredicatedTiling", CV_r_predicatedtiling, 0, VF_REQUIRE_APP_RESTART,
        "Toggles predicated tiling mode (deprecated platform only)\n"
        "Usage: r_PredicatedTiling [0/1]");
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_CPP_SECTION_5
    #include AZ_RESTRICTED_FILE(Renderer_cpp)
#endif
    DefineConstIntCVar3("r_MeasureOverdraw", CV_r_measureoverdraw, 0, VF_CHEAT,
        "Activate a special rendering mode that visualize the rendering cost of each pixel by color.\n"
        "0=off,\n"
        "1=pixel shader instructions,\n"
        "2=pass count,\n"
        "3=vertex shader instructions,\n"
        "4=overdraw estimation with Hi-Z (deprecated),\n"
        "Usage: r_MeasureOverdraw [0/1/2/3/4]");
    REGISTER_CVAR3("r_MeasureOverdrawScale", CV_r_measureoverdrawscale, 1.5f, VF_CHEAT, "");

    DefineConstIntCVar3("r_PrintMemoryLeaks", CV_r_printmemoryleaks, 0, VF_NULL, "");
    DefineConstIntCVar3("r_ReleaseAllResourcesOnExit", CV_r_releaseallresourcesonexit, 1, VF_NULL, "");

    REGISTER_CVAR3("r_ShowVideoMemoryStats", CV_r_ShowVideoMemoryStats, 0, VF_NULL, "");
    REGISTER_COMMAND("r_ShowRenderTarget", &Cmd_ShowRenderTarget, VF_CHEAT, showRenderTargetHelp);

    REGISTER_CVAR3("r_VRAMDebug", CV_r_VRAMDebug, 0, VF_NULL,
        "Display debug information for VRAM heaps on platforms where we have direct access to video memory\n"
        "\t0: Disabled\n"
        "\t1: VRAM heap statistics and occupancy visualization enabled");

    REGISTER_CVAR3("r_BreakOnError", CV_r_BreakOnError, 0, VF_NULL, "calls debugbreak on illegal behaviour");
    REGISTER_CVAR3("r_D3D12SubmissionThread", CV_r_D3D12SubmissionThread, 1, VF_NULL, "run DX12 command queue submission tasks from a dedicated thread");

    REGISTER_CVAR3("r_ReprojectOnlyStaticObjects", CV_r_ReprojectOnlyStaticObjects, 1, VF_NULL, "Forces a split in the zpass, to prevent moving object from beeing reprojected");
    REGISTER_CVAR3("r_ReverseDepth", CV_r_ReverseDepth, 1, VF_NULL, "Use 1-z depth rendering for increased depth precision");

    REGISTER_CVAR3("r_EnableDebugLayer", CV_r_EnableDebugLayer, 0, VF_NULL, "DX12: Enable Debug Layer");
    REGISTER_CVAR3("r_NoDraw", CV_r_NoDraw, 0, VF_NULL, "Disable submitting of certain draw operations: 1-(Do not process render objects at all), 2-(Do not submit individual render objects), 3-(No DrawIndexed)");

    // show texture debug routine + auto completion
    CV_r_ShowTexture = REGISTER_STRING("r_ShowTexture", "", VF_CHEAT, "Displays loaded texture - for debug purpose\n");
    gEnv->pConsole->RegisterAutoComplete("r_ShowTexture", &g_TextureNameAutoComplete);

    DefineConstIntCVar3("r_ShowLightBounds", CV_r_ShowLightBounds, 0, VF_CHEAT,
        "Display light bounds - for debug purpose\n"
        "Usage: r_ShowLightBounds [0=off/1=on]");
    DefineConstIntCVar3("r_MergeRenderChunks", CV_r_MergeRenderChunks, 1, VF_NULL, "");

    REGISTER_CVAR3("r_ParticlesTessellation", CV_r_ParticlesTessellation, 1, VF_NULL, "Enables particle tessellation for higher quality lighting. (DX11 only)");
    REGISTER_CVAR3("r_ParticlesTessellationTriSize", CV_r_ParticlesTessellationTriSize, 16, VF_NULL, "Sets particles tessellation triangle screen space size in pixels (DX11 only)");

    REGISTER_CVAR3("r_ZFightingDepthScale", CV_r_ZFightingDepthScale, 0.995f, VF_CHEAT, "Controls anti z-fighting measures in shaders (scaling homogeneous z).");
    REGISTER_CVAR3("r_ZFightingExtrude", CV_r_ZFightingExtrude, 0.001f, VF_CHEAT, "Controls anti z-fighting measures in shaders (extrusion along normal in world units).");

    REGISTER_CVAR3_CB("r_TexelsPerMeter", CV_r_TexelsPerMeter, 0, VF_ALWAYSONCHANGE,
        "Enables visualization of the color coded \"texels per meter\" ratio for objects in view.\n"
        "The checkerboard pattern displayed represents the mapping of the assigned diffuse\n"
        "texture onto the object's uv space. One block in the pattern represents 8x8 texels.\n"
        "Usage: r_TexelsPerMeter [n] (where n is the desired number of texels per meter; 0 = off)",
        OnChange_CV_r_TexelsPerMeter);

    REGISTER_CVAR3("r_enableAltTab", CV_r_enableAltTab, 1, VF_NULL,
        "Toggles alt tabbing in and out of fullscreen when the game is not in devmode.\n"
        "Usage: r_enableAltTab [toggle]\n"
        "Notes: Should only be added to system.cfg and requires a restart");

    REGISTER_CVAR3("r_StereoDevice", CV_r_StereoDevice, 0, VF_REQUIRE_APP_RESTART | VF_DUMPTODISK,
        "Sets stereo device (only possible before app start)\n"
        "Usage: r_StereoDevice [0/1/2/3/4]\n"
        "0: No stereo support (default)\n"
        "1: Frame compatible formats (side-by-side, interlaced, anaglyph)\n"
        "2: HDMI 1.4\n"
        "3: Stereo driver (PC only, NVidia or AMD)\n"
        "4: Dualhead (PC only, two projectors or iZ3D screen)\n"
        "100: Auto-detect device for platform");

    REGISTER_CVAR3("r_StereoMode", CV_r_StereoMode, 0, VF_DUMPTODISK,
        "Sets stereo rendering mode.\n"
        "Usage: r_StereoMode [0=off/1]\n"
        "1: Dual rendering\n");

    REGISTER_CVAR3("r_StereoOutput", CV_r_StereoOutput, 0, VF_DUMPTODISK,
        "Sets stereo output. Output depends on the stereo monitor\n"
        "Usage: r_StereoOutput [0=off/1/2/3/4/5/6/...]\n"
        "0: Standard\n"
        "1: IZ3D\n"
        "2: Checkerboard\n"
        "3: Above and Below (not supported)\n"
        "4: Side by Side\n"
        "5: Line by Line (Interlaced)\n"
        "6: Anaglyph\n"
        "7: VR Device (Oculus/Vive)\n"
        );

    REGISTER_CVAR3("r_StereoFlipEyes", CV_r_StereoFlipEyes, 0, VF_DUMPTODISK,
        "Flip eyes in stereo mode.\n"
        "Usage: r_StereoFlipEyes [0=off/1=on]\n"
        "0: don't flip\n"
        "1: flip\n");

    REGISTER_CVAR3("r_StereoStrength", CV_r_StereoStrength, 1.0f, VF_DUMPTODISK,
        "Multiplier which influences the strength of the stereo effect.");

    REGISTER_CVAR3("r_StereoEyeDist", CV_r_StereoEyeDist, 0.02f, VF_DUMPTODISK,
        "[For 3D TV] Maximum separation between stereo images in percentage of the screen.");

    REGISTER_CVAR3("r_StereoScreenDist", CV_r_StereoScreenDist, 0.25f, VF_DUMPTODISK,
        "Distance to plane where stereo parallax converges to zero.");

    REGISTER_CVAR3("r_StereoNearGeoScale", CV_r_StereoNearGeoScale, 0.65f, VF_DUMPTODISK,
        "Scale for near geometry (weapon) that gets pushed into the screen");

    REGISTER_CVAR3("r_StereoHudScreenDist", CV_r_StereoHudScreenDist, 0.5f, VF_DUMPTODISK,
        "Distance to plane where hud stereo parallax converges to zero.\n"
        "If not zero, HUD needs to be rendered two times.");

    REGISTER_CVAR3("r_StereoGammaAdjustment", CV_r_StereoGammaAdjustment, 0.12f, VF_DUMPTODISK,
        "Additional adjustment to the graphics card gamma correction when Stereo is enabled.\n"
        "Usage: r_StereoGammaAdjustment [offset]"
        "0: off");

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_CPP_SECTION_7
    #include AZ_RESTRICTED_FILE(Renderer_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    const int DEVICE_WIDTH  =   1152;
    const int   DEVICE_HEIGHT   =   720;
#endif

    REGISTER_CVAR3("r_ConsoleBackbufferWidth", CV_r_ConsoleBackbufferWidth, DEVICE_WIDTH, VF_DUMPTODISK,
        "console specific backbuffer resolution - width");
    REGISTER_CVAR3("r_ConsoleBackbufferHeight", CV_r_ConsoleBackbufferHeight, DEVICE_HEIGHT, VF_DUMPTODISK,
        "console specific backbuffer resolution - height");

    REGISTER_CVAR3("r_ConditionalRendering", CV_r_ConditionalRendering, 0, VF_NULL, "Enables conditional rendering .");

    REGISTER_CVAR3("r_CustomResMaxSize", CV_r_CustomResMaxSize, s_CustomResMaxSize_USE_MAX_RESOURCES, VF_NULL, "Maximum resolution of custom resolution rendering. A value of -1 sets the maximum to the upper limit of the device.");
    REGISTER_CVAR3("r_CustomResWidth", CV_r_CustomResWidth, 0, VF_NULL, "Width of custom resolution rendering");
    REGISTER_CVAR3("r_CustomResHeight", CV_r_CustomResHeight, 0, VF_NULL, "Height of custom resolution rendering");
    REGISTER_CVAR3("r_CustomResPreview", CV_r_CustomResPreview, 1, VF_NULL, "Enable/disable preview of custom resolution rendering in viewport"
        "(0 - no preview, 1 - scaled to match viewport, 2 - custom resolution clipped to viewport");
    REGISTER_CVAR3("r_Supersampling", CV_r_Supersampling, 1, VF_NULL, "Use supersampled antialiasing"
        "(1 - 1x1 no SSAA, 2 - 2x2, 3 - 3x3 ...)");
    REGISTER_CVAR3("r_SupersamplingFilter", CV_r_SupersamplingFilter, 0, VF_NULL, "Filter method to use when resolving supersampled output"
        "\n0 - Box filter"
        "\n1 - Tent filter"
        "\n2 - Gaussian filter"
        "\n3 - Lanczos filter");

#if !defined(CONSOLE) && !defined(NULL_RENDERER)
    REGISTER_COMMAND("r_PrecacheShaderList", &ShadersPrecacheList, VF_NULL, "");
    REGISTER_COMMAND("r_StatsShaderList", &ShadersStatsList, VF_NULL, "");
    REGISTER_COMMAND("r_OptimiseShaders", &ShadersOptimise, VF_NULL, "");
    REGISTER_COMMAND("r_GetShaderList", &GetShaderList, VF_NULL, "");
#endif

    DefineConstIntCVar3("r_TextureCompressor", CV_r_TextureCompressor, 1, VF_DUMPTODISK,
        "Defines which texture compressor is used (fallback is DirectX)\n"
        "Usage: r_TextureCompressor [0/1]\n"
        "0 uses DirectX, 1 uses squish if possible");

    REGISTER_CVAR3("r_FogDepthTest", CV_r_FogDepthTest, -0.0005f, VF_NULL,
        "Enables per-pixel culling for deferred volumetric fog pass.\n"
        "Fog computations for all pixels closer than a given depth value will be skipped.\n"
        "Usage: r_FogDepthTest z with...\n"
        "  z = 0, culling disabled\n"
        "  z > 0, fixed linear world space culling depth\n"
        "  z < 0, optimal culling depth will be computed automatically based on camera direction and fog settings");

#if defined(VOLUMETRIC_FOG_SHADOWS)
    REGISTER_CVAR3("r_FogShadows", CV_r_FogShadows, 0, VF_NULL,
        "Enables deferred volumetric fog shadows\n"
        "Usage: r_FogShadows [0/1/2]\n"
        "  0: off\n"
        "  1: standard resolution\n"
        "  2: reduced resolution\n");
    REGISTER_CVAR3("r_FogShadowsMode", CV_r_FogShadowsMode, 0, VF_NULL,
        "Ray-casting mode for shadowed fog\n"
        "Usage: r_FogShadowsMode [0/1]\n"
        "  0: brute force shadowmap sampling\n"
        "  1: optimized shadowmap sampling\n");
#endif
    REGISTER_CVAR3("r_FogShadowsWater", CV_r_FogShadowsWater, 1, VF_NULL, "Enables volumetric fog shadows for watervolumes");

    DefineConstIntCVar3("r_RainDropsEffect", CV_r_RainDropsEffect, 1, VF_CHEAT,
        "Enable RainDrops effect.\n"
        "Usage: r_RainDropEffect [0/1/2]\n"
        "0: force off\n"
        "1: on (default)\n"
        "2: on (forced)");

    DefineConstIntCVar3("r_RefractionPartialResolves", CV_r_RefractionPartialResolves, 2, VF_NULL,
        "Do a partial screen resolve before refraction\n"
        "Usage: r_RefractionPartialResolves [0/1]\n"
        "0: disable \n"
        "1: enable conservatively (non-optimal)\n"
        "2: enable (default)");

    DefineConstIntCVar3("r_RefractionPartialResolvesDebug", CV_r_RefractionPartialResolvesDebug, 0, VF_NULL,
        "Toggle refraction partial resolves debug display\n"
        "Usage: r_RefractionPartialResolvesDebug [0/1]\n"
        "0: disable \n"
        "1: Additive 2d area \n"
        "2: Bounding boxes \n"
        "3: Alpha overlay with varying colours \n");

    DefineConstIntCVar3("r_Batching", CV_r_Batching, 1, VF_NULL,
        "Enable/disable render items batching\n"
        "Usage: r_Batching [0/1]\n");

    DefineConstIntCVar3("r_Unlit", CV_r_Unlit, 0, VF_CHEAT,
        "Render just diffuse texture with no lighting (for most materials).");

    DefineConstIntCVar3("r_HideSunInCubemaps", CV_r_HideSunInCubemaps, 1, VF_NULL,
        "Stops the sun being drawn during cubemap generation.\n");

    // more details: http://en.wikipedia.org/wiki/Overscan
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_CPP_SECTION_8
    #include AZ_RESTRICTED_FILE(Renderer_cpp)
#endif
    REGISTER_COMMAND("r_OverscanBorders", &cmd_OverscanBorders, VF_NULL,
        "Changes the size of the overscan borders for the left/right and top/bottom\n"
        "of the screen for adjusting the title safe area. This is for logo placements\n"
        "and text printout to account for the TV overscan and is mostly needed for consoles.\n"
        "If only one value is specified, the overscan borders for left/right and top/bottom\n"
        "are set simultaneously, but you may also specify different percentages for left/right\n"
        "and top/bottom.\n"
        "Usage: r_OverscanBorders [0..25]\n"
        "       r_OverscanBorders [0..25] [0..25]\n"
        "Default is 0=off, >0 defines the size of the overscan borders for left/right\n"
        "or top/bottom as percentages of the whole screen size (e.g. 7.5).");

#if defined(WIN32) || defined(WIN64) || defined(APPLE) || defined(LINUX)
    float overscanBorderScale = 0.0f;
#else
    float overscanBorderScale = 0.03f; // Default consoles to sensible overscan border
#endif
    REGISTER_CVAR3_CB("r_OverscanBorderScaleX", s_overscanBorders.x, overscanBorderScale, VF_NULL,
        "Sets the overscan border width scale\n"
        "Usage: r_OverscanBorderScaleX [0.0->0.25]",
        OnChange_r_OverscanBorderScale);

    REGISTER_CVAR3_CB("r_OverscanBorderScaleY", s_overscanBorders.y, overscanBorderScale, VF_NULL,
        "Sets the overscan border height scale\n"
        "Usage: r_OverscanBorderScaleY [0.0->0.25]",
        OnChange_r_OverscanBorderScale);

    REGISTER_CVAR2("r_UsePersistentRTForModelHUD", &CV_r_UsePersistentRTForModelHUD, 0, VF_NULL,
        "Uses a seperate RT to render models for the ModelHud Renderer");

#if defined(ENABLE_RENDER_AUX_GEOM)
    const int defValAuxGeomEnable = 1;
    REGISTER_CVAR2("r_enableAuxGeom", &CV_r_enableauxgeom, defValAuxGeomEnable, VF_REQUIRE_APP_RESTART, "Enables aux geometry rendering.");
#endif

    REGISTER_CVAR2("r_ParticleVerticePoolSize", &CV_r_ParticleVerticePoolSize, 15360, VF_REQUIRE_APP_RESTART, "Max Number of Particle Vertices to support");

    DefineConstIntCVar3("r_ParticlesDebug", CV_r_ParticlesDebug, 0, VF_NULL,
        "Particles debugging\n"
        "Usage: \n"
        "0 disabled\n"
        "1 particles screen coverage (red = bad, blue = good)\n"
        "2 particles overdraw (white = really bad, red = bad, blue = good)");

    REGISTER_CVAR3("r_GeomCacheInstanceThreshold", CV_r_GeomCacheInstanceThreshold, 10, VF_NULL, "Threshold after which instancing is used to draw geometry cache pieces");

    REGISTER_CVAR3("r_VisAreaClipLightsPerPixel", CV_r_VisAreaClipLightsPerPixel, 1, VF_NULL, "Per pixel light/cubemap culling for vis areas: 0=off, 1=on");
    REGISTER_CVAR3("r_OutputShaderSourceFiles", CV_r_OutputShaderSourceFiles, 0, VF_NULL, "If true, HLSL and GLSL files will be saved in the USER\\Shader\\* folders during shader compilation.  Does not work on console or mobile targets.");

    REGISTER_CVAR3("r_VolumetricFogTexScale", CV_r_VolumetricFogTexScale, 10, VF_NULL,
        "Width and height scale factor (divided by screen resolution) for volume texture.\n"
        "Acceptable value is more than equal 2.\n"
        );
    REGISTER_CVAR3("r_VolumetricFogTexDepth", CV_r_VolumetricFogTexDepth, 32, VF_NULL,
        "Depth resolution of volume texture.\n"
        "Huge value runs out of performance and video memory.\n"
        );
    REGISTER_CVAR3("r_VolumetricFogReprojectionBlendFactor", CV_r_VolumetricFogReprojectionBlendFactor, 0.9f, VF_NULL,
        "Adjust the blend factor of temporal reprojection.\n"
        "Acceptable value is between 0 and 1.\n"
        "0 means temporal reprojecton is off.\n"
        );
    REGISTER_CVAR3("r_VolumetricFogSample", CV_r_VolumetricFogSample, 0, VF_NULL,
        "Adjust number of sample points.\n"
        "0: 1 sample point in a voxel\n"
        "1: 2 sample points in a voxel\n"
        "2: 4 sample points in a voxel\n"
        );
    REGISTER_CVAR3("r_VolumetricFogShadow", CV_r_VolumetricFogShadow, 1, VF_NULL,
        "Adjust shadow sample count per sample point.\n"
        "0: 1 shadow sample per sample point\n"
        "1: 2 shadow samples per sample point \n"
        "2: 3 shadow samples per sample point\n"
        "3: 4 shadow samples per sample point\n"
        );

    // Confetti David Srour: Upscaling Quality for Metal
    DefineConstIntCVar3("r_UpscalingQuality", CV_r_UpscalingQuality, 0, VF_NULL,
        "iOS Metal Upscaling Quality\n"
        "Usage: \n"
        "0 Point\n"
        "1 Bilinear\n"
        "2 Bicubic\n"
        "3 Lanczos\n");

    //Clears GMEM G-Buffer through a shader. Has support for fixed point
    DefineConstIntCVar3("r_ClearGMEMGBuffer", CV_r_ClearGMEMGBuffer, 0, VF_NULL,
        "GMEM G-Buffer Clear\n"
        "Usage: \n"
        "0 no clearing\n"
        "1 full screen clear pass before Z-Pass. Done through a shader and supports fixed point\n"
        "2 full screen clear pass before Z-Pass. Done through loadactions (faster)\n");
    
    // Enables fast math for metal shaders
    DefineConstIntCVar3("r_MetalShadersFastMath", CV_r_MetalShadersFastMath, 1, VF_NULL,
        "Metal shaders fast math. Default is 1.\n"
        "Usage: \n"
        "0 Dont use fast math\n"
        "1 Use fast math\n");

    
    // Confetti David Srour: GMEM paths
    REGISTER_CVAR3("r_EnableGMEMPath", CV_r_EnableGMEMPath, 0, VF_REQUIRE_APP_RESTART,
        "Mobile GMEM Paths\n"
        "Usage: \n"
        "0 Standard Rendering\n"
        "1 256bpp GMEM Path\n"
        "2 128bpp GMEM Path\n");

    REGISTER_CVAR3("r_GMEM_DOF_Gather1_Quality", CV_r_GMEM_DOF_Gather1_Quality, 3, VF_REQUIRE_APP_RESTART,
        "Value represents # of taps squared for 1st gather pass.\n"
        "Usage: \n"
        "Clamped between 1 & 7 (default is 3)\n");

    REGISTER_CVAR3("r_GMEM_DOF_Gather2_Quality", CV_r_GMEM_DOF_Gather2_Quality, 2, VF_REQUIRE_APP_RESTART,
        "Value represents # of taps squared for second gather pass.\n"
        "Usage: \n"
        "Clamped between 1 & 7 (default is 2)\n");

    REGISTER_CVAR3("r_EnableGMEMPostProcCS", CV_r_EnableGMEMPostProcCS, 0, VF_REQUIRE_APP_RESTART,
        "GMEM Compute Postprocess Pipeline\n"
        "Usage: \n"
        "0 Compute disabled with postprocessing on GMEM path\n"
        "1 Compute enabled with postprocessing on GMEM path\n");

    REGISTER_CVAR3("r_RainUseStencilMasking", CV_r_RainUseStencilMasking, 0, VF_REQUIRE_APP_RESTART,
        "GMEM Deferred Rain enable stencil masking\n"
        "Usage: \n"
        "0 Use single pass rain on GMEM path\n"
        "1 Generate stencil mask for rain on GMEM path\n");

    // Confetti David Srour: Global VisArea/Portals blend weight for GMEM path
    REGISTER_CVAR3("r_GMEMVisAreasBlendWeight", CV_r_GMEMVisAreasBlendWeight, 1, VF_NULL,
        "Global VisArea/Portal Blend Weight for GMEM Render Path\n"
        "GMEM render path doesn't support per-portal blend weight.\n"
        "0.f to 1.f weight\n");

    REGISTER_CVAR3("r_ForceFixedPointRenderTargets", CV_r_ForceFixedPointRenderTargets, 0, VF_NULL,
        "Forces the engine to use fixed point render targets instead of floating point ones.\n"
        "This variable is respected on Android OpenGL ES only\n"
        "0 Off\n"
        "1 ON\n");

    REGISTER_CVAR3_CB("r_Fur", CV_r_Fur, 1, VF_NULL,
        "Specifies how fur is rendered:\n"
        "0: Fur is disabled - objects using Fur shader appear similar to Illum\n"
        "1: Alpha blended transparent passes\n"
        "2: Alpha tested opaque passes",
    OnChange_CV_r_Fur);

    REGISTER_CVAR3("r_FurShellPassCount", CV_r_FurShellPassCount, 64, VF_NULL,
        "Number of passes to perform for rendering fur shells");

    REGISTER_CVAR3("r_FurShowBending", CV_r_FurShowBending, 0, VF_DEV_ONLY,
        "Toggles visibility of fur bending vectors.");

    REGISTER_CVAR3("r_FurDebug", CV_r_FurDebug, 0, VF_DEV_ONLY,
        "Debug visualizers for fur.\n"
        "0: off\n"
        "1: base/tip sample validity (red = base valid; green = tip valid; yellow = both valid)\n"
        "2: base/tip selection (red = base chosen; green = tip chosen)\n"
        "3: show offscreen UVs for base deferred sample (gray = onscreen)\n"
        "4: show offscreen UVs for tip deferred sample (gray = onscreen)\n"
        "5: show final lighting with all base lighting selected\n"
        "6: show final lighting with all tip lighting selected\n"
        "7: visualize fur length scaling\n"
        "8: visualize fur animation bending velocity");

    REGISTER_CVAR3("r_FurDebugOneShell", CV_r_FurDebugOneShell, 0, VF_DEV_ONLY,
        "Debug cvar to draw only the specified shell number for fur. 0 = disabled.");

    REGISTER_CVAR3("r_FurFinPass", CV_r_FurFinPass, 0, VF_NULL,
        "Toggles view orthogonal fin pass for fur. 0 = disabled.");

    REGISTER_CVAR3("r_FurFinShadowPass", CV_r_FurFinShadowPass, 1, VF_NULL,
        "Toggles view orthogonal fin pass for fur in shadow passes. 0 = disabled.");

    REGISTER_CVAR3("r_FurMovementBendingBias", CV_r_FurMovementBendingBias, 0.1f, VF_NULL,
        "Bias for fur bending from animation & movement. Closer to 1 causes fur to bend back faster.");

    REGISTER_CVAR3("r_FurMaxViewDist", CV_r_FurMaxViewDist, 32.0f, VF_NULL,
        "Maximum view distance for fur shell passes.");

    REGISTER_CVAR3("r_EnableComputeDownSampling", CV_r_EnableComputeDownSampling, 0, VF_NULL,
        "Metal compute down sample\n"
        "Usage: \n"
        "0 Off\n"
        "1 ON\n");

    REGISTER_CVAR3("r_VolumetricFogDownscaledSunShadow", CV_r_VolumetricFogDownscaledSunShadow, 1, VF_NULL,
        "Enable replacing sun shadow maps with downscaled shadow maps or static shadow map if possible.\n"
        "0: disabled\n"
        "1: replace first and second cascades with downscaled shadow maps. the others are replaced with static shadow map if possible.\n"
        "2: replace first, second, and third cascades with downscaled shadow maps. the others are replaced with static shadow map if possible.\n"
        );
    REGISTER_CVAR3("r_VolumetricFogDownscaledSunShadowRatio", CV_r_VolumetricFogDownscaledSunShadowRatio, 1, VF_NULL,
        "Set downscale ratio for sun shadow maps\n"
        "0: 1/4 downscaled sun shadow maps\n"
        "1: 1/8 downscaled sun shadow maps\n"
        "2: 1/16 downscaled sun shadow maps\n"
        );
    REGISTER_CVAR3("r_VolumetricFogReprojectionMode", CV_r_VolumetricFogReprojectionMode, 1, VF_NULL,
        "Set the mode of ghost reduction for temporal reprojection.\n"
        "0: conservative\n"
        "1: advanced"
        );
    REGISTER_CVAR3("r_VolumetricFogMinimumLightBulbSize", CV_r_VolumetricFogMinimumLightBulbSize, 0.4f, VF_NULL,
        "Adjust the minimum size threshold for light attenuation bulb size. Small bulb size causes the light flicker."
        );

    REGISTER_CVAR2("r_ResolutionScale", &CV_r_ResolutionScale, CV_r_ResolutionScale, VF_NULL,
        "Scales the resolution for better performance. A value of 1 indicates no scaling.\n"
        "Minimum value 0.25: Scale resolution by 25% in width and height (retains the aspect ratio).\n"
        "Maximum value 4: Scale resolution by 400%\n"
        );

    REGISTER_CVAR3_CB("r_GPUParticleDepthCubemapResolution", CV_r_CubeDepthMapResolution, 256, VF_EXPERIMENTAL,
        "The resolution for the cubemaps used by the cubemap depth collision feature for GPU particles",
        OnChange_CV_r_CubeDepthMapResolution);
    
    REGISTER_CVAR3("r_SkipNativeUpscale", CV_r_SkipNativeUpscale, 0, VF_NULL,
        "Renders to the back buffer during the final post processing step and skips the native upscale.\n"
        "Used when a second upscale already exists to avoid having two upscales.\n"
        "0: Does not skip native upscale. \n"
        "1: Skips native upscale."
        );
    
    REGISTER_CVAR3_CB("r_SkipRenderComposites", CV_r_SkipRenderComposites, 0, VF_NULL,
        "Skips the RenderComposites call for rendering Flares and Grain. Can be used as an\n"
        "optimization to avoid a full screen render when these effects are not being used."
        "0: Does not skip RenderComposites. \n"
        "1: Skips RenderComposites",
        OnChange_CV_r_SkipRenderComposites);

    REGISTER_CVAR3("r_minConsoleFontSize", CV_r_minConsoleFontSize, 19.0f, VF_NULL,
        "Minimum size used for scaling the font when rendering the console"
    );

    REGISTER_CVAR3("r_maxConsoleFontSize", CV_r_maxConsoleFontSize, 24.0f, VF_NULL,
        "Maximum size used for scaling the font when rendering the console"
    );

    REGISTER_CVAR3("r_linuxSkipWindowCreation", CV_r_linuxSkipWindowCreation, 0, VF_NULL,
        "0: Create a rendering window like normal"
        "1: (Linux Only) Skip window creation and only render to an offscreen pixel buffer surface.  Screenshots can still be captured with r_GetScreenShot."
    );

    REGISTER_CVAR3("r_GraphicsTest00", CV_r_GraphicsTest00, 0, VF_DEV_ONLY, "Graphics programmers: Use in your code for misc graphics tests/debugging.");
    REGISTER_CVAR3("r_GraphicsTest01", CV_r_GraphicsTest01, 0, VF_DEV_ONLY, "Graphics programmers: Use in your code for misc graphics tests/debugging.");
    REGISTER_CVAR3("r_GraphicsTest02", CV_r_GraphicsTest02, 0, VF_DEV_ONLY, "Graphics programmers: Use in your code for misc graphics tests/debugging.");
    REGISTER_CVAR3("r_GraphicsTest03", CV_r_GraphicsTest03, 0, VF_DEV_ONLY, "Graphics programmers: Use in your code for misc graphics tests/debugging.");
    REGISTER_CVAR3("r_GraphicsTest04", CV_r_GraphicsTest04, 0, VF_DEV_ONLY, "Graphics programmers: Use in your code for misc graphics tests/debugging.");
    REGISTER_CVAR3("r_GraphicsTest05", CV_r_GraphicsTest05, 0, VF_DEV_ONLY, "Graphics programmers: Use in your code for misc graphics tests/debugging.");
    REGISTER_CVAR3("r_GraphicsTest06", CV_r_GraphicsTest06, 0, VF_DEV_ONLY, "Graphics programmers: Use in your code for misc graphics tests/debugging.");
    REGISTER_CVAR3("r_GraphicsTest07", CV_r_GraphicsTest07, 0, VF_DEV_ONLY, "Graphics programmers: Use in your code for misc graphics tests/debugging.");
    REGISTER_CVAR3("r_GraphicsTest08", CV_r_GraphicsTest08, 0, VF_DEV_ONLY, "Graphics programmers: Use in your code for misc graphics tests/debugging.");
    REGISTER_CVAR3("r_GraphicsTest09", CV_r_GraphicsTest09, 0, VF_DEV_ONLY, "Graphics programmers: Use in your code for misc graphics tests/debugging.");

#ifndef NULL_RENDERER
    AZ::Debug::DrillerManager* drillerManager = nullptr;
    EBUS_EVENT_RESULT(drillerManager, AZ::ComponentApplicationBus, GetDrillerManager);
    if (drillerManager && !gEnv->IsEditor())
    {
        // Create the VRAM driller
        m_vramDriller = aznew Render::Debug::VRAMDriller();
        m_vramDriller->CreateAllocationRecords(false, false, false);
        drillerManager->Register(m_vramDriller);

        // Register categories and subcategories
        Render::Debug::VRAMSubCategoryType textureSubcategories;
        textureSubcategories.push_back(Render::Debug::VRAMSubcategory(Render::Debug::VRAM_SUBCATEGORY_TEXTURE_TEXTURE, "Texture Assets"));
        textureSubcategories.push_back(Render::Debug::VRAMSubcategory(Render::Debug::VRAM_SUBCATEGORY_TEXTURE_RENDERTARGET, "Rendertargets"));
        textureSubcategories.push_back(Render::Debug::VRAMSubcategory(Render::Debug::VRAM_SUBCATEGORY_TEXTURE_DYNAMIC, "Dynamic Textures"));
        EBUS_EVENT(Render::Debug::VRAMDrillerBus, RegisterCategory, Render::Debug::VRAM_CATEGORY_TEXTURE, "Texture", textureSubcategories);

        Render::Debug::VRAMSubCategoryType meshSubcategories;
        meshSubcategories.push_back(Render::Debug::VRAMSubcategory(Render::Debug::VRAM_SUBCATEGORY_BUFFER_VERTEX_BUFFER, "Vertex Buffers"));
        meshSubcategories.push_back(Render::Debug::VRAMSubcategory(Render::Debug::VRAM_SUBCATEGORY_BUFFER_INDEX_BUFFER, "Index Buffers"));
        meshSubcategories.push_back(Render::Debug::VRAMSubcategory(Render::Debug::VRAM_SUBCATEGORY_BUFFER_CONSTANT_BUFFER, "Constant Buffers"));
        meshSubcategories.push_back(Render::Debug::VRAMSubcategory(Render::Debug::VRAM_SUBCATEGORY_BUFFER_OTHER_BUFFER, "Other Buffers"));
        EBUS_EVENT(Render::Debug::VRAMDrillerBus, RegisterCategory, Render::Debug::VRAM_CATEGORY_BUFFER, "Buffer", meshSubcategories);
    }

    m_DevMan.Init();
#endif

    m_cClearColor = ColorF(0, 0, 0, 128.0f / 255.0f);       // 128 is default GBuffer value
    m_clearBackground = false;
    m_pDefaultFont = NULL;
    m_TexGenID = 1;
    m_VSync = CV_r_vsync;
#if defined(SUPPORT_DEVICE_INFO_USER_DISPLAY_OVERRIDES)
    m_overrideRefreshRate = CV_r_overrideRefreshRate;
    m_overrideScanlineOrder = CV_r_overrideScanlineOrder;
#endif
    m_Features = 0;
    m_bVendorLibInitialized = false;

    // Initialize SThreadInfo and PerFrameParameters
    for (uint32 id = 0; id < RT_COMMAND_BUF_COUNT; ++id)
    {
        memset(&m_RP.m_TI[id].m_perFrameParameters, 0, sizeof(PerFrameParameters));
        m_RP.m_TI[id].m_nFrameID = -2;
        m_RP.m_TI[id].m_FS.m_bEnable = true;
    }

    m_bPauseTimer = 0;
    m_fPrevTime = -1.0f;

    m_CurFontColor = Col_White;

    m_bUseHWSkinning = CV_r_usehwskinning != 0;

    m_bSwapBuffers = true;

#if defined(_DEBUG) && defined(WIN32)
    if (CV_r_printmemoryleaks)
    {
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    }
#endif

    m_nUseZpass = CV_r_usezpass;

    m_nShadowPoolHeight = m_nShadowPoolWidth = 0;

    m_cloudShadowTexId = 0;
    m_cloudShadowSpeed = Vec3(0, 0, 0);
    m_cloudShadowTiling = 1;
    m_cloudShadowInvert = false;
    m_cloudShadowBrightness = 1;

    m_nGPUs = 1;

    //assert(!(FOB_MASK_AFFECTS_MERGING & 0xffff));
    //assert(sizeof(CRenderObject) == 256);
#if !defined(PLATFORM_64BIT) && !defined(ANDROID)
    // Disabled this check for ShaderCache Gen, it is only a performacne thingy for last gen consoles anyway
    //STATIC_CHECK(sizeof(CRenderObject) == 128, CRenderObject);
#endif
    STATIC_CHECK(!(FOB_MASK_AFFECTS_MERGING & 0xffff), FOB_MASK_AFFECTS_MERGING);

    if (!g_pSDynTexture_PoolAlloc)
    {
        g_pSDynTexture_PoolAlloc = new SDynTexture_PoolAlloc(stl::FHeap().FreeWhenEmpty(true));
    }

    //m_RP.m_VertPosCache.m_nBufSize = 500000 * sizeof(Vec3);
    //m_RP.m_VertPosCache.m_pBuf = new byte [gRenDev->m_RP.m_VertPosCache.m_nBufSize];

    m_pDefaultMaterial = NULL;
    m_pTerrainDefaultMaterial = NULL;

    m_ViewMatrix.SetIdentity();
    m_CameraMatrix.SetIdentity();
    for (int i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
    {
        m_CameraZeroMatrix[i].SetIdentity();
    }

    for (int i = 0; i < MAX_NUM_VIEWPORTS; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            m_PreviousFrameMatrixSets[i][j].m_ViewMatrix.SetIdentity();
            m_PreviousFrameMatrixSets[i][j].m_ProjMatrix.SetIdentity();
            m_PreviousFrameMatrixSets[i][j].m_ViewProjMatrix.SetIdentity();
            m_PreviousFrameMatrixSets[i][j].m_ViewNoTranslateMatrix.SetIdentity();
            m_PreviousFrameMatrixSets[i][j].m_ViewProjNoTranslateMatrix.SetIdentity();
            m_PreviousFrameMatrixSets[i][j].m_WorldViewPosition.zero();
        }
    }

    m_CameraMatrixNearest.SetIdentity();

    m_ProjMatrix.SetIdentity();
    m_TranspOrigCameraProjMatrix.SetIdentity();
    m_ViewProjMatrix.SetIdentity();
    m_ViewProjNoTranslateMatrix.SetIdentity();
    m_ViewProjInverseMatrix.SetIdentity();
    m_IdentityMatrix.SetIdentity();

    m_TemporalJitterClipSpace = Vec4(0.0f);
    
    m_RP.m_nZOcclusionBufferID = -1;

    m_RP.m_nCurrResolveBounds[0] = m_RP.m_nCurrResolveBounds[1] = m_RP.m_nCurrResolveBounds[2] = m_RP.m_nCurrResolveBounds[3] = 0;

    for (int i = 0; i < SIZEOF_ARRAY(m_TempMatrices); i++)
    {
        for (int j = 0; j < SIZEOF_ARRAY(m_TempMatrices[0]); j++)
        {
            m_TempMatrices[i][j].SetIdentity();
        }
    }

    CParserBin::m_bParseFX = true;
    //CParserBin::m_bEmbeddedSearchInfo = false;
    if (gEnv->IsEditor())
    {
        CParserBin::m_bEditable = true;
    }
#ifndef CONSOLE_CONST_CVAR_MODE
    CV_e_DebugTexelDensity = 0;
#endif
    m_nFlushAllPendingTextureStreamingJobs = 0;
    m_fTexturesStreamingGlobalMipFactor = 0.f;

    m_fogCullDistance = 0.0f;

    m_pDebugRenderNode = NULL;

    m_bCollectDrawCallsInfo = false;
    m_bCollectDrawCallsInfoPerNode = false;

    m_nMeshPoolTimeoutCounter = nMeshPoolMaxTimeoutCounter;

    // Init Thread Safe Worker Containers
    threadID nThreadId = CryGetCurrentThreadId();
    for (int i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
    {
        m_RP.m_arrCustomShadowMapFrustumData[i].Init();
        m_RP.m_arrCustomShadowMapFrustumData[i].SetNonWorkerThreadID(nThreadId);
        m_RP.m_TempObjects[i].Init();
        m_RP.m_TempObjects[i].SetNonWorkerThreadID(nThreadId);
    }
    for (int i = 0; i < RT_COMMAND_BUF_COUNT; i++)
    {
        m_RP.m_pRenderViews[i].reset(new CRenderView);
    }
    m_RP.m_pCurrentRenderView = m_RP.m_pRenderViews[0].get();
    m_RP.m_pCurrentFillView = m_RP.m_pRenderViews[0].get();
    m_pRT = new SRenderThread;
    m_pRT->StartRenderThread();

    m_ShadowFrustumMGPUCache.Init();
    RegisterSyncWithMainListener(&m_ShadowFrustumMGPUCache);

    // on console some float values in vertex formats can be 16 bit
    iLog->Log("CRenderer sizeof(Vec2f16)=%" PRISIZE_T " sizeof(Vec3f16)=%" PRISIZE_T "", sizeof(Vec2f16), sizeof(Vec3f16));
    CRenderMesh::Initialize();
}

CRenderer::~CRenderer()
{
    //Code now moved to Release()

    CCryNameR::ReleaseNameTable();
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::PostInit()
{
    LOADING_TIME_PROFILE_SECTION;

    //////////////////////////////////////////////////////////////////////////
    // Initialize the shader system
    //////////////////////////////////////////////////////////////////////////
    m_cEF.mfPostInit();

    // Initialize asset messaging listener
    m_assetListener.Connect();
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED && !defined(NULL_RENDERER)
    if (m_contextManager)
    {
        m_contextManager->Init();
    }
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED && !defined(NULL_RENDERER)

#if !defined(NULL_RENDERER)
    //////////////////////////////////////////////////////////////////////////
    // Load internal renderer font.
    //////////////////////////////////////////////////////////////////////////
    if (gEnv->pCryFont)
    {
        m_pDefaultFont = gEnv->pCryFont->GetFont("default");
        if (!m_pDefaultFont)
        {
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Error getting default font");
        }
    }

    if (!m_bShaderCacheGen)
    {
        // Create system resources while in fast load phase
        gEnv->pRenderer->InitSystemResources(FRR_SYSTEM_RESOURCES);
    }
#endif
}

//////////////////////////////////////////////////////////////////////////

void CRenderer::Release()
{
    m_assetListener.Disconnect();

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED && !defined(NULL_RENDERER)
    m_contextManager = nullptr;
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED && !defined(NULL_RENDERER)

    // Reminder for Andrey/AntonKap: this needs to be properly handled
    //SAFE_DELETE(g_pSDynTexture_PoolAlloc)
    //g_pSDynTexture_PoolAlloc = NULL;

    RemoveSyncWithMainListener(&m_ShadowFrustumMGPUCache);
    m_ShadowFrustumMGPUCache.Release();
    CRenderMesh::ShutDown();
    CHWShader::mfCleanupCache();

    if (!m_DevBufMan.Shutdown())
    {
        CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR_DBGBRK, "could not free all buffers from CDevBufferMan!");
    }

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_CPP_SECTION_9
    #include AZ_RESTRICTED_FILE(Renderer_cpp)
#endif

    if (g_shaderGeneralHeap)
    {
        g_shaderGeneralHeap->Release();
    }

    // Shutdown the VRAM driller
    if (m_vramDriller)
    {
        EBUS_EVENT(Render::Debug::VRAMDrillerBus, UnregisterAllCategories);

        AZ::Debug::DrillerManager* drillerManager = nullptr;
        EBUS_EVENT_RESULT(drillerManager, AZ::ComponentApplicationBus, GetDrillerManager);
        if (drillerManager)
        {
            drillerManager->Unregister(m_vramDriller);
            delete m_vramDriller;
            m_vramDriller = nullptr;
        }
    }

    gRenDev = NULL;
}

//////////////////////////////////////////////////////////////////////
void CRenderer::AddRenderDebugListener(IRenderDebugListener* pRenderDebugListener)
{
    stl::push_back_unique(m_listRenderDebugListeners, pRenderDebugListener);
}

//////////////////////////////////////////////////////////////////////
void CRenderer::RemoveRenderDebugListener(IRenderDebugListener* pRenderDebugListener)
{
    stl::find_and_erase(m_listRenderDebugListeners, pRenderDebugListener);
}

//////////////////////////////////////////////////////////////////////
void CRenderer::TextToScreenColor(int x, int y, float r, float g, float b, float a, const char* format, ...)
{
    //  if(!cVars->e_text_info)
    //  return;

    char buffer[512];
    va_list args;
    va_start(args, format);
    if (azvsnprintf(buffer, sizeof(buffer), format, args) == -1)
    {
        buffer[sizeof(buffer) - 1] = 0;
    }
    va_end(args);

    WriteXY((int)(8 * x), (int)(6 * y), 1, 1, r, g, b, a, buffer);
}

//////////////////////////////////////////////////////////////////////
void CRenderer::TextToScreen(float x, float y, const char* format, ...)
{
    //  if(!cVars->e_text_info)
    //  return;

    char buffer[512];
    va_list args;
    va_start(args, format);
    if (azvsnprintf(buffer, sizeof(buffer), format, args) == -1)
    {
        buffer[sizeof(buffer) - 1] = 0;
    }
    va_end(args);

    WriteXY((int)(8 * x), (int)(6 * y), 1, 1, 1, 1, 1, 1, buffer);
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::Draw2dText(float posX, float posY, const char* pStr, const SDrawTextInfo& ti)
{
    Draw2dTextWithDepth(posX, posY, 1.0f, pStr, ti);
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::Draw2dTextWithDepth(float posX, float posY, float posZ, const char* pStr, const SDrawTextInfo& ti)
{
    IF (!m_pDefaultFont, 0)
    {
        return;
    }

    IFFont* pFont = m_pDefaultFont;

    const float r = CLAMP(ti.color[0], 0.0f, 1.0f);
    const float g = CLAMP(ti.color[1], 0.0f, 1.0f);
    const float b = CLAMP(ti.color[2], 0.0f, 1.0f);
    const float a = CLAMP(ti.color[3], 0.0f, 1.0f);

    STextDrawContext ctx;
    ctx.SetBaseState(GS_NODEPTHTEST);
    ctx.SetColor(ColorF(r, g, b, a));
    ctx.SetCharWidthScale(1.0f);
    ctx.EnableFrame((ti.flags & eDrawText_Framed) != 0);

    if (ti.flags & eDrawText_Monospace)
    {
        if (ti.flags & eDrawText_FixedSize)
        {
            ctx.SetSizeIn800x600(false);
        }
        ctx.SetSize(Vec2(UIDRAW_TEXTSIZEFACTOR * ti.xscale, UIDRAW_TEXTSIZEFACTOR * ti.yscale));
        ctx.SetCharWidthScale(0.5f);
        ctx.SetProportional(false);

        if (ti.flags & eDrawText_800x600)
        {
            ScaleCoordInternal(posX, posY);
        }
    }
    else if (ti.flags & eDrawText_FixedSize)
    {
        ctx.SetSizeIn800x600(false);
        ctx.SetSize(Vec2(UIDRAW_TEXTSIZEFACTOR * ti.xscale, UIDRAW_TEXTSIZEFACTOR * ti.yscale));
        ctx.SetProportional(true);

        if (ti.flags & eDrawText_800x600)
        {
            ScaleCoordInternal(posX, posY);
        }
    }
    else
    {
        ctx.SetSizeIn800x600(true);
        ctx.SetProportional(false);
        ctx.SetCharWidthScale(0.5f);
        ctx.SetSize(Vec2(UIDRAW_TEXTSIZEFACTOR * ti.xscale, UIDRAW_TEXTSIZEFACTOR * ti.yscale));
    }

    // align left/right/center
    if (ti.flags & (eDrawText_Center | eDrawText_CenterV | eDrawText_Right))
    {
        Vec2 textSize = pFont->GetTextSize(pStr, true, ctx);

        // If we're using virtual 800x600 coordinates, convert the text size from
        // pixels to that before using it as an offset.
        if (ctx.m_sizeIn800x600)
        {
            float width = 1.0f;
            float height = 1.0f;
            ScaleCoordInternal(width, height);
            textSize.x /= width;
            textSize.y /= height;
        }

        if (ti.flags & eDrawText_Center)
        {
            posX -= textSize.x * 0.5f;
        }
        else if (ti.flags & eDrawText_Right)
        {
            posX -= textSize.x;
        }

        if (ti.flags & eDrawText_CenterV)
        {
            posY -= textSize.y * 0.5f;
        }
    }

    // Pass flags so that overscan borders can be applied if necessary
    ctx.SetFlags(ti.flags);

    pFont->DrawString(posX, posY, posZ, pStr, true, ctx);
}

void CRenderer::PrintToScreen(float x, float y, float size, const char* buf)
{
    SDrawTextInfo ti;
    ti.xscale = size * 0.5f / 8;
    ti.yscale = size * 1.f / 8;
    ti.color[0] = 1;
    ti.color[1] = 1;
    ti.color[2] = 1;
    ti.color[3] = 1;
    ti.flags = eDrawText_800x600 | eDrawText_2D;
    Draw2dText(x, y, buf, ti);
}

void CRenderer::WriteXY(int x, int y, float xscale, float yscale, float r, float g, float b, float a, const char* format, ...)
{
    //////////////////////////////////////////////////////////////////////
    // Write a string to the screen
    //////////////////////////////////////////////////////////////////////

    va_list args;

    char buffer[512];

    // Check for the presence of a D3D device
    // Format the string
    va_start(args, format);
    if (azvsnprintf(buffer, sizeof(buffer), format, args) == -1)
    {
        buffer[sizeof(buffer) - 1] = 0;
    }
    va_end(args);

    SDrawTextInfo ti;
    ti.xscale = xscale;
    ti.yscale = yscale;
    ti.color[0] = r;
    ti.color[1] = g;
    ti.color[2] = b;
    ti.color[3] = a;
    ti.flags = eDrawText_800x600 | eDrawText_2D;
    Draw2dText((float)x, (float)y, buffer, ti);
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::DrawTextQueued(Vec3 pos, SDrawTextInfo& ti, const char* text)
{
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    if (IsRenderToTextureActive())
    {
        // don't add debug text from the render to texture pass
        return;
    }
#endif // AZ_RENDER_TO_TEXTURE_GEM_ENABLED

    int nT = m_pRT->GetThreadList();
    if (text && !gEnv->IsDedicated())
    {
        // ti.yscale is currently ignored, input struct can be refactored

        ColorB col(ColorF(ti.color[0], ti.color[1], ti.color[2], ti.color[3]));

        m_TextMessages[nT].PushEntry_Text(pos, col, ti.xscale, ti.flags, text);
    }
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::DrawTextQueued(Vec3 pos, SDrawTextInfo& ti, const char* format, va_list args)
{
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    if (IsRenderToTextureActive())
    {
        // don't add debug text from the render to texture pass
        return;
    }
#endif // AZ_RENDER_TO_TEXTURE_GEM_ENABLED

    int nT = m_pRT->GetThreadList();
    if (format && !gEnv->IsDedicated())
    {
        char str[512];

        vsnprintf_s(str, sizeof(str), sizeof(str) - 1, format, args);
        str[sizeof(str) - 1] = 0;

        // ti.yscale is currently ignored, input struct can be refactored

        ColorB col(ColorF(ti.color[0], ti.color[1], ti.color[2], ti.color[3]));

        m_TextMessages[nT].PushEntry_Text(pos, col, ti.xscale, ti.flags, str);
    }
}

//////////////////////////////////////////////////////////////////////
void CRenderer::EF_RenderTextMessages()
{
    ASSERT_IS_MAIN_THREAD(m_pRT)

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    if (IsRenderToTextureActive())
    {
        // don't draw debug text in the render to texture pass
        return;
    }
#endif // AZ_RENDER_TO_TEXTURE_GEM_ENABLED

    CTextMessages& textMessage = m_TextMessages[m_RP.m_nFillThreadID];
    if (!textMessage.empty())
    {
        RenderTextMessages(textMessage);
        textMessage.Clear(false);
    }
}

void CRenderer::RenderTextMessages(CTextMessages& messages)
{
    bool prevFog = EnableFog(false);    //save previous fog value returned by EnableFog
    int vx, vy, vw, vh;
    GetViewport(&vx, &vy, &vw, &vh);

    while (const CTextMessages::CTextMessageHeader* pEntry = messages.GetNextEntry())
    {
        const CTextMessages::SText* pText = pEntry->CastTo_Text();

        Vec3 vPos(0, 0, 0);
        int nDrawFlags = 0;
        const char* szText = 0;
        Vec4 vColor(1, 1, 1, 1);
        float fSize = 0;

        if (pText)
        {
            nDrawFlags = pText->m_nDrawFlags;
            szText = pText->GetText();
            vPos = pText->m_vPos;
            vColor = pText->m_Color.toVec4() * 1.0f / 255.0f;
            fSize = pText->m_fFontSize;
        }

        bool b800x600 = (nDrawFlags & eDrawText_800x600) != 0;

        float fMaxPosX = 100.0f;
        float fMaxPosY = 100.0f;

        if (!b800x600)
        {
            fMaxPosX = (float)vw;
            fMaxPosY = (float)vh;
        }

        float sx, sy, sz;

        if (!(nDrawFlags & eDrawText_2D))
        {
            float fDist = 1; //GetDistance(pTextInfo->pos,GetCamera().GetPosition());

            float K = GetCamera().GetFarPlane() / fDist;
            if (fDist > GetCamera().GetFarPlane() * 0.5)
            {
                vPos = GetCamera().GetPosition() + K * (vPos - GetCamera().GetPosition());
            }

            ProjectToScreen(vPos.x, vPos.y, vPos.z, &sx, &sy, &sz);

            if (!b800x600)
            {
                // ProjectToScreen() returns virtual screen values in range [0-100], while the Draw2dTextWithDepth() method expects screen coords.
                // Correcting sx, sy values if not in virtual screen mode (sz is depth in range [0-1], and does not need to be altered).
                sx = vw ? (sx / 100.f) * vw : sx;
                sy = vh ? (sy / 100.f) * vh : sy;
            }
        }
        else
        {
            if (b800x600)
            {
                // Make 2D coords in range 0-100
                sx = (vPos.x) / vw * 100;
                sy = (vPos.y) / vh * 100;
            }
            else
            {
                sx = vPos.x;
                sy = vPos.y;
            }

            sz = vPos.z;
        }

        if (sx >= 0 && sx <= fMaxPosX)
        {
            if (sy >= 0 && sy <= fMaxPosY)
            {
                if (sz >= 0 && sz <= 1)
                {
                    // calculate size
                    float sizeX;
                    float sizeY;
                    if (nDrawFlags & eDrawText_FixedSize)
                    {
                        sizeX = fSize;
                        sizeY = fSize;
                        //sizeX = pTextInfo->font_size * 800.0f/vw;
                        //sizeY = pTextInfo->font_size * 500.0f/vh;
                    }
                    else
                    {
                        sizeX = sizeY = (1.0f - (float)(sz)) * 32.f * fSize;
                        sizeX *= 0.5f;
                    }

                    if (szText)
                    {
                        // print
                        SDrawTextInfo ti;
                        ti.flags = nDrawFlags;
                        ti.color[0] = vColor.x;
                        ti.color[1] = vColor.y;
                        ti.color[2] = vColor.z;
                        ti.color[3] = vColor.w;
                        ti.xscale = sizeX;
                        ti.yscale = sizeY;
                        if (nDrawFlags & eDrawText_DepthTest)
                        {
                            sz = 1.0f - 2.0f * sz;
                        }
                        else
                        {
                            sz = 1.0f;
                        }
                        if (b800x600)
                        {
                            Draw2dTextWithDepth(0.01f * 800 * sx, 0.01f * 600 * sy, sz, szText, ti);
                        }
                        else
                        {
                            Draw2dTextWithDepth(sx, sy, sz, szText, ti);
                        }
                    }
                }
            }
        }
    }

    //If fog was enabled before we disabled it here, re-enable it
    if (prevFog)
    {
        EnableFog(true);
    }
}

void CRenderer::RT_RenderTextMessages()
{
    ASSERT_IS_RENDER_THREAD(m_pRT)
    FUNCTION_PROFILER_LEGACYONLY(GetISystem(), PROFILE_RENDERER);
    AZ_TRACE_METHOD();

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    if (IsRenderToTextureActive())
    {
        // don't draw debug text in the render to texture pass
        return;
    }
#endif // AZ_RENDER_TO_TEXTURE_GEM_ENABLED

    int nT = m_pRT->GetThreadList();

    if (gEnv->IsDedicated() || (m_pRT && m_pRT->m_eVideoThreadMode != SRenderThread::eVTM_Disabled))
    {
        m_TextMessages[nT].Clear();
        return;
    }

    bool resetTextMessages = true;

    // If stereo is enabled then we don't want to reset the flush position UNLESS we're rendering the second eye. Otherwise,
    // the text will only get drawn to the first eye.
    if (gEnv->pRenderer->IsStereoEnabled())
    {
        if (gEnv->pRenderer->GetIStereoRenderer()->GetStatus() == IStereoRenderer::Status::kRenderingFirstEye)
        {
            resetTextMessages = false;
        }
    }

    RenderTextMessages(m_TextMessages[nT]);
    m_TextMessages[nT].Clear(!resetTextMessages);
}

#if !defined(LINUX) && !defined(APPLE)
#pragma pack (push)
#pragma pack (1)
typedef struct
{
    unsigned char  id_length, colormap_type, image_type;
    unsigned short colormap_index, colormap_length;
    unsigned char  colormap_size;
    unsigned short x_origin, y_origin, width, height;
    unsigned char  pixel_size, attributes;
}   TargaHeader_t;
#pragma pack (pop)
#else
typedef struct
{
    unsigned char  id_length, colormap_type, image_type;
    unsigned short colormap_index _PACK, colormap_length _PACK;
    unsigned char  colormap_size;
    unsigned short x_origin _PACK, y_origin _PACK, width _PACK, height _PACK;
    unsigned char  pixel_size, attributes;
}   TargaHeader_t;
#endif

bool    CRenderer::SaveTga(unsigned char* sourcedata, int sourceformat, int w, int h, const char* filename, bool flip) const
{
    //assert(0);
    //  return CImage::SaveTga(sourcedata,sourceformat,w,h,filename,flip);

    if (flip)
    {
        int size = w * (sourceformat / 8);
        unsigned char* tempw = new unsigned char[size];
        unsigned char* src1 = sourcedata;
        unsigned char* src2 = sourcedata + (w * (sourceformat / 8)) * (h - 1);
        for (int k = 0; k < h / 2; k++)
        {
            memcpy(tempw, src1, size);
            memcpy(src1, src2, size);
            memcpy(src2, tempw, size);
            src1 += size;
            src2 -= size;
        }
        delete[] tempw;
    }


    unsigned char* oldsourcedata = sourcedata;

    if (sourceformat == FORMAT_8_BIT)
    {
        unsigned char* desttemp = new unsigned char[w * h * 3];
        memset(desttemp, 0, w * h * 3);

        unsigned char* destptr = desttemp;
        unsigned char* srcptr = sourcedata;

        unsigned char col;

        for (int k = 0; k < w * h; k++)
        {
            col = *srcptr++;
            *destptr++ = col;
            *destptr++ = col;
            *destptr++ = col;
        }

        sourcedata = desttemp;

        sourceformat = FORMAT_24_BIT;
    }

    TargaHeader_t header;

    memset(&header, 0, sizeof(header));
    header.image_type = 2;
    header.width = w;
    header.height = h;
    header.pixel_size = sourceformat;

    unsigned char* data = new unsigned char[w * h * (sourceformat >> 3)];
    unsigned char* dest = data;
    unsigned char* source = sourcedata;

    //memcpy(dest,source,w*h*(sourceformat>>3));

    for (int ax = 0; ax < h; ax++)
    {
        for (int by = 0; by < w; by++)
        {
            unsigned char r, g, b, a;
            r = *source;
            source++;
            g = *source;
            source++;
            b = *source;
            source++;
            if (sourceformat == FORMAT_32_BIT)
            {
                a = *source;
                source++;
            }
            *dest = b;
            dest++;
            *dest = g;
            dest++;
            *dest = r;
            dest++;
            if (sourceformat == FORMAT_32_BIT)
            {
                *dest = a;
                dest++;
            }
        }
    }

    AZ::IO::HandleType fileHandle = fxopen(filename, "wb");
    if (fileHandle == AZ::IO::InvalidHandle)
    {
        //("Cannot save %s\n",filename);
        delete[] data;
        return (false);
    }

    if (!gEnv->pFileIO->Write(fileHandle, &header, sizeof(header)))
    {
        //CLog::LogToFile("Cannot save %s\n",filename);
        delete[] data;
        gEnv->pFileIO->Close(fileHandle);
        return (false);
    }

    if (!gEnv->pFileIO->Write(fileHandle, data, w * h * (sourceformat >> 3)))
    {
        //CLog::LogToFile("Cannot save %s\n",filename);
        delete[] data;
        gEnv->pFileIO->Close(fileHandle);
        return (false);
    }

    gEnv->pFileIO->Close(fileHandle);

    delete[] data;
    if (sourcedata != oldsourcedata)
    {
        delete[] sourcedata;
    }


    return (true);
}

//================================================================

void CRenderer::EF_ReleaseInputShaderResource(SInputShaderResources* pRes)
{
    pRes->Cleanup();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

//#include "../Common/Character/CryModel.h"

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_CPP_SECTION_10
    #include AZ_RESTRICTED_FILE(Renderer_cpp)
#endif
void CRenderer::ForceSwapBuffers()
{
    m_pRT->RC_ForceSwapBuffers();
    ForceFlushRTCommands();
}

// Initializes the default textures as well as texture semantics
void CRenderer::InitTexturesSemantics()
{
    CTextureManager::Instance()->LoadMaterialTexturesSemantics();
}

void CRenderer::InitSystemResources([[maybe_unused]] int nFlags)
{
    LOADING_TIME_PROFILE_SECTION;
    if (!m_bSystemResourcesInit || m_bDeviceLost == 2)
    {
        iLog->Log("*** Init system render resources ***");

        bool bPrecache = CTexture::s_bPrecachePhase;
        CTexture::s_bPrecachePhase = false;

        ForceFlushRTCommands();
        m_cEF.mfPreloadBinaryShaders();
        m_cEF.mfLoadBasicSystemShaders();
        m_cEF.mfLoadDefaultSystemShaders();

        // The following two lines will initiate the Texture manager instance and load all 
        // default textures and create engine textures
        CTextureManager::Instance()->Init();
        CTexture::LoadDefaultSystemTextures();

        m_pRT->RC_CreateRenderResources();
        m_pRT->RC_PrecacheDefaultShaders();
        m_pRT->RC_CreateSystemTargets();
        ForceFlushRTCommands();

        CTexture::s_bPrecachePhase = bPrecache;

        if (m_bDeviceLost == 2)
        {
            m_bDeviceLost = 0;
        }
        m_bSystemResourcesInit = 1;
    }
}

void CRenderer::FreeResources(int nFlags)
{
    if (iLog)
    {
        iLog->Log("*** Start clearing render resources ***");
    }

    if (m_bEditor)
    {
        return;
    }

    CTimeValue tBegin = gEnv->pTimer->GetAsyncTime();

    // Throughout this function, we are queuing up a lot of work to execute on the render thread while also manipulating 
    // global state on both the render and main threads.  We need to call ForceFlushRTCommands to synchronize the main and
    // render threads whenever we are manipulating global state on either of these threads.
    ForceFlushRTCommands();

    AZ::RenderNotificationsBus::Broadcast(&AZ::RenderNotifications::OnRendererFreeResources, nFlags);

#if !defined(_RELEASE)
    ClearDrawCallsInfo();
#endif
    CHWShader::mfFlushPendedShadersWait(-1);

    EF_ReleaseDeferredData();

    if (nFlags & FRR_FLUSH_TEXTURESTREAMING)
    {
        m_pRT->RC_FlushTextureStreaming(true);
    }

    if (nFlags & FRR_DELETED_MESHES)
    {
        for (size_t i = 0; i < MAX_RELEASED_MESH_FRAMES; ++i)
        {
            m_pRT->RC_ForceMeshGC(false, false);
        }
        ForceFlushRTCommands();
    }

    if (nFlags & FRR_SHADERS)
    {
        gRenDev->m_cEF.ShutDown();
    }

    if (nFlags & FRR_RP_BUFFERS)
    {
        ForceFlushRTCommands();

        for (int i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
        {
            for (int j = 0; j < MAX_REND_RECURSION_LEVELS; ++j)
            {
                TArray<CREClientPoly*>& storage = CREClientPoly::m_PolysStorage[i][j];
                storage.SetUse(storage.Capacity());
                for (size_t poly = 0, polyCount = storage.Capacity(); poly != polyCount; ++poly)
                {
                    if (storage[poly])
                    {
                        storage[poly]->Release(false);
                    }
                }
                storage.Free();

                m_RP.m_SMFrustums[i][j].Free();
                m_RP.m_SMCustomFrustumIDs[i][j].Free();
                m_RP.m_DLights[i][j].Free();
                m_RP.m_DeferredDecals[i][j].clear();
            }

            m_RP.m_arrCustomShadowMapFrustumData[i].clear();
            m_RP.m_fogVolumeContibutionsData[i].clear();
        }

        for (int i = 0; i < sizeof(m_RP.m_RIs) / sizeof(m_RP.m_RIs[0]); ++i)
        {
            m_RP.m_RIs[i].Free();
        }

        stl::for_each_array(m_RP.m_SysVertexPool, stl::container_freer());
        stl::for_each_array(m_RP.m_SysIndexPool, stl::container_freer());

        // Reset render views
        for (int i = 0; i < RT_COMMAND_BUF_COUNT; i++)
        {
            m_RP.m_pRenderViews[i]->FreeRenderItems();
        }

        for (ShadowFrustumListsCache::iterator it = m_FrustumsCache.begin();
             it != m_FrustumsCache.end(); ++it)
        {
            SAFE_DELETE(it->second);
        }
    }

    if (nFlags & (FRR_SYSTEM | FRR_OBJECTS))
    {
        CMotionBlur::FreeData();
        FurBendData::Get().FreeData();

        for (int i = 0; i < 3; ++i)
        {
            m_SkinningDataPool[i].FreePoolMemory();
        }

        ForceFlushRTCommands();

        // Get object pool range
        CRenderObject* pObjPoolStart = &m_RP.m_ObjectsPool[0];
        CRenderObject* pObjPoolEnd = &m_RP.m_ObjectsPool[m_RP.m_nNumObjectsInPool * RT_COMMAND_BUF_COUNT];

        // Delete all items that have not been allocated from the object pool
        for (int i = 0; i < RT_COMMAND_BUF_COUNT; i++)
        {
            m_RP.m_TempObjects[i].clear(SDeleteNonePoolRenderObjs(pObjPoolStart, pObjPoolEnd));
        }
    }

    if (nFlags & FRR_TEXTURES)
    {
        // Release the system textures on the render thread and then force that task to be completed
        // before shutting down the texture system
        m_pRT->RC_ReleaseSystemTextures();
        ForceFlushRTCommands();

        CTexture::ShutDown();
    }

    if (nFlags & FRR_OBJECTS)
    {
        for (uint32 j = 0; j < RT_COMMAND_BUF_COUNT; j++)
        {
            m_RP.m_TempObjects[j].clear();
        }
        if (m_RP.m_ObjectsPool)
        {
            for (int j = 0; j < (int)(m_RP.m_nNumObjectsInPool * RT_COMMAND_BUF_COUNT); j++)
            {
                CRenderObject* pRendObj = &m_RP.m_ObjectsPool[j];
                pRendObj->~CRenderObject();
            }

            CryModuleMemalignFree(m_RP.m_ObjectsPool);
            m_RP.m_ObjectsPool = NULL;
            SAFE_DELETE(m_RP.m_pIdendityRenderObject);
            m_RP.m_nNumObjectsInPool = 0;
        }
    }

    if (nFlags == FRR_ALL)
    {
        ForceFlushRTCommands();
        CRendElementBase::ShutDown();
    }
    else
    if (nFlags & FRR_RENDERELEMENTS)
    {
        CRendElement::Cleanup();
    }

    if (nFlags & FRR_POST_EFFECTS)
    {
        m_pRT->RC_ReleasePostEffects();
        ForceFlushRTCommands();
    }

    if (nFlags & FRR_SYSTEM_RESOURCES)
    {
        for (int i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
        {
            stl::free_container(m_RP.SShadowFrustumToRenderList[i]);
        }

        // Free sprite vertices (indices are packed into the same buffer so no need to free them explicitly);
        CryModuleMemalignFree(m_pSpriteVerts);
        m_pSpriteVerts = NULL;
        m_pSpriteInds = NULL;

        m_p3DEngineCommon.m_RainOccluders.Release(true);
        m_p3DEngineCommon.m_CausticInfo.Release();

        m_pRT->RC_UnbindResources();
        m_pRT->RC_ResetGlass();        
        m_pRT->RC_ForceMeshGC(false, false);
        m_cEF.mfReleaseSystemShaders();
        ForceFlushRTCommands();

        m_pRT->RC_ReleaseRenderResources();
        ForceFlushRTCommands();

        if (m_pPostProcessMgr)
        {
            m_pPostProcessMgr->ReleaseResources();
        }
        ForceFlushRTCommands();

        m_pRT->RC_FlushTextureStreaming(true);
        ForceFlushRTCommands();

        m_pRT->RC_ReleaseSystemTextures();
        ForceFlushRTCommands();

        m_pRT->RC_UnbindTMUs();

        // This function calls FlushAndWait internally in order to synchronize the main and render threads
        CRendElement::Cleanup();

        // sync dev buffer only once per frame, to prevent syncing to the currently rendered frame
        // which would result in a deadlock
        if (nFlags & (FRR_SYSTEM_RESOURCES | FRR_DELETED_MESHES))
        {
            m_pRT->RC_DevBufferSync();
            ForceFlushRTCommands();
        }

        PrintResourcesLeaks();

        if (!m_bDeviceLost)
        {
            m_bDeviceLost = 2;
        }
        m_bSystemResourcesInit = 0;
    }



    // free flare queries
    CRELensOptics::ClearResources();

    if ((nFlags & FRR_RESTORE) && !(nFlags & FRR_SYSTEM))
    {
        m_cEF.mfInit();
    }

    CTimeValue tDeltaTime = gEnv->pTimer->GetAsyncTime() - tBegin;
    if (iLog)
    {
        iLog->Log("*** Clearing render resources took %.1f msec ***", tDeltaTime.GetMilliSeconds());
    }
}

Vec2 CRenderer::SetViewportDownscale(float xscale, float yscale)
{
#ifdef WIN32
    // refuse to downscale in editor or if MSAA is enabled
    if (gEnv->IsEditor() || m_RP.IsMSAAEnabled())
    {
        m_ReqViewportScale = Vec2(1, 1);
        return m_ReqViewportScale;
    }

    // PC can have awkward resolutions. When setting to full scale, take it as literal (below rounding
    // to multiple of 8 might not work as intended for some resolutions).
    if (xscale >= 1.0f && yscale >= 1.0f)
    {
        m_ReqViewportScale = Vec2(1, 1);
        return m_ReqViewportScale;
    }
#endif

    float fWidth = float(GetWidth());
    float fHeight = float(GetHeight());

    int xres = int(fWidth * xscale);
    int yres = int(fHeight * yscale);

    // clamp to valid value
    xres = CLAMP(xres, 128, GetWidth());
    yres = CLAMP(yres, 128, GetHeight());

    // round down to multiple of 8
    xres &= ~0x7;
    yres &= ~0x7;

    m_ReqViewportScale.x = float(xres) / fWidth;
    m_ReqViewportScale.y = float(yres) / fHeight;

    return m_ReqViewportScale;
}

EScreenAspectRatio CRenderer::GetScreenAspect(int nWidth, int nHeight)
{
    EScreenAspectRatio eSA = eAspect_Unknown;

    float fNeed16_9 = 16.0f / 9.0f;
    float fNeed16_10 = 16.0f / 10.0f;
    float fNeed4_3 = 4.0f / 3.0f;

    float fCur = (float)nWidth / (float)nHeight;
    if (fabs(fCur - fNeed16_9) < 0.1f)
    {
        eSA = eAspect_16_9;
    }

    if (fabs(fCur - fNeed4_3) < 0.1f)
    {
        eSA = eAspect_4_3;
    }

    if (fabs(fCur - fNeed16_10) < 0.1f)
    {
        eSA = eAspect_16_10;
    }

    return eSA;
}

bool CRenderer::WriteTGA(const byte* dat, int wdt, int hgt, const char* name, int src_bits_per_pixel, int dest_bits_per_pixel)
{
    return ::WriteTGA(dat, wdt, hgt, name, src_bits_per_pixel, dest_bits_per_pixel);
}

bool CRenderer::WriteDDS([[maybe_unused]] const byte* dat, [[maybe_unused]] int wdt, [[maybe_unused]] int hgt, [[maybe_unused]] int Size, [[maybe_unused]] const char* nam, [[maybe_unused]] ETEX_Format eFDst, [[maybe_unused]] int NumMips)
{
#if (defined(WIN32) || defined(WIN64)) && !defined(NULL_RENDERER)
    bool bRet = true;

    byte* data = NULL;
    if (Size == 3)
    {
        data = new byte[wdt * hgt * 4];
        for (int i = 0; i < wdt * hgt; i++)
        {
            data[i * 4 + 0] = dat[i * 3 + 0];
            data[i * 4 + 1] = dat[i * 3 + 1];
            data[i * 4 + 2] = dat[i * 3 + 2];
            data[i * 4 + 3] = 255;
        }
        dat = data;
    }
    char name[256];
    fpStripExtension(nam, name);
    cry_strcat(name, ".dds");

    bool bMips = false;
    if (NumMips != 1)
    {
        bMips = true;
    }
    int nDxtSize;
    byte* dst = CTexture::Convert(dat, wdt, hgt, NumMips, eTF_R8G8B8A8, eFDst, nDxtSize, true);
    if (dst)
    {
        ::WriteDDS(dst, wdt, hgt, 1, name, eFDst, NumMips, eTT_2D);
        delete[] dst;
    }
    if (data)
    {
        delete[] data;
    }

    return bRet;
#else
    return false;
#endif
}

void CRenderer::EF_SetShaderMissCallback(ShaderCacheMissCallback callback)
{
    m_cEF.m_ShaderCacheMissCallback = callback;
}

const char* CRenderer::EF_GetShaderMissLogPath()
{
    return m_cEF.m_ShaderCacheMissPath.c_str();
}

string* CRenderer::EF_GetShaderNames(int& nNumShaders)
{
    nNumShaders = m_cEF.m_ShaderNames.size();
    return nNumShaders ? &m_cEF.m_ShaderNames[0] : NULL;
}

IShader* CRenderer::EF_LoadShader([[maybe_unused]] const char* name, [[maybe_unused]] int flags, [[maybe_unused]] uint64 nMaskGen)
{
#ifdef NULL_RENDERER
    return m_cEF.s_DefaultShader;
#else
    return m_cEF.mfForName(name, flags, NULL, nMaskGen);
#endif
}

void CRenderer::EF_SetShaderQuality(EShaderType eST, EShaderQuality eSQ)
{
    m_pRT->RC_SetShaderQuality(eST, eSQ);

    if (gEnv->p3DEngine)
    {
        gEnv->p3DEngine->GetMaterialManager()->RefreshMaterialRuntime();
    }
}

uint64 CRenderer::EF_GetRemapedShaderMaskGen(const char* name, uint64 nMaskGen, bool bFixup)
{
    return m_cEF.mfGetRemapedShaderMaskGen(name, nMaskGen, bFixup);
}

uint64 CRenderer::EF_GetShaderGlobalMaskGenFromString(const char* szShaderName, const char* szShaderGen, uint64 nMaskGen)
{
    if (!m_cEF.mfUsesGlobalFlags(szShaderName))
    {
        return nMaskGen;
    }

    return m_cEF.mfGetShaderGlobalMaskGenFromString(szShaderGen);
}

// inverse of EF_GetShaderMaskGenFromString
AZStd::string CRenderer::EF_GetStringFromShaderGlobalMaskGen(const char* szShaderName, uint64 nMaskGen)
{
    if (!m_cEF.mfUsesGlobalFlags(szShaderName))
    {
        return "\0";
    }

    return m_cEF.mfGetShaderBitNamesFromGlobalMaskGen(nMaskGen);
}

SShaderItem CRenderer::EF_LoadShaderItem([[maybe_unused]] const char* szName, [[maybe_unused]] bool bShare, [[maybe_unused]] int flags, [[maybe_unused]] SInputShaderResources* Res, [[maybe_unused]] uint64 nMaskGen)
{
    LOADING_TIME_PROFILE_SECTION;

#ifdef NULL_RENDERER
    return m_cEF.s_DefaultShaderItem;
#else
    return m_cEF.mfShaderItemForName(szName, bShare, flags, Res, nMaskGen);
#endif
}

//////////////////////////////////////////////////////////////////////////
bool CRenderer::EF_ReloadFile_Request(const char* szFileName)
{
    // if its a source or destination texture, then post it into the queue to avoid render deadlocks:
    if (IResourceCompilerHelper::IsSourceImageFormatSupported(szFileName))
    {
        // Replace image extensions with .dds extensions.
        char realName[MAX_PATH + 1];
        azstrncpy(realName, MAX_PATH, szFileName, MAX_PATH);
        char* szExt = (char*)fpGetExtension(realName);
        azstrncpy(szExt, MAX_PATH - (szExt - realName), ".dds", 4);
        return CTexture::ReloadFile_Request(realName); // post in queue
    }
    else if (IResourceCompilerHelper::IsGameImageFormatSupported(szFileName))
    {
        return CTexture::ReloadFile_Request(szFileName); // post in queue
    }

    // the texture reader did not queue it, so try reloading it directly :
    return EF_ReloadFile(szFileName);
}

bool CRenderer::EF_ReloadFile(const char* szFileName)
{
    if (!szFileName)
    {
        return false; //might want to check this it hits
    }
    char realName[MAX_PATH + 1];
    azstrncpy(realName, MAX_PATH, szFileName, MAX_PATH);
    const char* szExt = fpGetExtension(realName);

    if (IResourceCompilerHelper::IsSourceImageFormatSupported(szExt) ||
        IResourceCompilerHelper::IsGameImageFormatSupported(szExt)) //note: it was also looking for .pcx... removed it
    {
        CRY_ASSERT_MESSAGE(false, "You must call EF_ReloadFile_Request for texture assets.");
    }
    else if (szExt && !azstricmp(szExt, ".cgf"))
    {
        IStatObj* pStatObjectToReload = (gEnv && gEnv->p3DEngine) ? gEnv->p3DEngine->FindStatObjectByFilename(realName) : nullptr;
        if (pStatObjectToReload)
        {
            pStatObjectToReload->Refresh(FRO_GEOMETRY | FRO_SHADERS | FRO_TEXTURES);
            return true;
        }
        return false;
    }
    else if (szExt && (!azstricmp(szExt, ".cfx") || (!CV_r_shadersignoreincludeschanging && !azstricmp(szExt, ".cfi"))))
    {
        gRenDev->m_cEF.m_Bin.InvalidateCache();
        // This is a temporary fix so that shaders would reload during hot update.
        bool bRet = gRenDev->m_cEF.mfReloadAllShaders(FRO_SHADERS, 0);
        if (gEnv && gEnv->p3DEngine)
        {
            gEnv->p3DEngine->UpdateShaderItems();
        }
        return bRet;
        //    return gRenDev->m_cEF.mfReloadFile(drn, nmf, FRO_SHADERS);
    }
#if defined(USE_GEOM_CACHES)
    else if (szExt && !azstricmp(szExt, ".cax"))
    {
        IGeomCache* pGeomCache = (gEnv && gEnv->p3DEngine) ? gEnv->p3DEngine->FindGeomCacheByFilename(realName) : nullptr;
        if (pGeomCache)
        {
            pGeomCache->Reload();
        }
    }
#endif //defined(USE_GEOM_CACHES)
    return false;
}

void CRenderer::EF_ReloadShaderFiles([[maybe_unused]] int nCategory)
{
    //gRenDev->m_cEF.mfLoadFromFiles(nCategory);
}

void CRenderer::EF_ReloadTextures()
{
    CTexture::ReloadTextures();
}

_smart_ptr<IImageFile> CRenderer::EF_LoadImage(const char* szFileName, uint32 nFlags)
{
    return CImageFile::mfLoad_file(szFileName, nFlags);
}

bool CRenderer::EF_RenderEnvironmentCubeHDR(int size, Vec3& Pos, TArray<unsigned short>& vecData)
{
    return CTexture::RenderEnvironmentCMHDR(size, Pos, vecData);
}

int CRenderer::EF_LoadLightmap(const char* name)
{
    CTexture* tp = (CTexture*)EF_LoadTexture(name, FT_DONT_STREAM | FT_STATE_CLAMP | FT_NOMIPS);
    if (tp->IsTextureLoaded())
    {
        return tp->GetID();
    }
    else
    {
        return -1;
    }
}

ITexture* CRenderer::EF_GetTextureByID(int Id)
{
    if (Id > 0)
    {
        CTexture* tp = CTexture::GetByID(Id);
        if (tp)
        {
            return tp;
        }
    }
    return NULL;
}

ITexture* CRenderer::EF_GetTextureByName(const char* nameTex, uint32 flags)
{
    if (nameTex)
    {
        INDENT_LOG_DURING_SCOPE(true, "While trying to find texture '%s' flags=0x%x...", nameTex, flags);

        const char* ext = fpGetExtension(nameTex);
        if (ext != 0 && (azstricmp(ext, ".tif") == 0 || azstricmp(ext, ".hdr") == 0 || azstricmp(ext, ".png") == 0))
        {
            // for compilable files, register by the dds file name (to not load it twice)
            char nameDDS[256];
            fpStripExtension(nameTex, nameDDS);
            cry_strcat(nameDDS, ".dds");

            return CTexture::GetByName(nameDDS, flags);
        }
        else
        {
            return CTexture::GetByName(nameTex, flags);
        }
    }

    return NULL;
}

ITexture* CRenderer::EF_LoadTexture(const char* nameTex, const uint32 flags)
{
    if (nameTex)
    {
        INDENT_LOG_DURING_SCOPE(true, "While trying to load texture '%s' flags=0x%x...", nameTex, flags);

        //if its a source image format try to load the dds
        const char* ext = fpGetExtension(nameTex);
        if (ext != 0 && (azstricmp(ext, ".tif") == 0 || azstricmp(ext, ".hdr") == 0 || azstricmp(ext, ".png") == 0))
        {
            // for compilable files, register by the dds file name (to not load it twice)
            char nameDDS[256];
            fpStripExtension(nameTex, nameDDS);
            cry_strcat(nameDDS, ".dds");

#if AZ_LOADSCREENCOMPONENT_ENABLED
            if (GetISystem() && GetISystem()->GetGlobalEnvironment() && GetISystem()->GetGlobalEnvironment()->mMainThreadId == CryGetCurrentThreadId())
            {
                EBUS_EVENT(LoadScreenBus, UpdateAndRender);
            }
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED
            return CTexture::ForName(nameDDS, flags, eTF_Unknown);
        }
        else
        {
#if AZ_LOADSCREENCOMPONENT_ENABLED
            if (GetISystem() && GetISystem()->GetGlobalEnvironment() && GetISystem()->GetGlobalEnvironment()->mMainThreadId == CryGetCurrentThreadId())
            {
                EBUS_EVENT(LoadScreenBus, UpdateAndRender);
            }
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED
            return CTexture::ForName(nameTex, flags, eTF_Unknown);
        }
    }

    return NULL;
}

ITexture* CRenderer::EF_LoadDefaultTexture(const char* nameTex)
{
    if (nameTex)
    {
        return CTextureManager::Instance()->GetDefaultTexture(nameTex);
    }
    return nullptr;
}

ITexture* CRenderer::EF_LoadCubemapTexture(const char* nameTex, const uint32 flags)
{
    ITexture* cubeMap = EF_LoadTexture(nameTex, flags);
    // Explicitly set the texture type for the texture for the cases where it is not loaded
    // at this time but the renderer still tries to bind it. On devices running Metal not doing this
    // causes the metal validation to fail as the texture uses black, a 2D texture, as a replacement
    // for an unloaded cubemap. Now the texture can choose to use black cubemap instead and make
    // the validation layer happy.
    if (cubeMap)
    {
        cubeMap->SetTextureType(eTT_Cube);
    }
    return cubeMap;
}

bool SShaderItem::Update()
{
    if (!(m_pShader->GetFlags() & EF_LOADED))
    {
        return false;
    }
    if ((uint32)m_nTechnique > 1000 && m_nTechnique != -1) // HACK HACK HACK
    {
        CCryNameTSCRC Name(m_nTechnique);
        if (!gRenDev->m_cEF.mfUpdateTechnik(*this, Name))
        {
            return false;
        }
    }

    uint32 nPreprocessFlags = PostLoad();

    // force the write to m_nPreprocessFlags to be last
    // to ensure the main thread has all correct data when the shaderitem is used
    // (m_nPreprocessFlags can indicate a not yet initialized shaderitem)
    MemoryBarrier();
    m_nPreprocessFlags = nPreprocessFlags;
    return true;
}

bool SShaderItem::RefreshResourceConstants()
{
    return gRenDev->m_cEF.mfRefreshResourceConstants(*this);
}
void CRenderer::EF_StartEf(const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_LEGACYONLY(GetISystem(), PROFILE_RENDERER);
    AZ_TRACE_METHOD();
    int i;
    ASSERT_IS_MAIN_THREAD(m_pRT)
    int nThreadID = passInfo.ThreadID();
    int nR = passInfo.GetRecursiveLevel();
    assert(nR < MAX_REND_RECURSION_LEVELS);
    if (nR == 0)
    {
        SRendItem::m_RecurseLevel[nThreadID] = -1;
        CRenderView::CurrentFillView()->ClearRenderItems();

        m_RP.m_TempObjects[nThreadID].resize(0);
        m_nShadowGenId[nThreadID] = 0;

        //SG frustums
        for (i = 0; i < MAX_SHADOWMAP_FRUSTUMS; i++)
        {
            SRendItem::m_ShadowsStartRI[nThreadID][i] = 0;
            SRendItem::m_ShadowsEndRI[nThreadID][i] = 0;
        }

        for (i = 0; i < (MAX_REND_LIGHTS + MAX_DEFERRED_LIGHTS); i++)
        {
            SRendItem::m_StartFrust[nThreadID][i] = 0;
            SRendItem::m_EndFrust[nThreadID][i] = 0;
        }

        // Clear all cached lists of shadow frustums
        for (ShadowFrustumListsCache::iterator it = m_FrustumsCache.begin(); it != m_FrustumsCache.end(); ++it)
        {
            if (it->second)
            {
                it->second->Clear();
            }
        }

        EF_RemovePolysFromScene();
        m_RP.m_fogVolumeContibutionsData[nThreadID].resize(0);
        //If we clear these flags during the recursion pass, it causes flickering and popping of objects.
        passInfo.GetRenderView()->PrepareForWriting();
    }

#ifndef _RELEASE
    if (nR >= MAX_REND_RECURSION_LEVELS)
    {
        CryLogAlways("nR (%d) >= MAX_REND_RECURSION_LEVELS (%d)\n", nR, MAX_REND_RECURSION_LEVELS);
        __debugbreak(); // otherwise about to go out of bounds in the loop below
    }
#endif

    m_RP.m_DeferredDecals[nThreadID][nR].clear();
    m_RP.m_isDeferrredNormalDecals[nThreadID][nR] = false;

    FurBendData::Get().OnBeginFrame();

    CPostEffectsMgr* pPostEffectMgr = PostEffectMgr();

    if (pPostEffectMgr)
    {
        pPostEffectMgr->OnBeginFrame();
    }

    SRendItem::m_RecurseLevel[nThreadID]++;

    EF_ClearLightsList();
    EF_ClearDeferredLightsList();
}

void CRenderer::RT_PostLevelLoading()
{
    int nThreadID = m_pRT->GetThreadList();
    m_RP.m_fogVolumeContibutionsData[nThreadID].reserve(2048);
   
    m_cEF.m_Bin.InvalidateCache();
    CHWShader::mfCleanupCache();
    CResFile::m_nMaxOpenResFiles = 4;
}

void CRenderer::RT_DisableTemporalEffects()
{
    m_nDisableTemporalEffects = GetActiveGPUCount();
}

void CRenderer::DrawStringU(IFFont_RenderProxy* pFont, float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx) const
{
    m_pRT->RC_DrawStringU(pFont, x, y, z, pStr, asciiMultiLine, ctx);
}

void CRenderer::RT_CreateREPostProcess(CRendElementBase** re)
{
    *re = new CREPostProcess;
}

IRenderElement* CRenderer::EF_CreateRE(EDataType edt)
{
    CRendElementBase* re = NULL;
    switch (edt)
    {
    case eDATA_Mesh:
        re = new CREMeshImpl;
        break;
    case eDATA_Imposter:
        re = new CREImposter;
        break;
    case eDATA_HDRProcess:
        re = new CREHDRProcess;
        break;

    case eDATA_DeferredShading:
        re = new CREDeferredShading;
        break;

    case eDATA_OcclusionQuery:
        re = new CREOcclusionQuery;
        break;

    case eDATA_LensOptics:
        re = new CRELensOptics;
        break;
    case eDATA_Cloud:
        re = new CRECloud;
        break;
    case eDATA_Sky:
        re = new CRESky;
        break;

    case eDATA_HDRSky:
        re = new CREHDRSky;
        break;

    case eDATA_Beam:
        re = new CREBeam;
        break;

    case eDATA_PostProcess:
        re = new CREPostProcess;
        break;

    case eDATA_FogVolume:
        re = new CREFogVolume;
        break;

    case eDATA_WaterVolume:
        re = new CREWaterVolume;
        break;

    case eDATA_WaterOcean:
        re = new CREWaterOcean;
        break;
    case eDATA_VolumeObject:
        re = new CREVolumeObject;
        break;
#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
    case eDATA_PrismObject:
        re = new CREPrismObject;
        break;
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

    case eDATA_GameEffect:
        re = new CREGameEffect;
        break;
#if defined(USE_GEOM_CACHES)
    case eDATA_GeomCache:
        re = new CREGeomCache;
        break;
#endif
    case eDATA_Gem:
        // For gems we return a base element which will be accessed through the IRenderElement interface.
        // The gem is expected to provide a delegate that implement the IRenderElementDelegate interface
        re = new CRendElementBase();
        break;
    }
    return re;
}

void CRenderer::EF_RemovePolysFromScene()
{
    ASSERT_IS_MAIN_THREAD(m_pRT)

    for (int i = 0; i < MAX_RECURSION_LEVELS; i++)
    {
        CREClientPoly::m_PolysStorage[m_RP.m_nFillThreadID][i].SetUse(0);
    }
    m_RP.m_SysVertexPool[m_RP.m_nFillThreadID].SetUse(0);
    m_RP.m_SysIndexPool[m_RP.m_nFillThreadID].SetUse(0);
}

CRenderObject* CRenderer::EF_AddPolygonToScene(SShaderItem& si, int numPts, const SVF_P3F_C4B_T2F* verts, const SPipTangents* tangs, CRenderObject* obj, const SRenderingPassInfo& passInfo, uint16* inds, int ninds, int nAW, const SRendItemSorter& rendItemSorter)
{
    ASSERT_IS_MAIN_THREAD(m_pRT)
    int nThreadID = m_RP.m_nFillThreadID;
    const uint32 nPersFlags = m_RP.m_TI[nThreadID].m_PersFlags;

    assert(si.m_pShader && si.m_pShaderResources);
    if (!si.m_pShader || !si.m_pShaderResources)
    {
        Warning("CRenderer::EF_AddPolygonToScene without shader...");
        return NULL;
    }
    if (si.m_nPreprocessFlags == -1)
    {
        if (!si.Update())
        {
            return obj;
        }
    }

    int recursiveLevel = SRendItem::m_RecurseLevel[nThreadID];
    if (recursiveLevel < 0)
    {
        return NULL;
    }

    int num = CREClientPoly::m_PolysStorage[nThreadID][recursiveLevel].Num();
    CREClientPoly::m_PolysStorage[nThreadID][recursiveLevel].GrowReset(1);

    CREClientPoly* pl = CREClientPoly::m_PolysStorage[nThreadID][recursiveLevel][num];
    if (!pl)
    {
        pl = new CREClientPoly;
        CREClientPoly::m_PolysStorage[nThreadID][recursiveLevel][num] = pl;
    }

    pl->m_Shader = si;
    pl->m_sNumVerts = numPts;
    pl->m_pObject = obj;
    pl->m_nCPFlags = 0;
    pl->rendItemSorter = rendItemSorter;
    if (nAW)
    {
        pl->m_nCPFlags |= CREClientPoly::efAfterWater;
    }
    if (passInfo.IsShadowPass())
    {
        pl->m_nCPFlags |= CREClientPoly::efShadowGen;
    }

    int nSize = AZ::Vertex::Format(eVF_P3F_C4B_T2F).GetStride() * numPts;
    int nOffs = m_RP.m_SysVertexPool[nThreadID].Num();
    SVF_P3F_C4B_T2F* vt = (SVF_P3F_C4B_T2F*)m_RP.m_SysVertexPool[nThreadID].GrowReset(nSize);
    pl->m_nOffsVert = nOffs;
    for (int i = 0; i < numPts; i++, vt++)
    {
        vt->xyz = verts[i].xyz;
        vt->st = verts[i].st;
        vt->color.dcolor = verts[i].color.dcolor;
    }
    if (tangs)
    {
        nSize = sizeof(SPipTangents) * numPts;
        nOffs = m_RP.m_SysVertexPool[nThreadID].Num();
        SPipTangents* t = (SPipTangents*)m_RP.m_SysVertexPool[nThreadID].GrowReset(nSize);
        pl->m_nOffsTang = nOffs;
        for (int i = 0; i < numPts; i++, t++)
        {
            *t = tangs[i];
        }
    }
    else
    {
        pl->m_nOffsTang = -1;
    }


    pl->m_nOffsInd = m_RP.m_SysIndexPool[nThreadID].Num();

    if (inds && ninds)
    {
        uint16* dstind = m_RP.m_SysIndexPool[nThreadID].Grow(ninds);
        memcpy(dstind, inds, ninds * sizeof(uint16));
        pl->m_sNumIndices = ninds;
    }
    else
    {
        uint16* dstind = m_RP.m_SysIndexPool[nThreadID].Grow((numPts - 2) * 3);
        for (int i = 0; i < numPts - 2; i++, dstind += 3)
        {
            dstind[0] = 0;
            dstind[1] = i + 1;
            dstind[2] = i + 2;
        }
        pl->m_sNumIndices = (numPts - 2) * 3;
    }

    return obj;
}

CRenderObject* CRenderer::EF_AddPolygonToScene(SShaderItem& si, CRenderObject* obj, const SRenderingPassInfo& passInfo, int numPts, int ninds, SVF_P3F_C4B_T2F*& verts, SPipTangents*& tangs, uint16*& inds, int nAW, [[maybe_unused]] const SRendItemSorter& rendItemSorter)
{
    ASSERT_IS_MAIN_THREAD(m_pRT)
    int nThreadID = m_RP.m_nFillThreadID;
    const uint32 nPersFlags = m_RP.m_TI[nThreadID].m_PersFlags;

    assert(si.m_pShader && si.m_pShaderResources);
    if (!si.m_pShader || !si.m_pShaderResources)
    {
        Warning("CRenderer::EF_AddPolygonToScene without shader...");
        return NULL;
    }
    if (si.m_nPreprocessFlags == -1)
    {
        if (!si.Update())
        {
            return obj;
        }
    }

    int recursiveLevel = SRendItem::m_RecurseLevel[nThreadID];
    assert(recursiveLevel >= 0);

    int num = CREClientPoly::m_PolysStorage[nThreadID][recursiveLevel].Num();
    CREClientPoly::m_PolysStorage[nThreadID][recursiveLevel].GrowReset(1);

    CREClientPoly* pl = CREClientPoly::m_PolysStorage[nThreadID][recursiveLevel][num];
    if (!pl)
    {
        pl = new CREClientPoly;
        CREClientPoly::m_PolysStorage[nThreadID][recursiveLevel][num] = pl;
    }

    pl->m_Shader = si;
    pl->m_pObject = obj;
    if (nAW)
    {
        pl->m_nCPFlags |= CREClientPoly::efAfterWater;
    }
    if (passInfo.IsShadowPass())
    {
        pl->m_nCPFlags |= CREClientPoly::efShadowGen;
    }

    //////////////////////////////////////////////////////////////////////////
    // allocate buffer space for caller to fill

    pl->m_sNumVerts = numPts;
    pl->m_nOffsVert = m_RP.m_SysVertexPool[nThreadID].Num();
    pl->m_nOffsTang = m_RP.m_SysVertexPool[nThreadID].Num() + sizeof(SVF_P3F_C4B_T2F) * numPts;
    m_RP.m_SysVertexPool[nThreadID].GrowReset((sizeof(SVF_P3F_C4B_T2F) + sizeof(SPipTangents)) * numPts);
    verts = (SVF_P3F_C4B_T2F*)&m_RP.m_SysVertexPool[nThreadID][pl->m_nOffsVert];
    tangs = (SPipTangents*)&m_RP.m_SysVertexPool[nThreadID][pl->m_nOffsTang];

    pl->m_sNumIndices = ninds;
    pl->m_nOffsInd = m_RP.m_SysIndexPool[nThreadID].Num();
    inds = m_RP.m_SysIndexPool[nThreadID].Grow(ninds);

    //////////////////////////////////////////////////////////////////////////

    return obj;
}

void CRenderer::EF_AddClientPolys([[maybe_unused]] const SRenderingPassInfo& passInfo)
{
#if !defined(NULL_RENDERER)
    AZ_TRACE_METHOD();
    uint32 i;
    CREClientPoly* pl;

    ASSERT_IS_MAIN_THREAD(m_pRT)
    int nThreadID = m_pRT->GetThreadList();
    int recursiveLevel = SRendItem::m_RecurseLevel[nThreadID];
    assert(recursiveLevel >= 0);

    const SThreadInfo& rTI = m_RP.m_TI[nThreadID];
    const uint32 nPersFlags = rTI.m_PersFlags;

    for (i = 0; i < CREClientPoly::m_PolysStorage[nThreadID][recursiveLevel].Num(); i++)
    {
        pl = CREClientPoly::m_PolysStorage[nThreadID][recursiveLevel][i];

        CShader* pShader = (CShader*)pl->m_Shader.m_pShader;
        CShaderResources* const __restrict pShaderResources = static_cast<CShaderResources*>(pl->m_Shader.m_pShaderResources);
        SShaderTechnique* pTech = pl->m_Shader.GetTechnique();

        uint32 nBatchFlags = FB_GENERAL;

        if (pl->m_Shader.m_nPreprocessFlags & FSPR_MASK)
        {
            passInfo.GetRenderView()->AddRenderItem(pl, pl->m_pObject, pl->m_Shader, EFSLIST_PREPROCESS, 0, FB_GENERAL, passInfo, pl->rendItemSorter);
        }

        if (pShader->GetFlags() & EF_DECAL) // || pl->m_pObject && (pl->m_pObject->m_ObjFlags & FOB_DECAL))
        {
            if (pTech && pTech->m_nTechnique[TTYPE_Z] > 0 && (pShader && (pShader->m_Flags & EF_SUPPORTSDEFERREDSHADING)))
            {
                nBatchFlags |= FB_Z;
            }

            if (!passInfo.IsShadowPass() && !(pl->m_nCPFlags & CREClientPoly::efShadowGen))
            {
                passInfo.GetRenderView()->AddRenderItem(pl, pl->m_pObject, pl->m_Shader, EFSLIST_DECAL, pl->m_nCPFlags & CREClientPoly::efAfterWater, nBatchFlags, passInfo, pl->rendItemSorter);
            }
            else if (passInfo.IsShadowPass() && (pl->m_nCPFlags & CREClientPoly::efShadowGen))
            {
                passInfo.GetRenderView()->AddRenderItem(pl, pl->m_pObject, pl->m_Shader, EFSLIST_SHADOW_GEN, SG_SORT_GROUP, FB_GENERAL, passInfo, pl->rendItemSorter);
            }
        }
        else
        {
            uint32 list = EFSLIST_GENERAL;
            if (pl->m_pObject->m_fAlpha < 1.0f || (pShaderResources && pShaderResources->IsTransparent()))
            {
                list = EFSLIST_TRANSP;
            }
            nBatchFlags |= FB_TRANSPARENT;
            passInfo.GetRenderView()->AddRenderItem(pl, pl->m_pObject, pl->m_Shader, list, pl->m_nCPFlags & CREClientPoly::efAfterWater, nBatchFlags, passInfo, pl->rendItemSorter);
        }
    }
#endif
}


// Dynamic lights
bool CRenderer::EF_IsFakeDLight(const CDLight* Source)
{
    if (!Source)
    {
        iLog->Log("Warning: EF_IsFakeDLight: NULL light source\n");
        return true;
    }

    bool bIgnore = false;
    if (Source->m_Flags & (DLF_FAKE))
    {
        bIgnore = true;
    }

    return bIgnore;
}

void CRenderer::EF_CheckLightMaterial(CDLight* pLight, uint16 nRenderLightID, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter)
{
    ASSERT_IS_MAIN_THREAD(m_pRT);
    const int32 nThreadID = m_RP.m_nFillThreadID;
    const int32 recursiveLevel = SRendItem::m_RecurseLevel[nThreadID];
    assert(recursiveLevel >= 0);

    if (!(m_RP.m_TI[nThreadID].m_PersFlags & RBPF_IMPOSTERGEN))
    {
        // Add render element if light has mtl bound
        IShader* pShader = pLight->m_Shader.m_pShader;
        TArray<CRendElementBase*>* pRendElemBase = pShader ? pLight->m_Shader.m_pShader->GetREs(pLight->m_Shader.m_nTechnique) : 0;
        if (pRendElemBase && !pRendElemBase->empty())
        {
            pLight->m_pObject[recursiveLevel] = EF_GetObject_Temp(passInfo.ThreadID());
            pLight->m_pObject[recursiveLevel]->m_fAlpha = 1.0f;
            pLight->m_pObject[recursiveLevel]->m_II.m_AmbColor = Vec3(0, 0, 0);

            SRenderObjData* pOD = EF_GetObjData(pLight->m_pObject[recursiveLevel], true, passInfo.ThreadID());

            pOD->m_fTempVars[0] = 0;
            pOD->m_fTempVars[1] = 0;
            pOD->m_fTempVars[3] = pLight->m_fRadius;
            pOD->m_nLightID = nRenderLightID;

            pLight->m_pObject[recursiveLevel]->m_II.m_AmbColor = pLight->m_Color;
            pLight->m_pObject[recursiveLevel]->m_II.m_Matrix = pLight->m_ObjMatrix;

            CRendElementBase* pRE = pRendElemBase->Get(0);
            const int32 nList = (pRE->mfGetType() != eDATA_LensOptics) ? EFSLIST_TRANSP : EFSLIST_LENSOPTICS;

            if (pRE->mfGetType() == eDATA_Beam)
            {
                pLight->m_Flags |= DLF_LIGHT_BEAM;
            }

            int32 nAW;
            if (OceanToggle::IsActive() && !OceanRequest::OceanIsEnabled())
            {
                nAW = 1;
            }
            else
            {
                const float fWaterLevel = OceanToggle::IsActive() ? OceanRequest::GetOceanLevel() : gEnv->p3DEngine->GetWaterLevel();
                const float fCamZ = m_RP.m_TI[nThreadID].m_cam.GetPosition().z;
                nAW = ((fCamZ - fWaterLevel) * (pLight->m_Origin.z - fWaterLevel) > 0) ? 1 : 0;
            }

            EF_AddEf(pRE, pLight->m_Shader, pLight->m_pObject[recursiveLevel], passInfo, nList, nAW, rendItemSorter);
        }
    }
}

void CRenderer::EF_ADDDlight(CDLight* Source, [[maybe_unused]] const SRenderingPassInfo& passInfo)
{
    if (!Source)
    {
        iLog->Log("Warning: EF_ADDDlight: NULL light source\n");
        return;
    }

    ASSERT_IS_MAIN_THREAD(m_pRT)

    bool bIgnore = EF_IsFakeDLight(Source);

    int nThreadID = m_RP.m_nFillThreadID;
    int recursiveLevel = SRendItem::m_RecurseLevel[nThreadID];
    assert(recursiveLevel >= 0);

    SRenderLight* pNew = NULL;
    if (bIgnore)
    {
        Source->m_Id = -1;
    }
    else
    {
        assert((Source->m_Flags & DLF_LIGHTTYPE_MASK) != 0);
        Source->m_Id = (int16)m_RP.m_DLights[nThreadID][recursiveLevel].Num();
        if (Source->m_Id >= 32)
        {
            Source->m_Id = -1;
            return;
        }
        pNew = m_RP.m_DLights[nThreadID][recursiveLevel].AddIndex(1);
        memcpy(pNew, Source, sizeof(SRenderLight));
    }
    EF_PrecacheResource(Source, (m_RP.m_TI[nThreadID].m_cam.GetPosition() - Source->m_Origin).GetLengthSquared() / max(0.001f, Source->m_fRadius * Source->m_fRadius), 0.1f, 0, 0);
}

bool CRenderer::EF_AddDeferredDecal(const SDeferredDecal& rDecal)
{
    ASSERT_IS_MAIN_THREAD(m_pRT)

    int nThreadID = m_RP.m_nFillThreadID;
    int recursiveLevel = SRendItem::m_RecurseLevel[nThreadID];
    assert(recursiveLevel >= 0);
    if (recursiveLevel < 0)
    {
        iLog->Log("Warning: CRenderer::EF_AddDeferredDecal: decal adding before calling EF_StartEf");
        return false;
    }

    if (m_RP.m_DeferredDecals[nThreadID][recursiveLevel].size() < 1024)
    {
        m_RP.m_DeferredDecals[nThreadID][recursiveLevel].push_back(rDecal);
        int nLastElem = m_RP.m_DeferredDecals[nThreadID][recursiveLevel].size() - 1;

        SDeferredDecal& rDecalCopy = m_RP.m_DeferredDecals[nThreadID][recursiveLevel].at(nLastElem);

        //////////////////////////////////////////////////////////////////////////
        _smart_ptr<IMaterial> pDecalMaterial = rDecalCopy.pMaterial;
        if (pDecalMaterial == NULL)
        {
            AZ_WarningOnce("Renderer", pDecalMaterial == NULL, "Decal missing material.");
            return false;
        }

        SShaderItem& sItem = pDecalMaterial->GetShaderItem(0);
        if (sItem.m_pShaderResources == NULL)
        {
            assert(0);
            return false;
        }

        if (SEfResTexture* pNormalRes0 = sItem.m_pShaderResources->GetTextureResource((uint16)EFTT_NORMALS))
        {
            if (pNormalRes0->m_Sampler.m_pITex)
            {
                rDecalCopy.nFlags |= DECAL_HAS_NORMAL_MAP;
                m_RP.m_isDeferrredNormalDecals[nThreadID][recursiveLevel] = true;
            }
            else
            {
                rDecalCopy.nFlags &= ~DECAL_HAS_NORMAL_MAP;
            }
        }

        if (SEfResTexture* pSpecularRes0 = sItem.m_pShaderResources->GetTextureResource((uint16)EFTT_SPECULAR))
        {
            if (pSpecularRes0->m_Sampler.m_pITex)
            {
                rDecalCopy.nFlags |= DECAL_HAS_SPECULAR_MAP;
            }
            else
            {
                rDecalCopy.nFlags &= ~DECAL_HAS_SPECULAR_MAP;
            }
        }
        //////////////////////////////////////////////////////////////////////////

        if IsCVarConstAccess(constexpr) (CV_r_deferredDecalsDebug)
        {
            Vec3 vCenter = rDecalCopy.projMatrix.GetTranslation();
            float fSize = rDecalCopy.projMatrix.GetColumn(2).GetLength();
            Vec3 vSize(fSize, fSize, fSize);
            AABB aabbCenter(vCenter - vSize * 0.05f, vCenter + vSize * 0.05f);
            GetIRenderAuxGeom()->DrawAABB(aabbCenter, false, Col_Yellow, eBBD_Faceted);
            GetIRenderAuxGeom()->DrawLine(vCenter, Col_Red, vCenter + rDecalCopy.projMatrix.GetColumn(0), Col_Red);
            GetIRenderAuxGeom()->DrawLine(vCenter, Col_Green, vCenter + rDecalCopy.projMatrix.GetColumn(1), Col_Green);
            GetIRenderAuxGeom()->DrawLine(vCenter, Col_Blue, vCenter + rDecalCopy.projMatrix.GetColumn(2), Col_Blue);
        }

        return true;
    }
    return false;
}

void CRenderer::EF_ClearLightsList()
{
    ASSERT_IS_MAIN_THREAD(m_pRT)
    assert(SRendItem::m_RecurseLevel[m_RP.m_nFillThreadID] >= 0);
    m_RP.m_DLights[m_RP.m_nFillThreadID][SRendItem::m_RecurseLevel[m_RP.m_nFillThreadID]].SetUse(0);
    m_RP.m_SMFrustums[m_RP.m_nFillThreadID][SRendItem::m_RecurseLevel[m_RP.m_nFillThreadID]].SetUse(0);
    m_RP.m_SMCustomFrustumIDs[m_RP.m_nFillThreadID][SRendItem::m_RecurseLevel[m_RP.m_nFillThreadID]].SetUse(0);

    if (SRendItem::m_RecurseLevel[m_RP.m_nFillThreadID] == 0)
    {
        m_RP.m_arrCustomShadowMapFrustumData[m_RP.m_nFillThreadID].resize(0);
    }
}

inline Matrix44 ToLightMatrix(const Ang3& angle)
{
    Matrix33 ViewMatZ = Matrix33::CreateRotationZ(-angle.x);
    Matrix33 ViewMatX = Matrix33::CreateRotationX(-angle.y);
    Matrix33 ViewMatY = Matrix33::CreateRotationY(+angle.z);
    return Matrix44(ViewMatX * ViewMatY * ViewMatZ).GetTransposed();
}

bool CRenderer::EF_UpdateDLight(SRenderLight* dl)
{
    if (!dl)
    {
        return false;
    }

    float fTime = iTimer->GetCurrTime() * dl->GetAnimSpeed();

    const uint32 nStyle = dl->m_nLightStyle;

    const IAnimNode* pLightAnimNode = 0;

    ILightAnimWrapper* pLightAnimWrapper = dl->m_pLightAnim;
    IF (pLightAnimWrapper, 0)
    {
        pLightAnimNode = pLightAnimWrapper->GetNode();
        IF (!pLightAnimNode, 0)
        {
            pLightAnimWrapper->Resolve();
            pLightAnimNode = pLightAnimWrapper->GetNode();
        }
    }

    IF (pLightAnimNode, 0)
    {
        //TODO: This may require further optimizations.
        IAnimTrack* pPosTrack = pLightAnimNode->GetTrackForParameter(AnimParamType::Position);
        IAnimTrack* pRotTrack = pLightAnimNode->GetTrackForParameter(AnimParamType::Rotation);
        IAnimTrack* pColorTrack = pLightAnimNode->GetTrackForParameter(AnimParamType::LightDiffuse);
        IAnimTrack* pDiffMultTrack = pLightAnimNode->GetTrackForParameter(AnimParamType::LightDiffuseMult);
        IAnimTrack* pRadiusTrack = pLightAnimNode->GetTrackForParameter(AnimParamType::LightRadius);
        IAnimTrack* pSpecMultTrack = pLightAnimNode->GetTrackForParameter(AnimParamType::LightSpecularMult);
        IAnimTrack* pHDRDynamicTrack = pLightAnimNode->GetTrackForParameter(AnimParamType::LightHDRDynamic);

        Range timeRange = const_cast<IAnimNode*>(pLightAnimNode)->GetSequence()->GetTimeRange();
        float time = (dl->m_Flags & DLF_TRACKVIEW_TIMESCRUBBING) ? dl->m_fTimeScrubbed : fTime;
        float phase = static_cast<float>(dl->m_nLightPhase) / 100.0f;

        if (pPosTrack && pPosTrack->GetNumKeys() > 0 &&
            !(pPosTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled))
        {
            Vec3 vOffset(0, 0, 0);
            float duration = max(pPosTrack->GetKeyTime(pPosTrack->GetNumKeys() - 1), 0.001f);
            float timeNormalized = static_cast<float>(fmod(time + phase * duration, duration));
            pPosTrack->GetValue(timeNormalized, vOffset);
            dl->m_Origin = dl->m_BaseOrigin + vOffset;
        }

        if (pRotTrack && pRotTrack->GetNumKeys() > 0 &&
            !(pRotTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled))
        {
            Vec3 vRot(0, 0, 0);
            float duration = max(pRotTrack->GetKeyTime(pRotTrack->GetNumKeys() - 1), 0.001f);
            float timeNormalized = static_cast<float>(fmod(time + phase * duration, duration));
            pRotTrack->GetValue(timeNormalized, vRot);
            static_cast<CDLight*>(dl)->SetMatrix(
                dl->m_BaseObjMatrix * Matrix34::CreateRotationXYZ(Ang3(DEG2RAD(vRot.x), DEG2RAD(vRot.y), DEG2RAD(vRot.z))),
                false);
        }

        if (pColorTrack && pColorTrack->GetNumKeys() > 0 &&
            !(pColorTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled))
        {
            Vec3 vColor(dl->m_Color.r, dl->m_Color.g, dl->m_Color.b);
            float duration = max(pColorTrack->GetKeyTime(pColorTrack->GetNumKeys() - 1), 0.001f);
            float timeNormalized = static_cast<float>(fmod(time + phase * duration, duration));
            pColorTrack->GetValue(timeNormalized, vColor);
            dl->m_Color = ColorF(vColor.x / 255.0f, vColor.y / 255.0f, vColor.z / 255.0f);
        }
        else
        {
            dl->m_Color = dl->m_BaseColor;
        }

        if (pDiffMultTrack && pDiffMultTrack->GetNumKeys() > 0 &&
            !(pDiffMultTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled))
        {
            float diffMult = 1.0;
            float duration = max(pDiffMultTrack->GetKeyTime(pDiffMultTrack->GetNumKeys() - 1), 0.001f);
            float timeNormalized = static_cast<float>(fmod(time + phase * duration, duration));
            pDiffMultTrack->GetValue(timeNormalized, diffMult);
            dl->m_Color *= diffMult;
        }

        if (pRadiusTrack && pRadiusTrack->GetNumKeys() > 0 &&
            !(pRadiusTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled))
        {
            float radius = dl->m_fRadius;
            float duration = max(pRadiusTrack->GetKeyTime(pRadiusTrack->GetNumKeys() - 1), 0.001f);
            float timeNormalized = static_cast<float>(fmod(time + phase * duration, duration));
            pRadiusTrack->GetValue(timeNormalized, radius);
            dl->m_fRadius = radius;
        }

        if (pSpecMultTrack && pSpecMultTrack->GetNumKeys() > 0 &&
            !(pSpecMultTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled))
        {
            float specMult = dl->m_SpecMult;
            float duration = max(pSpecMultTrack->GetKeyTime(pSpecMultTrack->GetNumKeys() - 1), 0.001f);
            float timeNormalized = static_cast<float>(fmod(time + phase * duration, duration));
            pSpecMultTrack->GetValue(timeNormalized, specMult);
            dl->m_SpecMult = specMult;
        }

        if (pHDRDynamicTrack && pHDRDynamicTrack->GetNumKeys() > 0 &&
            !(pHDRDynamicTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled))
        {
            float hdrDynamic = dl->m_fHDRDynamic;
            float duration = max(pHDRDynamicTrack->GetKeyTime(pHDRDynamicTrack->GetNumKeys() - 1), 0.001f);
            float timeNormalized = static_cast<float>(fmod(time + phase * duration, duration));
            pHDRDynamicTrack->GetValue(timeNormalized, hdrDynamic);
            dl->m_fHDRDynamic = hdrDynamic;
        }
    }
    else if (nStyle > 0 && nStyle < CLightStyle::s_LStyles.Num() && CLightStyle::s_LStyles[nStyle])
    {
        CLightStyle* ls = CLightStyle::s_LStyles[nStyle];

        const float fRecipMaxInt8 = 1.0f / 255.0f;

        // Add user light phase
        float fPhaseFromID = ((float)dl->m_nLightPhase) * fRecipMaxInt8;
        fTime += (fPhaseFromID - floorf(fPhaseFromID)) * ls->m_TimeIncr;

        ls->mfUpdate(fTime);

        // It's possible the use of m_Color could be broken if ls->m_Color.a is being used for blending/fading, 
        // because we moved probe blending out of the alpha channel into m_fProbeAttenuation. This code wasn't tested 
        // after those changes as it only appears to be used in legacy code paths (light animation via old ScriptBind
        // system and/or legacy LensFlareComponent). (9/20/2017, see LY-64767)
        dl->m_Color = dl->m_BaseColor * ls->m_Color;
        dl->m_SpecMult = dl->m_BaseSpecMult * ls->m_Color.a;
        dl->m_Origin = dl->m_BaseOrigin + ls->m_vPosOffset;
    }
    else
    {
        dl->m_Color = dl->m_BaseColor;
    }

    return false;
}

void CRenderer::FX_ApplyShaderQuality(const EShaderType eST)
{
    SShaderProfile* const pSP = &m_cEF.m_ShaderProfiles[eST];
    const uint64 quality = g_HWSR_MaskBit[HWSR_QUALITY];
    const uint64 quality1 = g_HWSR_MaskBit[HWSR_QUALITY1];
    m_RP.m_FlagsShader_RT &= ~(quality | quality1);
    int nQuality = (int)pSP->GetShaderQuality();
    m_RP.m_nShaderQuality = nQuality;
    switch (nQuality)
    {
    case eSQ_Medium:
        m_RP.m_FlagsShader_RT |= quality;
        break;
    case eSQ_High:
        m_RP.m_FlagsShader_RT |= quality1;
        break;
    case eSQ_VeryHigh:
        m_RP.m_FlagsShader_RT |= (quality | quality1);
        break;
    }
}

EShaderQuality CRenderer::EF_GetShaderQuality(EShaderType eST)
{
    SShaderProfile* pSP = &m_cEF.m_ShaderProfiles[eST];
    int nQuality = (int)pSP->GetShaderQuality();

    switch (nQuality)
    {
    case eSQ_Low:
        return eSQ_Low;
    case eSQ_Medium:
        return eSQ_Medium;
    case eSQ_High:
        return eSQ_High;
    case eSQ_VeryHigh:
        return eSQ_VeryHigh;
    }

    return eSQ_Low;
}

ERenderQuality CRenderer::EF_GetRenderQuality() const
{
    return (ERenderQuality)m_RP.m_eQuality;
}

int CRenderer::RT_CurThreadList()
{
    return m_pRT->GetThreadList();
}

namespace {
    // util funtion to write to the provided output memory
    template<typename T>
    void WriteQueryResult(void* pOutput, [[maybe_unused]] uint32 nOutputSize, const T& rQueryResult)
    {
#if !defined(_RELEASE)
        if (pOutput == NULL)
        {
            CryFatalError("No Output Storage Specified");
        }
        if (sizeof(T) != nOutputSize)
        {
            CryFatalError("Insufficient storage for EF_Query Output");
        }
#endif
        *alias_cast<T*>(pOutput) = rQueryResult;
    }

    // util function to read POD types from query parameters
    template<typename T>
    T ReadQueryParameter(void* pInput, [[maybe_unused]] uint32 nInputSize)
    {
#if !defined(_RELEASE)
        if (pInput == NULL)
        {
            CryFatalError("No Input Storage Specified");
        }
        if (sizeof(T) != nInputSize)
        {
            CryFatalError("Insufficient storage for EF_Query Input");
        }
#endif
        return *alias_cast<T*>(pInput);
    }
}

void CRenderer::EF_QueryImpl(ERenderQueryTypes eQuery, void* pInOut0, uint32 nInOutSize0, void* pInOut1, uint32 nInOutSize1)
{
    switch (eQuery)
    {
    case EFQ_DeleteMemoryArrayPtr:
    {
        char* pPtr = ReadQueryParameter<char*>(pInOut0, nInOutSize0);
        delete[] pPtr;
    }
    break;

    case EFQ_DeleteMemoryPtr:
    {
        char* pPtr = ReadQueryParameter<char*>(pInOut0, nInOutSize0);
        delete pPtr;
    }
    break;

    case EFQ_LightSource:
    {
        assert(SRendItem::m_RecurseLevel[m_RP.m_nFillThreadID] >= 0);
        uint16 nLightId = ReadQueryParameter<uint16>(pInOut0, nInOutSize0);
        if (m_RP.m_DLights[m_RP.m_nFillThreadID][SRendItem::m_RecurseLevel[m_RP.m_nFillThreadID]].Num() > nLightId)
        {
            WriteQueryResult(pInOut1, nInOutSize1, &m_RP.m_DLights[m_RP.m_nFillThreadID][SRendItem::m_RecurseLevel[m_RP.m_nFillThreadID]][nLightId]);
        }
    }
    break;

    case EFQ_MainThreadList:
    {
        WriteQueryResult(pInOut0, nInOutSize0, m_RP.m_nFillThreadID);
    }
    break;

    case EFQ_RenderThreadList:
    {
        WriteQueryResult(pInOut0, nInOutSize0, m_RP.m_nProcessThreadID);
    }
    break;

    case EFQ_RenderMultithreaded:
    {
        WriteQueryResult(pInOut0, nInOutSize0, static_cast<bool>(m_pRT->IsMultithreaded()));
    }
    break;

    case EFQ_IncrementFrameID:
    {
        m_RP.m_TI[m_pRT->GetThreadList()].m_nFrameID += 1;
    }
    break;

    case EFQ_DeviceLost:
    {
        WriteQueryResult(pInOut0, nInOutSize0, static_cast<bool>(m_bDeviceLost != 0));
    }
    break;

    case EFQ_RecurseLevel:
    {
        WriteQueryResult(pInOut0, nInOutSize0, SRendItem::m_RecurseLevel[m_pRT->GetThreadList()]);
    }
    break;

    case EFQ_Alloc_APITextures:
    {
        int nSize = 0;
        SResourceContainer* pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
        if (pRL)
        {
            ResourcesMapItor itor;
            for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
            {
                CTexture* tp = (CTexture*)itor->second;
                if (!tp || tp->IsNoTexture())
                {
                    continue;
                }
                if (!(tp->GetFlags() & (FT_USAGE_DYNAMIC | FT_USAGE_RENDERTARGET)))
                {
                    nSize += tp->GetDeviceDataSize();
                }
            }
        }
        WriteQueryResult(pInOut0, nInOutSize0, nSize);
    }
    break;

    case EFQ_Alloc_APIMesh:
    {
        uint32 nSize = 0;
        for (util::list<CRenderMesh>* iter = CRenderMesh::m_MeshList.next; iter != &CRenderMesh::m_MeshList; iter = iter->next)
        {
            CRenderMesh* pRM = iter->item<&CRenderMesh::m_Chain>();
            nSize += static_cast<uint32>(pRM->Size(CRenderMesh::SIZE_VB));
            nSize += static_cast<uint32>(pRM->Size(CRenderMesh::SIZE_IB));
        }
        WriteQueryResult(pInOut0, nInOutSize0, nSize);
    }
    break;

    case EFQ_Alloc_Mesh_SysMem:
    {
        uint32 nSize = 0;
        for (util::list<CRenderMesh>* iter = CRenderMesh::m_MeshList.next; iter != &CRenderMesh::m_MeshList; iter = iter->next)
        {
            CRenderMesh* pRM = iter->item<&CRenderMesh::m_Chain>();
            nSize += static_cast<uint32>(pRM->Size(CRenderMesh::SIZE_ONLY_SYSTEM));
        }
        WriteQueryResult(pInOut0, nInOutSize0, nSize);
    }
    break;

    case EFQ_Mesh_Count:
    {
        uint32 nCount = 0;
        AUTO_LOCK(CRenderMesh::m_sLinkLock);
        for (util::list<CRenderMesh>* iter = CRenderMesh::m_MeshList.next; iter != &CRenderMesh::m_MeshList; iter = iter->next)
        {
            ++nCount;
        }
        WriteQueryResult(pInOut0, nInOutSize0, nCount);
    }
    break;

    case EFQ_GetAllMeshes:
    {
        //Get render mesh lock, to ensure that the mesh list doesn't change while we're copying
        AUTO_LOCK(CRenderMesh::m_sLinkLock);
        IRenderMesh** ppMeshes = NULL;
        uint32 nSize = 0;
        for (util::list<CRenderMesh>* iter = CRenderMesh::m_MeshList.next; iter != &CRenderMesh::m_MeshList; iter = iter->next)
        {
            ++nSize;
        }
        if (pInOut0 && nSize)
        {
            //allocate the array. The calling function is responsible for cleaning it up.
            ppMeshes = new IRenderMesh*[nSize];
            nSize = 0;
            for (util::list<CRenderMesh>* iter = CRenderMesh::m_MeshList.next; iter != &CRenderMesh::m_MeshList; iter = iter->next)
            {
                ppMeshes[nSize] = iter->item<&CRenderMesh::m_Chain>();
                ;
                nSize++;
            }
        }
        WriteQueryResult(pInOut0, nInOutSize0, ppMeshes);
        WriteQueryResult(pInOut1, nInOutSize1, nSize);
    }
    break;

    case EFQ_GetAllTextures:
    {
        AUTO_LOCK(CBaseResource::s_cResLock);

        SRendererQueryGetAllTexturesParam* pParam = (SRendererQueryGetAllTexturesParam*)(pInOut0);
        pParam->pTextures = NULL;
        pParam->numTextures = 0;

        SResourceContainer* pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
        if (pRL)
        {
            for (ResourcesMapItor itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
            {
                CTexture* tp = (CTexture*)itor->second;
                if (!tp || tp->IsNoTexture())
                {
                    continue;
                }
                ++pParam->numTextures;
            }

            if (pParam->numTextures > 0)
            {
                pParam->pTextures = new _smart_ptr<ITexture>[pParam->numTextures];

                uint32 texIdx = 0;
                for (ResourcesMapItor itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
                {
                    CTexture* tp = (CTexture*)itor->second;
                    if (!tp || tp->IsNoTexture())
                    {
                        continue;
                    }
                    pParam->pTextures[texIdx++] = tp;
                }
            }
        }
    }
    break;

    case EFQ_GetAllTexturesRelease:
    {
        SRendererQueryGetAllTexturesParam* pParam = (SRendererQueryGetAllTexturesParam*)(pInOut0);
        SAFE_DELETE_ARRAY(pParam->pTextures);
    }
    break;

    case EFQ_TexturesPoolSize:
    {
        uint32 streamPoolSize = (uint32)(CRenderer::GetTexturesStreamPoolSize() * 1024 * 1024);
        WriteQueryResult(pInOut0, nInOutSize0, streamPoolSize);
    }
    break;

    case EFQ_RenderTargetPoolSize:
    {
        WriteQueryResult(pInOut0, nInOutSize0, (CRenderer::CV_r_rendertargetpoolsize + 2) * 1024 * 1024);
    }
    break;

    case EFQ_HDRModeEnabled:
    {
        WriteQueryResult(pInOut0, nInOutSize0, static_cast<bool>(IsHDRModeEnabled() ? 1 : 0));
    }
    break;

    case EFQ_ParticlesTessellation:
    {
#if defined(PARTICLES_TESSELLATION_RENDERER)
        WriteQueryResult<bool>(pInOut0, nInOutSize0, m_bDeviceSupportsTessellation && CV_r_ParticlesTessellation != 0);
#else
        WriteQueryResult<bool>(pInOut0, nInOutSize0, false);
#endif
    }
    break;

    case EFQ_WaterTessellation:
    {
#if defined(WATER_TESSELLATION_RENDERER)
        WriteQueryResult<bool>(pInOut0, nInOutSize0, m_bDeviceSupportsTessellation && CV_r_WaterTessellationHW != 0);
#else
        WriteQueryResult<bool>(pInOut0, nInOutSize0, false);
#endif
    }
    break;

    case EFQ_MeshTessellation:
    {
#if defined(MESH_TESSELLATION_RENDERER)
        WriteQueryResult<bool>(pInOut0, nInOutSize0, m_bDeviceSupportsTessellation);
#else
        WriteQueryResult<bool>(pInOut0, nInOutSize0, false);
#endif
    }
    break;

#ifndef _RELEASE
    case EFQ_GetShadowPoolFrustumsNum:
    {
        WriteQueryResult(pInOut0, nInOutSize0, m_RP.m_PS[m_RP.m_nFillThreadID].m_NumShadowPoolFrustums);
    }
    break;

    case EFQ_GetShadowPoolAllocThisFrameNum:
    {
        WriteQueryResult(pInOut0, nInOutSize0, m_RP.m_PS[m_RP.m_nFillThreadID].m_NumShadowPoolAllocsThisFrame);
    }
    break;

    case EFQ_GetShadowMaskChannelsNum:
    {
        WriteQueryResult(pInOut0, nInOutSize0, m_RP.m_PS[m_RP.m_nFillThreadID].m_NumShadowMaskChannels);
    }
    break;

    case EFQ_GetTiledShadingSkippedLightsNum:
    {
        WriteQueryResult(pInOut0, nInOutSize0, m_RP.m_PS[m_RP.m_nFillThreadID].m_NumTiledShadingSkippedLights);
    }
    break;
#endif

    case EFQ_MultiGPUEnabled:
    {
        WriteQueryResult(pInOut0, nInOutSize0, static_cast<bool>(GetActiveGPUCount() > 1 ? 1 : 0));
    }
    break;

    // deprecated, always enabled
    case EFQ_sLinearSpaceShadingEnabled:
    {
        WriteQueryResult(pInOut0, nInOutSize0, static_cast<bool>(1));
    }
    break;

    case EFQ_SetDrawNearFov:
    {
        CV_r_drawnearfov = ReadQueryParameter<float>(pInOut0, nInOutSize0);
    }
    break;

    case EFQ_GetDrawNearFov:
    {
        WriteQueryResult(pInOut0, nInOutSize0, CV_r_drawnearfov);
    }
    break;

    case EFQ_TextureStreamingEnabled:
    {
        WriteQueryResult(pInOut0, nInOutSize0, static_cast<bool>(CRenderer::CV_r_texturesstreaming ? 1 : 0));
    }
    break;

    case EFQ_MSAAEnabled:
    {
        WriteQueryResult(pInOut0, nInOutSize0, static_cast<bool>(m_RP.IsMSAAEnabled() ? 1 : 0));
    }
    break;

    case EFQ_AAMode:
    {
        const uint32 nMode = CRenderer::CV_r_AntialiasingMode;
        WriteQueryResult(pInOut0, nInOutSize0, s_pszAAModes[nMode]);
    }
    break;

    case EFQ_GetShaderCombinations:
    case EFQ_SetShaderCombinations:
    case EFQ_CloseShaderCombinations:
    {
        // no longer used, can be ignored
    }
    break;

    case EFQ_Fullscreen:
    {
        WriteQueryResult(pInOut0, nInOutSize0, static_cast<bool>(QueryIsFullscreen() ? 1 : 0));
    }
    break;

    case EFQ_GetTexStreamingInfo:
    {
#ifndef NULL_RENDERER
        STextureStreamingStats* stats = (STextureStreamingStats*)(pInOut0);
        if (stats)
        {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_CPP_SECTION_11
    #include AZ_RESTRICTED_FILE(Renderer_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
            stats->nCurrentPoolSize = CTexture::s_pPoolMgr->GetReservedSize();    // s_nStatsStreamPoolInUseMem;
#endif
            stats->nStreamedTexturesSize = CTexture::s_nStatsStreamPoolInUseMem;

            stats->nStaticTexturesSize = CTexture::s_nStatsCurManagedNonStreamedTexMem;
            stats->bPoolOverflow = CTexture::s_pTextureStreamer->IsOverflowing();
            stats->bPoolOverflowTotally = CTexture::s_bOutOfMemoryTotally;
            CTexture::s_bOutOfMemoryTotally = false;
            stats->nMaxPoolSize = CRenderer::GetTexturesStreamPoolSize() * 1024 * 1024;
            stats->nThroughput = (CTexture::s_nStreamingTotalTime > 0.f) ? size_t((double)CTexture::s_nStreamingThroughput / CTexture::s_nStreamingTotalTime) : 0;

#ifndef _RELEASE
            stats->nNumTexturesPerFrame = gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_NumTextures;
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_CPP_SECTION_12
    #include AZ_RESTRICTED_FILE(Renderer_cpp)
#endif

            if (stats->bComputeReuquiredTexturesPerFrame)
            {
                stats->nRequiredStreamedTexturesCount = 0;
                stats->nRequiredStreamedTexturesSize = 0;

                AUTO_LOCK(CBaseResource::s_cResLock);

                // compute all sizes
                SResourceContainer* pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
                if (pRL)
                {
                    ResourcesMapItor itor;
                    for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
                    {
                        CTexture* tp = (CTexture*)itor->second;
                        if (!tp || tp->IsNoTexture() || !tp->IsStreamed())
                        {
                            continue;
                        }

                        const STexStreamingInfo* pTSI = tp->GetStreamingInfo();
                        if (!pTSI)
                        {
                            continue;
                        }

                        int nPersMip = tp->GetNumMipsNonVirtual() - tp->GetNumPersistentMips();
                        bool bStale = CTexture::s_pTextureStreamer->StatsWouldUnload(tp);
                        int nCurMip = bStale ? nPersMip : tp->GetRequiredMipNonVirtual();
                        if (tp->IsForceStreamHighRes())
                        {
                            nCurMip = 0;
                        }
                        int nMips = tp->GetNumMipsNonVirtual();
                        nCurMip = min(nCurMip, nPersMip);

                        int nTexSize = tp->StreamComputeDevDataSize(nCurMip);

                        stats->nRequiredStreamedTexturesSize += nTexSize;
                        ++stats->nRequiredStreamedTexturesCount;
                    }
                }
            }

            stats->bPoolOverflow = CTexture::s_pTextureStreamer->IsOverflowing();
            stats->bPoolOverflowTotally = CTexture::s_bOutOfMemoryTotally;
            CTexture::s_bOutOfMemoryTotally = 0;
        }
        if (pInOut1)
        {
            WriteQueryResult(pInOut1, nInOutSize1, static_cast<bool>(CTexture::s_nStreamingTotalTime > 0.f && stats != NULL));
        }
#endif // NULL_RENDERER
    }
    break;

    case EFQ_GetShaderCacheInfo:
    {
        SShaderCacheStatistics* pStats = (SShaderCacheStatistics*)(pInOut0);
        if (pStats)
        {
            memcpy(pStats, &m_cEF.m_ShaderCacheStats, sizeof(SShaderCacheStatistics));
            pStats->m_bShaderCompileActive = CV_r_shadersAllowCompilation != 0;
        }
    }
    break;

    case EFQ_OverscanBorders:
    {
        WriteQueryResult(pInOut0, nInOutSize0, s_overscanBorders);
    }
    break;

    case EFQ_NumActivePostEffects:
    {
        int nSize = 0;
        if (CV_r_PostProcess && PostEffectMgr())
        {
            //assume query is from main thread
            nSize = PostEffectMgr()->GetActiveEffects(m_RP.m_nFillThreadID).size();
        }

        WriteQueryResult(pInOut0, nInOutSize0, nSize);
    }
    break;

    case EFQ_GetFogCullDistance:
    {
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
        // only return the culling distance for the main camera, this helps
        // prevent r_displayinfo flickering
        if (!IsRenderToTextureActive())
        {
            WriteQueryResult(pInOut0, nInOutSize0, m_fogCullDistance);
        }
#else
        WriteQueryResult(pInOut0, nInOutSize0, m_fogCullDistance);
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    }
    break;

    case EFQ_GetMaxRenderObjectsNum:
    {
        WriteQueryResult(pInOut0, nInOutSize0, MAX_REND_OBJECTS);
    }
    break;

    case EFQ_IsRenderLoadingThreadActive:
    {
        WriteQueryResult(pInOut0, nInOutSize0, static_cast<bool>(m_pRT && m_pRT->m_pThreadLoading ? 1 : 0));
    }
    break;

    case EFQ_GetSkinningDataPoolSize:
    {
        int nSkinningPoolSize = 0;
        for (int i = 0; i < 3; ++i)
        {
            nSkinningPoolSize += m_SkinningDataPool[i].AllocatedMemory();
        }
        WriteQueryResult(pInOut0, nInOutSize0, nSkinningPoolSize);
    }
    break;

    case EFQ_GetMeshPoolInfo:
    {
        if (SMeshPoolStatistics* stats = (SMeshPoolStatistics*)(pInOut0))
        {
            CRenderMesh::GetPoolStats(stats);
        }
    }
    break;

    case EFQ_GetViewportDownscaleFactor:
    {
        WriteQueryResult(pInOut0, nInOutSize0, m_CurViewportScale);
    }
    break;
    case EFQ_ReverseDepthEnabled:
    {
        const uint32 nThreadID = m_pRT->GetThreadList();
        uint32 nReverseDepth = gRenDev->m_RP.m_TI[nThreadID].m_PersFlags & RBPF_REVERSE_DEPTH;

        WriteQueryResult(pInOut0, nInOutSize0, nReverseDepth);
    }
    break;
    case EFQ_GetLastD3DDebugMessage:
    {
#if defined(SUPPORT_D3D_DEBUG_RUNTIME)
        class D3DDebugMessage
            : public ID3DDebugMessage
        {
        public:
            D3DDebugMessage(const char* pMsg)
                : m_msg(pMsg) {}

            virtual void Release() { delete this; }
            virtual const char* GetMessage() const { return m_msg.c_str(); }

        protected:
            string m_msg;
        };

        if (pInOut0)
        {
            *((ID3DDebugMessage**) pInOut0) = new D3DDebugMessage(D3DDebug_GetLastMessage());
        }
#endif
        break;
    }

    default:
        assert(0);
    }
}

void CRenderer::ForceGC(){ gRenDev->m_pRT->RC_ForceMeshGC(false); }

//================================================================================================================

_smart_ptr<IRenderMesh> CRenderer::CreateRenderMesh(const char* szType, const char* szSourceName, IRenderMesh::SInitParamerers* pInitParams, ERenderMeshType eBufType)
{
    if (pInitParams)
    {
        return CreateRenderMeshInitialized(pInitParams->pVertBuffer, pInitParams->nVertexCount, pInitParams->vertexFormat, pInitParams->pIndices, pInitParams->nIndexCount, pInitParams->nPrimetiveType, szType, szSourceName,
            pInitParams->eType, pInitParams->nRenderChunkCount, pInitParams->nClientTextureBindID, 0, 0, pInitParams->bOnlyVideoBuffer, pInitParams->bPrecache, pInitParams->pTangents, pInitParams->bLockForThreadAccess, pInitParams->pNormals);
    }

    // make material table with clean elements
    _smart_ptr<CRenderMesh> pRenderMesh = new CRenderMesh(szType, szSourceName);
    pRenderMesh->_SetRenderMeshType(eBufType);

    return pRenderMesh.get();
}

// Creates the RenderMesh with the materials, secondary buffer (system buffer)
// indices and perhaps some other stuff initialized.
// NOTE: if the pVertBuffer is NULL, the system buffer doesn't get initialized with any values
// (trash may be in it)
_smart_ptr<IRenderMesh> CRenderer::CreateRenderMeshInitialized(
    const void* pVertBuffer, int nVertCount, const AZ::Vertex::Format& vertexFormat,
    const vtx_idx* pIndices, int nIndices,
    const PublicRenderPrimitiveType nPrimetiveType, const char* szType, const char* szSourceName, ERenderMeshType eBufType,
    int nMatInfoCount, int nClientTextureBindID,
    bool(* PrepareBufferCallback)(IRenderMesh*, bool),
    void* CustomData, bool bOnlyVideoBuffer, bool bPrecache,
    const SPipTangents* pTangents, bool bLockForThreadAcc, Vec3* pNormals)
{
    FUNCTION_PROFILER_RENDERER;

    _smart_ptr<CRenderMesh> pRenderMesh = new CRenderMesh(szType, szSourceName, bLockForThreadAcc);
    pRenderMesh->_SetRenderMeshType(eBufType);
    pRenderMesh->LockForThreadAccess();

    // make mats info list
    pRenderMesh->m_Chunks.reserve(nMatInfoCount);
    pRenderMesh->_SetVertexFormat(vertexFormat);
    pRenderMesh->_SetNumVerts(nVertCount);
    pRenderMesh->_SetNumInds(nIndices);

    // copy vert buffer
    if (pVertBuffer && !PrepareBufferCallback && !bOnlyVideoBuffer)
    {
        pRenderMesh->UpdateVertices(pVertBuffer, nVertCount, 0, VSF_GENERAL, 0u, false);
        if (pTangents)
        {
            pRenderMesh->UpdateVertices(pTangents, nVertCount, 0, VSF_TANGENTS, 0u, false);
        }
#if ENABLE_NORMALSTREAM_SUPPORT
        if (pNormals)
        {
            pRenderMesh->UpdateVertices(pNormals, nVertCount, 0, VSF_NORMALS, 0u, false);
        }
#endif
    }

    if (CustomData)
    {
        CryFatalError("CRenderMesh::CustomData not supported anymore. Will be removed from interface");
    }

    if (pIndices)
    {
        pRenderMesh->UpdateIndices(pIndices, nIndices, 0, 0u, false);
    }
    pRenderMesh->_SetPrimitiveType(GetInternalPrimitiveType(nPrimetiveType));

    pRenderMesh->m_nClientTextureBindID = nClientTextureBindID;

    // Precache for static buffers
    if (CV_r_meshprecache && pRenderMesh->GetNumVerts() && bPrecache && !m_bDeviceLost && m_pRT->IsRenderThread())
    {
        pRenderMesh->CheckUpdate(-1);
    }

    pRenderMesh->UnLockForThreadAccess();
    return pRenderMesh.get();
}

//=======================================================================

void CRenderer::SetWhiteTexture()
{
    m_pRT->RC_SetTexture(CTextureManager::Instance()->GetWhiteTexture()->GetID(), 0);
}

int CRenderer::GetWhiteTextureId() const
{
    const int textureId = (CTextureManager::Instance()->GetWhiteTexture()) ? CTextureManager::Instance()->GetWhiteTexture()->GetID() : -1;
    return textureId;
}

int CRenderer::GetBlackTextureId() const
{
    const int textureId = (CTextureManager::Instance()->GetBlackTexture()) ? CTextureManager::Instance()->GetBlackTexture()->GetID() : -1;
    return textureId;
}

void CRenderer::SetTexture(int tnum)
{
    m_pRT->RC_SetTexture(tnum, 0);
}

void CRenderer::SetTexture(int tnum, int nUnit)
{
    m_pRT->RC_SetTexture(tnum, nUnit);
}

// used for sprite generation
void CRenderer::SetTextureAlphaChannelFromRGB(byte* pMemBuffer, int nTexSize)
{
    // set alpha channel
    for (int y = 0; y < nTexSize; y++)
    {
        for (int x = 0; x < nTexSize; x++)
        {
            int t = (x + nTexSize * y) * 4;
            if (abs(pMemBuffer[t + 0] - pMemBuffer[0 + 0]) < 2 &&
                abs(pMemBuffer[t + 1] - pMemBuffer[0 + 1]) < 2 &&
                abs(pMemBuffer[t + 2] - pMemBuffer[0 + 2]) < 2)
            {
                pMemBuffer[t + 3] = 0;
            }
            else
            {
                pMemBuffer[t + 3] = 255;
            }

            // set border alpha to 0
            if (x == 0 || y == 0 || x == nTexSize - 1 || y == nTexSize - 1)
            {
                pMemBuffer[t + 3] = 0;
            }
        }
    }
}

//=============================================================================
// Precaching
bool CRenderer::EF_PrecacheResource(IRenderMesh* _pPB, _smart_ptr<IMaterial> pMaterial, float fMipFactor, [[maybe_unused]] float fTimeToReady, int nFlags, int nUpdateId)
{
    int i;
    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_texturesstreaming)
    {
        return true;
    }

    CRenderMesh* pPB = (CRenderMesh*)_pPB;

    for (i = 0; i < pPB->m_Chunks.size(); i++)
    {
        CRenderChunk* pChunk = &pPB->m_Chunks[i];
        assert(!"do pre-cache with real materials");

        assert(0);

        //@TODO: Timur
        assert(pMaterial && "RenderMesh must have material");
        CShaderResources* pSR = (CShaderResources*)pMaterial->GetShaderItem(pChunk->m_nMatID).m_pShaderResources;
        if (!pSR)
        {
            continue;
        }
        if (pSR->m_nFrameLoad != m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameID)
        {
            pSR->m_nFrameLoad = m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameID;
            pSR->m_fMinMipFactorLoad = 999999.0f;
        }
        else
        if (fMipFactor >= pSR->m_fMinMipFactorLoad)
        {
            continue;
        }

        pSR->m_fMinMipFactorLoad = fMipFactor;
        for (auto& iter : pSR->m_TexturesResourcesMap )
        {
            SEfResTexture*      pTexture = &(iter.second);
            CTexture*           tp = static_cast<CTexture*> (pTexture->m_Sampler.m_pITex);
            if (!tp)
            {
                continue;
            }
            fMipFactor *= pTexture->GetTiling(0) * pTexture->GetTiling(1);
            m_pRT->RC_PrecacheResource(tp, fMipFactor, 0, nFlags, nUpdateId);
        }
    }
    return true;
}

bool CRenderer::EF_PrecacheResource(CDLight* pLS, float fMipFactor, [[maybe_unused]] float fTimeToReady, int Flags, int nUpdateId)
{
    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_RENDERER, g_bProfilerEnabled);

    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_texturesstreaming)
    {
        return true;
    }

    ITexture* pLightTexture = pLS->m_pLightImage ? pLS->m_pLightImage : NULL;
    if (pLightTexture)
    {
        m_pRT->RC_PrecacheResource(pLightTexture, fMipFactor, 0, Flags, nUpdateId);
    }
    if (pLS->GetDiffuseCubemap())
    {
        m_pRT->RC_PrecacheResource(pLS->GetDiffuseCubemap(), fMipFactor, 0, Flags, nUpdateId);
    }
    if (pLS->GetSpecularCubemap())
    {
        m_pRT->RC_PrecacheResource(pLS->GetSpecularCubemap(), fMipFactor, 0, Flags, nUpdateId);
    }
    return true;
}

void CRenderer::PrecacheTexture(ITexture* pTP, float fMipFactor, [[maybe_unused]] float fTimeToReady, int Flags, int nUpdateId, int nCounter)
{
    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_texturesstreaming)
    {
        return;
    }

    assert(m_pRT->IsRenderThread());

    if (pTP)
    {
        ((CTexture*)pTP)->CTexture::PrecacheAsynchronously(fMipFactor, Flags, nUpdateId, nCounter);
    }
}

bool CRenderer::EF_PrecacheResource([[maybe_unused]] IShader* pSH, [[maybe_unused]] float fMipFactor, [[maybe_unused]] float fTimeToReady, [[maybe_unused]] int Flags)
{
    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_texturesstreaming)
    {
        return true;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
// HDR_UPPERNORM -> factor used when converting from [0,32768] high dynamic range images
//                  to [0,1] low dynamic range images; 32768 = 2^(2^4-1), 4 exponent bits
// LDR_UPPERNORM -> factor used when converting from [0,1] low dynamic range images
//                  to 8bit outputs

#define HDR_UPPERNORM   1.0f    // factor set to 1.0, to be able to see content in our rather dark HDR images
#define LDR_UPPERNORM   255.0f

//////////////////////////////////////////////////////////////////////////
// NOTE: AMD64 port: implement
#define PROCESS_IN_PARALLEL

// Squish uses non-standard inline friend templates which Recode cannot parse
#if !defined(__RECODE__) && defined(USING_SQUISH_SDK)
#include <squish-ccr/squish.h>
#endif

// number of bytes per block per type
#define BLOCKSIZE_BC1   8
#define BLOCKSIZE_BC2   16
#define BLOCKSIZE_BC3   16
#define BLOCKSIZE_BC4   8
#define BLOCKSIZE_BC5   16
#define BLOCKSIZE_BC6   16
#define BLOCKSIZE_BC7   16

#if !defined(__RECODE__) && defined(USING_SQUISH_SDK)
struct SCompressRowData
{
    struct squish::sqio* pSqio;
    byte* destinationData;
    const byte* sourceData;
    int row;
    int width;
    int height;
    int blockWidth;
    int blockHeight;
    int pixelStride;
    int rowStride;
    int blockStride;
    int sourceChannels;
    int destinationChannels;
    int offs;
};

static void DXTDecompressRow(SCompressRowData data)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
#if defined(WIN32)
    SCOPED_DISABLE_FLOAT_EXCEPTIONS;
#endif

    byte* dst = data.destinationData + (data.row * data.rowStride);
    const byte* src = data.sourceData + ((data.row >> 2) * data.blockStride);

    for (int x = 0; x < data.width; x += data.blockWidth)
    {
        uint8 values[4][4][4];

        // decode
        data.pSqio->decoder((uint8*)values, const_cast<void*>(static_cast<const void*>(src)), data.pSqio->flags);

        // transfer
        for (int by = 0; by < data.blockHeight; by += 1)
        {
            byte* bdst = ((uint8*)dst) + (by * data.rowStride);

            for (int bx = 0; bx < data.blockWidth; bx += 1)
            {
                bdst[bx * data.pixelStride + 0] = data.sourceChannels <= 0 ? 0U : (values[by][bx][0] + data.offs);
                bdst[bx * data.pixelStride + 1] = data.sourceChannels <= 1 ? bdst[bx * data.pixelStride + 0] : (values[by][bx][1] + data.offs);
                bdst[bx * data.pixelStride + 2] = data.sourceChannels <= 1 ? bdst[bx * data.pixelStride + 0] : (values[by][bx][2] + data.offs);
                bdst[bx * data.pixelStride + 3] = data.sourceChannels <= 3 ? 255U : (values[by][bx][3]);
            }
        }

        dst += data.blockWidth * data.pixelStride;
        src += data.pSqio->blocksize;
    }
}

static void DXTDecompressRowFloat(SCompressRowData data)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
#if defined(WIN32)
    SCOPED_DISABLE_FLOAT_EXCEPTIONS;
#endif

    byte* dst = data.destinationData + (data.row * data.rowStride);
    const byte* src = data.sourceData + ((data.row >> 2) * data.blockStride);

    for (int x = 0; x < data.width; x += data.blockWidth)
    {
        byte values[4][4][4];

        // decode
        data.pSqio->decoder((byte*)values, const_cast<void*>(static_cast<const void*>(src)), data.pSqio->flags);

        // transfer
        for (int by = 0; by < data.blockHeight; by += 1)
        {
            byte* bdst = ((byte*)dst) + (by * data.rowStride);

            for (int bx = 0; bx < data.blockWidth; bx += 1)
            {
                bdst[bx * data.pixelStride + 0] = data.sourceChannels <= 0 ? 0U : std::min((byte)255, (byte)floorf(values[by][bx][0] * LDR_UPPERNORM / HDR_UPPERNORM + 0.5f));
                bdst[bx * data.pixelStride + 1] = data.sourceChannels <= 1 ? bdst[bx * data.pixelStride + 0] : std::min((uint8)255, (uint8)floorf(values[by][bx][1] * LDR_UPPERNORM / HDR_UPPERNORM + 0.5f));
                bdst[bx * data.pixelStride + 2] = data.sourceChannels <= 1 ? bdst[bx * data.pixelStride + 0] : std::min((uint8)255, (uint8)floorf(values[by][bx][2] * LDR_UPPERNORM / HDR_UPPERNORM + 0.5f));
                bdst[bx * data.pixelStride + 3] = data.sourceChannels <= 3 ? 255U : 255U;
            }
        }

        dst += data.blockWidth * data.pixelStride;
        src += data.pSqio->blocksize;
    }
}

static void DXTCompressRow(SCompressRowData data)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
#if defined(WIN32)
    SCOPED_DISABLE_FLOAT_EXCEPTIONS;
#endif

    byte* dst = data.destinationData + ((data.row >> 2) * data.blockStride);
    const byte* src = data.sourceData + (data.row * data.rowStride);

    for (int x = 0; x < data.width; x += data.blockWidth)
    {
        byte values[4][4][4];

        // transfer
        for (int by = 0; by < data.blockHeight; by += 1)
        {
            const byte* bsrc = src + (by * data.rowStride);

            for (int bx = 0; bx < data.blockWidth; bx += 1)
            {
                values[by][bx][0] = data.destinationChannels <= 0 ? 0U : bsrc[bx * data.pixelStride + 0] - data.offs;
                values[by][bx][1] = data.destinationChannels <= 1 ? values[by][bx][0] : bsrc[bx * data.pixelStride + 1] - data.offs;
                values[by][bx][2] = data.destinationChannels <= 1 ? values[by][bx][0] : bsrc[bx * data.pixelStride + 2] - data.offs;
                values[by][bx][3] = data.destinationChannels <= 3 ? 255U : bsrc[bx * data.pixelStride + 3];
            }
        }
        // encode
        data.pSqio->encoder((float*)values, 0xFFFF, (void*)dst, data.pSqio->flags);

        src += data.blockWidth * data.pixelStride;
        dst += data.pSqio->blocksize;
    }
}

static void DXTCompressRowFloat(SCompressRowData data)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
#if defined(WIN32)
    SCOPED_DISABLE_FLOAT_EXCEPTIONS;
#endif

    byte* dst = data.destinationData + ((data.row >> 2) * data.blockStride);
    const byte* src = data.sourceData + (data.row * data.rowStride);

    for (int x = 0; x < data.width; x += data.blockWidth)
    {
        float values[4][4][4];

        // transfer
        for (int by = 0; by < data.blockHeight; by += 1)
        {
            const byte* bsrc = src + (by * data.rowStride);

            for (int bx = 0; bx < data.blockWidth; bx += 1)
            {
                values[by][bx][0] = data.destinationChannels <= 0 ? 0U : bsrc[bx * data.pixelStride + 0] * HDR_UPPERNORM / LDR_UPPERNORM;
                values[by][bx][1] = data.destinationChannels <= 1 ? values[by][bx][0] : bsrc[bx * data.pixelStride + 1] * HDR_UPPERNORM / LDR_UPPERNORM;
                values[by][bx][2] = data.destinationChannels <= 1 ? values[by][bx][0] : bsrc[bx * data.pixelStride + 2] * HDR_UPPERNORM / LDR_UPPERNORM;
                values[by][bx][3] = data.destinationChannels <= 3 ? 255U : 1.0f;
            }
        }

        // encode
        data.pSqio->encoder((float*)values, 0xFFFF, (void*)dst, data.pSqio->flags);

        src += data.blockWidth * data.pixelStride;
        dst += data.pSqio->blocksize;
    }
}

#endif // #if !defined(__RECODE__) && defined(USING_SQUISH_SDK)

bool CRenderer::DXTDecompress([[maybe_unused]] const byte* sourceData, [[maybe_unused]] const size_t srcFileSize, [[maybe_unused]] byte* destinationData, [[maybe_unused]] int width, [[maybe_unused]] int height, [[maybe_unused]] int mips, [[maybe_unused]] ETEX_Format sourceFormat, bool bUseHW, [[maybe_unused]] int nDstBytesPerPix)
{
    if (bUseHW)
    {
        return false;
    }

    // Squish uses non-standard inline friend templates which Recode cannot parse
#if !defined(__RECODE__) && defined(USING_SQUISH_SDK)
    int flags = 0;
    int offs = 0;
    int sourceChannels = 4;
    switch (sourceFormat)
    {
    case eTF_BC1:
        sourceChannels = 4;
        flags = squish::kBtc1;
        break;
    case eTF_BC2:
        sourceChannels = 4;
        flags = squish::kBtc2;
        break;
    case eTF_BC3:
        sourceChannels = 4;
        flags = squish::kBtc3;
        break;
    case eTF_BC4U:
        sourceChannels = 1;
        flags = squish::kBtc4;
        break;
    case eTF_BC5U:
        sourceChannels = 2;
        flags = squish::kBtc5 + squish::kColourMetricUnit;
        break;
    case eTF_BC6UH:
        sourceChannels = 3;
        flags = squish::kBtc6;
        break;
    case eTF_BC7:
        sourceChannels = 4;
        flags = squish::kBtc7;
        break;

    case eTF_BC4S:
        sourceChannels = 1;
        flags = squish::kBtc4 + squish::kSignedInternal + squish::kSignedExternal;
        offs = 0x80;
        break;
    case eTF_BC5S:
        sourceChannels = 2;
        flags = squish::kBtc5 + squish::kSignedInternal + squish::kSignedExternal + squish::kColourMetricUnit;
        offs = 0x80;
        break;
    case eTF_BC6SH:
        sourceChannels = 3;
        flags = squish::kBtc6 + squish::kSignedInternal + squish::kSignedExternal;
        offs = 0x80;
        break;

    // unsupported input
    default:
        return false;
    }

    squish::sqio::dtp datatype = !CImageExtensionHelper::IsRangeless(sourceFormat) ? squish::sqio::DT_U8 : squish::sqio::DT_F23;

    if (nDstBytesPerPix == 4)
    {
        datatype = squish::sqio::DT_U8;
    }
    //  else if (nDstBytesPerPix == 8)
    //      datatype = squish::sqio::DT_U16;
    //  else if (nDstBytesPerPix == 16)
    //      datatype = squish::sqio::DT_F23;
    else
    {
        // unsupported output
        return false;
    }

    struct squish::sqio sqio = squish::GetSquishIO(width, height, datatype, flags);

    const int blockChannels = 4;
    const int blockWidth = 4;
    const int blockHeight = 4;

    SCompressRowData data;
    data.pSqio = &sqio;
    data.destinationData = destinationData;
    data.sourceData = sourceData;
    data.width = width;
    data.height = height;
    data.blockWidth = blockWidth;
    data.blockHeight = blockHeight;
    data.pixelStride = blockChannels * sizeof(uint8);
    data.rowStride = data.pixelStride * width;
    data.blockStride = sqio.blocksize * (width >> 2);
    data.sourceChannels = sourceChannels;
    data.offs = offs;

    if ((datatype == squish::sqio::DT_U8) && (nDstBytesPerPix == 4))
    {
        for (int y = 0; y < height; y += blockHeight)
        {
            data.row = y;
            DXTDecompressRow(data);
        }
    }
    else if ((datatype == squish::sqio::DT_F23) && (nDstBytesPerPix == 4))
    {
        for (int y = 0; y < height; y += blockHeight)
        {
            data.row = y;
            DXTDecompressRowFloat(data);
        }
    }
    else
    {
        // implement decode to 16bit and floats
        assert(0);
        delete[] destinationData;
        return false;
    }

    return true;
#else
    return false;
#endif
}

bool CRenderer::DXTCompress(const byte* sourceData, int width, int height, [[maybe_unused]] ETEX_Format destinationFormat, bool bUseHW, bool bGenMips, int nSrcBytesPerPix, [[maybe_unused]] MIPDXTcallback callback)
{
    if (bUseHW)
    {
        return false;
    }
    if (bGenMips)
    {
        return false;
    }
    if IsCVarConstAccess(constexpr) (CV_r_TextureCompressor == 0)
    {
        return false;
    }

#ifdef WIN32
    if (IsBadReadPtr(sourceData, width * height * nSrcBytesPerPix))
    {
        assert(0);
        iLog->Log("Warning: CRenderer::DXTCompress: invalid data passed to the function");
        return false;
    }
#endif

#if !defined(__RECODE__) && defined(USING_SQUISH_SDK)
    int flags = 0;
    int offs = 0;
    int destinationChannels = 4;
    switch (destinationFormat)
    {
    // fastest encoding parameters possible
    case eTF_BC1:
        destinationChannels = 4;
        flags = squish::kBtc1 + squish::kColourMetricPerceptual + squish::kColourRangeFit + squish::kExcludeAlphaFromPalette;
        break;
    case eTF_BC2:
        destinationChannels = 4;
        flags = squish::kBtc2 + squish::kColourMetricPerceptual + squish::kColourRangeFit /*squish::kWeightColourByAlpha*/;
        break;
    case eTF_BC3:
        destinationChannels = 4;
        flags = squish::kBtc3 + squish::kColourMetricPerceptual + squish::kColourRangeFit /*squish::kWeightColourByAlpha*/;
        break;
    case eTF_BC4U:
        destinationChannels = 1;
        flags = squish::kBtc4 + squish::kColourMetricUniform;
        break;
    case eTF_BC5U:
        destinationChannels = 2;
        flags = squish::kBtc5 + squish::kColourMetricUnit;
        break;
    case eTF_BC6UH:
        destinationChannels = 3;
        flags = squish::kBtc6 + squish::kColourMetricPerceptual + squish::kColourRangeFit;
        break;
    case eTF_BC7:
        destinationChannels = 4;
        flags = squish::kBtc7 + squish::kColourMetricPerceptual + squish::kColourRangeFit;
        break;

    case eTF_BC4S:
        destinationChannels = 1;
        flags = squish::kBtc4 + squish::kSignedInternal + squish::kSignedExternal + squish::kColourMetricUniform;
        offs = 0x80;
        break;
    case eTF_BC5S:
        destinationChannels = 2;
        flags = squish::kBtc5 + squish::kSignedInternal + squish::kSignedExternal + squish::kColourMetricUnit;
        offs = 0x80;
        break;
    case eTF_BC6SH:
        destinationChannels = 3;
        flags = squish::kBtc6 + squish::kSignedInternal + squish::kSignedExternal;
        offs = 0x80;
        break;

    // unsupported output
    default:
        return false;
    }

    squish::sqio::dtp datatype = !CImageExtensionHelper::IsRangeless(destinationFormat) ? squish::sqio::DT_U8 : squish::sqio::DT_F23;

    if (nSrcBytesPerPix == 4)
    {
        datatype = squish::sqio::DT_U8;
    }
    //  else if (nSrcBytesPerPix == 8)
    //      datatype = squish::sqio::DT_U16;
    //  else if (nSrcBytesPerPix == 16)
    //      datatype = squish::sqio::DT_F23;
    else
    {
        // unsupported input
        return false;
    }

    struct squish::sqio sqio = squish::GetSquishIO(width, height, datatype, flags);

    uint8* destinationData = new uint8[sqio.compressedsize];
    if (!destinationData)
    {
        return false;
    }

    const int blockChannels = 4;
    const int blockWidth = 4;
    const int blockHeight = 4;

    SCompressRowData data;
    data.pSqio = &sqio;
    data.destinationData = destinationData;
    data.sourceData = sourceData;
    data.width = width;
    data.height = height;
    data.blockWidth = blockWidth;
    data.blockHeight = blockHeight;
    data.pixelStride = blockChannels * sizeof(uint8);
    data.rowStride = data.pixelStride * width;
    data.blockStride = sqio.blocksize * (width >> 2);
    data.destinationChannels = destinationChannels;
    data.offs = offs;

    if ((datatype == squish::sqio::DT_U8) && (nSrcBytesPerPix == 4))
    {
        for (int y = 0; y < height; y += blockHeight)
        {
            data.row = y;
            DXTCompressRow(data);
        }
    }
    else if ((datatype == squish::sqio::DT_F23) && (nSrcBytesPerPix == 4))
    {
        for (int y = 0; y < height; y += blockHeight)
        {
            data.row = y;
            DXTCompressRowFloat(data);
        }
    }
    else
    {
        // implement encode from 16bit and floats
        assert(0);
        delete[] destinationData;
        return false;
    }


    (*callback)(destinationData, sqio.compressedsize, NULL);
    delete[] destinationData;

    return true;
#else
    return false;
#endif
}
bool CRenderer::WriteJPG(const byte* dat, int wdt, int hgt, char* name, int src_bits_per_pixel, int nQuality)
{
    return ::WriteJPG(dat, wdt, hgt, name, src_bits_per_pixel, nQuality);
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::GetThreadIDs(threadID& mainThreadID, threadID& renderThreadID) const
{
    if (m_pRT)
    {
        mainThreadID = m_pRT->m_nMainThread;
        renderThreadID = m_pRT->m_nRenderThread;
    }
    else
    {
        mainThreadID = renderThreadID = gEnv->mMainThreadId;
    }
}
//////////////////////////////////////////////////////////////////////////
void CRenderer::PostLevelLoading()
{
    int nThreadID = m_pRT->GetThreadList();
    m_RP.m_fogVolumeContibutionsData[nThreadID].reserve(2048);
}

const char* CRenderer::GetTextureFormatName(ETEX_Format eTF)
{
    return CTexture::NameForTextureFormat(eTF);
}

int CRenderer::GetTextureFormatDataSize(int nWidth, int nHeight, int nDepth, int nMips, ETEX_Format eTF)
{
    return CTexture::TextureDataSize(nWidth, nHeight, nDepth, nMips, 1, eTF);
}


//////////////////////////////////////////////////////////////////////////
ERenderType CRenderer::GetRenderType() const
{
#if defined(NULL_RENDERER)
    return eRT_Null;
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION RENDERER_CPP_SECTION_13
    #include AZ_RESTRICTED_FILE(Renderer_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(CRY_USE_METAL)
    return eRT_Metal;
#elif defined(OPENGL)
    return eRT_OpenGL;
#elif defined(CRY_USE_DX12)
    return eRT_DX12;
#else
    return eRT_DX11;
#endif
}
/////////////////////////////////////////////////////////////////////////////////////////////////////

void IRenderer::SDrawCallCountInfo::Update(CRenderObject* pObj, IRenderMesh* pRM)
{
    SRenderPipeline& RESTRICT_REFERENCE rRP = gRenDev->m_RP;
    if (((IRenderNode*)pObj->m_pRenderNode))
    {
        pPos = pObj->GetTranslation();

        if (meshName[0] == '\0')
        {
            const char* pMeshName = pRM->GetSourceName();
            if (pMeshName)
            {
                const size_t nameLen = strlen(pMeshName);

                // truncate if necessary
                if (nameLen >= sizeof(meshName))
                {
                    pMeshName += nameLen - (sizeof(meshName) - 1);
                }
                cry_strcpy(meshName, pMeshName);
            }

            const char* pTypeName = pRM->GetTypeName();
            if (pTypeName)
            {
                cry_strcpy(typeName, pTypeName);
            }
        }

        if (rRP.m_nBatchFilter & (FB_MOTIONBLUR | FB_CUSTOM_RENDER | FB_POST_3D_RENDER | FB_SOFTALPHATEST | FB_DEBUG))
        {
            nMisc++;
        }
        else
        if (!(rRP.m_TI[rRP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN))
        {
            if (rRP.m_nBatchFilter & FB_GENERAL)
            {
                if (rRP.m_nPassGroupID == EFSLIST_TRANSP)
                {
                    nTransparent++;
                }
                else
                {
                    nGeneral++;
                }
            }
            else if (rRP.m_nBatchFilter & (FB_Z | FB_ZPREPASS))
            {
                nZpass++;
            }
        }
        else
        {
            nShadows++;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void S3DEngineCommon::Update(threadID nThreadID)
{
    I3DEngine* p3DEngine = gEnv->p3DEngine;

    // Camera vis area
    IVisArea* pCamVisArea = p3DEngine->GetVisAreaFromPos(gRenDev->GetViewParameters().vOrigin);
    m_pCamVisAreaInfo.nFlags &= ~VAF_MASK;
    if (pCamVisArea)
    {
        m_pCamVisAreaInfo.nFlags |= VAF_EXISTS_FOR_POSITION;
        if (pCamVisArea->IsConnectedToOutdoor())
        {
            m_pCamVisAreaInfo.nFlags |= VAF_CONNECTED_TO_OUTDOOR;
        }
        if (pCamVisArea->IsAffectedByOutLights())
        {
            m_pCamVisAreaInfo.nFlags |= VAF_AFFECTED_BY_OUT_LIGHTS;
        }
    }

    // Update ocean info
    m_OceanInfo.m_fWaterLevel = OceanToggle::IsActive() ? OceanRequest::GetWaterLevel(gRenDev->GetViewParameters().vOrigin) : p3DEngine->GetWaterLevel(&gRenDev->GetViewParameters().vOrigin);
    m_OceanInfo.m_nOceanRenderFlags = p3DEngine->GetOceanRenderFlags();

    if IsCVarConstAccess(constexpr) (bool( CRenderer::CV_r_rain))
    {
        const int nFrmID = gRenDev->GetFrameID();
        if (m_RainInfo.nUpdateFrameID != nFrmID)
        {
            UpdateRainInfo(nThreadID);
            UpdateSnowInfo(nThreadID);
            m_RainInfo.nUpdateFrameID = nFrmID;
        }
    }

    // Release rain occluders
    if (CRenderer::CV_r_rain < 2 || m_RainInfo.bDisableOcclusion)
    {
        m_RainOccluders.Release();
        stl::free_container(m_RainOccluders.m_arrCurrOccluders[nThreadID]);
        m_RainInfo.bApplyOcclusion = false;
    }
}

void S3DEngineCommon::UpdateRainInfo(threadID nThreadID)
{
    gEnv->p3DEngine->GetRainParams(m_RainInfo);

    bool bProcessedAll = true;
    const uint32 numGPUs = gRenDev->GetActiveGPUCount();
    for (uint32 i = 0; i < numGPUs; ++i)
    {
        bProcessedAll &= m_RainOccluders.m_bProcessed[i];
    }
    const bool bUpdateOcc = bProcessedAll;
    if (bUpdateOcc)
    {
        m_RainOccluders.Release();
    }

    const Vec3 vCamPos = gRenDev->GetViewParameters().vOrigin;
    const float fUnderWaterAtten = clamp_tpl(vCamPos.z - m_OceanInfo.m_fWaterLevel + 1.f, 0.f, 1.f);
    m_RainInfo.fCurrentAmount *= fUnderWaterAtten;

    //#define RAIN_DEBUG
#ifndef RAIN_DEBUG
    if (m_RainInfo.fCurrentAmount < 0.05f)
    {
        return;
    }
#endif

#ifdef RAIN_DEBUG
    m_RainInfo.fAmount = 1.f;
    m_RainInfo.fCurrentAmount = 1.f;
    m_RainInfo.fRadius = 2000.f;
    m_RainInfo.fFakeGlossiness = 0.5f;
    m_RainInfo.fFakeReflectionAmount = 1.5f;
    m_RainInfo.fDiffuseDarkening = 0.5f;
    m_RainInfo.fRainDropsAmount = 0.5f;
    m_RainInfo.fRainDropsSpeed = 1.f;
    m_RainInfo.fRainDropsLighting = 1.f;
    m_RainInfo.fMistAmount = 3.f;
    m_RainInfo.fMistHeight = 8.f;
    m_RainInfo.fPuddlesAmount = 1.5f;
    m_RainInfo.fPuddlesMaskAmount = 1.0f;
    m_RainInfo.fPuddlesRippleAmount = 2.0f;
    m_RainInfo.fSplashesAmount = 1.3f;

    m_RainInfo.vColor.Set(1, 1, 1);
    m_RainInfo.vWorldPos.Set(0, 0, 0);
#endif

    UpdateRainOccInfo(nThreadID);
}

void S3DEngineCommon::UpdateSnowInfo(int nThreadID)
{
    gEnv->p3DEngine->GetSnowSurfaceParams(m_SnowInfo.m_vWorldPos, m_SnowInfo.m_fRadius, m_SnowInfo.m_fSnowAmount, m_SnowInfo.m_fFrostAmount, m_SnowInfo.m_fSurfaceFreezing);
    gEnv->p3DEngine->GetSnowFallParams(m_SnowInfo.m_nSnowFlakeCount, m_SnowInfo.m_fSnowFlakeSize, m_SnowInfo.m_fSnowFallBrightness, m_SnowInfo.m_fSnowFallGravityScale, m_SnowInfo.m_fSnowFallWindScale, m_SnowInfo.m_fSnowFallTurbulence, m_SnowInfo.m_fSnowFallTurbulenceFreq);

    //#define RAIN_DEBUG
#ifndef RAIN_DEBUG
    if (m_SnowInfo.m_fSnowAmount < 0.05f && m_SnowInfo.m_fFrostAmount < 0.05f)
    {
        return;
    }
#endif

    UpdateRainOccInfo(nThreadID);
}


void S3DEngineCommon::UpdateRainOccInfo(int nThreadID)
{
    bool bSnowEnabled = (m_SnowInfo.m_fSnowAmount > 0.05f || m_SnowInfo.m_fFrostAmount > 0.05f) && m_SnowInfo.m_fRadius > 0.05f;

    bool bProcessedAll = true;
    const uint32 numGPUs = gRenDev->GetActiveGPUCount();
    for (uint32 i = 0; i < numGPUs; ++i)
    {
        bProcessedAll &= m_RainOccluders.m_bProcessed[i];
    }
    const bool bUpdateOcc = bProcessedAll; // there is no field called bForecedUpdate in RainOccluders - m_RainOccluders.bForceUpdate || bProcessedAll;
    if (bUpdateOcc)
    {
        m_RainOccluders.Release();
    }

    // Get the rendering camera position from the renderer
    const Vec3 vCamPos = gEnv->p3DEngine->GetRenderingCamera().GetPosition();
    bool bDisableOcclusion = m_RainInfo.bDisableOcclusion;
    static bool bOldDisableOcclusion = true;    // set to true to allow update at first run

    if (CRenderer::CV_r_rain == 2 && !bDisableOcclusion)
    {
        N3DEngineCommon::ArrOccluders& arrOccluders = m_RainOccluders.m_arrOccluders;
        static const unsigned int nMAX_OCCLUDERS = bSnowEnabled ? 768 : 512;
        static const float rainBBHalfSize = 18.f;

        if (bUpdateOcc)
        {
            // Choose world position and radius.
            // Snow takes priority since occlusion has a much stronger impact on it.
            Vec3 vWorldPos = bSnowEnabled ? m_SnowInfo.m_vWorldPos : m_RainInfo.vWorldPos;
            float fRadius = bSnowEnabled ? m_SnowInfo.m_fRadius : m_RainInfo.fRadius;
            float fViewerArea = bSnowEnabled ? 128.f : 32.f; // Snow requires further view distance, otherwise obvious "unoccluded" snow regions become visible.
            float fOccArea = fViewerArea;

            // Rain volume BB
            AABB bbRainVol(fRadius);
            bbRainVol.Move(vWorldPos);

            // Visible area BB (Expanded to BB diagonal length to allow rotation)
            AABB bbViewer(fViewerArea);
            bbViewer.Move(vCamPos);

            // Area around viewer/rain source BB
            AABB bbArea(bbViewer);
            bbArea.ClipToBox(bbRainVol);

            // Snap BB to grid
            Vec3 vSnapped = bbArea.min / rainBBHalfSize;
            bbArea.min.Set(floor_tpl(vSnapped.x), floor_tpl(vSnapped.y), floor_tpl(vSnapped.z));
            bbArea.min *= rainBBHalfSize;
            vSnapped = bbArea.max / rainBBHalfSize;
            bbArea.max.Set(ceil_tpl(vSnapped.x), ceil_tpl(vSnapped.y), ceil_tpl(vSnapped.z));
            bbArea.max *= rainBBHalfSize;

            // If occlusion map info dirty
            static float fOccTreshold = CRenderer::CV_r_rainOccluderSizeTreshold;
            static float fOldRadius = fRadius;
            const AABB& oldAreaBounds = m_RainInfo.areaAABB;
            if (!oldAreaBounds.min.IsEquivalent(bbArea.min)
                || !oldAreaBounds.max.IsEquivalent(bbArea.max)
                || fOldRadius != fRadius //m_RainInfo.fRadius
                || bOldDisableOcclusion != bDisableOcclusion
                || fOccTreshold != CRenderer::CV_r_rainOccluderSizeTreshold)
            {
                // Get occluders inside area
                AZ::u32 numAllOccluders = 0;
                const AZ::u32 numAZStaticMeshOccluders = gEnv->p3DEngine->GetObjectsByTypeInBox(eERType_StaticMeshRenderComponent, bbArea);
                const AZ::u32 numAZSkinnedMeshOccluders = gEnv->p3DEngine->GetObjectsByTypeInBox(eERType_SkinnedMeshRenderComponent, bbArea);

                numAllOccluders = numAZStaticMeshOccluders + numAZSkinnedMeshOccluders;

                AZStd::vector<IRenderNode*> occluders(numAllOccluders);

                //Retrieve all AZ static mesh components in the bounding box
                if (numAZStaticMeshOccluders > 0)
                {
                    gEnv->p3DEngine->GetObjectsByTypeInBox(eERType_StaticMeshRenderComponent, bbArea, &occluders[0]);
                }
                //Retrieve all AZ skinned mesh components in the bounding box
                if (numAZSkinnedMeshOccluders > 0)
                {
                    gEnv->p3DEngine->GetObjectsByTypeInBox(eERType_SkinnedMeshRenderComponent, bbArea, &occluders[numAZStaticMeshOccluders]);
                }

                // Set to new values, will be needed for other rain passes
                m_RainInfo.areaAABB = bbArea;
                fOccTreshold = CRenderer::CV_r_rainOccluderSizeTreshold;
                fOldRadius = fRadius;

                AABB geomBB(AABB::RESET);
                const size_t occluderLimit = min(numAllOccluders, nMAX_OCCLUDERS);
                arrOccluders.resize(occluderLimit);
                // Filter occluders and get bounding box
                for (AZStd::vector<IRenderNode*>::const_iterator it = occluders.begin();
                     it != occluders.end() && m_RainOccluders.m_nNumOccluders < occluderLimit;
                     it++)
                {
                    IRenderNode* pRndNode = *it;
                    if (pRndNode)
                    {
                        const AABB& aabb = pRndNode->GetBBox();
                        const Vec3 vDiag = aabb.max - aabb.min;
                        const float fSqrFlatRadius = Vec2(vDiag.x, vDiag.y).GetLength2();
                        const unsigned nRndNodeFlags = pRndNode->GetRndFlags();
                        // TODO: rainoccluder should be the only flag tested
                        // (ie. enabled ONLY for small subset of geometry assets - means going through all assets affected by rain)
                        if ((fSqrFlatRadius < CRenderer::CV_r_rainOccluderSizeTreshold)
                            || !(nRndNodeFlags & ERF_RAIN_OCCLUDER)
                            || (nRndNodeFlags & (ERF_COLLISION_PROXY | ERF_RAYCAST_PROXY | ERF_HIDDEN)))
                        {
                            continue;
                        }

                        N3DEngineCommon::SRainOccluder rainOccluder;
                        IStatObj* pObj = pRndNode->GetEntityStatObj(0, 0, &rainOccluder.m_WorldMat);
                        if (pObj)
                        {
                            const size_t nPrevIdx = m_RainOccluders.m_nNumOccluders;
                            if (pObj->GetFlags() & STATIC_OBJECT_COMPOUND)
                            {
                                const Matrix34A matParentTM = rainOccluder.m_WorldMat;
                                int nSubCount = pObj->GetSubObjectCount();
                                for (int nSubId = 0; nSubId < nSubCount && m_RainOccluders.m_nNumOccluders < occluderLimit; nSubId++)
                                {
                                    IStatObj::SSubObject* pSubObj = pObj->GetSubObject(nSubId);
                                    if (pSubObj->bIdentityMatrix)
                                    {
                                        rainOccluder.m_WorldMat = matParentTM;
                                    }
                                    else
                                    {
                                        rainOccluder.m_WorldMat = matParentTM * pSubObj->localTM;
                                    }

                                    IStatObj* pSubStatObj = pSubObj->pStatObj;
                                    if (pSubStatObj && pSubStatObj->GetRenderMesh())
                                    {
                                        rainOccluder.m_RndMesh = pSubStatObj->GetRenderMesh();
                                        arrOccluders[m_RainOccluders.m_nNumOccluders++] = rainOccluder;
                                    }
                                }
                            }
                            else if (pObj->GetRenderMesh())
                            {
                                rainOccluder.m_RndMesh = pObj->GetRenderMesh();
                                arrOccluders[m_RainOccluders.m_nNumOccluders++] = rainOccluder;
                            }

                            if (m_RainOccluders.m_nNumOccluders > nPrevIdx)
                            {
                                geomBB.Add(pRndNode->GetBBox());
                            }
                        }
                    }
                }
                const bool bProcess = m_RainOccluders.m_nNumOccluders == 0;
                for (uint32 i = 0; i < numGPUs; ++i)
                {
                    m_RainOccluders.m_bProcessed[i] = bProcess;
                }
                m_RainInfo.bApplyOcclusion = m_RainOccluders.m_nNumOccluders > 0;

                geomBB.ClipToBox(bbArea);

                // Clip to ocean level
                if (OceanToggle::IsActive())
                {
                    if (OceanRequest::OceanIsEnabled())
                    {
                        geomBB.min.z = max(geomBB.min.z, OceanRequest::GetOceanLevel()) - 0.5f;
                    }
                }
                else
                {
                    geomBB.min.z = max(geomBB.min.z, gEnv->p3DEngine->GetWaterLevel()) - 0.5f;
                }

                float fWaterOffset = m_OceanInfo.m_fWaterLevel - geomBB.min.z;
                fWaterOffset = (float)fsel(fWaterOffset, fWaterOffset, 0.0f);

                geomBB.min.z += fWaterOffset - 0.5f;
                geomBB.max.z += fWaterOffset;

                Vec3 vSnappedCenter = bbArea.GetCenter() / rainBBHalfSize;
                vSnappedCenter.Set(floor_tpl(vSnappedCenter.x), floor_tpl(vSnappedCenter.y), floor_tpl(vSnappedCenter.z));
                vSnappedCenter *= rainBBHalfSize;

                AABB occBB(fOccArea);
                occBB.Move(vSnappedCenter);
                occBB.min.z = max(occBB.min.z, geomBB.min.z);
                occBB.max.z = min(occBB.max.z, geomBB.max.z);

                // Generate rotation matrix part-way from identity
                //  - Typical shadow filtering issues at grazing angles
                Quat qOcc = m_RainInfo.qRainRotation;
                qOcc.SetSlerp(qOcc, Quat::CreateIdentity(), 0.75f);
                Matrix44 matRot(Matrix33(qOcc.GetInverted()));

                // Get occlusion transformation matrix
                Matrix44& matOccTrans = m_RainInfo.matOccTrans;
                Matrix44 matScale;
                matOccTrans.SetIdentity();
                matOccTrans.SetTranslation(-occBB.min);
                matScale.SetIdentity();
                const Vec3 vScale(occBB.max - occBB.min);
                matScale.m00 = 1.f / vScale.x;
                matScale.m11 = 1.f / vScale.y;
                matScale.m22 = 1.f / vScale.z;
                matOccTrans = matRot * matScale * matOccTrans;
            }
        }

#if (defined(WIN32) || defined(WIN64)) && !defined(_RELEASE)
        if (m_RainOccluders.m_nNumOccluders >= nMAX_OCCLUDERS)
        {
            CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING,
                "Reached max rain occluder limit (Max: %i), some objects may have been discarded!", nMAX_OCCLUDERS);
        }
#endif

        m_RainOccluders.m_arrCurrOccluders[nThreadID].resize(m_RainOccluders.m_nNumOccluders);
        std::copy(arrOccluders.begin(), arrOccluders.begin() + m_RainOccluders.m_nNumOccluders, m_RainOccluders.m_arrCurrOccluders[nThreadID].begin());
    }

    bOldDisableOcclusion = bDisableOcclusion;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
namespace WaterVolumeStaticData {
    void GetMemoryUsage(ICrySizer* pSizer);
}

void CRenderer::GetMemoryUsage(ICrySizer* pSizer)
{
    // should be called by all derived classes
    for (uint32 i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
    {
        pSizer->AddObject(m_TextMessages[i]);
    }
    pSizer->AddObject(m_RP);
    pSizer->AddObject(m_pRT);
# if 0 //XXXXXXXXXXXXXXXXXXXXXXX
    pSizer->AddObject(m_DevBufMan);
# endif
    WaterVolumeStaticData::GetMemoryUsage(pSizer);
}

// retrieves the bandwidth calculations for the audio streaming
void CRenderer::GetBandwidthStats([[maybe_unused]] float* fBandwidthRequested)
{
#if !defined (_RELEASE)
    if (fBandwidthRequested)
    {
        *fBandwidthRequested = (CTexture::s_nBytesSubmittedToStreaming + CTexture::s_nBytesRequiredNotSubmitted) / 1024.0f;
    }
#endif
}


//////////////////////////////////////////////////////////////////////////
void CRenderer::SetTextureStreamListener([[maybe_unused]] ITextureStreamListener* pListener)
{
#ifdef ENABLE_TEXTURE_STREAM_LISTENER
    CTexture::s_pStreamListener = pListener;
#endif
}
//////////////////////////////////////////////////////////////////////////
float CRenderer::GetGPUFrameTime()
{
#if defined(CRY_USE_METAL) || defined(ANDROID)
    //  TODO: if this won't work consider different frame time calculation
    return gEnv->pTimer->GetRealFrameTime();
#else
#if 0
    //If we are CPU bound and the GPU is starved for data, this causes idle time on the GPU
    //between events, so the gpu timer events cannot be relied upon

    if (GpuTimerEvent::s_eventCount[GpuTimerEvent::s_readIdx] > 0)
    {
        LARGE_INTEGER ticksPerSecond;
        QueryPerformanceFrequency(&ticksPerSecond);

        //Grab GPU frame time - assume first event
        return GpuTimerEvent::s_events[GpuTimerEvent::s_readIdx][0].totalTime / (double)ticksPerSecond.QuadPart;
    }
    return 0.f;
#else
    int nThr = m_pRT->GetThreadList();
    float fGPUidle = m_fTimeGPUIdlePercent[nThr] * 0.01f; // normalise %
    float fGPUload = 1.0f - fGPUidle; // normalised non-idle time
    float fGPUtime = (m_fTimeProcessedGPU[nThr] * fGPUload); //GPU time in seconds
    return fGPUtime;
#endif
#endif
}

void CRenderer::GetRenderTimes(SRenderTimes& outTimes)
{
    int nThr = m_pRT->GetThreadList();
    //Query render times on main thread
    outTimes.fWaitForMain = m_fTimeWaitForMain[nThr];
    outTimes.fWaitForRender = m_fTimeWaitForRender[nThr];
    outTimes.fWaitForGPU = m_fTimeWaitForGPU[nThr];
    outTimes.fTimeProcessedRT = m_fTimeProcessedRT[nThr];
    outTimes.fTimeProcessedRTScene = m_RP.m_PS[nThr].m_fRenderTime;
    outTimes.fTimeProcessedGPU = m_fTimeProcessedGPU[nThr];
    outTimes.fTimeGPUIdlePercent = m_fTimeGPUIdlePercent[nThr];
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::PreShutDown()
{
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::PostShutDown()
{
    if (CTextureManager::InstanceExists())
        CTextureManager::Instance()->Release();
}

//////////////////////////////////////////////////////////////////////////

bool CRenderer::IsCustomRenderModeEnabled([[maybe_unused]] uint32 nRenderModeMask)
{
    assert(nRenderModeMask);

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CRenderer::IsPost3DRendererEnabled() const
{
    CPostEffectsMgr* pPostEffectMgr = PostEffectMgr();
    if (!pPostEffectMgr || !pPostEffectMgr->IsCreated())
    {
        return false;
    }

    CPostEffect* pPost3DRenderer = pPostEffectMgr->GetEffect(ePFX_Post3DRenderer);
    if (pPost3DRenderer)
    {
        return pPost3DRenderer->IsActive();
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::EF_SetPostEffectParam(const char* pParam, float fValue, bool bForceValue)
{
    if (pParam && m_RP.m_pREPostProcess)
    {
        m_RP.m_pREPostProcess->mfSetParameter(pParam, fValue, bForceValue);
    }
}

void CRenderer::EF_SetPostEffectParamVec4(const char* pParam, const Vec4& pValue, bool bForceValue)
{
    if (pParam && m_RP.m_pREPostProcess)
    {
        m_RP.m_pREPostProcess->mfSetParameterVec4(pParam, pValue, bForceValue);
    }
}


//////////////////////////////////////////////////////////////////////////
void CRenderer::EF_SetPostEffectParamString(const char* pParam, const char* pszArg)
{
    if (pParam && pszArg && m_RP.m_pREPostProcess)
    {
        m_RP.m_pREPostProcess->mfSetParameterString(pParam, pszArg);
    }
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::EF_GetPostEffectParam(const char* pParam, float& fValue)
{
    if (pParam && m_RP.m_pREPostProcess)
    {
        m_RP.m_pREPostProcess->mfGetParameter(pParam, fValue);
    }
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::EF_GetPostEffectParamVec4(const char* pParam, Vec4& pValue)
{
    if (pParam && m_RP.m_pREPostProcess)
    {
        m_RP.m_pREPostProcess->mfGetParameterVec4(pParam, pValue);
    }
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::EF_GetPostEffectParamString(const char* pParam, const char*& pszArg)
{
    if (pParam && m_RP.m_pREPostProcess)
    {
        m_RP.m_pREPostProcess->mfGetParameterString(pParam, pszArg);
    }
}

//////////////////////////////////////////////////////////////////////////
int32 CRenderer::EF_GetPostEffectID(const char* pPostEffectName)
{
    if (pPostEffectName && m_RP.m_pREPostProcess)
    {
        return m_RP.m_pREPostProcess->mfGetPostEffectID(pPostEffectName);
    }
    return ePFX_Invalid;
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::EF_ResetPostEffects(bool bOnSpecChange)
{
    m_pRT->RC_ResetPostEffects(bOnSpecChange);
}

void CRenderer::SyncPostEffects()
{
    if (PostEffectMgr())
    {
        PostEffectMgr()->SyncMainWithRender();
    }
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::EF_DisableTemporalEffects()
{
    m_pRT->RC_DisableTemporalEffects();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


void CRenderer::EF_AddWaterSimHit(const Vec3& vPos, const float scale, const float strength)
{
    CPostEffectsMgr* pPostEffectsMgr = PostEffectMgr();
    if (pPostEffectsMgr)
    {
        CWaterRipples* pWaterRipplesTech = (CWaterRipples*)pPostEffectsMgr->GetEffect(ePFX_WaterRipples);
        if (pWaterRipplesTech)
        {
            pWaterRipplesTech->AddHit(vPos, scale, strength);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CRenderer::EF_DrawWaterSimHits()
{
    if (CPostEffectsMgr* pPostEffectsMgr = PostEffectMgr())
    {
        if (CWaterRipples* pWaterRipplesTech = (CWaterRipples*)pPostEffectsMgr->GetEffect(ePFX_WaterRipples))
        {
            pWaterRipplesTech->DEBUG_DrawWaterHits();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void CRenderer::SetTexturePrecaching(bool stat)
{
    CTexture::s_bPrecachePhase = stat;
}

IOpticsElementBase* CRenderer::CreateOptics([[maybe_unused]] EFlareType type) const
{
#ifdef NULL_RENDERER
    return NULL;
#else
    return COpticsFactory::GetInstance()->Create(type);
#endif
}

void CRenderer::RT_UpdateLightVolumes([[maybe_unused]] int32 nFlags, [[maybe_unused]] int32 nRecurseLevel)
{
    AZ_TRACE_METHOD();
    gEnv->p3DEngine->GetLightVolumes(m_RP.m_nProcessThreadID, m_pLightVols, m_nNumVols);
}
//////////////////////////////////////////////////////////////////////////

SSkinningData* CRenderer::EF_CreateSkinningData(uint32 nNumBones, bool bNeedJobSyncVar, bool bUseMatrixSkinning)
{
    AZ_TRACE_METHOD();
    int nList = m_nPoolIndex % 3;

    uint32 skinningFlags = bUseMatrixSkinning ? eHWS_Skinning_Matrix : 0;

    uint32 nNeededSize = Align(sizeof(SSkinningData), 16);

    size_t boneSize = bUseMatrixSkinning ? sizeof(Matrix34) : sizeof(DualQuat);

    nNeededSize += Align(nNumBones * boneSize, 16);
    byte* pData = m_SkinningDataPool[nList].Allocate(nNeededSize);

    SSkinningData* pSkinningRenderData = alias_cast<SSkinningData*>(pData);
    pData += Align(sizeof(SSkinningData), 16);

    pSkinningRenderData->pAsyncJobExecutor = bNeedJobSyncVar ? m_jobExecutorPool.Allocate() : nullptr;
    pSkinningRenderData->pAsyncDataJobExecutor = bNeedJobSyncVar ? m_jobExecutorPool.Allocate() : nullptr;

    if (bUseMatrixSkinning)
    {
        pSkinningRenderData->pBoneMatrices = alias_cast<Matrix34*>(pData);
        pSkinningRenderData->pBoneQuatsS = nullptr;
    }
    else
    {
        pSkinningRenderData->pBoneQuatsS = alias_cast<DualQuat*>(pData);
        pSkinningRenderData->pBoneMatrices = nullptr;
    }

    pData += Align(nNumBones * boneSize, 16);

    pSkinningRenderData->pRemapTable = NULL;
    pSkinningRenderData->pCustomData = NULL;
    pSkinningRenderData->nNumBones = nNumBones;
    pSkinningRenderData->nHWSkinningFlags = skinningFlags;
    pSkinningRenderData->pPreviousSkinningRenderData = NULL;
    pSkinningRenderData->pCharInstCB = FX_AllocateCharInstCB(pSkinningRenderData, m_nPoolIndex);
    pSkinningRenderData->remapGUID = ~0u;

    pSkinningRenderData->pNextSkinningData = NULL;
    pSkinningRenderData->pMasterSkinningDataList = &pSkinningRenderData->pNextSkinningData;

    return pSkinningRenderData;
}

SSkinningData* CRenderer::EF_CreateRemappedSkinningData(uint32 nNumBones, SSkinningData* pSourceSkinningData, uint32 nCustomDataSize, uint32 pairGuid)
{
    assert(pSourceSkinningData);
    assert(pSourceSkinningData->nNumBones >= nNumBones); // don't try to remap more bones than exist

    int nList = m_nPoolIndex % 3;

    uint32 nNeededSize = Align(sizeof(SSkinningData), 16);
    nNeededSize += Align(nCustomDataSize, 16);

    byte* pData = m_SkinningDataPool[nList].Allocate(nNeededSize);

    SSkinningData* pSkinningRenderData = alias_cast<SSkinningData*>(pData);
    pData += Align(sizeof(SSkinningData), 16);

    pSkinningRenderData->pRemapTable = NULL;

    pSkinningRenderData->pCustomData = nCustomDataSize ? alias_cast<void*>(pData) : NULL;
    pData += nCustomDataSize ? Align(nCustomDataSize, 16) : 0;

    pSkinningRenderData->nNumBones = nNumBones;
    pSkinningRenderData->nHWSkinningFlags = pSourceSkinningData->nHWSkinningFlags;
    pSkinningRenderData->pPreviousSkinningRenderData = NULL;

    // use actual bone information from original skinning data
    pSkinningRenderData->pBoneQuatsS = pSourceSkinningData->pBoneQuatsS;
    pSkinningRenderData->pBoneMatrices = pSourceSkinningData->pBoneMatrices;
    pSkinningRenderData->pAsyncJobExecutor = pSourceSkinningData->pAsyncJobExecutor;
    pSkinningRenderData->pAsyncDataJobExecutor = pSourceSkinningData->pAsyncDataJobExecutor;

    pSkinningRenderData->pCharInstCB = pSourceSkinningData->pCharInstCB;

    pSkinningRenderData->remapGUID = pairGuid;
    pSkinningRenderData->pNextSkinningData = NULL;
    pSkinningRenderData->pMasterSkinningDataList = &pSourceSkinningData->pNextSkinningData;

    return pSkinningRenderData;
}

void CRenderer::RT_SetSkinningPoolId(uint32 poolId)
{
    m_nPoolIndexRT = poolId;
}

void CRenderer::EF_ClearSkinningDataPool()
{
    AZ_TRACE_METHOD();
    m_pRT->RC_PushSkinningPoolId(++m_nPoolIndex);
    m_SkinningDataPool[m_nPoolIndex % 3].ClearPool();
    FX_ClearCharInstCB(m_nPoolIndex);

    m_jobExecutorPool.AdvanceCurrent();
}

int CRenderer::EF_GetSkinningPoolID()
{
    return m_nPoolIndex;
}

// If the render pipeline has a reference to this shader item, set it to nullptr to avoid a dangling pointer
void CRenderer::ClearShaderItem(SShaderItem* pShaderItem)
{
    if (pShaderItem->m_pShaderResources && gRenDev->m_pRT && gRenDev->m_pRT->IsRenderThread())
    {
        CShaderResources* shaderResources = static_cast<CShaderResources*>(pShaderItem->m_pShaderResources);
        if (gRenDev->m_RP.m_pShaderResources == shaderResources)
        {
            gRenDev->m_RP.m_pShaderResources = nullptr;
        }

        for (int i = 0; i < EFTT_MAX; ++i)
        {
            SEfResTexture*      pTexture = shaderResources->GetTextureResource(i);
            if (gRenDev->m_RP.m_ShaderTexResources[i] == pTexture)
            {
                gRenDev->m_RP.m_ShaderTexResources[i] = nullptr;
            }
        }
/* @barled - adibugbug - put back in place!!!
        for (auto iter = shaderResources->m_TexturesResourcesMap.begin(); iter != shaderResources->m_TexturesResourcesMap.end(); ++iter)
        {
            SEfResTexture*      pTexture = &iter->second;
            uint16              nSlot = iter->first;
            if (gRenDev->m_RP.m_ShaderTexResources[nSlot] == pTexture)
            {   // Notice the usage of pointer for the comparison - should be revisited?
                gRenDev->m_RP.m_ShaderTexResources[nSlot] = nullptr;
            }
        }
*/
    }
}

void CRenderer::UpdateShaderItem(SShaderItem* pShaderItem, _smart_ptr<IMaterial> pMaterial)
{
    bool bShaderReloaded = false;
#if !defined(_RELEASE) || (defined(WIN32) || defined(WIN64))
    IShader* pShader = pShaderItem->m_pShader;
    if (pShader)
    {
        bShaderReloaded = (pShader->GetFlags() & EF_RELOADED) != 0;
    }
#endif

    if (pShaderItem->m_nPreprocessFlags == -1 || bShaderReloaded)
    {
        CRenderer::ForceUpdateShaderItem(pShaderItem, pMaterial);
    }
}

void CRenderer::RefreshShaderResourceConstants(SShaderItem* pShaderItem, IMaterial* pMaterial)
{
    m_pRT->RC_RefreshShaderResourceConstants(pShaderItem, pMaterial);
}

void CRenderer::ForceUpdateShaderItem(SShaderItem* pShaderItem, _smart_ptr<IMaterial> pMaterial)
{
    m_pRT->RC_UpdateShaderItem(pShaderItem, pMaterial);
}

void CRenderer::RT_UpdateShaderItem(SShaderItem* pShaderItem, IMaterial* material)
{
    AZ_TRACE_METHOD();
    CShader* pShader = static_cast<CShader*>(pShaderItem->m_pShader);
    if (pShader)
    {
        pShader->m_Flags &= ~EF_RELOADED;
        pShaderItem->Update();
    }

    if (material)
    {
        // We received a raw pointer to the material instead of a smart_ptr because writing/reading smart pointers from
        // the render thread queue causes the ref count to be increased incorrectly in some platforms (e.g. 32 bit architectures). 
        // In order to keep the material reference "alive" we manually increment the reference count and decrement it when we finish using it.
        material->AddRef();
        AZStd::function<void()> runOnMainThread = [material]()
        {
            // Updating the material flags after the shader has been parse and loaded.
            material->UpdateFlags();
            // Remove reference that we added.
            material->Release();
        };

        EBUS_QUEUE_FUNCTION(AZ::MainThreadRenderRequestBus, runOnMainThread);
    }
}

void CRenderer::RT_RefreshShaderResourceConstants(SShaderItem* shaderItem) const
{
    if (CShader* shader = static_cast<CShader*>(shaderItem->m_pShader))
    {
        if (shaderItem->RefreshResourceConstants())
        {
            shaderItem->m_pShaderResources->UpdateConstants(shader);
        }
    }
}

void CRenderer::GetClampedWindowSize(int& widthPixels, int& heightPixels)
{
    const int maxWidth = gEnv->pConsole->GetCVar("r_maxWidth")->GetIVal();
    const int maxHeight = gEnv->pConsole->GetCVar("r_maxheight")->GetIVal();

    if (maxWidth > 0 && maxWidth < widthPixels)
    {
        const float widthScaleFactor = static_cast<float>(maxWidth) / static_cast<float>(widthPixels);
        widthPixels = aznumeric_cast<int>(widthPixels * widthScaleFactor);
        heightPixels = aznumeric_cast<int>(heightPixels * widthScaleFactor); 
    }

    if (maxHeight > 0 && maxHeight < heightPixels)
    {
        const float heightScaleFactor = static_cast<float>(maxHeight) / static_cast<float>(heightPixels);
        widthPixels = aznumeric_cast<int>(widthPixels * heightScaleFactor);
        heightPixels = aznumeric_cast<int>(heightPixels * heightScaleFactor);
    }
}

CRenderView* CRenderer::GetRenderViewForThread(int nThreadID)
{
    return m_RP.m_pRenderViews[nThreadID].get();
}

bool CRenderer::UseHalfFloatRenderTargets()
{
#if defined(OPENGL_ES) && !defined(AZ_PLATFORM_LINUX)
    // When using OpenGL ES on Linux, DXGL_GL_EXTENSION_SUPPORTED(EXT_color_buffer_half_float) returns false,
    // however with at least the Mesa driver, half float render targets are natively supported.
    return RenderCapabilities::SupportsHalfFloatRendering() && (!CRenderer::CV_r_ForceFixedPointRenderTargets);
#else
    return true;
#endif
}

Matrix44A CRenderer::GetCameraMatrix()
{
    return m_CameraMatrix;
}

void CRenderer::SyncMainWithRender()
{
    const uint numListeners = m_syncMainWithRenderListeners.size();
    for (uint i = 0; i < numListeners; ++i)
    {
        m_syncMainWithRenderListeners[i]->SyncMainWithRender();
    }
}

void CRenderer::RegisterSyncWithMainListener(ISyncMainWithRenderListener* pListener)
{
    stl::push_back_unique(m_syncMainWithRenderListeners, pListener);
}

void CRenderer::RemoveSyncWithMainListener(const ISyncMainWithRenderListener* pListener)
{
    stl::find_and_erase(m_syncMainWithRenderListeners, pListener);
}

void CRenderer::UpdateCachedShadowsLodCount([[maybe_unused]] int nGsmLods) const
{
    OnChange_CachedShadows(NULL);
}

ITexture* CRenderer::GetWhiteTexture()
{
    return CTextureManager::Instance()->GetWhiteTexture();
}

ITexture* CRenderer::GetTextureForName(const char* name, uint32 nFlags, ETEX_Format eFormat)
{
    return CTexture::ForName(name, nFlags, eFormat);
}

int CRenderer::GetRecursionLevel()
{
    return SRendItem::m_RecurseLevel[GetRenderPipeline()->m_nProcessThreadID];
}

int CRenderer::GetIntegerConfigurationValue(const char* varName, int defaultValue)
{
    ICVar* var = varName ? gEnv->pConsole->GetCVar(varName) : nullptr;
    AZ_Assert(var, "Unable to find cvar: %s", varName)
    return var ? var->GetIVal() : defaultValue;
}

float CRenderer::GetFloatConfigurationValue(const char* varName, float defaultValue)
{
    ICVar* var = varName ? gEnv->pConsole->GetCVar(varName) : nullptr;
    AZ_Assert(var, "Unable to find cvar: %s", varName)
    return var ? var->GetFVal() : defaultValue;
}

bool CRenderer::GetBooleanConfigurationValue(const char* varName, bool defaultValue)
{
    ICVar* var = varName ? gEnv->pConsole->GetCVar(varName) : nullptr;
    AZ_Assert(var, "Unable to find cvar: %s", varName)
    return var ? (var->GetIVal() != 0) : defaultValue;
}

// Methods exposed to external libraries
void CRenderer::ApplyDepthTextureState(int unit, int nFilter, bool clamp)
{
    CTexture::ApplyDepthTextureState(unit, nFilter, clamp);
}

ITexture* CRenderer::GetZTargetTexture()
{
    return CTexture::GetZTargetTexture();
}

int CRenderer::GetTextureState(const STexState& TS)
{
    return CTexture::GetTextureState(TS);
}

uint32 CRenderer::TextureDataSize(uint32 nWidth, uint32 nHeight, uint32 nDepth, uint32 nMips, uint32 nSlices, const ETEX_Format eTF, ETEX_TileMode eTM)
{
    return CTexture::TextureDataSize(nWidth, nHeight, nDepth, nMips, nSlices, eTF, eTM);
}

void CRenderer::ApplyForID(int nID, int nTUnit, int nTState, int nTexMaterialSlot, int nSUnit, bool useWhiteDefault)
{
    CTexture::ApplyForID(nID, nTUnit, nTState, nTexMaterialSlot, nSUnit, useWhiteDefault);
}

ITexture* CRenderer::Create3DTexture(const char* szName, int nWidth, int nHeight, int nDepth, int nMips, int nFlags, const byte* pData, ETEX_Format eTFSrc, ETEX_Format eTFDst)
{
    return CTexture::Create3DTexture(szName, nWidth, nHeight, nDepth, nMips, nFlags, pData, eTFSrc, eTFDst);
}

bool CRenderer::IsTextureExist(const ITexture* pTex)
{
    return CTexture::IsTextureExist(pTex);
}

const char* CRenderer::NameForTextureFormat(ETEX_Format eTF)
{
    return CTexture::NameForTextureFormat(eTF);
}

const char* CRenderer::NameForTextureType(ETEX_Type eTT)
{
    return CTexture::NameForTextureType(eTT);
}

bool CRenderer::IsVideoThreadModeEnabled()
{
    return m_pRT->m_eVideoThreadMode != SRenderThread::eVTM_Disabled;
}

IDynTexture* CRenderer::CreateDynTexture2(uint32 nWidth, uint32 nHeight, uint32 nTexFlags, const char* szSource, ETexPool eTexPool)
{
    return new SDynTexture2(nWidth, nHeight, nTexFlags, szSource, eTexPool);
}

uint32 CRenderer::GetCurrentTextureAtlasSize()
{
    return SDynTexture::s_CurTexAtlasSize;
}

void CRenderer::RT_InitializeVideoRenderer(AZ::VideoRenderer::IVideoRenderer* pVideoRenderer)
{
    AZ_Assert(pVideoRenderer != nullptr, "Expected video player to be passed in");

    AZ::VideoRenderer::VideoTexturesDesc videoTexturesDesc;
    if (pVideoRenderer && pVideoRenderer->GetVideoTexturesDesc(videoTexturesDesc))
    {
        AZ::VideoRenderer::VideoTextures videoTextures;

        auto InitVideoTexture = [](const AZ::VideoRenderer::VideoTextureDesc& videoTextureDesc, uint32 flags) -> uint32
        {
            uint32 resultTextureId = 0;
            if (videoTextureDesc.m_used)
            {
                CTexture* pCreatedPlaneTexture = CTexture::Create2DTexture(videoTextureDesc.m_name, videoTextureDesc.m_width, videoTextureDesc.m_height, 1, flags, nullptr, videoTextureDesc.m_format, videoTextureDesc.m_format);
                if (pCreatedPlaneTexture != nullptr)
                {
                    resultTextureId = pCreatedPlaneTexture->GetTextureID();
                }
            }
            return resultTextureId;
        };

        videoTextures.m_outputTextureId = InitVideoTexture(videoTexturesDesc.m_outputTextureDesc, FT_USAGE_RENDERTARGET);

        for (uint32 videoIndex = 0; videoIndex < AZ::VideoRenderer::MaxInputTextureCount; videoIndex++)
        {
            videoTextures.m_inputTextureIds[videoIndex] = InitVideoTexture(videoTexturesDesc.m_inputTextureDescs[videoIndex], 0);
        }

        pVideoRenderer->NotifyTexturesCreated(videoTextures);
    }
}

void CRenderer::RT_CleanupVideoRenderer(AZ::VideoRenderer::IVideoRenderer* pVideoRenderer)
{
    AZ_Assert(pVideoRenderer != nullptr, "Expected video player to be passed in");

    AZ::VideoRenderer::VideoTextures videoTextures;
    if (pVideoRenderer && pVideoRenderer->GetVideoTextures(videoTextures))
    {
        auto ReleaseVideoTexture = [](uint32 textureId)
        {
            if (textureId != 0)
            {
                if (CTexture* pTexture = CTexture::GetByID(textureId))
                {
                    pTexture->Release();
                }
            }
        };

        ReleaseVideoTexture(videoTextures.m_outputTextureId);

        for (uint32 textureId : videoTextures.m_inputTextureIds)
        {
            ReleaseVideoTexture(textureId);
        }

        pVideoRenderer->NotifyTexturesDestroyed();
    }
}

#ifndef _RELEASE
int CRenderer::GetDrawCallsPerNode(IRenderNode* pRenderNode)
{
    uint32 t = m_RP.m_nFillThreadID;
    IRenderer::RNDrawcallsMapNodeItor iter = m_RP.m_pRNDrawCallsInfoPerNode[t].find(pRenderNode);
    if (iter != m_RP.m_pRNDrawCallsInfoPerNode[t].end())
    {
        SDrawCallCountInfo& pInfo = iter->second;
        uint32 nDrawcalls = pInfo.nShadows + pInfo.nZpass + pInfo.nGeneral + pInfo.nTransparent + pInfo.nMisc;
        return nDrawcalls;
    }

    return 0;
}
#endif

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
bool CRenderer::IsRenderToTextureActive() const
{
    return ((m_RP.m_TI[m_pRT->GetThreadList()].m_PersFlags & RBPF_RENDER_SCENE_TO_TEXTURE) != 0);
}

int CRenderer::GetWidth() const
{
    return IsRenderToTextureActive() ? m_RP.m_pRenderViews[m_pRT->GetThreadList()]->GetWidth() : m_width;
}

void CRenderer::SetWidth(int width)
{
    if (IsRenderToTextureActive())
    {
        m_RP.m_pRenderViews[m_pRT->GetThreadList()]->SetWidth(width);
    }
    else
    {
        m_width = width;
    }
}

int CRenderer::GetHeight() const
{
    return IsRenderToTextureActive() ? m_RP.m_pRenderViews[m_pRT->GetThreadList()]->GetHeight() : m_height;
}

void CRenderer::SetHeight(int height)
{
    if (IsRenderToTextureActive())
    {
        m_RP.m_pRenderViews[m_pRT->GetThreadList()]->SetHeight(height);
    }
    else
    {
        m_height = height;
    }
}

int CRenderer::GetOverlayWidth() const
{
    return IsRenderToTextureActive() ? m_RP.m_pRenderViews[m_pRT->GetThreadList()]->GetWidth() : m_nativeWidth;
}

int CRenderer::GetOverlayHeight() const
{
    return IsRenderToTextureActive() ? m_RP.m_pRenderViews[m_pRT->GetThreadList()]->GetHeight() : m_nativeHeight;
}

#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
