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

#include "RenderDll_precompiled.h"
#include "D3DHMDRenderer.h"
#include "DriverD3D.h"
#include <HMDBus.h>
#include <IStereoRenderer.h>
#include <D3DPostProcess.h>
#include <Common/Textures/Texture.h>
#include <string.h> // for memset

D3DHMDRenderer::D3DHMDRenderer() :
    m_eyeWidth(0),
    m_eyeHeight(0),
    m_renderer(nullptr),
    m_stereoRenderer(nullptr),
    m_framePrepared(false)
{
    memset(m_eyes, 0, sizeof(m_eyes));
}

D3DHMDRenderer::~D3DHMDRenderer()
{
}

bool D3DHMDRenderer::Initialize(CD3D9Renderer* renderer, CD3DStereoRenderer* stereoRenderer)
{
    m_renderer       = renderer;
    m_stereoRenderer = stereoRenderer;
    m_eyeWidth       = m_renderer->GetWidth();
    m_eyeHeight      = m_renderer->GetHeight();

    AZ::VR::HMDDeviceBus::TextureDesc textureDesc;
    textureDesc.width = m_eyeWidth;
    textureDesc.height = m_eyeHeight;

    if (!ResizeRenderTargets(textureDesc))
    {
        return false;
    }

    return true;
}

void D3DHMDRenderer::Shutdown()
{
    m_stereoRenderer->SetEyeTextures(nullptr, nullptr);
    FreeRenderTargets();
}

void D3DHMDRenderer::CalculateBackbufferResolution(int eyeWidth, int eyeHeight, int& backbufferWidth, int& backbufferHeight)
{
    backbufferWidth  = eyeWidth;
    backbufferHeight = eyeHeight;
}

void D3DHMDRenderer::OnResolutionChanged()
{
    if (m_eyeWidth != m_renderer->GetWidth() ||
        m_eyeHeight != m_renderer->GetHeight())
    {
        // The size has actually changed, re-create the internal buffers.
        m_eyeWidth  = m_renderer->GetWidth();
        m_eyeHeight = m_renderer->GetHeight();

        AZ::VR::HMDDeviceBus::TextureDesc textureDesc;
        textureDesc.width = m_eyeWidth;
        textureDesc.height = m_eyeHeight;

        ResizeRenderTargets(textureDesc);
    }
}

void D3DHMDRenderer::RenderSocialScreen()
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(D3DHMDRenderer_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    //Only render the social screen if we're rendering to the main viewport
    if (!gcpRendD3D->m_CurrContext->m_bMainViewport)
    {
        return;
    }

    static ICVar* socialScreen = gEnv->pConsole->GetCVar("hmd_social_screen");
    AZ::VR::HMDSocialScreen socialScreenType = static_cast<AZ::VR::HMDSocialScreen>(socialScreen->GetIVal());

    switch (socialScreenType)
    {
        case AZ::VR::HMDSocialScreen::Off:
        {
            // Don't render true black in order to distinguish between a rendering error and no social screen.
            GetUtils().ClearScreen(0.1f, 0.1f, 0.1f, 1.0f);
            break;
        }

        case AZ::VR::HMDSocialScreen::UndistortedLeftEye:
        case AZ::VR::HMDSocialScreen::UndistortedRightEye:
        {
            bool isLeftEye = (socialScreenType == AZ::VR::HMDSocialScreen::UndistortedLeftEye);
            CTexture* sourceTexture = isLeftEye ? m_stereoRenderer->GetLeftEye() : m_stereoRenderer->GetRightEye();
            GetUtils().CopyTextureToScreen(sourceTexture);
            break;
        }

        default:
            AZ_Assert(false, "Unknown social screen type specified in HMD renderer");
            break;
    }
#endif
}

void D3DHMDRenderer::PrepareFrame()
{
    // Determine which frame texture we should be using.
    AZ::u32 leftEyeIndex = 0;
    AZ::u32 rightEyeIndex = 0;
    EBUS_EVENT_RESULT(leftEyeIndex, AZ::VR::HMDDeviceRequestBus, GetSwapchainIndex, STEREO_EYE_LEFT);
    EBUS_EVENT_RESULT(rightEyeIndex, AZ::VR::HMDDeviceRequestBus, GetSwapchainIndex, STEREO_EYE_RIGHT);

    CTexture* leftEye  = m_eyes[STEREO_EYE_LEFT].textureChain[leftEyeIndex];
    CTexture* rightEye = m_eyes[STEREO_EYE_RIGHT].textureChain[rightEyeIndex];

    m_stereoRenderer->SetEyeTextures(leftEye, rightEye);
    m_framePrepared = true;

    EBUS_EVENT(AZ::VR::HMDDeviceRequestBus, PrepareFrame);
}

void D3DHMDRenderer::SubmitFrame()
{
    AZ_Assert(m_framePrepared, "D3DHMDRenderer::PrepareFrame() must be called BEFORE SubmitFrame()");

    AZ::VR::HMDDeviceBus::EyeTarget targets[STEREO_EYE_COUNT];
    for (uint32 eye = 0; eye < STEREO_EYE_COUNT; ++eye)
   { 
        targets[eye].renderTarget     = m_eyes[eye].deviceRenderTarget.deviceSwapTextureSet;
        targets[eye].viewportPosition = m_eyes[eye].viewportPosition;
        targets[eye].viewportSize     = m_eyes[eye].viewportSize;
    }

    // Pass the final images to the HMD for final compositing and display.
    EBUS_EVENT(AZ::VR::HMDDeviceRequestBus, SubmitFrame, targets[STEREO_EYE_LEFT], targets[STEREO_EYE_RIGHT]);

    m_framePrepared = false;
}

bool D3DHMDRenderer::ResizeRenderTargets(const AZ::VR::HMDDeviceBus::TextureDesc& textureDesc)
{
    FreeRenderTargets();

    const char* eyeTextureNames[2] = { "$LeftEye_%d", "$RightEye_%d" };

    // Initialize the eye textures for rendering.
    ID3D11Device* d3d11Device = &m_renderer->GetDevice();

    AZ::VR::HMDRenderTarget* renderTargets[STEREO_EYE_COUNT];
    for (int eye = 0; eye < STEREO_EYE_COUNT; ++eye)
    {
        renderTargets[eye] = &((&(m_eyes[eye]))->deviceRenderTarget);
    }

    {
        bool result = false;
        EBUS_EVENT_RESULT(result, AZ::VR::HMDDeviceRequestBus, CreateRenderTargets, d3d11Device, textureDesc, STEREO_EYE_COUNT, renderTargets);

        if (!result)
        {
            Shutdown();
            return false;
        }
    }

    for (int eye = 0; eye < STEREO_EYE_COUNT; ++eye)
    {
        EyeRenderTarget* eyeTarget = &(m_eyes[eye]);

        // The render target was successfully created, now wrap the device target in a DX11 texture for use by the rest
        // of the engine.
        uint32 numTextures = eyeTarget->deviceRenderTarget.numTextures;
        eyeTarget->textureChain.reserve(numTextures);
        for (uint32 texID = 0; texID < numTextures; ++texID)
        {
            char textureName[256];
            sprintf_s(textureName, eyeTextureNames[eye], texID);

            ETEX_Format format = CTexture::TexFormatFromDeviceFormat(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
            CTexture* eyeTexture = WrapD3DRenderTarget(static_cast<ID3D11Texture2D*>(eyeTarget->deviceRenderTarget.textures[texID]), textureDesc.width, textureDesc.height, format, textureName, true);
            if (eyeTexture == nullptr)
            {
                return false;
            }

            eyeTarget->textureChain.push_back(eyeTexture);
            eyeTarget->viewportPosition = Vec2i(0, 0);
            eyeTarget->viewportSize = Vec2i(textureDesc.width, textureDesc.height);
        }
    }

    return true;
}

void D3DHMDRenderer::FreeRenderTargets()
{
    for (uint32 eye = 0; eye < STEREO_EYE_COUNT; ++eye)
    {
        for (uint32 texID = 0; texID < m_eyes[eye].textureChain.size(); ++texID)
        {
            SAFE_RELEASE_FORCE(m_eyes[eye].textureChain[texID]);
        }

        // Destroy the device render target as well.
        if (m_eyes[eye].textureChain.size())
        {
            EBUS_EVENT(AZ::VR::HMDDeviceRequestBus, DestroyRenderTarget, m_eyes[eye].deviceRenderTarget);
        }

        m_eyes[eye].textureChain.clear();
    }
}

CTexture* D3DHMDRenderer::WrapD3DRenderTarget(ID3D11Texture2D* d3dTexture, uint32 width, uint32 height, ETEX_Format format, const char* name, bool shaderResourceView)
{
    CTexture* texture = CTexture::CreateTextureObject(name, width, height, 1, eTT_2D, FT_DONT_STREAM | FT_USAGE_RENDERTARGET, format);
    if (texture == nullptr)
    {
        CryLogAlways("[HMD] Unable to create texture object!");
        return nullptr;
    }

    // CTexture::CreateTextureObject does not set width and height if the texture already existed
    AZ_Assert(texture->GetWidth() == width, "Texture was already wrapped");
    AZ_Assert(texture->GetHeight() == height, "Texture was already wrapped");
    AZ_Assert(texture->GetDepth() == 1, "Texture was already wrapped");

    d3dTexture->AddRef();
    CDeviceTexture* deviceTexture = new CDeviceTexture(d3dTexture);
    deviceTexture->SetNoDelete(true);

    texture->SetDevTexture(deviceTexture);
    texture->ClosestFormatSupported(format);

    if (shaderResourceView)
    {
        void* default_srv = texture->CreateDeviceResourceView(SResourceView::ShaderResourceView(format, 0, -1, 0, 1, false, false));
        if (default_srv == nullptr)
        {
            CryLogAlways("[HMD] Unable to create default shader resource view!");
            texture->Release();
            return nullptr;
        }
        texture->SetShaderResourceView(static_cast<D3DShaderResourceView*>(default_srv), false);
    }

    return texture;
}
