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

#include "D3DStereo.h"
#include "DriverD3D.h"
#include "D3DPostProcess.h"
#include "D3DHMDRenderer.h"


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define D3DSTEREO_CPP_SECTION_1 1
#define D3DSTEREO_CPP_SECTION_2 2
#endif

#include <MathConversion.h>

CD3DStereoRenderer::CD3DStereoRenderer(CD3D9Renderer& renderer, EStereoDevice device)
    : m_renderer(renderer)
    , m_device(device)
    , m_deviceState(STEREO_DEVSTATE_UNSUPPORTED_DEVICE)
    , m_mode(STEREO_MODE_NO_STEREO)
    , m_output(STEREO_OUTPUT_STANDARD)
    , m_driver(DRIVER_UNKNOWN)
    , m_pLeftTex(nullptr)
    , m_pRightTex(nullptr)
    , m_nvStereoHandle(nullptr)
    , m_nvStereoStrength(0.0f)
    , m_nvStereoActivated(0)
    , m_renderStatus(IStereoRenderer::Status::kIdle)
    , m_frontBufWidth(0)
    , m_frontBufHeight(0)
    , m_stereoStrength(0.0f)
    , m_zeroParallaxPlaneDist(0.25f)
    , m_maxSeparationScene(0.0f)
    , m_nearGeoScale(0.0f)
    , m_gammaAdjustment(0)
    , m_screenSize(0.0f)
    , m_resourcesPatched(false)
    , m_needClearLeft(true)
    , m_needClearRight(true)
    , m_hmdRenderer(nullptr)
{
    if (device == STEREO_DEVICE_DEFAULT)
    {
        SelectDefaultDevice();
    }
}


CD3DStereoRenderer::~CD3DStereoRenderer()
{
    Shutdown();
}


void CD3DStereoRenderer::SelectDefaultDevice()
{
    EStereoDevice device = STEREO_DEVICE_NONE;

#if D3DSTEREO_CPP_TRAIT_SELECTDEFAULTDEVICE_STEREODEVICEDRIVER
    device = STEREO_DEVICE_DRIVER;
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSTEREO_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(D3DStereo_cpp)
#elif AZ_TRAIT_OS_PLATFORM_APPLE || defined(AZ_PLATFORM_LINUX)
    device = STEREO_DEVICE_FRAMECOMP;
#endif

    m_device = device;
}


void CD3DStereoRenderer::InitDeviceBeforeD3D()
{
    LOADING_TIME_PROFILE_SECTION;

    if (m_device == STEREO_DEVICE_NONE)
    {
        return;
    }

    bool success = true;
    m_deviceState = STEREO_DEVSTATE_UNSUPPORTED_DEVICE;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSTEREO_CPP_SECTION_2
    #include AZ_RESTRICTED_FILE(D3DStereo_cpp)
#endif

    if (success)
    {
        m_deviceState = STEREO_DEVSTATE_OK;
    }
    else
    {
        iLog->LogError("Failed to create stereo device");
        m_device = STEREO_DEVICE_NONE;
    }
}


void CD3DStereoRenderer::InitDeviceAfterD3D()
{
    LOADING_TIME_PROFILE_SECTION;

    // Note: Resources will be created in EF_Init, so no need to create them here
    AZ::StereoRendererRequestBus::Handler::BusConnect();
}


void CD3DStereoRenderer::CreateResources()
{
    m_SourceSizeParamName = CCryNameR("SourceSize");

    if (!CTexture::s_ptexStereoL || !CTexture::s_ptexStereoR)
    {
        CreateIntermediateBuffers();
    }
}


void CD3DStereoRenderer::CreateIntermediateBuffers()
{
    SAFE_RELEASE(CTexture::s_ptexStereoL);
    SAFE_RELEASE(CTexture::s_ptexStereoR);

    int nWidth = m_renderer.GetWidth();
    int nHeight = m_renderer.GetHeight();

    uint32 nFlags = FT_DONT_STREAM | FT_USAGE_RENDERTARGET;

    CTexture::s_ptexStereoL = CTexture::CreateRenderTarget("$StereoL", nWidth, nHeight, Clr_Empty, eTT_2D, nFlags, eTF_R8G8B8A8);
    CTexture::s_ptexStereoR = CTexture::CreateRenderTarget("$StereoR", nWidth, nHeight, Clr_Empty, eTT_2D, nFlags, eTF_R8G8B8A8);
}

void CD3DStereoRenderer::Shutdown()
{
    AZ::StereoRendererRequestBus::Handler::BusDisconnect();

    ReleaseResources();

    ShutdownHmdRenderer();
}


void CD3DStereoRenderer::ReleaseResources()
{
    if (m_device == STEREO_DEVICE_NONE)
    {
        return;
    }

    SAFE_RELEASE(CTexture::s_ptexStereoL);
    SAFE_RELEASE(CTexture::s_ptexStereoR);
}


bool CD3DStereoRenderer::EnableStereo()
{
    if (!m_hmdRenderer)
    {
        return InitializeHmdRenderer();
    }

    return true;
}


void CD3DStereoRenderer::DisableStereo()
{
    m_gammaAdjustment = 0;

    ShutdownHmdRenderer();
}


void CD3DStereoRenderer::ChangeOutputFormat()
{
    m_frontBufWidth = 0;
    m_frontBufHeight = 0;
}


bool CD3DStereoRenderer::InitializeHmdRenderer()
{
    assert(m_hmdRenderer == nullptr);

    if (AZ::VR::HMDDeviceRequestBus::GetTotalNumOfEventHandlers() <= 0)
    {
        return false;
    }

    m_hmdRenderer = new D3DHMDRenderer();

    if (m_hmdRenderer != nullptr)
    {
        if (!m_hmdRenderer->Initialize(&m_renderer, this))
        {
            SAFE_DELETE(m_hmdRenderer);
            return false;
        }
    }
    return (m_hmdRenderer != nullptr);
}


void CD3DStereoRenderer::ShutdownHmdRenderer()
{
    if (m_hmdRenderer != NULL)
    {
        m_hmdRenderer->Shutdown();
        SAFE_DELETE(m_hmdRenderer);
    }
}


void CD3DStereoRenderer::PrepareStereo(EStereoMode mode, EStereoOutput output)
{
    if (m_mode != mode || m_output != output)
    {
        m_renderer.ForceFlushRTCommands();

        if (m_mode != mode)
        {
            m_mode = mode;
            m_output = output;

            if (mode != STEREO_MODE_NO_STEREO)
            {
                EnableStereo();
                ChangeOutputFormat();
            }
        }
        else if (m_output != output)
        {
            m_output = output;

            if (IsStereoEnabled())
            {
                ChangeOutputFormat();
            }
        }
    }

    if (IsStereoEnabled())
    {
        // VR FIX ME
        // These code was written for 3D TV stereo, not for VR.
        // VR EyeDist concept is the half distance between the eyes, it can be different for 3D TV with reprojection

        // Update stereo parameters
        m_stereoStrength = CRenderer::CV_r_StereoStrength;
        m_maxSeparationScene = CRenderer::CV_r_StereoEyeDist;
        m_zeroParallaxPlaneDist = CRenderer::CV_r_StereoScreenDist;
        m_nearGeoScale = CRenderer::CV_r_StereoNearGeoScale;
        m_gammaAdjustment = CRenderer::CV_r_StereoGammaAdjustment;

        float screenDiagonal = GetScreenDiagonalInInches();
        if (screenDiagonal > 0.0f) // override CVar if we can determine the correct maximum separation
        {
            float aspect = 9.0f / 16.0f;
            float horizontal = screenDiagonal / sqrtf(1.0f + aspect * aspect);
            float typicalEyeSeperation = 2.5f; // In inches
            m_maxSeparationScene = min(typicalEyeSeperation / horizontal, m_maxSeparationScene); // Get bleeding at edges so limit to CVar
        }

        // Apply stereo strength
        m_maxSeparationScene *= m_stereoStrength;

        if (m_hmdRenderer != NULL)
        {
            m_hmdRenderer->PrepareFrame();
        }
    }
}


void CD3DStereoRenderer::HandleNVControl()
{
}


void CD3DStereoRenderer::SetEyeTextures(CTexture* leftTex, CTexture* rightTex)
{
    m_pLeftTex = leftTex;
    m_pRightTex = rightTex;
}


void CD3DStereoRenderer::Update()
{
    if (m_device != STEREO_DEVICE_NONE)
    {
        m_renderer.m_pRT->RC_PrepareStereo(CRenderer::CV_r_StereoMode, CRenderer::CV_r_StereoOutput);
    }
    else
    {
        static int prevMode = 0;
        if (CRenderer::CV_r_StereoMode != prevMode)
        {
            LogWarning("No stereo device enabled, ignoring stereo mode");
            prevMode = CRenderer::CV_r_StereoMode;
        }
    }
}


CCamera CD3DStereoRenderer::PrepareCamera(EStereoEye nEye,  const CCamera& currentCamera, const SRenderingPassInfo& passInfo)
{
    int nThreadID = passInfo.ThreadID();
    CCamera cam = currentCamera;

    if (IsRenderingToHMD())
    {
        AZ::VR::PerEyeCameraInfo cameraInfo;
        EBUS_EVENT(AZ::VR::HMDDeviceRequestBus, GetPerEyeCameraInfo, nEye, cam.GetNearPlane(), cam.GetFarPlane(), cameraInfo);

        const float asymmetricHorizontalTranslation = cameraInfo.frustumPlane.horizontalDistance * cam.GetNearPlane();
        const float asymmetricVerticalTranslation = cameraInfo.frustumPlane.verticalDistance * cam.GetNearPlane();

        Matrix34 stereoMat = Matrix34::CreateTranslationMat(AZVec3ToLYVec3(cameraInfo.eyeOffset));
        cam.SetMatrix(cam.GetMatrix() * stereoMat);
        cam.SetFrustum(1, 1, cameraInfo.fov, cam.GetNearPlane(), cam.GetFarPlane(), 1.0f / cameraInfo.aspectRatio);
        cam.SetAsymmetry(asymmetricHorizontalTranslation, asymmetricHorizontalTranslation, asymmetricVerticalTranslation, asymmetricVerticalTranslation);
    }
    else
    {
        float fNear = cam.GetNearPlane();
        float screenDist = CRenderer::CV_r_StereoScreenDist;
        float z = 99999.0f;  // Point which is far away

        // standard 3D TV stereo projection parameters
        float wT = tanf(cam.GetFov() * 0.5f) * fNear;
        float wR = wT * cam.GetProjRatio(), wL = -wR;
        float p00 = 2 * fNear / (wR - wL);
        float p02 = (wL + wR) / (wL - wR);

        // Compute required camera shift, so that a distant point gets the desired maximum separation
        const float maxSeparation = CRenderer::CV_r_StereoEyeDist;
        const float camOffset = (maxSeparation - p02) / (p00 / z - p00 / screenDist);
        float frustumShift = camOffset * (fNear / screenDist);
        //frustumShift *= (nEye == STEREO_EYE_LEFT ? 1 : -1);  // Support postive and negative parallax for non-VR stereo

        Matrix34 stereoMat = Matrix34::CreateTranslationMat(Vec3(nEye == STEREO_EYE_LEFT ? -camOffset : camOffset, 0, 0));
        cam.SetMatrix(cam.GetMatrix() * stereoMat);
        cam.SetAsymmetry(frustumShift, frustumShift, 0, 0);
    }

    return cam;
}


void CD3DStereoRenderer::ProcessScene(int sceneFlags, const SRenderingPassInfo& passInfo)
{
    int nThreadID = passInfo.ThreadID();

    //for recursive rendering (e.g. rendering to ocean reflection texture), stereo is not needed
    //We also don't need stereo rendering if we're not rendering on the main viewport (don't check if main viewport when in the launcher)
    if (CRenderer::CV_r_StereoMode == STEREO_MODE_DUAL_RENDERING && SRendItem::m_RecurseLevel[nThreadID] < 1
        && (!gEnv->IsEditor() || gcpRendD3D->m_CurrContext->m_bMainViewport))      
    {
        CCamera cam = m_renderer.m_RP.m_TI[nThreadID].m_cam;

        // Left eye
        {
            m_renderStatus = IStereoRenderer::Status::kRenderingFirstEye;
            m_renderer.m_pRT->RC_SetStereoEye(STEREO_EYE_LEFT);
            m_renderer.PushProfileMarker("LEFT_EYE");

            CCamera camL = PrepareCamera(STEREO_EYE_LEFT, cam, passInfo);
            m_renderer.SetCamera(camL);

            CD3DStereoRenderer::RenderScene(sceneFlags, passInfo);

            CopyToStereoFromMainThread(STEREO_EYE_LEFT);
            m_renderer.PopProfileMarker("LEFT_EYE");
        }

        // Right eye
        {
            m_renderStatus = IStereoRenderer::Status::kRenderingSecondEye;
            m_renderer.m_pRT->RC_SetStereoEye(STEREO_EYE_RIGHT);
            m_renderer.PushProfileMarker("RIGHT_EYE");

            CCamera camR = PrepareCamera(STEREO_EYE_RIGHT, cam, passInfo);
            m_renderer.SetCamera(camR);

            CD3DStereoRenderer::RenderScene(sceneFlags | SHDF_NO_SHADOWGEN, passInfo);
            CopyToStereoFromMainThread(STEREO_EYE_RIGHT);

            m_renderer.PopProfileMarker("RIGHT_EYE");
        }

        m_renderStatus = IStereoRenderer::Status::kIdle;
    }
    else
    {
        if (CRenderer::CV_r_StereoMode != STEREO_MODE_DUAL_RENDERING)
        {
            m_renderer.m_pRT->RC_SetStereoEye(0);
        }

        CD3DStereoRenderer::RenderScene(sceneFlags, passInfo);
    }
}


void CD3DStereoRenderer::CopyToStereo(int channel)
{
    assert(IsRenderThread());

    PROFILE_LABEL_SCOPE("COPY_TO_STEREO");

    CTexture* pTex;

    if (channel == STEREO_EYE_LEFT)
    {
        pTex = GetLeftEye();
        m_needClearLeft = false;
    }
    else
    {
        pTex = GetRightEye();
        m_needClearRight = false;
    }

    if (pTex == NULL)
    {
        return;
    }

    GetUtils().CopyScreenToTexture(pTex);

    CTexture* finalCompositeSource = GetUtils().GetFinalCompositeTarget();
    if (gcpRendD3D->FX_GetCurrentRenderTarget(0) == finalCompositeSource && finalCompositeSource != nullptr)
    {
        gcpRendD3D->FX_PopRenderTarget(0);
    }
}


void CD3DStereoRenderer::DisplayStereo()
{
    assert(IsRenderThread());

    if (!IsStereoEnabled() || m_renderer.m_bDeviceLost)  // When unloading level, m_bDeviceLost is set to 2
    {
        return;
    }

    if (m_hmdRenderer != NULL)
    {
        m_hmdRenderer->RenderSocialScreen();
        m_hmdRenderer->SubmitFrame();
        return;
    }

    ResolveStereoBuffers();
    m_needClearLeft = true;
    m_needClearRight = true;

    m_renderer.m_cEF.mfRefreshSystemShader("Stereo", CShaderMan::s_ShaderStereo);

    PROFILE_LABEL_SCOPE("DISPLAY_STEREO");

    // TODO: Fix this properly
    if (!gEnv->IsEditor())
    {
        m_renderer.RT_SetViewport(0, 0, m_renderer.GetBackbufferWidth(), m_renderer.GetBackbufferHeight());
    }

    if (m_renderer.m_pSecondBackBuffer != 0)
    {
        m_renderer.FX_PushRenderTarget(1, m_renderer.m_pSecondBackBuffer, NULL);
        m_renderer.RT_SetViewport(0, 0, m_renderer.m_NewViewport.nWidth, m_renderer.m_NewViewport.nHeight);
    }

    CShader* pSH = m_renderer.m_cEF.s_ShaderStereo;
    SelectShaderTechnique();

    uint32 nPasses = 0;
    pSH->FXBegin(&nPasses, FEF_DONTSETSTATES);
    pSH->FXBeginPass(0);

    m_renderer.FX_SetState(GS_NODEPTHTEST);

    int width = m_renderer.GetWidth();
    int height = m_renderer.GetHeight();

    Vec4 pParams = Vec4((float) width, (float) height, 0, 0);
    CShaderMan::s_shPostEffects->FXSetPSFloat(m_SourceSizeParamName, &pParams, 1);

    GetUtils().SetTexture(!CRenderer::CV_r_StereoFlipEyes ? GetLeftEye() : GetRightEye(), 0, FILTER_LINEAR);
    GetUtils().SetTexture(!CRenderer::CV_r_StereoFlipEyes ? GetRightEye() : GetLeftEye(), 1, FILTER_LINEAR);

    GetUtils().DrawFullScreenTri(width, height);

    pSH->FXEndPass();
    pSH->FXEnd();

    if (m_renderer.m_pSecondBackBuffer != 0)
    {
        m_renderer.FX_PopRenderTarget(1);
    }
}


void CD3DStereoRenderer::BeginRenderingMRT(bool disableClear)
{
    if (!IsStereoEnabled())
    {
        return;
    }

    if (disableClear)
    {
        m_needClearLeft = false;
        m_needClearRight = false;
    }

    PushRenderTargets();
}


void CD3DStereoRenderer::EndRenderingMRT(bool bResolve)
{
    if (!IsStereoEnabled())
    {
        return;
    }

    PopRenderTargets(bResolve);
}


void CD3DStereoRenderer::TakeScreenshot(const char path[])
{
    stack_string pathL(path);
    stack_string pathR(path);

    pathL.insert(pathL.find('.'), "_L");
    pathR.insert(pathR.find('.'), "_R");

    ((CD3D9Renderer*) gRenDev)->CaptureFrameBufferToFile(pathL.c_str(), GetLeftEye());
    ((CD3D9Renderer*) gRenDev)->CaptureFrameBufferToFile(pathR.c_str(), GetRightEye());
}


void CD3DStereoRenderer::ResolveStereoBuffers()
{
}

IStereoRenderer::Status ConvertFromEyeToStatus(const EStereoEye eye)
{
    if (eye == STEREO_EYE_LEFT)
    {
        return IStereoRenderer::Status::kRenderingFirstEye;
    }
    else if (eye == STEREO_EYE_RIGHT)
    {
        return IStereoRenderer::Status::kRenderingSecondEye;
    }

    return IStereoRenderer::Status::kIdle;
}

void CD3DStereoRenderer::BeginRenderingTo(EStereoEye eye)
{
    m_renderStatus = ConvertFromEyeToStatus(eye);

    if (eye == STEREO_EYE_LEFT)
    {
        gRenDev->SetProfileMarker("LEFT_EYE", CRenderer::ESPM_PUSH);
        if (m_needClearLeft)
        {
            m_renderer.FX_ClearTarget(GetLeftEye(), Clr_Transparent);
            m_needClearLeft = false;
        }
        m_renderer.FX_PushRenderTarget(0, GetLeftEye(), &m_renderer.m_DepthBufferOrig);
    }
    else
    {
        gRenDev->SetProfileMarker("RIGHT_EYE", CRenderer::ESPM_PUSH);
        if (m_needClearRight)
        {
            m_renderer.FX_ClearTarget(GetRightEye(), Clr_Transparent);
            m_needClearRight = false;
        }
        m_renderer.FX_PushRenderTarget(0, GetRightEye(), &m_renderer.m_DepthBufferOrig);
    }

    m_renderer.FX_SetActiveRenderTargets();

    if (eye == STEREO_EYE_LEFT)
    {
        GetLeftEye()->SetResolved(true);
    }
    else
    {
        GetRightEye()->SetResolved(true);
    }
}


void CD3DStereoRenderer::EndRenderingTo(EStereoEye eye)
{
    if (eye == STEREO_EYE_LEFT)
    {
        gRenDev->SetProfileMarker("LEFT_EYE", CRenderer::ESPM_POP);
        GetLeftEye()->SetResolved(true);
        m_renderer.FX_PopRenderTarget(0);
    }
    else
    {
        gRenDev->SetProfileMarker("RIGHT_EYE", CRenderer::ESPM_POP);
        GetRightEye()->SetResolved(true);
        m_renderer.FX_PopRenderTarget(0);
    }
}


void CD3DStereoRenderer::SelectShaderTechnique()
{
    gRenDev->m_cEF.mfRefreshSystemShader("Stereo", CShaderMan::s_ShaderStereo);

    CShader* pSH = m_renderer.m_cEF.s_ShaderStereo;

    if (m_device == STEREO_DEVICE_FRAMECOMP)
    {
        switch (GetStereoOutput())
        {
        case STEREO_OUTPUT_CHECKERBOARD:
            pSH->FXSetTechnique("Checkerboard");
            break;
        case STEREO_OUTPUT_SIDE_BY_SIDE:
            pSH->FXSetTechnique("SideBySide");
            break;
        case STEREO_OUTPUT_LINE_BY_LINE:
            pSH->FXSetTechnique("LineByLine");
            break;
    #ifndef _RELEASE
        case STEREO_OUTPUT_ANAGLYPH:
            pSH->FXSetTechnique("Anaglyph");
            break;
    #endif
        default:
            pSH->FXSetTechnique("Emulation");
        }
    }
    else if (IsDriver(DRIVER_NV))
    {
        pSH->FXSetTechnique("NV3DVision");
    }
    else if (m_device == STEREO_DEVICE_DUALHEAD)
    {
        switch (GetStereoOutput())
        {
        case STEREO_OUTPUT_STANDARD:
            pSH->FXSetTechnique("DualHead");
            break;
        case STEREO_OUTPUT_IZ3D:
            pSH->FXSetTechnique("IZ3D");
            break;
        default:
            pSH->FXSetTechnique("Emulation");
        }
    }
    else
    {
        pSH->FXSetTechnique("Emulation");
    }
}


void CD3DStereoRenderer::RenderScene(int sceneFlags, const SRenderingPassInfo& passInfo)
{
    m_renderer.EF_Scene3D(m_renderer.m_MainRTViewport, sceneFlags, passInfo);
}


bool CD3DStereoRenderer::IsRenderThread() const
{
    return m_renderer.m_pRT->IsRenderThread();
}


void CD3DStereoRenderer::CopyToStereoFromMainThread(int channel)
{
    m_renderer.m_pRT->RC_CopyToStereoTex(channel);
}


void CD3DStereoRenderer::PushRenderTargets()
{
    if (m_needClearLeft)
    {
        m_needClearLeft = false;
        m_renderer.FX_ClearTarget(GetLeftEye(), Clr_Transparent);
    }

    if (m_needClearRight)
    {
        m_needClearRight = false;
        m_renderer.FX_ClearTarget(GetRightEye(), Clr_Transparent);
    }

    m_renderer.FX_PushRenderTarget(0, GetLeftEye(),  &m_renderer.m_DepthBufferOrig);
    m_renderer.FX_PushRenderTarget(1, GetRightEye(), NULL);

    m_renderer.RT_SetViewport(0, 0, GetLeftEye()->GetWidth(), GetLeftEye()->GetHeight());

    m_renderer.FX_SetActiveRenderTargets();
}


void CD3DStereoRenderer::PopRenderTargets([[maybe_unused]] bool bResolve)
{
    m_renderer.FX_PopRenderTarget(1);
    m_renderer.FX_PopRenderTarget(0);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// IStereoRenderer Interface
////////////////////////////////////////////////////////////////////////////////////////////////////

EStereoDevice CD3DStereoRenderer::GetDevice()
{
    return m_device;
}


EStereoDeviceState CD3DStereoRenderer::GetDeviceState()
{
    return m_deviceState;
}


void CD3DStereoRenderer::GetInfo(EStereoDevice* device, EStereoMode* mode, EStereoOutput* output, EStereoDeviceState* state) const
{
    if (device)
    {
        *device = m_device;
    }
    if (mode)
    {
        *mode = m_mode;
    }
    if (output)
    {
        *output = m_output;
    }
    if (state)
    {
        *state = m_deviceState;
    }
}


bool CD3DStereoRenderer::GetStereoEnabled()
{
    return IsStereoEnabled();
}


float CD3DStereoRenderer::GetStereoStrength()
{
    return m_stereoStrength;
}


float CD3DStereoRenderer::GetMaxSeparationScene(bool half)
{
    return m_maxSeparationScene * (half ? 0.5f : 1.0f);
}


float CD3DStereoRenderer::GetZeroParallaxPlaneDist()
{
    return m_zeroParallaxPlaneDist;
}


void CD3DStereoRenderer::GetNVControlValues(bool& stereoActivated, float& stereoStrength)
{
    stereoActivated = m_nvStereoActivated != 0;
    stereoStrength = m_nvStereoStrength;
}


void CD3DStereoRenderer::ReleaseBuffers()
{
}

void CD3DStereoRenderer::OnResolutionChanged()
{
    // StereoL and StereoR buffers are used as temporary buffers in other passes and always required
    CreateIntermediateBuffers();

    if (m_device == STEREO_DEVICE_NONE)
    {
        return;
    }

    if (m_hmdRenderer != NULL)
    {
        m_hmdRenderer->OnResolutionChanged();
    }
}


void CD3DStereoRenderer::CalculateBackbufferResolution(int eyeWidth, int eyeHeight, int& backbufferWidth, int& backbufferHeight)
{
    if (m_hmdRenderer != NULL)
    {
        m_hmdRenderer->CalculateBackbufferResolution(eyeWidth, eyeHeight, backbufferWidth, backbufferHeight);
    }
    else
    {
        switch (m_output)
        {
        case STEREO_OUTPUT_SIDE_BY_SIDE:
            backbufferWidth  = eyeWidth * 2;
            backbufferHeight = eyeHeight;
            break;
        case STEREO_OUTPUT_ABOVE_AND_BELOW:
            backbufferWidth = eyeWidth;
            backbufferHeight = eyeHeight * 2;
            break;
        default:
            backbufferWidth = eyeWidth;
            backbufferHeight = eyeHeight;
            break;
        }
    }
}


void CD3DStereoRenderer::OnHmdDeviceChanged()
{
    if (m_hmdRenderer)
    {
        ShutdownHmdRenderer();
        InitializeHmdRenderer();
    }
}

bool CD3DStereoRenderer::IsRenderingToHMD()
{
    bool valid = false;
    if (m_hmdRenderer)
    {
        // If the renderer is valid, make sure that we're actually supposed to be rendering to the HMD.

        EStereoDevice device = STEREO_DEVICE_NONE;
        EStereoMode mode     = STEREO_MODE_NO_STEREO;
        EStereoOutput output = STEREO_OUTPUT_STANDARD;

        GetInfo(&device, &mode, &output, nullptr);

        const bool hmdSelected = (output == STEREO_OUTPUT_HMD);
        if (hmdSelected)
        {
            // Make sure the mode and device are set to use the HMD.
            valid = (mode == STEREO_MODE_DUAL_RENDERING);
        }
    }

    return valid;
}


