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


#include "CryRenderOther_precompiled.h"
#include "AtomShim_Renderer.h"
#include "AtomShim_DynamicDraw.h"
#include "Common/Textures/TextureManager.h"

#include "../CryFont/FBitmap.h"

#include <Atom/RPI.Public/DynamicDraw/DynamicDrawSystemInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/RPIUtils.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/DrawPacket.h>
#include <Atom/RHI/ImagePool.h>

#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>

#ifdef _DEBUG
// static array used to check that calls to Set2DMode and Unset2DMode are matched. (static array initialized to zeros automatically).
int s_isIn2DMode[RT_COMMAND_BUF_COUNT];
#endif

CAtomShimDynamicDraw::CAtomShimDynamicDraw()
{
    using namespace AZ;

    RPI::Scene* scene = RPI::RPISystemInterface::Get()->GetDefaultScene().get();
    RPI::DynamicDrawInterface* dynamicDraw = AZ::RPI::GetDynamicDraw(scene);
    AZ_Assert(dynamicDraw, "DynamicDrawSystemInterface not initialized for this scene!");

    RHI::Factory& factory = RHI::Factory::Get();
    RHI::ResultCode result;

    // Input assembly data...
    {
        // vertex/index buffers...
        {
            AZ_Assert(sizeof(SVF_P3F_C4B_T2F) == sizeof(SVF_P2F_C4B_T2F_F4B), "SInce we shared the same vertex buffer, SVF_P3F_C4B_T2F and SVF_P2F_C4B_T2F_F4B must be the same size.");
            m_inputAssemblyPool = dynamicDraw->GetInputAssemblyBufferHostPool();

            m_vertexBuffer = factory.CreateBuffer();
            m_vertexBuffer->SetName(AZ::Name("Font VB"));
            RHI::BufferInitRequest vbReq;
            vbReq.m_buffer = m_vertexBuffer.get();
            vbReq.m_descriptor = RHI::BufferDescriptor{ RHI::BufferBindFlags::InputAssembly, MaxVerts * sizeof(SVF_P3F_C4B_T2F) };
            result = m_inputAssemblyPool->InitBuffer(vbReq);
            AZ_Assert(result == RHI::ResultCode::Success, "Failed to init vertex buffer");

            m_indexBuffer = factory.CreateBuffer();
            m_indexBuffer->SetName(AZ::Name("Font IB"));
            RHI::BufferInitRequest ibReq;
            ibReq.m_buffer = m_indexBuffer.get();
            ibReq.m_descriptor = RHI::BufferDescriptor{ RHI::BufferBindFlags::InputAssembly, MaxIndices * sizeof(u16) };
            result = m_inputAssemblyPool->InitBuffer(ibReq);
            AZ_Assert(result == RHI::ResultCode::Success, "Failed to init index buffer");

            // Vertex format...
            m_streamBufferViews[0] = {
                *m_vertexBuffer,                   // buffer
                0,                                 // byte offset
                MaxVerts*sizeof(SVF_P3F_C4B_T2F),  // byte count
                sizeof(SVF_P3F_C4B_T2F)            // byte stride
            };

            m_indexBufferView = {
                *m_indexBuffer,             // buffer
                0,                          // byte offset
                MaxIndices * sizeof(u16),   // byte count
                RHI::IndexFormat::Uint16    // index format
            };
        }
    }

    // Shaders and SRGs...
    LoadShader("Shaders/SimpleTextured.azshader", m_simpleTexturedShader);
    LoadUiShader("Shaders/LyShineUI.azshader", m_uiShader);

    // cache the 2D mode drawlist tag
    RHI::RHISystemInterface* rhiSystem = RHI::RHISystemInterface::Get();
    AZ_Assert(rhiSystem, "RHISystemInterface not initialized");

    m_2DPassDrawListTag = rhiSystem->GetDrawListTagRegistry()->FindTag(AZ::Name(DrawList2DPass));
}

void CAtomShimDynamicDraw::BeginFrame()
{
    using namespace AZ;

    // In case the RenderPipeline wasn't rendered after last BeginFrame/EndFrame
    uint64_t currentTick = RPI::RPISystemInterface::Get()->GetCurrentTick();
    if (m_lastRenderTick == currentTick)
    {
        return;
    }
    m_lastRenderTick = currentTick;
    m_inFrame = true;

    m_processSrgs.clear();

    {
        RHI::BufferPool* pool = azrtti_cast<RHI::BufferPool*>(m_vertexBuffer->GetPool());
        pool->OrphanBuffer(*m_vertexBuffer);
        RHI::BufferMapRequest mapRequest;
        mapRequest.m_buffer = m_vertexBuffer.get();
        mapRequest.m_byteCount = MaxVerts*sizeof(SVF_P3F_C4B_T2F);
        mapRequest.m_byteOffset = 0;
        RHI::BufferMapResponse mapResponse;
        RHI::ResultCode resultCode = pool->MapBuffer(mapRequest, mapResponse);
        if (resultCode == RHI::ResultCode::Success)
        {
            m_mappedVertexPtr = (SVF_P3F_C4B_T2F*)mapResponse.m_data;
        }
        else
        {
            m_mappedVertexPtr = nullptr;
        }
        m_vertexCount = 0;
    }

    {
        RHI::BufferPool* pool = azrtti_cast<RHI::BufferPool*>(m_indexBuffer->GetPool());
        pool->OrphanBuffer(*m_indexBuffer);
        RHI::BufferMapRequest mapRequest;
        mapRequest.m_buffer = m_indexBuffer.get();
        mapRequest.m_byteCount = MaxIndices*sizeof(u16);
        mapRequest.m_byteOffset = 0;
        RHI::BufferMapResponse mapResponse;
        RHI::ResultCode resultCode = pool->MapBuffer(mapRequest, mapResponse);
        if (resultCode == RHI::ResultCode::Success)
        {
            m_mappedIndexPtr = (u16*)mapResponse.m_data;
        }
        else
        {
            m_mappedIndexPtr = nullptr;
        }
        m_indexCount = 0;
    }
    m_currentViewProj = AZ::Matrix4x4::CreateIdentity();
    m_drawCount = 0;
}

void CAtomShimDynamicDraw::EndFrame()
{
    if (!m_inFrame)
    {
        return;
    }
    m_inFrame = false;
    using namespace AZ;
    if (m_mappedVertexPtr)
    {
        RHI::BufferPool* pool = azrtti_cast<RHI::BufferPool*>(m_vertexBuffer->GetPool());
        pool->UnmapBuffer(*m_vertexBuffer.get());
        m_mappedVertexPtr = nullptr;
    }
    if (m_mappedIndexPtr)
    {
        RHI::BufferPool* pool = azrtti_cast<RHI::BufferPool*>(m_indexBuffer->GetPool());
        pool->UnmapBuffer(*m_indexBuffer.get());
        m_mappedIndexPtr = nullptr;   
    }
}

bool CAtomShimDynamicDraw::CreateFontImage(AtomShimTexture* texture, AZ::s32 width, AZ::s32 height, AZ::u8* pData, ETEX_Format format, [[maybe_unused]] bool genMips, const char* textureName)
{
    using namespace AZ;

    if (!texture)
    {
        return false;
    }

    RHI::Format rhiImageFormat = RHI::Format::Unknown;
    RHI::Format rhiViewFormat = RHI::Format::Unknown;
    if (format == eTF_A8)
    {
        rhiImageFormat = RHI::Format::R8_UNORM;
        rhiViewFormat = RHI::Format::R8_UNORM;
    }
    else if (format == eTF_R8G8B8A8)
    {
        rhiImageFormat = RHI::Format::R8G8B8A8_UNORM;
        rhiViewFormat = RHI::Format::R8G8B8A8_UNORM;
    }
    else if (format == eTF_B8G8R8A8)
    {
        rhiImageFormat = RHI::Format::B8G8R8A8_UNORM;
        rhiViewFormat = RHI::Format::R8G8B8A8_UNORM;
    }
    else
    {
        AZ_Assert(false, "Unsupported ETEX_Format: %d", format);
        return false;
    }

    RHI::Factory& factory = RHI::Factory::Get();
    RHI::ResultCode result = RHI::ResultCode::Success;

    RHI::Ptr<RHI::Image> image = factory.CreateImage();
    image->SetName(Name(textureName));

    RPI::DynamicDrawInterface* dynamicDraw = RPI::GetDynamicDraw();

    RHI::ImageInitRequest initReq;
    initReq.m_image = image.get();
    initReq.m_descriptor = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::ShaderRead, width, height, rhiImageFormat);
    result = dynamicDraw->GetImagePool()->InitImage(initReq);
    if (result != RHI::ResultCode::Success)
    {
        AZ_Assert(false, "InitImage() failed!");
        return false;
    }

    RHI::ImageSubresourceRange range;
    range.m_mipSliceMin = 0;
    range.m_mipSliceMax = 0;
    range.m_arraySliceMin = 0;
    range.m_arraySliceMax = 0;
    RHI::ImageSubresourceLayoutPlaced layout;
    image->GetSubresourceLayouts(range, &layout, nullptr);

    AZStd::vector<uint8_t> imageData;
    u32 srcBytesPerRow = RHI::GetFormatSize(rhiImageFormat) * width;
    bool useNewData = srcBytesPerRow < layout.m_bytesPerImage;
    // If the source data is not aligned with the layout, we need to copy src data to new buffer with correct layout
    if (useNewData)
    {
        imageData.resize(layout.m_bytesPerImage);
        for (s32 row = 0; row < height; row++)
        {
            memcpy(&imageData[layout.m_bytesPerRow * row], &pData[srcBytesPerRow * row], srcBytesPerRow);
        }
    }

    RHI::ImageUpdateRequest imageUpdateReq;
    imageUpdateReq.m_image = image.get();
    imageUpdateReq.m_imageSubresource = RHI::ImageSubresource{0, 0};
    imageUpdateReq.m_sourceData = useNewData?imageData.data():pData;
    imageUpdateReq.m_sourceSubresourceLayout = layout;
    dynamicDraw->GetImagePool()->UpdateImageContents(imageUpdateReq);

    RHI::Ptr<RHI::ImageView> imageView = image->GetImageView(RHI::ImageViewDescriptor(rhiViewFormat));
    if(!imageView.get())
    {
        AZ_Assert(false, "Failed to acquire an image view");
        return false;
    }
    
    // Store the new image and image view in the AtomShimTexture
    texture->m_image = image;
    texture->m_imageView = imageView;
    texture->SetWidth(width);
    texture->SetHeight(height);

    return true;
}

bool CAtomShimDynamicDraw::UpdateFontImage(AZ::RHI::Ptr<AZ::RHI::Image> image, int x, int y, int uSize, int vSize, AZ::u8* pData)
{
    using namespace AZ;

    if (!image)
    {
        return false;
    }

    if (x != 0 || y != 0 || uSize != image->GetDescriptor().m_size.m_width || vSize != image->GetDescriptor().m_size.m_height)
    {
        AZ_Assert(false, "Update rectangle must cover entire image. Partial image update not currently supported!");
        return false;
    }

    RHI::ImageSubresourceRange range;
    range.m_mipSliceMin = 0;
    range.m_mipSliceMax = 0;
    range.m_arraySliceMin = 0;
    range.m_arraySliceMax = 0;
    RHI::ImageSubresourceLayoutPlaced layout;
    image->GetSubresourceLayouts(range, &layout, nullptr);

    RHI::ImageUpdateRequest imageUpdateReq;
    imageUpdateReq.m_image = image.get();
    imageUpdateReq.m_imageSubresource = RHI::ImageSubresource{ 0, 0 };
    imageUpdateReq.m_sourceData = pData;
    imageUpdateReq.m_sourceSubresourceLayout = layout;

    RPI::DynamicDrawInterface* dynamicDraw = RPI::GetDynamicDraw();
    dynamicDraw->GetImagePool()->UpdateImageContents(imageUpdateReq);

    return true;
}

void CAtomShimDynamicDraw::AddFontDraw([[maybe_unused]] const AZ::RHI::ImageView* imageView, [[maybe_unused]] AZ::RPI::Scene* scene, [[maybe_unused]] SVF_P3F_C4B_T2F* pBuf, [[maybe_unused]] uint16* pInds, [[maybe_unused]] int nVerts, [[maybe_unused]] int nInds, [[maybe_unused]] const PublicRenderPrimitiveType nPrimType)
{
    // draw disabled, ICryFont has been directly implemented on Atom by Gems/AtomLyIntegration/AtomFont.
}

void CAtomShimDynamicDraw::AddSimpleTexturedDraw(const AZ::RHI::ImageView* imageView, AZ::RPI::Scene* scene,
    SVF_P3F_C4B_T2F* pBuf, uint16* pInds, int nVerts, int nInds, const PublicRenderPrimitiveType nPrimType, bool useClamp)
{
    AZ::RPI::ShaderVariantStableId variantStableId = (useClamp) ? m_simpleTexturedShader.m_clampedImageVariantStableId : m_simpleTexturedShader.m_wrappedImageVariantStableId;

    AddDraw(m_simpleTexturedShader, imageView, scene, variantStableId, pBuf, pInds, nVerts, nInds, nPrimType);
}

void CAtomShimDynamicDraw::AddUiDraw(AZ::RPI::Scene* scene,
    IRenderer::DynUiPrimitiveList & primitives, int totalNumVertices, int totalNumIndices, const PublicRenderPrimitiveType nPrimType,
    AtomShimTexture* currentTextureForUnit[], bool clampFlagPerTextureUnit[])
{
    using namespace AZ;

    ShaderDataUi& shaderData = m_uiShader;

    AZ::RPI::ShaderVariantStableId variantStableId = shaderData.m_defaultVariantStableId;

    ShaderVariantData& shaderVariantData = GetUiShaderVariantData(variantStableId, shaderData);

    RHI::PrimitiveTopology topology;
    switch (nPrimType)
    {
    case prtLineList:
        topology = RHI::PrimitiveTopology::LineList;
        break;
    case prtTriangleList:
        topology = RHI::PrimitiveTopology::TriangleList;
        break;
    case prtTriangleStrip:
        topology = RHI::PrimitiveTopology::TriangleStrip;
        break;
    default:
        AZ_Assert(false, "Unsupported primitive topology. Skipping draw.");
        return;
    }

    // get the appropriate pipeline state for this shader/scene/topology, if not yet setup then set it up
    AZ::RHI::ConstPtr<AZ::RHI::PipelineState> pipelineState = GetUiPipelineState(scene, shaderData, shaderVariantData, topology);
    if (!pipelineState)
    {
        return;
    }

    RPI::DynamicDrawInterface* dynamicDraw = RPI::GetDynamicDraw(scene);
    if (!dynamicDraw)
    {
        AZ_WarningOnce(LogName, false, "CAtomShimDynamicDraw::AddDraw being used for a scene which has no DynamcDrawFeatureProcessor");
        return;
    }

    AZ_Assert(m_mappedVertexPtr && m_mappedIndexPtr, "Vertex and Index buffer must be mapped. Perhaps AddUiDraw is being called outside Begin() and End()?");

    if ((m_vertexCount + totalNumVertices) >= CAtomShimDynamicDraw::MaxVerts || (m_indexCount + totalNumIndices) >= CAtomShimDynamicDraw::MaxIndices)
    {
        return; // [GFX TODO] instead of just skipping the draw: orphan the buffer, reset fd->m_vertexCount and fd->m_indexcount and map new buffers?
    }

    RHI::DrawPacketBuilder drawPacketBuilder;
    drawPacketBuilder.Begin(nullptr);
    if (totalNumIndices > 0)
    {
        RHI::DrawIndexed drawIndexed;
        drawIndexed.m_indexOffset = m_indexCount;
        drawIndexed.m_indexCount = totalNumIndices;
        drawIndexed.m_vertexOffset = m_vertexCount;
        drawPacketBuilder.SetDrawArguments(drawIndexed);
        drawPacketBuilder.SetIndexBufferView(m_indexBufferView);

        // When copying the indices we have to adjust them to be the correct index in the combined vertex buffer
        // The drawIndexed above already has an offset for previous draws.
        // [GFX TODO] This is mildly expensive (compared to a memcpy) but required because we are combining many
        // vertex buffers into one so the indices need to change. Investigate alternatives.
        AZ::u32 indexOffset = m_indexCount;
        AZ::u16 vbOffset = 0;
        for (const IRenderer::DynUiPrimitive& primitive : primitives)
        {
            for (int i = 0; i < primitive.m_numIndices; ++i)
            {
                m_mappedIndexPtr[indexOffset+i] = primitive.m_indices[i] + vbOffset;
            }
            indexOffset += primitive.m_numIndices;
            vbOffset += primitive.m_numVertices;
        }

        m_indexCount += totalNumIndices;
    }
    else
    {
        RHI::DrawLinear drawLinear;
        drawLinear.m_vertexCount = totalNumVertices;
        drawLinear.m_vertexOffset = m_vertexCount;
        drawPacketBuilder.SetDrawArguments(drawLinear);
    }

    // [GFX TODO] [ATOM-2333] Try to avoid doing SRG create/compile per draw.
    auto srg = AZ::RPI::ShaderResourceGroup::Create(shaderData.m_perDrawSrgAsset);
    if (!srg)
    {
        AZ_Error(LogName, false, "Failed to create shader resource group");
        return;
    }

    // set textures
    for (int textureIndex = 0; textureIndex < MaxUiTextures; ++textureIndex)
    {
        AtomShimTexture* atomTexture = currentTextureForUnit[textureIndex];
        if (atomTexture && atomTexture->m_imageView.get())
        {
            const RHI::ImageView* imageView = atomTexture->m_imageView.get();
            srg->SetImageView(shaderData.m_imageInputIndex, imageView, textureIndex);

            AZ::RHI::SamplerState samplerState;
            RHI::AddressMode adddressMode = (clampFlagPerTextureUnit[textureIndex]) ? RHI::AddressMode::Clamp : RHI::AddressMode::Wrap;
            samplerState.m_addressU = adddressMode;
            samplerState.m_addressV = adddressMode;
            samplerState.m_addressW = adddressMode;

            srg->SetSampler(shaderData.m_samplerInputIndex, samplerState, textureIndex);
        }
    }

    srg->SetShaderVariantKeyFallbackValue(shaderData.m_shaderVariantKeyFallback);
    srg->SetConstant(shaderData.m_viewProjInputIndex, m_currentViewProj);
    srg->Compile();
    drawPacketBuilder.AddShaderResourceGroup(srg->GetRHIShaderResourceGroup());
    m_processSrgs.push_back(srg);

    AZ::u32 vertexOffset = m_vertexCount;
    for (const IRenderer::DynUiPrimitive& primitive : primitives)
    {
        SVF_P2F_C4B_T2F_F4B* baseVert = (SVF_P2F_C4B_T2F_F4B*)&m_mappedVertexPtr[vertexOffset];
        memcpy(baseVert, primitive.m_vertices, primitive.m_numVertices * sizeof(SVF_P2F_C4B_T2F_F4B));
        vertexOffset += primitive.m_numVertices;
    }

    m_vertexCount += totalNumVertices;    

    RHI::DrawPacketBuilder::DrawRequest drawRequest;
    drawRequest.m_listTag = shaderVariantData.m_drawListTag;
    drawRequest.m_streamBufferViews = m_streamBufferViews;
    drawRequest.m_pipelineState = pipelineState.get();

    // [GFX TODO] It is possible that we don't need the m_drawCount at all since items with the same sort key will
    // render in the order they were added.
    drawRequest.m_sortKey = m_drawCount;
    drawPacketBuilder.AddDrawItem(drawRequest);

    AZStd::unique_ptr<const RHI::DrawPacket> drawPacket(drawPacketBuilder.End());
    dynamicDraw->AddDrawPacket(AZStd::move(drawPacket));

    m_drawCount++;
}

void CAtomShimDynamicDraw::SetUiOptions(byte eCo, byte eAo)
{
    int alphaOp = eAo;
    int colorOp = eCo;

    // The alphaOp and colorOp values are set in LyShine's RenderGraph to specific values that correspond
    // to the values checked in the legacy renderer. See LyShine's RenderGraph.cpp for more detail.
    // Since we can't include that enum definition here we use these consts for clarity.
    const int ColorOp_Normal = 1;
    const int ColorOp_PreMultiplyAlpha = 2;

    const int AlphaOp_Normal = 1;
    const int AlphaOp_ModulateAlpha = 2;
    const int AlphaOp_ModulateAlphaAndColor = 3;

    // [GFX TODO] these settings are not yet used to select the correct shader variant.
    // Need to decide whether to lookup the shader variant on each draw or cache the variant index for every combination.
    m_uiUsePreMultipliedAlpha = (colorOp == ColorOp_PreMultiplyAlpha);

    m_uiModulateOption = UiModulate::None;
    switch (alphaOp)
    {
    case AlphaOp_ModulateAlpha:
        m_uiModulateOption = UiModulate::ModulateAlpha;
        break;
    case AlphaOp_ModulateAlphaAndColor:
        m_uiModulateOption = UiModulate::ModulateAlphaAndColor;
        break;
    }

    // Note there is more state that could be relevant in :
    //   m_RP.m_CurState
    //   m_RP.m_CurAlphaRef
}

void CAtomShimDynamicDraw::AddDraw(ShaderData& shaderData, const AZ::RHI::ImageView* imageView, AZ::RPI::Scene* scene,
    AZ::RPI::ShaderVariantStableId variantStableId,
    SVF_P3F_C4B_T2F* pBuf, uint16* pInds, int nVerts, int nInds, const PublicRenderPrimitiveType nPrimType)
{
    using namespace AZ;

    if (!imageView)
    {
        return;
    }

    if (!m_inFrame)
    {
        return;
    }

    ShaderVariantData& shaderVariantData = GetShaderVariantData(variantStableId, shaderData);

    RHI::DrawListTag drawListTag = m_2DMode ? m_2DPassDrawListTag : shaderVariantData.m_drawListTag;
    if (!drawListTag.IsValid())
    { 
        return;
    }

    RHI::PrimitiveTopology topology;
    switch (nPrimType)
    {
    case prtLineList:
        topology = RHI::PrimitiveTopology::LineList;
        break;
    case prtTriangleList:
        topology = RHI::PrimitiveTopology::TriangleList;
        break;
    case prtTriangleStrip:
        topology = RHI::PrimitiveTopology::TriangleStrip;
        break;
    default:
        AZ_Assert(false, "Unsupported primitive topology. Skipping draw.");
        return;
    }

    // get the appropriate pipeline state for this shader/scene/topology, if not yet setup then set it up
    AZ::RHI::ConstPtr<AZ::RHI::PipelineState> pipelineState = GetPipelineState(scene, shaderData, shaderVariantData, topology);
    if (!pipelineState)
    {
        return;
    }

    RPI::DynamicDrawInterface* dynamicDraw = RPI::GetDynamicDraw(scene);
    if (!dynamicDraw)
    {
        AZ_WarningOnce(LogName, false, "CAtomShimDynamicDraw::AddDraw being used for a scene which has no DynamcDrawFeatureProcessor");
        return;
    }

    AZ_Assert(m_mappedVertexPtr && m_mappedIndexPtr, "Vertex and Index buffer must be mapped. Perhaps AddDraw is being called outside Begin() and End()?");

    if ((m_vertexCount + nVerts) >= CAtomShimDynamicDraw::MaxVerts || (m_indexCount + nInds) >= CAtomShimDynamicDraw::MaxIndices)
    {
        return; // [GFX TODO] instead of just skipping the draw: orphan the buffer, reset fd->m_vertexCount and fd->m_indexcount and map new buffers?
    }

    RHI::DrawPacketBuilder drawPacketBuilder;
    drawPacketBuilder.Begin(nullptr);
    if (nInds > 0)
    {
        RHI::DrawIndexed drawIndexed;
        drawIndexed.m_indexOffset = m_indexCount;
        drawIndexed.m_indexCount = nInds;
        drawIndexed.m_vertexOffset = m_vertexCount;
        drawPacketBuilder.SetDrawArguments(drawIndexed);
        drawPacketBuilder.SetIndexBufferView(m_indexBufferView);

        memcpy(&m_mappedIndexPtr[m_indexCount], pInds, nInds*sizeof(u16));
        m_indexCount += nInds;
    }
    else
    {
        RHI::DrawLinear drawLinear;
        drawLinear.m_vertexCount = nVerts;
        drawLinear.m_vertexOffset = m_vertexCount;
        drawPacketBuilder.SetDrawArguments(drawLinear);
    }

    // [GFX TODO] [ATOM-2333] Try to avoid doing SRG create/compile per draw.
    auto srg = AZ::RPI::ShaderResourceGroup::Create(shaderData.m_perDrawSrgAsset);
    if (!srg)
    {
        AZ_Error(LogName, false, "Failed to create shader resource group");
        return;
    }

    srg->SetShaderVariantKeyFallbackValue(shaderVariantData.m_shaderVariantKeyFallback);
    srg->SetConstant(shaderData.m_viewProjInputIndex, m_currentViewProj);
    srg->SetImageView(shaderData.m_imageInputIndex, imageView);
    srg->Compile();
    drawPacketBuilder.AddShaderResourceGroup(srg->GetRHIShaderResourceGroup());
    m_processSrgs.push_back(srg);

    SVF_P3F_C4B_T2F* baseVert = (SVF_P3F_C4B_T2F*)&m_mappedVertexPtr[m_vertexCount];
    memcpy(baseVert, pBuf, nVerts * sizeof(SVF_P3F_C4B_T2F));
    m_vertexCount += nVerts;    

    RHI::DrawPacketBuilder::DrawRequest drawRequest;
    drawRequest.m_listTag = drawListTag;
    drawRequest.m_streamBufferViews = m_streamBufferViews;
    drawRequest.m_pipelineState = pipelineState.get();

    // The big hex constant is a quick hack to sort the draws to the end of the queue so 2d rendering is on top of 3D.
    // We only need the constant if we are rendering in the forward pass (i.e. m_2DMode is false).
    // [GFX TODO] It is possible that we don't need the m_drawCount at all since items with the same sort key will
    // render in the order they were added.
    drawRequest.m_sortKey = (m_2DMode) ? m_drawCount : 0xffffffffff000000 + m_drawCount;
    drawPacketBuilder.AddDrawItem(drawRequest);

    AZStd::unique_ptr<const RHI::DrawPacket> drawPacket(drawPacketBuilder.End());
    dynamicDraw->AddDrawPacket(AZStd::move(drawPacket));

    m_drawCount++;
}

void CAtomShimDynamicDraw::LoadShader(const char* shaderFilepath, ShaderData& outShaderData)
{
    using namespace AZ;

    outShaderData.m_shaderFilepath = shaderFilepath;

    outShaderData.m_shader = RPI::LoadShader(shaderFilepath);

    // SRGs ...
    outShaderData.m_perDrawSrgAsset = outShaderData.m_shader->FindShaderResourceGroupAsset(AZ::Name{ "InstanceSrg" });
    if (!outShaderData.m_perDrawSrgAsset.GetId().IsValid())
    {
        AZ_Error(LogName, false, "Failed to get shader resource group asset");
        return;
    }
    else if (!outShaderData.m_perDrawSrgAsset.IsReady())
    {
        AZ_Error(LogName, false, "Shader resource group asset is not loaded");
        return;
    }

    const RHI::ShaderResourceGroupLayout* shaderResourceGroupLayout = outShaderData.m_perDrawSrgAsset->GetLayout();

    outShaderData.m_imageInputIndex = shaderResourceGroupLayout->FindShaderInputImageIndex(Name{"m_texture"});
    outShaderData.m_viewProjInputIndex = shaderResourceGroupLayout->FindShaderInputConstantIndex(Name{"m_worldToProj"});

    // variant for fonts
    {
        auto shaderOption = outShaderData.m_shader->CreateShaderOptionGroup();
        shaderOption.SetUnspecifiedToDefaultValues();
        shaderOption.SetValue(AZ::Name("o_clamp"), AZ::Name("true"));
        shaderOption.SetValue(AZ::Name("o_useColorChannels"), AZ::Name("false"));
        const auto findVariantResult = outShaderData.m_shader->FindVariantStableId(shaderOption.GetShaderVariantId());
        AZ_Warning(LogName, findVariantResult.IsFullyBaked(), "Variant not found. Defaulting to root variant");
        outShaderData.m_fontVariantStableId = findVariantResult.GetStableId();
        outShaderData.m_fontVariantKeyFallback = shaderOption.GetShaderVariantKeyFallbackValue();
    }

    // variant for clamped images
    {
        auto shaderOption = outShaderData.m_shader->CreateShaderOptionGroup();
        shaderOption.SetUnspecifiedToDefaultValues();
        shaderOption.SetValue(AZ::Name("o_clamp"), AZ::Name("true"));
        shaderOption.SetValue(AZ::Name("o_useColorChannels"), AZ::Name("true"));
        const auto findVariantResult = outShaderData.m_shader->FindVariantStableId(shaderOption.GetShaderVariantId());
        AZ_Warning(LogName, findVariantResult.IsFullyBaked(), "Variant not found. Defaulting to root variant");
        outShaderData.m_clampedImageVariantStableId = findVariantResult.GetStableId();
        outShaderData.m_clampedImageVariantKeyFallback = shaderOption.GetShaderVariantKeyFallbackValue();
    }

    // variant for wrapped images
    {
        auto shaderOption = outShaderData.m_shader->CreateShaderOptionGroup();
        shaderOption.SetUnspecifiedToDefaultValues();
        shaderOption.SetValue(AZ::Name("o_clamp"), AZ::Name("false"));
        shaderOption.SetValue(AZ::Name("o_useColorChannels"), AZ::Name("true"));
        const auto findVariantResult = outShaderData.m_shader->FindVariantStableId(shaderOption.GetShaderVariantId());
        AZ_Warning(LogName, findVariantResult.IsFullyBaked(), "Variant not found. Defaulting to root variant");
        outShaderData.m_wrappedImageVariantStableId = findVariantResult.GetStableId();
        outShaderData.m_wrappedImageVariantKeyFallback = shaderOption.GetShaderVariantKeyFallbackValue();
    }
}

void CAtomShimDynamicDraw::LoadUiShader(const char* shaderFilepath, ShaderDataUi& outShaderData)
{
    using namespace AZ;

    outShaderData.m_shaderFilepath = shaderFilepath;

    outShaderData.m_shader = RPI::LoadShader(shaderFilepath);

    // SRGs ...
    outShaderData.m_perDrawSrgAsset = outShaderData.m_shader->FindShaderResourceGroupAsset(AZ::Name{ "InstanceSrg" });
    if (!outShaderData.m_perDrawSrgAsset.GetId().IsValid())
    {
        AZ_Error(LogName, false, "Failed to get shader resource group asset");
        return;
    }
    else if (!outShaderData.m_perDrawSrgAsset.IsReady())
    {
        AZ_Error(LogName, false, "Shader resource group asset is not loaded");
        return;
    }

    const RHI::ShaderResourceGroupLayout* shaderResourceGroupLayout = outShaderData.m_perDrawSrgAsset->GetLayout();

    outShaderData.m_imageInputIndex = shaderResourceGroupLayout->FindShaderInputImageIndex(Name{"m_texture"});
    outShaderData.m_samplerInputIndex = shaderResourceGroupLayout->FindShaderInputSamplerIndex(Name{"m_sampler"});
    outShaderData.m_viewProjInputIndex = shaderResourceGroupLayout->FindShaderInputConstantIndex(Name{"m_worldToProj"});

    // variant for default test
    {
        auto shaderOption = outShaderData.m_shader->CreateShaderOptionGroup();
        shaderOption.SetUnspecifiedToDefaultValues();
        shaderOption.SetValue(AZ::Name("o_preMultiplyAlpha"), AZ::Name("true"));
        shaderOption.SetValue(AZ::Name("o_alphaTest"), AZ::Name("false"));
        shaderOption.SetValue(AZ::Name("o_srgbWrite"), AZ::Name("true"));
        shaderOption.SetValue(AZ::Name("o_modulate"), AZ::Name("Modulate::None"));
        const auto findVariantResult = outShaderData.m_shader->FindVariantStableId(shaderOption.GetShaderVariantId());
        AZ_Warning(LogName, findVariantResult.IsFullyBaked(), "Variant not found. Defaulting to root variant");
        outShaderData.m_defaultVariantStableId = findVariantResult.GetStableId();
        outShaderData.m_shaderVariantKeyFallback = shaderOption.GetShaderVariantKeyFallbackValue();
    }
}

CAtomShimDynamicDraw::ShaderVariantData& CAtomShimDynamicDraw::GetShaderVariantData(AZ::RPI::ShaderVariantStableId variantStableId, ShaderData& outShaderData)
{
    using namespace AZ;

    // If variant data for this shader variant already setup then return it
    auto variantsIter = outShaderData.m_shaderVariants.find(variantStableId);
    if (variantsIter != outShaderData.m_shaderVariants.end())
    {
        return variantsIter->second;
    }

    ShaderVariantData shaderVariantData;

    const RPI::ShaderVariant& shaderVariant = outShaderData.m_shader->GetVariant(variantStableId);
    shaderVariantData.m_variantStableId = variantStableId;
    shaderVariantData.m_drawListTag = outShaderData.m_shader->GetDrawListTag();

    if (variantStableId == outShaderData.m_fontVariantStableId)
    {
        shaderVariantData.m_shaderVariantKeyFallback = outShaderData.m_fontVariantKeyFallback;
    }
    else if (variantStableId == outShaderData.m_clampedImageVariantStableId)
    {
        shaderVariantData.m_shaderVariantKeyFallback = outShaderData.m_clampedImageVariantKeyFallback;
    }
    else if (variantStableId == outShaderData.m_wrappedImageVariantStableId)
    {
        shaderVariantData.m_shaderVariantKeyFallback = outShaderData.m_wrappedImageVariantKeyFallback;
    }

    auto entry = outShaderData.m_shaderVariants.emplace(variantStableId, shaderVariantData);

    // return a reference to the emplaced ShaderVariantData
    return entry.first->second;
}

CAtomShimDynamicDraw::ShaderVariantData& CAtomShimDynamicDraw::GetUiShaderVariantData(AZ::RPI::ShaderVariantStableId variantStableId, ShaderDataUi& outShaderData)
{
    using namespace AZ;

    // If variant data for this shader variant already setup then return it
    auto variantsIter = outShaderData.m_shaderVariants.find(variantStableId);
    if (variantsIter != outShaderData.m_shaderVariants.end())
    {
        return variantsIter->second;
    }

    ShaderVariantData shaderVariantData;

    const RPI::ShaderVariant& shaderVariant = outShaderData.m_shader->GetVariant(variantStableId);
    shaderVariantData.m_variantStableId = variantStableId;
    shaderVariantData.m_drawListTag = outShaderData.m_shader->GetDrawListTag();

    auto entry = outShaderData.m_shaderVariants.emplace(variantStableId, shaderVariantData);

    // return a reference to the emplaced ShaderVariantData
    return entry.first->second;
}

AZ::RHI::ConstPtr<AZ::RHI::PipelineState> CAtomShimDynamicDraw::GetPipelineState(const AZ::RPI::Scene* scene, const ShaderData& shaderData, ShaderVariantData& shaderVariantData,
    AZ::RHI::PrimitiveTopology topology)
{
    using namespace AZ;

    PipelineStateMapKey pipelineStatesMapKey = { topology, scene->GetId() };

    // If pipeline state for this scene/topology already setup in the variant data then return it
    auto pipelineStatesIter = shaderVariantData.m_pipelineStates.find(pipelineStatesMapKey);
    if (pipelineStatesIter != shaderVariantData.m_pipelineStates.end())
    {
        return pipelineStatesIter->second;
    }

    RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;

    const RPI::ShaderVariant& shaderVariant = shaderData.m_shader->GetVariant(shaderVariantData.m_variantStableId);

    shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

    RHI::InputStreamLayoutBuilder layoutBuilder;
    layoutBuilder.AddBuffer()
        ->Channel("POSITION", RHI::Format::R32G32B32_FLOAT)
        ->Channel("COLOR", RHI::Format::B8G8R8A8_UNORM)
        ->Channel("TEXCOORD0", RHI::Format::R32G32_FLOAT);

    layoutBuilder.SetTopology(topology);

    pipelineStateDescriptor.m_inputStreamLayout = layoutBuilder.End();

    RHI::ValidateStreamBufferViews(pipelineStateDescriptor.m_inputStreamLayout, m_streamBufferViews);

    scene->ConfigurePipelineState(shaderData.m_shader->GetDrawListTag(), pipelineStateDescriptor);

    AZ::RHI::ConstPtr<AZ::RHI::PipelineState> pipelineState = shaderData.m_shader->AcquirePipelineState(pipelineStateDescriptor);
    if (!pipelineState)
    {
        AZ_Error(LogName, false, "Failed to acquire pipeline state for shader %s", shaderData.m_shaderFilepath);
        return nullptr;
    }

    shaderVariantData.m_pipelineStates[pipelineStatesMapKey] = pipelineState;

    return pipelineState;
}

AZ::RHI::ConstPtr<AZ::RHI::PipelineState> CAtomShimDynamicDraw::GetUiPipelineState(const AZ::RPI::Scene* scene, const ShaderDataUi& shaderData, ShaderVariantData& shaderVariantData,
    AZ::RHI::PrimitiveTopology topology)
{
    using namespace AZ;

    PipelineStateMapKey pipelineStatesMapKey = { topology, scene->GetId() };

    // If pipeline state for this scene/topology already setup in the variant data then return it
    auto pipelineStatesIter = shaderVariantData.m_pipelineStates.find(pipelineStatesMapKey);
    if (pipelineStatesIter != shaderVariantData.m_pipelineStates.end())
    {
        return pipelineStatesIter->second;
    }

    RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;

    const RPI::ShaderVariant& shaderVariant = shaderData.m_shader->GetVariant(shaderVariantData.m_variantStableId);

    shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

    RHI::InputStreamLayoutBuilder layoutBuilder;
    layoutBuilder.AddBuffer()
        ->Channel("POSITION", RHI::Format::R32G32_FLOAT)
        ->Channel("COLOR", RHI::Format::B8G8R8A8_UNORM)     // The Cry UCol stores the color bytes in BGRA order
        ->Channel("TEXCOORD0", RHI::Format::R32G32_FLOAT)
        ->Channel("BLENDINDICES", RHI::Format::R16G16_UINT);

    layoutBuilder.SetTopology(topology);

    pipelineStateDescriptor.m_inputStreamLayout = layoutBuilder.End();

    RHI::ValidateStreamBufferViews(pipelineStateDescriptor.m_inputStreamLayout, m_streamBufferViews);

    scene->ConfigurePipelineState(shaderData.m_shader->GetDrawListTag(), pipelineStateDescriptor);

    AZ::RHI::ConstPtr<AZ::RHI::PipelineState> pipelineState = shaderData.m_shader->AcquirePipelineState(pipelineStateDescriptor);
    if (!pipelineState)
    {
        AZ_Error(LogName, false, "Failed to acquire pipeline state for shader %s", shaderData.m_shaderFilepath);
        return nullptr;
    }

    shaderVariantData.m_pipelineStates[pipelineStatesMapKey] = pipelineState;

    return pipelineState;
}

bool CAtomShimRenderer::FontUploadTexture([[maybe_unused]] class CFBitmap* pBmp, [[maybe_unused]] ETEX_Format eTF)
{
    AZ_Warning(LogName, false, "FontUploadTexture(class CFBitmap* pBmp, ETEX_Format eTF) not yet supported!");
    return false;
}

int CAtomShimRenderer::FontCreateTexture(int Width, int Height, byte* pData, ETEX_Format eTF, bool genMips, const char* textureName)
{
    // These flags are what is used in D3DFont.cpp. They are probably irrelevant here
    int iFlags = FT_TEX_FONT | FT_DONT_STREAM;
    if (genMips)
    {
        iFlags |= FT_FORCE_MIPS;
    }

    // Create a new AtomShimTexture
    AtomShimTexture* texture = new AtomShimTexture(iFlags);
    texture->Register(CTexture::mfGetClassName(), textureName);
    texture->SetSourceName( textureName );

    // Create the Atom image and image view for the texture
    if (!m_pAtomShimDynamicDraw->CreateFontImage(texture, Width, Height, pData, eTF, genMips, textureName))
    {
        SAFE_RELEASE(texture);
        return -1;
    }

    return texture->GetID();
}

bool CAtomShimRenderer::FontUpdateTexture(int nTexId, int X, int Y, int USize, int VSize, byte* pData)
{
    // Find the existing texture for this id
    AtomShimTexture* atomTexture = CastITextureToAtomShimTexture(EF_GetTextureByID(nTexId));
    if (!(atomTexture && atomTexture->m_image))
    {
        return false;
    }

    // Update the Atom image for the texture
    return m_pAtomShimDynamicDraw->UpdateFontImage(atomTexture->m_image, X, Y, USize, VSize, pData);
}

void CAtomShimRenderer::FontReleaseTexture(class CFBitmap* /*pBmp*/)
{
    AZ_Warning(LogName, false, "FontReleaseTexture(class CFBitmap* pBmp, int nTexFiltMode) not yet supported!");
}

void CAtomShimRenderer::FontSetTexture(class CFBitmap* /*pBmp*/, int /*nTexFiltMode*/)
{
    // see D3DFont.cpp. Note that the legacy function stores the CTexture in pBmp->GetRenderData()
    AZ_Warning(LogName, false, "FontSetTexture(class CFBitmap* pBmp, int nTexFiltMode) not yet supported!");
}

void CAtomShimRenderer::FontSetTexture(int nTexId, int /*nFilterMode*/)
{
   m_currentFontTextureId = nTexId;
}

void CAtomShimRenderer::FontSetRenderingState(bool overrideViewProjMatrices, TransformationMatrices& backupMatrices)
{
    AZ_Assert(m_isInFontMode == 0, "already in font mode!");
    m_isInFontMode++;   

    AZ_Assert(m_pAtomShimDynamicDraw, "FontRenderData not created!");

    Matrix44A matView, matProj;
    GetModelViewMatrix(matView.GetData());
    GetProjectionMatrix(matProj.GetData());

    if (overrideViewProjMatrices)
    {
        backupMatrices.m_viewMatrix = matView;
        backupMatrices.m_projectMatrix = matProj;
        
        const bool bReverseDepth = false;        // [GFX TODO]: un-hardcode this and query it from Atom?
        const float zn = bReverseDepth ? 1.0f : -1.0f;
        const float zf = bReverseDepth ? -1.0f : 1.0f;

        int x,y,width,height;
        GetViewport(&x, &y, &width, &height);

        matView.SetIdentity();

        if (width != 0 && height != 0)
        {
            mathMatrixOrthoOffCenter(&matProj, (float)x, (float)(x+width), (float)(y+height), (float)y, zn, zf);
        }
        else
        {
            mathMatrixOrthoOffCenter(&matProj, 0.0f, (float)m_width, (float)m_height, 0.0f, zn, zf); //TEST
        }

        SetMatrices(matProj.GetData(), matView.GetData());

        m_pAtomShimDynamicDraw->Set2DMode(true);
    }

    Matrix44A matViewProj = matView*matProj;
    m_pAtomShimDynamicDraw->SetCurrentViewProj( AZ::Matrix4x4::CreateFromColumnMajorFloat16(matViewProj.GetData()) );
}

void CAtomShimRenderer::FontSetBlending([[maybe_unused]] int src, [[maybe_unused]] int dst, [[maybe_unused]] int baseState)
{

}

void CAtomShimRenderer::FontRestoreRenderingState(bool overrideViewProjMatrices, const TransformationMatrices& restoringMatrices)
{
    AZ_Assert(m_isInFontMode == 1, "not in font mode!");
    m_isInFontMode--;

    if (overrideViewProjMatrices)
    {
        SetMatrices(const_cast<float*>(restoringMatrices.m_projectMatrix.GetData()), 
            const_cast<float*>(restoringMatrices.m_viewMatrix.GetData()));

        m_pAtomShimDynamicDraw->Set2DMode(false);
    }
    m_pAtomShimDynamicDraw->SetCurrentViewProj(AZ::Matrix4x4::CreateIdentity());
}

void CAtomShimRenderer::DrawStringU([[maybe_unused]] IFFont_RenderProxy* pFont, [[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] float z, [[maybe_unused]] const char* pStr, [[maybe_unused]] const bool asciiMultiLine, [[maybe_unused]] const STextDrawContext& ctx) const
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

    AZ::RPI::Scene* scene = (m_currContext) ? m_currContext->m_scene : nullptr;
    if (!scene)
    {
        scene = RPI::RPISystemInterface::Get()->GetDefaultScene().get();
    }

    if (m_isInFontMode)
    {
        if (m_currentFontTextureId < 0)
        {
            AZ_Warning(LogName, false, "m_currentFontTextureId is invalid! FontSetTexture() must be called before DrawDynVB() for a font draw");
            return;
        }

        AtomShimTexture* atomTexture = CastITextureToAtomShimTexture(EF_GetTextureByID(m_currentFontTextureId));
        if (atomTexture && atomTexture->m_image.get())
        {
            const RHI::ImageView* imageView = atomTexture->m_imageView.get();
            m_pAtomShimDynamicDraw->AddFontDraw(imageView, scene, pBuf, pInds, nVerts, nInds, nPrimType);
        }
    }
    else
    {
        // Set the viewProj matrix for m_pAtomShimDynamicDraw
        Matrix44A matView, matProj;
        GetModelViewMatrix(matView.GetData());
        GetProjectionMatrix(matProj.GetData());
        Matrix44A matViewProj = matView*matProj;
        m_pAtomShimDynamicDraw->SetCurrentViewProj( AZ::Matrix4x4::CreateFromColumnMajorFloat16(matViewProj.GetData()) );

        AtomShimTexture* atomTexture = m_currentTextureForUnit[0];
        if (atomTexture && atomTexture->m_imageView)
        {
            const RHI::ImageView* imageView = atomTexture->m_imageView.get();
            bool isClamp = m_clampFlagPerTextureUnit[0];
            m_pAtomShimDynamicDraw->AddSimpleTexturedDraw(imageView, scene, pBuf, pInds, nVerts, nInds, nPrimType, isClamp);
        }

        // Reset the viewProj matrix for m_pAtomShimDynamicDraw
        m_pAtomShimDynamicDraw->SetCurrentViewProj(AZ::Matrix4x4::CreateIdentity());
    }
}

void CAtomShimRenderer::DrawDynUiPrimitiveList(DynUiPrimitiveList& primitives, int totalNumVertices, int totalNumIndices)
{
    using namespace AZ;

    AZ::RPI::Scene* scene = (m_currContext) ? m_currContext->m_scene : nullptr;
    if (!scene)
    {
        scene = RPI::RPISystemInterface::Get()->GetDefaultScene().get();
    }

    Matrix44A matView, matProj;
    GetModelViewMatrix(matView.GetData());
    GetProjectionMatrix(matProj.GetData());
    Matrix44A matViewProj = matView*matProj;
    m_pAtomShimDynamicDraw->SetCurrentViewProj( AZ::Matrix4x4::CreateFromColumnMajorFloat16(matViewProj.GetData()) );

    m_pAtomShimDynamicDraw->AddUiDraw(scene, primitives, totalNumVertices, totalNumIndices, prtTriangleList, m_currentTextureForUnit, m_clampFlagPerTextureUnit);
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
    m_pAtomShimDynamicDraw->Set2DMode(m_isIn2dModeCounter > 0);
    if(m_isIn2dModeCounter > 0)
    {
        //We're still in 2d mode, so set the viewProjOverride to the current matrix
        //For 2d drawing, the view matrix is an identity matrix, so viewProj == proj
        AZ::Matrix4x4 viewProj = AZ::Matrix4x4::CreateFromColumnMajorFloat16(m_RP.m_TI[nThreadID].m_matProj.GetData());
        m_pAtomShimRenderAuxGeom->SetViewProjOverride(viewProj);
    }
    else
    {
        m_pAtomShimRenderAuxGeom->UnsetViewProjOverride();
    }
}

void CAtomShimRenderer::Set2DModeNonZeroTopLeft(float orthoLeft, float orthoTop, float orthoWidth, float orthoHeight, TransformationMatrices& backupMatrices, float znear, float zfar)
{
    int nThreadID = m_pRT->GetThreadList();

#ifdef _DEBUG
    // Increment the counter used to check that Set2DMode and Unset2DMode are balanced.
    // It should never be negative before the increment.
    AZ_Assert(s_isIn2DMode[nThreadID]++ >= 0, "Calls to Set2DMode and Unset2DMode appear mismatched");
#endif

    backupMatrices.m_projectMatrix = m_RP.m_TI[nThreadID].m_matProj;

    //Move the zfar a bit away from the znear if they're the same.
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
    m_pAtomShimDynamicDraw->Set2DMode(true);

    //For 2d drawing, the view matrix is an identity matrix, so viewProj == proj
    AZ::Matrix4x4 viewProj = AZ::Matrix4x4::CreateFromColumnMajorFloat16(m_RP.m_TI[nThreadID].m_matProj.GetData());
    m_pAtomShimRenderAuxGeom->SetViewProjOverride(viewProj);
}


void CAtomShimRenderer::SetColorOp(byte eCo, byte eAo, [[maybe_unused]] byte eCa, [[maybe_unused]] byte eAa)
{
    m_pAtomShimDynamicDraw->SetUiOptions(eCo, eAo);
}

// [GFX TODO] For text rendering, may also need to implement:
//  called from FFont.cpp:
//   DeleteFont(IFFont *)
//   RemoveTexture(m_texID)
//   DrawStringU(IFFont_RenderProxy *pFont, ..., STextDrawContext &ctx)
//   ScaleCoord
//   EF_Query(EFQ_OverscanBorders, Vec2& border)
//   FontSetBlending()
//   ScaleCoordX
//  called from CryFont.cpp:
//   m_rndPropIsRGBA = (pRenderer->GetFeatures() & RFT_RGBA) != 0;
//  CGlphyCache depends on CFontRenderer, which includes a bunch of freetype stuff, but seems to be CPU only

// [GFX TODO] For generic 2d drawing, may also need to implement:
//   SetState
//   DrawImage
//   Push/PopProfileMarker
