// ----------------------------------------------------------------------------
// Interface for the shortcut method.
// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <stdint.h>

#include "EngineInterface.h"
#include "TressFXLayouts.h"
#include "TressFXShortCut.h"
#include "HairStrands.h"
#include "TressFXHairObject.h"

using namespace AMD;

TressFXShortCut::TressFXShortCut() :
    m_nScreenWidth(0),
    m_nScreenHeight(0),
    m_pDepths(),
    m_pInvAlpha(),
    m_pColors(),
    // Bind Sets
    m_pShortCutDepthsAlphaBindSet(nullptr),
    m_pShortCutDepthReadBindSet(nullptr),
    m_pShortCutColorReadBindSet(nullptr),
    // Render Pass Sets
    m_pShortCutDepthsAlphaRenderTargetSet(nullptr),
    m_pShortCutDepthResolveRenderTargetSet(nullptr),
    m_pShortCutHairColorRenderTargetSet(nullptr),
    m_pColorResolveRenderTargetSet(nullptr),
    // PSOs
    m_pDepthsAlphaPSO(nullptr),
    m_pDepthResolvePSO(nullptr),
    m_pHairColorPSO(nullptr),
    m_pHairResolvePSO(nullptr),
    // Bindsets
    m_ShadeParamsBindSet(nullptr)
{
}

TressFXShortCut::~TressFXShortCut()
{
    m_ShadeParamsConstantBuffer.Reset();
}

void TressFXShortCut::Initialize(int width, int height)
{
    Create(GetDevice(), width, height);

    // Create constant buffer and bind set
    m_ShadeParamsConstantBuffer.CreateBufferResource("TressFXShadeParams");
    EI_BindSetDescription set = { { m_ShadeParamsConstantBuffer.GetBufferResource() } };
    m_ShadeParamsBindSet = GetDevice()->CreateBindSet(GetShortCutShadeParamLayout(), set);

    // Depth Alpha Pass
    {
        //std::vector<VkVertexInputAttributeDescription> DepthAlphaDesc;

        EI_PSOParams psoParams;
        psoParams.primitiveTopology = EI_Topology::TriangleList;
        psoParams.colorWriteEnable = true;
        psoParams.depthTestEnable = true;
        psoParams.depthWriteEnable = false;
        psoParams.depthCompareOp = EI_CompareFunc::LessEqual;

        psoParams.colorBlendParams.colorBlendEnabled = true;
        psoParams.colorBlendParams.colorBlendOp = EI_BlendOp::Add;
        psoParams.colorBlendParams.colorSrcBlend = EI_BlendFactor::Zero;
        psoParams.colorBlendParams.colorDstBlend = EI_BlendFactor::SrcColor;
        psoParams.colorBlendParams.alphaBlendOp = EI_BlendOp::Add;
        psoParams.colorBlendParams.alphaSrcBlend = EI_BlendFactor::Zero;
        psoParams.colorBlendParams.alphaDstBlend = EI_BlendFactor::SrcAlpha;

        EI_BindLayout* DepthAlphaLayouts[] = { GetTressFXParamLayout(), GetRenderPosTanLayout(), GetViewLayout(), GetShortCutDepthsAlphaLayout(), GetSamplerLayout() };
        psoParams.layouts = DepthAlphaLayouts;
        psoParams.numLayouts = 5;
        psoParams.renderTargetSet = m_pShortCutDepthsAlphaRenderTargetSet.get();

        m_pDepthsAlphaPSO = GetDevice()->CreateGraphicsPSO("TressFXShortCut.hlsl", "RenderHairDepthAlphaVS", "TressFXShortCut.hlsl", "DepthsAlphaPS", psoParams);
    }

    // Depth resolve pass
    {
        //std::vector<VkVertexInputAttributeDescription> DepthResolveDesc;

        EI_PSOParams psoParams;
        psoParams.primitiveTopology = EI_Topology::TriangleStrip;
        psoParams.colorWriteEnable = false;
        psoParams.depthTestEnable = true;
        psoParams.depthWriteEnable = true;
        psoParams.depthCompareOp = EI_CompareFunc::LessEqual;

        psoParams.colorBlendParams.colorBlendEnabled = false;

        EI_BindLayout* DepthResolveLayouts[] = { GetShortCutDepthReadLayout() };
        psoParams.layouts = DepthResolveLayouts;
        psoParams.numLayouts = 1;
        psoParams.renderTargetSet = m_pShortCutDepthResolveRenderTargetSet.get();

        m_pDepthResolvePSO = GetDevice()->CreateGraphicsPSO("TressFXShortCut.hlsl", "FullScreenVS", "TressFXShortCut.hlsl", "ResolveDepthPS", psoParams);
    }

    // Hair Color Pass
    {
        //std::vector<VkVertexInputAttributeDescription> HairColorDesc;

        EI_PSOParams psoParams;
        psoParams.primitiveTopology = EI_Topology::TriangleList;
        psoParams.colorWriteEnable = true;
        psoParams.depthTestEnable = true;
        psoParams.depthWriteEnable = false;
        psoParams.depthCompareOp = EI_CompareFunc::LessEqual;

        psoParams.colorBlendParams.colorBlendEnabled = true;
        psoParams.colorBlendParams.colorSrcBlend = EI_BlendFactor::One;
        psoParams.colorBlendParams.colorDstBlend = EI_BlendFactor::One;
        psoParams.colorBlendParams.colorBlendOp = EI_BlendOp::Add;
        psoParams.colorBlendParams.alphaSrcBlend = EI_BlendFactor::One;
        psoParams.colorBlendParams.alphaDstBlend = EI_BlendFactor::One;
        psoParams.colorBlendParams.alphaBlendOp = EI_BlendOp::Add;

        EI_BindLayout* HairColorLayouts[] = { GetTressFXParamLayout(), GetRenderPosTanLayout(), GetViewLayout(), GetLightLayout(), GetSamplerLayout(), GetShortCutShadeParamLayout() };
        psoParams.layouts = HairColorLayouts;
        psoParams.numLayouts = 6;
        psoParams.renderTargetSet = m_pShortCutHairColorRenderTargetSet.get();

        m_pHairColorPSO = GetDevice()->CreateGraphicsPSO("TressFXShortCut.hlsl", "RenderHairColorVS", "TressFXShortCut.hlsl", "HairColorPS", psoParams);
    }

    // Hair Resolve
    {
        //std::vector<VkVertexInputAttributeDescription> descs;

        EI_PSOParams psoParams;
        psoParams.primitiveTopology = EI_Topology::TriangleStrip;
        psoParams.colorWriteEnable = true;
        psoParams.depthTestEnable = false;
        psoParams.depthWriteEnable = false;
        psoParams.depthCompareOp = EI_CompareFunc::LessEqual;

        psoParams.colorBlendParams.colorBlendEnabled = true;
        psoParams.colorBlendParams.colorBlendOp = EI_BlendOp::Add;
        psoParams.colorBlendParams.colorSrcBlend = EI_BlendFactor::One;
        psoParams.colorBlendParams.colorDstBlend = EI_BlendFactor::SrcAlpha;
        psoParams.colorBlendParams.alphaBlendOp = EI_BlendOp::Add;
        psoParams.colorBlendParams.alphaSrcBlend = EI_BlendFactor::Zero;
        psoParams.colorBlendParams.alphaDstBlend = EI_BlendFactor::Zero;

        EI_BindLayout* layouts[] = { GetShortCutColorReadLayout() };
        psoParams.layouts = layouts;
        psoParams.numLayouts = 1;
        psoParams.renderTargetSet = m_pColorResolveRenderTargetSet.get();
        m_pHairResolvePSO = GetDevice()->CreateGraphicsPSO("TressFXShortCut.hlsl", "FullScreenVS", "TressFXShortCut.hlsl", "ResolveHairPS", psoParams);
    }
}

bool TressFXShortCut::Create(EI_Device* pDevice, int width, int height)
{
    m_nScreenWidth  = width;
    m_nScreenHeight = height;

    // Create required resources
    m_pDepths = pDevice->CreateUint32Resource(width, height, TRESSFX_SHORTCUT_K, "ShortCutDepthsTexture", TRESSFX_SHORTCUT_INITIAL_DEPTH);
    AMD::float4 clearValues = { 1.f, 1.f, 1.f, 1.f };
    m_pInvAlpha = pDevice->CreateRenderTargetResource(width, height, 1, 4, "ShortCutInvAlphaTexture", &clearValues);
    clearValues = { 0.f, 0.f, 0.f, 1.f };
    m_pColors = pDevice->CreateRenderTargetResource(width, height, 4, 4, "ShortCutColorsTexture", &clearValues);

    // Create bind sets
    CreateDepthsAlphaBindSet(pDevice);
    CreateDepthReadBindSet(pDevice);
    CreateColorReadBindSet(pDevice);

    // Create RenderPasss sets
    CreateDepthsAlphaRenderTargetSet(pDevice);
    CreateDepthResolveRenderTargetSet(pDevice);
    CreateHairColorRenderTargetSet(pDevice);
    CreateColorResolveRenderTargetSet(pDevice);

    return true;
}

void TressFXShortCut::CreateDepthsAlphaBindSet(EI_Device* pDevice)
{
    EI_BindSetDescription bindSet =
    {
        { m_pDepths.get() }
    };
    m_pShortCutDepthsAlphaBindSet = pDevice->CreateBindSet(GetShortCutDepthsAlphaLayout(), bindSet);
}

void TressFXShortCut::CreateDepthReadBindSet(EI_Device* pDevice)
{
    EI_BindSetDescription bindSet =
    {
        { m_pDepths.get() }
    };
    m_pShortCutDepthReadBindSet = pDevice->CreateBindSet(GetShortCutDepthReadLayout(), bindSet);
}

void TressFXShortCut::CreateColorReadBindSet(EI_Device* pDevice)
{
    EI_BindSetDescription bindSet =
    {
        { m_pColors.get(), m_pInvAlpha.get() }
    };
    m_pShortCutColorReadBindSet = pDevice->CreateBindSet(GetShortCutColorReadLayout(), bindSet);
}

void TressFXShortCut::CreateDepthsAlphaRenderTargetSet(EI_Device* pDevice)
{
    // For shortcut depth alpha render pass, we need InvAlphaTexture bound with Depth buffer.
    const EI_Resource* ResourceArray[] = { m_pInvAlpha.get(), GetDevice()->GetDepthBufferResource() };
    const EI_AttachmentParams AttachmentParams[] = { {EI_RenderPassFlags::Load | EI_RenderPassFlags::Clear | EI_RenderPassFlags::Store},
                                                     {EI_RenderPassFlags::Depth | EI_RenderPassFlags::Load | EI_RenderPassFlags::Store} };

    float clearValues[] = { 1.f, 1.f, 1.f, 1.f };	// Color
    m_pShortCutDepthsAlphaRenderTargetSet = pDevice->CreateRenderTargetSet(ResourceArray, 2, AttachmentParams, clearValues);
}

void TressFXShortCut::CreateDepthResolveRenderTargetSet(EI_Device* pDevice)
{
    // For shortcut depth resolve render pass, we just need the depth buffer bound
    const EI_Resource* ResourceArray[] = { GetDevice()->GetDepthBufferResource() };
    const EI_AttachmentParams AttachmentParams[] = { {EI_RenderPassFlags::Depth | EI_RenderPassFlags::Load | EI_RenderPassFlags::Store} };
    m_pShortCutDepthResolveRenderTargetSet = pDevice->CreateRenderTargetSet(ResourceArray, 1, AttachmentParams, nullptr);
}

void TressFXShortCut::CreateHairColorRenderTargetSet(EI_Device* pDevice)
{
    // For shortcut hair color render pass, we need the color texture bound with Depth buffer.
    const EI_Resource* ResourceArray[] = { m_pColors.get(), GetDevice()->GetDepthBufferResource() };
    const EI_AttachmentParams AttachmentParams[] = { {EI_RenderPassFlags::Load | EI_RenderPassFlags::Clear | EI_RenderPassFlags::Store},
                                                     {EI_RenderPassFlags::Depth | EI_RenderPassFlags::Load | EI_RenderPassFlags::Store} };

    float clearValues[] = { 0.f, 0.f, 0.f, 1.f };	// Clear Color
    m_pShortCutHairColorRenderTargetSet = pDevice->CreateRenderTargetSet(ResourceArray, 2, AttachmentParams, clearValues);
}

void TressFXShortCut::CreateColorResolveRenderTargetSet(EI_Device* pDevice)
{
    const EI_Resource* ResourceArray[] = { pDevice->GetColorBufferResource(), pDevice->GetDepthBufferResource() };
    const EI_AttachmentParams AttachmentParams[] = { {EI_RenderPassFlags::Load | EI_RenderPassFlags::Store},
                                                     {EI_RenderPassFlags::Depth | EI_RenderPassFlags::Load | EI_RenderPassFlags::Store} };
    m_pColorResolveRenderTargetSet = pDevice->CreateRenderTargetSet(ResourceArray, 2, AttachmentParams, nullptr);
}

void TressFXShortCut::Clear(EI_CommandContext& context)
{
    // Consider being explicit on values here.
    // In DX, UAV counter clears are actually done when UAV is set, which makes this a bit weird, I
    // suppose.
    // Also try clearing in shader during fullscreen pass (although can't miss frames, and makes
    // reads slower)
    // or look at 0-based for fast clears.

    // Begin the render pass and transition any resources to write if needed

    if (m_firstRun)
    {
        // The first time we use the resource, we need to transition it out of UNDEFINED state to COPY_DEST.
        // Doing from PS_SRV causes validation errors.
#ifdef TRESSFX_VK
        EI_Barrier readToClear[] =
        {
            { m_pDepths.get(), EI_STATE_UNDEFINED, EI_STATE_COPY_DEST },
        };
        context.SubmitBarrier(AMD_ARRAY_SIZE(readToClear), readToClear);
#endif // TRESSFX_VK
    }
    else
    {
        EI_Barrier readToClear[] =
        {
#ifdef TRESSFX_VK
            { m_pDepths.get(), EI_STATE_SRV, EI_STATE_COPY_DEST },
#else
            { m_pDepths.get(), EI_STATE_SRV, EI_STATE_UAV },
#endif // TRESSFX_VK
        };
        context.SubmitBarrier(AMD_ARRAY_SIZE(readToClear), readToClear);
    }

    context.ClearUint32Image(m_pDepths.get(), TRESSFX_SHORTCUT_INITIAL_DEPTH);
}

void TressFXShortCut::BeginDepthsAlpha(EI_CommandContext& commandContext)
{
    // Begin the render pass and transition any resources to write if needed
    if (m_firstRun)
    {
        EI_Barrier readToWrite[] =
        {
            { m_pInvAlpha.get(), EI_STATE_UNDEFINED, EI_STATE_RENDER_TARGET }, 
#ifdef TRESSFX_VK
            { m_pDepths.get(), EI_STATE_COPY_DEST, EI_STATE_UAV },
#endif  // TRESSFX            
        };

        commandContext.SubmitBarrier(AMD_ARRAY_SIZE(readToWrite), readToWrite);
    }
    else
    {
        EI_Barrier readToWrite[] =
        {
            { m_pInvAlpha.get(), EI_STATE_SRV, EI_STATE_RENDER_TARGET },
#ifdef TRESSFX_VK
            { m_pDepths.get(), EI_STATE_COPY_DEST, EI_STATE_UAV },
#endif  // TRESSFX
        };

        commandContext.SubmitBarrier(AMD_ARRAY_SIZE(readToWrite), readToWrite);
    }

    // Begin the render pass
    GetDevice()->BeginRenderPass(commandContext, m_pShortCutDepthsAlphaRenderTargetSet.get(), L"BeginDepthsAlpha Pass");
}

void TressFXShortCut::EndDepthsAlpha(EI_CommandContext& commandContext)
{
    // End the render pass
    GetDevice()->EndRenderPass(commandContext);

    EI_Barrier writeToRead[] =
    {
        { m_pInvAlpha.get(), EI_STATE_RENDER_TARGET, EI_STATE_SRV },
        { m_pDepths.get(), EI_STATE_UAV, EI_STATE_SRV },
    };

    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(writeToRead), writeToRead);
}

void TressFXShortCut::BeginDepthResolve(EI_CommandContext& commandContext)
{
    // Begin the render pass
    GetDevice()->BeginRenderPass(commandContext, m_pShortCutDepthResolveRenderTargetSet.get(), L"BeginDepthResolve Pass");
}

void TressFXShortCut::EndDepthResolve(EI_CommandContext& commandContext)
{
    // End the render pass
    GetDevice()->EndRenderPass(commandContext);
}

void TressFXShortCut::BeginHairColor(EI_CommandContext& commandContext)
{
    // Begin the render pass and transition any resources to write if needed
    if (m_firstRun)
    {
        EI_Barrier readToWrite[] =
        {
            { m_pColors.get(), EI_STATE_UNDEFINED, EI_STATE_RENDER_TARGET },
        };

        commandContext.SubmitBarrier(AMD_ARRAY_SIZE(readToWrite), readToWrite);
    }
    else
    {
        EI_Barrier readToWrite[] =
        {
            { m_pColors.get(), EI_STATE_SRV, EI_STATE_RENDER_TARGET },
        };

        commandContext.SubmitBarrier(AMD_ARRAY_SIZE(readToWrite), readToWrite);
    }

    // Begin the render pass
    GetDevice()->BeginRenderPass(commandContext, m_pShortCutHairColorRenderTargetSet.get(), L"BeginHairColor Pass");
}

void TressFXShortCut::EndHairColor(EI_CommandContext& commandContext)
{
    // End the render pass
    GetDevice()->EndRenderPass(commandContext);

    // Transition resources to read for next pass
    EI_Barrier writeToRead[] =
    {
        { m_pColors.get(), EI_STATE_RENDER_TARGET, EI_STATE_SRV },
    };

    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(writeToRead), writeToRead);
}

void TressFXShortCut::BeginColorResolve(EI_CommandContext& commandContext)
{
    // Color/Depth should already be in a write state
    // hair colors and inv alpha should also already be in a read state

    // Begin the render pass
    GetDevice()->BeginRenderPass(commandContext, m_pColorResolveRenderTargetSet.get(), L"BeginColorResolve");
}

void TressFXShortCut::EndColorResolve(EI_CommandContext& commandContext)
{
    // End the render pass
    GetDevice()->EndRenderPass(commandContext);
}

void TressFXShortCut::DrawHairStrands(EI_CommandContext& commandContext, int numHairStrands, HairStrands** hairStrands, EI_PSO* pPSO, EI_BindSet** extraBindSets, uint32_t numExtraBindSets)
{
    // Loop through all hair strands and render them
    for (size_t i = 0; i < numHairStrands; i++)
    {
        if (hairStrands[i]->GetTressFXHandle())
            hairStrands[i]->GetTressFXHandle()->DrawStrands(commandContext, *pPSO, extraBindSets, numExtraBindSets);
    }
}

void TressFXShortCut::Draw(EI_CommandContext& commandContext, int numHairStrands, HairStrands** hairStrands, EI_BindSet* viewBindSet, EI_BindSet* lightBindSet)
{
    // Clear out depth texture array
    Clear(commandContext);

    // Render the DepthAlpha pass
    BeginDepthsAlpha(commandContext);
    {
        EI_BindSet* ExtraBindSets[] = { viewBindSet, m_pShortCutDepthsAlphaBindSet.get(), GetDevice()->GetSamplerBindSet() };
        DrawHairStrands(commandContext, numHairStrands, hairStrands, m_pDepthsAlphaPSO.get(), ExtraBindSets, 3);
    }
    EndDepthsAlpha(commandContext);
    GetDevice()->GetTimeStamp("Shortcut DepthAlpha");

    // Depth Resolve pass
    BeginDepthResolve(commandContext);
    {
        EI_BindSet* BindSets[] = { m_pShortCutDepthReadBindSet.get() };
        GetDevice()->DrawFullScreenQuad(commandContext, *m_pDepthResolvePSO, BindSets, 1);
    }
    EndDepthResolve(commandContext);
    GetDevice()->GetTimeStamp("Shortcut DepthAlpha Resolve");

    // Render the color pass (hair render)
    BeginHairColor(commandContext);
    {
        EI_BindSet* ExtraBindSets[] = { viewBindSet, lightBindSet, GetDevice()->GetSamplerBindSet(), m_ShadeParamsBindSet.get() };
        DrawHairStrands(commandContext, numHairStrands, hairStrands, m_pHairColorPSO.get(), ExtraBindSets, 4);
    }
    EndHairColor(commandContext);
    GetDevice()->GetTimeStamp("Shortcut Hair Pass");

    // Apply hair to main target
    BeginColorResolve(commandContext);
    {
        EI_BindSet* BindSets[] = { m_pShortCutColorReadBindSet.get() };
        GetDevice()->DrawFullScreenQuad(commandContext, *m_pHairResolvePSO, BindSets, 1);
    }
    EndColorResolve(commandContext);
    GetDevice()->GetTimeStamp("Shortcut Hair Apply");

    m_firstRun = false;
}

void TressFXShortCut::UpdateShadeParameters(std::vector<const TressFXRenderingSettings*>& renderSettings)
{
    // Update Render Parameters
    for (int i = 0; i < renderSettings.size(); ++i)
    {
        m_ShadeParamsConstantBuffer->HairShadeParams[i].FiberRadius = renderSettings[i]->m_FiberRadius; // Don't modify radius by LOD multiplier as this one is used to calculate shadowing and that calculation should remain unaffected
        m_ShadeParamsConstantBuffer->HairShadeParams[i].ShadowAlpha = renderSettings[i]->m_HairShadowAlpha;
        m_ShadeParamsConstantBuffer->HairShadeParams[i].FiberSpacing = renderSettings[i]->m_HairFiberSpacing;
        m_ShadeParamsConstantBuffer->HairShadeParams[i].HairEx2 = renderSettings[i]->m_HairSpecExp2;
        m_ShadeParamsConstantBuffer->HairShadeParams[i].HairKs2 = renderSettings[i]->m_HairKSpec2;
        m_ShadeParamsConstantBuffer->HairShadeParams[i].MatKValue = { 0.f, renderSettings[i]->m_HairKDiffuse, renderSettings[i]->m_HairKSpec1, renderSettings[i]->m_HairSpecExp1 }; // no ambient
    }
    
    m_ShadeParamsConstantBuffer.Update(GetDevice()->GetCurrentCommandContext());
}