// ----------------------------------------------------------------------------
// Interface for TressFX OIT using per-pixel linked lists.
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

//#include "AMD_TressFX.h"

#include "TressFXLayouts.h"
#include "TressFXPPLL.h"
#include "EngineInterface.h"
#include "HairStrands.h"
#include "TressFXHairObject.h"

using namespace AMD;

TressFXPPLL::TressFXPPLL() : 
    m_nScreenWidth(0),
    m_nScreenHeight(0), 
    m_nNodes(0), 
    m_nNodeSize(0), 
    m_PPLLHeads(nullptr), 
    m_PPLLNodes(nullptr), 
    m_PPLLCounter(nullptr),
    m_pPPLLFillBindSet(nullptr), 
    m_pPPLLResolveBindSet(nullptr),
    m_PPLLRenderTargetSet(nullptr),
    m_PPLLFillPSO(nullptr),
    m_ShadeParamsBindSet(nullptr)
{
}

void TressFXPPLL::Initialize(int width, int height, int nNodes, int nodeSize)
{
    Create(GetDevice(), width, height, nNodes, nodeSize);

    // Create constant buffer and bind set
    m_ShadeParamsConstantBuffer.CreateBufferResource("TressFXShadeParams");
    EI_BindSetDescription set = { { m_ShadeParamsConstantBuffer.GetBufferResource() } };
    m_ShadeParamsBindSet = GetDevice()->CreateBindSet(GetShortCutShadeParamLayout(), set);

    // Setup PSOs

    // Hair Fill Pass
    {
        //std::vector<VkVertexInputAttributeDescription> FillDesc;

        EI_PSOParams psoParams;
        psoParams.primitiveTopology = EI_Topology::TriangleList;
        psoParams.colorWriteEnable = false;
        psoParams.depthTestEnable = true;
        psoParams.depthWriteEnable = false;
        psoParams.depthCompareOp = EI_CompareFunc::LessEqual;

        psoParams.colorBlendParams.colorBlendEnabled = false;
        
        EI_BindLayout* HairColorLayouts[] = { GetTressFXParamLayout(), GetRenderPosTanLayout(), GetViewLayout(), GetPPLLFillLayout(), GetSamplerLayout() };
        psoParams.layouts = HairColorLayouts;
        psoParams.numLayouts = 5;
        psoParams.renderTargetSet = m_PPLLRenderTargetSet.get();

        m_PPLLFillPSO = GetDevice()->CreateGraphicsPSO("TressFXPPLL.hlsl", "RenderHairVS", "TressFXPPLL.hlsl", "PPLLFillPS", psoParams);
    }

    // Hair Resolve
    {
        //std::vector<VkVertexInputAttributeDescription> ResolveDesc;

        EI_PSOParams psoParams;
        psoParams.primitiveTopology = EI_Topology::TriangleStrip;
        psoParams.colorWriteEnable = true;
        psoParams.depthTestEnable = false;
        psoParams.depthWriteEnable = false;
        psoParams.depthCompareOp = EI_CompareFunc::LessEqual;

        // sushi code was:
        /*
            // Blending matches SDK Sample, which
            // works in terms of (1-a) and otherwise premultiplied.
            BlendEnable = true
            SrcBlend = ONE
            DstBlend = SRCALPHA
            BlendOp = ADD
            SrcBlendAlpha = ZERO
            DstBlendAlpha = ZERO
            BlendOpAlpha = ADD
        */
        psoParams.colorBlendParams.colorBlendEnabled = true;
        psoParams.colorBlendParams.colorBlendOp = EI_BlendOp::Add;
        psoParams.colorBlendParams.colorSrcBlend = EI_BlendFactor::One; //::One;
        psoParams.colorBlendParams.colorDstBlend = EI_BlendFactor::SrcAlpha; //::SrcAlpha;
        psoParams.colorBlendParams.alphaBlendOp = EI_BlendOp::Add;
        psoParams.colorBlendParams.alphaSrcBlend = EI_BlendFactor::Zero;
        psoParams.colorBlendParams.alphaDstBlend = EI_BlendFactor::Zero;

        EI_BindLayout* layouts[] = { GetPPLLResolveLayout(), GetPPLLShadeParamLayout(), GetViewLayout(), GetLightLayout(), GetSamplerLayout() };
        psoParams.layouts = layouts;
        psoParams.numLayouts = 5;
        psoParams.renderTargetSet = m_PPLLRenderTargetSet.get();
        m_PPLLResolvePSO = GetDevice()->CreateGraphicsPSO("TressFXPPLL.hlsl", "FullScreenVS", "TressFXPPLL.hlsl", "PPLLResolvePS", psoParams);
    }
}

bool TressFXPPLL::Create(EI_Device* pDevice, int width, int height, int nNodes, int nodeSize)
{
    m_nNodes        = nNodes;
    m_nNodeSize     = nodeSize;
    m_nScreenWidth  = width;
    m_nScreenHeight = height;

    // Create required resources
    m_PPLLHeads = pDevice->CreateUint32Resource(width, height, 1, "PPLLHeads", TRESSFX_PPLL_NULL_PTR);
    m_PPLLNodes = pDevice->CreateBufferResource(nodeSize, nNodes, EI_BF_NEEDSUAV, "PPLLNodes");
    m_PPLLCounter = pDevice->CreateBufferResource(sizeof(uint32_t), 1, EI_BF_NEEDSUAV, "PPLLNodes");

    // Create bind sets
    CreateFillBindSet(pDevice);
    CreateResolveBindSet(pDevice);

    // Create RenderPasss sets
    CreatePPLLRenderTargetSet(pDevice);
    return true;
}

// return the set?
void TressFXPPLL::CreateFillBindSet(EI_Device* pDevice)
{
    EI_BindSetDescription bindSet =
    {
        { m_PPLLHeads.get(), m_PPLLNodes.get(), m_PPLLCounter.get() }
    };

    m_pPPLLFillBindSet = pDevice->CreateBindSet(GetPPLLFillLayout(), bindSet);
}

void TressFXPPLL::CreateResolveBindSet(EI_Device* pDevice)
{
    EI_BindSetDescription bindSet =
    {
        { m_PPLLHeads.get(), m_PPLLNodes.get() }
    };

    m_pPPLLResolveBindSet = pDevice->CreateBindSet(GetPPLLResolveLayout(), bindSet);
}

void TressFXPPLL::CreatePPLLRenderTargetSet(EI_Device* pDevice)
{
    const EI_Resource* ResourceArray[] = { pDevice->GetColorBufferResource(), pDevice->GetDepthBufferResource() };
    const EI_AttachmentParams AttachmentParams[] = { {EI_RenderPassFlags::Load | EI_RenderPassFlags::Store},
                                                     {EI_RenderPassFlags::Depth | EI_RenderPassFlags::Load | EI_RenderPassFlags::Store} };
    m_PPLLRenderTargetSet = pDevice->CreateRenderTargetSet(ResourceArray, 2, AttachmentParams, nullptr);
}

void TressFXPPLL::Clear(EI_CommandContext& context)
{
    // Consider being explicit on values here.
    // In DX, UAV counter clears are actually done when UAV is set, which makes this a bit weird, I
    // suppose.
    // Also try clearing in shader during fullscreen pass (although can't miss frames, and makes
    // reads slower)
    // or look at 0-based for fast clears.

    // Transition and clear any resources we need to
    
    if (m_firstRun)
    {
        // The first time we use the resource, we need to transition it out of UNDEFINED state to COPY_DEST.
        // Doing from PS_SRV causes validation errors.
        EI_Barrier readToClear[] =
        {
#ifdef TRESSFX_VK
            { m_PPLLHeads.get(), EI_STATE_UNDEFINED, EI_STATE_COPY_DEST },
#endif // TRESSFX_VK
            { m_PPLLCounter.get(), EI_STATE_UAV, EI_STATE_COPY_DEST },
            { m_PPLLNodes.get(), EI_STATE_UAV, EI_STATE_SRV },   // Just need to do this on the first frame so our usual transition doesn't bug out.
        };

        context.SubmitBarrier(AMD_ARRAY_SIZE(readToClear), readToClear);
    }
    else
    {
        EI_Barrier readToClear[] =
        {
#ifdef TRESSFX_VK
            { m_PPLLHeads.get(), EI_STATE_SRV, EI_STATE_COPY_DEST },
#else
            { m_PPLLHeads.get(), EI_STATE_SRV, EI_STATE_UAV },
#endif // TRESSFX_VK
            { m_PPLLCounter.get(), EI_STATE_UAV, EI_STATE_COPY_DEST },
        };
        context.SubmitBarrier(AMD_ARRAY_SIZE(readToClear), readToClear);
    }

    context.ClearUint32Image(m_PPLLHeads.get(), TRESSFX_PPLL_NULL_PTR);

    uint32_t clearCounter = 0;
    context.UpdateBuffer(m_PPLLCounter.get(), &clearCounter);
}

void TressFXPPLL::BeginFill(EI_CommandContext& commandContext)
{
    // Begin the render pass and transition any resources to write if needed
    EI_Barrier readToWrite[] =
    {
#ifdef TRESSFX_VK
        { m_PPLLHeads.get(), EI_STATE_COPY_DEST, EI_STATE_UAV },
#endif // TRESSFX_VK
        { m_PPLLNodes.get(), EI_STATE_SRV, EI_STATE_UAV },
        { m_PPLLCounter.get(), EI_STATE_COPY_DEST, EI_STATE_UAV },
    };

    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(readToWrite), readToWrite);

    // Begin the render pass
    GetDevice()->BeginRenderPass(commandContext, m_PPLLRenderTargetSet.get(), L"BeginFill Pass");
}

void TressFXPPLL::EndFill(EI_CommandContext& commandContext)
{
    // End the render pass so we can transition the UAVs
    GetDevice()->EndRenderPass(commandContext);

    EI_Barrier writeToRead[] =
    {
        { m_PPLLHeads.get(), EI_STATE_UAV, EI_STATE_SRV },
        { m_PPLLNodes.get(), EI_STATE_UAV, EI_STATE_SRV },
    };

    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(writeToRead), writeToRead);
}

void TressFXPPLL::BeginResolve(EI_CommandContext& commandContext)
{
    // Begin the render pass
    GetDevice()->BeginRenderPass(commandContext, m_PPLLRenderTargetSet.get(), L"BeginResolve Pass");
}

void TressFXPPLL::EndResolve(EI_CommandContext& commandContext)
{
    // End the PPLL render pass
    GetDevice()->EndRenderPass(commandContext);
}

void TressFXPPLL::DrawHairStrands(EI_CommandContext& commandContext, int numHairStrands, HairStrands** hairStrands, EI_PSO* pPSO, EI_BindSet** extraBindSets, uint32_t numExtraBindSets)
{
    // Loop through all hair strands and render them
    for (size_t i = 0; i < numHairStrands; i++)
    {
        if (hairStrands[i]->GetTressFXHandle())
            hairStrands[i]->GetTressFXHandle()->DrawStrands(commandContext, *pPSO, extraBindSets, numExtraBindSets);
    }
}

void TressFXPPLL::Draw(EI_CommandContext& commandContext, int numHairStrands, HairStrands** hairStrands, EI_BindSet* viewBindSet, EI_BindSet* lightBindSet)
{
    // Clear out resources
    Clear(commandContext);

    // Render the fill pass
    BeginFill(commandContext);
    {
        EI_BindSet* ExtraBindSets[] = { viewBindSet, m_pPPLLFillBindSet.get(), GetDevice()->GetSamplerBindSet() };
        DrawHairStrands(commandContext, numHairStrands, hairStrands, m_PPLLFillPSO.get(), ExtraBindSets, 3);
    }
    EndFill(commandContext);
    GetDevice()->GetTimeStamp("PPLL Fill");

    // Hair Resolve pass
    BeginResolve(commandContext);
    {
        EI_BindSet* BindSets[] = { m_pPPLLResolveBindSet.get(), m_ShadeParamsBindSet.get(), viewBindSet, lightBindSet, GetDevice()->GetSamplerBindSet() };
        GetDevice()->DrawFullScreenQuad(commandContext, *m_PPLLResolvePSO, BindSets, 5);
    }
    EndResolve(commandContext);
    GetDevice()->GetTimeStamp("PPLL Resolve");

    m_firstRun = false;
}

void TressFXPPLL::UpdateShadeParameters(std::vector<const TressFXRenderingSettings*>& renderSettings)
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