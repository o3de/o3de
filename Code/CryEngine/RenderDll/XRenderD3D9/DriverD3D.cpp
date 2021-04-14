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

#include "RenderDll_precompiled.h"

#include "DriverD3D.h"
#include "I3DEngine.h"
#include "AnimKey.h"

#include <AzCore/Math/MathUtils.h>
#include <IEngineModule.h>
#include <CryExtension/Impl/ClassWeaver.h>

#include "../Common/Textures/TextureManager.h"
#include "../Common/Textures/TextureStreamPool.h"
#include "../Common/ReverseDepth.h"
#include "D3DStereo.h"
#include "D3DPostProcess.h"
#include "MultiLayerAlphaBlendPass.h"

#include "ScopeGuard.h"
#include "IImageHandler.h"
#include <CryPath.h>
#include <Pak/CryPakUtils.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define DRIVERD3D_CPP_SECTION_1 1
#define DRIVERD3D_CPP_SECTION_3 3
#define DRIVERD3D_CPP_SECTION_4 4
#define DRIVERD3D_CPP_SECTION_5 5
#define DRIVERD3D_CPP_SECTION_6 6
#define DRIVERD3D_CPP_SECTION_7 7
#define DRIVERD3D_CPP_SECTION_8 8
#define DRIVERD3D_CPP_SECTION_9 9
#define DRIVERD3D_CPP_SECTION_10 10
#define DRIVERD3D_CPP_SECTION_11 11
#define DRIVERD3D_CPP_SECTION_12 12
#define DRIVERD3D_CPP_SECTION_13 13
#define DRIVERD3D_CPP_SECTION_14 14
#define DRIVERD3D_CPP_SECTION_15 15
#define DRIVERD3D_CPP_SECTION_16 16
#endif

#if defined(FEATURE_SVO_GI)
#include "D3D_SVO.h"
#endif

#if (defined (WIN32) || defined(WIN64)) && !defined(OPENGL)
#   include <D3Dcompiler.h>
#endif

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#if RENDERER_ENABLE_BREAK_ON_ERROR
#include <d3d9.h>
namespace detail
{
    const char* ToString(long const hr)
    {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(DriverD3D_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
# else
        if (D3DOK_NOAUTOGEN == hr)
            return "D3DOK_NOAUTOGEN This is a success code. However, the autogeneration of mipmaps is not supported for this format. This means that resource creation will succeed but the mipmap levels will not be automatically generated";
        else if (D3DERR_CONFLICTINGRENDERSTATE == hr)
            return "D3DERR_CONFLICTINGRENDERSTATE The currently set render states cannot be used together";
        else if (D3DERR_CONFLICTINGTEXTUREFILTER == hr)
            return "D3DERR_CONFLICTINGTEXTUREFILTER The current texture filters cannot be used together";
        else if (D3DERR_CONFLICTINGTEXTUREPALETTE == hr)
            return "D3DERR_CONFLICTINGTEXTUREPALETTE The current textures cannot be used simultaneously.";
        else if (D3DERR_DEVICEHUNG == hr)
            return "D3DERR_DEVICEHUNG The device that returned this code caused the hardware adapter to be reset by the OS. Most applications should destroy the device and quit. Applications that must continue should destroy all video memory objects (surfaces, textures, state blocks etc) and call Reset() to put the device in a default state. If the application then continues rendering in the same way, the device will return to this state";
        else if (D3DERR_DEVICELOST == hr)
            return "D3DERR_DEVICELOST The device has been lost but cannot be reset at this time. Therefore, rendering is not possible.A Direct 3D device object other than the one that returned this code caused the hardware adapter to be reset by the OS. Delete all video memory objects (surfaces, textures, state blocks) and call Reset() to return the device to a default state. If the application continues rendering without a reset, the rendering calls will succeed.";
        else if (D3DERR_DEVICENOTRESET == hr)
            return "D3DERR_DEVICENOTRESET The device has been lost but can be reset at this time.";
        else if (D3DERR_DEVICEREMOVED == hr)
            return "D3DERR_DEVICEREMOVED The hardware adapter has been removed. Application must destroy the device, do enumeration of adapters and create another Direct3D device. If application continues rendering without calling Reset, the rendering calls will succeed";
        else if (D3DERR_DRIVERINTERNALERROR == hr)
            return "D3DERR_DRIVERINTERNALERROR Internal driver error. Applications should destroy and recreate the device when receiving this error. For hints on debugging this error, see Driver Internal Errors (Direct3D 9).";
        else if (D3DERR_DRIVERINVALIDCALL == hr)
            return "D3DERR_DRIVERINVALIDCALL Not used.";
        else if (D3DERR_INVALIDCALL == hr)
            return "D3DERR_INVALIDCALL The method call is invalid. For example, a method's parameter may not be a valid pointer.";
        else if (D3DERR_INVALIDDEVICE == hr)
            return "D3DERR_INVALIDDEVICE The requested device type is not valid.";
        else if (D3DERR_MOREDATA == hr)
            return "D3DERR_MOREDATA There is more data available than the specified buffer size can hold.";
        else if (D3DERR_NOTAVAILABLE == hr)
            return "D3DERR_NOTAVAILABLE This device does not support the queried technique.";
        else if (D3DERR_NOTFOUND == hr)
            return "D3DERR_NOTFOUND The requested item was not found.";
        else if (D3D_OK == hr)
            return "D3D_OK No error occurred.";
        else if (D3DERR_OUTOFVIDEOMEMORY == hr)
            return "D3DERR_OUTOFVIDEOMEMORY Direct3D does not have enough display memory to perform the operation. The device is using more resources in a single scene than can fit simultaneously into video memory. Present, PresentEx, or CheckDeviceState can return this error. Recovery is similar to D3DERR_DEVICEHUNG, though the application may want to reduce its per-frame memory usage as well to avoid having the error recur.";
        else if (D3DERR_TOOMANYOPERATIONS == hr)
            return "D3DERR_TOOMANYOPERATIONS The application is requesting more texture-filtering operations than the device supports.";
        else if (D3DERR_UNSUPPORTEDALPHAARG == hr)
            return "D3DERR_UNSUPPORTEDALPHAARG The device does not support a specified texture-blending argument for the alpha channel.";
        else if (D3DERR_UNSUPPORTEDALPHAOPERATION == hr)
            return "D3DERR_UNSUPPORTEDALPHAOPERATION The device does not support a specified texture-blending operation for the alpha channel.";
        else if (D3DERR_UNSUPPORTEDCOLORARG == hr)
            return "D3DERR_UNSUPPORTEDCOLORARG The device does not support a specified texture-blending argument for color values.";
        else if (D3DERR_UNSUPPORTEDCOLOROPERATION == hr)
            return "D3DERR_UNSUPPORTEDCOLOROPERATION The device does not support a specified texture-blending operation for color values.";
        else if (D3DERR_UNSUPPORTEDFACTORVALUE == hr)
            return "D3DERR_UNSUPPORTEDFACTORVALUE The device does not support the specified texture factor value. Not used; provided only to support older drivers.";
        else if (D3DERR_UNSUPPORTEDTEXTUREFILTER == hr)
            return "D3DERR_UNSUPPORTEDTEXTUREFILTER The device does not support the specified texture filter.";
        else if (D3DERR_WASSTILLDRAWING == hr)
            return "D3DERR_WASSTILLDRAWING The previous blit operation that is transferring information to or from this surface is incomplete.";
        else if (D3DERR_WRONGTEXTUREFORMAT == hr)
            return "D3DERR_WRONGTEXTUREFORMAT The pixel format of the texture surface is not valid.";
        else if (E_FAIL == hr)
            return "E_FAIL An undetermined error occurred inside the Direct3D subsystem.";
        else if (E_INVALIDARG == hr)
            return "E_INVALIDARG An invalid parameter was passed to the returning function.";
        else if (E_NOINTERFACE == hr)
            return "E_NOINTERFACE No object interface is available.";
        else if (E_NOTIMPL == hr)
            return "E_NOTIMPL Not implemented.";
        else if (E_OUTOFMEMORY == hr)
            return "E_OUTOFMEMORY Direct3D could not allocate sufficient memory to complete the call.";
        else if (D3DERR_UNSUPPORTEDOVERLAY == hr)
            return "D3DERR_UNSUPPORTEDOVERLAY The device does not support overlay for the specified size or display mode.";
        else if (D3DERR_UNSUPPORTEDOVERLAYFORMAT == hr)
            return "D3DERR_UNSUPPORTEDOVERLAYFORMAT The device does not support overlay for the specified surface format.";
        else if (D3DERR_CANNOTPROTECTCONTENT == hr)
            return "D3DERR_CANNOTPROTECTCONTENT The specified content cannot be protected";
        else if (D3DERR_UNSUPPORTEDCRYPTO == hr)
            return "D3DERR_UNSUPPORTEDCRYPTO The specified cryptographic algorithm is not supported.";
        else if (DXGI_ERROR_DEVICE_REMOVED == hr)
        {
            HRESULT subResult = gcpRendD3D->GetDevice().GetDeviceRemovedReason();
            if (DXGI_ERROR_DEVICE_HUNG == subResult)
            {
                return "DXGI_ERROR_DEVICE_HUNG. The device was removed as it hung";
            }
            else if (DXGI_ERROR_DEVICE_REMOVED == subResult)
            {
                return "DXGI_ERROR_DEVICE_REMOVED. The device was removed";
            }
            else if (DXGI_ERROR_DEVICE_RESET == subResult)
            {
                return "DXGI_ERROR_DEVICE_RESET. The device was reset";
            }
            else if (DXGI_ERROR_DRIVER_INTERNAL_ERROR == subResult)
            {
                return "DXGI_ERROR_DRIVER_INTERNAL_ERROR. The device was removed due to an internal error";
            }
            else if (DXGI_ERROR_INVALID_CALL == subResult)
            {
                return "DXGI_ERROR_INVALID_CALL. The device was removed due to an invalid call";
            }
        }
#endif
        return "Unknown HRESULT CODE!";
    }
    bool CheckHResult(long const hr
        , bool breakOnError
        , const char* file
        , const int line)
    {
        IF (hr == S_OK, 1)
        {
            return true;
        }
        CryLogAlways("%s(%d): d3d error: '%s'", file, line, ToString(hr));
        IF (breakOnError, 0)
        {
            __debugbreak();
        }
        return false;
    }
}
#endif

void CD3D9Renderer::LimitFramerate(const int maxFPS, const bool bUseSleep)
{
    FRAME_PROFILER("RT_FRAME_CAP", gEnv->pSystem, PROFILE_RENDERER);

    if (maxFPS > 0)
    {
        CTimeValue timeFrameMax;
        const float safeMarginFPS = 0.5f;//save margin to not drop below 30 fps
        static CTimeValue sTimeLast = gEnv->pTimer->GetAsyncTime();

        timeFrameMax.SetMilliSeconds((int64)(1000.f / ((float)maxFPS + safeMarginFPS)));
        const CTimeValue timeLast = timeFrameMax + sTimeLast;
        while (timeLast.GetValue() > gEnv->pTimer->GetAsyncTime().GetValue())
        {
            if (bUseSleep)
            {
                CrySleep(1);
            }
            else
            {
                volatile int i = 0;
                while (i++ < 1000)
                {
                    ;
                }
            }
        }

        sTimeLast = gEnv->pTimer->GetAsyncTime();
    }
}


CCryNameTSCRC CTexture::s_sClassName = CCryNameTSCRC("CTexture");

CCryNameTSCRC CHWShader::s_sClassNameVS = CCryNameTSCRC("CHWShader_VS");
CCryNameTSCRC CHWShader::s_sClassNamePS = CCryNameTSCRC("CHWShader_PS");

CCryNameTSCRC CShader::s_sClassName = CCryNameTSCRC("CShader");

StaticInstance<CD3D9Renderer> gcpRendD3D;

int CD3D9Renderer::CV_d3d11_CBUpdateStats;
ICVar* CD3D9Renderer::CV_d3d11_forcedFeatureLevel;
int CD3D9Renderer::CV_r_AlphaBlendLayerCount;

#if defined(SUPPORT_D3D_DEBUG_RUNTIME)
int CD3D9Renderer::CV_d3d11_debugruntime;
ICVar* CD3D9Renderer::CV_d3d11_debugMuteSeverity;
ICVar* CD3D9Renderer::CV_d3d11_debugMuteMsgID;
ICVar* CD3D9Renderer::CV_d3d11_debugBreakOnMsgID;
int CD3D9Renderer::CV_d3d11_debugBreakOnce;
#endif

const char* resourceName[] = {
    "UNKNOWN",
    "Surfaces",
    "Volumes",
    "Textures",
    "Volume Textures",
    "Cube Textures",
    "Vertex Buffers",
    "Index Buffers"
};

#if defined(SUPPORT_D3D_DEBUG_RUNTIME)
static void OnChange_CV_d3d11_debugMuteMsgID(ICVar* /*pCVar*/)
{
    gcpRendD3D->m_bUpdateD3DDebug = true;
}
#endif



bool QueryIsFullscreen()
{
    return gcpRendD3D->IsFullscreen();
}

unsigned int CD3D9Renderer::GetCurrentBackBufferIndex([[maybe_unused]] IDXGISwapChain* pSwapChain)
{
    unsigned index = 0;

#ifdef CRY_USE_DX12
    DXGISwapChain* dxgiSwapChain;
    pSwapChain->QueryInterface(__uuidof(DXGISwapChain), reinterpret_cast<void**>(&dxgiSwapChain));
    if (dxgiSwapChain)
    {
        index = dxgiSwapChain->GetCurrentBackBufferIndex();
        dxgiSwapChain->Release();
    }
#endif

    return index;
}


// Direct 3D console variables
CD3D9Renderer::CD3D9Renderer()
    : m_DeviceOwningthreadID(0)
    , m_nMaxRT2Commit(-1)
#if defined(WIN32)
    , m_bDisplayChanged(false)
    , m_nConnectedMonitors(1)
#endif // defined(WIN32)
{
    m_screenshotFilepathCache[0] = '\0';

    m_bDraw2dImageStretchMode = false;
    m_techShadowGen = CCryNameTSCRC("ShadowGen");

    CreateDeferredUnitBox(m_arrDeferredInds, m_arrDeferredVerts);
}

void CD3D9Renderer::InitRenderer()
{
    CRenderer::InitRenderer();

    memset(m_arrWireFrameStack, 0, sizeof(m_arrWireFrameStack));
    m_nWireFrameStack = 0;

    m_uLastBlendFlagsPassGroup = 0xFFFFFFFF;
    m_bInitialized = false;
    gRenDev = this;

    m_pSecondBackBuffer = NULL;
    m_pStereoRenderer = new CD3DStereoRenderer(*this, (EStereoDevice)CRenderer::CV_r_StereoDevice);
    m_bDualStereoSupport = CRenderer::CV_r_StereoDevice > 0;
    m_GraphicsPipeline = new CStandardGraphicsPipeline();
    m_pTiledShading = new CTiledShading();

    m_pPipelineProfiler = NULL;
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_CPP_SECTION_3
    #include AZ_RESTRICTED_FILE(DriverD3D_cpp)
#endif

#if defined(ENABLE_PROFILING_GPU_TIMERS)
    m_pPipelineProfiler = new CRenderPipelineProfiler();
#endif

    m_logFileHandle = AZ::IO::InvalidHandle;

#if defined(ENABLE_PROFILING_CODE)
    for (unsigned int i = 0; i < MAXFRAMECAPTURECALLBACK; i++)
    {
        m_pCaptureCallBack[i] = NULL;
    }
    m_frameCaptureRegisterNum = 0;
    m_captureFlipFlop = 0;
    m_pSaveTexture[0] = NULL;
    m_pSaveTexture[1] = NULL;

    for (int i = 0; i < RT_COMMAND_BUF_COUNT; i++)
    {
        m_nScreenCaptureRequestFrame[i] = 0;
        m_screenCapTexHandle[i] = 0;
    }
#endif

    m_nCurStateBL = (uint32) - 1;
    m_nCurStateRS = (uint32) - 1;
    m_nCurStateDP = (uint32) - 1;
    m_CurTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    m_dwPresentStatus = 0;

    m_hWnd = NULL;
    m_hWnd2             = NULL;
#ifdef WIN32
    m_hIconBig          = NULL;
    m_hIconSmall        = NULL;
#endif

    m_numOcclusionDownsampleStages = 1;
    m_occlusionSourceSizeX = 0;
    m_occlusionSourceSizeY = 0;

    m_PerInstanceConstantBufferPool.Init();

    m_lockCharCB = 0;
    m_CharCBFrameRequired[0] = m_CharCBFrameRequired[1] = m_CharCBFrameRequired[2] = 0;
    m_CharCBAllocated = 0;

    for (int i = 0; i < SHAPE_MAX; i++)
    {
        m_pUnitFrustumVB[i] = NULL;
        m_pUnitFrustumIB[i] = NULL;
    }

    m_pQuadVB = NULL;
    m_nQuadVBSize = 0;

    m_2dImages.reserve(32);

    m_pZBufferReadOnlyDSV = NULL;
    m_pZBufferDepthReadOnlySRV = NULL;
    m_pZBufferStencilReadOnlySRV = NULL;

    m_Features = RFT_SUPPORTZBIAS;

    REGISTER_CVAR2("d3d11_CBUpdateStats", &CV_d3d11_CBUpdateStats, 0, 0, "Logs constant buffer updates statistics.");
    CV_d3d11_forcedFeatureLevel = REGISTER_STRING("d3d11_forcedFeatureLevel", NULL, VF_DEV_ONLY | VF_REQUIRE_APP_RESTART,
            "Forces the Direct3D device to target a specific feature level - supported values are:\n"
            " 10.0\n"
            " 10.1\n"
            " 11.0");

#if defined(SUPPORT_D3D_DEBUG_RUNTIME)
    REGISTER_CVAR2("d3d11_debugruntime", &CV_d3d11_debugruntime, 0, 0, "Avoid D3D debug runtime errors for certain cases");
    CV_d3d11_debugMuteSeverity = REGISTER_INT("d3d11_debugMuteSeverity", 2, VF_NULL,
            "Mute whole group of messages of certain severity when D3D debug runtime enabled:\n"
            " 0 - no severity based mute\n"
            " 1 - mute INFO only\n"
            " 2 - mute INFO and WARNING (default)\n"
            " 3 - mute INFO, WARNING and ERROR\n"
            " 4 - mute all (INFO, WARNING, ERROR, CORRUPTION)");
    CV_d3d11_debugMuteMsgID = REGISTER_STRING("d3d11_debugMuteMsgID", "388", VF_NULL,
            "List of D3D debug runtime messages to mute (see DirectX Control Panel for full message ID list)\n"
            "Use space separated list of IDs, eg. '388 10 544'");
    CV_d3d11_debugBreakOnMsgID = REGISTER_STRING("d3d11_debugBreakOnMsgID", "0", VF_NULL,
            "List of D3D debug runtime messages to break on.\nUsage:\n"
            " 0                    - no break (default)\n"
            " msgID1 msgID2 msgID3 - break whenever a message with one of given IDs occurs\n"
            " -1                   - break on any error or corruption message");
    REGISTER_CVAR2("d3d11_debugBreakOnce", &CV_d3d11_debugBreakOnce, 1, VF_NULL,
        "If enabled, D3D debug runtime break on message/error will be enabled only for 1 frame since last change.\n");

    CV_d3d11_debugMuteSeverity->SetOnChangeCallback(OnChange_CV_d3d11_debugMuteMsgID);
    CV_d3d11_debugMuteMsgID->SetOnChangeCallback(OnChange_CV_d3d11_debugMuteMsgID);
    CV_d3d11_debugBreakOnMsgID->SetOnChangeCallback(OnChange_CV_d3d11_debugMuteMsgID);
#endif

    REGISTER_CVAR3("r_AlphaBlendLayerCount", CV_r_AlphaBlendLayerCount, 0, VF_RENDERER_CVAR,
        "Set the number of layers to use for alpha blending to allow for order independent transparency.\n"
        "0: disabled\n"
        "1: Use an extra alpha depth check to over or under blend with existing alpha providing more accuracy.\n"
        "2-4: Use additional layers to gaurantee correct alpha n layers deep."
        );

#if defined(ENABLE_RENDER_AUX_GEOM)
    m_pRenderAuxGeomD3D = 0;
    if (CV_r_enableauxgeom)
    {
        m_pRenderAuxGeomD3D = CRenderAuxGeomD3D::Create(*this);
    }
#endif
    m_pColorGradingControllerD3D = new CColorGradingControllerD3D(this);

    CV_capture_frames = 0;
    CV_capture_folder = 0;
    CV_capture_buffer = 0;

    m_NewViewport.fMinZ = 0;
    m_NewViewport.fMaxZ = 1;

    m_wireframe_mode = R_SOLID_MODE;

#ifdef SHADER_ASYNC_COMPILATION
    uint32 nThreads = CV_r_shadersasyncmaxthreads; //clamp_tpl(CV_r_shadersasyncmaxthreads, 1, 4);
    uint32 nOldThreads = m_AsyncShaderTasks.size();
    for (uint32 a = nThreads; a < nOldThreads; a++)
    {
        delete m_AsyncShaderTasks[a];
    }
    m_AsyncShaderTasks.resize(nThreads);
    for (uint32 a = nOldThreads; a < nThreads; a++)
    {
        m_AsyncShaderTasks[a] = new CAsyncShaderTask();
    }
    for (int32 i = 0; i < m_AsyncShaderTasks.size(); i++)
    {
        m_AsyncShaderTasks[i]->SetThread(i);
    }
#endif

#if !defined(NULL_RENDERER)
    m_OcclQueriesUsed = 0;
#endif

    m_pPostProcessMgr = 0;
    m_pWaterSimMgr = 0;

    m_gmemDepthStencilMode = eGDSM_Invalid;
    AZ::RenderNotificationsBus::Handler::BusConnect();
    AZ::RenderScreenshotRequestBus::Handler::BusConnect();

    // Ensure tiled shading is disabled on initialization for OpenGL as HLSLcc does not properly support our tiled lighting shaders
    if (gRenDev->GetRenderType() == eRT_OpenGL)
    {
        gRenDev->CV_r_DeferredShadingTiled = 0;
    }
}

CD3D9Renderer::~CD3D9Renderer()
{
    //Code moved to Release
}

void CD3D9Renderer::StaticCleanup()
{
    stl::free_container(s_tempRIs);
    stl::free_container(s_tempObjects[0]);
    stl::free_container(s_tempObjects[1]);
}

void CD3D9Renderer::Release()
{

    AZ::RenderNotificationsBus::Handler::BusDisconnect();
    AZ::RenderScreenshotRequestBus::Handler::BusDisconnect();
    //FreeResources(FRR_ALL);
    ShutDown();

#if defined(ENABLE_PROFILING_CODE)
    SAFE_RELEASE(m_pSaveTexture[0]);
    SAFE_RELEASE(m_pSaveTexture[1]);
#endif

#ifdef SHADER_ASYNC_COMPILATION
    for (int32 a = 0; a < m_AsyncShaderTasks.size(); a++)
    {
        delete m_AsyncShaderTasks[a];
    }
#endif

    m_2dImages.clear();

    CRenderer::Release();

    DestroyWindow();
}

void CD3D9Renderer::Reset(void)
{
    m_pRT->RC_ResetDevice();
}
void CD3D9Renderer::RT_Reset(void)
{
    if (CheckDeviceLost())
    {
        return;
    }
    m_bDeviceLost = 1;
    //iLog->Log("...Reset");
    RestoreGamma();
    m_bDeviceLost = 0;
    m_MSAA = 0;
    if (m_bFullScreen)
    {
        SetGamma(CV_r_gamma + m_fDeltaGamma, CV_r_brightness, CV_r_contrast, false);
    }
}

void CD3D9Renderer::ChangeViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height, bool bMainViewport, float scaleWidth, float scaleHeight)
{
    if (m_bDeviceLost)
    {
        return;
    }
    assert(m_CurrContext);

    m_CurrContext->m_nViewportWidth = width;
    m_CurrContext->m_nViewportHeight = height;
    m_CurrContext->m_bMainViewport = bMainViewport;

    const int nMaxRes = GetMaxSquareRasterDimension();
    const float fMaxRes = aznumeric_cast<float>(nMaxRes);
    float fWidth = aznumeric_cast<float>(width);
    float fHeight = aznumeric_cast<float>(height);

    if (bMainViewport)
    {
        if (CV_r_CustomResWidth && CV_r_CustomResHeight)
        {
            width = clamp_tpl(CV_r_CustomResWidth, 32, nMaxRes);
            height = clamp_tpl(CV_r_CustomResHeight, 32, nMaxRes);
            fWidth = aznumeric_cast<float>(width);
            fHeight = aznumeric_cast<float>(height);
        }
        m_CurrContext->m_fPixelScaleX = 1.0f;
        m_CurrContext->m_fPixelScaleY = 1.0f;

        if (CV_r_Supersampling > 1)
        {
            m_CurrContext->m_fPixelScaleX *= aznumeric_cast<float>(CV_r_Supersampling);
            m_CurrContext->m_fPixelScaleY *= aznumeric_cast<float>(CV_r_Supersampling);
        }
        if ( scaleWidth > 1.0f || scaleHeight > 1.0f )
        {
            m_CurrContext->m_fPixelScaleX *= scaleWidth;
            m_CurrContext->m_fPixelScaleY *= scaleHeight;
        }

        const float nMaxSSX = fMaxRes / fWidth;
        const float nMaxSSY = fMaxRes / fHeight;
        m_CurrContext->m_fPixelScaleX = clamp_tpl(m_CurrContext->m_fPixelScaleX, 1.0f, nMaxSSX);
        m_CurrContext->m_fPixelScaleY = clamp_tpl(m_CurrContext->m_fPixelScaleY, 1.0f, nMaxSSY);

    }

    width  = aznumeric_cast<int>(fWidth * m_CurrContext->m_fPixelScaleX);
    height = aznumeric_cast<int>(fHeight * m_CurrContext->m_fPixelScaleY);

    DXGI_FORMAT fmt = DXGI_FORMAT_R8G8B8A8_UNORM;

    if (!m_CurrContext->m_pSwapChain && !m_CurrContext->m_pBackBuffer)
    {
        DXGI_SWAP_CHAIN_DESC scDesc;
        scDesc.BufferDesc.Width = width;
        scDesc.BufferDesc.Height = height;
        scDesc.BufferDesc.RefreshRate.Numerator = 0;
        scDesc.BufferDesc.RefreshRate.Denominator = 1;
        scDesc.BufferDesc.Format = fmt;
        scDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        scDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

        scDesc.SampleDesc.Count = 1;
        scDesc.SampleDesc.Quality = 0;

        scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
        scDesc.BufferCount = 1;
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_CPP_SECTION_4
    #include AZ_RESTRICTED_FILE(DriverD3D_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        scDesc.OutputWindow = m_CurrContext->m_hWnd;
#endif
        scDesc.Windowed = TRUE;
        scDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        scDesc.Flags = 0;

#if defined(SUPPORT_DEVICE_INFO)
        HRESULT hr = m_devInfo.Factory()->CreateSwapChain(&GetDevice(), &scDesc, &m_CurrContext->m_pSwapChain);
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_CPP_SECTION_5
    #include AZ_RESTRICTED_FILE(DriverD3D_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    #error UNKNOWN PLATFORM TRYING TO CREATE SWAP CHAIN
#endif
        assert(SUCCEEDED(hr) && m_CurrContext->m_pSwapChain != 0);

        assert(m_CurrContext->m_pSwapChain);
        PREFAST_ASSUME(m_CurrContext->m_pSwapChain);

        m_CurrContext->m_pSwapChain->GetDesc(&scDesc);

        AZ::u32 bufferCount = scDesc.BufferCount;
#if !defined(CRY_USE_DX12)
        bufferCount = 1;
#endif
        m_CurrContext->m_pBackBuffers.resize(bufferCount);

        for (unsigned int b = 0; b < bufferCount; ++b)
        {
            void* pBackBuf(0);
            hr = m_CurrContext->m_pSwapChain->GetBuffer(b, __uuidof(ID3D11Texture2D), &pBackBuf);
            ID3D11Texture2D* pBackBufferTex = (ID3D11Texture2D*)pBackBuf;
            assert(SUCCEEDED(hr) && pBackBufferTex != 0);

            D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
            rtDesc.Format = fmt;
            rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtDesc.Texture2D.MipSlice = 0;
            hr = GetDevice().CreateRenderTargetView(pBackBufferTex, &rtDesc, &m_CurrContext->m_pBackBuffers[b]);
            assert(SUCCEEDED(hr) && m_CurrContext->m_pBackBuffers[b] != 0);

            SAFE_RELEASE(pBackBufferTex);
        }

        m_CurrContext->m_pCurrentBackBufferIndex = GetCurrentBackBufferIndex(m_CurrContext->m_pSwapChain);
        m_CurrContext->m_pBackBuffer = m_CurrContext->m_pBackBuffers[m_CurrContext->m_pCurrentBackBufferIndex];
    }
    else if (m_CurrContext->m_Width != width || m_CurrContext->m_Height != height)
    {
        assert(m_CurrContext->m_pSwapChain && m_CurrContext->m_pBackBuffer);

        PREFAST_ASSUME(m_CurrContext->m_pSwapChain);
        DXGI_SWAP_CHAIN_DESC scDesc;
        m_CurrContext->m_pSwapChain->GetDesc(&scDesc);

        AZ::u32 bufferCount = scDesc.BufferCount;
#if !defined(CRY_USE_DX12)
        bufferCount = 1;
#endif

        // remove dangling view-refcounts before resize!
        for (unsigned int b = 0; b < m_CurrContext->m_pBackBuffers.size(); ++b)
        {
            SAFE_RELEASE(m_CurrContext->m_pBackBuffers[b]);
        }

        HRESULT hr = m_CurrContext->m_pSwapChain->ResizeBuffers(0, width, height, fmt, 0);
        CRY_ASSERT(SUCCEEDED(hr));

        m_CurrContext->m_pBackBuffers.resize(bufferCount);
        for (unsigned int b = 0; b < bufferCount; ++b)
        {
            void* pBackBuf(0);
            hr = m_CurrContext->m_pSwapChain->GetBuffer(b, __uuidof(ID3D11Texture2D), &pBackBuf);
            ID3D11Texture2D* pBackBufferTex = (ID3D11Texture2D*)pBackBuf;
            assert(SUCCEEDED(hr) && pBackBufferTex != 0);

            D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
            rtDesc.Format = fmt;
            rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtDesc.Texture2D.MipSlice = 0;
            hr = GetDevice().CreateRenderTargetView(pBackBufferTex, &rtDesc, &m_CurrContext->m_pBackBuffers[b]);
            assert(SUCCEEDED(hr) && m_CurrContext->m_pBackBuffers[b] != 0);

            SAFE_RELEASE(pBackBufferTex);
        }

        m_CurrContext->m_pCurrentBackBufferIndex = GetCurrentBackBufferIndex(m_CurrContext->m_pSwapChain);
        m_CurrContext->m_pBackBuffer = m_CurrContext->m_pBackBuffers[m_CurrContext->m_pCurrentBackBufferIndex];
    }

    if (m_CurrContext->m_pSwapChain && m_CurrContext->m_pBackBuffer)
    {
        assert(m_nRTStackLevel[0] == 0);
        m_CurrContext->m_pCurrentBackBufferIndex = GetCurrentBackBufferIndex(m_CurrContext->m_pSwapChain);
        m_CurrContext->m_pBackBuffer = m_CurrContext->m_pBackBuffers[m_CurrContext->m_pCurrentBackBufferIndex];
        m_pBackBuffer = m_CurrContext->m_pBackBuffer;
        m_pBackBuffers = decltype(m_pBackBuffers)(m_CurrContext->m_pBackBuffers.data(), m_CurrContext->m_pBackBuffers.data() + m_CurrContext->m_pBackBuffers.size());
        m_pCurrentBackBufferIndex = m_CurrContext->m_pCurrentBackBufferIndex;
        FX_SetRenderTarget(0, m_CurrContext->m_pBackBuffer, &m_DepthBufferOrig);
    }

    m_CurrContext->m_X = x;
    m_CurrContext->m_Y = y;
    m_CurrContext->m_Width = width;
    m_CurrContext->m_Height = height;
    m_width = m_nativeWidth = m_backbufferWidth = width;
    m_height = m_nativeHeight = m_backbufferHeight = height;

    SetCurDownscaleFactor(Vec2(1, 1));
    RT_SetViewport(x, y, width, height);
}

void CD3D9Renderer::SetFullscreenViewport()
{
    m_NewViewport.nX = 0;
    m_NewViewport.nY = 0;
    m_NewViewport.nWidth = GetWidth();
    m_NewViewport.nHeight = GetHeight();
    m_RP.m_PersFlags2 |= RBPF2_COMMIT_PF;
    m_RP.m_nCommitFlags |= FC_GLOBAL_PARAMS;
    m_bViewportDirty = true;
}

void CD3D9Renderer::SetCurDownscaleFactor(Vec2 sf)
{
    m_RP.m_CurDownscaleFactor = sf;

    m_FullResRect.left = m_FullResRect.top = 0;
    m_FullResRect.right = LONG(GetWidth() * m_RP.m_CurDownscaleFactor.x);
    m_FullResRect.bottom = LONG(GetHeight() * m_RP.m_CurDownscaleFactor.y);

    m_HalfResRect.left = m_HalfResRect.top = 0;
    m_HalfResRect.right = m_FullResRect.right >> 1;
    m_HalfResRect.bottom = m_FullResRect.bottom >> 1;

    // need to update shader parameters
    m_RP.m_PersFlags2 |= RBPF2_COMMIT_PF;
    m_RP.m_nCommitFlags |= FC_GLOBAL_PARAMS;
}

void CD3D9Renderer::ChangeLog()
{
#ifdef DO_RENDERLOG
    static bool singleFrame = false;

    if (CV_r_log && m_logFileHandle == AZ::IO::InvalidHandle)
    {
        if (CV_r_log < 0)
        {
            singleFrame = true;
            CV_r_log = abs(CV_r_log);
        }

        m_logFileHandle = fxopen("Direct3DLog.txt", "w");

        if (m_logFileHandle != AZ::IO::InvalidHandle)
        {
            iLog->Log("Direct3D log file '%s' opened\n", "Direct3DLog.txt");
            char time[128];
            char date[128];

            azstrtime(time);
            azstrdate(date);

            AZ::IO::Print(m_logFileHandle, "\n==========================================\n");
            AZ::IO::Print(m_logFileHandle, "Direct3D Log file opened: %s (%s)\n", date, time);
            AZ::IO::Print(m_logFileHandle, "==========================================\n");
        }
    }
    else if (m_logFileHandle != AZ::IO::InvalidHandle && singleFrame)
    {
        CV_r_log = 0;
        singleFrame = false;
    }

    if (!CV_r_log && m_logFileHandle != AZ::IO::InvalidHandle)
    {
        char time[128];
        char date[128];
        azstrtime(time);
        azstrdate(date);

        AZ::IO::Print(m_logFileHandle, "\n==========================================\n");
        AZ::IO::Print(m_logFileHandle, "Direct3D Log file closed: %s (%s)\n", date, time);
        AZ::IO::Print(m_logFileHandle, "==========================================\n");

        gEnv->pFileIO->Close(m_logFileHandle);
        m_logFileHandle = AZ::IO::InvalidHandle;
        iLog->Log("Direct3D log file '%s' closed\n", "Direct3DLog.txt");
    }

AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")
    if (CV_r_logTexStreaming && m_logFileStrHandle == AZ::IO::InvalidHandle)
AZ_POP_DISABLE_WARNING
    {
        m_logFileStrHandle = fxopen("Direct3DLogStreaming.txt", "w");
        if (m_logFileStrHandle != AZ::IO::InvalidHandle)
        {
            iLog->Log("Direct3D texture streaming log file '%s' opened\n", "Direct3DLogStreaming.txt");
            char time[128];
            char date[128];

            azstrtime(time);
            azstrdate(date);

            AZ::IO::Print(m_logFileStrHandle, "\n==========================================\n");
            AZ::IO::Print(m_logFileStrHandle, "Direct3D Textures streaming Log file opened: %s (%s)\n", date, time);
            AZ::IO::Print(m_logFileStrHandle, "==========================================\n");
        }
    }
    else
    if (!CV_r_logTexStreaming && m_logFileStrHandle != AZ::IO::InvalidHandle)
    {
        char time[128];
        char date[128];
        azstrtime(time);
        azstrdate(date);

        AZ::IO::Print(m_logFileStrHandle, "\n==========================================\n");
        AZ::IO::Print(m_logFileStrHandle, "Direct3D Textures streaming Log file closed: %s (%s)\n", date, time);
        AZ::IO::Print(m_logFileStrHandle, "==========================================\n");

        gEnv->pFileIO->Close(m_logFileStrHandle);
        m_logFileStrHandle = AZ::IO::InvalidHandle;
        iLog->Log("Direct3D texture streaming log file '%s' closed\n", "Direct3DLogStreaming.txt");
    }

AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")
    if (CV_r_logShaders && m_logFileShHandle == AZ::IO::InvalidHandle)
AZ_POP_DISABLE_WARNING
    {
        m_logFileShHandle = fxopen("Direct3DLogShaders.txt", "w");
        if (m_logFileShHandle != AZ::IO::InvalidHandle)
        {
            iLog->Log("Direct3D shaders log file '%s' opened\n", "Direct3DLogShaders.txt");
            char time[128];
            char date[128];

            azstrtime(time);
            azstrdate(date);

            AZ::IO::Print(m_logFileShHandle, "\n==========================================\n");
            AZ::IO::Print(m_logFileShHandle, "Direct3D Shaders Log file opened: %s (%s)\n", date, time);
            AZ::IO::Print(m_logFileShHandle, "==========================================\n");
        }
    }
    else
    if (!CV_r_logShaders && m_logFileShHandle != AZ::IO::InvalidHandle)
    {
        char time[128];
        char date[128];
        azstrtime(time);
        azstrdate(date);

        AZ::IO::Print(m_logFileShHandle, "\n==========================================\n");
        AZ::IO::Print(m_logFileShHandle, "Direct3D Textures streaming Log file closed: %s (%s)\n", date, time);
        AZ::IO::Print(m_logFileShHandle, "==========================================\n");

        gEnv->pFileIO->Close(m_logFileShHandle);
        m_logFileShHandle = AZ::IO::InvalidHandle;
        iLog->Log("Direct3D Shaders log file '%s' closed\n", "Direct3DLogShaders.txt");
    }
#endif
}

void CD3D9Renderer::PostMeasureOverdraw()
{
#if !defined(NULL_RENDERER) && !defined(_RELEASE)
    if (CV_r_measureoverdraw)
    {
        gRenDev->m_cEF.mfRefreshSystemShader("Debug", CShaderMan::s_ShaderDebug);

        int iTmpX, iTmpY, iTempWidth, iTempHeight;
        GetViewport(&iTmpX, &iTmpY, &iTempWidth, &iTempHeight);
        RT_SetViewport(0, 0, m_width, m_height);

        TransformationMatrices backupSceneMatrices;

        Set2DMode(1, 1, backupSceneMatrices);

        {
            TempDynVB<SVF_P3F_C4B_T2F> vb(gRenDev);
            vb.Allocate(4);
            SVF_P3F_C4B_T2F* vQuad = vb.Lock();

            vQuad[0].xyz.x = 0;
            vQuad[0].xyz.y = 0;
            vQuad[0].xyz.z = 1;
            vQuad[0].color.dcolor = (uint32) - 1;
            vQuad[0].st = Vec2(0.0f, 0.0f);

            vQuad[1].xyz.x = 1;
            vQuad[1].xyz.y = 0;
            vQuad[1].xyz.z = 1;
            vQuad[1].color.dcolor = (uint32) - 1;
            vQuad[1].st = Vec2(1.0f, 0.0f);

            vQuad[2].xyz.x = 0;
            vQuad[2].xyz.y = 1;
            vQuad[2].xyz.z = 1;
            vQuad[2].color.dcolor = (uint32) - 1;
            vQuad[2].st = Vec2(0.0f, 1.0f);

            vQuad[3].xyz.x = 1;
            vQuad[3].xyz.y = 1;
            vQuad[3].xyz.z = 1;
            vQuad[3].color.dcolor = (uint32) - 1;
            vQuad[3].st = Vec2(1.0f, 1.0f);

            vb.Unlock();
            vb.Bind(0);
            vb.Release();

            SetCullMode(R_CULL_DISABLE);
            FX_SetState(GS_NODEPTHTEST);
            CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
        }

        {
            uint32 nPasses;
            CShader* pSH = m_cEF.s_ShaderDebug;
            pSH->FXSetTechnique("ShowInstructions");
            pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
            pSH->FXBeginPass(0);
            STexState TexStateLinear = STexState(FILTER_LINEAR, true);

            if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
            {
                STexState TexStatePoint = STexState(FILTER_POINT, true);
                FX_Commit();
                
                CTexture::s_ptexSceneDiffuse->Apply(0, CTexture::GetTexState(TexStatePoint));
                CTextureManager::Instance()->GetDefaultTexture("PaletteDebug")->Apply(1, CTexture::GetTexState(TexStateLinear));

                FX_DrawPrimitive(eptTriangleStrip, 0, 4);
            }

            Unset2DMode(backupSceneMatrices);

            int nX = 800 - 100 + 2;
            int nY = 600 - 100 + 2;
            int nW = 96;
            int nH = 96;

            Draw2dImage(nX - 2, nY - 2, nW + 4, nH + 4, CTextureManager::Instance()->GetWhiteTexture()->GetTextureID());

            Set2DMode(800, 600, backupSceneMatrices);

            TempDynVB<SVF_P3F_C4B_T2F> vb(gRenDev);
            vb.Allocate(4);
            SVF_P3F_C4B_T2F* vQuad = vb.Lock();

            vQuad[0].xyz.x = nX;
            vQuad[0].xyz.y = nY;
            vQuad[0].xyz.z = 1;
            vQuad[0].color.dcolor = (uint32) - 1;
            vQuad[0].st = Vec2(0.0f, 0.0f);

            vQuad[1].xyz.x = nX + nW;
            vQuad[1].xyz.y = nY;
            vQuad[1].xyz.z = 1;
            vQuad[1].color.dcolor = (uint32) - 1;
            vQuad[1].st = Vec2(1.0f, 0.0f);

            vQuad[2].xyz.x = nX;
            vQuad[2].xyz.y = nY + nH;
            vQuad[2].xyz.z = 1;
            vQuad[2].color.dcolor = (uint32) - 1;
            vQuad[2].st = Vec2(0.0f, 1.0f);

            vQuad[3].xyz.x = nX + nW;
            vQuad[3].xyz.y = nY + nH;
            vQuad[3].xyz.z = 1;
            vQuad[3].color.dcolor = (uint32) - 1;
            vQuad[3].st = Vec2(1.0f, 1.0f);

            vb.Unlock();
            vb.Bind(0);
            vb.Release();

            pSH->FXSetTechnique("InstructionsGrad");
            pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
            pSH->FXBeginPass(0);
            FX_Commit();

            CTextureManager::Instance()->GetDefaultTexture("PaletteDebug")->Apply(0, CTexture::GetTexState(TexStateLinear));

            FX_DrawPrimitive(eptTriangleStrip, 0, 4);

            nX = nX * m_width / 800;
            nY = nY * m_height / 600;
            nW = 10 * m_width / 800;
            nH = 10 * m_height / 600;
            float color[4] = {1, 1, 1, 1};
            if (CV_r_measureoverdraw == 1 || CV_r_measureoverdraw == 3)
            {
                Draw2dLabel(nX + nW - 25, nY + nH - 30, 1.2f, color, false, CV_r_measureoverdraw == 1 ? "Pixel Shader:" : "Vertex Shader:");
                int n = FtoI(32.0f * CV_r_measureoverdrawscale);
                for (int i = 0; i < 8; i++)
                {
                    char str[256];
                    azsprintf(str, "-- >%d instructions --", n);

                    Draw2dLabel(nX + nW, nY + nH * (i + 1), 1.2f, color, false,  str);
                    n += FtoI(32.0f * CV_r_measureoverdrawscale);
                }
            }
            else
            {
                Draw2dLabel(nX + nW - 25, nY + nH - 30, 1.2f, color, false, CV_r_measureoverdraw == 2 ? "Pass Count:" : "Overdraw Estimation (360 Hi-Z):");
                Draw2dLabel(nX + nW, nY + nH, 1.2f, color, false,  "-- 1 pass --");
                Draw2dLabel(nX + nW, nY + nH * 2, 1.2f, color, false,  "-- 2 passes --");
                Draw2dLabel(nX + nW, nY + nH * 3, 1.2f, color, false,  "-- 3 passes --");
                Draw2dLabel(nX + nW, nY + nH * 4, 1.2f, color, false,  "-- 4 passes --");
                Draw2dLabel(nX + nW, nY + nH * 5, 1.2f, color, false,  "-- 5 passes --");
                Draw2dLabel(nX + nW, nY + nH * 6, 1.2f, color, false,  "-- 6 passes --");
                Draw2dLabel(nX + nW, nY + nH * 7, 1.2f, color, false,  "-- 7 passes --");
                Draw2dLabel(nX + nW, nY + nH * 8, 1.2f, color, false,  "-- >8 passes --");
            }
            //WriteXY(nX+10, nY+10, 1, 1,  1,1,1,1, "-- 10 instructions --");
        }
        Unset2DMode(backupSceneMatrices);
        RT_RenderTextMessages();
    }
#endif
}

void CD3D9Renderer::DrawTexelsPerMeterInfo()
{
#ifndef _RELEASE
    if (CV_r_TexelsPerMeter > 0)
    {
        FX_SetState(GS_NODEPTHTEST);

        int x = 800 - 310 + 2;
        int y = 600 - 20 + 2;
        int w = 296;
        int h = 6;

        Draw2dImage(x - 2, y - 2, w + 4, h + 4, CTextureManager::Instance()->GetWhiteTexture()->GetTextureID(), 0, 0, 1, 1, 0, 1, 1, 1, 1, 0);
        Draw2dImage(x, y, w, h, CTextureManager::Instance()->GetDefaultTexture("PaletteTexelsPerMeter")->GetTextureID(), 0, 0, 1, 1, 0, 1, 1, 1, 1, 0);

        float color[4] = { 1, 1, 1, 1 };

        x = x * m_width / 800;
        y = y * m_height / 600;
        w = w * m_width / 800;

        Draw2dLabel(x - 100, y - 20, 1.2f, color, false, "r_TexelsPerMeter:");
        Draw2dLabel(x - 2, y - 20, 1.2f, color, false, "0");
        Draw2dLabel(x + w / 2 - 5, y - 20, 1.2f, color, false, "%.0f", CV_r_TexelsPerMeter);
        Draw2dLabel(x + w - 25, y - 20, 1.2f, color, false, ">= %.0f", CV_r_TexelsPerMeter * 2.0f);

        RT_RenderTextMessages();
    }
#endif
}

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_CPP_SECTION_6
    #include AZ_RESTRICTED_FILE(DriverD3D_cpp)
#endif

void CD3D9Renderer::RT_ForceSwapBuffers()
{
}

void CD3D9Renderer::SwitchToNativeResolutionBackbuffer()
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_CPP_SECTION_7
    #include AZ_RESTRICTED_FILE(DriverD3D_cpp)
#endif

    m_pRT->RC_SwitchToNativeResolutionBackbuffer();
}

void CD3D9Renderer::CalculateResolutions(int width, int height, [[maybe_unused]] bool bUseNativeRes, int* pRenderWidth, int* pRenderHeight, int* pNativeWidth, int* pNativeHeight, int* pBackbufferWidth, int* pBackbufferHeight)
{
    width  = max(width,  512);
    height = max(height, 300);

    *pRenderWidth = width * m_numSSAASamples;
    *pRenderHeight = height * m_numSSAASamples;

#if DRIVERD3D_CPP_TRAIT_CALCULATERESOLUTIONS_1080
    *pNativeWidth = 1920;
    *pNativeHeight = 1080;
#elif defined(WIN32) || defined(WIN64)
    *pNativeWidth  = bUseNativeRes ? m_prefMonWidth  : width;
    *pNativeHeight = bUseNativeRes ? m_prefMonHeight : height;
#else
    *pNativeWidth  = width;
    *pNativeHeight = height;
#endif

    if (m_pStereoRenderer && m_pStereoRenderer->IsStereoEnabled())
    {
        m_pStereoRenderer->CalculateBackbufferResolution(*pNativeWidth, *pNativeHeight, *pBackbufferWidth, *pBackbufferHeight);
    }
    else
    {
        *pBackbufferWidth  = *pNativeWidth;
        *pBackbufferHeight = *pNativeHeight;
    }
}

void CD3D9Renderer::RT_SwitchToNativeResolutionBackbuffer([[maybe_unused]] bool resolveBackBuffer)
{
    FX_FinalComposite();
}

void CD3D9Renderer::HandleDisplayPropertyChanges()
{
    const bool msaaChanged = CheckMSAAChange();
    const bool ssaaChanged = CheckSSAAChange();

    if (!IsEditorMode())
    {
        bool bChangeRes = ssaaChanged;

#if defined(SUPPORT_DEVICE_INFO_USER_DISPLAY_OVERRIDES)
        bChangeRes |= m_overrideRefreshRate != CV_r_overrideRefreshRate || m_overrideScanlineOrder != CV_r_overrideScanlineOrder;
#endif

        bool bFullScreen;
#if DRIVERD3D_CPP_TRAIT_HANDLEDISPLAYPROPERTYCHANGES_FULLSCREEN
        bFullScreen = m_bFullScreen;
#else
        bFullScreen = m_CVFullScreen ? m_CVFullScreen->GetIVal() != 0 : m_bFullScreen;
#endif

        bool bForceReset = msaaChanged;
        bool bNativeRes;
#if DRIVERD3D_CPP_TRAIT_HANDLEDISPLAYPROPERTYCHANGES_NATIVERES
        bNativeRes = true;
#elif defined(WIN32) || defined(WIN64)
        bForceReset |= m_bDisplayChanged && bFullScreen;
        m_bDisplayChanged = false;

        const bool fullscreenWindow = CV_r_FullscreenWindow && CV_r_FullscreenWindow->GetIVal() != 0;
        bChangeRes |= m_fullscreenWindow != fullscreenWindow;
        m_fullscreenWindow = fullscreenWindow;

        const bool bFullScreenNativeRes = CV_r_FullscreenNativeRes && CV_r_FullscreenNativeRes->GetIVal() != 0;
        bNativeRes = bFullScreenNativeRes && (bFullScreen || m_fullscreenWindow);
#else
        bNativeRes = false;
#endif

        const int colorBits = m_CVColorBits ? m_CVColorBits->GetIVal() : m_cbpp;

        int width, height;
#if defined(IOS)|| defined(ANDROID)
        width  = m_width;
        height = m_height;
#else
        width  = m_CVWidth  ? m_CVWidth->GetIVal()  : m_width;
        height = m_CVHeight ? m_CVHeight->GetIVal() : m_height;

        const float scale = CLAMP(gEnv->pConsole->GetCVar("r_ResolutionScale")->GetFVal(), MIN_RESOLUTION_SCALE, MAX_RESOLUTION_SCALE);
        width  *= scale;
        height *= scale;
#endif

#if DRIVERD3D_CPP_TRAIT_HANDLEDISPLAYPROPERTYCHANGES_CALCRESOLUTIONS
        int renderWidth, renderHeight, nativeWidth, nativeHeight, backbufferWidth, backbufferHeight;
        CalculateResolutions(width, height, bNativeRes, &renderWidth, &renderHeight, &nativeWidth, &nativeHeight, &backbufferWidth, &backbufferHeight);

        if (m_width != renderWidth || m_height != renderHeight || m_nativeWidth != nativeWidth || m_nativeHeight != nativeHeight)
        {
            bChangeRes = true;
            width  = renderWidth;
            height = renderHeight;
            m_nativeWidth  = nativeWidth;
            m_nativeHeight = nativeHeight;
        }

        if (m_backbufferWidth != backbufferWidth || m_backbufferHeight != backbufferHeight)
        {
            bForceReset = true;
            m_backbufferWidth  = backbufferWidth;
            m_backbufferHeight = backbufferHeight;
        }
#endif

        if (bForceReset || bChangeRes || bFullScreen != m_bFullScreen || colorBits != m_cbpp || CV_r_vsync != m_VSync)
        {
            ChangeResolution(width, height, colorBits, 75, bFullScreen, bForceReset);
        }
    }
    else if (m_CurrContext && m_CurrContext->m_bMainViewport)
    {
        static bool bCustomRes = false;
        static int nOrigWidth = m_d3dsdBackBuffer.Width;
        static int nOrigHeight = m_d3dsdBackBuffer.Height;

        if (!bCustomRes)
        {
            nOrigWidth = m_d3dsdBackBuffer.Width;
            nOrigHeight = m_d3dsdBackBuffer.Height;
        }

        int nNewBBWidth = nOrigWidth;
        int nNewBBHeight = nOrigHeight;

        bCustomRes = (CV_r_CustomResWidth && CV_r_CustomResHeight) || CV_r_Supersampling > 1;
        const int maxRes = GetMaxSquareRasterDimension();
        if (bCustomRes)
        {
            const int nMaxBBSize = max(maxRes, max(nOrigWidth, nOrigHeight));
            nNewBBWidth = clamp_tpl(m_width, nOrigWidth, nMaxBBSize);
            nNewBBHeight = clamp_tpl(m_height, nOrigHeight, nMaxBBSize);
        }

        if (msaaChanged || m_d3dsdBackBuffer.Width != nNewBBWidth || m_d3dsdBackBuffer.Height != nNewBBHeight)
        {
            if (CV_r_CustomResWidth > maxRes || CV_r_CustomResHeight > maxRes)
            {
                iLog->LogWarning("Custom resolutions are limited to %d.", maxRes);
                iLog->LogWarning("    The requested resolution (%dx%d) will be adjusted to (%dx%d).", CV_r_CustomResWidth, CV_r_CustomResHeight, nNewBBWidth, nNewBBHeight);
                iLog->LogWarning("    Try increasing r_CustomResMaxSize to avoid this adjustment, or set it to -1 to use the maximum resources of the device.");
            }
            iLog->Log("Resizing back buffer to match custom resolution rendering:");
            ChangeResolution(nNewBBWidth, nNewBBHeight, 32, 75, false, true);
        }
    }
}

void CD3D9Renderer::BeginFrame()
{
    //////////////////////////////////////////////////////////////////////
    // Set up everything so we can start rendering
    //////////////////////////////////////////////////////////////////////

    assert(m_Device);

    CheckDeviceLost();

    FlushRTCommands(false, false, false);

    CaptureFrameBufferPrepare();

    // Switching of MT mode in run-time
    //CV_r_multithreaded = 0;

    GetS3DRend().Update();

    m_cEF.mfBeginFrame();

    CRendElement::Tick();

    CREImposter::m_PrevMemPostponed = CREImposter::m_MemPostponed;
    CREImposter::m_PrevMemUpdated = CREImposter::m_MemUpdated;
    CREImposter::m_MemPostponed = 0;
    CREImposter::m_MemUpdated = 0;

    m_RP.m_TI[m_RP.m_nFillThreadID].m_nFrameID++;
    m_RP.m_TI[m_RP.m_nFillThreadID].m_nFrameUpdateID++;
    m_RP.m_TI[m_RP.m_nFillThreadID].m_RealTime = iTimer->GetCurrTime();
    m_RP.m_TI[m_RP.m_nFillThreadID].m_PersFlags &= ~RBPF_HDR;

    CREOcclusionQuery::m_nQueriesPerFrameCounter = 0;
    CREOcclusionQuery::m_nReadResultNowCounter = 0;
    CREOcclusionQuery::m_nReadResultTryCounter = 0;

    m_RP.m_TI[m_RP.m_nFillThreadID].m_matView.SetIdentity();
    m_RP.m_TI[m_RP.m_nFillThreadID].m_matProj.SetIdentity();

    g_SelectedTechs.resize(0);
    m_RP.m_SysVertexPool[m_RP.m_nFillThreadID].SetUse(0);
    m_RP.m_SysIndexPool[m_RP.m_nFillThreadID].SetUse(0);

#ifdef DO_RENDERLOG
    if (CRenderer::CV_r_log)
    {
        Logv(0, "******************************* BeginFrame %d ********************************\n", m_RP.m_TI[m_RP.m_nFillThreadID].m_nFrameUpdateID);
    }
#endif
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_logTexStreaming)
    {
        LogStrv(0, "******************************* BeginFrame %d ********************************\n", m_RP.m_TI[m_RP.m_nFillThreadID].m_nFrameUpdateID);
    }

    PREFAST_SUPPRESS_WARNING(6326)
    bool bUseWaterTessHW(CV_r_WaterTessellationHW != 0 && m_bDeviceSupportsTessellation);
    if (bUseWaterTessHW != m_bUseWaterTessHW)
    {
        PREFAST_SUPPRESS_WARNING(6326)
        m_bUseWaterTessHW = bUseWaterTessHW;
        m_cEF.mfReloadAllShaders(1, SHGD_HW_WATER_TESSELLATION);
    }

    PREFAST_SUPPRESS_WARNING(6326)
    if ((CV_r_SilhouettePOM != 0) != m_bUseSilhouettePOM)
    {
        PREFAST_SUPPRESS_WARNING(6326)
        m_bUseSilhouettePOM = CV_r_SilhouettePOM != 0;
        m_cEF.mfReloadAllShaders(1, SHGD_HW_SILHOUETTE_POM);
    }

    PREFAST_SUPPRESS_WARNING(6326)
    if ((CV_r_SpecularAntialiasing != 0) != m_bUseSpecularAntialiasing)
    {
        PREFAST_SUPPRESS_WARNING(6326)
        m_bUseSpecularAntialiasing = CV_r_SpecularAntialiasing != 0;
        m_cEF.mfReloadAllShaders(1, SHGD_HW_SAA);
    }

    if IsCVarConstAccess(constexpr) (CV_r_reloadshaders)
    {
        //exit(0);
        //ShutDown();
        //iConsole->Exit("Test");

        m_cEF.m_Bin.InvalidateCache();
        m_cEF.mfReloadAllShaders(CV_r_reloadshaders, 0);
#ifndef CONSOLE_CONST_CVAR_MODE
        CV_r_reloadshaders = 0;
#endif
        // after reloading shaders, update all shader items
        // and flush the RenderThread to make the changes visible
        gEnv->p3DEngine->UpdateShaderItems();
        gRenDev->FlushRTCommands(true, true, true);
    }

    m_pRT->RC_BeginFrame();
}

void CD3D9Renderer::RT_BeginFrame()
{
#ifdef CRY_USE_METAL
    //  Create autorelease pool before doing anything with the render device.
    //  This will be released in the Present method.
    IDXGISwapChain* swapChain = m_pSwapChain;

    if (m_bEditor && m_CurrContext->m_pSwapChain)
    {
        swapChain = m_CurrContext->m_pSwapChain;
    }

    CCryDXGLSwapChain::FromInterface(swapChain)->TryCreateAutoreleasePool();
#endif

    {
        static const ICVar* pCVarShadows = NULL;
        static const ICVar* pCVarShadowsClouds = NULL;
        // assume these cvars will exist
        if (!pCVarShadows)
        {
            pCVarShadows = iConsole->GetCVar("e_Shadows");
        }
        if (!pCVarShadowsClouds)
        {
            pCVarShadowsClouds = iConsole->GetCVar("e_ShadowsClouds");
        }

        m_bShadowsEnabled = pCVarShadows && pCVarShadows->GetIVal() != 0;
        m_bCloudShadowsEnabled = pCVarShadowsClouds && pCVarShadowsClouds->GetIVal() != 0;

#if defined(VOLUMETRIC_FOG_SHADOWS)
        Vec3 volFogShadowEnable(0, 0, 0);
        if (gEnv->p3DEngine)
        {
            gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_VOLFOG_SHADOW_ENABLE, volFogShadowEnable);
        }

        m_bVolFogShadowsEnabled = m_bShadowsEnabled && CV_r_PostProcess != 0 && CV_r_FogShadows != 0 && volFogShadowEnable.x != 0;
        m_bVolFogCloudShadowsEnabled = m_bVolFogShadowsEnabled && m_bCloudShadowsEnabled && m_cloudShadowTexId > 0 && volFogShadowEnable.y != 0;
#endif

        static ICVar* pCVarVolumetricFog = NULL;
        if (!pCVarVolumetricFog)
        {
            pCVarVolumetricFog = iConsole->GetCVar("e_VolumetricFog");
        }
        bool bVolumetricFog = pCVarVolumetricFog && pCVarVolumetricFog->GetIVal();
        CRenderer::CV_r_VolumetricFog = bVolumetricFog && GetVolumetricFog().IsEnableInFrame();
    }

#if defined(SUPPORT_D3D_DEBUG_RUNTIME)
    if (m_bUpdateD3DDebug)
    {
        m_d3dDebug.Update(ESeverityCombination(CV_d3d11_debugMuteSeverity->GetIVal()), CV_d3d11_debugMuteMsgID->GetString(), CV_d3d11_debugBreakOnMsgID->GetString());
        if (azstricmp(CV_d3d11_debugBreakOnMsgID->GetString(), "0") != 0 && CV_d3d11_debugBreakOnce)
        {
            CV_d3d11_debugBreakOnMsgID->Set("0");
        }
        else
        {
            m_bUpdateD3DDebug = false;
        }
    }
#endif

    {
        if (CV_r_flush > 0 && CV_r_minimizeLatency == 0)
        {
            FlushHardware(false);
        }
    }

    CResFile::Tick();
    m_DevBufMan.Update(m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameUpdateID, false);

    if (m_pPipelineProfiler)
    {
        m_pPipelineProfiler->BeginFrame();
    }

    //////////////////////////////////////////////////////////////////////
    // Build the matrices
    //////////////////////////////////////////////////////////////////////

    PROFILE_FRAME(Screen_Begin);
    //PROFILE_LABEL_PUSH_SKIP_GPU("Frame");

#ifndef CONSOLE_CONST_CVAR_MODE
    ICVar* pCVDebugTexelDensity = gEnv->pConsole->GetCVar("e_texeldensity");
    CV_e_DebugTexelDensity = pCVDebugTexelDensity ? pCVDebugTexelDensity->GetIVal() : 0;
#endif

    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView.SetIdentity();

    CheckDeviceLost();

    if (!m_bDeviceLost && (m_bIsWindowActive || m_bEditor))
    {
        SetGamma(CV_r_gamma + m_fDeltaGamma, CV_r_brightness, CV_r_contrast, false);
    }


    if (!m_bDeviceSupportsInstancing)
    {
        if IsCVarConstAccess(constexpr) (CV_r_geominstancing)
        {
            iLog->Log("Device doesn't support HW geometry instancing (or it's disabled)");
            _SetVar("r_GeomInstancing", 0);
        }
    }

    if (CV_r_usehwskinning != (int)m_bUseHWSkinning)
    {
        m_bUseHWSkinning = CV_r_usehwskinning != 0;
        CRendElement* pRE = CRendElement::m_RootGlobal.m_NextGlobal;
        for (pRE = CRendElement::m_RootGlobal.m_NextGlobal; pRE != &CRendElement::m_RootGlobal; pRE = pRE->m_NextGlobal)
        {
            CRendElementBase* pR = (CRendElementBase*)pRE;
            if (pR->mfIsHWSkinned())
            {
                pR->mfReset();
            }
        }
    }

    const bool bUseGlobalMipBias = m_TemporalJitterMipBias != 0.0f;
    if (CV_r_texminanisotropy != m_nCurMinAniso || CV_r_texmaxanisotropy != m_nCurMaxAniso || bUseGlobalMipBias != m_UseGlobalMipBias)
    {
        m_nCurMinAniso = CV_r_texminanisotropy;
        m_nCurMaxAniso = CV_r_texmaxanisotropy;
        m_UseGlobalMipBias = bUseGlobalMipBias;
        for (uint32 i = 0; i < CShader::s_ShaderResources_known.Num(); i++)
        {
            CShaderResources* pSR = CShader::s_ShaderResources_known[i];
            if (!pSR)
            {
                continue;
            }
            pSR->AdjustForSpec();
        }

        auto getAnisoFilter = [](int nLevel)
            {
                int8 nFilter =
                    nLevel >= 16 ? FILTER_ANISO16X :
                    (nLevel >=  8 ? FILTER_ANISO8X  :
                     (nLevel >=  4 ? FILTER_ANISO4X  :
                      (nLevel >=  2 ? FILTER_ANISO2X  :
                       FILTER_TRILINEAR)));

                return nFilter;
            };

        int8 nAniso = min(CRenderer::CV_r_texminanisotropy, CRenderer::CV_r_texmaxanisotropy); // for backwards compatibility. should really be CV_r_texmaxanisotropy
        m_nMaterialAnisoHighSampler = CTexture::GetTexState(STexState(getAnisoFilter(nAniso), false));
        m_nMaterialAnisoLowSampler  = CTexture::GetTexState(STexState(getAnisoFilter(CRenderer::CV_r_texminanisotropy), false));
        m_nMaterialAnisoSamplerBorder = CTexture::GetTexState(STexState(getAnisoFilter(nAniso), TADDR_BORDER, TADDR_BORDER, TADDR_BORDER, 0x0));
    }

    m_drawNearFov = CV_r_drawnearfov;

#if defined(SUPPORT_DEVICE_INFO_MSG_PROCESSING)
    m_devInfo.ProcessSystemEventQueue();
#endif

    HandleDisplayPropertyChanges();

    if (CV_r_wireframe != m_wireframe_mode)
    {
        //assert(CV_r_wireframe == R_WIREFRAME_MODE || CV_r_wireframe == R_SOLID_MODE);
        FX_SetWireframeMode(CV_r_wireframe);
    }

#if !defined(_RELEASE)
    m_RP.m_pRNDrawCallsInfoPerNode[m_RP.m_nProcessThreadID].swap(m_RP.m_pRNDrawCallsInfoPerNodePreviousFrame[m_RP.m_nProcessThreadID]);
    m_RP.m_pRNDrawCallsInfoPerNode[m_RP.m_nProcessThreadID].clear();

    m_RP.m_pRNDrawCallsInfoPerMesh[m_RP.m_nProcessThreadID].swap(m_RP.m_pRNDrawCallsInfoPerMeshPreviousFrame[m_RP.m_nProcessThreadID]);
    m_RP.m_pRNDrawCallsInfoPerMesh[m_RP.m_nProcessThreadID].clear();
#endif

    //////////////////////////////////////////////////////////////////////
    // Begin the scene
    //////////////////////////////////////////////////////////////////////

    SetMaterialColor(1, 1, 1, 1);

    ChangeLog();

    ResetToDefault();

    if (!m_SceneRecurseCount)
    {
        m_SceneRecurseCount++;
    }

    if (CV_r_wireframe != 0 || !CV_r_usezpass)
    {
        EF_ClearTargetsLater(FRT_CLEAR);
    }

    m_nStencilMaskRef = 1;

    if (SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID] == -1)
    {
        memset(&m_RP.m_PS[m_RP.m_nProcessThreadID], 0, sizeof(SPipeStat));
        m_RP.m_RTStats.resize(0);
        m_RP.m_Profile.Free();
    }
#if !defined(NULL_RENDERER)
    m_OcclQueriesUsed = 0;
#endif

    {
        static float fWaitForGPU;
        float fSmooth = 5.0f;
        fWaitForGPU = (m_fTimeWaitForGPU[m_RP.m_nFillThreadID] + fWaitForGPU * fSmooth) / (fSmooth + 1.0f);
        if (fWaitForGPU >= 0.004f)
        {
            if (m_nGPULimited < 1000)
            {
                m_nGPULimited++;
            }
        }
        else
        {
            m_nGPULimited = 0;
        }

        m_bUseGPUFriendlyBatching[m_RP.m_nProcessThreadID] = m_nGPULimited > 10; // On PC if we are GPU limited use z-pass distance sorting and disable instancing
        if (CV_r_batchtype == 0)
        {
            m_bUseGPUFriendlyBatching[m_RP.m_nProcessThreadID] = false;
        }
        else
        if (CV_r_batchtype == 1)
        {
            m_bUseGPUFriendlyBatching[m_RP.m_nProcessThreadID] = true;
        }
    }

#if defined(SUPPORT_DEVICE_INFO)

    if (m_bEditor)
    {
        const int width = GetWidth();
        const int height = GetHeight();

        if (m_DepthBufferOrigMSAA.nWidth < width || m_DepthBufferOrigMSAA.nHeight < height)
        {
            m_DepthBufferOrig.Release();
            m_DepthBufferOrigMSAA.Release();
            m_DepthBufferNative.Release();

            GetS3DRend().ReleaseBuffers();

            m_devInfo.SwapChainDesc().BufferDesc.Width = max(m_DepthBufferOrigMSAA.nWidth, width);
            m_devInfo.SwapChainDesc().BufferDesc.Height = max(m_DepthBufferOrigMSAA.nHeight, height);
            m_devInfo.ResizeDXGIBuffers();

            OnD3D11PostCreateDevice(m_devInfo.Device());

            ChangeViewport(0, 0, width, height, true);
        }
    }
#endif

#if REFRACTION_PARTIAL_RESOLVE_DEBUG_VIEWS
    // Refraction Partial Resolves debug views
    if (CRenderer::CV_r_RefractionPartialResolvesDebug == eRPR_DEBUG_VIEW_2D_AREA)
    {
        // Clear the screen for the partial resolve debug view 1
        IRenderAuxGeom* pAuxRenderer = gEnv->pRenderer->GetIRenderAuxGeom();
        if (pAuxRenderer)
        {
            SAuxGeomRenderFlags oldRenderFlags = pAuxRenderer->GetRenderFlags();

            SAuxGeomRenderFlags newRenderFlags;
            newRenderFlags.SetDepthTestFlag(e_DepthTestOff);
            newRenderFlags.SetAlphaBlendMode(e_AlphaNone);
            newRenderFlags.SetMode2D3DFlag(e_Mode2D);
            pAuxRenderer->SetRenderFlags(newRenderFlags);

            const bool bConsoleVisible = GetISystem()->GetIConsole()->GetStatus();
            const float screenTop = (bConsoleVisible) ? 0.5f : 0.0f;

            // Draw full screen black triangle
            const uint vertexCount = 3;
            Vec3 vert[vertexCount] = {
                Vec3(0.0f, screenTop, 0.0f),
                Vec3(0.0f, 2.0f, 0.0f),
                Vec3(2.0f, screenTop, 0.0f)
            };

            pAuxRenderer->DrawTriangles(vert, vertexCount, Col_Black);
            pAuxRenderer->SetRenderFlags(oldRenderFlags);
        }
    }
#endif
}

bool CD3D9Renderer::CheckDeviceLost()
{
#if (defined(WIN32) || defined(WIN64))
    if (m_pRT && CV_r_multithreaded == 1 && m_pRT->IsRenderThread())
    {
        return false;
    }

    // DX10/11 should still handle gamma changes on window focus loss
    if (!m_bStartLevelLoading)
    {
        const bool bWindowActive = (GetForegroundWindow() == m_hWnd);

        // Adjust gamma to match focused window
        if (bWindowActive != m_bIsWindowActive)
        {
            if (bWindowActive)
            {
                SetGamma(CV_r_gamma + m_fDeltaGamma, CV_r_brightness, CV_r_contrast, true);
            }
            else
            {
                RestoreGamma();
            }

            m_bIsWindowActive = bWindowActive;
        }
    }
#endif
    return false;
}

void CD3D9Renderer::FlushHardware(bool bIssueBeforeSync)
{
    PROFILE_FRAME(FlushHardware);

    if (m_bDeviceLost)
    {
        return;
    }

    int nFr = m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameUpdateID & (MAX_FRAME_QUERIES - 1);

    HRESULT hr;
    if (CV_r_flush)
    {
        if (m_pQuery[nFr])
        {
            if (bIssueBeforeSync)
            {
                GetDeviceContext().End(m_pQuery[nFr]);
            }
            BOOL bQuery = false;
            CTimeValue Time = iTimer->GetAsyncTime();
            bool bInfinite = false;
            int nCounter = 0;

            do
            {
                nCounter++;
                // Check time-out once in 512 iterations
                if (!(nCounter & 0x1ff))
                {
                    float fDif = iTimer->GetAsyncTime().GetDifferenceInSeconds(Time);
                    if (fDif > 5.0f)
                    {
                        // 5 seconds in the loop
                        bInfinite = true;
                        break;
                    }
                }
                hr = GetDeviceContext().GetData(m_pQuery[nFr], (void*)&bQuery, sizeof(BOOL), 0);
            } while (hr == S_FALSE);

            if (bInfinite)
            {
                iLog->Log("Error: Seems like infinite loop in GPU sync query");
            }

            m_fTimeWaitForGPU[m_RP.m_nProcessThreadID] += iTimer->GetAsyncTime().GetDifferenceInSeconds(Time);

            if (!bIssueBeforeSync)
            {
                GetDeviceContext().End(m_pQuery[nFr]);
            }
        }
    }
}

bool CD3D9Renderer::PrepFrameCapture(FrameBufferDescription& frameBufDesc, CTexture* pRenderTarget)
{
    assert(m_pBackBuffer);
    assert(!IsEditorMode() || (m_CurrContext && m_CurrContext->m_pBackBuffer == m_pBackBuffer));

    ID3D11Resource* pBackBufferRsrc(0);

    if (!pRenderTarget)
    {
        m_pBackBuffer->GetResource(&pBackBufferRsrc);
    }
    else
    {
        D3DSurface* pSurface = pRenderTarget->GetSurface(0, 0);
        assert(pSurface);
        pSurface->GetResource(&pBackBufferRsrc);
    }

    frameBufDesc.pBackBufferTex = (ID3D11Texture2D*)pBackBufferRsrc;

    if (!frameBufDesc.pBackBufferTex)
    {
        return false;
    }
        
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //get size of image
    frameBufDesc.pBackBufferTex->GetDesc(&frameBufDesc.backBufferDesc);
    if (frameBufDesc.backBufferDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
        || (m_CurrContext
            && (m_CurrContext->m_fPixelScaleX > 1.0f
                || m_CurrContext->m_fPixelScaleY > 1.0f)))
    {
        const int nResolvedWidth = aznumeric_cast<int>(aznumeric_cast<float>(m_width) / m_CurrContext->m_fPixelScaleX);
        const int nResolvedHeight = aznumeric_cast<int>(aznumeric_cast<float>(m_height) / m_CurrContext->m_fPixelScaleY);
        frameBufDesc.backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        frameBufDesc.backBufferDesc.Width = nResolvedWidth;
        frameBufDesc.backBufferDesc.Height = nResolvedHeight;
        CTexture* pTempCopyTex = CTexture::Create2DTexture("TempCopyTex", nResolvedWidth, nResolvedHeight, 1, FT_USAGE_RENDERTARGET, nullptr, eTF_Unknown, eTF_R8G8B8A8, false);
        if (pTempCopyTex)
        {
            CTexture* pSrc = pRenderTarget;
            RT_SetViewport(0, 0, nResolvedWidth, nResolvedHeight);
            if (!pSrc)
            {
                PostProcessUtils().CopyScreenToTexture(CTexture::s_ptexBackBuffer);
                pSrc = CTexture::s_ptexBackBuffer;
            }
            assert(pSrc);
            PostProcessUtils().StretchRect(pSrc, pTempCopyTex, false, false, false, false, SPostEffectsUtils::eDepthDownsample_None, false, nullptr);

            frameBufDesc.pBackBufferTex->Release();
            CDeviceTexture* pTempDeviceTex = pTempCopyTex->GetDevTexture();
            frameBufDesc.pBackBufferTex = pTempDeviceTex->Get2DTexture();
            // we're holding on to pTempCopyTex's d3dSurface. IncRef so releasing pTempCopyTex doesn't invalidate.
            // pBackBufferTex will release in frameBufDesc destructor.
            frameBufDesc.pBackBufferTex->AddRef();
            pTempCopyTex->Release();
        }
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // set up resources and textures for generating alpha channel from ZTarget texture, if needed
    ID3D11RenderTargetView* rtv = CTexture::s_ptexZTarget->GetSurface(0, 0);
    ID3D11Resource* zResource = nullptr;
    rtv->GetResource(&zResource);
    ID3D11Texture2D* zTargetTex = static_cast<ID3D11Texture2D*>(zResource);

    D3D11_TEXTURE2D_DESC zDesc;
    zTargetTex->GetDesc(&zDesc);

    D3D11_TEXTURE2D_DESC tmpZdesc = zDesc;
    tmpZdesc.Usage = D3D11_USAGE_STAGING;
    tmpZdesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    tmpZdesc.BindFlags = 0;


    HRESULT hrZ = GetDevice().CreateTexture2D(&tmpZdesc, NULL, &frameBufDesc.tempZtex);
    D3D11_MAPPED_SUBRESOURCE zMappedResource;

    // default to includeAlpha 'on'
    frameBufDesc.includeAlpha = frameBufDesc.tempZtex && tmpZdesc.Width == frameBufDesc.backBufferDesc.Width &&
        tmpZdesc.Height == frameBufDesc.backBufferDesc.Height;

    if (frameBufDesc.includeAlpha)
    {
        gcpRendD3D->GetDeviceContext().CopyResource(frameBufDesc.tempZtex, zTargetTex);
        hrZ = gcpRendD3D->GetDeviceContext().Map(frameBufDesc.tempZtex, 0, D3D11_MAP_READ, 0, &zMappedResource);

        frameBufDesc.depthData = reinterpret_cast<float*>(zMappedResource.pData);
        if (!frameBufDesc.depthData)
        {
            SAFE_RELEASE(frameBufDesc.tempZtex);
            frameBufDesc.includeAlpha = false;
        }
    }

    SAFE_RELEASE(zResource);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // set up resources and textures for backbuffer - uses passed in pointer
    D3D11_TEXTURE2D_DESC tmpDesc = frameBufDesc.backBufferDesc;
    tmpDesc.Usage = D3D11_USAGE_STAGING;
    tmpDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    tmpDesc.BindFlags = 0;

    
    HRESULT hr = GetDevice().CreateTexture2D(&tmpDesc, NULL, &frameBufDesc.pTmpTexture);
    if (!frameBufDesc.pTmpTexture)
    {
        return false;
    }

    gcpRendD3D->GetDeviceContext().CopyResource(frameBufDesc.pTmpTexture, frameBufDesc.pBackBufferTex);

    hr = gcpRendD3D->GetDeviceContext().Map(frameBufDesc.pTmpTexture, 0, D3D11_MAP_READ, 0, &frameBufDesc.resource);
    
    frameBufDesc.outputBytesPerPixel = frameBufDesc.includeAlpha ? 4 : 3;
    frameBufDesc.texSize = frameBufDesc.backBufferDesc.Width * frameBufDesc.backBufferDesc.Height * frameBufDesc.outputBytesPerPixel;

    
    assert(frameBufDesc.pDest == nullptr);
    frameBufDesc.pDest = new byte[frameBufDesc.texSize + sizeof(uint32)];  // Extra space required since we always copy 32 bits
    assert(frameBufDesc.pDest);

    return(SUCCEEDED(hr));
}

void CD3D9Renderer::FillFrameBuffer(FrameBufferDescription& frameBufDesc, bool redBlueSwap)
{
    // copy, flip red with blue if needed
    byte* dst = frameBufDesc.pDest;
    const byte* pSrc = reinterpret_cast<const byte*>(frameBufDesc.resource.pData);

    for (int i = 0; i < frameBufDesc.backBufferDesc.Height; i++)
    {
        for (int j = 0; j < frameBufDesc.backBufferDesc.Width; j++)
        {
            *(uint32*)&dst[j * frameBufDesc.outputBytesPerPixel] = *(uint32*)&pSrc[j * frameBufDesc.inputBytesPerPixel];
        }
        if (redBlueSwap)
        {
            PREFAST_ASSUME(texsize == frameBufDesc.backBufferDesc.Width * 4);
            for (int j = 0; j < frameBufDesc.backBufferDesc.Width; j++)
            {
                Exchange(dst[j * frameBufDesc.outputBytesPerPixel + 0], dst[j * frameBufDesc.outputBytesPerPixel + 2]);
            }
        }
        pSrc += frameBufDesc.resource.RowPitch;
        dst += frameBufDesc.backBufferDesc.Width * frameBufDesc.outputBytesPerPixel;
    }

    // re-loop over the image to process Alpha, if needed
    if (frameBufDesc.includeAlpha)
    {
        dst = frameBufDesc.pDest;
        const int numPx = frameBufDesc.backBufferDesc.Height * frameBufDesc.backBufferDesc.Width;

        const int alphaIDX = 3;
        const byte alphaOn = 255;
        const byte alphaOff = 0;

        for (int px = 0; px < numPx; px++)
        {
            // depthData is normalized - set alpha to 0 where depthData is 1.0f, 255 otherwise
            dst[px *  frameBufDesc.outputBytesPerPixel + alphaIDX] = (AZ::IsClose(frameBufDesc.depthData[px], 1.0f, FLT_EPSILON) ? alphaOff : alphaOn);
        }
    }
}



bool CD3D9Renderer::CaptureFrameBufferToFile(const char* pFilePath, CTexture* pRenderTarget)
{
    //file prep
    assert(pFilePath);
    if (!pFilePath)
    {
        return false;
    }

    enum EFileFormat
    {
        EFF_FILE_TGA, EFF_FILE_JPG, EFF_FILE_TIF
    };
    struct SCaptureFormatInfo
    {
        const char* pExt;
        EFileFormat fmt;
    };
    const SCaptureFormatInfo captureFormats[] =
    {
        {"tga", EFF_FILE_TGA}, {"jpg", EFF_FILE_JPG}, {"tif", EFF_FILE_TIF}
    };

    const char* pReqFileFormatExt(fpGetExtension(pFilePath));

    // resolve file format from file path
    int fmtIdx(-1);
    if (pReqFileFormatExt)
    {
        ++pReqFileFormatExt; // skip '.'
        for (int i(0); i < sizeof(captureFormats) / sizeof(captureFormats[0]); ++i)
        {
            PREFAST_SUPPRESS_WARNING(6387)
            if (!azstricmp(pReqFileFormatExt, captureFormats[i].pExt))
            {
                fmtIdx = i;
                break;
            }
        }
    }

    if (fmtIdx < 0)
    {
        if (iLog)
        {
            iLog->Log("Warning: Captured frame cannot be saved as \"%s\" format is not supported!\n", pReqFileFormatExt);
        }
        return false;
    }

    if (pRenderTarget && pRenderTarget->GetDstFormat() != eTF_R8G8B8A8)
    {
        if (iLog)
        {
            iLog->Log("Warning: Captured RenderTarget has unsupported format.\n");
        }
        return false;
    }

    //prep frame Buffer
    FrameBufferDescription frameBufDesc;
    bool framePrepSuccessful = PrepFrameCapture(frameBufDesc, pRenderTarget);
    bool writeToDiskSuccessful = false;

    if (!framePrepSuccessful)
    {
        return writeToDiskSuccessful;
    }

    //console code
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_CPP_SECTION_8
    #include AZ_RESTRICTED_FILE(DriverD3D_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    bool formatBGRA = false;
#endif

    bool needRBSwap = captureFormats[fmtIdx].fmt == EFF_FILE_TGA ? !formatBGRA : formatBGRA;

    //copy the frame buffer
    FillFrameBuffer(frameBufDesc, needRBSwap);


    //file capture
    if (captureFormats[fmtIdx].fmt == EFF_FILE_TGA)
    {
        writeToDiskSuccessful = ::WriteTGA(frameBufDesc.pDest, frameBufDesc.backBufferDesc.Width, frameBufDesc.backBufferDesc.Height, pFilePath, 8 * frameBufDesc.outputBytesPerPixel, 8 * frameBufDesc.outputBytesPerPixel);
    }
    else if (captureFormats[fmtIdx].fmt == EFF_FILE_JPG)
    {
        writeToDiskSuccessful = ::WriteJPG(frameBufDesc.pDest, frameBufDesc.backBufferDesc.Width, frameBufDesc.backBufferDesc.Height, pFilePath, 8 * frameBufDesc.outputBytesPerPixel, 90);
    }
    else
    {
        //If saving as tiff use our built in tiff saver.
        CRY_ASSERT_MESSAGE(captureFormats[fmtIdx].fmt == EFF_FILE_TIF, ("Error - unknown image extension"));
        writeToDiskSuccessful = InternalSaveToTIFF(frameBufDesc.pBackBufferTex, pFilePath);
    }

    return writeToDiskSuccessful;
}

bool CD3D9Renderer::InternalSaveToTIFF(ID3D11Texture2D* backBuffer, const char* filePath)
{
    bool result = false;
    //Have to copy from GPU to staging texture first.
    const int nResolvedWidth = aznumeric_cast<int>(aznumeric_cast<float>(m_width) / m_CurrContext->m_fPixelScaleX);
    const int nResolvedHeight = aznumeric_cast<int>(aznumeric_cast<float>(m_height) / m_CurrContext->m_fPixelScaleY);
    D3D11_TEXTURE2D_DESC bbDesc;
    backBuffer->GetDesc(&bbDesc);
    bbDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    bbDesc.Width = nResolvedWidth;
    bbDesc.Height = nResolvedHeight;
    bbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    bbDesc.Usage = D3D11_USAGE_STAGING;
    bbDesc.BindFlags = 0;

    ID3D11Texture2D* pTempCopyTex = nullptr;
    if (SUCCEEDED(GetDevice().CreateTexture2D(&bbDesc, 0, &pTempCopyTex)))
    {
        D3D11_BOX srcRegion;
        srcRegion.left = 0;
        srcRegion.right = nResolvedWidth;
        srcRegion.top = 0;
        srcRegion.bottom = nResolvedHeight;
        srcRegion.front = 0;
        srcRegion.back = 1;

        GetDeviceContext().CopySubresourceRegion(pTempCopyTex, 0, 0, 0, 0, backBuffer, 0, &srcRegion);
        backBuffer->Release();
        backBuffer = pTempCopyTex;
        {
            D3D11_MAPPED_SUBRESOURCE mapped;
            auto hr = GetDeviceContext().Map(backBuffer, 0, D3D11_MAP_READ, 0, &mapped);
            if (SUCCEEDED(hr))
            {
                auto releaseTexture = std17::scope_guard([&]() { GetDeviceContext().Unmap(backBuffer, 0); });

                std::vector<unsigned char> data;
                data.resize(nResolvedWidth * nResolvedHeight * 3);
                //strip alpha and padding, tiff util is rgb only currently. and we don't need alpha anyway.
                for (int y = 0; y < nResolvedHeight; ++y)
                {
                    auto srcrow = &((unsigned char*)mapped.pData)[y * mapped.RowPitch];
                    auto dstrow = &data[y * nResolvedWidth * 3];
                    for (int x = 0; x < nResolvedWidth; ++x)
                    {
                        dstrow[3 * x] = srcrow[4 * x];
                        dstrow[3 * x + 1] = srcrow[4 * x + 1];
                        dstrow[3 * x + 2] = srcrow[4 * x + 2];
                    }
                }
                auto shot = gEnv->pSystem->GetImageHandler()->CreateImage(std::move(data), nResolvedWidth, nResolvedHeight);
                if (shot)
                {
                    result = gEnv->pSystem->GetImageHandler()->SaveImage(shot.get(), filePath);
                }
            }
        }
    }

    SAFE_RELEASE(pTempCopyTex);

    return result;
}

#define DEPTH_BUFFER_SCALE 1024.0f

void CD3D9Renderer::CacheCaptureCVars()
{
    // cache console vars
    if (!CV_capture_frames || !CV_capture_folder || !CV_capture_frame_once ||
        !CV_capture_file_name || !CV_capture_file_prefix || !CV_capture_buffer)
    {
        ISystem* pSystem(GetISystem());
        if (!pSystem)
        {
            return;
        }

        IConsole* pConsole(gEnv->pConsole);
        if (!pConsole)
        {
            return;
        }

        CV_capture_frames = !CV_capture_frames ? pConsole->GetCVar("capture_frames") : CV_capture_frames;
        CV_capture_folder = !CV_capture_folder ? pConsole->GetCVar("capture_folder") : CV_capture_folder;
        CV_capture_frame_once = !CV_capture_frame_once ? pConsole->GetCVar("capture_frame_once") : CV_capture_frame_once;
        CV_capture_file_name = !CV_capture_file_name ? pConsole->GetCVar("capture_file_name") : CV_capture_file_name;
        CV_capture_file_prefix = !CV_capture_file_prefix ? pConsole->GetCVar("capture_file_prefix") : CV_capture_file_prefix;
        CV_capture_buffer = !CV_capture_buffer ? pConsole->GetCVar("capture_buffer") : CV_capture_buffer;
    }
}

void CD3D9Renderer::CaptureFrameBuffer()
{
    CDebugAllowFileAccess ignoreInvalidFileAccess;

    CacheCaptureCVars();
    if (!CV_capture_frames || !CV_capture_folder || !CV_capture_frame_once ||
        !CV_capture_file_name || !CV_capture_file_prefix || !CV_capture_buffer)
    {
        return;
    }

    int frameNum(CV_capture_frames->GetIVal());
    if (frameNum > 0)
    {
        char path[AZ::IO::IArchive::MaxPath];
        path[0] = '\0';

        const char* capture_file_name = CV_capture_file_name->GetString();
        if (capture_file_name && capture_file_name[0])
        {
            gEnv->pCryPak->AdjustFileName(capture_file_name, path, AZ_ARRAY_SIZE(path), AZ::IO::IArchive::FLAGS_PATH_REAL | AZ::IO::IArchive::FLAGS_FOR_WRITING);
        }

        if (path[0] == '\0')
        {
            gEnv->pCryPak->AdjustFileName(CV_capture_folder->GetString(), path, AZ_ARRAY_SIZE(path), AZ::IO::IArchive::FLAGS_PATH_REAL | AZ::IO::IArchive::FLAGS_FOR_WRITING);
            gEnv->pCryPak->MakeDir(path);

            char prefix[64] = "Frame";
            const char* capture_file_prefix = CV_capture_file_prefix->GetString();
            if (capture_file_prefix != 0 && capture_file_prefix[0])
            {
                cry_strcpy(prefix, capture_file_prefix);
            }

            size_t pathLen = strlen(path);
            snprintf(&path[pathLen], sizeof(path) - 1 - pathLen, "\\%s%06d.%s", prefix, frameNum - 1, "jpg");
        }

        if (CV_capture_frame_once->GetIVal())
        {
            CV_capture_frames->Set(0);
            CV_capture_frame_once->Set(0);
        }
        else
        {
            CV_capture_frames->Set(frameNum + 1);
        }

        if (!CaptureFrameBufferToFile(path))
        {
            if (iLog)
            {
                iLog->Log("Warning: Frame capture failed!\n");
            }
            CV_capture_frames->Set(0); // disable capturing
        }
    }
}

void CD3D9Renderer::ResolveSupersampledBackbuffer()
{
    if (IsEditorMode() && CV_r_Supersampling <= 1 )
    {
        return;
    }

    PROFILE_LABEL_SCOPE("RESOLVE_SUPERSAMPLED");

    SD3DPostEffectsUtils::EFilterType eFilter = SD3DPostEffectsUtils::FilterType_Box;
    if (CV_r_SupersamplingFilter == 1)
    {
        eFilter = SD3DPostEffectsUtils::FilterType_Tent;
    }
    else if (CV_r_SupersamplingFilter == 2)
    {
        eFilter = SD3DPostEffectsUtils::FilterType_Gauss;
    }
    else if (CV_r_SupersamplingFilter == 3)
    {
        eFilter = SD3DPostEffectsUtils::FilterType_Lanczos;
    }

    if (IsEditorMode())
    {
        const int nResolvedWidth = aznumeric_cast<int>(aznumeric_cast<float>(m_width) / m_CurrContext->m_fPixelScaleX);
        const int nResolvedHeight = aznumeric_cast<int>(aznumeric_cast<float>(m_height) / m_CurrContext->m_fPixelScaleY);
        RT_SetViewport(0, 0, m_width, m_height);
        PostProcessUtils().CopyScreenToTexture(CTexture::s_ptexBackBuffer);
        PostProcessUtils().Downsample(CTexture::s_ptexBackBuffer, NULL,
            m_width, m_height,
            nResolvedWidth, nResolvedHeight,
            eFilter, false);
    }
    else
    {
        PostProcessUtils().Downsample(CTexture::s_ptexSceneSpecular, NULL,
            m_width, m_height,
            m_backbufferWidth, m_backbufferHeight,
            eFilter, false);
    }
}

void CD3D9Renderer::ScaleBackbufferToViewport()
{
    // fPixelScale comes from OS screen scale as well as super sampling, so check both are active before scaling backbuffer
    if ( CV_r_Supersampling > 1 && (m_CurrContext->m_fPixelScaleX > 1.0f || m_CurrContext->m_fPixelScaleY > 1.0f))
    {
        PROFILE_LABEL_SCOPE("STRETCH_TO_VIEWPORT");

        const int nResolvedWidth = aznumeric_cast<int>(aznumeric_cast<float>(m_width) / m_CurrContext->m_fPixelScaleX);
        const int nResolvedHeight = aznumeric_cast<int>(aznumeric_cast<float>(m_height) / m_CurrContext->m_fPixelScaleY);

        RECT srcRegion;
        srcRegion.left = 0;
        srcRegion.right = nResolvedWidth;
        srcRegion.top = 0;
        srcRegion.bottom = nResolvedHeight;
        // copy resolved part of backbuffer to temporary target
        PostProcessUtils().CopyScreenToTexture(CTexture::s_ptexBackBufferScaled[0], &srcRegion);
        // stretch back to backbuffer
        PostProcessUtils().CopyTextureToScreen(CTexture::s_ptexBackBufferScaled[0], &srcRegion);
    }
}

void CD3D9Renderer::DebugDrawRect([[maybe_unused]] float x1, [[maybe_unused]] float y1, [[maybe_unused]] float x2, [[maybe_unused]] float y2, [[maybe_unused]] float* fColor)
{
#ifndef _RELEASE
    SetMaterialColor(fColor[0], fColor[1], fColor[2], fColor[3]);
    int w = GetWidth();
    int h = GetHeight();
    float dx = 1.0f / w;
    float dy = 1.0f / h;
    x1 *= dx;
    x2 *= dx;
    y1 *= dy;
    y2 *= dy;

    ColorB col((uint8)(fColor[0] * 255.0f), (uint8)(fColor[1] * 255.0f), (uint8)(fColor[2] * 255.0f), (uint8)(fColor[3] * 255.0f));

    IRenderAuxGeom* pAux = GetIRenderAuxGeom();
    SAuxGeomRenderFlags flags = pAux->GetRenderFlags();
    flags.SetMode2D3DFlag(e_Mode2D);
    pAux->SetRenderFlags(flags);
    pAux->DrawLine(Vec3(x1, y1, 0), col, Vec3(x2, y1, 0), col);
    pAux->DrawLine(Vec3(x1, y2, 0), col, Vec3(x2, y2, 0), col);
    pAux->DrawLine(Vec3(x1, y1, 0), col, Vec3(x1, y2, 0), col);
    pAux->DrawLine(Vec3(x2, y1, 0), col, Vec3(x2, y2, 0), col);
#endif
}

void CD3D9Renderer::PrintResourcesLeaks()
{
#ifndef _RELEASE
    iLog->Log("\n \n");

    AUTO_LOCK(CBaseResource::s_cResLock);

    CCryNameTSCRC Name;
    SResourceContainer* pRL;
    uint32 n = 0;
    uint32 i;
    Name = CShader::mfGetClassName();
    pRL = CBaseResource::GetResourcesForClass(Name);
    if (pRL)
    {
        ResourcesMapItor itor;
        for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
        {
            CShader* sh = (CShader*)itor->second;
            if (!sh)
            {
                continue;
            }
            Warning("--- CShader '%s' leak after level unload", sh->GetName());
            n++;
        }
    }
    iLog->Log("\n \n");

    n = 0;
    for (i = 0; i < CShader::s_ShaderResources_known.Num(); i++)
    {
        CShaderResources* pSR = CShader::s_ShaderResources_known[i];
        if (!pSR)
        {
            continue;
        }
        n++;
        if (pSR->m_szMaterialName)
        {
            Warning("--- Shader Resource '%s' leak after level unload", pSR->m_szMaterialName);
        }
    }
    if (!n)
    {
        CShader::s_ShaderResources_known.Free();
    }
    iLog->Log("\n \n");

    int nVS = 0;
    Name = CHWShader::mfGetClassName(eHWSC_Vertex);
    pRL = CBaseResource::GetResourcesForClass(Name);
    if (pRL)
    {
        ResourcesMapItor itor;
        for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
        {
            CHWShader* vsh = (CHWShader*)itor->second;
            if (!vsh)
            {
                continue;
            }
            nVS++;
            Warning("--- Vertex Shader '%s' leak after level unload", vsh->GetName());
        }
    }
    iLog->Log("\n \n");

    int nPS = 0;
    Name = CHWShader::mfGetClassName(eHWSC_Pixel);
    pRL = CBaseResource::GetResourcesForClass(Name);
    if (pRL)
    {
        ResourcesMapItor itor;
        for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
        {
            CHWShader* psh = (CHWShader*)itor->second;
            if (!psh)
            {
                continue;
            }
            nPS++;
            Warning("--- Pixel Shader '%s' leak after level unload", psh->GetName());
        }
    }
    iLog->Log("\n \n");

    n = 0;
    Name = CTexture::mfGetClassName();
    pRL = CBaseResource::GetResourcesForClass(Name);
    if (pRL)
    {
        ResourcesMapItor itor;
        for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
        {
            CTexture* pTX = (CTexture*)itor->second;
            if (!pTX)
            {
                continue;
            }
            if (!pTX->m_bCreatedInLevel)
            {
                continue;
            }
            Warning("--- CTexture '%s' leak after level unload", pTX->GetName());
            n++;
        }
    }
    iLog->Log("\n \n");

    CRendElement* pRE;
    for (pRE = CRendElement::m_RootGlobal.m_NextGlobal; pRE != &CRendElement::m_RootGlobal; pRE = pRE->m_NextGlobal)
    {
        Warning("--- CRenderElement %s leak after level unload", pRE->mfTypeString());
    }
    iLog->Log("\n \n");

    CRenderMesh::PrintMeshLeaks();
#endif
}

#define BYTES_TO_KB(bytes) ((bytes) / 1024.0)
#define BYTES_TO_MB(bytes) ((bytes) / 1024.0 / 1024.0)

void CD3D9Renderer::DebugDrawStats1()
{
#if !defined(EXCLUDE_RARELY_USED_R_STATS) && defined(ENABLE_PROFILING_CODE)
    const int nYstep = 10;
    uint32 i;
    int nY = 30; // initial Y pos
    int nX = 50; // initial X pos

    ColorF col = Col_Yellow;
    Draw2dLabel(nX, nY, 2.0f, &col.r, false, "Per-frame stats:");

    col = Col_White;
    nX += 10;
    nY += 25;
    //DebugDrawRect(nX-2, nY, nX+180, nY+150, &col.r);
    Draw2dLabel(nX, nY, 1.4f, &col.r, false, "Draw-calls:");

    float fFSize = 1.2f;

    nX += 5;
    nY += 10;
    int nXBars = nX;

    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "General: %d (%d polys, %.3fms)", m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[EFSLIST_GENERAL], m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[EFSLIST_GENERAL], (m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_GENERAL] + m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPsZ) * 1000.0f);
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Decals: %d (%d polys, %.3fms)", m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[EFSLIST_DECAL], m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[EFSLIST_DECAL], m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_DECAL] * 1000.0f);
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Transparent: %d (%d polys, %.3fms)", m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[EFSLIST_TRANSP], m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[EFSLIST_TRANSP], m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_TRANSP] * 1000.0f);
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Shadow-gen: %d (%d polys, %.3fms)", m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[EFSLIST_SHADOW_GEN], m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[EFSLIST_SHADOW_GEN], m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_SHADOW_GEN] * 1000.0f);
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Shadow-pass: %d (%d polys)", m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[EFSLIST_SHADOW_PASS], m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[EFSLIST_SHADOW_PASS]);
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Water: %d (%d polys, %.3fms)", m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[EFSLIST_WATER], m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[EFSLIST_WATER], m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_WATER_VOLUMES] * 1000.0f);
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Refractive Surface: %d (%d polys, %.3fms)", m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[EFSLIST_REFRACTIVE_SURFACE], m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[EFSLIST_REFRACTIVE_SURFACE], m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_REFRACTIVE_SURFACE] * 1000.0f);
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Imposters: %d (Updates: %d)", m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumCloudImpostersDraw, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumCloudImpostersUpdates);
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Sprites: %d (%d dips, %d updates, %d altases, %d cells, %d polys)", m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumSprites, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumSpriteDIPS, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumSpriteUpdates, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumSpriteAltasesUsed, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumSpriteCellsUsed, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumSpritePolys /*, m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPsSprites*1000.0f*/);

    Draw2dLabel(nX - 5, nY + 20, 1.4f, &col.r, false, "Total: %d (%d polys)", GetCurrentNumberOfDrawCalls(), GetPolyCount());

    col = Col_Yellow;
    nX -= 5;
    nY += 45;
    //DebugDrawRect(nX-2, nY, nX+180, nY+160, &col.r);
    Draw2dLabel(nX, nY, 1.4f, &col.r, false, "Occlusions: Issued: %d, Occluded: %d, NotReady: %d", m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumQIssued, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumQOccluded, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumQNotReady);

    col = Col_Cyan;
    nX -= 5;
    nY += 45;
    //DebugDrawRect(nX-2, nY, nX+180, nY+160, &col.r);
    Draw2dLabel(nX, nY, 1.4f, &col.r, false, "Device resource switches:");

    nX += 5;
    nY += 10;
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "VShaders: %d (%d unique)", m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumVShadChanges, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumVShaders);
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "PShaders: %d (%d unique)", m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumPShadChanges, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumPShaders);
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Textures: %d (%d unique)", m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumTextChanges, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumTextures);
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "RT's: %d (%d unique), cleared: %d times, copied: %d times", m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumRTChanges, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumRTs, m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTCleared, m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTCopied);
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "States: %d", m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumStateChanges);
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "MatBatches: %d, GeomBatches: %d, Instances: %d", m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumRendMaterialBatches, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumRendGeomBatches, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumRendInstances);
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "HW Instances: DIP's: %d, Instances: %d (polys: %d/%d)", m_RP.m_PS[m_RP.m_nProcessThreadID].m_RendHWInstancesDIPs, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumRendHWInstances, m_RP.m_PS[m_RP.m_nProcessThreadID].m_RendHWInstancesPolysOne, m_RP.m_PS[m_RP.m_nProcessThreadID].m_RendHWInstancesPolysAll);
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Skinned instances: %d", m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumRendSkinnedObjects);

    CHWShader_D3D* ps = (CHWShader_D3D*)m_RP.m_PS[m_RP.m_nProcessThreadID].m_pMaxPShader;
    CHWShader_D3D::SHWSInstance* pInst = (CHWShader_D3D::SHWSInstance*)m_RP.m_PS[m_RP.m_nProcessThreadID].m_pMaxPSInstance;
    if (ps)
    {
        Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "MAX PShader: %s (instructions: %d, lights: %d)", ps->GetName(), pInst->m_nInstructions, pInst->m_Ident.m_LightMask & 0xf);
    }
    CHWShader_D3D* vs = (CHWShader_D3D*)m_RP.m_PS[m_RP.m_nProcessThreadID].m_pMaxVShader;
    pInst = (CHWShader_D3D::SHWSInstance*)m_RP.m_PS[m_RP.m_nProcessThreadID].m_pMaxVSInstance;
    if (vs)
    {
        Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "MAX VShader: %s (instructions: %d, lights: %d)", vs->GetName(), pInst->m_nInstructions, pInst->m_Ident.m_LightMask & 0xf);
    }

    col = Col_Green;
    nX -= 5;
    nY += 35;
    //DebugDrawRect(nX-2, nY, nX+180, nY+160, &col.r);
    Draw2dLabel(nX, nY, 1.4f, &col.r, false, "Device resource sizes:");

    nX += 5;
    nY += 10;
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Managed non-streamed textures: Sys=%.3f Mb, Vid:=%.3f", BYTES_TO_MB(m_RP.m_PS[m_RP.m_nProcessThreadID].m_ManagedTexturesSysMemSize), BYTES_TO_MB(m_RP.m_PS[m_RP.m_nProcessThreadID].m_ManagedTexturesVidMemSize));
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Managed streamed textures: Sys=%.3f Mb, Vid:=%.3f", BYTES_TO_MB(m_RP.m_PS[m_RP.m_nProcessThreadID].m_ManagedTexturesStreamSysSize), BYTES_TO_MB(m_RP.m_PS[m_RP.m_nProcessThreadID].m_ManagedTexturesStreamVidSize));
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "RT textures: Used: %.3f Mb, Updated: %.3f Mb, Cleared: %.3f Mb, Copied: %.3f Mb", BYTES_TO_MB(m_RP.m_PS[m_RP.m_nProcessThreadID].m_DynTexturesSize), BYTES_TO_MB(m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTSize), BYTES_TO_MB(m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTClearedSize), BYTES_TO_MB(m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTCopiedSize));
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Meshes updated: Static: %.3f Mb, Dynamic: %.3f Mb", BYTES_TO_MB(m_RP.m_PS[m_RP.m_nProcessThreadID].m_MeshUpdateBytes), BYTES_TO_MB(m_RP.m_PS[m_RP.m_nProcessThreadID].m_DynMeshUpdateBytes));
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Cloud textures updated: %.3f Mb", BYTES_TO_MB(m_RP.m_PS[m_RP.m_nProcessThreadID].m_CloudImpostersSizeUpdate));

    int nYBars = nY;

    nY = 30; // initial Y pos
    nX = 430; // initial X pos
    col = Col_Yellow;
    Draw2dLabel(nX, nY, 2.0f, &col.r, false, "Global stats:");

    col = Col_YellowGreen;
    nX += 10;
    nY += 55;
    Draw2dLabel(nX, nY, 1.4f, &col.r, false, "Mesh size:");

    size_t nMemApp = 0;
    size_t nMemDevVB = 0;
    size_t nMemDevIB = 0;
    {
        AUTO_LOCK(CRenderMesh::m_sLinkLock);
        for (util::list<CRenderMesh>* iter = CRenderMesh::m_MeshList.prev; iter != &CRenderMesh::m_MeshList; iter = iter->prev)
        {
            CRenderMesh* pRM = iter->item<&CRenderMesh::m_Chain>();
            nMemApp += pRM->Size(CRenderMesh::SIZE_ONLY_SYSTEM);
            nMemDevVB += pRM->Size(CRenderMesh::SIZE_VB);
            nMemDevIB += pRM->Size(CRenderMesh::SIZE_IB);
        }
    }
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Static: (app: %.3f Mb, dev VB: %.3f Mb, dev IB: %.3f Mb)",
        nMemApp / 1024.0f / 1024.0f, nMemDevVB / 1024.0f / 1024.0f, nMemDevIB / 1024.0f / 1024.0f);

    for (i = 0; i < BBT_MAX; i++)
    {
        for (int j = 0; j < BU_MAX; j++)
        {
            SDeviceBufferPoolStats stats;
            if (m_DevBufMan.GetStats((BUFFER_BIND_TYPE)i, (BUFFER_USAGE)j, stats) == false)
            {
                continue;
            }
            Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Pool '%10s': size %5.3f banks %4" PRISIZE_T " allocs %6d frag %3.3f pinned %4d moving %4d",
                stats.buffer_descr.c_str()
                , (stats.num_banks * stats.bank_size) / (1024.f * 1024.f)
                , stats.num_banks
                , stats.allocator_stats.nInUseBlocks
                , (stats.allocator_stats.nCapacity - stats.allocator_stats.nInUseSize - stats.allocator_stats.nLargestFreeBlockSize) / (float)max(stats.allocator_stats.nCapacity, (size_t)1)
                , stats.allocator_stats.nPinnedBlocks
                , stats.allocator_stats.nMovingBlocks);
        }
    }

    nMemDevVB = 0;
    nMemDevIB = 0;
    nMemApp = m_RP.m_SizeSysArray;

    uint32 j;
    for (i = 0; i < SHAPE_MAX; i++)
    {
        nMemDevVB += _VertBufferSize(m_pUnitFrustumVB[i]);
        nMemDevIB += _IndexBufferSize(m_pUnitFrustumIB[i]);
    }

#if defined(ENABLE_RENDER_AUX_GEOM)
    if (m_pRenderAuxGeomD3D)
    {
        nMemDevVB += m_pRenderAuxGeomD3D->GetDeviceDataSize();
    }
#endif

    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Dynamic: %.3f (app: %.3f Mb, dev VB: %.3f Mb, dev IB: %.3f Mb)", BYTES_TO_MB(nMemDevVB + nMemDevIB), BYTES_TO_MB(nMemApp), BYTES_TO_MB(nMemDevVB), BYTES_TO_MB(nMemDevIB) /*, BYTES_TO_MB(nMemDevVBPool)*/);

    CCryNameTSCRC Name;
    SResourceContainer* pRL;
    uint32 n = 0;
    size_t nSize = 0;
    Name = CShader::mfGetClassName();
    pRL = CBaseResource::GetResourcesForClass(Name);
    if (pRL)
    {
        ResourcesMapItor itor;
        for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
        {
            CShader* sh = (CShader*)itor->second;
            if (!sh)
            {
                continue;
            }
            nSize += sh->Size(0);
            n++;
        }
    }
    nY += nYstep;
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "FX Shaders: %d (size: %.3f Mb)", n, BYTES_TO_MB(nSize));

    n = 0;
    nSize = m_cEF.m_Bin.mfSizeFXParams(n);
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "FX Shader parameters for %d shaders (size: %.3f Mb)", n, BYTES_TO_MB(nSize));

    nSize = 0;
    n = 0;
    for (i = 0; i < (int)CShader::s_ShaderResources_known.Num(); i++)
    {
        CShaderResources* pSR = CShader::s_ShaderResources_known[i];
        if (!pSR)
        {
            continue;
        }
        nSize += pSR->Size();
        n++;
    }
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Shader resources: %d (size: %.3f Mb)", n, BYTES_TO_MB(nSize));
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Shader manager (size: %.3f Mb)", BYTES_TO_MB(m_cEF.Size()));

    n = 0;
    for (i = 0; i < CGParamManager::s_Groups.size(); i++)
    {
        n += CGParamManager::s_Groups[i].nParams;
    }
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Groups: %d, Shader parameters: %d (size: %.3f Mb), in pool: %d (size: %.3f Mb)", i, n, BYTES_TO_MB(n * sizeof(SCGParam)), CGParamManager::s_Pools.size(), BYTES_TO_MB(CGParamManager::s_Pools.size() * PARAMS_POOL_SIZE * sizeof(SCGParam)));

    nY += nYstep;
    nY += nYstep;
    TArray<void*> ShadersVS;
    TArray<void*> ShadersPS;

    nSize = 0;
    n = 0;
    int nInsts = 0;
    Name = CHWShader::mfGetClassName(eHWSC_Vertex);
    pRL = CBaseResource::GetResourcesForClass(Name);
    if (pRL)
    {
        ResourcesMapItor itor;
        for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
        {
            CHWShader* vsh = (CHWShader*)itor->second;
            if (!vsh)
            {
                continue;
            }
            nSize += vsh->Size();
            n++;

            CHWShader_D3D* pD3D = (CHWShader_D3D*)vsh;
            for (i = 0; i < pD3D->m_Insts.size(); i++)
            {
                CHWShader_D3D::SHWSInstance* pShInst = pD3D->m_Insts[i];
                if (pShInst->m_bDeleted)
                {
                    continue;
                }
                nInsts++;
                if (pShInst->m_Handle.m_pShader)
                {
                    for (j = 0; j < (int)ShadersVS.Num(); j++)
                    {
                        if (ShadersVS[j] == pShInst->m_Handle.m_pShader->m_pHandle)
                        {
                            break;
                        }
                    }
                    if (j == ShadersVS.Num())
                    {
                        ShadersVS.AddElem(pShInst->m_Handle.m_pShader->m_pHandle);
                    }
                }
            }
        }
    }
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "VShaders: %d (size: %.3f Mb), Instances: %d, Device VShaders: %d (Size: %.3f Mb)", n, BYTES_TO_MB(nSize), nInsts, ShadersVS.Num(), BYTES_TO_MB(CHWShader_D3D::s_nDeviceVSDataSize));

    nInsts = 0;
    Name = CHWShader::mfGetClassName(eHWSC_Pixel);
    pRL = CBaseResource::GetResourcesForClass(Name);
    if (pRL)
    {
        ResourcesMapItor itor;
        for (itor = pRL->m_RMap.begin(); itor != pRL->m_RMap.end(); itor++)
        {
            CHWShader* psh = (CHWShader*)itor->second;
            if (!psh)
            {
                continue;
            }
            nSize += psh->Size();
            n++;

            CHWShader_D3D* pD3D = (CHWShader_D3D*)psh;
            for (i = 0; i < pD3D->m_Insts.size(); i++)
            {
                CHWShader_D3D::SHWSInstance* pShInst = pD3D->m_Insts[i];
                if (pShInst->m_bDeleted)
                {
                    continue;
                }
                nInsts++;
                if (pShInst->m_Handle.m_pShader)
                {
                    for (j = 0; j < (int)ShadersPS.Num(); j++)
                    {
                        if (ShadersPS[j] == pShInst->m_Handle.m_pShader->m_pHandle)
                        {
                            break;
                        }
                    }
                    if (j == ShadersPS.Num())
                    {
                        ShadersPS.AddElem(pShInst->m_Handle.m_pShader->m_pHandle);
                    }
                }
            }
        }
    }
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "PShaders: %d (size: %.3f Mb), Instances: %d, Device PShaders: %d (Size: %.3f Mb)", n, BYTES_TO_MB(nSize), nInsts, ShadersPS.Num(), BYTES_TO_MB(CHWShader_D3D::s_nDevicePSDataSize));

    n = 0;
    nSize = 0;
    size_t nSizeD = 0;
    size_t nSizeAll = 0;
    for (FXCompressedShadersItor it = CHWShader::m_CompressedShaders.begin(); it != CHWShader::m_CompressedShaders.end(); ++it)
    {
        SHWActivatedShader* pAS = it->second;
        for (FXCompressedShaderItor itor = pAS->m_CompressedShaders.begin(); itor != pAS->m_CompressedShaders.end(); ++itor)
        {
            n++;
            SCompressedData& Data = itor->second;
            nSize += Data.m_nSizeCompressedShader;
            nSizeD += Data.m_nSizeDecompressedShader;
        }
    }
    nSizeAll = sizeOfMapP(CHWShader::m_CompressedShaders);
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Compressed Shaders in memory: %d (size: %.3f Mb), Decompressed size: %.3f Mb, Overall: %.3f", n, BYTES_TO_MB(nSize), BYTES_TO_MB(nSizeD), BYTES_TO_MB(nSizeAll));

    FXShaderCacheItor FXitor;
    size_t nCache = 0;
    nSize = 0;
    for (FXitor = CHWShader::m_ShaderCache.begin(); FXitor != CHWShader::m_ShaderCache.end(); FXitor++)
    {
        SShaderCache* sc = FXitor->second;
        if (!sc)
        {
            continue;
        }
        nCache++;
        nSize += sc->Size();
    }
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Shader Cache: %" PRISIZE_T " (size: %.3f Mb)", nCache, BYTES_TO_MB(nSize));

    nSize = 0;
    n = 0;
    CRendElement* pRE = CRendElement::m_RootGlobal.m_NextGlobal;
    while (pRE != &CRendElement::m_RootGlobal)
    {
        n++;
        nSize += pRE->Size();
        pRE = pRE->m_NextGlobal;
    }
    nY += nYstep;
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Render elements: %d (size: %.3f Mb)", n, BYTES_TO_MB(nSize));

    size_t nSAll = 0;
    size_t nSOneMip = 0;
    size_t nSNM = 0;
    size_t nSysAll = 0;
    size_t nSysOneMip = 0;
    size_t nSysNM = 0;
    size_t nSRT = 0;
    size_t nObjSize = 0;
    size_t nStreamed = 0;
    size_t nStreamedSys = 0;
    size_t nStreamedUnload = 0;
    n = 0;
    pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
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
            n++;
            nObjSize += tp->GetSize(true);
            int nS = tp->GetDeviceDataSize();
            int nSys = tp->GetDataSize();
            if (tp->IsStreamed())
            {
                if (tp->IsUnloaded())
                {
                    assert(nS == 0);
                    nStreamedUnload += nSys;
                }
                else
                if (tp->GetDevTexture())
                {
                    nStreamedSys += nSys;
                }
                nStreamed += nS;
            }
            if (tp->GetDevTexture())
            {
                if (!(tp->GetFlags() & (FT_USAGE_DYNAMIC | FT_USAGE_RENDERTARGET)))
                {
                    if (tp->GetName()[0] != '$' && tp->GetNumMips() <= 1)
                    {
                        nSysOneMip += nSys;
                    }
                    if (tp->GetFlags() & FT_TEX_NORMAL_MAP)
                    {
                        nSysNM += nSys;
                    }
                    else
                    {
                        nSysAll += nSys;
                    }
                }
            }
            if (!nS)
            {
                continue;
            }
            if (tp->GetFlags() & (FT_USAGE_DYNAMIC | FT_USAGE_RENDERTARGET))
            {
                nSRT += nS;
            }
            else
            {
                if (tp->GetName()[0] != '$' && tp->GetNumMips() <= 1)
                {
                    nSOneMip += nS;
                }
                if (tp->GetFlags() & FT_TEX_NORMAL_MAP)
                {
                    nSNM += nS;
                }
                else
                {
                    nSAll += nS;
                }
            }
        }
    }

    nY += nYstep;
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "CryName: %d, Size: %.3f Mb...", CCryNameR::GetNumberOfEntries(), BYTES_TO_MB(CCryNameR::GetMemoryUsage()));
    nY += nYstep;

    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, "Textures: %d, ObjSize: %.3f Mb...", n, BYTES_TO_MB(nObjSize));
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, " All Managed Video Size: %.3f Mb (Normals: %.3f Mb + Other: %.3f Mb), One mip: %.3f", BYTES_TO_MB(nSNM + nSAll), BYTES_TO_MB(nSNM), BYTES_TO_MB(nSAll), BYTES_TO_MB(nSOneMip));
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, " All Managed System Size: %.3f Mb (Normals: %.3f Mb + Other: %.3f Mb), One mip: %.3f", BYTES_TO_MB(nSysNM + nSysAll), BYTES_TO_MB(nSysNM), BYTES_TO_MB(nSysAll), BYTES_TO_MB(nSysOneMip));
    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, " Streamed Size: Video: %.3f, System: %.3f, Unloaded: %.3f", BYTES_TO_MB(nStreamed), BYTES_TO_MB(nStreamedSys), BYTES_TO_MB(nStreamedUnload));

    SDynTexture_Shadow* pTXSH = SDynTexture_Shadow::s_RootShadow.m_NextShadow;
    size_t nSizeSH = 0;
    while (pTXSH != &SDynTexture_Shadow::s_RootShadow)
    {
        if (pTXSH->m_pTexture)
        {
            nSizeSH += pTXSH->m_pTexture->GetDeviceDataSize();
        }
        pTXSH = pTXSH->m_NextShadow;
    }

    size_t nSizeAtlasClouds = SDynTexture2::s_nMemoryOccupied[eTP_Clouds];
    size_t nSizeAtlasSprites = SDynTexture2::s_nMemoryOccupied[eTP_Sprites];
    size_t nSizeAtlas = nSizeAtlasClouds + nSizeAtlasSprites;
    size_t nSizeManagedDyn = SDynTexture::s_nMemoryOccupied;

    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, " Dynamic DataSize: %.3f Mb (Atlases: %.3f Mb, Managed: %.3f Mb (Shadows: %.3f Mb), Other: %.3f Mb)", BYTES_TO_MB(nSRT), BYTES_TO_MB(nSizeAtlas), BYTES_TO_MB(nSizeManagedDyn), BYTES_TO_MB(nSizeSH), BYTES_TO_MB(nSRT - nSizeManagedDyn - nSizeAtlas));

    size_t nSizeZRT = 0;
    size_t nSizeCRT = 0;

    if (m_DepthBufferOrig.pSurf)
    {
        nSizeZRT += m_DepthBufferOrig.nWidth * m_DepthBufferOrig.nHeight * 4;
    }
    if (m_DepthBufferOrigMSAA.pSurf && m_DepthBufferOrig.pSurf != m_DepthBufferOrigMSAA.pSurf)
    {
        nSizeZRT += m_DepthBufferOrig.nWidth * m_DepthBufferOrig.nHeight * 2 * 4;
    }
    for (i = 0; i < (int)m_TempDepths.Num(); i++)
    {
        SDepthTexture* pSrf = m_TempDepths[i];
        if (pSrf->pSurf)
        {
            nSizeZRT += pSrf->nWidth * pSrf->nHeight * 4;
        }
    }

    nSizeCRT += m_d3dsdBackBuffer.Width * m_d3dsdBackBuffer.Height * 4 * 2;

    Draw2dLabel(nX, nY += nYstep, fFSize, &col.r, false, " Targets DataSize: %.3f Mb (Color Buffer RT's: %.3f Mb, Z-Buffers: %.3f Mb", BYTES_TO_MB(nSizeCRT + nSizeZRT), BYTES_TO_MB(nSizeCRT), BYTES_TO_MB(nSizeZRT));

    DebugPerfBars(nXBars, nYBars + 30);
#endif
}

void CD3D9Renderer::DebugVidResourcesBars([[maybe_unused]] int nX, [[maybe_unused]] int nY)
{
#if !defined(EXCLUDE_RARELY_USED_R_STATS)
    int i;
    int nYst = 15;
    float fFSize = 1.4f;
    ColorF col = Col_Yellow;

    // Draw performance bars
    EF_SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
    EF_SetSrgbWrite(false);
    FX_SetState(GS_NODEPTHTEST);

    float fMaxBar = 200;
    float fOffs = 190.0f;

    ColorF colT = Col_Gray;
    Draw2dLabel(nX + 50, nY, 1.6f, &colT.r, false, "Video resources:");
    nY += 20;

    double fMaxTextureMemory = m_MaxTextureMemory * 1024.0 * 1024.0;

    ColorF colF = Col_Orange;
    Draw2dLabel(nX, nY, fFSize, &colF.r, false, "Total memory: %.1f Mb", BYTES_TO_MB(fMaxTextureMemory));
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 1, nX + fOffs + fMaxBar, nY + 12, Col_Cyan, 1.0f);
    nY += nYst;

    SDynTexture_Shadow* pTXSH = SDynTexture_Shadow::s_RootShadow.m_NextShadow;
    size_t nSizeSH = 0;
    while (pTXSH != &SDynTexture_Shadow::s_RootShadow)
    {
        if (pTXSH->m_pTexture)
        {
            nSizeSH += pTXSH->m_pTexture->GetDeviceDataSize();
        }
        pTXSH = pTXSH->m_NextShadow;
    }
    Draw2dLabel(nX, nY, fFSize, &col.r, false, "Shadow textures: %.1f Mb", BYTES_TO_MB(nSizeSH));
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 1, nX + fOffs + (float)nSizeSH / fMaxTextureMemory * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst;

    SDynTexture* pTX = SDynTexture::s_Root.m_Next;
    size_t nSizeD = 0;
    while (pTX != &SDynTexture::s_Root)
    {
        if (pTX->m_pTexture)
        {
            nSizeD += pTX->m_pTexture->GetDeviceDataSize();
        }
        pTX = pTX->m_Next;
    }
    nSizeD -= nSizeSH;
    Draw2dLabel(nX, nY, fFSize, &col.r, false, "Dyn. text.: %.1f Mb", BYTES_TO_MB(nSizeD));
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 1, nX + fOffs + (float)nSizeD / fMaxTextureMemory * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst;

    size_t nSizeD2 = 0;
    for (i = 0; i < eTP_Max; i++)
    {
        nSizeD2 += SDynTexture2::s_nMemoryOccupied[i];
    }
    Draw2dLabel(nX, nY, fFSize, &col.r, false, "Dyn. atlas text.: %.1f Mb", BYTES_TO_MB(nSizeD2));
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 1, nX + fOffs + (float)nSizeD2 / fMaxTextureMemory * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst;

    size_t nSizeZRT = 0;
    size_t nSizeCRT = 0;
    if (m_DepthBufferOrig.pSurf)
    {
        nSizeZRT += m_DepthBufferOrig.nWidth * m_DepthBufferOrig.nHeight * 4;
    }
    if (m_DepthBufferOrigMSAA.pSurf && m_DepthBufferOrig.pSurf != m_DepthBufferOrigMSAA.pSurf)
    {
        nSizeZRT += m_DepthBufferOrig.nWidth * m_DepthBufferOrig.nHeight * 2 * 4;
    }
    for (i = 0; i < (int)m_TempDepths.Num(); i++)
    {
        SDepthTexture* pSrf = m_TempDepths[i];
        if (pSrf->pSurf)
        {
            nSizeZRT += pSrf->nWidth * pSrf->nHeight * 4;
        }
    }
    nSizeCRT += m_d3dsdBackBuffer.Width * m_d3dsdBackBuffer.Height * 4 * 2;
    nSizeCRT += nSizeZRT;
    Draw2dLabel(nX, nY, fFSize, &col.r, false, "Frame targets: %.1f Mb", BYTES_TO_MB(nSizeCRT));
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 1, nX + fOffs + (float)nSizeCRT / fMaxTextureMemory * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst;

    size_t nSAll = 0;
    size_t nSOneMip = 0;
    size_t nSNM = 0;
    size_t nSRT = 0;
    size_t nObjSize = 0;
    size_t nStreamed = 0;
    size_t nStreamedSys = 0;
    size_t nStreamedUnload = 0;
    i = 0;
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
            i++;
            nObjSize += tp->GetSize(true);
            int nS = tp->GetDeviceDataSize();
            if (tp->IsStreamed())
            {
                int nSizeSys = tp->GetDataSize();
                if (tp->IsUnloaded())
                {
                    assert(nS == 0);
                    nStreamedUnload += nSizeSys;
                }
                else
                {
                    nStreamedSys += nSizeSys;
                }
                nStreamed += nS;
            }
            if (!nS)
            {
                continue;
            }
            if (tp->GetFlags() & (FT_USAGE_DYNAMIC | FT_USAGE_RENDERTARGET))
            {
                nSRT += nS;
            }
            else
            {
                if (!tp->IsStreamed())
                {
                }
                if (tp->GetName()[0] != '$' && tp->GetNumMips() <= 1)
                {
                    nSOneMip += nS;
                }
                if (tp->GetFlags() & FT_TEX_NORMAL_MAP)
                {
                    nSNM += nS;
                }
                else
                {
                    nSAll += nS;
                }
            }
        }
    }
    nSRT -= (nSizeD + nSizeD2 + nSizeSH);
    Draw2dLabel(nX, nY, fFSize, &col.r, false, "Render targets: %.1f Mb", BYTES_TO_MB(nSRT));
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 1, nX + fOffs + (float)nSRT / fMaxTextureMemory * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst;

    Draw2dLabel(nX, nY, fFSize, &col.r, false, "Textures: %.1f Mb", BYTES_TO_MB(nSAll));
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 1, nX + fOffs + (float)nSAll / fMaxTextureMemory * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst;

    size_t nSizeMeshes = 0;
    {
        AUTO_LOCK(CRenderMesh::m_sLinkLock);
        for (util::list<CRenderMesh>* iter = CRenderMesh::m_MeshList.next; iter != &CRenderMesh::m_MeshList; iter = iter->next)
        {
            nSizeMeshes += iter->item<&CRenderMesh::m_Chain>()->Size(CRenderMesh::SIZE_VB | CRenderMesh::SIZE_IB);
        }
    }
    Draw2dLabel(nX, nY, fFSize, &col.r, false, "Meshes: %.1f Mb", BYTES_TO_MB(nSizeMeshes));
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 1, nX + fOffs + (float)nSizeMeshes / fMaxTextureMemory * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst;

    size_t nSizeDynM = 0;
    for (i = 0; i < SHAPE_MAX; i++)
    {
        nSizeDynM += _VertBufferSize(m_pUnitFrustumVB[i]);
        nSizeDynM += _IndexBufferSize(m_pUnitFrustumIB[i]);
    }

#if defined(ENABLE_RENDER_AUX_GEOM)
    if (m_pRenderAuxGeomD3D)
    {
        m_pRenderAuxGeomD3D->GetDeviceDataSize();
    }
#endif

    Draw2dLabel(nX, nY, fFSize, &col.r, false, "Dyn. mesh: %.1f Mb", BYTES_TO_MB(nSizeDynM));
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 1, nX + fOffs + (float)nSizeDynM / fMaxTextureMemory * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst + 4;

    size_t nSize = nSizeDynM + nSRT + nSizeCRT + nSizeSH + nSizeD + nSizeD2;
    ColorF colO = Col_Blue;
    Draw2dLabel(nX, nY, fFSize, &colO.r, false, "Overall (Pure): %.1f Mb", BYTES_TO_MB(nSize));
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 1, nX + fOffs + (float)nSize / fMaxTextureMemory * fMaxBar, nY + 12, Col_White, 1.0f);
    nY += nYst;
#endif
}

void CD3D9Renderer::DebugPerfBars([[maybe_unused]] int nX, [[maybe_unused]] int nY)
{
#if !defined(EXCLUDE_RARELY_USED_R_STATS) && defined(ENABLE_PROFILING_CODE)
    int nYst = 15;
    float fFSize = 1.4f;
    ColorF col = Col_Yellow;
    ColorF colP = Col_Cyan;

    // Draw performance bars
    TransformationMatrices backupSceneMatrices;
    Set2DMode(m_width, m_height, backupSceneMatrices);

    EF_SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
    EF_SetSrgbWrite(false);
    FX_SetState(GS_NODEPTHTEST);
    D3DSetCull(eCULL_None);
    FX_SetFPMode();

    static float fTimeDIP[EFSLIST_NUM];
    static float fTimeDIPAO;
    static float fTimeDIPZ;
    static float fTimeDIPRAIN;
    static float fTimeDIPLayers;
    static float fTimeDIPSprites;
    static float fWaitForGPU;
    static float fFrameTime;
    static float fRTTimeProcess;
    static float fRTTimeEndFrame;
    static float fRTTimeFlashRender;
    static float fRTTimeSceneRender;
    static float fRTTimeMiscRender;

    float fMaxBar = 200;
    float fOffs = 180.0f;

    Draw2dLabel(nX + 30, nY, 1.6f, &colP.r, false, "Instances: %d, GeomBatches: %d, MatBatches: %d, DrawCalls: %d, Text: %d, Stat: %d, PShad: %d, VShad: %d", m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumRendInstances, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumRendGeomBatches, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumRendMaterialBatches, GetCurrentNumberOfDrawCalls(), m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumTextChanges, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumStateChanges, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumPShadChanges, m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumVShadChanges);
    nY += 30;

    ColorF colT = Col_Gray;
    Draw2dLabel(nX + 50, nY, 1.6f, &colT.r, false, "Performance:");
    nY += 20;

    float fSmooth = 5.0f;
    fFrameTime = (iTimer->GetFrameTime() + fFrameTime * fSmooth) / (fSmooth + 1.0f);

    ColorF colF = Col_Orange;
    Draw2dLabel(nX, nY, fFSize, &colF.r, false, "Frame: %.3fms", fFrameTime * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fMaxBar, nY + 12, Col_Cyan, 1.0f);
    nY += nYst + 5;

    fTimeDIPZ = (m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPsZ + fTimeDIPZ * fSmooth) / (fSmooth + 1.0f);
    Draw2dLabel(nX, nY, fFSize, &col.r, false, "ZPass: %.3fms", fTimeDIPZ * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fTimeDIPZ / fFrameTime * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst;

    fTimeDIP[EFSLIST_DEFERRED_PREPROCESS] = (m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_DEFERRED_PREPROCESS] + fTimeDIP[EFSLIST_DEFERRED_PREPROCESS] * fSmooth) / (fSmooth + 1.0f);
    Draw2dLabel(nX, nY, fFSize, &col.r, false, "Deferred Prepr.: %.3fms", fTimeDIP[EFSLIST_DEFERRED_PREPROCESS] * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fTimeDIP[EFSLIST_DEFERRED_PREPROCESS] / fFrameTime * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst;

    fTimeDIP[EFSLIST_GENERAL] = (m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_GENERAL] + fTimeDIP[EFSLIST_GENERAL] * fSmooth) / (fSmooth + 1.0f);
    Draw2dLabel(nX, nY, fFSize, &col.r, false, "General: %.3fms", fTimeDIP[EFSLIST_GENERAL] * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fTimeDIP[EFSLIST_GENERAL] / fFrameTime * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst;

    fTimeDIP[EFSLIST_SHADOW_GEN] = (m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_SHADOW_GEN] + fTimeDIP[EFSLIST_SHADOW_GEN] * fSmooth) / (fSmooth + 1.0f);
    Draw2dLabel(nX, nY, fFSize, &col.r, false, "ShadowGen: %.3fms", fTimeDIP[EFSLIST_SHADOW_GEN] * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fTimeDIP[EFSLIST_SHADOW_GEN] / fFrameTime * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst;

    fTimeDIP[EFSLIST_DECAL] = (m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_DECAL] + fTimeDIP[EFSLIST_DECAL] * fSmooth) / (fSmooth + 1.0f);
    Draw2dLabel(nX, nY, fFSize, &col.r, false, "Decals: %.3fms", fTimeDIP[EFSLIST_DECAL] * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fTimeDIP[EFSLIST_DECAL] / fFrameTime * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst;

    fTimeDIPRAIN = (m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPsRAIN + fTimeDIPRAIN * fSmooth) / (fSmooth + 1.0f);
    Draw2dLabel(nX, nY, fFSize, &col.r, false, "Rain: %.3fms", fTimeDIPRAIN * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fTimeDIPRAIN / fFrameTime * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst;

    fTimeDIPLayers = (m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPsDeferredLayers + fTimeDIPLayers * fSmooth) / (fSmooth + 1.0f);
    Draw2dLabel(nX, nY, fFSize, &col.r, false, "Deferred Layers: %.3fms", fTimeDIPLayers * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fTimeDIPLayers / fFrameTime * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst;

    fTimeDIP[EFSLIST_WATER_VOLUMES] = (m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_WATER_VOLUMES] + fTimeDIP[EFSLIST_WATER_VOLUMES] * fSmooth) / (fSmooth + 1.0f);
    Draw2dLabel(nX, nY, fFSize, &col.r, false, "Water volumes: %.3fms", fTimeDIP[EFSLIST_WATER_VOLUMES] * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fTimeDIP[EFSLIST_WATER_VOLUMES] / fFrameTime * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst;

    fTimeDIP[EFSLIST_REFRACTIVE_SURFACE] = (m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_REFRACTIVE_SURFACE] + fTimeDIP[EFSLIST_REFRACTIVE_SURFACE] * fSmooth) / (fSmooth + 1.0f);
    Draw2dLabel(nX, nY, fFSize, &col.r, false, "Refractive Surfaces: %.3fms", fTimeDIP[EFSLIST_REFRACTIVE_SURFACE] * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fTimeDIP[EFSLIST_REFRACTIVE_SURFACE] / fFrameTime * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst;

    fTimeDIP[EFSLIST_TRANSP] = (m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_TRANSP] + fTimeDIP[EFSLIST_TRANSP] * fSmooth) / (fSmooth + 1.0f);
    Draw2dLabel(nX, nY, fFSize, &col.r, false, "Transparent: %.3fms", fTimeDIP[EFSLIST_TRANSP] * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fTimeDIP[EFSLIST_TRANSP] / fFrameTime * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst;

    fTimeDIPAO = (m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPsAO + fTimeDIPAO * fSmooth) / (fSmooth + 1.0f);
    Draw2dLabel(nX, nY, fFSize, &col.r, false, "AO: %.3fms", fTimeDIPAO * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fTimeDIPAO / fFrameTime * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst;

    fTimeDIP[EFSLIST_POSTPROCESS] = (m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPs[EFSLIST_POSTPROCESS] + fTimeDIP[EFSLIST_POSTPROCESS] * fSmooth) / (fSmooth + 1.0f);
    Draw2dLabel(nX, nY, fFSize, &col.r, false, "Postprocessing: %.3fms", fTimeDIP[EFSLIST_POSTPROCESS] * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fTimeDIP[EFSLIST_POSTPROCESS] / fFrameTime * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst;

    fTimeDIPSprites = (m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTimeDIPsSprites + fTimeDIPSprites * fSmooth) / (fSmooth + 1.0f);
    Draw2dLabel(nX, nY, fFSize, &col.r, false, "Sprites: %.3fms", fTimeDIPSprites * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fTimeDIPSprites / fFrameTime * fMaxBar, nY + 12, Col_Green, 1.0f);
    nY += nYst;

    float fTimeDIPSum = fTimeDIPZ + fTimeDIP[EFSLIST_DEFERRED_PREPROCESS] + fTimeDIP[EFSLIST_GENERAL] + fTimeDIP[EFSLIST_SHADOW_GEN] + fTimeDIP[EFSLIST_DECAL] + fTimeDIPAO + fTimeDIPRAIN + fTimeDIPLayers + fTimeDIP[EFSLIST_WATER_VOLUMES] + fTimeDIP[EFSLIST_TRANSP] + fTimeDIP[EFSLIST_POSTPROCESS] + fTimeDIPSprites;
    Draw2dLabel(nX, nY, fFSize, &colF.r, false, "Sum all passes: %.3fms", fTimeDIPSum * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fTimeDIPSum / fFrameTime * fMaxBar, nY + 12, Col_Yellow, 1.0f);
    nY += nYst + 5;


    fRTTimeProcess = (m_fTimeProcessedRT[m_RP.m_nFillThreadID] + fRTTimeProcess * fSmooth) / (fSmooth + 1.0f);
    Draw2dLabel(nX, nY, fFSize, &colT.r, false, "RT process time: %.3fms", fRTTimeProcess * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fRTTimeProcess / fFrameTime * fMaxBar, nY + 12, Col_Gray, 1.0f);
    nY += nYst;
    nX += 5;

    fRTTimeEndFrame = (m_fRTTimeEndFrame + fRTTimeEndFrame * fSmooth) / (fSmooth + 1.0f);
    Draw2dLabel(nX, nY, fFSize, &colT.r, false, "RT end frame: %.3fms", fRTTimeEndFrame * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fRTTimeEndFrame / fFrameTime * fMaxBar, nY + 12, Col_Gray, 1.0f);
    nY += nYst;

    fRTTimeMiscRender = (m_fRTTimeMiscRender + fRTTimeMiscRender * fSmooth) / (fSmooth + 1.0f);
    Draw2dLabel(nX, nY, fFSize, &colT.r, false, "RT misc render: %.3fms", fRTTimeMiscRender * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fRTTimeMiscRender / fFrameTime * fMaxBar, nY + 12, Col_Gray, 1.0f);
    nY += nYst;

    fRTTimeSceneRender = (m_fRTTimeSceneRender + fRTTimeSceneRender * fSmooth) / (fSmooth + 1.0f);
    Draw2dLabel(nX, nY, fFSize, &colT.r, false, "RT scene render: %.3fms", fRTTimeSceneRender * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fRTTimeSceneRender / fFrameTime * fMaxBar, nY + 12, Col_Gray, 1.0f);
    nY += nYst;

    float fRTTimeOverall = fRTTimeSceneRender + fRTTimeEndFrame + fRTTimeFlashRender + fRTTimeMiscRender;
    Draw2dLabel(nX, nY, fFSize, &colT.r, false, "RT CPU overall: %.3fms", fRTTimeOverall * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fRTTimeOverall / fFrameTime * fMaxBar, nY + 12, Col_LightGray, 1.0f);
    nY += nYst + 5;
    nX -= 5;


    fWaitForGPU = (m_fTimeWaitForGPU[m_RP.m_nFillThreadID] + fWaitForGPU * fSmooth) / (fSmooth + 1.0f);
    Draw2dLabel(nX, nY, fFSize, &colF.r, false, "Wait for GPU: %.3fms", fWaitForGPU * 1000.0f);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);
    DrawQuad(nX + fOffs, nY + 4, nX + fOffs + fWaitForGPU / fFrameTime * fMaxBar, nY + 12, Col_Blue, 1.0f);
    nY += nYst;

    Unset2DMode(backupSceneMatrices);
#endif
}

_inline bool CompareTexturesSize(CTexture* a, CTexture* b)
{
    return (a->GetDeviceDataSize() > b->GetDeviceDataSize());
}

void CD3D9Renderer::VidMemLog()
{
#if !defined(_RELEASE) && !defined(CONSOLE_CONST_CVAR_MODE)
    if (!CV_r_logVidMem)
    {
        return;
    }

    SResourceContainer* pRL = 0;

    pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());

    DynArray<CTexture*> pRenderTargets;
    DynArray<CTexture*> pDynTextures;
    DynArray<CTexture*> pTextures;

    size_t nSizeTotalRT = 0;
    size_t nSizeTotalDynTex = 0;
    size_t nSizeTotalTex = 0;

    for (uint32 i = 0; i < pRL->m_RList.size(); ++i)
    {
        CTexture* pTexResource = (CTexture*)pRL->m_RList[i];
        if (!pTexResource)
        {
            continue;
        }

        if (!pTexResource->GetDeviceDataSize())
        {
            continue;
        }

        if (pTexResource->GetFlags() & FT_USAGE_RENDERTARGET)
        {
            pRenderTargets.push_back(pTexResource);
            nSizeTotalRT += pTexResource->GetDeviceDataSize();
        }
        else
        if (pTexResource->GetFlags() & FT_USAGE_DYNAMIC)
        {
            pDynTextures.push_back(pTexResource);
            nSizeTotalDynTex += pTexResource->GetDeviceDataSize();
        }
        else
        {
            pTextures.push_back(pTexResource);
            nSizeTotalTex += pTexResource->GetDeviceDataSize();
        }
    }

    std::sort(pRenderTargets.begin(), pRenderTargets.end(), CompareTexturesSize);
    std::sort(pDynTextures.begin(), pDynTextures.end(), CompareTexturesSize);
    std::sort(pTextures.begin(), pTextures.end(), CompareTexturesSize);

    AZ::IO::HandleType fileHandle = fxopen("VidMemLogTest.txt", "w");
    if (fileHandle != AZ::IO::InvalidHandle)
    {
        char time[128];
        char date[128];

        azstrtime(time);
        azstrdate(date);

        AZ::IO::Print(fileHandle, "\n==========================================\n");
        AZ::IO::Print(fileHandle, "VidMem Log file opened: %s (%s)\n", date, time);
        AZ::IO::Print(fileHandle, "==========================================\n");

        AZ::IO::Print(fileHandle, "\nTotal Vid mem used for textures: %.1f MB", BYTES_TO_MB(nSizeTotalRT + nSizeTotalDynTex + nSizeTotalTex));
        AZ::IO::Print(fileHandle, "\nRender targets (%d): %.1f kb, Dynamic textures (%d): %.1f kb, Textures (%d): %.1f kb", pRenderTargets.size(), BYTES_TO_KB(nSizeTotalRT), pDynTextures.size(), BYTES_TO_KB(nSizeTotalDynTex), pTextures.size(), BYTES_TO_KB(nSizeTotalTex));

        AZ::IO::Print(fileHandle, "\n\n*** Start render targets list *** \n");
        int i = 0;
        for (i = 0; i < pRenderTargets.size(); ++i)
        {
            AZ::IO::Print(fileHandle, "\nName: %s, Format: %s, Width: %d, Height: %d, Size: %.1f kb", pRenderTargets[i]->GetName(), pRenderTargets[i]->GetFormatName(), pRenderTargets[i]->GetWidth(),
                pRenderTargets[i]->GetHeight(), BYTES_TO_KB(pRenderTargets[i]->GetDeviceDataSize()));
        }

        AZ::IO::Print(fileHandle, "\n\n*** Start dynamic textures list *** \n");
        for (i = 0; i < pDynTextures.size(); ++i)
        {
            AZ::IO::Print(fileHandle, "\nName: %s, Format: %s, Width: %d, Height: %d, Size: %.1f kb", pDynTextures[i]->GetName(), pDynTextures[i]->GetFormatName(), pDynTextures[i]->GetWidth(),
                pDynTextures[i]->GetHeight(), BYTES_TO_KB(pDynTextures[i]->GetDeviceDataSize()));
        }

        AZ::IO::Print(fileHandle, "\n\n*** Start textures list *** \n");
        for (i = 0; i < pTextures.size(); ++i)
        {
            AZ::IO::Print(fileHandle, "\nName: %s, Format: %s, Width: %d, Height: %d, Size: %.1f kb", pTextures[i]->GetName(), pTextures[i]->GetFormatName(), pTextures[i]->GetWidth(),
                pTextures[i]->GetHeight(), BYTES_TO_KB(pTextures[i]->GetDeviceDataSize()));
        }

        AZ::IO::Print(fileHandle, "\n\n==========================================\n");
        AZ::IO::Print(fileHandle, "VidMem Log file closed\n");
        AZ::IO::Print(fileHandle, "==========================================\n\n");

        gEnv->pFileIO->Close(fileHandle);
        fileHandle = AZ::IO::InvalidHandle;
    }

    CV_r_logVidMem = 0;
#endif
}


void CD3D9Renderer::DebugPrintShader([[maybe_unused]] CHWShader_D3D* pSH, [[maybe_unused]] void* pI, [[maybe_unused]] int nX, [[maybe_unused]] int nY, [[maybe_unused]] ColorF colSH)
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_CPP_SECTION_9
    #include AZ_RESTRICTED_FILE(DriverD3D_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    if (!pSH)
    {
        return;
    }
    CHWShader_D3D::SHWSInstance* pInst = (CHWShader_D3D::SHWSInstance*)pI;
    if (!pInst)
    {
        return;
    }

    char str[512];
    pSH->m_pCurInst = pInst;
    azstrcpy(str, AZ_ARRAY_SIZE(str), pSH->m_EntryFunc.c_str());
    uint32 nSize = strlen(str);
    pSH->mfGenName(pInst, &str[nSize], 512 - nSize, 1);

    ColorF col = Col_Green;
    Draw2dLabel(nX, nY, 1.6f, &col.r, false, "Shader: %s (%d instructions)", str, pInst->m_nInstructions);
    nX += 10;
    nY += 25;

    SD3DShader* pHWS = pInst->m_Handle.m_pShader;
    if (!pHWS || !pHWS->m_pHandle)
    {
        return;
    }
    if (!pInst->m_pShaderData)
    {
        return;
    }
    byte* pData = NULL;
    ID3D10Blob* pAsm = NULL;
    D3DDisassemble((UINT*)pInst->m_pShaderData, pInst->m_nDataSize, 0, NULL, &pAsm);
    if (!pAsm)
    {
        return;
    }
    char* szAsm = (char*)pAsm->GetBufferPointer();
    int nMaxY = m_height;
    int nM = 0;
    while (szAsm[0])
    {
        fxFillCR(&szAsm, str);
        Draw2dLabel(nX, nY, 1.2f, &colSH.r, false, "%s", str);
        nY += 11;
        if (nY + 12 > nMaxY)
        {
            nX += 280;
            nY = 120;
            nM++;
        }
        if (nM == 2)
        {
            break;
        }
    }

    SAFE_RELEASE(pAsm);
    SAFE_DELETE_ARRAY(pData);
#endif
}

void CD3D9Renderer::DebugDrawStats8()
{
#if !defined(_RELEASE) && defined(ENABLE_PROFILING_CODE)
    ColorF col = Col_White;
    Draw2dLabel(30, 50, 1.2f, &col.r, false, "%d total instanced DIPs in %d batches", m_RP.m_PS[m_RP.m_nProcessThreadID].m_nInsts, m_RP.m_PS[m_RP.m_nProcessThreadID].m_nInstCalls);
#endif
}

void CD3D9Renderer::DebugDrawStats2()
{
#if !defined(EXCLUDE_RARELY_USED_R_STATS)
    const int nYstep = 10;
    int nY = 30; // initial Y pos
    int nX = 20; // initial X pos
    static int snTech = 0;

    if (!g_SelectedTechs.size())
    {
        return;
    }

    if (CryGetAsyncKeyState('0') & 0x1)
    {
        snTech = 0;
    }
    if (CryGetAsyncKeyState('1') & 0x1)
    {
        snTech = 1;
    }
    if (CryGetAsyncKeyState('2') & 0x1)
    {
        snTech = 2;
    }
    if (CryGetAsyncKeyState('3') & 0x1)
    {
        snTech = 3;
    }
    if (CryGetAsyncKeyState('4') & 0x1)
    {
        snTech = 4;
    }
    if (CryGetAsyncKeyState('5') & 0x1)
    {
        snTech = 5;
    }
    if (CryGetAsyncKeyState('6') & 0x1)
    {
        snTech = 6;
    }
    if (CryGetAsyncKeyState('7') & 0x1)
    {
        snTech = 7;
    }
    if (CryGetAsyncKeyState('8') & 0x1)
    {
        snTech = 8;
    }
    if (CryGetAsyncKeyState('9') & 0x1)
    {
        snTech = 9;
    }

    TArray<SShaderTechniqueStat*> Techs;
    uint32 i;
    int j;
    for (i = 0; i < g_SelectedTechs.size(); i++)
    {
        SShaderTechniqueStat* pTech = &g_SelectedTechs[i];
        for (j = 0; j < (int)Techs.Num(); j++)
        {
            if (Techs[j]->pVSInst == pTech->pVSInst && Techs[j]->pPSInst == pTech->pPSInst)
            {
                break;
            }
        }
        if (j == Techs.Num())
        {
            Techs.AddElem(pTech);
        }
    }

    if (snTech >= (int)Techs.Num())
    {
        snTech = Techs.Num() - 1;
    }

    if (!(snTech >= 0 && snTech < (int)Techs.Num()))
    {
        return;
    }

    SShaderTechniqueStat* pTech = Techs[snTech];

    ColorF col = Col_Yellow;
    Draw2dLabel(nX, nY, 2.0f, &col.r, false, "FX Shader: %s, Technique: %s (%d out of %d), Pass: %d", pTech->pShader->GetName(), pTech->pTech->m_NameStr.c_str(), snTech, Techs.Num(), 0);
    nY += 25;

    CHWShader_D3D* pPS = pTech->pPS;
    DebugPrintShader(pTech->pVS, pTech->pVSInst, nX - 10, nY, Col_White);
    DebugPrintShader(pPS, pTech->pPSInst, nX + 450, nY, Col_Cyan);
#endif
}

void CD3D9Renderer::DebugDrawStats()
{
#ifndef _RELEASE
    if (CV_r_stats)
    {
        CRenderer* crend = gRenDev;

        CCryNameTSCRC Name;
        switch (CV_r_stats)
        {
        case 1:
            DebugDrawStats1();
            break;
        case 2:
            DebugDrawStats2();
            break;
        case 3:
            DebugPerfBars(40, 50);
            DebugVidResourcesBars(450, 80);
            break;
        case 4:
            DebugPerfBars(40, 50);
            break;
        case 8:
            DebugDrawStats8();
            break;
        case 13:
            EF_PrintRTStats("Cleared Render Targets:");
            break;
        case 5:
        {
            const int nYstep = 30;
            int nYpos = 270;     // initial Y pos
            crend->WriteXY(10, nYpos += nYstep, 2, 2, 1, 1, 1, 1, "CREOcclusionQuery stats:%d",
                CREOcclusionQuery::m_nQueriesPerFrameCounter);
            crend->WriteXY(10, nYpos += nYstep, 2, 2, 1, 1, 1, 1, "CREOcclusionQuery::m_nQueriesPerFrameCounter=%d",
                CREOcclusionQuery::m_nQueriesPerFrameCounter);
            crend->WriteXY(10, nYpos += nYstep, 2, 2, 1, 1, 1, 1, "CREOcclusionQuery::m_nReadResultNowCounter=%d",
                CREOcclusionQuery::m_nReadResultNowCounter);
            crend->WriteXY(10, nYpos += nYstep, 2, 2, 1, 1, 1, 1, "CREOcclusionQuery::m_nReadResultTryCounter=%d",
                CREOcclusionQuery::m_nReadResultTryCounter);
        }
        break;

        case 6:
        {
            ColorF clrDPBlue = ColorF(0, 1, 1, 1);
            ColorF clrDPRed = ColorF(1, 0, 0, 1);
            ColorF clrDPInterp = ColorF(1, 0, 0, 1);
            ;
            ColorF clrInfo = ColorF(1, 1, 0, 1);

            IRenderer::RNDrawcallsMapNodeItor pEnd = m_RP.m_pRNDrawCallsInfoPerNode[m_RP.m_nProcessThreadID].end();
            IRenderer::RNDrawcallsMapNodeItor pItor = m_RP.m_pRNDrawCallsInfoPerNode[m_RP.m_nProcessThreadID].begin();
            for (; pItor != pEnd; ++pItor)
            {
                SDrawCallCountInfo& pInfo = pItor->second;

                uint32 nDrawcalls = pInfo.nShadows + pInfo.nZpass + pInfo.nGeneral + pInfo.nTransparent + pInfo.nMisc;

                // Display drawcall count per render object
                const float* pfColor;
                int nMinDrawCalls = CV_r_statsMinDrawcalls;
                if (nDrawcalls < nMinDrawCalls)
                {
                    continue;
                }
                else if (nDrawcalls <= 4)
                {
                    pfColor = &clrDPBlue.r;
                }
                else if (nDrawcalls > 20)
                {
                    pfColor = &clrDPRed.r;
                }
                else
                {
                    // interpolation from orange to red
                    clrDPInterp.g = 0.5f - 0.5f * (nDrawcalls - 4) / (20 - 4);
                    pfColor = &clrDPInterp.r;
                }

                DrawLabelEx(pInfo.pPos, 1.3f, pfColor, true, true, "DP: %d (%d/%d/%d/%d/%d)",
                    nDrawcalls, pInfo.nZpass, pInfo.nGeneral, pInfo.nTransparent, pInfo.nShadows, pInfo.nMisc);
            }

            Draw2dLabel(40.f, 40.f, 1.3f, &clrInfo.r, false, "Instance drawcall count (zpass/general/transparent/shadows/misc)");
        }
        break;
        }
    }

    if (m_pDebugRenderNode)
    {
#ifndef _RELEASE
        static ICVar* debugDraw = gEnv->pConsole->GetCVar("e_DebugDraw");
        if (debugDraw && debugDraw->GetIVal() == 16)
        {
            float yellow[4] = { 1.f, 1.f, 0.f, 1.f };

            IRenderer::RNDrawcallsMapNodeItor pEnd = m_RP.m_pRNDrawCallsInfoPerNode[m_RP.m_nProcessThreadID].end();
            IRenderer::RNDrawcallsMapNodeItor pItor = m_RP.m_pRNDrawCallsInfoPerNode[m_RP.m_nProcessThreadID].begin();
            for (; pItor != pEnd; ++pItor)
            {
                IRenderNode* pRenderNode = pItor->first;

                //display info for render node under debug gun
                if (pRenderNode == m_pDebugRenderNode)
                {
                    SDrawCallCountInfo& pInfo = pItor->second;

                    uint32 nDrawcalls = pInfo.nShadows + pInfo.nZpass + pInfo.nGeneral + pInfo.nTransparent + pInfo.nMisc;

                    Draw2dLabel(970.f, 65.f, 1.5f, yellow, false, "Draw calls: %d \n  zpass: %d\n  general: %d\n  transparent: %d\n  shadows: %d\n  misc: %d\n",
                        nDrawcalls, pInfo.nZpass, pInfo.nGeneral, pInfo.nTransparent, pInfo.nShadows, pInfo.nMisc);
                }
            }
        }
#endif
    }
#endif
}

void CD3D9Renderer::RenderDebug(bool bRenderStats)
{
    m_pRT->RC_RenderDebug(bRenderStats);
}

void CD3D9Renderer::RT_RenderDebug([[maybe_unused]] bool bRenderStats)
{
    if (gEnv->IsEditor() && !m_CurrContext->m_bMainViewport)
    {
        return;
    }

    if (m_bDeviceLost)
    {
        return;
    }
#if !defined(_RELEASE)

#if REFRACTION_PARTIAL_RESOLVE_DEBUG_VIEWS
    if (CV_r_RefractionPartialResolvesDebug)
    {
        SPipeStat* pRP = &m_RP.m_PS[m_RP.m_nFillThreadID];
        const float xPos = 0.0f;
        float yPos = 90.0f;
        const float textYSpacing = 18.0f;
        const float size = 1.5f;
        const ColorF titleColor = Col_SpringGreen;
        const ColorF textColor = Col_Yellow;
        const bool bCentre = false;

        float fInvScreenArea = 1.0f / ((float)GetWidth() * (float)GetHeight());

        Draw2dLabel(xPos, yPos, size, &titleColor.r, bCentre, "Partial Resolves");
        yPos += textYSpacing;
        Draw2dLabel(xPos, yPos, size, &textColor.r, bCentre, "Count: %d", pRP->m_refractionPartialResolveCount);
        yPos += textYSpacing;
        Draw2dLabel(xPos, yPos, size, &textColor.r, bCentre, "Pixels: %d", pRP->m_refractionPartialResolvePixelCount);
        yPos += textYSpacing;
        Draw2dLabel(xPos, yPos, size, &textColor.r, bCentre, "Percentage of Screen area: %d", (int)(pRP->m_refractionPartialResolvePixelCount * fInvScreenArea * 100.0f));
        yPos += textYSpacing;
        Draw2dLabel(xPos, yPos, size, &textColor.r, bCentre, "Estimated cost: %.2fms", pRP->m_fRefractionPartialResolveEstimatedCost);
    }
#endif

#ifndef EXCLUDE_DOCUMENTATION_PURPOSE
    if (CV_r_DebugFontRendering)
    {
        const float fPixelPerfectScale = 16.0f / UIDRAW_TEXTSIZEFACTOR * 2.0f;      // we have to compensate  vSize.x/16.0f; and pFont->SetCharWidthScale(0.5f);
        const int line = 10;

        float y = 0;
        SDrawTextInfo ti;
        ti.flags = eDrawText_2D | eDrawText_FixedSize | eDrawText_Monospace;

        ti.color[2] = 0.0f;
        DrawTextQueued(Vec3(0, y += line, 0), ti, "Colors (black,white,blue,..): { $00$11$22$33$44$55$66$77$88$99$$$o } ()_!+*/# ?");
        ti.color[2] = 1.0f;
        DrawTextQueued(Vec3(0, y += line, 0), ti, "Colors (black,white,blue,..): { $00$11$22$33$44$55$66$77$88$99$$$o } ()_!+*/# ?");

        for (int iPass = 0; iPass < 3; ++iPass)         // left, center, right
        {
            ti.xscale = ti.yscale = fPixelPerfectScale * 0.5f;
            float x = 0;

            y = 3 * line;

            int passflags = eDrawText_2D;

            if (iPass == 1)
            {
                passflags |= eDrawText_Center;
                x = (float)GetWidth() * 0.5f;
            }
            else if (iPass == 2)
            {
                x = (float)GetWidth();
                passflags |= eDrawText_Right;
            }

            ti.color[3] = 0.5f;
            ti.flags = passflags | eDrawText_FixedSize | eDrawText_Monospace;
            DrawTextQueued(Vec3(x, y += line, 0), ti, "0");
            DrawTextQueued(Vec3(x, y += line, 0), ti, "AbcW !.....!");
            DrawTextQueued(Vec3(x, y += line, 0), ti, "AbcW !MMMMM!");

            ti.color[0] = 0.0f;
            ti.color[3] = 1.0f;
            ti.flags = passflags | eDrawText_FixedSize;
            DrawTextQueued(Vec3(x, y += line, 0), ti, "1");
            DrawTextQueued(Vec3(x, y += line, 0), ti, "AbcW !.....!");
            DrawTextQueued(Vec3(x, y += line, 0), ti, "AbcW !MMMMM!");
            /*
                        ti.flags = passflags | eDrawText_Monospace;
                        DrawTextQueued(Vec3(x,y+=line,0),ti,"2");
                        DrawTextQueued(Vec3(x,y+=line,0),ti,"AbcW !.....!");
                        DrawTextQueued(Vec3(x,y+=line,0),ti,"AbcW !MMMMM!");

                        ti.flags = passflags;
                        DrawTextQueued(Vec3(x,y+=line,0),ti,"3");
                        DrawTextQueued(Vec3(x,y+=line,0),ti,"AbcW !.....!");
                        DrawTextQueued(Vec3(x,y+=line,0),ti,"AbcW !MMMMM!");
                        */
            ti.color[1] = 0.0f;
            ti.flags = passflags | eDrawText_800x600 | eDrawText_FixedSize | eDrawText_Monospace;
            DrawTextQueued(Vec3(x, y += line, 0), ti, "4");
            DrawTextQueued(Vec3(x, y += line, 0), ti, "AbcW !.....!");
            DrawTextQueued(Vec3(x, y += line, 0), ti, "AbcW !MMMMM!");

            ti.color[0] = 1.0f;
            ti.color[1] = 1.0f;
            ti.flags = passflags | eDrawText_800x600 | eDrawText_FixedSize;
            DrawTextQueued(Vec3(x, y += line, 0), ti, "5");
            DrawTextQueued(Vec3(x, y += line, 0), ti, "AbcW !.....!");
            DrawTextQueued(Vec3(x, y += line, 0), ti, "AbcW !MMMMM!");

            /*
                        ti.flags = passflags | eDrawText_800x600 | eDrawText_Monospace;
                        DrawTextQueued(Vec3(x,y+=line,0),ti,"6");
                        DrawTextQueued(Vec3(x,y+=line,0),ti,"Halfsize");
                        DrawTextQueued(Vec3(x,y+=line,0),ti,"AbcW !MMMMM!");

                        ti.flags = passflags | eDrawText_800x600;
                        DrawTextQueued(Vec3(x,y+=line,0),ti,"7");
                        DrawTextQueued(Vec3(x,y+=line,0),ti,"AbcW !.....!");
                        DrawTextQueued(Vec3(x,y+=line,0),ti,"AbcW !MMMMM!");
                        */
            // Pixel Perfect (1:1 mapping of the texels to the pixels on the screeen)

            ti.flags = passflags | eDrawText_FixedSize | eDrawText_Monospace;
            ti.xscale = ti.yscale = fPixelPerfectScale;
            DrawTextQueued(Vec3(x, y += line * 2, 0), ti, "8");
            DrawTextQueued(Vec3(x, y += line * 2, 0), ti, "AbcW !.....!");
            DrawTextQueued(Vec3(x, y += line * 2, 0), ti, "AbcW !MMMMM!");

            ti.flags = passflags | eDrawText_FixedSize;
            ti.xscale = ti.yscale = fPixelPerfectScale;
            DrawTextQueued(Vec3(x, y += line * 2, 0), ti, "9");
            DrawTextQueued(Vec3(x, y += line * 2, 0), ti, "AbcW !.....!");
            DrawTextQueued(Vec3(x, y += line * 2, 0), ti, "AbcW !MMMMM!");
        }
    }
#endif // EXCLUDE_DOCUMENTATION_PURPOSE


#ifdef DO_RENDERSTATS
    static std::vector<SStreamEngineStatistics::SAsset> vecStreamingProblematicAssets;

    if (CV_r_showtimegraph)
    {
        static byte* fg;
        static float fPrevTime = iTimer->GetCurrTime();
        static int sPrevWidth = 0;
        static int sPrevHeight = 0;
        static int nC;

        float fCurTime = iTimer->GetCurrTime();
        float frametime = fCurTime - fPrevTime;
        fPrevTime = fCurTime;
        int wdt = m_width;
        int hgt = m_height;

        if (sPrevHeight != hgt || sPrevWidth != wdt)
        {
            if (fg)
            {
                delete[] fg;
                fg = NULL;
            }
            sPrevWidth = wdt;
            sPrevHeight = hgt;
        }

        if (!fg)
        {
            fg = new byte[wdt];
            memset(fg, -1, wdt);
            nC = 0;
        }

        int type = CV_r_showtimegraph;
        float f;
        float fScale;
        if (type > 1)
        {
            type = 1;
            fScale = (float)CV_r_showtimegraph / 1000.0f;
        }
        else
        {
            fScale = 0.1f;
        }
        f = frametime / fScale;
        f = 255.0f - CLAMP(f * 255.0f, 0, 255.0f);
        if (fg)
        {
            fg[nC] = (byte)f;
            ColorF c = Col_Green;
            Graph(fg, 0, hgt - 280, wdt, 256, nC, type, "Frame Time", c, fScale);
        }
        nC++;
        if (nC >= wdt)
        {
            nC = 0;
        }
    }
    else
    if (CV_profileStreaming)
    {
        static byte* fgUpl;
        static byte* fgStreamSync;
        static byte* fgTimeUpl;
        static byte* fgDistFact;
        static byte* fgTotalMem;
        static byte* fgCurMem;
        static byte* fgStreamSystem;

        static float fScaleUpl = 10;      // in Mb
        static float fScaleStreamSync = 10;      // in Mb
        static float fScaleTimeUpl = 75;      // in Ms
        static float fScaleDistFact = 4;      // Ratio
        static FLOAT fScaleTotalMem = 0;      // in Mb
        static float fScaleCurMem = 80;       // in Mb
        static float fScaleStreaming = 4;       // in Mb

        static ColorF ColUpl = Col_White;
        static ColorF ColStreamSync = Col_Cyan;
        static ColorF ColTimeUpl = Col_SeaGreen;
        static ColorF ColDistFact = Col_Orchid;
        static ColorF ColTotalMem = Col_Red;
        static ColorF ColCurMem = Col_Yellow;
        static ColorF ColCurStream = Col_BlueViolet;

        static int sMask = -1;

        fScaleTotalMem = (float)CRenderer::GetTexturesStreamPoolSize() - 1;

        static float fPrevTime = iTimer->GetCurrTime();
        static int sPrevWidth = 0;
        static int sPrevHeight = 0;
        static int nC;

        int wdt = m_width;
        int hgt = m_height;
        int type = 2;

        if (sPrevHeight != hgt || sPrevWidth != wdt)
        {
            SAFE_DELETE_ARRAY(fgUpl);
            SAFE_DELETE_ARRAY(fgStreamSync);
            SAFE_DELETE_ARRAY(fgTimeUpl);
            SAFE_DELETE_ARRAY(fgDistFact);
            SAFE_DELETE_ARRAY(fgTotalMem);
            SAFE_DELETE_ARRAY(fgCurMem);
            SAFE_DELETE_ARRAY(fgStreamSystem);
            sPrevWidth = wdt;
            sPrevHeight = hgt;
        }

        if (!fgUpl)
        {
            fgUpl = new byte[wdt];
            memset(fgUpl, -1, wdt);
            fgStreamSync = new byte[wdt];
            memset(fgStreamSync, -1, wdt);
            fgTimeUpl = new byte[wdt];
            memset(fgTimeUpl, -1, wdt);
            fgDistFact = new byte[wdt];
            memset(fgDistFact, -1, wdt);
            fgTotalMem = new byte[wdt];
            memset(fgTotalMem, -1, wdt);
            fgCurMem = new byte[wdt];
            memset(fgCurMem, -1, wdt);
            fgStreamSystem = new byte[wdt];
            memset(fgStreamSystem, -1, wdt);
        }

        TransformationMatrices backupSceneMatrices;
        Set2DMode(m_width, m_height, backupSceneMatrices);

        ColorF col = Col_White;
        int num = CTextureManager::Instance()->GetWhiteTexture()->GetID();
        DrawImage((float)nC, (float)(hgt - 280), 1, 256, num, 0, 0, 1, 1, col.r, col.g, col.b, col.a);

        Unset2DMode(backupSceneMatrices);

        // disable some statistics
        sMask &= ~(1 | 2 | 4 | 8 | 64);

        float f;
        if (sMask & 1)
        {
            f = (BYTES_TO_MB(CTexture::s_nTexturesDataBytesUploaded)) / fScaleUpl;
            f = 255.0f - CLAMP(f * 255.0f, 0, 255.0f);
            fgUpl[nC] = (byte)f;
            Graph(fgUpl, 0, hgt - 280, wdt, 256, nC, type, NULL, ColUpl, fScaleUpl);
            col = ColUpl;
            WriteXY(4, hgt - 280, 1, 1, col.r, col.g, col.b, 1, "UploadMB (%d-%d)", (int)(BYTES_TO_MB(CTexture::s_nTexturesDataBytesUploaded)), (int)fScaleUpl);
        }

        if (sMask & 2)
        {
            f = m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTexUploadTime / fScaleTimeUpl;
            f = 255.0f - CLAMP(f * 255.0f, 0, 255.0f);
            fgTimeUpl[nC] = (byte)f;
            Graph(fgTimeUpl, 0, hgt - 280, wdt, 256, nC, type, NULL, ColTimeUpl, fScaleTimeUpl);
            col = ColTimeUpl;
            WriteXY(4, hgt - 280 + 16, 1, 1, col.r, col.g, col.b, 1, "Upload Time (%.3fMs - %.3fMs)", m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTexUploadTime, fScaleTimeUpl);
        }

        if (sMask & 4)
        {
            f = BYTES_TO_MB(CTexture::s_nTexturesDataBytesLoaded) / fScaleStreamSync;
            f = 255.0f - CLAMP(f * 255.0f, 0, 255.0f);
            fgStreamSync[nC] = (byte)f;
            Graph(fgStreamSync, 0, hgt - 280, wdt, 256, nC, type, NULL, ColStreamSync, fScaleStreamSync);
            col = ColStreamSync;
            WriteXY(4, hgt - 280 + 16 * 2, 1, 1, col.r, col.g, col.b, 1, "StreamMB (%d-%d)", (int)BYTES_TO_MB(CTexture::s_nTexturesDataBytesLoaded), (int)fScaleStreamSync);
        }

        if (sMask & 32)
        {
            size_t pool = CTexture::s_nStatsStreamPoolInUseMem;
            f = BYTES_TO_MB(pool) / fScaleTotalMem;
            f = 255.0f - CLAMP(f * 255.0f, 0, 255.0f);
            fgTotalMem[nC] = (byte)f;
            Graph(fgTotalMem, 0, hgt - 280, wdt, 256, nC, type, NULL, ColTotalMem, fScaleTotalMem);
            col = ColTotalMem;
            WriteXY(4, hgt - 280 + 16 * 5, 1, 1, col.r, col.g, col.b, 1, "Streaming textures pool used (Mb) (%d of %d)", (int)BYTES_TO_MB(pool), (int)fScaleTotalMem);
        }
        if (sMask & 64)
        {
            f = (BYTES_TO_MB(m_RP.m_PS[m_RP.m_nProcessThreadID].m_ManagedTexturesSysMemSize + m_RP.m_PS[m_RP.m_nProcessThreadID].m_ManagedTexturesStreamSysSize + m_RP.m_PS[m_RP.m_nProcessThreadID].m_DynTexturesSize)) / fScaleCurMem;
            f = 255.0f - CLAMP(f * 255.0f, 0, 255.0f);
            fgCurMem[nC] = (byte)f;
            Graph(fgCurMem, 0, hgt - 280, wdt, 256, nC, type, NULL, ColCurMem, fScaleCurMem);
            col = ColCurMem;
            WriteXY(4, hgt - 280 + 16 * 6, 1, 1, col.r, col.g, col.b, 1, "Cur Scene Size: Dyn. + Stat. (Mb) (%d-%d)", (int)(BYTES_TO_MB(m_RP.m_PS[m_RP.m_nProcessThreadID].m_ManagedTexturesSysMemSize + m_RP.m_PS[m_RP.m_nProcessThreadID].m_ManagedTexturesStreamSysSize + m_RP.m_PS[m_RP.m_nProcessThreadID].m_DynTexturesSize)), (int)fScaleCurMem);
        }
        if (sMask & 128)            // streaming stat
        {
            int nLineStep = 12;
            static float thp = 0.f;

            SStreamEngineStatistics& stats = gEnv->pSystem->GetStreamEngine()->GetStreamingStatistics();

            float dt = 1.0f;    //max(stats.m_fDeltaTime / 1000, .00001f);

            float newThp = (float)stats.nTotalCurrentReadBandwidth / 1024 / dt;
            thp = (thp + min(1.f, dt / 5) * (newThp - thp));        // lerp

            f = thp / (fScaleStreaming * 1024);
            if (f > 1.f && !stats.vecHeavyAssets.empty())
            {
                for (int i = stats.vecHeavyAssets.size() - 1; i >= 0; --i)
                {
                    SStreamEngineStatistics::SAsset asset = stats.vecHeavyAssets[i];

                    bool bPart = false;
                    // remove texture mips extensions
                    if (asset.m_sName.size() > 2 && asset.m_sName[asset.m_sName.size() - 2] == '.' &&
                        asset.m_sName[asset.m_sName.size() - 1] >= '0' && asset.m_sName[asset.m_sName.size() - 1] <= '9')
                    {
                        asset.m_sName = asset.m_sName.substr(0, asset.m_sName.size() - 2);
                        bPart = true;
                    }

                    // check for unique name
                    uint32 j = 0;
                    for (; j < vecStreamingProblematicAssets.size(); ++j)
                    {
                        if (vecStreamingProblematicAssets[j].m_sName.compareNoCase(asset.m_sName) == 0)
                        {
                            break;
                        }
                    }
                    if (j == vecStreamingProblematicAssets.size())
                    {
                        vecStreamingProblematicAssets.insert(vecStreamingProblematicAssets.begin(), asset);
                    }
                    else if (bPart)
                    {
                        vecStreamingProblematicAssets[j].m_nSize = max(vecStreamingProblematicAssets[j].m_nSize, asset.m_nSize);        // else adding size to existing asset
                    }
                }
                if (vecStreamingProblematicAssets.size() > 20)
                {
                    vecStreamingProblematicAssets.resize(20);
                }
                std::sort(vecStreamingProblematicAssets.begin(), vecStreamingProblematicAssets.end());      // sort by descending
            }
            f = 255.0f - CLAMP(f * 255.0f, 0, 255.0f);
            fgStreamSystem[nC] = (byte)f;
            Graph(fgStreamSystem, 0, hgt - 280, wdt, 256, nC, type, NULL, ColCurStream, fScaleStreaming);
            col = ColCurStream;
            WriteXY(4, hgt - 280 + 14 * 7, 1, 1, col.r, col.g, col.b, 1, "Streaming throughput (Kb/s) (%d of %d)", (int)thp, (int)fScaleStreaming * 1024);

            // output assets
            if (!vecStreamingProblematicAssets.empty())
            {
                int top = vecStreamingProblematicAssets.size() * nLineStep + 320;
                WriteXY(4, hgt - top - nLineStep, 1, 1, col.r, col.g, col.b, 1, "Problematic assets:");
                for (int i = vecStreamingProblematicAssets.size() - 1; i >= 0; --i)
                {
                    WriteXY(4, hgt - top + nLineStep * i, 1, 1, col.r, col.g, col.b, 1, "[%.1fKB] '%s'", BYTES_TO_KB(vecStreamingProblematicAssets[i].m_nSize), vecStreamingProblematicAssets[i].m_sName.c_str());
                }
            }
        }
        nC++;
        if (nC == wdt)
        {
            nC = 0;
        }
    }
    else
    {
        vecStreamingProblematicAssets.clear();
    }

    PostMeasureOverdraw();

    DrawTexelsPerMeterInfo();

    if (m_pColorGradingControllerD3D)
    {
        m_pColorGradingControllerD3D->DrawDebugInfo();
    }

    double time = 0;
    ticks(time);

    if (bRenderStats)
    {
        DebugDrawStats();
    }

    VidMemLog();

    if (CV_r_profileshaders)
    {
        EF_PrintProfileInfo();
    }

    { // print shadow maps on the screen
        static ICVar* pVar = iConsole->GetCVar("e_ShadowsDebug");
        if (pVar && pVar->GetIVal() >= 1 && pVar->GetIVal() <= 2)
        {
            DrawAllShadowsOnTheScreen();
        }
    }
#endif

    if (CV_r_DeferredShadingDebug == 1 || CV_r_DeferredShadingDebug >= 3)
    {
        // Draw a custom RT list for deferred rendering
        m_showRenderTargetInfo.Reset();
        m_showRenderTargetInfo.bShowList = false;

        SShowRenderTargetInfo::RT rt;
        rt.bFiltered = false;
        rt.bRGBKEncoded = false;
        rt.bAliased = false;
        rt.channelWeight = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        m_showRenderTargetInfo.col = (CV_r_DeferredShadingDebug == 1 ? 2 : 1);

        if (CV_r_DeferredShadingDebug == 1 || CV_r_DeferredShadingDebug == 3)
        {
            AZ_Assert(CTexture::s_ptexZTarget, "Z (depth) render target is NULL");
            rt.textureID = CTexture::s_ptexZTarget->GetID();
            rt.channelWeight = Vec4(10.0f, 10.0f, 10.0f, 1.0f);
            m_showRenderTargetInfo.rtList.push_back(rt);
        }
        if (CV_r_DeferredShadingDebug == 1 || CV_r_DeferredShadingDebug == 4)
        {
            rt.channelWeight = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            AZ_Assert(CTexture::s_ptexSceneNormalsMap, "Scene normals render target is NULL");
            rt.textureID = CTexture::s_ptexSceneNormalsMap->GetID();
            m_showRenderTargetInfo.rtList.push_back(rt);
        }
        // DiffuseAccuMap and SpecularAccMap are only calculated when CV_r_DeferredShadingTiled == 0 or 1.
        if (CV_r_DeferredShadingTiled < 2)
        {
            if (CV_r_DeferredShadingDebug == 1 || CV_r_DeferredShadingDebug == 5)
            {
                AZ_Assert(CTexture::s_ptexCurrentSceneDiffuseAccMap, "Current scene diffuse accumulator render target is NULL");
                rt.textureID = CTexture::s_ptexCurrentSceneDiffuseAccMap->GetID();
                m_showRenderTargetInfo.rtList.push_back(rt);
            }
            if (CV_r_DeferredShadingDebug == 1 || CV_r_DeferredShadingDebug == 6)
            {
                AZ_Assert(CTexture::s_ptexSceneSpecularAccMap, "Current scene specular accumulator render target is NULL");
                rt.textureID = CTexture::s_ptexSceneSpecularAccMap->GetID();
                m_showRenderTargetInfo.rtList.push_back(rt);
            }
        }
        DebugShowRenderTarget();
        m_showRenderTargetInfo.rtList.clear();
    }

    if (CV_r_DeferredShadingDebugGBuffer >= 1 && CV_r_DeferredShadingDebugGBuffer <= 9)
    {
        m_showRenderTargetInfo.rtList.clear();
        m_showRenderTargetInfo.bShowList = false;
        m_showRenderTargetInfo.bDisplayTransparent = true;
        m_showRenderTargetInfo.col = 1;

        SShowRenderTargetInfo::RT rt;
        rt.bFiltered = false;
        rt.bRGBKEncoded = false;
        rt.bAliased = false;
        rt.channelWeight = Vec4(1.0f, 1.0f, 1.0f, 1.0f);

        AZ_Assert(CTexture::s_ptexStereoR, "Right stereo render target is NULL");
        rt.textureID = CTexture::s_ptexStereoR->GetID();
        m_showRenderTargetInfo.rtList.push_back(rt);

        DebugShowRenderTarget();
        m_showRenderTargetInfo.rtList.clear();

        const char* nameList[9] = { "Normals", "Smoothness", "Reflectance", "Albedo", "Lighting model", "Translucency", "Sun self-shadowing", "Subsurface Scattering", "Specular Validation" };
        const char* descList[9] = {
            "", "", "", "", "gray: standard -- yellow: transmittance -- blue: pom self-shadowing", "", "", "",
            "blue: too low -- orange: too high and not yet metal -- pink: just valid for oxidized metal/rust"
        };
        WriteXY(10, 10, 1.0f, 1.0f, 0, 1, 0, 1, nameList[clamp_tpl(CV_r_DeferredShadingDebugGBuffer - 1, 0, 8)]);
        WriteXY(10, 30, 0.85f, 0.85f, 0, 1, 0, 1, descList[clamp_tpl(CV_r_DeferredShadingDebugGBuffer - 1, 0, 8)]);
    }

    if (m_showRenderTargetInfo.bShowList)
    {
        iLog->Log("RenderTargets:\n");
        for (size_t i = 0; i < m_showRenderTargetInfo.rtList.size(); ++i)
        {
            CTexture* pTex = CTexture::GetByID(m_showRenderTargetInfo.rtList[i].textureID);
            if (pTex && pTex != CTextureManager::Instance()->GetNoTexture())
            {
                iLog->Log("\t%" PRISIZE_T "  %s\t--------------%s  %d x %d\n",  i, pTex->GetName(), pTex->GetFormatName(), pTex->GetWidth(), pTex->GetHeight());
            }
            else
            {
                iLog->Log("\t%" PRISIZE_T "  %d\t--------------(NOT AVAILABLE)\n", i, m_showRenderTargetInfo.rtList[i].textureID);
            }
        }

        m_showRenderTargetInfo.Reset();
    }
    else if (m_showRenderTargetInfo.rtList.empty() == false)
    {
        DebugShowRenderTarget();
    }

    static char r_showTexture_prevString[256] = "";  // a wrokaround to reset the "??" command
    // show custom texture
    if (CV_r_ShowTexture)
    {
        const char* arg = CV_r_ShowTexture->GetString();

        SetState(GS_NODEPTHTEST);
        int iTmpX, iTmpY, iTempWidth, iTempHeight;
        GetViewport(&iTmpX, &iTmpY, &iTempWidth, &iTempHeight);

        TransformationMatrices backupSceneMatrices;
        Set2DMode(1, 1, backupSceneMatrices);

        char* endp;
        int texId = strtol(arg, &endp, 10);

        if (endp != arg)
        {
            CTexture* pTexToShow = CTexture::GetByID(texId);

            if (pTexToShow)
            {
                RT_SetViewport(m_width - m_width / 3 - 10, m_height - m_height / 3 - 10, m_width / 3, m_height / 3);
                DrawImage(0, 0, 1, 1, pTexToShow->GetID(), 0, 1, 1, 0, 1, 1, 1, 1, true);
                WriteXY(10, 10, 1, 1, 1, 1, 1, 1, "Name: %s", pTexToShow->GetSourceName());
                WriteXY(10, 25, 1, 1, 1, 1, 1, 1, "Fmt: %s, Type: %s", pTexToShow->GetFormatName(), CTexture::NameForTextureType(pTexToShow->GetTextureType()));
                WriteXY(10, 40, 1, 1, 1, 1, 1, 1, "Size: %dx%dx%d", pTexToShow->GetWidth(), pTexToShow->GetHeight(), pTexToShow->GetDepth());
                WriteXY(10, 40, 1, 1, 1, 1, 1, 1, "Size: %dx%d", pTexToShow->GetWidth(), pTexToShow->GetHeight());
                WriteXY(10, 55, 1, 1, 1, 1, 1, 1, "Mips: %d", pTexToShow->GetNumMips());
            }
        }
        else if (strlen(arg) == 2)
        {
            // Special cmd
            if (strcmp(arg, "??") == 0)
            {
                // print all entries in the index book
                iLog->Log("All entries:\n");

                AUTO_LOCK(CBaseResource::s_cResLock);

                const CCryNameTSCRC& resClass = CTexture::mfGetClassName();
                const SResourceContainer* pRes = CBaseResource::GetResourcesForClass(resClass);
                if (pRes)
                {
                    const size_t size = pRes->m_RList.size();
                    for (size_t i = 0; i < size; ++i)
                    {
                        CTexture* pTex = (CTexture*) pRes->m_RList[i];
                        if (pTex)
                        {
                            const char* pName = pTex->GetName();
                            if (pName && !strchr(pName, '/'))
                            {
                                iLog->Log("\t%" PRISIZE_T " %s -- fmt: %s, dim: %d x %d\n", i, pName, pTex->GetFormatName(), pTex->GetWidth(), pTex->GetHeight());
                            }
                        }
                    }
                }

                // reset after done  (revert to previous argument set)
                CV_r_ShowTexture->ForceSet(r_showTexture_prevString);
            }
        }
        else if (strlen(arg) > 2)
        {
            cry_strcpy(r_showTexture_prevString, arg);    // save argument set (for "??" processing)

            CTexture* pTexToShow = CTexture::GetByName(arg);

            if (pTexToShow)
            {
                RT_SetViewport(m_width - m_width / 3 - 10, m_height - m_height / 3 - 10, m_width / 3, m_height / 3);
                DrawImage(0, 0, 1, 1, pTexToShow->GetID(), 0, 1, 1, 0, 1, 1, 1, 1, true);
                WriteXY(10, 10, 1, 1, 1, 1, 1, 1, "Name: %s", pTexToShow->GetSourceName());
                WriteXY(10, 25, 1, 1, 1, 1, 1, 1, "Fmt: %s, Type: %s", pTexToShow->GetFormatName(), CTexture::NameForTextureType(pTexToShow->GetTextureType()));
                WriteXY(10, 40, 1, 1, 1, 1, 1, 1, "Size: %dx%dx%d", pTexToShow->GetWidth(), pTexToShow->GetHeight(), pTexToShow->GetDepth());
                WriteXY(10, 40, 1, 1, 1, 1, 1, 1, "Size: %dx%d", pTexToShow->GetWidth(), pTexToShow->GetHeight());
                WriteXY(10, 55, 1, 1, 1, 1, 1, 1, "Mips: %d", pTexToShow->GetNumMips());
            }
            else
            {
                // parse the arguments (use space as delimiter)
                string nameListStr = arg;
                char* nameListCh = (char*)nameListStr.c_str();

                std::vector<string> nameList;
                char* nextToken = nullptr;
                char* p = azstrtok(nameListCh, 0, " ", &nextToken);
                while (p)
                {
                    string s(p);
                    s.Trim();
                    if (s.length() > 0)
                    {
                        nameList.push_back(s);
                    }
                    p = azstrtok(NULL, 0, " ", &nextToken);
                }

                // render them in a tiled fashion
                RT_SetViewport(0, 0, m_width, m_height);
                const float tileW = 0.24f;
                const float tileH = 0.24f;
                const float tileGapW = 5.f / m_width;
                const float tileGapH = 25.f / m_height;

                const int maxTilesInRow = (int) ((1 - tileGapW) / (tileW + tileGapW));
                for (size_t i = 0; i < nameList.size(); i++)
                {
                    CTexture* tex = CTexture::GetByName(nameList[i].c_str());
                    if (!tex)
                    {
                        continue;
                    }

                    int row = i / maxTilesInRow;
                    int col = i - row * maxTilesInRow;
                    float curX = tileGapW + col * (tileW + tileGapW);
                    float curY = tileGapH + row * (tileH + tileGapH);
                    gcpRendD3D->FX_SetState(GS_NODEPTHTEST);  // fix the state change by using WriteXY
                    DrawImage(curX, curY, tileW, tileH, tex->GetID(),   0, 1, 1, 0, 1, 1, 1, 1, true);
                    WriteXY((int)(curX * 800 + 2), (int)((curY + tileH) * 600 - 15), 1, 1, 1, 1, 1, 1, "Fmt: %s, Type: %s", tex->GetFormatName(), CTexture::NameForTextureType(tex->GetTextureType()));
                    WriteXY((int)(curX * 800 + 2), (int)((curY + tileH) * 600 + 1), 1, 1, 1, 1, 1, 1, "%s   %d x %d", nameList[i].c_str(), tex->GetWidth(), tex->GetHeight());
                }
            }
        }

        RT_SetViewport(iTmpX, iTmpY, iTempWidth, iTempHeight);
        Unset2DMode(backupSceneMatrices);
    }

    // print dyn. textures
    {
        static bool bWasOn = false;

        if (CV_r_showdyntextures)
        {
            DrawAllDynTextures(CV_r_ShowDynTexturesFilter->GetString(), !bWasOn, CV_r_showdyntextures == 2);
            bWasOn = true;
        }
        else
        {
            bWasOn = false;
        }
    }
    // debug render listeners
    {
        for (TListRenderDebugListeners::iterator itr = m_listRenderDebugListeners.begin();
             itr != m_listRenderDebugListeners.end();
             ++itr)
        {
            (*itr)->OnDebugDraw();
        }
    }
#endif//_RELEASE
}

void CD3D9Renderer::TryFlush()
{
    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_RENDERER, g_bProfilerEnabled);

    //////////////////////////////////////////////////////////////////////
    // End the scene and update
    //////////////////////////////////////////////////////////////////////

    // Check for the presence of a D3D device
    assert(m_Device);

    m_pRT->RC_TryFlush();
}

void CD3D9Renderer::EndFrame()
{
    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_RENDERER, g_bProfilerEnabled);

    //////////////////////////////////////////////////////////////////////
    // End the scene and update
    //////////////////////////////////////////////////////////////////////

    // Check for the presence of a D3D device
    assert(m_Device);

    EF_RenderTextMessages();

    m_pRT->RC_EndFrame(!m_bStartLevelLoading);
}

static uint32 ComputePresentInterval(bool vsync, uint32 refreshNumerator, uint32 refreshDenominator)
{
    uint32 presentInterval = vsync ? 1 : 0;
    if (vsync && refreshNumerator != 0 && refreshDenominator != 0)
    {
        ICVar* pSysMaxFPS = gEnv && gEnv->pConsole ? gEnv->pConsole->GetCVar("sys_MaxFPS") : 0;
        if (pSysMaxFPS)
        {
            const int32 maxFPS = pSysMaxFPS->GetIVal();
            if (maxFPS > 0)
            {
                const float refreshRate = (float)refreshNumerator / refreshDenominator;
                const float lockedFPS = maxFPS;
                // presentInterval: how many vsync blanks between each presentation
                // 0.1f is a magic number to compensate refreshRate is not exact 60 on hardware queried by FindClosestMatchingMode (for example: 59.99x)
                presentInterval = (uint32) clamp_tpl((int) floorf(refreshRate / lockedFPS + 0.1f), 1, 4);
            }
        }
    }
    return presentInterval;
};

void CD3D9Renderer::RT_EndFrame()
{
    RT_EndFrame(false);
}

void CD3D9Renderer::RT_EndFrame(bool isLoading)
{
    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_RENDERER, g_bProfilerEnabled);

    if (!m_SceneRecurseCount)
    {
        iLog->Log("EndScene without BeginScene\n");
        return;
    }

    if (m_bDeviceLost)
    {
        return;
    }

    HRESULT hReturn = E_FAIL;

    CTimeValue TimeEndF = iTimer->GetAsyncTime();

    if (isLoading)
    {
        // Call loading-safe texture update which only applies the schedule. The render loading thread will handle the update.
        CTexture::RT_LoadingUpdate();
    }
    else
    {
        CTexture::Update();
    }

    if (CV_r_VRAMDebug == 1)
    {
        m_DevMan.DisplayMemoryUsage();
    }

    if (m_CVDisplayInfo && m_CVDisplayInfo->GetIVal() && iSystem && iSystem->IsDevMode())
    {
        static const int nIconSize = 32;
        int nIconIndex = 0;

        FX_SetState(GS_NODEPTHTEST);

        const Vec2 overscanOffset = Vec2(s_overscanBorders.x * VIRTUAL_SCREEN_WIDTH, s_overscanBorders.y * VIRTUAL_SCREEN_HEIGHT);

        CTexture*   pDefTexture = CTextureManager::Instance()->GetDefaultTexture("IconShaderCompiling");
        if (SShaderAsyncInfo::s_nPendingAsyncShaders > 0 && CV_r_shadersasynccompiling > 0 && pDefTexture)
        {
            Push2dImage(nIconSize * nIconIndex + overscanOffset.x, overscanOffset.y, nIconSize, nIconSize, pDefTexture->GetID(), 0, 1, 1, 0);
        }

        ++nIconIndex;

        pDefTexture = CTextureManager::Instance()->GetDefaultTexture("IconStreaming");
        if (CTexture::IsStreamingInProgress() && pDefTexture)
        {
            Push2dImage(nIconSize * nIconIndex + overscanOffset.x, overscanOffset.y, nIconSize, nIconSize, pDefTexture->GetID(), 0, 1, 1, 0);
        }

        ++nIconIndex;

        ++nIconIndex;

        Draw2dImageList();
    }

    m_prevCamera = m_RP.m_TI[m_RP.m_nProcessThreadID].m_cam;

    m_nDisableTemporalEffects = max(0, m_nDisableTemporalEffects - 1);

    //////////////////////////////////////////////////////////////////////////

#if defined(ENABLE_RENDER_AUX_GEOM)
    if (m_pRenderAuxGeomD3D)
    {
        assert(1 == m_SceneRecurseCount);
        // in case of MT rendering flush render thread CB is flashed (main thread CB gets flushed in SRenderThread::FlushFrame()),
        // otherwise unified "main-render" tread CB is flashed

        if (CAuxGeomCB* pAuxGeomCB = m_pRenderAuxGeomD3D->GetRenderAuxGeom())
        {
            pAuxGeomCB->Commit();
        }

        m_pRenderAuxGeomD3D->Process();
    }
#endif

    FX_SetState(GS_NODEPTHTEST);

    if (m_pPipelineProfiler)
    {
        m_pPipelineProfiler->EndFrame();
    }

    GetS3DRend().DisplayStereo();

#ifdef DO_RENDERLOG
    if (CRenderer::CV_r_log)
    {
        Logv(0, "******************************* EndFrame ********************************\n");
    }
#endif

    m_SceneRecurseCount--;

#if !defined(RELEASE)
    if (m_DevMan.GetNumInvalidDrawcalls() > 0)
    {
        // If drawcalls are skipped although there are no pending shaders, something is going wrong
        iLog->LogError("Renderer: Skipped %i drawcalls", m_DevMan.GetNumInvalidDrawcalls());
    }
#endif

    gRenDev->m_DevMan.RT_Tick();

    gRenDev->m_fRTTimeEndFrame = iTimer->GetAsyncTime().GetDifferenceInSeconds(TimeEndF);

    // Update downscaled viewport
    m_PrevViewportScale = m_CurViewportScale;
    m_CurViewportScale = m_ReqViewportScale;

    // debug modes that disable viewport downscaling for ease of use
    AZ_PUSH_DISABLE_WARNING(, "-Wconstant-logical-operand")
    if IsCVarConstAccess(constexpr) (CV_r_wireframe || CV_r_shownormals || CV_r_showtangents || CV_r_measureoverdraw)
    AZ_POP_DISABLE_WARNING
    {
        m_CurViewportScale = Vec2(1, 1);
    }

    // will be overridden in RT_RenderScene by m_CurViewportScale
    SetCurDownscaleFactor(Vec2(1, 1));

    //assert (m_nRTStackLevel[0] == 0 && m_nRTStackLevel[1] == 0 && m_nRTStackLevel[2] == 0 && m_nRTStackLevel[3] == 0);
    if (!m_bDeviceLost)
    {
        FX_Commit();
    }

    CTimeValue Time = iTimer->GetAsyncTime();

    // Flip the back buffer to the front
    if (m_bSwapBuffers)
    {
        if (IsEditorMode())
        {
            ResolveSupersampledBackbuffer();
        }

        CaptureFrameBuffer();

        CaptureFrameBufferCallBack();

        if (!IsEditorMode())
        {
#if !defined(SUPPORT_DEVICE_INFO)
            if (gEnv && gEnv->pConsole)
            {
                static ICVar* pSysMaxFPS = gEnv->pConsole->GetCVar("sys_MaxFPS");
                static ICVar* pVSync = gEnv->pConsole->GetCVar("r_Vsync");
                if (pSysMaxFPS && pVSync)
                {
                    int32 maxFPS = pSysMaxFPS->GetIVal();
                    uint32 vSync = pVSync->GetIVal();
                    if (vSync != 0)
                    {
                        LimitFramerate(maxFPS, false);
                    }
                }
            }
#endif
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_CPP_SECTION_10
    #include AZ_RESTRICTED_FILE(DriverD3D_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(SUPPORT_DEVICE_INFO)
#if defined(WIN32) || defined(WIN64)
            m_devInfo.EnforceFullscreenPreemption();
    #ifdef CRY_INTEGRATE_DX12
            m_devInfo.WaitForGPUFrames();
    #endif

#endif
            DWORD syncInterval = ComputePresentInterval(m_devInfo.SyncInterval() != 0, m_devInfo.RefreshRate().Numerator, m_devInfo.RefreshRate().Denominator);
            DWORD presentFlags = m_devInfo.PresentFlags();
            hReturn = m_pSwapChain->Present(syncInterval, presentFlags);

            if (DXGI_ERROR_DEVICE_RESET == hReturn)
            {
                CryFatalError("DXGI_ERROR_DEVICE_RESET");
            }
            else if (DXGI_ERROR_DEVICE_REMOVED == hReturn)
            {
                //CryFatalError("DXGI_ERROR_DEVICE_REMOVED");
                HRESULT result = m_Device->GetDeviceRemovedReason();
                switch (result)
                {
                case DXGI_ERROR_DEVICE_HUNG:
                    CryFatalError("DXGI_ERROR_DEVICE_HUNG");
                    break;
                case DXGI_ERROR_DEVICE_REMOVED:
                    CryFatalError("DXGI_ERROR_DEVICE_REMOVED");
                    break;
                case DXGI_ERROR_DEVICE_RESET:
                    CryFatalError("DXGI_ERROR_DEVICE_RESET");
                    break;
                case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
                    CryFatalError("DXGI_ERROR_DRIVER_INTERNAL_ERROR");
                    break;
                case DXGI_ERROR_INVALID_CALL:
                    CryFatalError("DXGI_ERROR_INVALID_CALL");
                    break;

                default:
                    CryFatalError("DXGI_ERROR_DEVICE_REMOVED");
                    break;
                }
            }
            else if (SUCCEEDED(hReturn))
            {
                m_dwPresentStatus = 0;
            }

            assert(m_nRTStackLevel[0] == 0);

            m_pCurrentBackBufferIndex = GetCurrentBackBufferIndex(m_pSwapChain);
            m_pBackBuffer = m_pBackBuffers[m_pCurrentBackBufferIndex];
            FX_SetRenderTarget(0, m_pBackBuffer, nullptr, 0);
            FX_SetActiveRenderTargets(true);
#endif
        }
        else
        {
            ScaleBackbufferToViewport();

#if DRIVERD3D_CPP_TRAIT_RT_ENDFRAME_NOTIMPL
            CRY_ASSERT_MESSAGE(0, "Case in EndFrame() not implemented yet");
#elif defined(SUPPORT_DEVICE_INFO)
            DWORD dwFlags = 0;
            if (m_dwPresentStatus & (epsOccluded | epsNonExclusive))
            {
                dwFlags = DXGI_PRESENT_TEST;
            }
            else
            {
                dwFlags = m_devInfo.PresentFlags();
            }

            if (m_CurrContext->m_pSwapChain)
            {
                hReturn = m_CurrContext->m_pSwapChain->Present(0, dwFlags);
                if (hReturn == DXGI_ERROR_INVALID_CALL)
                {
                    assert(0);
                }
                else
                if (DXGI_STATUS_OCCLUDED == hReturn)
                {
                    m_dwPresentStatus |= epsOccluded;
                }
                else
                if (DXGI_ERROR_DEVICE_RESET == hReturn)
                {
                    CryFatalError("DXGI_ERROR_DEVICE_RESET");
                }
                else
                if (DXGI_ERROR_DEVICE_REMOVED == hReturn)
                {
                    CryFatalError("DXGI_ERROR_DEVICE_REMOVED");
                }
                else
                if (SUCCEEDED(hReturn))
                {
                    // Now that we're no longer occluded and the other app has gone non-exclusive
                    // allow us to render again
                    m_dwPresentStatus = 0;
                }

                m_CurrContext->m_pCurrentBackBufferIndex = GetCurrentBackBufferIndex(m_CurrContext->m_pSwapChain);
                m_CurrContext->m_pBackBuffer = m_CurrContext->m_pBackBuffers[m_CurrContext->m_pCurrentBackBufferIndex];

                assert(m_nRTStackLevel[0] == 0);

                FX_SetRenderTarget(0, m_CurrContext->m_pBackBuffer, nullptr, 0);
                FX_SetActiveRenderTargets(true);
            }
#endif
        }
        m_nFrameSwapID++;

        m_pCurrentBackBufferIndex = GetCurrentBackBufferIndex(m_pSwapChain);
        m_pBackBuffer = m_pBackBuffers[m_pCurrentBackBufferIndex];
        FX_SetRenderTarget(0, m_pBackBuffer, &m_DepthBufferNative);
    }

    {
        if (CV_r_flush > 0 && CV_r_minimizeLatency > 0)
        {
            FlushHardware(false);
        }
    }

    CheckDeviceLost();
    //disable screenshot code path for pure release builds on consoles
#if !defined(_RELEASE) || defined(WIN32) || defined(WIN64) || defined(ENABLE_LW_PROFILERS)
    if (CV_r_GetScreenShot && m_CurrContext->m_bMainViewport)
    {
        if (CV_r_GetScreenShot == static_cast<int>(ScreenshotType::NormalToBuffer))
        {
            ScreenShot();
        }
        else if (CV_r_GetScreenShot == static_cast<int>(ScreenshotType::NormalWithFilepath))
        {
            ScreenShot(m_screenshotFilepathCache);
        }
        else
        {
            ScreenShot(nullptr);
        }
        CV_r_GetScreenShot = static_cast<int>(ScreenshotType::None);
    }
#endif
#ifndef CONSOLE_CONST_CVAR_MODE
    if (m_wireframe_mode != m_wireframe_mode_prev)
    {
        if (m_wireframe_mode > R_SOLID_MODE) // disable zpass in wireframe mode
        {
            if (m_wireframe_mode_prev == R_SOLID_MODE)
            {
                m_nUseZpass = CV_r_usezpass;
                CV_r_usezpass = 0;
            }
        }
        else
        {
            CV_r_usezpass = m_nUseZpass;
        }
    }
#endif
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_logTexStreaming)
    {
        LogStrv(0, "******************************* EndFrame ********************************\n");
        LogStrv(0, "Loaded: %.3f Kb, UpLoaded: %.3f Kb, UploadTime: %.3fMs\n\n", BYTES_TO_KB(CTexture::s_nTexturesDataBytesLoaded), BYTES_TO_KB(CTexture::s_nTexturesDataBytesUploaded), m_RP.m_PS[m_RP.m_nProcessThreadID].m_fTexUploadTime);
    }

    if (!m_2dImages.empty())
    {
        m_2dImages.resize(0);
        assert(0);
    }

    /*if (CTexture::s_nTexturesDataBytesLoaded > 3.0f*1024.0f*1024.0f)
      CTexture::m_fStreamDistFactor = min(2048.0f, CTexture::m_fStreamDistFactor*1.2f);
    else
      CTexture::m_fStreamDistFactor = max(1.0f, CTexture::m_fStreamDistFactor/1.2f);*/
    CTexture::s_nTexturesDataBytesUploaded = 0;
    CTexture::s_nTexturesDataBytesLoaded = 0;

    m_wireframe_mode_prev = m_wireframe_mode;

    m_SceneRecurseCount++;

    // we render directly to a video memory buffer
    // we need to unlock it here in case we renderered a frame without any particles
    // lock the VMEM buffer for the next frame here (to prevent a lock in the mainthread)
    // NOTE: main thread is already working on buffer+1 and we want to prepare the next one => hence buffer+2
    gRenDev->LockParticleVideoMemory((gRenDev->m_nPoolIndexRT + (SRenderPipeline::nNumParticleVertexIndexBuffer - 1)) % SRenderPipeline::nNumParticleVertexIndexBuffer);

    m_fTimeWaitForGPU[m_RP.m_nProcessThreadID] += iTimer->GetAsyncTime().GetDifferenceInSeconds(Time);

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_CPP_SECTION_11
    #include AZ_RESTRICTED_FILE(DriverD3D_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    m_fTimeGPUIdlePercent[m_RP.m_nProcessThreadID] = 0;

  #if !defined(ENABLE_PROFILING_GPU_TIMERS)
    // BK: We need a way of getting gpu frame time in release, without gpu timers
    // for now we just use overall frame time
    m_fTimeProcessedGPU[m_RP.m_nProcessThreadID] = m_fTimeProcessedRT[m_RP.m_nProcessThreadID];
  #else
    RPProfilerStats profileStats = m_pPipelineProfiler->GetBasicStats(eRPPSTATS_OverallFrame, m_RP.m_nProcessThreadID);
    m_fTimeProcessedGPU[m_RP.m_nProcessThreadID] = profileStats.gpuTime / 1000.0f;
  #endif
#endif

#if defined(USE_GEOM_CACHES)
    if (m_SceneRecurseCount == 1)
    {
        CREGeomCache::UpdateModified();
    }
#endif

#if defined(OPENGL) && !defined(CRY_USE_METAL)
    DXGLIssueFrameFences(&GetDevice());
#endif //defined(OPENGL)

    // Note: Please be aware that the below has to be called AFTER the xenon texturemanager performs
    // Note: Please be aware that the below has to be called AFTER the texturemanager performs
    // it's garbage collection because a scheduled gpu copy might be pending
    // touching the memory that will be reclaimed below.
    m_DevBufMan.ReleaseEmptyBanks(m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameUpdateID);
}

void CD3D9Renderer::RT_PresentFast()
{
    HRESULT hReturn = S_OK;
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_CPP_SECTION_12
    #include AZ_RESTRICTED_FILE(DriverD3D_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(SUPPORT_DEVICE_INFO)

    GetS3DRend().DisplayStereo();
#if defined(WIN32) || defined(WIN64)
    m_devInfo.EnforceFullscreenPreemption();
#endif
    DWORD syncInterval = m_devInfo.SyncInterval();
    DWORD presentFlags = m_devInfo.PresentFlags();
    hReturn = m_pSwapChain->Present(syncInterval, presentFlags);
#endif
    assert(hReturn == S_OK);

    m_pCurrentBackBufferIndex = GetCurrentBackBufferIndex(m_pSwapChain);
    m_pBackBuffer = m_pBackBuffers[m_pCurrentBackBufferIndex];

    assert(m_nRTStackLevel[0] == 0);

    FX_ClearTarget(m_pBackBuffer, Clr_Transparent, 0, nullptr);
    FX_SetRenderTarget(0, m_pBackBuffer, nullptr, 0);
    FX_SetActiveRenderTargets(true);

    m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameUpdateID++;
}

void CD3D9Renderer::WriteScreenshotToFile(const char* filepath)
{
    CV_r_GetScreenShot = static_cast<int>(ScreenshotType::NormalWithFilepath);
    // If filepath is a nullptr, m_screenshotFilepathCache will be empty string.
    cry_strcpy(m_screenshotFilepathCache, filepath);
}

void CD3D9Renderer::WriteScreenshotToBuffer()
{
    CV_r_GetScreenShot = static_cast<int>(ScreenshotType::NormalToBuffer);
}

bool CD3D9Renderer::CopyScreenshotToBuffer(unsigned char* imageBuffer, unsigned int width, unsigned int height)
{
    AZ_Assert(m_frameBufDesc, "Frame Buffer Description is nullptr");
    AZ_Assert(imageBuffer, "Image Buffer is nullptr");

    if (!(m_frameBufDesc->pDest))
    {
        return false;
    }

    if ((width != m_frameBufDesc->backBufferDesc.Width) || (height != m_frameBufDesc->backBufferDesc.Height))
    {
        return false;
    }

    memcpy(imageBuffer, m_frameBufDesc->pDest, m_frameBufDesc->texSize);

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CD3D9Renderer::ScreenShotInternal([[maybe_unused]] const char* filename, [[maybe_unused]] int iPreWidth)
{
    // ignore invalid file access for screenshots
    CDebugAllowFileAccess ignoreInvalidFileAccess;

#if !defined(_RELEASE) || defined(WIN32) || defined(WIN64) || defined(ENABLE_LW_PROFILERS)
    if (m_pRT && !m_pRT->IsRenderThread())
    {
        m_pRT->FlushAndWait();
    }

    if (!gEnv || !gEnv->pSystem || gEnv->pSystem->IsQuitting() || gEnv->bIsOutOfMemory)
    {
        return false;
    }

    char path[AZ::IO::IArchive::MaxPath];

    path[sizeof(path) - 1] = 0;
    gEnv->pCryPak->AdjustFileName(filename != 0 ? filename : "@user@/ScreenShots", path, AZ_ARRAY_SIZE(path), AZ::IO::IArchive::FLAGS_PATH_REAL | AZ::IO::IArchive::FLAGS_FOR_WRITING);

    if (!filename)
    {
        size_t pathLen = strlen(path);
        const char* pSlash = (!pathLen || path[pathLen - 1] == '/' || path[pathLen - 1] == '\\') ? "" : "/";

        int i = 0;
        for (; i < 10000; i++)
        {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_CPP_SECTION_13
    #include AZ_RESTRICTED_FILE(DriverD3D_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
            snprintf(&path[pathLen], sizeof(path) - 1 - pathLen, "%sScreenShot%04d.jpg", pSlash, i);
#endif
            // HACK
            // CaptureFrameBufferToFile must be fixed for 64-bit stereo screenshots
            if (GetS3DRend().IsStereoEnabled())
            {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_CPP_SECTION_14
    #include AZ_RESTRICTED_FILE(DriverD3D_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
                snprintf(&path[pathLen], sizeof(path) - 1 - pathLen, "%sScreenShot%04d_L.jpg", pSlash, i);
#endif
            }

            AZ::IO::HandleType fileHandle = fxopen(path, "rb");
            if (fileHandle == AZ::IO::InvalidHandle)
            {
                break; // file doesn't exist
            }

            gEnv->pFileIO->Close(fileHandle);
        }

        // HACK: DONT REMOVE Stereo3D will add _L and _R suffix later
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_CPP_SECTION_15
    #include AZ_RESTRICTED_FILE(DriverD3D_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        snprintf(&path[pathLen], sizeof(path) - 1 - pathLen, "%sScreenShot%04d.jpg", pSlash, i);
#endif

        if (i == 10000)
        {
            iLog->Log("Cannot save screen shot! Too many files.");
            return false;
        }
    }

    if (!gEnv->pCryPak->MakeDir(PathUtil::GetParentDirectory(path).c_str()))
    {
        iLog->Log("Cannot save screen shot! Failed to create directory \"%s\".", path);
        return false;
    }

    // Log some stats
    // -----------------------------------
    iLog->LogWithType(ILog::eInputResponse, " ");
    iLog->LogWithType(ILog::eInputResponse, "Screenshot: %s", path);
    gEnv->pConsole->ExecuteString("goto");  // output position and angle, can be used with "goto" command from console

    iLog->LogWithType(ILog::eInputResponse, " ");
    iLog->LogWithType(ILog::eInputResponse, "$5Drawcalls: %d", gEnv->pRenderer->GetCurrentNumberOfDrawCalls());
    iLog->LogWithType(ILog::eInputResponse, "$5FPS: %.1f (%.1f ms)", gEnv->pTimer->GetFrameRate(), gEnv->pTimer->GetFrameTime() * 1000.0f);

    int nPolygons, nShadowVolPolys;
    GetPolyCount(nPolygons, nShadowVolPolys);
    iLog->LogWithType(ILog::eInputResponse, "Tris: %2d,%03d", nPolygons / 1000, nPolygons % 1000);

    int nStreamCgfPoolSize = -1;
    ICVar* pVar = gEnv->pConsole->GetCVar("e_StreamCgfPoolSize");
    if (pVar)
    {
        nStreamCgfPoolSize = pVar->GetIVal();
    }

    if (gEnv->p3DEngine)
    {
        I3DEngine::SObjectsStreamingStatus objectsStreamingStatus;
        gEnv->p3DEngine->GetObjectsStreamingStatus(objectsStreamingStatus);
        iLog->LogWithType(ILog::eInputResponse, "CGF streaming: Loaded:%d InProg:%d All:%d Act:%d MemUsed:%2.2f MemReq:%2.2f PoolSize:%d",
            objectsStreamingStatus.nReady, objectsStreamingStatus.nInProgress, objectsStreamingStatus.nTotal, objectsStreamingStatus.nActive, BYTES_TO_MB(objectsStreamingStatus.nAllocatedBytes), BYTES_TO_MB(objectsStreamingStatus.nMemRequired), nStreamCgfPoolSize);
    }

    STextureStreamingStats stats(false);
    EF_Query(EFQ_GetTexStreamingInfo, stats);
    const int iPercentage = int((float)stats.nCurrentPoolSize / stats.nMaxPoolSize * 100.f);
    iLog->LogWithType(ILog::eInputResponse, "TexStreaming: MemUsed:%.2fMB(%d%%%%) PoolSize:%2.2fMB Trghput:%2.2fKB/s", BYTES_TO_MB(stats.nCurrentPoolSize), iPercentage, BYTES_TO_MB(stats.nMaxPoolSize), BYTES_TO_KB(stats.nThroughput));

    gEnv->pConsole->ExecuteString("sys_RestoreSpec test*");     // to get useful debugging information about current spec settings to the log file
    iLog->LogWithType(ILog::eInputResponse, " ");

    if (GetS3DRend().IsStereoEnabled())
    {
        GetS3DRend().TakeScreenshot(path);
        return true;
    }

    return CaptureFrameBufferToFile(path);

#else

    return true;

#endif//_RELEASE
}

bool CD3D9Renderer::ScreenShot(const char* filename, int iPreWidth)
{
#if defined(AZ_PLATFORM_LINUX)
    // TODO Linux: Development hack for the time being.
    // The screenshot system works with absolute paths, which crypak tries to lower case by default.
    // This breaks any root paths that have upper case letters.
    // There are many path casing issues though, so somewhere we are probably missing some code to handle this properly on linux.
    // Ensure we preserve all file casing for the screenshot system to work properly.
    ICVar* pCaseSensitivityCVar = gEnv->pConsole->GetCVar("sys_FilesystemCaseSensitivity");
    int previousCasingValue = 0;
    if (pCaseSensitivityCVar)
    {
        previousCasingValue = pCaseSensitivityCVar->GetIVal();
        pCaseSensitivityCVar->Set(1);
    }
    bool res = ScreenShotInternal(filename, iPreWidth);

    if (pCaseSensitivityCVar)
    {
        pCaseSensitivityCVar->Set(previousCasingValue);
    }
    return res;
#else
    return ScreenShotInternal(filename, iPreWidth);
#endif
}

bool CD3D9Renderer::ScreenShot()
{
    FrameBufferDescription frameBufDesc;
    m_frameBufDesc = &frameBufDesc;  //set the pointer so GetScreenshot has access later

    bool framePrepSuccessful = PrepFrameCapture(frameBufDesc);

    if (!framePrepSuccessful)
    {
        return false;
    }

    FillFrameBuffer(frameBufDesc, false);

    AZ::RenderScreenshotNotificationBus::Broadcast(&AZ::RenderScreenshotNotifications::OnScreenshotReady);

    m_frameBufDesc = nullptr;

    return true;
}

int CD3D9Renderer::CreateRenderTarget(const char* name, int nWidth, int nHeight, const ColorF& clearColor, ETEX_Format eTF)
{
    // check if parameters are valid
    if (!nWidth || !nHeight)
    {
        return -1;
    }

    const int maxTextureSize = GetMaxTextureSize();
    if (maxTextureSize > 0 && (nWidth > maxTextureSize || nHeight > maxTextureSize))
    {
        return -1;
    }

    CTexture* pTex = CTexture::CreateRenderTarget(name, nWidth, nHeight, clearColor, eTT_2D, FT_NOMIPS, eTF);

    return pTex->GetID();
}

bool CD3D9Renderer::DestroyRenderTarget(int nHandle)
{
    CTexture* pTex = CTexture::GetByID(nHandle);
    SAFE_RELEASE(pTex);
    return true;
}

bool CD3D9Renderer::ResizeRenderTarget(int nHandle, int nWidth, int nHeight)
{
    // check if parameters are valid
    if (!nWidth || !nHeight)
    {
        return false;
    }

    const int maxTextureSize = GetMaxTextureSize();
    if (maxTextureSize > 0 && (nWidth > maxTextureSize || nHeight > maxTextureSize))
    {
        return false;
    }
    
    CTexture* pTex = CTexture::GetByID(nHandle);
    if (!pTex || !(pTex->GetFlags() & FT_USAGE_RENDERTARGET))
    {
        return false;
    }    
    m_pRT->EnqueueRenderCommand(
        [nHandle, nWidth, nHeight]() mutable 
        {
            CTexture* pTex = CTexture::GetByID(nHandle);
            if (pTex && pTex->GetFlags() & FT_USAGE_RENDERTARGET)
            {
                pTex->SetWidth(nWidth);
                pTex->SetHeight(nHeight);
                pTex->CreateRenderTarget(pTex->GetDstFormat(), pTex->GetClearColor());
            }
        });

    return true;
}

bool CD3D9Renderer::SetRenderTarget(int nHandle, SDepthTexture* pDepthSurf)
{
    if (nHandle == 0)
    {
        // Check: Restore not working
        m_pRT->RC_PopRT(0);
        return true;
    }

    CTexture* pTex = CTexture::GetByID(nHandle);
    if (!pTex)
    {
        return false;
    }

    m_pRT->RC_PushRT(0, pTex, pDepthSurf, -1);

    return true;
}

SDepthTexture* CD3D9Renderer::CreateDepthSurface(int nWidth, int nHeight, bool shaderResourceView)
{
    SDepthTexture* pDepthTexture = new SDepthTexture;
    pDepthTexture->nWidth = nWidth;
    pDepthTexture->nHeight = nHeight;

    m_pRT->EnqueueRenderCommand([this, pDepthTexture, shaderResourceView]() 
    {
        AZ_TRACE_METHOD_NAME("CreateDepthSurface");

        D3D11_TEXTURE2D_DESC descDepth;
        ZeroStruct(descDepth);
        descDepth.Width = pDepthTexture->nWidth;
        descDepth.Height = pDepthTexture->nHeight;
        descDepth.MipLevels = 1;
        descDepth.ArraySize = 1;
        descDepth.Format = shaderResourceView ? m_ZFormat : CTexture::ConvertToDepthStencilFmt(m_ZFormat);;
        descDepth.SampleDesc.Count = 1;
        descDepth.SampleDesc.Quality = 0;
        descDepth.Usage = D3D11_USAGE_DEFAULT;
        descDepth.BindFlags = shaderResourceView ? D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE : D3D11_BIND_DEPTH_STENCIL;
        descDepth.CPUAccessFlags = 0;
        descDepth.MiscFlags = 0;

        const float clearDepth = (gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) ? 0.f : 1.f;
        const uint clearStencil = 0;
        const FLOAT clearValues[4] = { clearDepth, FLOAT(clearStencil) };

        HRESULT hr = gcpRendD3D->m_DevMan.CreateD3D11Texture2D(&descDepth, clearValues, nullptr, &pDepthTexture->pTarget, "TempDepthBuffer");
        if (hr == S_OK)
        {
            D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
            ZeroStruct(descDSV);
            descDSV.Format = CTexture::ConvertToDepthStencilFmt(m_ZFormat);
            descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            descDSV.Texture2D.MipSlice = 0;

            // Create the depth stencil view
            hr = GetDevice().CreateDepthStencilView(pDepthTexture->pTarget, &descDSV, &pDepthTexture->pSurf);
            if (hr == S_OK)
            {
#if !defined(RELEASE) && defined(WIN64)
                pDepthTexture->pTarget->SetPrivateData(WKPDID_D3DDebugObjectName, strlen("Dynamically requested Depth-Buffer"), "Dynamically requested Depth-Buffer");
#endif
                const float clearValue = (gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) ? 0.f : 1.f;
                GetDeviceContext().ClearDepthStencilView(pDepthTexture->pSurf, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, clearValue, 0);
            }
        }
        else
        {
            AZ_Assert(false, "Failed to create temporary 2D depth buffer during depth surface resource creation.");
        }
    });

    return pDepthTexture;
}

void CD3D9Renderer::DestroyDepthSurface(SDepthTexture* pDepthTexture)
{
    if (pDepthTexture)
    {
        m_pRT->EnqueueRenderCommand([=]() mutable 
        {
            // passing in true indicates we want to release the associated texture resource as well 
            pDepthTexture->Release(true);
            SAFE_DELETE(pDepthTexture);
        });
    }
}

void CD3D9Renderer::ReadFrameBuffer(unsigned char* pRGB, int nImageX, int nSizeX, int nSizeY, ERB_Type eRBType, bool bRGBA, int nScaledX, int nScaledY)
{
    m_pRT->RC_ReadFrameBuffer(pRGB, nImageX, nSizeX, nSizeY, eRBType, bRGBA, nScaledX, nScaledY);
}

void CD3D9Renderer::RT_ReadFrameBuffer([[maybe_unused]] unsigned char* pRGB, [[maybe_unused]] int nImageX, [[maybe_unused]] int nSizeX, [[maybe_unused]] int nSizeY, [[maybe_unused]] ERB_Type eRBType, [[maybe_unused]] bool bRGBA, [[maybe_unused]] int nScaledX, [[maybe_unused]] int nScaledY)
{
    #if defined(USE_D3DX) && defined (WIN32) && !defined(OPENGL)
    if (!pRGB || nImageX <= 0 || nSizeX <= 0 || nSizeY <= 0 || eRBType != eRB_BackBuffer)
    {
        return;
    }

    assert(m_pBackBuffer);
    assert(!IsEditorMode() || m_CurrContext->m_pBackBuffer == m_pBackBuffer);

    ID3D11Texture2D* pBackBufferTex(0);

    D3D11_RENDER_TARGET_VIEW_DESC bbDesc;
    m_pBackBuffer->GetDesc(&bbDesc);
    if (bbDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMS)
    {
        // TODO: resolve
    }

    m_pBackBuffer->GetResource((ID3D11Resource**) &pBackBufferTex);
    if (pBackBufferTex)
    {
        D3D11_TEXTURE2D_DESC dstDesc;
        dstDesc.Width = nScaledX <= 0 ? nSizeX : nScaledX;
        dstDesc.Height = nScaledY <= 0 ? nSizeY : nScaledY;
        dstDesc.MipLevels = 1;
        dstDesc.ArraySize = 1;
        dstDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        dstDesc.SampleDesc.Count = 1;
        dstDesc.SampleDesc.Quality = 0;
        dstDesc.Usage = D3D11_USAGE_STAGING;
        dstDesc.BindFlags = 0;
        dstDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ /* | D3D11_CPU_ACCESS_WRITE*/;
        dstDesc.MiscFlags = 0;

        ID3D11Texture2D* pDstTex(0);
        if (SUCCEEDED(GetDevice().CreateTexture2D(&dstDesc, 0, &pDstTex)))
        {
            D3D11_BOX srcBox;
            srcBox.left = 0;
            srcBox.right = nSizeX;
            srcBox.top = 0;
            srcBox.bottom = nSizeY;
            srcBox.front = 0;
            srcBox.back = 1;

            D3D11_BOX dstBox;
            dstBox.left = 0;
            dstBox.right = dstDesc.Width;
            dstBox.top = 0;
            dstBox.bottom = dstDesc.Height;
            dstBox.front = 0;
            dstBox.back = 1;

            D3DX11_TEXTURE_LOAD_INFO loadInfo;
            loadInfo.pSrcBox = &srcBox;
            loadInfo.pDstBox = &dstBox;
            loadInfo.SrcFirstMip = 0;
            loadInfo.DstFirstMip = 0;
            loadInfo.NumMips = 1;
            loadInfo.SrcFirstElement = D3D11CalcSubresource(0, 0, 1);
            loadInfo.DstFirstElement = D3D11CalcSubresource(0, 0, 1);
            loadInfo.NumElements  = 0;
            loadInfo.Filter = D3DX11_FILTER_LINEAR;
            loadInfo.MipFilter = D3DX11_FILTER_LINEAR;

            if (SUCCEEDED(D3DX11LoadTextureFromTexture(&GetDeviceContext(), pBackBufferTex, &loadInfo, pDstTex)))
            {
                D3D11_MAPPED_SUBRESOURCE mappedTex;
                STALL_PROFILER("lock/read texture")
                if (SUCCEEDED(GetDeviceContext().Map(pDstTex, 0, D3D11_MAP_READ, 0, &mappedTex)))
                {
                    if (bRGBA)
                    {
                        for (unsigned int i = 0; i < dstDesc.Height; ++i)
                        {
                            uint8* pSrc((uint8*)mappedTex.pData + i* mappedTex.RowPitch);
                            uint8* pDst((uint8*)pRGB + (dstDesc.Height - 1 - i) * nImageX * 4);
                            for (unsigned int j = 0; j < dstDesc.Width; ++j, pSrc += 4, pDst += 4)
                            {
                                pDst[0] = pSrc[2];
                                pDst[1] = pSrc[1];
                                pDst[2] = pSrc[0];
                                pDst[3] = 255;
                            }
                        }
                    }
                    else
                    {
                        for (unsigned int i = 0; i < dstDesc.Height; ++i)
                        {
                            uint8* pSrc((uint8*)mappedTex.pData + i* mappedTex.RowPitch);
                            uint8* pDst((uint8*)pRGB + (dstDesc.Height - 1 - i) * nImageX * 3);
                            for (unsigned int j = 0; j < dstDesc.Width; ++j, pSrc += 4, pDst += 3)
                            {
                                pDst[0] = pSrc[2];
                                pDst[1] = pSrc[1];
                                pDst[2] = pSrc[0];
                            }
                        }
                    }
                    GetDeviceContext().Unmap(pDstTex, 0);
                }
            }
        }
        SAFE_RELEASE(pDstTex);
    }
    SAFE_RELEASE(pBackBufferTex);
#endif
}

void CD3D9Renderer::ReadFrameBufferFast(uint32* pDstARGBA8, int dstWidth, int dstHeight, [[maybe_unused]] bool BGRA)
{
    if (!pDstARGBA8 || dstWidth <= 0 || dstHeight <= 0)
    {
        return;
    }

#if defined (AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_MAC)
    gRenDev->ForceFlushRTCommands();
    assert(m_pBackBuffer);

    D3D11_RENDER_TARGET_VIEW_DESC bbDesc;
    m_pBackBuffer->GetDesc(&bbDesc);
    if (bbDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMS)
    {
        return;
    }

    ID3D11Texture2D* pBackBufferTex(0);
    m_pBackBuffer->GetResource((ID3D11Resource**)&pBackBufferTex);
    if (pBackBufferTex)
    {
        D3D11_TEXTURE2D_DESC dstDesc;
        dstDesc.Width = dstWidth > GetWidth() ? GetWidth() : dstWidth;
        dstDesc.Height = dstHeight > GetHeight() ? GetHeight() : dstHeight;
        dstDesc.MipLevels = 1;
        dstDesc.ArraySize = 1;
        dstDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        dstDesc.SampleDesc.Count = 1;
        dstDesc.SampleDesc.Quality = 0;
        dstDesc.Usage = D3D11_USAGE_STAGING;
        dstDesc.BindFlags = 0;
        dstDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ /* | D3D11_CPU_ACCESS_WRITE*/;
        dstDesc.MiscFlags = 0;

        ID3D11Texture2D* pDstTex(0);
        if (SUCCEEDED(GetDevice().CreateTexture2D(&dstDesc, 0, &pDstTex)))
        {
            D3D11_TEXTURE2D_DESC srcDesc;
            pBackBufferTex->GetDesc(&srcDesc);

            D3D11_BOX srcBox;
            srcBox.left = 0;
            srcBox.right = dstDesc.Width;
            srcBox.top = 0;
            srcBox.bottom = dstDesc.Height;
            srcBox.front = 0;
            srcBox.back = 1;

            GetDeviceContext().CopySubresourceRegion(pDstTex, 0, 0, 0, 0, pBackBufferTex, 0, &srcBox);

            D3D11_MAPPED_SUBRESOURCE mappedTex;
            STALL_PROFILER("lock/read texture")
            if (SUCCEEDED(GetDeviceContext().Map(pDstTex, 0, D3D11_MAP_READ, 0, &mappedTex)))
            {
                for (unsigned int i = 0; i < dstDesc.Height; ++i)
                {
                    uint8* pSrc((uint8*)mappedTex.pData + i* mappedTex.RowPitch);
                    uint8* pDst((uint8*)pDstARGBA8 + i* dstWidth * 4);
                    for (unsigned int j = 0; j < dstDesc.Width; ++j, pSrc += 4, pDst += 4)
                    {
                        //If BGRA was asked for swap Blue and Red components
                        if (BGRA)
                        {
                            pDst[0] = pSrc[2];
                            pDst[1] = pSrc[1];
                            pDst[2] = pSrc[0];
                        }
                        else
                        {
                            pDst[0] = pSrc[0];
                            pDst[1] = pSrc[1];
                            pDst[2] = pSrc[2];
                        }
                        pDst[3] = 255;
                    }
                }
                GetDeviceContext().Unmap(pDstTex, 0);
            }
        }
        SAFE_RELEASE(pDstTex);
    }
    SAFE_RELEASE(pBackBufferTex);
#endif
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
// This routine initializes 2 destination surfaces for use by the CaptureFrameBufferFast routine
// It also, captures the current backbuffer into one of the created surfaces
//
// Inputs :  none
//
// Outputs : returns true if surfaces were created otherwise returns false
//
bool CD3D9Renderer::InitCaptureFrameBufferFast([[maybe_unused]] uint32 bufferWidth, [[maybe_unused]] uint32 bufferHeight)
{
    bool status = false;

#if defined(ENABLE_PROFILING_CODE)
    if (!m_Device)
    {
        return(status);
    }

    // Free the existing textures if they exist
    SAFE_RELEASE(m_pSaveTexture[0]);
    SAFE_RELEASE(m_pSaveTexture[1]);

    assert(m_pBackBuffer);
    assert(!IsEditorMode() || m_CurrContext->m_pBackBuffer == m_pBackBuffer);

    D3D11_RENDER_TARGET_VIEW_DESC bbDesc;
    m_pBackBuffer->GetDesc(&bbDesc);
    if (bbDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMS)
    {
        return(false);
    }

    m_captureFlipFlop = 0;
    ID3D11Texture2D* pSourceTexture(0);
    m_pBackBuffer->GetResource((ID3D11Resource**) &pSourceTexture);
    if (pSourceTexture)
    {
        D3D11_TEXTURE2D_DESC sourceDesc;
        pSourceTexture->GetDesc(&sourceDesc);

        if (bufferWidth == 0)
        {
            bufferWidth = sourceDesc.Width;
        }
        if (bufferHeight == 0)
        {
            bufferHeight = sourceDesc.Height;
        }

        D3D11_TEXTURE2D_DESC dstDesc;
        dstDesc.Width = bufferWidth;
        dstDesc.Height = bufferHeight;
        dstDesc.MipLevels = 1;
        dstDesc.ArraySize = 1;
        dstDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        dstDesc.SampleDesc.Count = 1;
        dstDesc.SampleDesc.Quality = 0;
        dstDesc.Usage = D3D11_USAGE_STAGING;
        dstDesc.BindFlags = 0;
        dstDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ /* | D3D11_CPU_ACCESS_WRITE*/;
        dstDesc.MiscFlags = 0;

        HRESULT hr = GetDevice().CreateTexture2D(&dstDesc, 0, &m_pSaveTexture[0]);
        if (hr != D3D_OK)
        {
            SAFE_RELEASE(pSourceTexture);
            SAFE_RELEASE(m_pSaveTexture[0]);
            return(false);
        }

        hr = GetDevice().CreateTexture2D(&dstDesc, 0, &m_pSaveTexture[1]);
        if (hr != D3D_OK)
        {
            SAFE_RELEASE(pSourceTexture);
            SAFE_RELEASE(m_pSaveTexture[0]);
            SAFE_RELEASE(m_pSaveTexture[1]);
            return(false);
        }

        // Initialize one of the buffers by capturing the current backbuffer
        // If we allow this call on the MT we cannot interact with the device
        // The first screen grab will perform the copy anyway
        /*D3D11_BOX srcBox;
        srcBox.left = 0; srcBox.right = dstDesc.Width;
        srcBox.top = 0; srcBox.bottom = dstDesc.Height;
        srcBox.front = 0; srcBox.back = 1;
        GetDeviceContext().CopySubresourceRegion(m_pSaveTexture[0], 0, 0, 0, 0, pSourceTexture, 0, &srcBox);*/

        status = true;
        SAFE_RELEASE(pSourceTexture);
    }
#else
    status = true;
#endif

    return(status);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
// This routine releases the 2 surfaces used for frame capture by the CaptureFrameBufferFast routine
//
// Inputs : None
//
// Returns : None
//
void CD3D9Renderer::CloseCaptureFrameBufferFast(void)
{
#if defined(ENABLE_PROFILING_CODE)
    SAFE_RELEASE(m_pSaveTexture[0]);
    SAFE_RELEASE(m_pSaveTexture[1]);
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// This routines uses 2 destination surfaces.  It triggers a backbuffer copy to one of its surfaces,
// and then copies the other surface to system memory.  This hopefully will remove any
// CPU stalls due to the rect lock call since the buffer will already be in system
// memory when it is called
// Inputs :
//          pDstARGBA8          :   Pointer to a buffer that will hold the captured frame (should be at least 4*dstWidth*dstHieght for RGBA surface)
//          destinationWidth    :   Width of the frame to copy
//          destinationHeight   :   Height of the frame to copy
//
//          Note :  If dstWidth or dstHeight is larger than the current surface dimensions, the dimensions
//                  of the surface are used for the copy
//
bool CD3D9Renderer::CaptureFrameBufferFast([[maybe_unused]] unsigned char* pDstRGBA8, [[maybe_unused]] int destinationWidth, [[maybe_unused]] int destinationHeight)
{
    bool bStatus(false);

#if defined(ENABLE_PROFILING_CODE)
    ID3D11Texture2D* pSourceTexture(0);

    //In case this routine is called without the init function being called
    if (m_pSaveTexture[0] == NULL || m_pSaveTexture[1] == NULL  || !m_Device)
    {
        return bStatus;
    }

    assert(m_pBackBuffer);
    assert(!IsEditorMode() || m_CurrContext->m_pBackBuffer == m_pBackBuffer);

    D3D11_RENDER_TARGET_VIEW_DESC bbDesc;
    m_pBackBuffer->GetDesc(&bbDesc);
    if (bbDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMS)
    {
        return bStatus;
    }

    m_pBackBuffer->GetResource((ID3D11Resource**) &pSourceTexture);
    if (pSourceTexture)
    {
        D3D11_TEXTURE2D_DESC sourceDesc;
        pSourceTexture->GetDesc(&sourceDesc);

        if (sourceDesc.SampleDesc.Count == 1)
        {
            ID3D11Texture2D* pCopySourceTexture, * pTargetTexture, * pCopyTexture;
            unsigned int width(destinationWidth > GetWidth() ? GetWidth() : destinationWidth);
            unsigned int height(destinationHeight > GetHeight() ? GetHeight() : destinationHeight);

            pTargetTexture =  m_captureFlipFlop ? m_pSaveTexture[1] : m_pSaveTexture[0];
            pCopyTexture =  m_captureFlipFlop ? m_pSaveTexture[0] : m_pSaveTexture[1];
            m_captureFlipFlop = (m_captureFlipFlop + 1) % 2;

            if (width != GetWidth() || height != GetHeight())
            {
                // reuse stereo left and right RTs to downscale
                GetDeviceContext().CopyResource(CTexture::s_ptexStereoL->GetDevTexture()->Get2DTexture(), pSourceTexture);
                GetUtils().Downsample(CTexture::s_ptexStereoL, CTexture::s_ptexStereoR, GetWidth(), GetHeight(), width, height);
                pCopySourceTexture = CTexture::s_ptexStereoR->GetDevTexture()->Get2DTexture();
            }
            else
            {
                pCopySourceTexture = pSourceTexture;
            }

            // Setup the copy of the captured frame to our local surface in the background
            D3D11_BOX srcBox;
            srcBox.left = 0;
            srcBox.right = width;
            srcBox.top = 0;
            srcBox.bottom = height;
            srcBox.front = 0;
            srcBox.back = 1;
            GetDeviceContext().CopySubresourceRegion(pTargetTexture, 0, 0, 0, 0, pCopySourceTexture, 0, &srcBox);

            // Copy the previous frame from our local surface to the requested buffer location
            D3D11_MAPPED_SUBRESOURCE mappedTex;
            if (SUCCEEDED(GetDeviceContext().Map(pCopyTexture, 0, D3D11_MAP_READ, 0, &mappedTex)))
            {
                for (unsigned int i = 0; i < height; ++i)
                {
                    uint8* pSrc((uint8*)mappedTex.pData + i* mappedTex.RowPitch);
                    uint8* pDst((uint8*)pDstRGBA8 + i* width * 4);
                    for (unsigned int j = 0; j < width; ++j, pSrc += 4, pDst += 4)
                    {
                        pDst[0] = pSrc[2];
                        pDst[1] = pSrc[1];
                        pDst[2] = pSrc[0];
                        pDst[3] = 255;
                    }
                }
                GetDeviceContext().Unmap(pCopyTexture, 0);
                bStatus = true;
            }
        }
    }

    SAFE_RELEASE(pSourceTexture);
#endif

    return bStatus;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
// Copy a captured surface to a buffer
//
// Inputs :
//          pDstARGBA8          :   Pointer to a buffer that will hold the captured frame (should be at least 4*dstWidth*dstHieght for RGBA surface)
//          destinationWidth    :   Width of the frame to copy
//          destinationHeight   :   Height of the frame to copy
//
//          Note :  If dstWidth or dstHeight is larger than the current surface dimensions, the dimensions
//                  of the surface are used for the copy
//
bool CD3D9Renderer::CopyFrameBufferFast([[maybe_unused]] unsigned char* pDstRGBA8, [[maybe_unused]] int destinationWidth, [[maybe_unused]] int destinationHeight)
{
    bool bStatus(false);

#if defined(ENABLE_PROFILING_CODE)
    //In case this routine is called without the init function being called
    if (m_pSaveTexture[0] == NULL || m_pSaveTexture[1] == NULL  || !m_Device)
    {
        return bStatus;
    }

    ID3D11Texture2D* pCopyTexture =  m_captureFlipFlop ? m_pSaveTexture[0] : m_pSaveTexture[1];

    // Copy the previous frame from our local surface to the requested buffer location
    D3D11_MAPPED_SUBRESOURCE mappedTex;
    if (SUCCEEDED(GetDeviceContext().Map(pCopyTexture, 0, D3D11_MAP_READ, 0, &mappedTex)))
    {
        unsigned int width(destinationWidth > GetWidth() ? GetWidth() : destinationWidth);
        unsigned int height(destinationHeight > GetHeight() ? GetHeight() : destinationHeight);

        for (unsigned int i = 0; i < height; ++i)
        {
            uint8* pSrc((uint8*)mappedTex.pData + i* mappedTex.RowPitch);
            uint8* pDst((uint8*)pDstRGBA8 + i* width * 4);
            for (unsigned int j = 0; j < width; ++j, pSrc += 4, pDst += 4)
            {
                pDst[0] = pSrc[2];
                pDst[1] = pSrc[1];
                pDst[2] = pSrc[0];
                pDst[3] = 255;
            }
        }
        GetDeviceContext().Unmap(pCopyTexture, 0);
        bStatus = true;
    }
#endif

    return bStatus;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// This routine checks for any frame buffer callbacks that are needed and calls them
//
// Inputs : None
//
//  Outputs : None
//
void CD3D9Renderer::CaptureFrameBufferCallBack(void)
{
#if defined(ENABLE_PROFILING_CODE)
    bool firstCopy = true;
    for (unsigned int i = 0; i < MAXFRAMECAPTURECALLBACK; ++i)
    {
        if (m_pCaptureCallBack[i] != NULL)
        {
            {
                unsigned char* pDestImage = NULL;
                bool bRequiresScreenShot = m_pCaptureCallBack[i]->OnNeedFrameData(pDestImage);

                if (bRequiresScreenShot)
                {
                    if (pDestImage != NULL)
                    {
                        const int widthNotAligned = m_pCaptureCallBack[i]->OnGetFrameWidth();
                        const int width = widthNotAligned - widthNotAligned % 4;
                        const int height = m_pCaptureCallBack[i]->OnGetFrameHeight();

                        bool bCaptured;
                        if (firstCopy)
                        {
                            bCaptured = CaptureFrameBufferFast(pDestImage, width, height);
                        }
                        else
                        {
                            bCaptured = CopyFrameBufferFast(pDestImage, width, height);
                        }

                        if (bCaptured == true)
                        {
                            m_pCaptureCallBack[i]->OnFrameCaptured();
                        }

                        firstCopy = false;
                    }
                    else
                    {
                        //Assuming this texture is just going to be read by GPU, no need to block on fence
                        //if( !m_pd3dDevice->IsFencePending( m_FenceFrameCaptureReady ) )
                        {
                            m_pCaptureCallBack[i]->OnFrameCaptured();
                        }
                    }
                }
            }
        }
    }
#endif
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
// This routine checks for any frame buffer callbacks that are needed and checks their flags, calling preparation functions if required
//
// Inputs : None
//
//  Outputs : None
//
void CD3D9Renderer::CaptureFrameBufferPrepare(void)
{
#if defined(ENABLE_PROFILING_CODE)
    for (unsigned int i = 0; i < MAXFRAMECAPTURECALLBACK; ++i)
    {
        if (m_pCaptureCallBack[i] != NULL)
        {
            //The consoles collect screenshots asynchronously
            int texHandle = 0;
            int flags = m_pCaptureCallBack[i]->OnCaptureFrameBegin(&texHandle);

            //screen shot requested
            if (flags & ICaptureFrameListener::eCFF_CaptureThisFrame)
            {
                int currentFrame = GetFrameID(false);

                //Currently we only support one screen capture request per frame
                if (m_nScreenCaptureRequestFrame[m_RP.m_nFillThreadID] != currentFrame)
                {
                    m_screenCapTexHandle[m_RP.m_nFillThreadID] = texHandle;
                    m_nScreenCaptureRequestFrame[m_RP.m_nFillThreadID] = currentFrame;
                }
                else
                {
                    CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, "Multiple screen caps in a single frame not supported.");
                }
            }
        }
    }
#endif
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
// This routine registers an ICaptureFrameListener object for callback when a frame is available
//
// Inputs :
//          pCapture    :   address of the ICaptureFrameListener object
//
//  Outputs : true if successful, false if not successful
//
bool CD3D9Renderer::RegisterCaptureFrame([[maybe_unused]] ICaptureFrameListener* pCapture)
{
#if defined(ENABLE_PROFILING_CODE)
    // Check to see if the address is already registered
    for (unsigned int i = 0; i < MAXFRAMECAPTURECALLBACK; ++i)
    {
        if (m_pCaptureCallBack[i] == pCapture)
        {
            return(true);
        }
    }


    // Try to find a space for the caller
    for (unsigned int i = 0; i < MAXFRAMECAPTURECALLBACK; ++i)
    {
        if (m_pCaptureCallBack[i] == NULL)
        {
            // If no other callers are registered, init the capture routine
            if (m_frameCaptureRegisterNum == 0)
            {
                uint32 bufferWidth = 0, bufferHeight = 0;
                bool status = InitCaptureFrameBufferFast(bufferWidth, bufferHeight);
                if (status == false)
                {
                    return(false);
                }
            }

            m_pCaptureCallBack[i] = pCapture;
            ++m_frameCaptureRegisterNum;
            return(true);
        }
    }
#endif
    // Could not find space
    return(false);
}

/////////////////////////////////////////////////////////////////////////
// This routine unregisters an ICaptureFrameListener object for callback
//
// Inputs :
//          pCapture    :   address of the ICaptureFrameListener object
//
//  Outputs : 1 if successful, 0 if not successful
//
bool CD3D9Renderer::UnRegisterCaptureFrame([[maybe_unused]] ICaptureFrameListener* pCapture)
{
#if defined(ENABLE_PROFILING_CODE)
    // Check to see if the address is registered, and unregister if it is
    for (unsigned int i = 0; i < MAXFRAMECAPTURECALLBACK; ++i)
    {
        if (m_pCaptureCallBack[i] == pCapture)
        {
            m_pCaptureCallBack[i] = NULL;
            --m_frameCaptureRegisterNum;
            if (m_frameCaptureRegisterNum == 0)
            {
                CloseCaptureFrameBufferFast();
            }
            return(true);
        }
    }
#endif
    return(false);
}

int CD3D9Renderer::ScreenToTexture(int nTexID)
{
    CTexture* pTex = CTexture::GetByID(nTexID);
    if (!pTex)
    {
        return -1;
    }

    int iTempX, iTempY, iWidth, iHeight;
    GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

    return 0;
}

void CD3D9Renderer::Draw2dImage(float xpos, float ypos, float w, float h, int textureid, float s0, float t0, float s1, float t1, float angle, const ColorF& col, float z)
{
    Draw2dImage(xpos, ypos, w, h, textureid, s0, t0, s1, t1, angle, col.r, col.g, col.b, col.a, z);
}

void CD3D9Renderer::Draw2dImageStretchMode(bool bStretch)
{
    if (m_bDeviceLost)
    {
        return;
    }

    m_pRT->RC_Draw2dImageStretchMode(bStretch);
}

void CD3D9Renderer::Draw2dImage(float xpos, float ypos, float w, float h, int textureid, float s0, float t0, float s1, float t1, float angle, float r, float g, float b, float a, float z)
{
    if (m_bDeviceLost)
    {
        return;
    }

    //////////////////////////////////////////////////////////////////////
    // Draw a textured quad, texture is assumed to be in video memory
    //////////////////////////////////////////////////////////////////////

    // Check for the presence of a D3D device
    assert(m_Device);

    PROFILE_FRAME(Draw_2DImage);

    CTexture* pTexture = textureid >= 0 ? CTexture::GetByID(textureid) : NULL;

    m_pRT->RC_Draw2dImage(xpos, ypos, w, h, pTexture, s0, t0, s1, t1, angle, r, g, b, a, z);
}

void    CD3D9Renderer::Push2dImage(float xpos, float ypos, float w, float h, int textureid, float s0, float t0, float s1, float t1, float angle, float r, float g, float b, float a, float z, float stereoDepth)
{
    if (m_bDeviceLost)
    {
        return;
    }

    //////////////////////////////////////////////////////////////////////
    // Draw a textured quad, texture is assumed to be in video memory
    //////////////////////////////////////////////////////////////////////

    // Check for the presence of a D3D device
    assert(m_Device);

    PROFILE_FRAME(Push_2DImage);

    CTexture* pTexture = textureid >= 0 ? CTexture::GetByID(textureid) : NULL;

    m_pRT->RC_Push2dImage(xpos, ypos, w, h, pTexture, s0, t0, s1, t1, angle, r, g, b, a, z, stereoDepth);
}

void    CD3D9Renderer::Draw2dImageList()
{
    m_pRT->RC_Draw2dImageList();
}

void    CD3D9Renderer::DrawImage(float xpos, float ypos, float w, float h, int texture_id, float s0, float t0, float s1, float t1, float r, float g, float b, float a, bool filtered)
{
    float s[4], t[4];

    s[0] = s0;
    t[0] = 1.0f - t0;
    s[1] = s1;
    t[1] = 1.0f - t0;
    s[2] = s1;
    t[2] = 1.0f - t1;
    s[3] = s0;
    t[3] = 1.0f - t1;

    DrawImageWithUV(xpos, ypos, 0, w, h, texture_id, s, t, r, g, b, a, filtered);
}


void    CD3D9Renderer::DrawImageWithUV(float xpos, float ypos, float z, float w, float h, int texture_id, float* s, float* t, float r, float g, float b, float a, bool filtered)
{
    assert(s);
    assert(t);

    if (m_bDeviceLost)
    {
        return;
    }

    m_pRT->RC_DrawImageWithUV(xpos, ypos, z, w, h, texture_id, s, t, r, g, b, a, filtered);
}

void    CD3D9Renderer::RT_DrawImageWithUV(float xpos, float ypos, float z, float w, float h, int texture_id, float s[4], float t[4], DWORD col, bool filtered)
{
    RT_DrawImageWithUVInternal(xpos, ypos, z, w, h, texture_id, s, t, col, filtered);
}

void CD3D9Renderer::RT_DrawImageWithUVInternal(float xpos, float ypos, float z, float w, float h, int texture_id, float s[4], float t[4], DWORD col, bool filtered)
{
    //////////////////////////////////////////////////////////////////////
    // Draw a textured quad, texture is assumed to be in video memory
    //////////////////////////////////////////////////////////////////////

    // Check for the presence of a D3D device
    assert(m_Device);

    PROFILE_FRAME(Draw_2DImage);



    SetCullMode(R_CULL_DISABLE);
    EF_SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
    EF_SetSrgbWrite(false);

    TempDynVB<SVF_P3F_C4B_T2F> vb(gRenDev);
    vb.Allocate(4);
    SVF_P3F_C4B_T2F* vQuad = vb.Lock();

    vQuad[0].xyz.x = xpos;
    vQuad[0].xyz.y = ypos;
    vQuad[0].xyz.z = z;
    vQuad[0].st = Vec2(s[0], t[0]);
    vQuad[0].color.dcolor = col;

    vQuad[1].xyz.x = xpos + w;
    vQuad[1].xyz.y = ypos;
    vQuad[1].xyz.z = z;
    vQuad[1].st = Vec2(s[1], t[1]);
    vQuad[1].color.dcolor = col;

    vQuad[2].xyz.x = xpos;
    vQuad[2].xyz.y = ypos + h;
    vQuad[2].xyz.z = z;
    vQuad[2].st = Vec2(s[3], t[3]);
    vQuad[2].color.dcolor = col;

    vQuad[3].xyz.x = xpos + w;
    vQuad[3].xyz.y = ypos + h;
    vQuad[3].xyz.z = z;
    vQuad[3].st = Vec2(s[2], t[2]);
    vQuad[3].color.dcolor = col;

    vb.Unlock();

    STexState TS;
    TS.SetFilterMode(filtered ? FILTER_BILINEAR : FILTER_POINT);
    TS.SetClampMode(1, 1, 1);
    CTexture::ApplyForID(0, texture_id, CTexture::GetTexState(TS), -1);

    FX_SetFPMode();

    vb.Bind(0);
    vb.Release();

    if (FAILED(FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
    {
        return;
    }

    FX_DrawPrimitive(eptTriangleStrip, 0, 4);
}

void CD3D9Renderer::DrawLines(Vec3 v[], int nump, ColorF& col, int flags, float fGround)
{
    if (nump > 1)
    {
        m_pRT->RC_DrawLines(v, nump, col, flags, fGround);
    }
}

void CD3D9Renderer::Graph(byte* g, int x, int y, int wdt, int hgt, int nC, int type, const char* text, ColorF& color, float fScale)
{
    ColorF col;
    PREFAST_SUPPRESS_WARNING(6255)
    Vec3 * vp = (Vec3*)alloca(wdt * sizeof(Vec3));
    int i;

    TransformationMatrices backupSceneMatrices;
    Set2DMode(m_width, m_height, backupSceneMatrices);

    SetState(GS_NODEPTHTEST);
    col = Col_Blue;
    int num = CTextureManager::Instance()->GetWhiteTexture()->GetTextureID();

    float fy = (float)y;
    float fx = (float)x;
    float fwdt = (float)wdt;
    float fhgt = (float)hgt;

    DrawImage(fx, fy, fwdt, 2, num, 0, 0, 1, 1, col.r, col.g, col.b, col.a);
    DrawImage(fx, fy + fhgt, fwdt, 2, num, 0, 0, 1, 1, col.r, col.g, col.b, col.a);
    DrawImage(fx, fy, 2, fhgt, num, 0, 0, 1, 1, col.r, col.g, col.b, col.a);
    DrawImage(fx + fwdt - 2, fy, 2, fhgt, num, 0, 0, 1, 1, col.r, col.g, col.b, col.a);

    float fGround = CV_r_graphstyle ? fy + fhgt : -1;

    for (i = 0; i < wdt; i++)
    {
        vp[i][0] = (float)i + fx;
        vp[i][1] = fy + (float)(g[i]) * fhgt / 255.0f;
        vp[i][2] = 0;
    }
    if (type == 1)
    {
        col = color;
        DrawLines(&vp[0], nC, col, 3, fGround);
        col = ColorF(1.0f) - col;
        col[3] = 1;
        DrawLines(&vp[nC], wdt - nC, col, 3, fGround);
    }
    else
    if (type == 2)
    {
        col = color;
        DrawLines(&vp[0], wdt, col, 3, fGround);
    }

    if (text)
    {
        WriteXY(4, y - 18, 0.5f, 1, 1, 1, 1, 1, text);
        WriteXY(wdt - 260, y - 18, 0.5f, 1, 1, 1, 1, 1, "%d ms", (int)(1000.0f * fScale));
    }

    Unset2DMode(backupSceneMatrices);
}

void CD3D9Renderer::GetModelViewMatrix(float* mat)
{
    int nThreadID = m_pRT->GetThreadList();
    *(Matrix44*)mat = m_RP.m_TI[nThreadID].m_matView;
}

void CD3D9Renderer::GetProjectionMatrix(float* mat)
{
    int nThreadID = m_pRT->GetThreadList();
    *(Matrix44*)mat = m_RP.m_TI[nThreadID].m_matProj;
}

void CD3D9Renderer::SetMatrices(float* pProjMat, float* pViewMat)
{
    int nThreadID = m_pRT->GetThreadList();
    m_RP.m_TI[nThreadID].m_matProj = *(Matrix44*)pProjMat;
    m_RP.m_TI[nThreadID].m_matView = *(Matrix44*)pViewMat;
}
///////////////////////////////////////////
void CD3D9Renderer::PushMatrix()
{
    CRY_ASSERT(static_cast<const CD3D9Renderer*> (this)->m_Device);
}

///////////////////////////////////////////
void CD3D9Renderer::PopMatrix()
{
    CRY_ASSERT(static_cast<const CD3D9Renderer*> (this)->m_Device);
}

//-----------------------------------------------------------------------------
// coded by ivo:
// calculate parameter for an off-center projection matrix.
// the projection matrix itself is calculated by D3D9.
//-----------------------------------------------------------------------------
Matrix44A OffCenterProjection(const CCamera& cam, const Vec3& nv, unsigned short max, unsigned short win_width, unsigned short win_height)
{

    //get the size of near plane
    float l = +nv.x;
    float r = -nv.x;
    float b = -nv.z;
    float t = +nv.z;

    //---------------------------------------------------

    float max_x = (float)max;
    float max_z = (float)max;

    float win_x = (float)win_width;
    float win_z = (float)win_height;

    if ((win_x < max_x) && (win_z < max_z))
    {
        //calculate parameters for off-center projection-matrix
        float ext_x = -nv.x * 2;
        float ext_z = +nv.z * 2;
        l = +nv.x + (ext_x / max_x) * win_x;
        r = +nv.x + (ext_x / max_x) * (win_x + 1);
        t = +nv.z - (ext_z / max_z) * win_z;
        b = +nv.z - (ext_z / max_z) * (win_z + 1);
    }

    Matrix44A m;
    mathMatrixPerspectiveOffCenter(&m, l, r, b, t, cam.GetNearPlane(), cam.GetFarPlane());

    return m;
}

void CD3D9Renderer::ApplyViewParameters(const CameraViewParameters& viewParameters)
{
    int nThreadID = m_pRT->GetThreadList();
    m_RP.m_TI[nThreadID].m_cam.m_viewParameters = viewParameters;
    Matrix44A* m = &m_RP.m_TI[nThreadID].m_matView;
    viewParameters.GetModelviewMatrix((float*)m);
    if (m_RP.m_TI[nThreadID].m_PersFlags & RBPF_MIRRORCAMERA)
    {
        Matrix44A tmp;

        tmp = Matrix44A(Matrix33::CreateScale(Vec3(1, -1, 1))).GetTransposed();
        m_RP.m_TI[nThreadID].m_matView = tmp * m_RP.m_TI[nThreadID].m_matView;
    }
    m = &m_RP.m_TI[nThreadID].m_matProj;
    //viewParameters.GetProjectionMatrix(*m);
    mathMatrixPerspectiveOffCenter(m, viewParameters.fWL, viewParameters.fWR, viewParameters.fWB, viewParameters.fWT, viewParameters.fNear, viewParameters.fFar);

    const bool bReverseDepth = (CV_r_ReverseDepth != 0 && (m_RP.m_TI[nThreadID].m_PersFlags & RBPF_SHADOWGEN) == 0) ? 1 : 0;
    const bool bWasReverseDepth = (m_RP.m_TI[nThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) != 0 ? 1 : 0;

    m_RP.m_TI[nThreadID].m_PersFlags &= ~RBPF_REVERSE_DEPTH;
    if (bReverseDepth)
    {
        mathMatrixPerspectiveOffCenterReverseDepth((Matrix44A*)m, viewParameters.fWL, viewParameters.fWR, viewParameters.fWB, viewParameters.fWT, viewParameters.fNear, viewParameters.fFar);
        m_RP.m_TI[nThreadID].m_PersFlags |= RBPF_REVERSE_DEPTH;
    }

    // directly reset affected states
    if ((bReverseDepth ^ bWasReverseDepth) && m_pRT->IsRenderThread())
    {
        uint32 depthState = ReverseDepthHelper::ConvertDepthFunc(m_RP.m_CurState);
        FX_SetState(m_RP.m_CurState, m_RP.m_CurAlphaRef, depthState);
    }
}

void CD3D9Renderer::SetCamera(const CCamera& cam)
{
    int nThreadID = m_pRT->GetThreadList();

    assert(m_Device);

    // Ortho-normalize camera matrix in double precision to minimize numerical errors and improve precision when inverting matrix
    Matrix34_tpl<f64> mCam34 = cam.GetMatrix();
    mCam34.OrthonormalizeFast();

    Matrix44_tpl<f64> mCam44T = mCam34.GetTransposed();
    Matrix44_tpl<f64> mView64;
    mathMatrixLookAtInverse(&mView64, &mCam44T);

    Matrix44 mView = (Matrix44_tpl<f32>)mView64;

    // Rotate around x-axis by -PI/2
    Matrix44 mViewFinal = mView;
    mViewFinal.m01 = mView.m02;
    mViewFinal.m02 = -mView.m01;
    mViewFinal.m11 = mView.m12;
    mViewFinal.m12 = -mView.m11;
    mViewFinal.m21 = mView.m22;
    mViewFinal.m22 = -mView.m21;
    mViewFinal.m31 = mView.m32;
    mViewFinal.m32 = -mView.m31;

    m_RP.m_TI[nThreadID].m_matView = mViewFinal;

    mViewFinal.m30 = 0;
    mViewFinal.m31 = 0;
    mViewFinal.m32 = 0;
    m_CameraZeroMatrix[nThreadID] = mViewFinal;

    if (m_RP.m_TI[nThreadID].m_PersFlags & RBPF_MIRRORCAMERA)
    {
        Matrix44A tmp;

        tmp = Matrix44A(Matrix33::CreateScale(Vec3(1, -1, 1))).GetTransposed();
        m_RP.m_TI[nThreadID].m_matView = tmp * m_RP.m_TI[nThreadID].m_matView;
    }

    m_RP.m_TI[nThreadID].m_cam = cam;

    CameraViewParameters viewParameters;

    // Asymmetric frustum
    float Near = cam.GetNearPlane(), Far = cam.GetFarPlane();

    float wT = tanf(cam.GetFov() * 0.5f) * Near, wB = -wT;
    float wR = wT * cam.GetProjRatio(), wL = -wR;

    if (SRendItem::m_RecurseLevel[nThreadID] <= 0 &&
        (m_RenderTileInfo.nGridSizeX > 1.f || m_RenderTileInfo.nGridSizeY > 1.f))
    {
        //
        // This math computes an off-axis projection for the screenshot system.
        // see C3DEngine::ScreenShotHighRes. This takes the would-be projection matrix
        // changes and maps them instead to frustum planes which plumb through the actual
        // camera system.
        //
        float scaleX = m_RenderTileInfo.nGridSizeX;
        float scaleY = m_RenderTileInfo.nGridSizeY;
        float scaleXInv = 1.0f / scaleX;
        float scaleYInv = 1.0f / scaleY;

        float m20 =  (scaleX - 1) - m_RenderTileInfo.nPosX * 2.0f;
        float m21 = -((scaleY - 1) - m_RenderTileInfo.nPosY * 2.0f);

        float asymLR = (m20 * (wR - wL)) * 0.5f;
        float asymTB = (m21 * (wT - wB)) * 0.5f;

        wR = (wR + asymLR) * scaleXInv;
        wL = (wL + asymLR) * scaleXInv;

        wT = (wT + asymTB) * scaleYInv;
        wB = (wB + asymTB) * scaleYInv;
    }

    viewParameters.Frustum(wL + cam.GetAsymL(), wR + cam.GetAsymR(), wB + cam.GetAsymB(), wT + cam.GetAsymT(), Near, Far);

    Vec3 vEye = cam.GetPosition();
    Vec3 vAt = vEye + Vec3((f32)mCam34(0, 1), (f32)mCam34(1, 1), (f32)mCam34(2, 1));
    Vec3 vUp = Vec3((f32)mCam34(0, 2), (f32)mCam34(1, 2), (f32)mCam34(2, 2));
    viewParameters.LookAt(vEye, vAt, vUp);
    ApplyViewParameters(viewParameters);

    EF_SetCameraInfo();
}

void CD3D9Renderer::SetRenderTile(f32 nTilesPosX, f32 nTilesPosY, f32 nTilesGridSizeX, f32 nTilesGridSizeY)
{
    m_RenderTileInfo.nPosX = nTilesPosX;
    m_RenderTileInfo.nPosY = nTilesPosY;
    m_RenderTileInfo.nGridSizeX = nTilesGridSizeX;
    m_RenderTileInfo.nGridSizeY = nTilesGridSizeY;
}

void CD3D9Renderer::SetViewport(int x, int y, int width, int height, int id)
{
    if (m_pRT->IsRenderThread())
    {
        RT_SetViewport(x, y, width, height, id);
    }
    else
    {
        ASSERT_IS_MAIN_THREAD(m_pRT)
        m_MainRTViewport.nX = x;
        m_MainRTViewport.nY = y;
        m_MainRTViewport.nWidth = width;
        m_MainRTViewport.nHeight = height;
        m_pRT->RC_SetViewport(x, y, width, height, id);
    }
}

void CD3D9Renderer::RT_SetViewport(int x, int y, int width, int height, int id)
{
    assert(m_pRT->IsRenderThread());

    if (x == 0 && y == 0 && width == GetWidth() && height == GetHeight())
    {
        width = m_FullResRect.right;
        height = m_FullResRect.bottom;
    }

    m_NewViewport.nX = x;
    m_NewViewport.nY = y;
    m_NewViewport.nWidth = width;
    m_NewViewport.nHeight = height;
    m_RP.m_PersFlags2 |= RBPF2_COMMIT_PF;
    m_RP.m_nCommitFlags |= FC_GLOBAL_PARAMS;
    m_bViewportDirty = true;
    if (-1 != id)
    {
        m_CurViewportID = clamp_tpl(id, 0, MAX_NUM_VIEWPORTS);
    }
}

void CD3D9Renderer::GetViewport(int* x, int* y, int* width, int* height) const
{
    const SViewport& vp = m_pRT->IsRenderThread() ? m_NewViewport : m_MainRTViewport;
    *x = vp.nX;
    *y = vp.nY;
    *width = vp.nWidth;
    *height = vp.nHeight;
}

void CD3D9Renderer::SetScissor(int x, int y, int width, int height)
{
    if (!x && !y && !width && !height)
    {
        EF_Scissor(false, x, y, width, height);
    }
    else
    {
        EF_Scissor(true, x, y, width, height);
    }
}

void CD3D9Renderer::SetCullMode(int mode)
{
    m_pRT->RC_SetCull(mode);
}

void CD3D9Renderer::RT_SetCull(int mode)
{
    //////////////////////////////////////////////////////////////////////
    // Change the culling mode
    //////////////////////////////////////////////////////////////////////

    assert(m_Device);

    switch (mode)
    {
    case R_CULL_DISABLE:
        D3DSetCull(eCULL_None);
        break;
    case R_CULL_BACK:
        D3DSetCull(eCULL_Back);
        break;
    case R_CULL_FRONT:
        D3DSetCull(eCULL_Front);
        break;
    }
}

void CD3D9Renderer::PushProfileMarker(const char* label)
{
    m_pRT->RC_PushProfileMarker(label);
}

void CD3D9Renderer::PopProfileMarker(const char* label)
{
    m_pRT->RC_PopProfileMarker(label);
}

void CD3D9Renderer::SetFogColor(const ColorF& color)
{
    int nThreadID = m_pRT->GetThreadList();
    m_RP.m_TI[nThreadID].m_FS.m_FogColor = color;

    // Fog color
    EF_SetFogColor(m_RP.m_TI[nThreadID].m_FS.m_FogColor);
}

bool CD3D9Renderer::EnableFog(bool enable)
{
    int nThreadID = m_pRT->GetThreadList();

    bool bPrevFog = m_RP.m_TI[nThreadID].m_FS.m_bEnable; // remember fog value

    m_RP.m_TI[nThreadID].m_FS.m_bEnable = enable;

    return bPrevFog;
}

///////////////////////////////////////////
void CD3D9Renderer::FX_PushWireframeMode(int mode)
{
    IF (m_nWireFrameStack >= MAX_WIREFRAME_STACK, 0)
    {
        CryFatalError("Pushing more than %d different WireFrame Modes onto stack", MAX_WIREFRAME_STACK);
    }
    assert(m_nWireFrameStack >= 0 && m_nWireFrameStack < MAX_WIREFRAME_STACK);
    PREFAST_ASSUME(m_nWireFrameStack >= 0 && m_nWireFrameStack < MAX_WIREFRAME_STACK);
    m_arrWireFrameStack[m_nWireFrameStack] = m_wireframe_mode;
    m_nWireFrameStack += 1;
    FX_SetWireframeMode(mode);
}

void CD3D9Renderer::FX_PopWireframeMode()
{
    IF (m_nWireFrameStack == 0, 0)
    {
        CryFatalError("WireFrame Mode more often popped than pushed");
    }

    m_nWireFrameStack -= 1;
    assert(m_nWireFrameStack >= 0 && m_nWireFrameStack < MAX_WIREFRAME_STACK);
    PREFAST_ASSUME(m_nWireFrameStack >= 0 && m_nWireFrameStack < MAX_WIREFRAME_STACK);
    int nMode = m_arrWireFrameStack[m_nWireFrameStack];
    FX_SetWireframeMode(nMode);
}

void CD3D9Renderer::FX_SetWireframeMode(int mode)
{
    assert(mode == R_WIREFRAME_MODE || mode == R_SOLID_MODE);

    if (m_wireframe_mode == mode)
    {
        return;
    }

    m_wireframe_mode = mode;

    int nState = m_RP.m_CurState;

    m_RP.m_StateOr &= ~(GS_WIREFRAME);
    if (m_wireframe_mode == R_WIREFRAME_MODE)
    {
        m_RP.m_StateOr |= GS_WIREFRAME;
    }
    else
    {
        nState &= ~(GS_WIREFRAME);
    }

    SetState(nState);
}

///////////////////////////////////////////
void CD3D9Renderer::EnableVSync([[maybe_unused]] bool enable)
{
    // TODO: remove
}

void CD3D9Renderer::DrawQuad(float x0, float y0, float x1, float y1, const ColorF& color, float z, float s0, float t0, float s1, float t1)
{
    PROFILE_FRAME(Draw_2DImage);

    ColorF c = color;
    c.NormalizeCol(c);
    DWORD col = c.pack_argb8888();

    TempDynVB<SVF_P3F_C4B_T2F> vb(gRenDev);
    vb.Allocate(4);
    SVF_P3F_C4B_T2F* vQuad = vb.Lock();

    float ftx0 = s0;
    float fty0 = t0;
    float ftx1 = s1;
    float fty1 = t1;

    // Define the quad
    vQuad[0].xyz = Vec3(x0, y0, z);
    vQuad[0].color.dcolor = col;
    vQuad[0].st = Vec2(ftx0, fty0);

    vQuad[1].xyz = Vec3(x1, y0, z);
    vQuad[1].color.dcolor = col;
    vQuad[1].st = Vec2(ftx1, fty0);

    vQuad[3].xyz = Vec3(x1, y1, z);
    vQuad[3].color.dcolor = col;
    vQuad[3].st = Vec2(ftx1, fty1);

    vQuad[2].xyz = Vec3(x0, y1, z);
    vQuad[2].color.dcolor = col;
    vQuad[2].st = Vec2(ftx0, fty1);

    vb.Unlock();
    vb.Bind(0);
    vb.Release();

    FX_Commit();

    if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
    {
        FX_DrawPrimitive(eptTriangleStrip, 0, 4);
    }
}

void CD3D9Renderer::DrawFullScreenQuad(CShader* pSH, const CCryNameTSCRC& TechName, float s0, float t0, float s1, float t1, uint32 nState)
{
    uint32 nPasses;
    pSH->FXSetTechnique(TechName);
    pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    pSH->FXBeginPass(0);

    float fWidth5 = (float)m_NewViewport.nWidth - 0.5f;
    float fHeight5 = (float)m_NewViewport.nHeight - 0.5f;

    TempDynVB<SVF_TP3F_C4B_T2F> vb(gRenDev);
    vb.Allocate(4);
    SVF_TP3F_C4B_T2F* Verts = vb.Lock();

    std::swap(t0, t1);
    Verts->pos = Vec4(-0.5f, -0.5f, 0.0f, 1.0f);
    Verts->st = Vec2(s0, t0);
    Verts->color.dcolor = (uint32) - 1;
    Verts++;

    Verts->pos = Vec4(fWidth5, -0.5f, 0.0f, 1.0f);
    Verts->st = Vec2(s1, t0);
    Verts->color.dcolor = (uint32) - 1;
    Verts++;

    Verts->pos = Vec4(-0.5f, fHeight5, 0.0f, 1.0f);
    Verts->st = Vec2(s0, t1);
    Verts->color.dcolor = (uint32) - 1;
    Verts++;

    Verts->pos = Vec4(fWidth5, fHeight5, 0.0f, 1.0f);
    Verts->st = Vec2(s1, t1);
    Verts->color.dcolor = (uint32) - 1;

    vb.Unlock();
    vb.Bind(0);
    vb.Release();

    FX_Commit();

    // Draw a fullscreen quad to sample the RT
    FX_SetState(nState);
    if (!FAILED(FX_SetVertexDeclaration(0, eVF_TP3F_C4B_T2F)))
    {
        FX_DrawPrimitive(eptTriangleStrip, 0, 4);
    }

    pSH->FXEndPass();
}

void CD3D9Renderer::DrawQuad3D(const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& v3,
    const ColorF& color, float ftx0, float fty0, float ftx1, float fty1)
{
    assert(color.r >= 0.0f && color.r <= 1.0f);
    assert(color.g >= 0.0f && color.g <= 1.0f);
    assert(color.b >= 0.0f && color.b <= 1.0f);
    assert(color.a >= 0.0f && color.a <= 1.0f);

    DWORD col = D3DRGBA(color.r, color.g, color.b, color.a);

    TempDynVB<SVF_P3F_C4B_T2F> vb(gRenDev);
    vb.Allocate(4);
    SVF_P3F_C4B_T2F* vQuad = vb.Lock();

    // Define the quad
    vQuad[0].xyz = v0;
    vQuad[0].color.dcolor = col;
    vQuad[0].st = Vec2(ftx0, fty0);

    vQuad[1].xyz = v1;
    vQuad[1].color.dcolor = col;
    vQuad[1].st = Vec2(ftx1, fty0);

    vQuad[3].xyz = v2;
    vQuad[3].color.dcolor = col;
    vQuad[3].st = Vec2(ftx1, fty1);

    vQuad[2].xyz = v3;
    vQuad[2].color.dcolor = col;
    vQuad[2].st = Vec2(ftx0, fty1);

    vb.Unlock();
    vb.Bind(0);
    vb.Release();

    FX_Commit();
    if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
    {
        FX_DrawPrimitive(eptTriangleStrip, 0, 4);
    }
}

///////////////////////////////////////////

void CD3D9Renderer::DrawPrimitivesInternal(CVertexBuffer* src, int vert_num, const eRenderPrimitiveType prim_type)
{
    size_t stride = src->m_vertexFormat.GetStride();
    switch (src->m_vertexFormat.GetEnum())
    {
    case eVF_P3F_C4B_T2F:
    case eVF_TP3F_C4B_T2F:
    case eVF_P3F_T3F:
    case eVF_P3F_T2F_T3F:
        break;
    default:
        assert(0);
        return;
    }

    FX_Commit();

    if (FAILED(FX_SetVertexDeclaration(0, src->m_vertexFormat)))
    {
        return;
    }

    FX_SetVStream(3, NULL, 0, 0);

    TempDynVBAny::CreateFillAndBind(src->m_VS.m_pLocalData, vert_num, 0, stride);

    FX_DrawPrimitive(prim_type, 0, vert_num);
}

void CD3D9Renderer::SetProfileMarker([[maybe_unused]] const char* label, ESPM mode) const
{
    if (mode == ESPM_PUSH)
    {
        PROFILE_LABEL_PUSH(label);
    }
    else
    {
        PROFILE_LABEL_POP(label);
    }
};

///////////////////////////////////////////
void CD3D9Renderer::ResetToDefault()
{
    assert(m_pRT->IsRenderThread());

    if (m_logFileHandle != AZ::IO::InvalidHandle)
    {
        Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], ".... ResetToDefault ....\n");
    }

    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_FP_DIRTY;

    EF_Scissor(false, 0, 0, 0, 0);

    SetDefaultRenderStates();

    GetDeviceContext().GSSetShader(NULL, NULL, 0);

    m_RP.m_CurState = GS_DEPTHWRITE;
    m_RP.m_previousPersFlags = 0;

    FX_ResetPipe();

    RT_UnbindTMUs();
    RT_UnbindResources();

    m_GraphicsPipeline->BindPerFrameConstantBuffer();
    m_GraphicsPipeline->BindPerViewConstantBuffer();

    FX_ResetVertexDeclaration();

    m_RP.m_ForceStateOr &= ~GS_STENCIL;

#ifdef DO_RENDERLOG
    if (m_logFileHandle != AZ::IO::InvalidHandle && CV_r_log == 3)
    {
        Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], ".... End ResetToDefault ....\n");
    }
#endif
}

///////////////////////////////////////////
void CD3D9Renderer::SetDefaultRenderStates()
{
    const bool bReverseDepth = (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) != 0;
    SStateDepth DS;
    SStateBlend BS;
    SStateRaster RS;
    DS.Desc.DepthEnable = TRUE;
    DS.Desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    DS.Desc.DepthFunc = bReverseDepth ? D3D11_COMPARISON_GREATER_EQUAL : D3D11_COMPARISON_LESS_EQUAL;
    DS.Desc.StencilEnable = FALSE;
    SetDepthState(&DS, 0);

    RS.Desc.CullMode = (m_nCurStateRS != (uint32) - 1) ? m_StatesRS[m_nCurStateRS].Desc.CullMode : D3D11_CULL_BACK;
    switch (RS.Desc.CullMode)
    {
    case D3D11_CULL_BACK:
        m_RP.m_eCull = eCULL_Back;
        break;
    case D3D11_CULL_NONE:
        m_RP.m_eCull = eCULL_None;
        break;
    case D3D11_CULL_FRONT:
        m_RP.m_eCull = eCULL_Front;
        break;
    }
    ;
    RS.Desc.FillMode = D3D11_FILL_SOLID;
    SetRasterState(&RS);

    BS.Desc.RenderTarget[0].BlendEnable = FALSE;
    BS.Desc.RenderTarget[1].BlendEnable = FALSE;
    BS.Desc.RenderTarget[2].BlendEnable = FALSE;
    BS.Desc.RenderTarget[3].BlendEnable = FALSE;
    BS.Desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    BS.Desc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    BS.Desc.RenderTarget[2].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    BS.Desc.RenderTarget[3].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    SetBlendState(&BS);
}

///////////////////////////////////////////
void CD3D9Renderer::SetMaterialColor(float r, float g, float b, float a)
{
    EF_SetGlobalColor(r, g, b, a);
}

///////////////////////////////////////////
bool CD3D9Renderer::ProjectToScreen(float ptx, float pty, float ptz, float* sx, float* sy, float* sz)
{
    int nThreadID = m_pRT->GetThreadList();
    SViewport& vp = m_pRT->IsRenderThread() ? m_NewViewport : m_MainRTViewport;

    Vec3 vOut, vIn;
    vIn.x = ptx;
    vIn.y = pty;
    vIn.z = ptz;

    int32 v[4];
    v[0] = vp.nX;
    v[1] = vp.nY;
    v[2] = vp.nWidth;
    v[3] = vp.nHeight;

    Matrix44A mIdent;
    mIdent.SetIdentity();
    if (mathVec3Project(
            &vOut,
            &vIn,
            v,
            &m_RP.m_TI[nThreadID].m_matProj,
            &m_RP.m_TI[nThreadID].m_matView,
            &mIdent))
    {
        *sx = vOut.x * 100 / vp.nWidth;
        *sy = vOut.y * 100 / vp.nHeight;
        *sz = (m_RP.m_TI[nThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) ? 1.0f - vOut.z : vOut.z;

        return true;
    }

    return false;
}


static bool InvertMatrixPrecise(Matrix44& out, const float* m)
{
    // Inverts matrix using Gaussian Elimination which is slower but numerically more stable than Cramer's Rule

    float expmat[4][8] = {
        { m[0], m[4], m[8], m[12], 1.f, 0.f, 0.f, 0.f },
        { m[1], m[5], m[9], m[13], 0.f, 1.f, 0.f, 0.f },
        { m[2], m[6], m[10], m[14], 0.f, 0.f, 1.f, 0.f },
        { m[3], m[7], m[11], m[15], 0.f, 0.f, 0.f, 1.f }
    };

    float t0, t1, t2, t3, t;
    float* r0 = expmat[0], * r1 = expmat[1], * r2 = expmat[2], * r3 = expmat[3];

    // Choose pivots and eliminate variables
    if (fabs(r3[0]) > fabs(r2[0]))
    {
        std::swap(r3, r2);
    }
    if (fabs(r2[0]) > fabs(r1[0]))
    {
        std::swap(r2, r1);
    }
    if (fabs(r1[0]) > fabs(r0[0]))
    {
        std::swap(r1, r0);
    }
    if (r0[0] == 0)
    {
        return false;
    }
    t1 = r1[0] / r0[0];
    t2 = r2[0] / r0[0];
    t3 = r3[0] / r0[0];
    t = r0[1];
    r1[1] -= t1 * t;
    r2[1] -= t2 * t;
    r3[1] -= t3 * t;
    t = r0[2];
    r1[2] -= t1 * t;
    r2[2] -= t2 * t;
    r3[2] -= t3 * t;
    t = r0[3];
    r1[3] -= t1 * t;
    r2[3] -= t2 * t;
    r3[3] -= t3 * t;
    t = r0[4];
    if (t != 0.0)
    {
        r1[4] -= t1 * t;
        r2[4] -= t2 * t;
        r3[4] -= t3 * t;
    }
    t = r0[5];
    if (t != 0.0)
    {
        r1[5] -= t1 * t;
        r2[5] -= t2 * t;
        r3[5] -= t3 * t;
    }
    t = r0[6];
    if (t != 0.0)
    {
        r1[6] -= t1 * t;
        r2[6] -= t2 * t;
        r3[6] -= t3 * t;
    }
    t = r0[7];
    if (t != 0.0)
    {
        r1[7] -= t1 * t;
        r2[7] -= t2 * t;
        r3[7] -= t3 * t;
    }

    if (fabs(r3[1]) > fabs(r2[1]))
    {
        std::swap(r3, r2);
    }
    if (fabs(r2[1]) > fabs(r1[1]))
    {
        std::swap(r2, r1);
    }
    if (r1[1] == 0)
    {
        return false;
    }
    t2 = r2[1] / r1[1];
    t3 = r3[1] / r1[1];
    r2[2] -= t2 * r1[2];
    r3[2] -= t3 * r1[2];
    r2[3] -= t2 * r1[3];
    r3[3] -= t3 * r1[3];
    t = r1[4];
    if (0.0 != t)
    {
        r2[4] -= t2 * t;
        r3[4] -= t3 * t;
    }
    t = r1[5];
    if (0.0 != t)
    {
        r2[5] -= t2 * t;
        r3[5] -= t3 * t;
    }
    t = r1[6];
    if (0.0 != t)
    {
        r2[6] -= t2 * t;
        r3[6] -= t3 * t;
    }
    t = r1[7];
    if (0.0 != t)
    {
        r2[7] -= t2 * t;
        r3[7] -= t3 * t;
    }

    if (fabs(r3[2]) > fabs(r2[2]))
    {
        std::swap(r3, r2);
    }
    if (r2[2] == 0)
    {
        return false;
    }
    t3 = r3[2] / r2[2];
    r3[3] -= t3 * r2[3];
    r3[4] -= t3 * r2[4];
    r3[5] -= t3 * r2[5];
    r3[6] -= t3 * r2[6];
    r3[7] -= t3 * r2[7];

    if (r3[3] == 0)
    {
        return false;
    }

    // Substitute back
    t = 1.0f / r3[3];
    r3[4] *= t;
    r3[5] *= t;
    r3[6] *= t;
    r3[7] *= t;                                                        // Row 3

    t2 = r2[3];
    t = 1.0f / r2[2];              // Row 2
    r2[4] = t * (r2[4] - r3[4] * t2);
    r2[5] = t * (r2[5] - r3[5] * t2);
    r2[6] = t * (r2[6] - r3[6] * t2);
    r2[7] = t * (r2[7] - r3[7] * t2);
    t1 = r1[3];
    r1[4] -= r3[4] * t1;
    r1[5] -= r3[5] * t1;
    r1[6] -= r3[6] * t1;
    r1[7] -= r3[7] * t1;
    t0 = r0[3];
    r0[4] -= r3[4] * t0;
    r0[5] -= r3[5] * t0;
    r0[6] -= r3[6] * t0;
    r0[7] -= r3[7] * t0;

    t1 = r1[2];
    t = 1.0f / r1[1];              // Row 1
    r1[4] = t * (r1[4] - r2[4] * t1);
    r1[5] = t * (r1[5] - r2[5] * t1);
    r1[6] = t * (r1[6] - r2[6] * t1);
    r1[7] = t * (r1[7] - r2[7] * t1);
    t0 = r0[2];
    r0[4] -= r2[4] * t0;
    r0[5] -= r2[5] * t0;
    r0[6] -= r2[6] * t0, r0[7] -= r2[7] * t0;

    t0 = r0[1];
    t = 1.0f / r0[0];              // Row 0
    r0[4] = t * (r0[4] - r1[4] * t0);
    r0[5] = t * (r0[5] - r1[5] * t0);
    r0[6] = t * (r0[6] - r1[6] * t0);
    r0[7] = t * (r0[7] - r1[7] * t0);

    out.m00 = r0[4];
    out.m01 = r0[5];
    out.m02 = r0[6];
    out.m03 = r0[7];
    out.m10 = r1[4];
    out.m11 = r1[5];
    out.m12 = r1[6];
    out.m13 = r1[7];
    out.m20 = r2[4];
    out.m21 = r2[5];
    out.m22 = r2[6];
    out.m23 = r2[7];
    out.m30 = r3[4];
    out.m31 = r3[5];
    out.m32 = r3[6];
    out.m33 = r3[7];

    return true;
}

static int sUnProject(float winx, float winy, float winz, const float model[16], const float proj[16], const int viewport[4], float* objx, float* objy, float* objz)
{
    Vec4 vIn;
    vIn.x = (winx - viewport[0]) * 2 / viewport[2] - 1.0f;
    vIn.y = (winy - viewport[1]) * 2 / viewport[3] - 1.0f;
    vIn.z = winz;//2.0f * winz - 1.0f;
    vIn.w = 1.0;

    float m1[16];
    for (int i = 0; i < 4; i++)
    {
        float ai0 = proj[i], ai1 = proj[4 + i], ai2 = proj[8 + i], ai3 = proj[12 + i];
        m1[i] = ai0 * model[0] + ai1 * model[1] + ai2 * model[2] + ai3 * model[3];
        m1[4 + i] = ai0 * model[4] + ai1 * model[5] + ai2 * model[6] + ai3 * model[7];
        m1[8 + i] = ai0 * model[8] + ai1 * model[9] + ai2 * model[10] + ai3 * model[11];
        m1[12 + i] = ai0 * model[12] + ai1 * model[13] + ai2 * model[14] + ai3 * model[15];
    }

    Matrix44 m;
    InvertMatrixPrecise(m, m1);

    Vec4 vOut = m * vIn;
    if (vOut.w == 0.0)
    {
        return false;
    }
    *objx = vOut.x / vOut.w;
    *objy = vOut.y / vOut.w;
    *objz = vOut.z / vOut.w;
    return true;
}

int CD3D9Renderer::UnProject(float sx, float sy, float sz, float* px, float* py, float* pz, const float modelMatrix[16], const float projMatrix[16], const int viewport[4])
{
    return sUnProject(sx, sy, sz, modelMatrix, projMatrix, viewport, px, py, pz);
}

int CD3D9Renderer::UnProjectFromScreen(float sx, float sy, float sz, float* px, float* py, float* pz)
{
    float modelMatrix[16];
    float projMatrix[16];
    int viewport[4];

    const int nThreadID = m_pRT->GetThreadList();
    if (m_RP.m_TI[nThreadID].m_PersFlags & RBPF_REVERSE_DEPTH)
    {
        sz = 1.0f - sz;
    }

    GetModelViewMatrix(modelMatrix);
    GetProjectionMatrix(projMatrix);
    GetViewport(&viewport[0], &viewport[1], &viewport[2], &viewport[3]);
    return sUnProject(sx, sy, sz, modelMatrix, projMatrix, viewport, px, py, pz);
}

//////////////////////////////////////////////////////////////////////

void CD3D9Renderer::ClearTargetsImmediately(uint32 nFlags)
{
    m_pRT->RC_ClearTargetsImmediately(0, nFlags, Clr_Transparent, Clr_FarPlane.r);
}

void CD3D9Renderer::ClearTargetsImmediately(uint32 nFlags, const ColorF& Colors, float fDepth)
{
    m_pRT->RC_ClearTargetsImmediately(1, nFlags, Colors, fDepth);
}

void CD3D9Renderer::ClearTargetsImmediately(uint32 nFlags, const ColorF& Colors)
{
    m_pRT->RC_ClearTargetsImmediately(2, nFlags, Colors, Clr_FarPlane.r);
}

void CD3D9Renderer::ClearTargetsImmediately(uint32 nFlags, float fDepth)
{
    m_pRT->RC_ClearTargetsImmediately(3, nFlags, Clr_Transparent, fDepth);
}

void CD3D9Renderer::ClearTargetsLater(uint32 nFlags)
{
    EF_ClearTargetsLater(nFlags);
}

void CD3D9Renderer::ClearTargetsLater(uint32 nFlags, const ColorF& Colors, float fDepth)
{
    EF_ClearTargetsLater(nFlags, Colors, fDepth, 0);
}

void CD3D9Renderer::ClearTargetsLater(uint32 nFlags, const ColorF& Colors)
{
    EF_ClearTargetsLater(nFlags, Colors);
}

void CD3D9Renderer::ClearTargetsLater(uint32 nFlags, float fDepth)
{
    EF_ClearTargetsLater(nFlags, fDepth, 0);
}
#ifdef _DEBUG

int s_In2DMode[RT_COMMAND_BUF_COUNT];

#endif

void CD3D9Renderer::Set2DMode(uint32 orthoX, uint32 orthoY, TransformationMatrices& backupMatrices, float znear, float zfar)
{
    Set2DModeNonZeroTopLeft(0.0f, 0.0f, orthoX, orthoY, backupMatrices, znear, zfar);
}

void CD3D9Renderer::Unset2DMode(const TransformationMatrices& restoringMatrices)
{
    int nThreadID = m_pRT->GetThreadList();

#ifdef _DEBUG
    assert(s_In2DMode[nThreadID]-- > 0);
#endif

#if defined(ENABLE_RENDER_AUX_GEOM)
    if (m_pRenderAuxGeomD3D)
    {
        m_pRenderAuxGeomD3D->SetOrthoMode(false);
    }
#endif

    m_RP.m_TI[nThreadID].m_matView = restoringMatrices.m_viewMatrix;
    m_RP.m_TI[nThreadID].m_matProj = restoringMatrices.m_projectMatrix;
    EF_SetCameraInfo();
}

void CD3D9Renderer::Set2DModeNonZeroTopLeft(float orthoLeft, float orthoTop, float orthoWidth, float orthoHeight, TransformationMatrices& backupMatrices, float znear, float zfar)
{
    int nThreadID = m_pRT->GetThreadList();

#ifdef _DEBUG
    assert(s_In2DMode[nThreadID]++ >= 0);
#endif

    backupMatrices.m_projectMatrix = m_RP.m_TI[nThreadID].m_matProj;
    Matrix44A* m = &m_RP.m_TI[nThreadID].m_matProj;

    //Move the zfar a bit away from the znear if they're the same.
    if (AZ::IsClose(znear, zfar, .001f))
    {
        zfar += .01f;
    }

    float left = orthoLeft;
    float right = left + orthoWidth;
    float top = orthoTop;
    float bottom = top + orthoHeight;

    // If we are doing tiled rendering (e.g. for a hi-res screenshot) then adjust the viewport
    const SRenderTileInfo* rti = GetRenderTileInfo();
    if (rti->nGridSizeX > 1.0f || rti->nGridSizeY > 1.0f)
    {
        // The tile size includes a transition border around it. This has all been pre-calculated by
        // the code that sets up the tiling.
        float halfTileWidth = (orthoWidth / rti->nGridSizeX) * 0.5f;
        float halfTileHeight = (orthoHeight / rti->nGridSizeY) * 0.5f;

        // This is the normalized offset from the center of the (non-tiled) viewport to the center of the tile.
        // Again the numbers in the struct have been pre-calculated to make setting a 3D matrix easier.
        float normalizedOffsetX = (rti->nGridSizeX - 1.f) - rti->nPosX * 2.0f;
        float normalizedOffsetY = (rti->nGridSizeY - 1.f) - rti->nPosY * 2.0f;

        // calculate the midpoint of this tile
        float midX = (orthoWidth * 0.5f) + (halfTileWidth * normalizedOffsetX);
        float midY = (orthoHeight * 0.5f) + (halfTileHeight * normalizedOffsetY);

        left = midX - halfTileWidth;
        right = midX + halfTileWidth;
        top = midY - halfTileHeight;
        bottom = midY + halfTileHeight;
    }

    mathMatrixOrthoOffCenterLH(m, left, right, bottom, top, znear, zfar);

    if (m_RP.m_TI[nThreadID].m_PersFlags & RBPF_REVERSE_DEPTH)
    {
        (*m) = ReverseDepthHelper::Convert(*m);
    }
#if defined(ENABLE_RENDER_AUX_GEOM)
    if (m_pRenderAuxGeomD3D)
    {
        m_pRenderAuxGeomD3D->SetOrthoMode(true, m);
    }
#endif

    backupMatrices.m_viewMatrix = m_RP.m_TI[nThreadID].m_matView;
    m = &m_RP.m_TI[nThreadID].m_matView;
    m_RP.m_TI[nThreadID].m_matView.SetIdentity();

    EF_SetCameraInfo();
}

void CD3D9Renderer::RemoveTexture(unsigned int TextureId)
{
    if (!TextureId)
    {
        return;
    }

    CTexture* tp = CTexture::GetByID(TextureId);
    if (!tp)
    {
        return;
    }

    bool bShadow = false;
    if (!bShadow)
    {
        if (tp->IsAsyncDevTexCreation())
        {
            SResourceAsync* pInfo = new SResourceAsync();
            pInfo->eClassName = eRCN_Texture;
            pInfo->pResource = tp;
            gRenDev->ReleaseResourceAsync(pInfo);
        }
        else
        {
            SAFE_RELEASE(tp);
        }
    }
}

void CD3D9Renderer::DeleteFont(IFFont* font)
{
    gRenDev->m_pRT->RC_ReleaseFont(font);
}

void CD3D9Renderer::UpdateTextureInVideoMemory(uint32 tnum, const byte* newdata, int posx, int posy, int w, int h, ETEX_Format eTFSrc, int posz, int sizez)
{
    if (m_bDeviceLost)
    {
        return;
    }

    CTexture* pTex = CTexture::GetByID(tnum);
    pTex->UpdateTextureRegion(newdata, posx, posy, posz, w, h, sizez, eTFSrc);
}

// Passing the actual texture resource to the renderer and prepping correct Mip
bool CD3D9Renderer::EF_PrecacheResource(SShaderItem* pSI, float fMipFactorSI, [[maybe_unused]] float fTimeToReady, int Flags, int nUpdateId, int nCounter)
{
    CShader*            pSH = (CShader*)pSI->m_pShader;
    CShaderResources*   pSR = (CShaderResources*)pSI->m_pShaderResources;

    if (pSH && !(pSH->m_Flags & EF_NODRAW) && pSR)
    {
        for (auto iter = pSR->m_TexturesResourcesMap.begin(); iter != pSR->m_TexturesResourcesMap.end(); ++iter)
        {
            SEfResTexture*  pResTex = &(iter->second);
            ITexture*       pITex = pResTex->m_Sampler.m_pITex;
            if (pITex)
            {
                float fMipFactor = fMipFactorSI * min(fabsf(pResTex->GetTiling(0)), fabsf(pResTex->GetTiling(1)));
                CD3D9Renderer::EF_PrecacheResource(pITex, fMipFactor, 0.f, Flags, nUpdateId, nCounter);
            }
        }
    }
    return true;
}

bool CD3D9Renderer::EF_PrecacheResource(ITexture* pTP, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId, int nCounter)
{
    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_RENDERER, g_bProfilerEnabled);

    // Check for the presence of a D3D device
    assert(m_Device);

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_TexturesStreamingDebug)
    {
        const char* const sTexFilter = CRenderer::CV_r_TexturesStreamingDebugfilter->GetString();
        if (sTexFilter != 0 && sTexFilter[0])
        {
            if (strstr(pTP->GetName(), sTexFilter))
            {
                CryLogAlways("CD3D9Renderer::EF_PrecacheResource: Mip=%5.2f nUpdateId=%4d (%s) Name=%s",
                    fMipFactor, nUpdateId, (Flags & FPR_SINGLE_FRAME_PRIORITY_UPDATE) ? "NEAR" : "FAR", pTP->GetName());
            }
        }
    }

    m_pRT->RC_PrecacheResource(pTP, fMipFactor, fTimeToReady, Flags, nUpdateId, nCounter);

    return true;
}

ITexture* CD3D9Renderer::EF_CreateCompositeTexture(int type, const char* szName, int nWidth, int nHeight, [[maybe_unused]] int nDepth, int nMips, int nFlags, ETEX_Format eTF, const STexComposition* pCompositions, size_t nCompositions, [[maybe_unused]] int8 nPriority)
{
    ITexture* pTex = NULL;

    switch (type)
    {
    case eTT_2DArray:
        pTex = CTexture::Create2DCompositeTexture(szName, nWidth, nHeight, nMips, nFlags, eTF, pCompositions, nCompositions);
        break;

    default:
        CRY_ASSERT_MESSAGE(0, "Not implemented texture format");
        pTex = CTextureManager::Instance()->GetNoTexture();
    }

    return pTex;
}

void CD3D9Renderer::CreateResourceAsync(SResourceAsync* pResource)
{
    m_pRT->RC_CreateResource(pResource);
}
void CD3D9Renderer::ReleaseResourceAsync(SResourceAsync* pResource)
{
    m_pRT->RC_ReleaseResource(pResource);
}
void CD3D9Renderer::ReleaseResourceAsync(AZStd::unique_ptr<SResourceAsync> pResource)
{
    m_pRT->RC_ReleaseResource(AZStd::move(pResource));
}

unsigned int CD3D9Renderer::DownLoadToVideoMemory(const byte* data, int w, int h, int d, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nummipmap, ETEX_Type eTT, bool repeat, int filter, int Id, const char* szCacheName, int flags, [[maybe_unused]] EEndian eEndian, RectI* pRegion, bool bAsyncDevTexCreation)
{
    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_RENDERER, g_bProfilerEnabled);

    char name[128];
    {
        if (!szCacheName)
        {
            sprintf_s(name, "$AutoDownload_%d", m_TexGenID++);
        }
        else
        {
            azstrcpy(name, AZ_ARRAY_SIZE(name), szCacheName);
        }
    }

    if (!nummipmap)
    {
        if (filter != FILTER_BILINEAR && filter != FILTER_TRILINEAR)
        {
            flags |= FT_NOMIPS;
        }
        nummipmap = 1;
    }
    else
    {
        // TODO (bethelz): Calc mips on magic number is confusing. Replace with explicit request.
        if (nummipmap < 0)
        {
            nummipmap = CTexture::CalcNumMips(w, h);
        }
    }

    if (!repeat)
    {
        flags |= FT_STATE_CLAMP;
    }

    if (!szCacheName)
    {
        flags |= FT_DONT_STREAM;
    }

    if (Id > 0)
    {
        //
        // Texture ID exists, grab the texture by id and update the texture data
        //

        CTexture* texture = CTexture::GetByID(Id);
        if (texture)
        {
            RectI rect(0, 0, w, h);
            if (pRegion)
            {
                rect.x = pRegion->x;
                rect.y = pRegion->y;
                rect.w = pRegion->w;
                rect.h = pRegion->h;
            }

            texture->UpdateTextureRegion(data, rect.x, rect.y, 0, rect.w, rect.h, 1, eTFSrc);
            return Id;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        CTexture* texture = nullptr;

        bool bIsMultiThreadedRenderer = false;
        gRenDev->EF_Query(EFQ_RenderMultithreaded, bIsMultiThreadedRenderer);

        //
        // Asynchronous creation path
        //
        if (bAsyncDevTexCreation && bIsMultiThreadedRenderer)
        {
            // make texture without device texture and request it async creation
            SResourceAsync* asyncRequest = new SResourceAsync();
            asyncRequest->pData = nullptr;
            if (data)
            {
                int nImgSize = CTexture::TextureDataSize(w, h, 1, nummipmap, 1, eTFSrc);
                asyncRequest->pData = new byte[nImgSize];
                memcpy(asyncRequest->pData, data, nImgSize);
            }
            asyncRequest->eClassName = eRCN_Texture;
            asyncRequest->nWidth = w;
            asyncRequest->nHeight = h;
            asyncRequest->nMips = nummipmap;
            asyncRequest->nTexFlags = flags;
            asyncRequest->nFormat = eTFDst;

            texture = CTexture::CreateTextureObject(name, w, h, 1, eTT_2D, flags, eTFDst);
            texture->m_bAsyncDevTexCreation = bAsyncDevTexCreation;
            texture->m_eTFSrc = eTFSrc;
            texture->m_nMips = nummipmap;

            asyncRequest->nTexId = texture->GetID();
            gRenDev->CreateResourceAsync(asyncRequest);
        }

        //
        // Synchronous creation path
        //
        else
        {
            switch (eTT)
            {
            case eTT_3D:
                texture = CTexture::Create3DTexture(name, w, h, d, nummipmap, flags, data, eTFSrc, eTFDst);
                break;
            case eTT_2D:
                texture = CTexture::Create2DTexture(name, w, h, nummipmap, flags, data, eTFSrc, eTFDst);
                break;
            default:
                assert(!"Not supported");
                return 0;
            }
        }

        return texture->GetID();
    }
}

unsigned int CD3D9Renderer::DownLoadToVideoMemory(const byte* data, int w, int h, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nummipmap, bool repeat, int filter, int Id, const char* szCacheName, int flags, EEndian eEndian, RectI* pRegion, bool bAsyncDevTexCreation)
{
    return DownLoadToVideoMemory(data, w, h, 1, eTFSrc, eTFDst, nummipmap, eTT_2D, repeat, filter, Id, szCacheName, flags, eEndian, pRegion, bAsyncDevTexCreation);
}

unsigned int CD3D9Renderer::DownLoadToVideoMemoryCube(const byte* data, int w, int h, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nummipmap, bool repeat, int filter, int Id, const char* szCacheName, int flags, EEndian eEndian, RectI* pRegion, bool bAsyncDevTexCreation)
{
    return DownLoadToVideoMemory(data, w, h, 1, eTFSrc, eTFDst, nummipmap, eTT_Cube, repeat, filter, Id, szCacheName, flags, eEndian, pRegion, bAsyncDevTexCreation);
}

unsigned int CD3D9Renderer::DownLoadToVideoMemory3D(const byte* data, int w, int h, int d, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nummipmap, bool repeat, int filter, int Id, const char* szCacheName, int flags, EEndian eEndian, RectI* pRegion, bool bAsyncDevTexCreation)
{
    return DownLoadToVideoMemory(data, w, h, d, eTFSrc, eTFDst, nummipmap, eTT_3D, repeat, filter, Id, szCacheName, flags, eEndian, pRegion, bAsyncDevTexCreation);
}

float CD3D9Renderer::GetGPUFrameTime()
{
    return CRenderer::GetGPUFrameTime();
}

void CD3D9Renderer::GetRenderTimes(SRenderTimes& outTimes)
{
    CRenderer::GetRenderTimes(outTimes);
}

const char* sStreamNames[] = {
    "VSF_GENERAL",
    "VSF_TANGENTS",
    "VSF_QTANGENTS",
    "VSF_HWSKIN_INFO",
    "VSF_VERTEX_VELOCITY",
#if ENABLE_NORMALSTREAM_SUPPORT
    "VSF_NORMALS"
#endif
};

void CD3D9Renderer::GetLogVBuffers()
{
    CRenderMesh* pRM = NULL;
    int nNums = 0;
    AUTO_LOCK(CRenderMesh::m_sLinkLock);
    for (util::list<CRenderMesh>* iter = CRenderMesh::m_MeshList.next; iter != &CRenderMesh::m_MeshList; iter = iter->next)
    {
        int nTotal = 0;
        string final;
        char tmp[128];

        COMPILE_TIME_ASSERT(sizeof(sStreamNames) / sizeof(sStreamNames[0]) == VSF_NUM);
        for (int i = 0; i < VSF_NUM; i++)
        {
            int nSize = iter->item<&CRenderMesh::m_Chain>()->GetStreamStride(i);

            sprintf_s(tmp, "| %s | %d ", sStreamNames[i], nSize);
            final += tmp;
            nTotal += nSize;
        }
        if (nTotal)
        {
            CryLog("%s | Total | %d %s", pRM->m_sSource.c_str(), nTotal, final.c_str());
        }
        nNums++;
    }
}

void CD3D9Renderer::GetMemoryUsage(ICrySizer* Sizer)
{
    assert(Sizer);

    {
        SIZER_COMPONENT_NAME(Sizer, "CRenderer");
        CRenderer::GetMemoryUsage(Sizer);
    }

    uint32 i, j;

    //SIZER_COMPONENT_NAME(Sizer, "GLRenderer");
    {
        SIZER_COMPONENT_NAME(Sizer, "Renderer dynamic");

        Sizer->AddObject(this, sizeof(CD3D9Renderer));

        for (j = 0; j < RT_COMMAND_BUF_COUNT; j++)
        {
            for (i = 0; i < MAX_REND_RECURSION_LEVELS; i++)
            {
                Sizer->AddObject(CREClientPoly::m_PolysStorage[i][j]);
            }
        }
    }
#if defined(ENABLE_RENDER_AUX_GEOM)
    {
        SIZER_COMPONENT_NAME(Sizer, "Renderer Aux Geometries");
        Sizer->AddObject(m_pRenderAuxGeomD3D);
    }
#endif
    {
        SIZER_COMPONENT_NAME(Sizer, "Renderer CryName");
        static CCryNameR sName;
        Sizer->AddObject(&sName, CCryNameR::GetMemoryUsage());
    }


    {
        SIZER_COMPONENT_NAME(Sizer, "Shaders");
        CCryNameTSCRC Name = CShader::mfGetClassName();
        SResourceContainer* pRL = NULL;
        {
            SIZER_COMPONENT_NAME(Sizer, "Shader manager");
            Sizer->AddObject(m_cEF);
        }
        {
            SIZER_COMPONENT_NAME(Sizer, "Shader resources");
            Sizer->AddObject(CShader::s_ShaderResources_known);
        }
        {
            SIZER_COMPONENT_NAME(Sizer, "ShaderCache");
            Sizer->AddObject(CHWShader::m_ShaderCache);
        }
        {
            SIZER_COMPONENT_NAME(Sizer, "HW Shaders");

            Name = CHWShader::mfGetClassName(eHWSC_Vertex);
            pRL = CBaseResource::GetResourcesForClass(Name);
            Sizer->AddObject(pRL);

            Name = CHWShader::mfGetClassName(eHWSC_Pixel);
            pRL = CBaseResource::GetResourcesForClass(Name);
            Sizer->AddObject(pRL);
        }
        {
            SIZER_COMPONENT_NAME(Sizer, "Compressed Shaders");
            Sizer->AddObject(CHWShader::m_CompressedShaders);
        }

        {
            SIZER_COMPONENT_NAME(Sizer, "Shared Shader Parameters");
            Sizer->AddObject(CGParamManager::s_Groups);
            Sizer->AddObject(CGParamManager::s_Pools);
        }
        {
            SIZER_COMPONENT_NAME(Sizer, "Light styles");
            Sizer->AddObject(CLightStyle::s_LStyles);
        }
        {
            SIZER_COMPONENT_NAME(Sizer, "SResourceContainer");
            Name = CShader::mfGetClassName();
            pRL = CBaseResource::GetResourcesForClass(Name);
            Sizer->AddObject(pRL);
        }
    }
    {
        SIZER_COMPONENT_NAME(Sizer, "Mesh");
        AUTO_LOCK(CRenderMesh::m_sLinkLock);
        for (util::list<CRenderMesh>* iter = CRenderMesh::m_MeshList.next; iter != &CRenderMesh::m_MeshList; iter = iter->next)
        {
            CRenderMesh* pRM = iter->item<&CRenderMesh::m_Chain>();
            pRM->m_sResLock.Lock();
            pRM->GetMemoryUsage(Sizer);
            if (pRM->_GetVertexContainer() != pRM)
            {
                pRM->_GetVertexContainer()->GetMemoryUsage(Sizer);
            }
            pRM->m_sResLock.Unlock();
        }
    }
    {
        SIZER_COMPONENT_NAME(Sizer, "Render elements");

        AUTO_LOCK(m_sREResLock);
        CRendElement* pRE = CRendElement::m_RootGlobal.m_NextGlobal;
        while (pRE != &CRendElement::m_RootGlobal)
        {
            Sizer->AddObject(pRE);
            pRE = pRE->m_NextGlobal;
        }
    }
    {
        SIZER_COMPONENT_NAME(Sizer, "Texture Objects");
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
                tp->GetMemoryUsage(Sizer);
            }
        }
    }
    CTexture::s_pPoolMgr->GetMemoryUsage(Sizer);
}

bool CD3D9Renderer::IsStereoEnabled() const
{
    return GetS3DRend().IsStereoEnabled();
}


void CD3D9Renderer::PostLevelLoading()
{
    CRenderer::PostLevelLoading();
    if (gRenDev)
    {
        m_bStartLevelLoading = false;
        if (m_pRT->IsMultithreaded())
        {
            iLog->Log("-- Render thread was idle during level loading: %.3f secs", gRenDev->m_pRT->m_fTimeIdleDuringLoading);
            iLog->Log("-- Render thread was busy during level loading: %.3f secs", gRenDev->m_pRT->m_fTimeBusyDuringLoading);
        }

        //if (!IsEditorMode() && !CheckDeviceLost())
        //  Reset();
        //STLALLOCATOR_CLEANUP
        m_pRT->RC_PostLoadLevel();
        m_cEF.mfSortResources();
    }

    {
        LOADING_TIME_PROFILE_SECTION(iSystem);
        CTexture::Precache();
    }
}

//====================================================================

void CD3D9Renderer::PostLevelUnload()
{
    if (m_pRT)
    {
        m_pRT->RC_FlushTextureStreaming(true);

        m_pRT->FlushAndWait();

        //In the level unload event shaders may get deleted. If that's the case any PSOs
        //that point to those deleted shaders will be invalid and will cause a crash if used.
        //Most shaders will be unloaded so invalidate the entire PSO cache and reset the pipeline.
        
        CDeviceObjectFactory::GetInstance().InvalidatePSOCache();
        //Tell the graphics pipeline to reset and throw out existing PSOs since they're now invalid
        gcpRendD3D->GetGraphicsPipeline().Reset();

        StaticCleanup();
        if (m_pColorGradingControllerD3D)
        {
            m_pColorGradingControllerD3D->ReleaseTextures();
        }
        if (CTexture::IsTextureExist(CTexture::s_ptexWaterVolumeTemp))
        {
            CTexture::s_ptexWaterVolumeTemp->ReleaseDeviceTexture(false);
        }
        for (unsigned int i = 0; i < m_TempDepths.Num(); i++)
        {
            SDepthTexture* pS = m_TempDepths[i];
            if (pS)
            {
                m_pRT->RC_ReleaseSurfaceResource(pS);
            }
        }

        PostProcessUtils().m_pCurDepthSurface = NULL;
        m_pRT->FlushAndWait();

        for (unsigned int i = 0; i < m_TempDepths.Num(); i++)
        {
            SAFE_DELETE(m_TempDepths[i])
            m_TempDepths.Free();
        }

        if (CDeferredShading::IsValid())
        {
            CDeferredShading::Instance().ResetAllLights();
            CDeferredShading::Instance().ResetAllClipVolumes();
        }
        EF_ResetPostEffects(); // reset post effects
    }

#if defined(ENABLE_RENDER_AUX_GEOM)
    if (m_pRenderAuxGeomD3D)
    {
        m_pRenderAuxGeomD3D->FreeMemory();
    }
#endif

    CPoissonDiskGen::FreeMemory();
    if (m_pColorGradingControllerD3D)
    {
        m_pColorGradingControllerD3D->FreeMemory();
    }

    g_shaderBucketAllocator.cleanup();
    g_shaderGeneralHeap->Cleanup();
}

void CD3D9Renderer::DebugShowRenderTarget()
{
    if (!m_showRenderTargetInfo.bDisplayTransparent)
    {
        SetState(GS_NODEPTHTEST);
    }
    else
    {
        SetState(GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
    }

    int x0, y0, w0, h0;
    GetViewport(&x0, &y0, &w0, &h0);
    SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
    SetSrgbWrite(false);

    TransformationMatrices backupSceneMatrices;
    Set2DMode(1, 1, backupSceneMatrices);

    m_cEF.mfRefreshSystemShader("Debug", CShaderMan::s_ShaderDebug);
    CShader* pSH = CShaderMan::s_ShaderDebug;

    RT_SetViewport(0, 0, m_width, m_height);
    float tileW = 1.f / static_cast<float>(m_showRenderTargetInfo.col);
    float tileH = 1.f / static_cast<float>(m_showRenderTargetInfo.col);

    const float tileGapW = tileW * 0.01f;
    const float tileGapH = tileH * 0.01f;

    if (m_showRenderTargetInfo.col != 1) // If not a fullsceen(= 1x1 table),
    {
        tileW -= tileGapW;
        tileH -= tileGapH;
    }

    // Draw render targets with a special debug shader.
    uint32 nPasses = 0;
    pSH->FXSetTechnique("Debug_RenderTarget");
    pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    pSH->FXBeginPass(0);

    static CCryNameR colorMultiplierName("colorMultiplier");
    static CCryNameR showRTFlagsName("showRTFlags");
    int nTexStateLinear = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
    int nTexStatePoint = CTexture::GetTexState(STexState(FILTER_POINT, true));

    size_t col2 = m_showRenderTargetInfo.col * m_showRenderTargetInfo.col;
    for (size_t i = 0; i < min(m_showRenderTargetInfo.rtList.size(), col2); ++i)
    {
        CTexture* pTex = CTexture::GetByID(m_showRenderTargetInfo.rtList[i].textureID);
        if (!pTex)
        {
            continue;
        }

        int row = i / m_showRenderTargetInfo.col;
        int col = i - row * m_showRenderTargetInfo.col;
        float curX = col * (tileW + tileGapW);
        float curY = row * (tileH + tileGapH);
        pTex->Apply(0, m_showRenderTargetInfo.rtList[i].bFiltered ? nTexStateLinear : nTexStatePoint);

        Vec4 channelWeight = m_showRenderTargetInfo.rtList[i].channelWeight;
        pSH->FXSetPSFloat(colorMultiplierName, &channelWeight, 1);

        Vec4 showRTFlags(0, 0, 0, 0); // onlyAlpha, RGBKEncoded, aliased, 0
        if (channelWeight.x == 0 && channelWeight.y == 0 && channelWeight.z == 0 && channelWeight.w > 0.5f)
        {
            showRTFlags.x = 1.0f;
        }
        showRTFlags.y = m_showRenderTargetInfo.rtList[i].bRGBKEncoded ? 1.0f : 0.0f;
        showRTFlags.z = m_showRenderTargetInfo.rtList[i].bAliased ? 1.0f : 0.0f;
        pSH->FXSetPSFloat(showRTFlagsName, &showRTFlags, 1);

        PostProcessUtils().DrawScreenQuad(pTex->GetWidth(), pTex->GetHeight(), curX, curY, curX + tileW, curY + tileH);
    }

    pSH->FXEndPass();
    pSH->FXEnd();

    // Draw overlay texts.
    for (size_t i = 0; i < min(m_showRenderTargetInfo.rtList.size(), col2); ++i)
    {
        CTexture* pTex = CTexture::GetByID(m_showRenderTargetInfo.rtList[i].textureID);
        if (!pTex)
        {
            continue;
        }

        int row = i / m_showRenderTargetInfo.col;
        int col = i - row * m_showRenderTargetInfo.col;
        float curX = col * (tileW + tileGapW);
        float curY = row * (tileH + tileGapH);
        gcpRendD3D->FX_SetState(GS_NODEPTHTEST);  // fix the state change by using WriteXY

        WriteXY((int)(curX * 800 + 2), (int)((curY + tileH) * 600 - 30), 1, 1, 1, 1, 1, 1, "Fmt: %s, Type: %s", pTex->GetFormatName(), CTexture::NameForTextureType(pTex->GetTextureType()));
        WriteXY((int)(curX * 800 + 2), (int)((curY + tileH) * 600 - 15), 1, 1, 1, 1, 1, 1, "%s   %d x %d", pTex->GetName(), pTex->GetWidth(), pTex->GetHeight());
    }

    RT_SetViewport(x0, y0, w0, h0);
    Unset2DMode(backupSceneMatrices);
}

//====================================================================

ILog* iLog;
IConsole* iConsole;
ITimer* iTimer;
ISystem* iSystem;


//////////////////////////////////////////////////////////////////////////
struct CSystemEventListner_Render
    : public ISystemEventListener
{
public:
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
    {
        static bool bInside = false;
        if (bInside)
        {
            return;
        }
        bInside = true;
        switch (event)
        {
        case ESYSTEM_EVENT_GAME_POST_INIT:
        {
        }
        break;

        case ESYSTEM_EVENT_LEVEL_LOAD_RESUME_GAME:
        case ESYSTEM_EVENT_LEVEL_LOAD_PREPARE:
        {
        }
        break;

        case ESYSTEM_EVENT_LEVEL_LOAD_START:
        {
            if (gRenDev)
            {
                gRenDev->m_cEF.m_bActivated = false;
                gRenDev->m_bEndLevelLoading = false;
                gRenDev->m_bStartLevelLoading = true;
                gRenDev->m_bInLevel = true;
                if (gRenDev->m_pRT)
                {
                    gRenDev->m_pRT->m_fTimeIdleDuringLoading = 0;
                    gRenDev->m_pRT->m_fTimeBusyDuringLoading = 0;
                }
            }
            STLALLOCATOR_CLEANUP
            if IsCVarConstAccess(constexpr) (CRenderer::CV_r_texpostponeloading)
            {
                CTexture::s_bPrecachePhase = true;
            }

            CTexture::s_bInLevelPhase = true;

            CResFile::m_nMaxOpenResFiles = MAX_OPEN_RESFILES * 2;
            SShaderBin::s_nMaxFXBinCache = MAX_FXBIN_CACHE * 2;
        }
        break;
        case ESYSTEM_EVENT_LEVEL_LOAD_END:
        {
            gRenDev->m_bStartLevelLoading = false;
            gRenDev->m_bEndLevelLoading = true;
            gRenDev->m_nFrameLoad++;
            //STLALLOCATOR_CLEANUP
        }
            {
                gRenDev->RefreshSystemShaders();

                // make sure all commands have been properly processes before leaving the level loading
                if (gRenDev->m_pRT)
                {
                    gRenDev->m_pRT->FlushAndWait();
                }

                g_shaderBucketAllocator.cleanup();
                g_shaderGeneralHeap->Cleanup();
            }
            break;

        case ESYSTEM_EVENT_LEVEL_PRECACHE_START:
        {
            CTexture::s_bPrestreamPhase = true;
        }
        break;

        case ESYSTEM_EVENT_LEVEL_PRECACHE_END:
        {
            CTexture::s_bPrestreamPhase = false;
        }
        break;

        case ESYSTEM_EVENT_LEVEL_UNLOAD:
        {
            CTexture::s_bInLevelPhase = false;
            gRenDev->m_bInLevel = false;
        }
        break;

        case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
        {
            gRenDev->PostLevelUnload();
            STLALLOCATOR_CLEANUP;
            break;
        }
        case ESYSTEM_EVENT_RESIZE:
            break;
        case ESYSTEM_EVENT_ACTIVATE:
#if defined(SUPPORT_DEVICE_INFO_MSG_PROCESSING)
            gcpRendD3D->DevInfo().OnActivate(wparam, lparam);
#endif
            break;
        case ESYSTEM_EVENT_CHANGE_FOCUS:
            break;
        case ESYSTEM_EVENT_GAME_POST_INIT_DONE:
            if (gRenDev && !gRenDev->IsEditorMode())
            {
                EnableCloseButton(gRenDev->GetHWND(), true);
            }
            break;

#if DRIVERD3D_CPP_TRAIT_ONSYSTEMEVENT_EVENTMOVE
        case ESYSTEM_EVENT_MOVE:
        {
            // When moving the window, update the preferred monitor dimensions so full-screen will pick the closest monitor
            HWND hWnd = (HWND)gcpRendD3D->GetHWND();
            HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

            MONITORINFO monitorInfo;
            monitorInfo.cbSize = sizeof(MONITORINFO);
            GetMonitorInfo(hMonitor, &monitorInfo);

            gcpRendD3D->m_prefMonX = monitorInfo.rcMonitor.left;
            gcpRendD3D->m_prefMonY = monitorInfo.rcMonitor.top;
            gcpRendD3D->m_prefMonWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
            gcpRendD3D->m_prefMonHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
        }
        break;
#endif
        }
        bInside = false;
    }
};

static CSystemEventListner_Render g_system_event_listener_render;

extern "C" DLL_EXPORT IRenderer * CreateCryRenderInterface(ISystem * pSystem)
{
    ModuleInitISystem(pSystem, "CryRenderer");

    gbRgb = false;

    iConsole = gEnv->pConsole;
    iLog = gEnv->pLog;
    iTimer = gEnv->pTimer;
    iSystem = gEnv->pSystem;

    gcpRendD3D->InitRenderer();

    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    srand((uint32)li.QuadPart);

    g_CpuFlags = iSystem->GetCPUFlags();

    iSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_render);
    return gRenDev;
}

class CEngineModule_CryRenderer
    : public IEngineModule
{
    CRYINTERFACE_SIMPLE(IEngineModule)
    CRYGENERATE_SINGLETONCLASS(CEngineModule_CryRenderer, "EngineModule_CryRenderer", 0x540c91a7338e41d3, 0xaceeac9d55614450)

    virtual const char* GetName() const {
        return "CryRenderer";
    }
    virtual const char* GetCategory() const { return "CryEngine"; }

    virtual bool Initialize(SSystemGlobalEnvironment& env, [[maybe_unused]] const SSystemInitParams& initParams)
    {
        ISystem* pSystem = env.pSystem;
        env.pRenderer = CreateCryRenderInterface(pSystem);
        return env.pRenderer != 0;
    }
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryRenderer)

CEngineModule_CryRenderer::CEngineModule_CryRenderer()
{
};

CEngineModule_CryRenderer::~CEngineModule_CryRenderer()
{
};

//=========================================================================================
void CD3D9Renderer::LockParticleVideoMemory(uint32 nThreadId)
{
    FRAME_PROFILER("LockParticleVideoMemory", gEnv->pSystem, PROFILE_RENDERER);
    // unlock the particle VMEM buffer in case no particel were rendered(and thus no unlock was called)
    UnLockParticleVideoMemory(nThreadId);

    SRenderPipeline& rp = gRenDev->m_RP;

    // lock video memory vertex/index buffer and expose base pointer and offset
    if (rp.m_pParticleVertexBuffer[nThreadId])
    {
        rp.m_pParticleVertexVideoMemoryBase[nThreadId] = rp.m_pParticleVertexBuffer[nThreadId]->LockVB(CV_r_ParticleVerticePoolSize);
    }
    if (rp.m_pParticleIndexBuffer[nThreadId])
    {
        rp.m_pParticleindexVideoMemoryBase[nThreadId] = alias_cast<byte*>(rp.m_pParticleIndexBuffer[nThreadId]->LockIB(CV_r_ParticleVerticePoolSize * 3));
    }

    rp.m_nParticleVertexOffset[nThreadId] = 0;
    rp.m_nParticleIndexOffset[nThreadId] = 0;
}

void CD3D9Renderer::UnLockParticleVideoMemory(uint32 nThreadId)
{
    SRenderPipeline& rp = gRenDev->m_RP;

    // unlock particle vertex/index buffer
    if (rp.m_pParticleVertexBuffer[nThreadId])
    {
        rp.m_pParticleVertexBuffer[nThreadId]->UnlockVB();
    }
    if (rp.m_pParticleIndexBuffer[nThreadId])
    {
        rp.m_pParticleIndexBuffer[nThreadId]->UnlockIB();
    }
}

void CD3D9Renderer::InsertParticleVideoMemoryFence(int nThreadId)
{
    SRenderPipeline& rp = gRenDev->m_RP;

    if (rp.m_pParticleVertexBuffer[nThreadId])
    {
        rp.m_pParticleVertexBuffer[nThreadId]->SetFence();
    }
    if (rp.m_pParticleIndexBuffer[nThreadId])
    {
        rp.m_pParticleIndexBuffer[nThreadId]->SetFence();
    }
}

#ifdef SUPPORT_HW_MOUSE_CURSOR
IHWMouseCursor* CD3D9Renderer::GetIHWMouseCursor()
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DRIVERD3D_CPP_SECTION_16
    #include AZ_RESTRICTED_FILE(DriverD3D_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    #error Need implementation of IHWMouseCursor
#endif
}

#endif

CD3D9Renderer::FrameBufferDescription::~FrameBufferDescription()
{
    if (pDest)
    {
        delete[] pDest;
    }

    if (includeAlpha)
    {
        if (tempZtex)
        {
            gcpRendD3D->GetDeviceContext().Unmap(tempZtex, 0);
        } 
    }
    SAFE_RELEASE(tempZtex);

    if (pTmpTexture)
    {
        gcpRendD3D->GetDeviceContext().Unmap(pTmpTexture, 0);
    }
    
    SAFE_RELEASE(pTmpTexture);

    SAFE_RELEASE(pBackBufferTex);
}

void CD3D9Renderer::RT_DrawVideoRenderer([[maybe_unused]] AZ::VideoRenderer::IVideoRenderer* pVideoRenderer, const AZ::VideoRenderer::DrawArguments& drawArguments)
{
    AZ_Assert(pVideoRenderer != nullptr, "Expected video player to be passed in");
    GetGraphicsPipeline().RenderVideo(drawArguments);
}
