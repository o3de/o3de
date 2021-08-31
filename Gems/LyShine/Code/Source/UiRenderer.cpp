/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiRenderer.h"
#include "LyShinePassDataBus.h"

#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/Bootstrap/DefaultWindowBus.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/RenderPipeline.h>

#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Debug/Trace.h>

#include <LyShine/Draw2d.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiRenderer::UiRenderer(AZ::RPI::ViewportContextPtr viewportContext)
    : m_viewportContext(viewportContext)
{
    // Use bootstrap scene event to indicate when the RPI has fully
    // initialized with all assets loaded and is ready to be used
    AZ::Render::Bootstrap::NotificationBus::Handler::BusConnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiRenderer::~UiRenderer()
{
    AZ::Render::Bootstrap::NotificationBus::Handler::BusDisconnect();

    if (m_viewportContext)
    {
        AZ::RPI::RPISystemInterface::Get()->UnregisterScene(m_viewportContext->GetRenderScene());
    }
    m_dynamicDraw = nullptr;
}

bool UiRenderer::IsReady()
{
    return m_isRPIReady;
}

void UiRenderer::OnBootstrapSceneReady([[maybe_unused]] AZ::RPI::Scene* bootstrapScene)
{
    // At this point the RPI is ready for use

    // Load the UI shader
    const char* uiShaderFilepath = "Shaders/LyShineUI.azshader";
    AZ::Data::Instance<AZ::RPI::Shader> uiShader = AZ::RPI::LoadCriticalShader(uiShaderFilepath);

    // Create scene to be used by the dynamic draw context
    if (m_viewportContext)
    {
        // Create a new scene based on the user specified viewport context
        m_scene = CreateScene(m_viewportContext);
    }
    else
    {
        // No viewport context specified, use default scene
        m_scene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    }

    // Create a dynamic draw context for UI Canvas drawing for the scene
    m_dynamicDraw = CreateDynamicDrawContext(m_scene, uiShader);

    if (m_dynamicDraw)
    {
        // Cache shader data such as input indices for later use
        CacheShaderData(m_dynamicDraw);

        m_isRPIReady = true;
    }
    else
    {
        AZ_Error(LogName, false, "Failed to create a dynamic draw context for LyShine. \
            This can happen if the LyShine pass hasn't been added to the main render pipeline.");
    }
}

AZ::RPI::ScenePtr UiRenderer::CreateScene(AZStd::shared_ptr<AZ::RPI::ViewportContext> viewportContext)
{
    // Create a scene with the necessary feature processors
    AZ::RPI::SceneDescriptor sceneDesc;
    AZ::RPI::ScenePtr atomScene = AZ::RPI::Scene::CreateScene(sceneDesc);
    atomScene->EnableAllFeatureProcessors(); // LYSHINE_ATOM_TODO - have a UI pipeline and enable only needed fps

    // Assign the new scene to the specified viewport context
    viewportContext->SetRenderScene(atomScene);

    // Create a render pipeline and add it to the scene
    AZStd::string pipelineAssetPath = "passes/MainRenderPipeline.azasset"; // LYSHINE_ATOM_TODO - make and use a UI pipeline
    AZ::Data::Asset<AZ::RPI::AnyAsset> pipelineAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(pipelineAssetPath.c_str(), AZ::RPI::AssetUtils::TraceLevel::Error);
    AZStd::shared_ptr<AZ::RPI::WindowContext> windowContext = viewportContext->GetWindowContext();
    auto renderPipeline = AZ::RPI::RenderPipeline::CreateRenderPipelineForWindow(pipelineAsset, *windowContext.get());
    pipelineAsset.Release();
    atomScene->AddRenderPipeline(renderPipeline);

    atomScene->Activate();

    // Register the scene
    AZ::RPI::RPISystemInterface::Get()->RegisterScene(atomScene);

    return atomScene;
}

AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> UiRenderer::CreateDynamicDrawContext(
    AZ::RPI::ScenePtr scene,
    AZ::Data::Instance<AZ::RPI::Shader> uiShader)
{
    // Find the pass that renders the UI canvases after the rtt passes
    AZ::RPI::RasterPass* uiCanvasPass = nullptr;
    AZ::RPI::SceneId sceneId = m_scene->GetId();
    LyShinePassRequestBus::EventResult(uiCanvasPass, sceneId, &LyShinePassRequestBus::Events::GetUiCanvasPass);

    AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> dynamicDraw = AZ::RPI::DynamicDrawInterface::Get()->CreateDynamicDrawContext();

    // Initialize the dynamic draw context
    dynamicDraw->InitShader(uiShader);
    dynamicDraw->InitVertexFormat(
        { { "POSITION", AZ::RHI::Format::R32G32_FLOAT },
        { "COLOR", AZ::RHI::Format::B8G8R8A8_UNORM },
        { "TEXCOORD", AZ::RHI::Format::R32G32_FLOAT },
        { "BLENDINDICES", AZ::RHI::Format::R16G16_UINT } }
    );
    dynamicDraw->AddDrawStateOptions(AZ::RPI::DynamicDrawContext::DrawStateOptions::StencilState
        | AZ::RPI::DynamicDrawContext::DrawStateOptions::BlendMode);

    if (uiCanvasPass)
    {
        dynamicDraw->SetOutputScope(uiCanvasPass);
    }
    else
    {
        // Render target support is disabled
        dynamicDraw->SetOutputScope(m_scene.get());
    }
    dynamicDraw->EndInit();

    return dynamicDraw;
}

AZStd::shared_ptr<AZ::RPI::ViewportContext> UiRenderer::GetViewportContext()
{
    if (!m_viewportContext)
    {
        // Return the default viewport context
        auto viewContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        auto defaultViewportContext = viewContextManager->GetViewportContextByName(viewContextManager->GetDefaultViewportContextName());
        return defaultViewportContext;
    }

    // Return the user specified viewport context
    return m_viewportContext;
}

void UiRenderer::CacheShaderData(const AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext>& dynamicDraw)
{
    // Cache draw srg input indices
    static const char textureIndexName[] = "m_texture";
    static const char worldToProjIndexName[] = "m_worldToProj";
    static const char isClampIndexName[] = "m_isClamp";
    AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> drawSrg = dynamicDraw->NewDrawSrg();
    const AZ::RHI::ShaderResourceGroupLayout* layout = drawSrg->GetLayout();
    m_uiShaderData.m_imageInputIndex = layout->FindShaderInputImageIndex(AZ::Name(textureIndexName));
    AZ_Error(LogName, m_uiShaderData.m_imageInputIndex.IsValid(), "Failed to find shader input constant %s.",
        textureIndexName);
    m_uiShaderData.m_viewProjInputIndex = layout->FindShaderInputConstantIndex(AZ::Name(worldToProjIndexName));
    AZ_Error(LogName, m_uiShaderData.m_viewProjInputIndex.IsValid(), "Failed to find shader input constant %s.",
        worldToProjIndexName);
    m_uiShaderData.m_isClampInputIndex = layout->FindShaderInputConstantIndex(AZ::Name(isClampIndexName));
    AZ_Error(LogName, m_uiShaderData.m_isClampInputIndex.IsValid(), "Failed to find shader input constant %s.",
        isClampIndexName);

    // Cache shader variants that will be used
    AZ::RPI::ShaderOptionList shaderOptionsTextureLinear;
    shaderOptionsTextureLinear.push_back(AZ::RPI::ShaderOption(AZ::Name("o_alphaTest"), AZ::Name("false")));
    shaderOptionsTextureLinear.push_back(AZ::RPI::ShaderOption(AZ::Name("o_srgbWrite"), AZ::Name("true")));
    shaderOptionsTextureLinear.push_back(AZ::RPI::ShaderOption(AZ::Name("o_modulate"), AZ::Name("Modulate::None")));
    m_uiShaderData.m_shaderVariantTextureLinear = dynamicDraw->UseShaderVariant(shaderOptionsTextureLinear);
    AZ::RPI::ShaderOptionList shaderOptionsTextureSrgb;
    shaderOptionsTextureSrgb.push_back(AZ::RPI::ShaderOption(AZ::Name("o_alphaTest"), AZ::Name("false")));
    shaderOptionsTextureSrgb.push_back(AZ::RPI::ShaderOption(AZ::Name("o_srgbWrite"), AZ::Name("false")));
    shaderOptionsTextureSrgb.push_back(AZ::RPI::ShaderOption(AZ::Name("o_modulate"), AZ::Name("Modulate::None")));
    m_uiShaderData.m_shaderVariantTextureSrgb = dynamicDraw->UseShaderVariant(shaderOptionsTextureSrgb);
    AZ::RPI::ShaderOptionList shaderVariantAlphaTestMask;
    shaderVariantAlphaTestMask.push_back(AZ::RPI::ShaderOption(AZ::Name("o_alphaTest"), AZ::Name("true")));
    shaderVariantAlphaTestMask.push_back(AZ::RPI::ShaderOption(AZ::Name("o_srgbWrite"), AZ::Name("false")));
    shaderVariantAlphaTestMask.push_back(AZ::RPI::ShaderOption(AZ::Name("o_modulate"), AZ::Name("Modulate::None")));
    m_uiShaderData.m_shaderVariantAlphaTestMask = dynamicDraw->UseShaderVariant(shaderVariantAlphaTestMask);
    AZ::RPI::ShaderOptionList shaderVariantGradientMask;
    shaderVariantGradientMask.push_back(AZ::RPI::ShaderOption(AZ::Name("o_alphaTest"), AZ::Name("false")));
    shaderVariantGradientMask.push_back(AZ::RPI::ShaderOption(AZ::Name("o_srgbWrite"), AZ::Name("false")));
    shaderVariantGradientMask.push_back(AZ::RPI::ShaderOption(AZ::Name("o_modulate"), AZ::Name("Modulate::Alpha")));
    m_uiShaderData.m_shaderVariantGradientMask = dynamicDraw->UseShaderVariant(shaderVariantGradientMask);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::BeginUiFrameRender()
{
#ifndef _RELEASE
    if (m_debugTextureDataRecordLevel > 0)
    {
        m_texturesUsedInFrame.clear();
    }
#endif
   
    // Various platform drivers expect all texture slots used in the shader to be bound
    BindNullTexture();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::EndUiFrameRender()
{
    // We never want to leave a texture bound that could get unloaded before the next render
    // So bind the global white texture for all the texture units we use.
    BindNullTexture();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::BeginCanvasRender()
{
    m_stencilRef = 0;

    // Set base state
    m_baseState.ResetToDefault();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::EndCanvasRender()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> UiRenderer::GetDynamicDrawContext()
{
    return m_dynamicDraw;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> UiRenderer::CreateDynamicDrawContextForRTT(const AZStd::string& rttName)
{
    // find the rtt pass with the specified name
    AZ::RPI::RasterPass* rttPass = nullptr;    
    AZ::RPI::SceneId sceneId = m_scene->GetId();
    LyShinePassRequestBus::EventResult(rttPass, sceneId, &LyShinePassRequestBus::Events::GetRttPass, rttName);
    if (!rttPass)
    {
        return nullptr;
    }

    AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> dynamicDraw = AZ::RPI::DynamicDrawInterface::Get()->CreateDynamicDrawContext();

    // Initialize the dynamic draw context
    dynamicDraw->InitShader(m_dynamicDraw->GetShader());
    dynamicDraw->InitVertexFormat(
        { { "POSITION", AZ::RHI::Format::R32G32_FLOAT },
        { "COLOR", AZ::RHI::Format::B8G8R8A8_UNORM },
        { "TEXCOORD", AZ::RHI::Format::R32G32_FLOAT },
        { "BLENDINDICES", AZ::RHI::Format::R16G16_UINT } }
    );
    dynamicDraw->AddDrawStateOptions(AZ::RPI::DynamicDrawContext::DrawStateOptions::StencilState
        | AZ::RPI::DynamicDrawContext::DrawStateOptions::BlendMode);

    dynamicDraw->SetOutputScope(rttPass);

    dynamicDraw->EndInit();

    return dynamicDraw;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const UiRenderer::UiShaderData& UiRenderer::GetUiShaderData()
{
    return m_uiShaderData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Matrix4x4 UiRenderer::GetModelViewProjectionMatrix()
{
    auto viewportContext = GetViewportContext();
    auto windowContext = viewportContext->GetWindowContext();

    const AZ::RHI::Viewport& viewport = windowContext->GetViewport();
    const float viewX = viewport.m_minX;
    const float viewY = viewport.m_minY;
    const float viewWidth = viewport.m_maxX - viewport.m_minX;
    const float viewHeight = viewport.m_maxY - viewport.m_minY;
    const float zf = viewport.m_minZ;
    const float zn = viewport.m_maxZ;

    AZ::Matrix4x4 modelViewProjMat;
    AZ::MakeOrthographicMatrixRH(modelViewProjMat, viewX, viewX + viewWidth, viewY + viewHeight, viewY, zn, zf);

    return modelViewProjMat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiRenderer::GetViewportSize()
{
    auto viewportContext = GetViewportContext();
    auto windowContext = viewportContext->GetWindowContext();

    const AZ::RHI::Viewport& viewport = windowContext->GetViewport();
    const float viewWidth = viewport.m_maxX - viewport.m_minX;
    const float viewHeight = viewport.m_maxY - viewport.m_minY;
    return AZ::Vector2(viewWidth, viewHeight);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiRenderer::BaseState UiRenderer::GetBaseState()
{
    return m_baseState;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::SetBaseState(BaseState state)
{
    m_baseState = state;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::RPI::ShaderVariantId UiRenderer::GetCurrentShaderVariant()
{
    AZ::RPI::ShaderVariantId variantId = m_uiShaderData.m_shaderVariantTextureLinear;

    if (m_baseState.m_useAlphaTest)
    {
        variantId = m_uiShaderData.m_shaderVariantAlphaTestMask;
    }
    else if (m_baseState.m_modulateAlpha)
    {
        variantId = m_uiShaderData.m_shaderVariantGradientMask;
    }
    else if (!m_baseState.m_useAlphaTest && m_baseState.m_srgbWrite)
    {
        variantId = m_uiShaderData.m_shaderVariantTextureLinear;
    }
    else if (!m_baseState.m_useAlphaTest && !m_baseState.m_srgbWrite)
    {
        variantId = m_uiShaderData.m_shaderVariantTextureSrgb;
    }
    else
    {
        AZ_Error(LogName, 0, "Unsupported shader variant.");
    }

    return variantId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32 UiRenderer::GetStencilRef()
{
    return m_stencilRef;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::SetStencilRef(uint32 stencilRef)
{
    m_stencilRef = stencilRef;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::IncrementStencilRef()
{
    ++m_stencilRef;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::DecrementStencilRef()
{
    --m_stencilRef;
}

#ifdef LYSHINE_ATOM_TODO
////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::SetTexture(ITexture* texture, int texUnit, bool clamp)
{
    if (!texture)
    {
        texture = m_renderer->GetWhiteTexture();
    }
    else
    {
        texture->SetClamp(clamp);
    }

    m_renderer->SetTexture(texture->GetTextureID(), texUnit);

#ifndef _RELEASE
    if (m_debugTextureDataRecordLevel > 0)
    {
        m_texturesUsedInFrame.insert(texture);
    }
#endif
}
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::BindNullTexture()
{
#ifdef LYSHINE_ATOM_TODO
    // Bind the global white texture for all the texture units we use
    const int MaxTextures = 16;
    int whiteTexId = m_renderer->GetWhiteTextureId();
    for (int texUnit = 0; texUnit < MaxTextures; ++texUnit)
    {
        m_renderer->SetTexture(whiteTexId, texUnit);
    }
#endif
}

#ifndef _RELEASE

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::DebugSetRecordingOptionForTextureData(int recordingOption)
{
    m_debugTextureDataRecordLevel = recordingOption;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::DebugDisplayTextureData(int recordingOption)
{
    if (recordingOption > 0)
    {
#ifdef LYSHINE_ATOM_TODO // Convert debug to use Atom images
        // compute the total area of all the textures, also create a vector that we can sort by area
        AZStd::vector<ITexture*> textures;
        int totalArea = 0;
        int totalDataSize = 0;
        for (ITexture* texture : m_texturesUsedInFrame)
        {
            int area = texture->GetWidth() * texture->GetHeight();
            int dataSize = texture->GetDataSize();
            totalArea += area;
            totalDataSize += dataSize;

            textures.push_back(texture);
        }

        // sort the vector by data size
        std::sort( textures.begin( ), textures.end( ), [ ]( const ITexture* lhs, const ITexture* rhs )
        {
            return lhs->GetDataSize() > rhs->GetDataSize();
        });

        CDraw2d* draw2d = Draw2dHelper::GetDefaultDraw2d();

        // setup to render lines of text for the debug display

        float xOffset = 20.0f;
        float yOffset = 20.0f;

        auto blackTexture = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::Black);
        float textOpacity = 1.0f;
        float backgroundRectOpacity = 0.75f;
        const float lineSpacing = 20.0f;

        const AZ::Vector3 white(1,1,1);
        const AZ::Vector3 red(1,0.3f,0.3f);
        const AZ::Vector3 blue(0.3f,0.3f,1);

        int xDim, yDim;
        if (totalArea > 2048 * 2048)
        {
            xDim = 4096;
            yDim = totalArea / 4096;
        }
        else
        {
            xDim = 2048;
            yDim = totalArea / 2048;
        }

        float totalDataSizeMB = static_cast<float>(totalDataSize) / (1024.0f * 1024.0f);

        // local function to write a line of text (with a background rect) and increment Y offset
        AZStd::function<void(const char*, const AZ::Vector3&)> WriteLine = [&](const char* buffer, const AZ::Vector3& color)
        {
            CDraw2d::TextOptions textOptions = draw2d->GetDefaultTextOptions();
            textOptions.color = color;
            AZ::Vector2 textSize = draw2d->GetTextSize(buffer, 16, &textOptions);
            AZ::Vector2 rectTopLeft = AZ::Vector2(xOffset - 2, yOffset);
            AZ::Vector2 rectSize = AZ::Vector2(textSize.GetX() + 4, lineSpacing);
            draw2d->DrawImage(blackTexture, rectTopLeft, rectSize, backgroundRectOpacity);
            draw2d->DrawText(buffer, AZ::Vector2(xOffset, yOffset), 16, textOpacity, &textOptions);
            yOffset += lineSpacing;
        };

        int numTexturesUsedInFrame = m_texturesUsedInFrame.size();
        char buffer[200];
        sprintf_s(buffer, "There are %d unique UI textures rendered in this frame, the total texture area is %d (%d x %d), total data size is %d (%.2f MB)",
            numTexturesUsedInFrame, totalArea, xDim, yDim, totalDataSize, totalDataSizeMB);
        WriteLine(buffer, white);
        sprintf_s(buffer, "Dimensions   Data Size   Format Texture name");
        WriteLine(buffer, blue);

        for (ITexture* texture : textures)
        {
            sprintf_s(buffer, "%4d x %4d, %9d %8s %s",
                texture->GetWidth(), texture->GetHeight(), texture->GetDataSize(), texture->GetFormatName(), texture->GetName());
            WriteLine(buffer, white);
        }
#endif
    }
}

#endif
