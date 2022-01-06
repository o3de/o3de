// ----------------------------------------------------------------------------
// Layouts describe resources for each type of shader in TressFX.
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

#include "TressFXLayouts.h"
#include "EngineInterface.h"

using namespace AMD;

// By default, app allocates space for each of these, and TressFX uses it.
// These are globals, because there should really just be one instance.
TressFXLayouts*                        g_TressFXLayouts = nullptr;

void CreateSimPosTanLayout(EI_Device* pDevice)
{
    EI_LayoutDescription desc = {
        {
            {"g_HairVertexPositions", 0, EI_RESOURCETYPE_BUFFER_RW },
            {"g_HairVertexPositionsPrev", 1, EI_RESOURCETYPE_BUFFER_RW },
            {"g_HairVertexPositionsPrevPrev", 2, EI_RESOURCETYPE_BUFFER_RW },
            {"g_HairVertexTangents", 3, EI_RESOURCETYPE_BUFFER_RW },
            {"g_StrandLevelData", 4, EI_RESOURCETYPE_BUFFER_RW },
        },
        EI_CS
    };

    g_TressFXLayouts->pSimPosTanLayout = pDevice->CreateLayout(desc);
}

void CreateRenderPosTanLayout(EI_Device* pDevice)
{
    EI_LayoutDescription desc = {
        {
            {"g_GuideHairVertexPositions", 0, EI_RESOURCETYPE_BUFFER_RO },
            {"g_GuideHairVertexTangents", 1, EI_RESOURCETYPE_BUFFER_RO },
        },
        EI_ALL
    };

    g_TressFXLayouts->pRenderPosTanLayout = pDevice->CreateLayout(desc);
}

void CreateRenderLayout(EI_Device* pDevice)
{
    EI_LayoutDescription desc = {
        {
            {"g_HairThicknessCoeffs", 0, EI_RESOURCETYPE_BUFFER_RO },
            {"g_HairStrandTexCd", 1, EI_RESOURCETYPE_BUFFER_RO },
            {"BaseAlbedoTexture", 2, EI_RESOURCETYPE_IMAGE_RO },
            {"TressFXParameters", 3, EI_RESOURCETYPE_UNIFORM },
            {"TressFXStrandParameters", 4, EI_RESOURCETYPE_UNIFORM },
            {"StrandAlbedoTexture", 5, EI_RESOURCETYPE_IMAGE_RO },
        },
        EI_ALL
    };

    g_TressFXLayouts->pTressFXParamLayout = pDevice->CreateLayout(desc);
}

void CreateSamplerLayout(EI_Device* pDevice)
{
    EI_LayoutDescription desc = {
        {
            {"LinearWrapSampler", 0, EI_RESOURCETYPE_SAMPLER },
        },
        EI_ALL
    };

    g_TressFXLayouts->pSamplerLayout = pDevice->CreateLayout(desc);
}

void CreateGenerateSDFLayout(EI_Device* pDevice)
{
    EI_LayoutDescription desc = {
        {
            {"g_TrimeshVertexIndices", 0, EI_RESOURCETYPE_BUFFER_RO },
            {"g_SignedDistanceField", 1, EI_RESOURCETYPE_BUFFER_RW },
            {"collMeshVertexPositions", 2, EI_RESOURCETYPE_BUFFER_RW },
            {"ConstBuffer_SDF", 3, EI_RESOURCETYPE_UNIFORM },
        },
        EI_CS
    };

    g_TressFXLayouts->pGenerateSDFLayout = pDevice->CreateLayout(desc);
}

void CreateSimLayout(EI_Device* pDevice)
{
    EI_LayoutDescription desc = {
        {
            {"g_InitialHairPositions", 4, EI_RESOURCETYPE_BUFFER_RO },
            {"g_HairRestLengthSRV", 5, EI_RESOURCETYPE_BUFFER_RO },
            {"g_HairStrandType", 6, EI_RESOURCETYPE_BUFFER_RO },
            {"g_FollowHairRootOffset", 7, EI_RESOURCETYPE_BUFFER_RO },
            {"g_BoneSkinningData", 12, EI_RESOURCETYPE_BUFFER_RO },
            {"tressfxSimParameters", 13, EI_RESOURCETYPE_UNIFORM },
        },
        EI_CS
    };

    g_TressFXLayouts->pSimLayout = pDevice->CreateLayout(desc);
}

void CreateApplySDFLayout(EI_Device* pDevice)
{
    EI_LayoutDescription desc = {
        {
            {"g_HairVertices", 0, EI_RESOURCETYPE_BUFFER_RW },
            {"g_PrevHairVertices", 1, EI_RESOURCETYPE_BUFFER_RW },
        },
        EI_CS
    };

    g_TressFXLayouts->pApplySDFLayout = pDevice->CreateLayout(desc);
}

void CreateBoneSkinningLayout(EI_Device* pDevice)
{
    EI_LayoutDescription desc = {
        {
            {"bs_boneSkinningData", 1, EI_RESOURCETYPE_BUFFER_RO },
            {"bs_initialVertexPositions", 2, EI_RESOURCETYPE_BUFFER_RO },
            {"bs_collMeshVertexPositions", 0, EI_RESOURCETYPE_BUFFER_RW },
            {"ConstBufferCS_BoneMatrix", 3, EI_RESOURCETYPE_UNIFORM },
        },
        EI_ALL
    };

    g_TressFXLayouts->pBoneSkinningLayout = pDevice->CreateLayout(desc);

}

void CreateSDFMarchingCubesLayout(EI_Device* pDevice)
{
    EI_LayoutDescription desc = {
        {
            {"g_MarchingCubesEdgeTable", 3, EI_RESOURCETYPE_BUFFER_RO },
            {"g_MarchingCubesTriangleTable", 4, EI_RESOURCETYPE_BUFFER_RO },
            {"g_MarchingCubesSignedDistanceField", 0, EI_RESOURCETYPE_BUFFER_RW },
            {"g_MarchingCubesTriangleVertices", 1, EI_RESOURCETYPE_BUFFER_RW },
            {"g_NumMarchingCubesVertices", 2, EI_RESOURCETYPE_BUFFER_RW },
            {"ConstBuffer_MC", 5, EI_RESOURCETYPE_UNIFORM },
        },
        EI_ALL
    };

    g_TressFXLayouts->pSDFMarchingCubesLayout = pDevice->CreateLayout(desc);
}

void CreatePPLLFillLayout(EI_Device* pDevice)
{
    EI_LayoutDescription desc = {
        {
            {"RWFragmentListHead", 0, EI_RESOURCETYPE_IMAGE_RW },
            {"LinkedListUAV", 1, EI_RESOURCETYPE_BUFFER_RW },
            {"LinkedListCounter", 2, EI_RESOURCETYPE_BUFFER_RW },
        },
        EI_PS
    };

    g_TressFXLayouts->pPPLLFillLayout = pDevice->CreateLayout(desc);
 }

void CreatePPLLResolveLayout(EI_Device* pDevice)
{
    EI_LayoutDescription desc = {
        {
            {"FragmentListHead", 0, EI_RESOURCETYPE_IMAGE_RO },
            {"LinkedListNodes", 1, EI_RESOURCETYPE_BUFFER_RO },
        },
        EI_PS
    };

    g_TressFXLayouts->pPPLLResolveLayout = pDevice->CreateLayout(desc);
}

void CreatePPLLShadeParamLayout(EI_Device* pDevice)
{
    EI_LayoutDescription desc = {
        {
            {"TressFXShadeParams", 0, EI_RESOURCETYPE_UNIFORM },
        },
        EI_PS
    };

    g_TressFXLayouts->pPPLLShadeParamLayout = pDevice->CreateLayout(desc);
}

void CreateShortcutDepthsAlphaLayout(EI_Device* pDevice)
{
    EI_LayoutDescription desc = {
        {
            {"RWFragmentDepthsTexture", 0, EI_RESOURCETYPE_IMAGE_RW },
        },
        EI_PS
    };
    g_TressFXLayouts->pShortcutDepthsAlphaLayout = pDevice->CreateLayout(desc);
}

void CreateShortcutDepthReadLayout(EI_Device* pDevice)
{
    EI_LayoutDescription desc = {
        {
            {"FragmentDepthsTexture", 0, EI_RESOURCETYPE_IMAGE_RO },
        },
        EI_PS
    };

    g_TressFXLayouts->pShortcutDepthReadLayout = pDevice->CreateLayout(desc);
}

void CreateShortCutShadeParamLayout(EI_Device* pDevice)
{
    EI_LayoutDescription desc = {
        {
            {"TressFXShadeParams", 0, EI_RESOURCETYPE_UNIFORM },
        },
        EI_PS
    };

    g_TressFXLayouts->pShortcutShadeParamLayout = pDevice->CreateLayout(desc);
}

void CreateShortCutColorReadLayout(EI_Device* pDevice)
{
    EI_LayoutDescription desc = {
        {
            {"HaiColorTexture", 0, EI_RESOURCETYPE_IMAGE_RO },
            {"AccumInvAlpha", 1, EI_RESOURCETYPE_IMAGE_RO },
        },
        EI_PS
    };
    g_TressFXLayouts->pShortCutColorReadLayout = pDevice->CreateLayout(desc);
}

void CreateViewLayout(EI_Device * pDevice)
{
    EI_LayoutDescription desc = {
        {
            {"viewConstants", 0, EI_RESOURCETYPE_UNIFORM },
        },
        EI_ALL
    };

    g_TressFXLayouts->pViewLayout = pDevice->CreateLayout(desc);
}

void CreateShadowViewLayout(EI_Device* pDevice)
{
    EI_LayoutDescription desc = {
        {
            {"shadowViewConstants", 0, EI_RESOURCETYPE_UNIFORM },
        },
        EI_ALL
    };

    g_TressFXLayouts->pShadowViewLayout = pDevice->CreateLayout(desc);
}

void CreateLightLayout(EI_Device* pDevice)
{
    EI_LayoutDescription desc = {
        {
            {"LightConstants", 0, EI_RESOURCETYPE_UNIFORM },
            {"ShadowTexture", 1, EI_RESOURCETYPE_IMAGE_RO },
        },
        EI_PS
    };

    g_TressFXLayouts->pLightLayout = pDevice->CreateLayout(desc);
}

void InitializeAllLayouts(EI_Device* pDevice)
{
    // See TressFXLayouts.h
    // Global storage for layouts.

    if (g_TressFXLayouts == 0)
        g_TressFXLayouts = new TressFXLayouts;

    CreateSimPosTanLayout(pDevice);
    CreateRenderPosTanLayout(pDevice);
    CreateRenderLayout(pDevice);
    CreateSamplerLayout(pDevice);
    CreateGenerateSDFLayout(pDevice);
    CreateSimLayout(pDevice);
    CreateApplySDFLayout(pDevice);
    CreateBoneSkinningLayout(pDevice);
    CreateSDFMarchingCubesLayout(pDevice);
    
    CreateShortcutDepthsAlphaLayout(pDevice);
    CreateShortcutDepthReadLayout(pDevice);
    CreateShortCutShadeParamLayout(pDevice);
    CreateShortCutColorReadLayout(pDevice);

    CreatePPLLFillLayout(pDevice);
    CreatePPLLResolveLayout(pDevice);
    CreatePPLLShadeParamLayout(pDevice);

    CreateViewLayout(GetDevice());
    CreateShadowViewLayout(GetDevice());
    CreateLightLayout(GetDevice());
}

void DestroyAllLayouts(EI_Device* pDevice)
{
    if (g_TressFXLayouts != nullptr)
    {
        delete g_TressFXLayouts;
    }
}