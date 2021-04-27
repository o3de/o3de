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

// Description : Implementation of the NULL renderer API

#include "CryRenderOther_precompiled.h"
#include "AtomShim_Renderer.h"
#include <IColorGradingController.h>
#include "IStereoRenderer.h"
#include "../Common/Textures/TextureManager.h"

#include <IEngineModule.h>
#include <CryExtension/Impl/ClassWeaver.h>
// init memory pool usage

#include "GraphicsPipeline/FurBendData.h"

#include <AzFramework/Render/RenderSystemBus.h>
#include <MathConversion.h>

#include <AzCore/Math/MatrixUtils.h>

#include <Atom/RHI/Factory.h>

#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

#include <AzFramework/Asset/AssetSystemBus.h>

#include <random>  //std::random_device

CCryNameTSCRC CTexture::s_sClassName = CCryNameTSCRC("CTexture");
CCryNameTSCRC CHWShader::s_sClassNameVS = CCryNameTSCRC("CHWShader_VS");
CCryNameTSCRC CHWShader::s_sClassNamePS = CCryNameTSCRC("CHWShader_PS");
CCryNameTSCRC CShader::s_sClassName = CCryNameTSCRC("CShader");

CAtomShimRenderer* gcpAtomShim = NULL;

 #ifdef _DEBUG
// static array used to check that calls to Set2DMode and Unset2DMode are matched. (static array initialized to zeros automatically).
int s_isIn2DMode[RT_COMMAND_BUF_COUNT];
#endif

//////////////////////////////////////////////////////////////////////

class CNullColorGradingController
    : public IColorGradingController
{
public:
    virtual int LoadColorChart([[maybe_unused]] const char* pChartFilePath) const { return 0; }
    virtual int LoadDefaultColorChart() const { return 0; }
    virtual void UnloadColorChart([[maybe_unused]] int texID) const {}
    virtual void SetLayers([[maybe_unused]] const SColorChartLayer* pLayers, [[maybe_unused]] uint32 numLayers) {}
};

//////////////////////////////////////////////////////////////////////

class CNullStereoRenderer
    : public IStereoRenderer
{
public:
    virtual EStereoDevice GetDevice() { return STEREO_DEVICE_NONE; }
    virtual EStereoDeviceState GetDeviceState() { return STEREO_DEVSTATE_UNSUPPORTED_DEVICE; }
    virtual void GetInfo(EStereoDevice* device, EStereoMode* mode, EStereoOutput* output, EStereoDeviceState* state) const
    {
        if (device)
        {
            *device = STEREO_DEVICE_NONE;
        }
        if (mode)
        {
            *mode = STEREO_MODE_NO_STEREO;
        }
        if (output)
        {
            *output = STEREO_OUTPUT_STANDARD;
        }
        if (state)
        {
            *state = STEREO_DEVSTATE_OK;
        }
    }
    virtual bool GetStereoEnabled() { return false; }
    virtual float GetStereoStrength() { return 0; }
    virtual float GetMaxSeparationScene([[maybe_unused]] bool half = true) { return 0; }
    virtual float GetZeroParallaxPlaneDist() { return 0; }
    virtual void GetNVControlValues([[maybe_unused]] bool& stereoEnabled, [[maybe_unused]] float& stereoStrength) {};
    virtual void OnHmdDeviceChanged() {}
    virtual bool IsRenderingToHMD() override { return false; }
    Status GetStatus() const override { return IStereoRenderer::Status::kIdle; }
};

//////////////////////////////////////////////////////////////////////
CAtomShimRenderer::CAtomShimRenderer()
{
    gcpAtomShim = this;
    m_pAtomShimRenderAuxGeom = CAtomShimRenderAuxGeom::Create(*this);
    m_pAtomShimColorGradingController = new CNullColorGradingController();
    m_pAtomShimStereoRenderer = new CNullStereoRenderer();
    m_pixelAspectRatio = 1.0f;
    Camera::ActiveCameraRequestBus::Handler::BusConnect();
}

//////////////////////////////////////////////////////////////////////////
bool QueryIsFullscreen()
{
    return false;
}


#include <stdio.h>

namespace Platform
{
    WIN_HWND GetNativeWindowHandle();
}

//////////////////////////////////////////////////////////////////////
CAtomShimRenderer::~CAtomShimRenderer()
{
    Camera::ActiveCameraRequestBus::Handler::BusDisconnect();
    ShutDown();
    delete m_pAtomShimRenderAuxGeom;
    delete m_pAtomShimColorGradingController;
    delete m_pAtomShimStereoRenderer;
}

//////////////////////////////////////////////////////////////////////
void CAtomShimRenderer::EnableTMU([[maybe_unused]] bool enable)
{
}

//////////////////////////////////////////////////////////////////////
void CAtomShimRenderer::CheckError([[maybe_unused]] const char* comment)
{
}

//////////////////////////////////////////////////////////////////////
void CAtomShimRenderer::BeginFrame()
{
    if (!m_isFinalInitializationDone)
    {
        // This will cause the default textures (such as the White texture) to be loaded. In legacy renderer it is called in CRenderer::PostInit
        // but that is disabled for AtomShim because NULL_RENDERER is defined. Anyway, it would not work if we called it there because the Asset Catalog
        // is not yet loaded when CRenderer::PostInit is called and we use it to load Atom textures.
        // [GFX TODO] Do we want NULL_RENDERER defined for AtomShim? It would affect a lot of code in AtomShim if we removed that define.
        InitSystemResources(FRR_SYSTEM_RESOURCES);

        // In the legacy renderer this is done in CRenderer::PostInit but that is only done is NULL_RENDERER is not defined. 
        if (gEnv->pCryFont)
        {
            m_pDefaultFont = gEnv->pCryFont->GetFont("default");
            if (!m_pDefaultFont)
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Error getting default font");
            }
        }

        AZ::Name apiName = AZ::RHI::Factory::Get().GetName();
        if (!apiName.IsEmpty())
        {
            m_rendererDescription = AZStd::string::format("Atom using %s RHI", apiName.GetCStr());
        }

        // Initialize dynamic draw which is used for 2d drawing
        const char* shaderFilepath = "Shaders/SimpleTextured.azshader";
        m_dynamicDraw = AZ::RPI::DynamicDrawInterface::Get()->CreateDynamicDrawContext(
            AZ::RPI::RPISystemInterface::Get()->GetDefaultScene().get());
        AZ::Data::Instance<AZ::RPI::Shader> shader = AZ::RPI::LoadShader(shaderFilepath);
        m_dynamicDraw->InitShader(shader);
        m_dynamicDraw->InitVertexFormat(
            {{"POSITION", AZ::RHI::Format::R32G32B32_FLOAT},
             {"COLOR", AZ::RHI::Format::R8G8B8A8_UNORM},
             {"TEXCOORD0", AZ::RHI::Format::R32G32_FLOAT}});
        // enable the ability to change cull mode, blend mode, the depth state
        m_dynamicDraw->AddDrawStateOptions( AZ::RPI::DynamicDrawContext::DrawStateOptions::BlendMode
            | AZ::RPI::DynamicDrawContext::DrawStateOptions::PrimitiveType
            | AZ::RPI::DynamicDrawContext::DrawStateOptions::DepthState
            | AZ::RPI::DynamicDrawContext::DrawStateOptions::FaceCullMode);
        m_dynamicDraw->EndInit();

        // declare the two shader variants it will use
        AZ::RPI::ShaderOptionList shaderOptionsClamp;
        shaderOptionsClamp.push_back(AZ::RPI::ShaderOption(AZ::Name("o_useColorChannels"), AZ::Name("true")));
        shaderOptionsClamp.push_back(AZ::RPI::ShaderOption(AZ::Name("o_clamp"), AZ::Name("true")));
        m_shaderVariantClamp = m_dynamicDraw->UseShaderVariant(shaderOptionsClamp);
        AZ::RPI::ShaderOptionList shaderOptionsWrap;
        shaderOptionsWrap.push_back(AZ::RPI::ShaderOption(AZ::Name("o_useColorChannels"), AZ::Name("true")));
        shaderOptionsWrap.push_back(AZ::RPI::ShaderOption(AZ::Name("o_clamp"), AZ::Name("false")));
        m_shaderVariantWrap = m_dynamicDraw->UseShaderVariant(shaderOptionsWrap);

        m_dynamicDraw->NewDrawSrg();

        m_isFinalInitializationDone = true;
    }

    if (m_isInFrame)
    {
        // If there has not been an EndFrame since the latest BeginFrame then ignore this call to BeginFrame.
        return;
    }

    m_isInFrame = true;

    m_RP.m_TI[m_RP.m_nFillThreadID].m_nFrameID++;
    m_RP.m_TI[m_RP.m_nFillThreadID].m_nFrameUpdateID++;
    m_RP.m_TI[m_RP.m_nFillThreadID].m_RealTime = iTimer->GetCurrTime();

    m_RP.m_TI[m_RP.m_nFillThreadID].m_matView.SetIdentity();
    m_RP.m_TI[m_RP.m_nFillThreadID].m_matProj.SetIdentity();

    m_pAtomShimRenderAuxGeom->BeginFrame();
}

//////////////////////////////////////////////////////////////////////
bool CAtomShimRenderer::ChangeDisplay([[maybe_unused]] unsigned int width, [[maybe_unused]] unsigned int height, [[maybe_unused]] unsigned int bpp)
{
    return false;
}

//////////////////////////////////////////////////////////////////////
void CAtomShimRenderer::ChangeViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height, bool bMainViewport, float scaleWidth, float scaleHeight)
{
    float fWidth = aznumeric_cast<float>(width);
    float fHeight = aznumeric_cast<float>(height);

    width = aznumeric_cast<unsigned int>(fWidth * scaleWidth);
    height = aznumeric_cast<unsigned int>(fHeight * scaleHeight);

    m_MainRTViewport.nX = x;
    m_MainRTViewport.nY = y;
    m_MainRTViewport.nWidth = width;
    m_MainRTViewport.nHeight = height;

    m_width = m_nativeWidth = m_backbufferWidth = width;
    m_height = m_nativeHeight = m_backbufferHeight = height;

    if (m_currContext)
    {
        m_currContext->m_width = width;
        m_currContext->m_height = height;
        m_currContext->m_isMainViewport = bMainViewport;
    }
}

void CAtomShimRenderer::RenderDebug([[maybe_unused]] bool bRenderStats)
{
#if !defined(_RELEASE)
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

void CAtomShimRenderer::EndFrame()
{
    if (!m_isInFrame)
    {
        // If there has not been a BeginFrame since the latest EndFrame then ignore this call to EndFrame.
        // This can happen when EndFrame is called from UnloadLevel.
        return;
    }

    m_pAtomShimRenderAuxGeom->EndFrame();

    EF_RenderTextMessages();

    // Hack: Assume we're just rendering to the default ViewContext
    // Proper multi viewport support will be handled after this shim is removed
    if (!m_viewportContext)
    {
        auto viewContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        auto viewportContext = viewContextManager->GetViewportContextByName(viewContextManager->GetDefaultViewportContextName());
        // If the viewportContext exists and is created with the default ID, we can safely assume control
        if (viewportContext && viewportContext->GetId() == -10)
        {
            m_viewportContext = viewportContext;
        }
    }

    if (m_viewportContext)
    {
        m_viewportContext->SetRenderScene(AZ::RPI::RPISystemInterface::Get()->GetDefaultScene());
        m_viewportContext->RenderTick();
    }

    m_isInFrame = false;
}

void CAtomShimRenderer::TryFlush()
{
}

void CAtomShimRenderer::GetMemoryUsage([[maybe_unused]] ICrySizer* Sizer)
{
}

WIN_HWND CAtomShimRenderer::GetHWND()
{
    return Platform::GetNativeWindowHandle();
}

bool CAtomShimRenderer::SetWindowIcon([[maybe_unused]] const char* path)
{
    return false;
}

ERenderType CAtomShimRenderer::GetRenderType() const
{
    return eRT_Undefined;
}

const char* CAtomShimRenderer::GetRenderDescription() const
{
    return m_rendererDescription.c_str();
}

void TexBlurAnisotropicVertical([[maybe_unused]] CTexture* pTex, [[maybe_unused]] int nAmount, [[maybe_unused]] float fScale, [[maybe_unused]] float fDistribution, [[maybe_unused]] bool bAlphaOnly)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//IMAGES DRAWING
////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void CAtomShimRenderer::Draw2dImage([[maybe_unused]] float xpos, [[maybe_unused]] float ypos, [[maybe_unused]] float w, [[maybe_unused]] float h, [[maybe_unused]] int texture_id, [[maybe_unused]] float s0, [[maybe_unused]] float t0, [[maybe_unused]] float s1, [[maybe_unused]] float t1, [[maybe_unused]] float angle, [[maybe_unused]] float r, [[maybe_unused]] float g, [[maybe_unused]] float b, [[maybe_unused]] float a, [[maybe_unused]] float z)
{
}

//////////////////////////////////////////////////////////////////////
void CAtomShimRenderer::Push2dImage([[maybe_unused]] float xpos, [[maybe_unused]] float ypos, [[maybe_unused]] float w, [[maybe_unused]] float h, [[maybe_unused]] int texture_id, [[maybe_unused]] float s0, [[maybe_unused]] float t0, [[maybe_unused]] float s1, [[maybe_unused]] float t1, [[maybe_unused]] float angle, [[maybe_unused]] float r, [[maybe_unused]] float g, [[maybe_unused]] float b, [[maybe_unused]] float a, [[maybe_unused]] float z, [[maybe_unused]] float stereoDepth)
{
}

void CAtomShimRenderer::Draw2dImageList()
{
}

//////////////////////////////////////////////////////////////////////
void CAtomShimRenderer::DrawImage(float xpos, float ypos, float w, float h, int texture_id, float s0, float t0, float s1, float t1, float r, float g, float b, float a, bool filtered)
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

///////////////////////////////////////////
void CAtomShimRenderer::DrawImageWithUV(float xpos, float ypos, float z, float w, float h, int texture_id, float s[4], float t[4], float r, float g, float b, float a, bool filtered)
{
    SetCullMode(R_CULL_DISABLE);
    EF_SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
    EF_SetSrgbWrite(false);

    DWORD col = D3DRGBA(r, g, b, a);

    SVF_P3F_C4B_T2F vQuad[4];

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

    STexState TS;
    TS.SetFilterMode(filtered ? FILTER_BILINEAR : FILTER_POINT);
    TS.SetClampMode(1, 1, 1);
    SetTexture(texture_id);

    DrawDynVB(vQuad, nullptr, 4, 0, prtTriangleStrip);
}

///////////////////////////////////////////
void CAtomShimRenderer::DrawBuffer([[maybe_unused]] CVertexBuffer* pVBuf, [[maybe_unused]] CIndexBuffer* pIBuf, [[maybe_unused]] int nNumIndices, [[maybe_unused]] int nOffsIndex, [[maybe_unused]] const PublicRenderPrimitiveType nPrmode, [[maybe_unused]] int nVertStart, [[maybe_unused]] int nVertStop)
{
}

///////////////////////////////////////////
void CAtomShimRenderer::DrawPrimitivesInternal([[maybe_unused]] CVertexBuffer* src, [[maybe_unused]] int vert_num, [[maybe_unused]] const eRenderPrimitiveType prim_type)
{
}

///////////////////////////////////////////
void CRenderMesh::DrawImmediately()
{
}

///////////////////////////////////////////
void CAtomShimRenderer::SetCullMode(int mode)
{
    AZ::RHI::CullMode cullMode = AZ::RHI::CullMode::None;
    switch (mode)
    {
    case R_CULL_FRONT:
        cullMode = AZ::RHI::CullMode::Front;
        break;
    case R_CULL_BACK:
        cullMode = AZ::RHI::CullMode::Back;
        break;
    }
    m_dynamicDraw->SetCullMode(cullMode);
}

///////////////////////////////////////////
bool CAtomShimRenderer::EnableFog([[maybe_unused]] bool enable)
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//MISC EXTENSIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////
void CAtomShimRenderer::EnableVSync([[maybe_unused]] bool enable)
{
}

//////////////////////////////////////////////////////////////////////
void CAtomShimRenderer::SelectTMU([[maybe_unused]] int tnum)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//MATRIX FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////
void CAtomShimRenderer::PushMatrix()
{
}

///////////////////////////////////////////
void CAtomShimRenderer::RotateMatrix([[maybe_unused]] float a, [[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] float z)
{
}

void CAtomShimRenderer::RotateMatrix([[maybe_unused]] const Vec3& angles)
{
}

///////////////////////////////////////////
void CAtomShimRenderer::TranslateMatrix([[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] float z)
{
}

void CAtomShimRenderer::MultMatrix([[maybe_unused]] const float* mat)
{
}

void CAtomShimRenderer::TranslateMatrix([[maybe_unused]] const Vec3& pos)
{
}

///////////////////////////////////////////
void CAtomShimRenderer::ScaleMatrix([[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] float z)
{
}

///////////////////////////////////////////
void CAtomShimRenderer::PopMatrix()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAtomShimRenderer::LoadMatrix([[maybe_unused]] const Matrix34* src)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//MISC
////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////
void CAtomShimRenderer::PushWireframeMode([[maybe_unused]] int mode){}
void CAtomShimRenderer::PopWireframeMode(){}
void CAtomShimRenderer::FX_PushWireframeMode([[maybe_unused]] int mode){}
void CAtomShimRenderer::FX_PopWireframeMode(){}
void CAtomShimRenderer::FX_SetWireframeMode([[maybe_unused]] int mode){}

///////////////////////////////////////////
void CAtomShimRenderer::SetCamera(const CCamera& cam)
{
    CacheCameraConfiguration(cam);
    CacheCameraTransform(cam);

    int nThreadID = m_pRT->GetThreadList();

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

    viewParameters.Frustum(wL + cam.GetAsymL(), wR + cam.GetAsymR(), wB + cam.GetAsymB(), wT + cam.GetAsymT(), Near, Far);

    Vec3 vEye = cam.GetPosition();
    Vec3 vAt = vEye + Vec3((f32)mCam34(0, 1), (f32)mCam34(1, 1), (f32)mCam34(2, 1));
    Vec3 vUp = Vec3((f32)mCam34(0, 2), (f32)mCam34(1, 2), (f32)mCam34(2, 2));
    viewParameters.LookAt(vEye, vAt, vUp);
    ApplyViewParameters(viewParameters);

    // Set the Atom view for the context to match the given camera
    {
        AZ::RPI::ViewPtr viewForCurrentContext;

        // If we have a current context (which we have in Editor but not yet in launcher) then use the view from that.
        // Otherwise use the default view from the default scene.
        if (m_currContext && m_currContext->m_view)
        {
            viewForCurrentContext = m_currContext->m_view;
        }
        else
        {
            AZ::RPI::ScenePtr scene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
            AZ::RPI::RenderPipelinePtr renderPipeline = scene->GetDefaultRenderPipeline();
            if (renderPipeline)
            {
                viewForCurrentContext = renderPipeline->GetDefaultView();
            }
        }

        if (viewForCurrentContext)
        {
            // Set camera to world transform for view
            AZ::Matrix3x4 cameraWorldTransform = LYTransformToAZMatrix3x4(cam.GetMatrix());
            viewForCurrentContext->SetCameraTransform(cameraWorldTransform);

            // Set projection transform for view
            // [GFX TODO] [ATOM-1501] Currently we always assume reverse depth
            float fov = cam.GetFov();
            float aspectRatio = cam.GetProjRatio();
            float nearPlane = cam.GetNearPlane();
            float farPlane = cam.GetFarPlane();
            AZ::Matrix4x4 viewToClipMatrix;
            AZ::MakePerspectiveFovMatrixRH(viewToClipMatrix, fov, aspectRatio, nearPlane, farPlane, true);
            viewForCurrentContext->SetViewToClipMatrix(viewToClipMatrix);
        }
    }
}

void CAtomShimRenderer::GetViewport(int* x, int* y, int* width, int* height) const
{
    const SViewport& vp = m_MainRTViewport;
    *x = vp.nX;
    *y = vp.nY;
    *width = vp.nWidth;
    *height = vp.nHeight;
}

void CAtomShimRenderer::SetViewport(int x, int y, int width, int height, [[maybe_unused]] int id)
{
    m_MainRTViewport.nX = x;
    m_MainRTViewport.nY = y;
    m_MainRTViewport.nWidth = width;
    m_MainRTViewport.nHeight = height;

    m_width = width;
    m_height = height;
}

void CAtomShimRenderer::SetScissor(int x, int y, int width, int height)
{
    m_dynamicDraw->SetScissor(AZ::RHI::Scissor(x, y, x + width, y + height));
}

//////////////////////////////////////////////////////////////////////
void CAtomShimRenderer::GetModelViewMatrix(float* mat)
{
    int nThreadID = m_pRT->GetThreadList();
    *(Matrix44*)mat = m_RP.m_TI[nThreadID].m_matView;
}

//////////////////////////////////////////////////////////////////////
void CAtomShimRenderer::GetProjectionMatrix(float* mat)
{
    int nThreadID = m_pRT->GetThreadList();
    *(Matrix44*)mat = m_RP.m_TI[nThreadID].m_matProj;
}

//////////////////////////////////////////////////////////////////////
void CAtomShimRenderer::SetMatrices(float* pProjMat, float* pViewMat)
{
    int nThreadID = m_pRT->GetThreadList();
    m_RP.m_TI[nThreadID].m_matProj = *(Matrix44*)pProjMat;
    m_RP.m_TI[nThreadID].m_matView = *(Matrix44*)pViewMat;
}

//////////////////////////////////////////////////////////////////////
void CAtomShimRenderer::ApplyViewParameters(const CameraViewParameters& viewParameters)
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

    const bool bReverseDepth = true; // [GFX TODO] [ATOM-1501] Currently we always assume reverse depth
    const bool bWasReverseDepth = (m_RP.m_TI[nThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) != 0 ? 1 : 0;

    m_RP.m_TI[nThreadID].m_PersFlags &= ~RBPF_REVERSE_DEPTH;
    if (bReverseDepth)
    {
        mathMatrixPerspectiveOffCenterReverseDepth((Matrix44A*)m, viewParameters.fWL, viewParameters.fWR, viewParameters.fWB, viewParameters.fWT, viewParameters.fNear, viewParameters.fFar);
        m_RP.m_TI[nThreadID].m_PersFlags |= RBPF_REVERSE_DEPTH;
    }

}

// Check if a file exists. This does not go through the AssetCatalog so that it can identify files that exist but aren't processed yet,
// and so that it will work before the AssetCatalog has loaded
bool CheckIfFileExists(const AZStd::string& sourceRelativePath, const AZStd::string& cacheRelativePath)
{
    // If the file exists, it has already been processed and does not need to be modified
    bool fileExists = AZ::IO::FileIOBase::GetInstance()->Exists(cacheRelativePath.c_str());

    if (!fileExists)
    {
        // If the texture doesn't exist check if it's queued or being compiled.
        AzFramework::AssetSystem::AssetStatus status;
        AzFramework::AssetSystemRequestBus::BroadcastResult(status, &AzFramework::AssetSystemRequestBus::Events::GetAssetStatus, sourceRelativePath);

        switch (status)
        {
        case AzFramework::AssetSystem::AssetStatus_Queued:
        case AzFramework::AssetSystem::AssetStatus_Compiling:
        case AzFramework::AssetSystem::AssetStatus_Compiled:
        case AzFramework::AssetSystem::AssetStatus_Failed:
            {
                // The file is queued, in progress, or finished processing after the initial FileIO check
                fileExists = true;
                break;
            }
        case AzFramework::AssetSystem::AssetStatus_Unknown:
        case AzFramework::AssetSystem::AssetStatus_Missing:
        default:
            {
                // The file does not exist
                fileExists = false;
                break;
            }
        }
    }

    return fileExists;
}

//////////////////////////////////////////////////////////////////////
ITexture* CAtomShimRenderer::EF_LoadTexture(const char * nameTex, const uint32 flags)
{
    AtomShimTexture* atomTexture = nullptr;

    // have to see if it is already loaded
    CBaseResource* pBR = CBaseResource::GetResource(CTexture::mfGetClassName(), nameTex, false);
    if (pBR)
    {
        // if a texture with this ID exists but it is not an Atom texture then we return nullptr
        CTexture* texture = static_cast<CTexture*>(pBR);
        AtomShimTexture* atomTexture2 = CastITextureToAtomShimTexture(texture);
        if (atomTexture2)
        {
            atomTexture2->AddRef();
            return atomTexture2;
        }
        else
        {
            return nullptr;
        }
    }

    AZ_Error("CAtomShimRenderer", AzFramework::StringFunc::Path::IsRelative(nameTex), "CAtomShimRenderer::EF_LoadTexture assumes that it will always be given a relative path, but got '%s'", nameTex);

    atomTexture = new AtomShimTexture(flags);
    atomTexture->Register(CTexture::mfGetClassName(), nameTex);
    atomTexture->SetSourceName( nameTex );  // needs to be normalized?
    
    AZStd::string sourceRelativePath(nameTex);
    AZStd::string cacheRelativePath = sourceRelativePath + ".streamingimage";

    bool textureExists = false;
    textureExists = CheckIfFileExists(sourceRelativePath, cacheRelativePath);

    if(!textureExists)
    {
        // A lot of cry code uses the .dds extension even when the actual source file is .tif.
        // For the .streamingimage file we need the correct source extension before .streamingimage
        // So if the file doesn't exist and the extension was .dds then try replacing it with .tif
        AZStd::string extension;
        AzFramework::StringFunc::Path::GetExtension(nameTex, extension, false);
        if (extension == "dds")
        {
            sourceRelativePath = nameTex;

            static const char* textureExtensions[] = { "png", "tif", "tiff", "tga", "jpg", "jpeg", "bmp", "gif" };

            for (const char* extensionReplacement : textureExtensions)
            {
                AzFramework::StringFunc::Path::ReplaceExtension(sourceRelativePath, extensionReplacement);
                cacheRelativePath = sourceRelativePath + ".streamingimage";

                textureExists = CheckIfFileExists(sourceRelativePath, cacheRelativePath);
                if (textureExists)
                {
                    break;
                }
            }
        }
    }

    if(!textureExists)
    {
        AZ_Error("CAtomShimRenderer", false, "EF_LoadTexture attempted to load '%s', but it does not exist.", nameTex);
        // Since neither the given extension nor the .dds version exist, we'll default to the given extension for hot-reloading in case the file is added to the source folder later
        sourceRelativePath = nameTex;
        cacheRelativePath = sourceRelativePath + ".streamingimage";
    }

    // now load the texture
    // NOTE: CTexture::CreateTexture does the actual setting of texture data in Cry D3D case
    // But it also calls CreateDeviceTexture

    {
        using namespace AZ;

        // The file may not be in the AssetCatalog at this point if it is still processing or doesn't exist on disk.
        // Use GenerateAssetIdTEMP instead of GetAssetIdByPath so that it will return a valid AssetId anyways
        Data::AssetId streamingImageAssetId;
        Data::AssetCatalogRequestBus::BroadcastResult(
            streamingImageAssetId, &Data::AssetCatalogRequestBus::Events::GenerateAssetIdTEMP,
            sourceRelativePath.c_str());
        streamingImageAssetId.m_subId = RPI::StreamingImageAsset::GetImageAssetSubId();

        auto streamingImageAsset = Data::AssetManager::Instance().FindOrCreateAsset<RPI::StreamingImageAsset>(streamingImageAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
        // Force a synchronous load for now - this will be replaced with a new system in future releases
        streamingImageAsset.QueueLoad();
        streamingImageAsset.BlockUntilLoadComplete();

        if (!streamingImageAsset.IsReady())
        {
            atomTexture->QueueForHotReload(streamingImageAssetId);
        }
        else
        {
            atomTexture->CreateFromStreamingImageAsset(streamingImageAsset);
        }
    }

    atomTexture->SetTexStates();

    return atomTexture;
}

//////////////////////////////////////////////////////////////////////
ITexture* CAtomShimRenderer::EF_LoadDefaultTexture(const char * nameTex)
{
    return CTextureManager::Instance()->GetDefaultTexture(nameTex);
}

//////////////////////////////////////////////////////////////////////
void CAtomShimRenderer::DrawQuad([[maybe_unused]] const Vec3& right, [[maybe_unused]] const Vec3& up, [[maybe_unused]] const Vec3& origin, [[maybe_unused]] int nFlipmode /*=0*/)
{
}

//////////////////////////////////////////////////////////////////////
bool CAtomShimRenderer::ProjectToScreen(float ptx, float pty, float ptz, float* sx, float* sy, float* sz)
{
    int nThreadID = m_pRT->GetThreadList();
    SViewport& vp = m_MainRTViewport;

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

int CAtomShimRenderer::UnProject(float sx, float sy, float sz,
    float* px, float* py, float* pz,
    const float modelMatrix[16],
    const float projMatrix[16],
    const int    viewport[4])
{
    return sUnProject(sx, sy, sz, modelMatrix, projMatrix, viewport, px, py, pz);
}

//////////////////////////////////////////////////////////////////////
int CAtomShimRenderer::UnProjectFromScreen(float  sx, float  sy, float  sz,
    float* px, float* py, float* pz)
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
bool CAtomShimRenderer::ScreenShot([[maybe_unused]] const char* filename, [[maybe_unused]] int width)
{
    return true;
}

int CAtomShimRenderer::ScreenToTexture([[maybe_unused]] int nTexID)
{
    return 0;
}

void CAtomShimRenderer::ResetToDefault()
{
}

///////////////////////////////////////////
void CAtomShimRenderer::SetMaterialColor([[maybe_unused]] float r, [[maybe_unused]] float g, [[maybe_unused]] float b, [[maybe_unused]] float a)
{
}

//////////////////////////////////////////////////////////////////////
void CAtomShimRenderer::ClearTargetsImmediately([[maybe_unused]] uint32 nFlags) {}
void CAtomShimRenderer::ClearTargetsImmediately([[maybe_unused]] uint32 nFlags, [[maybe_unused]] const ColorF& Colors, [[maybe_unused]] float fDepth) {}
void CAtomShimRenderer::ClearTargetsImmediately([[maybe_unused]] uint32 nFlags, [[maybe_unused]] const ColorF& Colors) {}
void CAtomShimRenderer::ClearTargetsImmediately([[maybe_unused]] uint32 nFlags, [[maybe_unused]] float fDepth) {}

void CAtomShimRenderer::ClearTargetsLater([[maybe_unused]] uint32 nFlags) {}
void CAtomShimRenderer::ClearTargetsLater([[maybe_unused]] uint32 nFlags, [[maybe_unused]] const ColorF& Colors, [[maybe_unused]] float fDepth) {}
void CAtomShimRenderer::ClearTargetsLater([[maybe_unused]] uint32 nFlags, [[maybe_unused]] const ColorF& Colors) {}
void CAtomShimRenderer::ClearTargetsLater([[maybe_unused]] uint32 nFlags, [[maybe_unused]] float fDepth) {}

void CAtomShimRenderer::ReadFrameBuffer([[maybe_unused]] unsigned char* pRGB, [[maybe_unused]] int nImageX, [[maybe_unused]] int nSizeX, [[maybe_unused]] int nSizeY, [[maybe_unused]] ERB_Type eRBType, [[maybe_unused]] bool bRGBA, [[maybe_unused]] int nScaledX, [[maybe_unused]] int nScaledY)
{
}

void CAtomShimRenderer::ReadFrameBufferFast([[maybe_unused]] uint32* pDstARGBA8, [[maybe_unused]] int dstWidth, [[maybe_unused]] int dstHeight, [[maybe_unused]] bool BGRA)
{
}

bool CAtomShimRenderer::CaptureFrameBufferFast([[maybe_unused]] unsigned char* pDstRGBA8, [[maybe_unused]] int destinationWidth, [[maybe_unused]] int destinationHeight)
{
    return false;
}
bool CAtomShimRenderer::CopyFrameBufferFast([[maybe_unused]] unsigned char* pDstRGBA8, [[maybe_unused]] int destinationWidth, [[maybe_unused]] int destinationHeight)
{
    return false;
}

bool CAtomShimRenderer::InitCaptureFrameBufferFast([[maybe_unused]] uint32 bufferWidth, [[maybe_unused]] uint32 bufferHeight)
{
    return(false);
}

void CAtomShimRenderer::CloseCaptureFrameBufferFast(void)
{
}

bool CAtomShimRenderer::RegisterCaptureFrame([[maybe_unused]] ICaptureFrameListener* pCapture)
{
    return(false);
}
bool CAtomShimRenderer::UnRegisterCaptureFrame([[maybe_unused]] ICaptureFrameListener* pCapture)
{
    return(false);
}

void CAtomShimRenderer::CaptureFrameBufferCallBack(void)
{
}


void CAtomShimRenderer::SetFogColor([[maybe_unused]] const ColorF& color)
{
}

void CAtomShimRenderer::DrawQuad([[maybe_unused]] float dy, [[maybe_unused]] float dx, [[maybe_unused]] float dz, [[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] float z)
{
}

//////////////////////////////////////////////////////////////////////

int CAtomShimRenderer::CreateRenderTarget([[maybe_unused]] const char* name, [[maybe_unused]] int nWidth, [[maybe_unused]] int nHeight, [[maybe_unused]] const ColorF& cClear, [[maybe_unused]] ETEX_Format eTF)
{
    return 0;
}

bool CAtomShimRenderer::ResizeRenderTarget([[maybe_unused]] int nHandle, [[maybe_unused]] int nWidth, [[maybe_unused]] int nHeight)
{
    return true;
}

bool CAtomShimRenderer::DestroyRenderTarget([[maybe_unused]] int nHandle)
{
    return true;
}

bool CAtomShimRenderer::SetRenderTarget([[maybe_unused]] int nHandle, [[maybe_unused]] SDepthTexture* pDepthSurf)
{
    return true;
}

SDepthTexture* CAtomShimRenderer::CreateDepthSurface([[maybe_unused]] int nWidth, [[maybe_unused]] int nHeight, [[maybe_unused]] bool shaderResourceView)
{
    return nullptr;
}

void CAtomShimRenderer::DestroyDepthSurface([[maybe_unused]] SDepthTexture* pDepthSurf)
{
}

void CAtomShimRenderer::WaitForParticleBuffer([[maybe_unused]] threadID nThreadId)
{
}

int CAtomShimRenderer::GetOcclusionBuffer([[maybe_unused]] uint16* pOutOcclBuffer, [[maybe_unused]] Matrix44* pmCamBuffe)
{
    return 0;
}

IColorGradingController* CAtomShimRenderer::GetIColorGradingController()
{
    return m_pAtomShimColorGradingController;
}

IStereoRenderer* CAtomShimRenderer::GetIStereoRenderer()
{
    return m_pAtomShimStereoRenderer;
}

ITexture* CAtomShimRenderer::Create2DTexture([[maybe_unused]] const char* name, [[maybe_unused]] int width, [[maybe_unused]] int height, [[maybe_unused]] int numMips, [[maybe_unused]] int flags, [[maybe_unused]] unsigned char* data, [[maybe_unused]] ETEX_Format format)
{
    return nullptr;
}

//=========================================================================================


ILog* iLog;
IConsole* iConsole;
ITimer* iTimer;
ISystem* iSystem;

StaticInstance<CAtomShimRenderer> g_nullRenderer;

extern "C" DLL_EXPORT IRenderer * CreateCryRenderInterface(ISystem * pSystem)
{
    ModuleInitISystem(pSystem, "CryRenderer");

    gbRgb = false;

    iConsole    = gEnv->pConsole;
    iLog            = gEnv->pLog;
    iTimer      = gEnv->pTimer;
    iSystem     = gEnv->pSystem;

    CRenderer* rd = g_nullRenderer;
    if (rd)
    {
        rd->InitRenderer();
    }

    std::random_device randDev;
    srand(static_cast<int>(randDev()));

    return rd;
}

class CEngineModule_CryRenderer
    : public IEngineModule
{
    CRYINTERFACE_SIMPLE(IEngineModule)
    CRYGENERATE_SINGLETONCLASS(CEngineModule_CryRenderer, "EngineModule_CryRenderer", 0x540c91a7338e41d3, 0xaceeac9d55614450)

    virtual const char* GetName() const {
        return "CryRenderer";
    }
    virtual const char* GetCategory() const {return "CryEngine"; }

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

void COcclusionQuery::Create()
{
}

void COcclusionQuery::Release()
{
}

void COcclusionQuery::BeginQuery()
{
}

void COcclusionQuery::EndQuery()
{
}

uint32 COcclusionQuery::GetVisibleSamples([[maybe_unused]] bool bAsynchronous)
{
    return 0;
}

/*static*/ FurBendData& FurBendData::Get()
{
    static FurBendData s_instance;
    return s_instance;
}

void FurBendData::InsertNewElements()
{
}

void FurBendData::FreeData()
{
}

void FurBendData::OnBeginFrame()
{
}

TArray<SRenderLight>* CRenderer::EF_GetDeferredLights([[maybe_unused]] const SRenderingPassInfo& passInfo, [[maybe_unused]] const eDeferredLightType eLightType)
{
    static TArray<SRenderLight> lights;
    return &lights;
}

SRenderLight* CRenderer::EF_GetDeferredLightByID([[maybe_unused]] const uint16 nLightID, [[maybe_unused]] const eDeferredLightType eLightType)
{
    return nullptr;
}


void CRenderer::BeginSpawningGeneratingRendItemJobs([[maybe_unused]] int nThreadID)
{
}

void CRenderer::BeginSpawningShadowGeneratingRendItemJobs([[maybe_unused]] int nThreadID)
{
}

void CRenderer::EndSpawningGeneratingRendItemJobs()
{
}

void CAtomShimRenderer::PrecacheResources()
{
}

bool CAtomShimRenderer::EF_PrecacheResource([[maybe_unused]] SShaderItem* pSI, [[maybe_unused]] float fMipFactorSI, [[maybe_unused]] float fTimeToReady, [[maybe_unused]] int Flags, [[maybe_unused]] int nUpdateId, [[maybe_unused]] int nCounter)
{
    return true;
}

ITexture* CAtomShimRenderer::EF_CreateCompositeTexture([[maybe_unused]] int type, [[maybe_unused]] const char* szName, [[maybe_unused]] int nWidth, [[maybe_unused]] int nHeight, [[maybe_unused]] int nDepth, [[maybe_unused]] int nMips, [[maybe_unused]] int nFlags, [[maybe_unused]] ETEX_Format eTF, [[maybe_unused]] const STexComposition* pCompositions, [[maybe_unused]] size_t nCompositions, [[maybe_unused]] int8 nPriority)
{
    return CTextureManager::Instance()->GetNoTexture();
}

void CAtomShimRenderer::FX_ClearTarget([[maybe_unused]] ITexture* pTex)
{
}

void CAtomShimRenderer::FX_ClearTarget([[maybe_unused]] SDepthTexture* pTex)
{
}

bool CAtomShimRenderer::FX_SetRenderTarget([[maybe_unused]] int nTarget, [[maybe_unused]] void* pTargetSurf, [[maybe_unused]] SDepthTexture* pDepthTarget, [[maybe_unused]] uint32 nTileCount)
{
    return true;
}

bool CAtomShimRenderer::FX_PushRenderTarget([[maybe_unused]] int nTarget, [[maybe_unused]] void* pTargetSurf, [[maybe_unused]] SDepthTexture* pDepthTarget, [[maybe_unused]] uint32 nTileCount)
{
    return true;
}

bool CAtomShimRenderer::FX_SetRenderTarget([[maybe_unused]] int nTarget, [[maybe_unused]] CTexture* pTarget, [[maybe_unused]] SDepthTexture* pDepthTarget, [[maybe_unused]] bool bPush, [[maybe_unused]] int nCMSide, [[maybe_unused]] bool bScreenVP, [[maybe_unused]] uint32 nTileCount)
{
    return true;
}

bool CAtomShimRenderer::FX_PushRenderTarget([[maybe_unused]] int nTarget, [[maybe_unused]] CTexture* pTarget, [[maybe_unused]] SDepthTexture* pDepthTarget, [[maybe_unused]] int nCMSide, [[maybe_unused]] bool bScreenVP, [[maybe_unused]] uint32 nTileCount)
{
    return true;
}
bool CAtomShimRenderer::FX_RestoreRenderTarget([[maybe_unused]] int nTarget)
{
    return true;
}
bool CAtomShimRenderer::FX_PopRenderTarget([[maybe_unused]] int nTarget)
{
    return true;
}

IDynTexture* CAtomShimRenderer::CreateDynTexture2([[maybe_unused]] uint32 nWidth, [[maybe_unused]] uint32 nHeight, [[maybe_unused]] uint32 nTexFlags, [[maybe_unused]] const char* szSource, [[maybe_unused]] ETexPool eTexPool)
{
    return nullptr;
}

void CAtomShimRenderer::InitSystemResources([[maybe_unused]] int nFlags)
{
    // This is an override of the implementation in CRenderer and is significantly cut down for the shim.
    if (!m_bSystemResourcesInit || m_bDeviceLost == 2)
    {
        CTextureManager::Instance()->Init();
        m_bSystemResourcesInit = 1;
    }
}

void CAtomShimRenderer::SetTexture(int tnum)
{
    SetTexture(tnum, 0);
}

void CAtomShimRenderer::SetTexture(int tnum, int nUnit)
{
    SetTextureForUnit(nUnit, tnum);
}

void CAtomShimRenderer::SetState([[maybe_unused]] int State, [[maybe_unused]] int AlphaRef)
{
    // [GFX TODO] would need to implement this for LyShine mask support and blend mode support
}

void CAtomShimRenderer::SetTextureForUnit(int unit, int textureId)
{
    AZ_Assert(unit >= 0 && unit < 32, "Invalid texture unit");
    AtomShimTexture* atomTexture = CastITextureToAtomShimTexture(EF_GetTextureByID(textureId));
    m_currentTextureForUnit[unit] = atomTexture;
    m_clampFlagPerTextureUnit[unit] = (atomTexture->GetFlags() & FT_STATE_CLAMP) ? true : false;
}

const AZ::Transform& CAtomShimRenderer::GetActiveCameraTransform()
{
    return m_cameraTransform;
}

const Camera::Configuration& CAtomShimRenderer::GetActiveCameraConfiguration()
{
    return m_cameraConfiguration;
}

void CAtomShimRenderer::CacheCameraTransform(const CCamera& camera)
{
    m_cameraTransform = LYTransformToAZTransform(camera.GetMatrix());
}

void CAtomShimRenderer::CacheCameraConfiguration(const CCamera& camera)
{
    Camera::Configuration& config = m_cameraConfiguration;
    config.m_fovRadians = camera.GetFov();
    config.m_nearClipDistance = camera.GetNearPlane();
    config.m_farClipDistance = camera.GetFarPlane();
    config.m_frustumHeight = config.m_farClipDistance * tanf(config.m_fovRadians / 2) * 2;
    config.m_frustumWidth = config.m_frustumHeight * camera.GetViewSurfaceX() / camera.GetViewSurfaceZ();
}

void CAtomShimRenderer::DrawStringU(
    [[maybe_unused]] IFFont_RenderProxy* pFont, [[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] float z,
    [[maybe_unused]] const char* pStr, [[maybe_unused]] const bool asciiMultiLine, [[maybe_unused]] const STextDrawContext& ctx) const
{
    // RenderCallback disabled, ICryFont has been directly implemented on Atom by Gems/AtomLyIntegration/AtomFont.
}

void CAtomShimRenderer::DrawDynVB(SVF_P3F_C4B_T2F* pBuf, uint16* pInds, int nVerts, int nInds, const PublicRenderPrimitiveType nPrimType)
{
    using namespace AZ;

    // if nothing to draw then return
    if (!pBuf || !nVerts || (pInds && !nInds) || (nInds && !pInds))
    {
        return;
    }

    // get view proj materix
    Matrix44A matView, matProj;
    GetModelViewMatrix(matView.GetData());
    GetProjectionMatrix(matProj.GetData());
    Matrix44A matViewProj = matView * matProj;
    Matrix4x4 azMatViewProj = Matrix4x4::CreateFromColumnMajorFloat16(matViewProj.GetData());

    bool isClamp = m_clampFlagPerTextureUnit[0];
    m_dynamicDraw->SetShaderVariant(isClamp? m_shaderVariantClamp : m_shaderVariantWrap);

    Data::Instance<RPI::ShaderResourceGroup> drawSrg = m_dynamicDraw->NewDrawSrg();
    drawSrg->SetConstant(m_viewProjInputIndex, azMatViewProj);

    AtomShimTexture* atomTexture = m_currentTextureForUnit[0];
    drawSrg->SetImageView(m_imageInputIndex, atomTexture->m_imageView.get());

    drawSrg->Compile();

    RHI::PrimitiveTopology primitiveType = RHI::PrimitiveTopology::TriangleList;

    switch (nPrimType)
    {
    case prtTriangleList:
        primitiveType = RHI::PrimitiveTopology::TriangleList;
        break;
    case prtTriangleStrip:
        primitiveType = RHI::PrimitiveTopology::TriangleStrip;
        break;
    case prtLineList:
        primitiveType = RHI::PrimitiveTopology::LineList;
        break;
    case prtLineStrip:
        primitiveType = RHI::PrimitiveTopology::LineStrip;
        break;
    }

    m_dynamicDraw->SetPrimitiveType(primitiveType);

    if (pInds)
    {
        m_dynamicDraw->DrawIndexed(pBuf, nVerts, pInds, nInds, RHI::IndexFormat::Uint16, drawSrg);
    }
    else
    {
        m_dynamicDraw->DrawLinear(pBuf, nVerts, drawSrg);
    }
}

void CAtomShimRenderer::DrawDynUiPrimitiveList(
    [[maybe_unused]] DynUiPrimitiveList& primitives, [[maybe_unused]] int totalNumVertices, [[maybe_unused]] int totalNumIndices)
{
    // This function was only used by LyShine and LyShine is moving to Atom implementation.
    return;
}

void CAtomShimRenderer::Set2DMode(uint32 orthoWidth, uint32 orthoHeight, TransformationMatrices& backupMatrices, float znear, float zfar)
{
    Set2DModeNonZeroTopLeft(0.0f, 0.0f, static_cast<float>(orthoWidth), static_cast<float>(orthoHeight), backupMatrices, znear, zfar);
}

void CAtomShimRenderer::Unset2DMode(const TransformationMatrices& restoringMatrices)
{
    int nThreadID = m_pRT->GetThreadList();

#ifdef _DEBUG
    // Check that we are already in 2D mode on this thread and decrement the counter used for this check.
    AZ_Assert(s_isIn2DMode[nThreadID]-- > 0, "Calls to Set2DMode and Unset2DMode appear mismatched");
#endif

    m_RP.m_TI[nThreadID].m_matView = restoringMatrices.m_viewMatrix;
    m_RP.m_TI[nThreadID].m_matProj = restoringMatrices.m_projectMatrix;

    // The legacy renderer supports nested Set2dMode/Unset2dMode so we use a counter to support that also.
    m_isIn2dModeCounter--;
    if (m_isIn2dModeCounter > 0)
    {
        // We're still in 2d mode, so set the viewProjOverride to the current matrix
        // For 2d drawing, the view matrix is an identity matrix, so viewProj == proj
        AZ::Matrix4x4 viewProj = AZ::Matrix4x4::CreateFromColumnMajorFloat16(m_RP.m_TI[nThreadID].m_matProj.GetData());
        m_pAtomShimRenderAuxGeom->SetViewProjOverride(viewProj);
    }
    else
    {
        m_pAtomShimRenderAuxGeom->UnsetViewProjOverride();
    }
}

void CAtomShimRenderer::Set2DModeNonZeroTopLeft(
    float orthoLeft, float orthoTop, float orthoWidth, float orthoHeight, TransformationMatrices& backupMatrices, float znear, float zfar)
{
    int nThreadID = m_pRT->GetThreadList();

#ifdef _DEBUG
    // Increment the counter used to check that Set2DMode and Unset2DMode are balanced.
    // It should never be negative before the increment.
    AZ_Assert(s_isIn2DMode[nThreadID]++ >= 0, "Calls to Set2DMode and Unset2DMode appear mismatched");
#endif

    backupMatrices.m_projectMatrix = m_RP.m_TI[nThreadID].m_matProj;

    // Move the zfar a bit away from the znear if they're the same.
    if (AZ::IsClose(znear, zfar, .001f))
    {
        zfar += .01f;
    }

    float left = orthoLeft;
    float right = left + orthoWidth;
    float top = orthoTop;
    float bottom = top + orthoHeight;

    mathMatrixOrthoOffCenterLH(&m_RP.m_TI[nThreadID].m_matProj, left, right, bottom, top, znear, zfar);

    if (m_RP.m_TI[nThreadID].m_PersFlags & RBPF_REVERSE_DEPTH)
    {
        // [GFX TODO] [ATOM-661] may need to reverse the depth here (though for 2D it may not be necessary)
    }

    backupMatrices.m_viewMatrix = m_RP.m_TI[nThreadID].m_matView;
    m_RP.m_TI[nThreadID].m_matView.SetIdentity();

    m_isIn2dModeCounter++;

    // For 2d drawing, the view matrix is an identity matrix, so viewProj == proj
    AZ::Matrix4x4 viewProj = AZ::Matrix4x4::CreateFromColumnMajorFloat16(m_RP.m_TI[nThreadID].m_matProj.GetData());
    m_pAtomShimRenderAuxGeom->SetViewProjOverride(viewProj);
}

void CAtomShimRenderer::SetColorOp(
    [[maybe_unused]] byte eCo, [[maybe_unused]] byte eAo, [[maybe_unused]] byte eCa, [[maybe_unused]] byte eAa)
{
    // this is only used by LY ImGui gem
}
