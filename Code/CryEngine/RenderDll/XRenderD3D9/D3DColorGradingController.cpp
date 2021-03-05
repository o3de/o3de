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
#include "D3DColorGradingController.h"
#include "DriverD3D.h"
#include "D3DPostProcess.h"
#include <AzFramework/Asset/AssetSystemBus.h>
#include <StringUtils.h>


const int COLORCHART_SIZE = 16;
const int COLORCHART_ALIGNED_SIZE = 16;
const int COLORCHART_WIDTH = COLORCHART_SIZE * COLORCHART_SIZE;
const int COLORCHART_RENDERTARGET_WIDTH = COLORCHART_SIZE * COLORCHART_ALIGNED_SIZE;
const int COLORCHART_HEIGHT = COLORCHART_SIZE;
const uint32 COLORCHART_TEXFLAGS = FT_NOMIPS |  FT_DONT_STREAM | FT_STATE_CLAMP;
const char* COLORCHART_DEF_TEX = "EngineAssets/Textures/default_cch.dds";

const ETEX_Format COLORCHART_FORMAT = eTF_R8G8B8A8;

CColorGradingControllerD3D::CColorGradingControllerD3D(CD3D9Renderer* pRenderer)
    : m_layers()
    , m_pRenderer(pRenderer)
    , m_pSlicesVB(0)
    , m_pChartIdentity(0)
    , m_pChartStatic(0)
    , m_pChartToUse(0)
{
    assert(m_pRenderer);
    m_pMergeLayers[0] = 0;
    m_pMergeLayers[1] = 0;

    m_vecSlicesData.reserve(6 * COLORCHART_SIZE);
}


CColorGradingControllerD3D::~CColorGradingControllerD3D()
{
    ReleaseTextures();
}

void CColorGradingControllerD3D::ReleaseTextures()
{
    SAFE_RELEASE(m_pChartIdentity);
    SAFE_RELEASE(m_pChartStatic);
    SAFE_RELEASE(m_pMergeLayers[0]);
    SAFE_RELEASE(m_pMergeLayers[1]);
    SAFE_DELETE(m_pSlicesVB);
    m_pChartToUse = 0;
}

void CColorGradingControllerD3D::FreeMemory()
{
    stl::reconstruct(m_layers);
}

bool CColorGradingControllerD3D::ValidateColorChart(const CTexture* pChart) const
{
    if (!CTexture::IsTextureExist(pChart))
    {
        return false;
    }

    if (pChart->IsNoTexture())
    {
        return false;
    }

    if (pChart->GetWidth() != COLORCHART_WIDTH || pChart->GetHeight() != COLORCHART_HEIGHT)
    {
        return false;
    }

    return true;
}


CTexture* CColorGradingControllerD3D::LoadColorChartInt(const char* pChartFilePath) const
{
    if (!pChartFilePath || !pChartFilePath[0])
    {
        return 0;
    }

    // color charts don't currently support default fallbacks, so we'll force it to sync compile here if possible.
    if (!gEnv->pCryPak->IsFileExist(pChartFilePath))
    {
        EBUS_EVENT(AzFramework::AssetSystemRequestBus, CompileAssetSync, pChartFilePath);
    }

    CTexture* pChart = (CTexture*) m_pRenderer->EF_LoadTexture(pChartFilePath, COLORCHART_TEXFLAGS);
    if (!ValidateColorChart(pChart))
    {
        SAFE_RELEASE(pChart);
        return 0;
    }
    return pChart;
}


int CColorGradingControllerD3D::LoadColorChart(const char* pChartFilePath) const
{
    CTexture* pChart = LoadColorChartInt(pChartFilePath);
    return pChart ? pChart->GetID() : -1;
}


int CColorGradingControllerD3D::LoadDefaultColorChart() const
{
    CTexture* pChartIdentity = LoadColorChartInt(COLORCHART_DEF_TEX);
    return pChartIdentity ? pChartIdentity->GetID() : -1;
}


void CColorGradingControllerD3D::UnloadColorChart(int texID) const
{
    CTexture* pChart = CTexture::GetByID(texID);
    SAFE_RELEASE(pChart);
}


void CColorGradingControllerD3D::SetLayers(const SColorChartLayer* pLayers, uint32 numLayers)
{
    gRenDev->m_pRT->RC_CGCSetLayers(this, pLayers, numLayers);
}


void CColorGradingControllerD3D::RT_SetLayers(const SColorChartLayer* pLayerInfo, uint32 numLayers)
{
    m_layers.reserve(numLayers);
    m_layers.resize(0);

    if (numLayers)
    {
        float blendSum = 0;
        for (size_t i = 0; i < numLayers; ++i)
        {
            const SColorChartLayer& l = pLayerInfo[i];
            if (l.m_texID > 0 && l.m_blendAmount > 0)
            {
                const CTexture* pChart = CTexture::GetByID(l.m_texID);
                if (ValidateColorChart(pChart))
                {
                    m_layers.push_back(l);
                    blendSum += l.m_blendAmount;
                }
            }
        }

        const size_t numActualLayers = m_layers.size();
        if (numActualLayers)
        {
            if (numActualLayers > 1)
            {
                float normalizeBlendAmount = (float) (1.0 / (double) blendSum);
                for (size_t i = 0; i < numActualLayers; ++i)
                {
                    m_layers[i].m_blendAmount *= normalizeBlendAmount;
                }
            }
            else
            {
                m_layers[0].m_blendAmount = 1;
            }
        }
    }
}


bool CColorGradingControllerD3D::InitResources()
{
    if (!m_pChartIdentity)
    {
        m_pChartIdentity = LoadColorChartInt(COLORCHART_DEF_TEX);
        if (!m_pChartIdentity)
        {
            static bool bPrint = true;
            if (bPrint)
            {
                iLog->LogError("Failed to initialize Color Grading: Default color chart is missing");
            }
            bPrint = false;
            return false;
        }
    }

    if (!m_pMergeLayers[0])
    {
        m_pMergeLayers[0] = CTexture::CreateRenderTarget("ColorGradingMergeLayer0", COLORCHART_RENDERTARGET_WIDTH, COLORCHART_HEIGHT, Clr_Empty, eTT_2D, COLORCHART_TEXFLAGS, COLORCHART_FORMAT);
        if (!CTexture::IsTextureExist(m_pMergeLayers[0]))
        {
            return false;
        }
    }

    if (!m_pMergeLayers[1])
    {
        m_pMergeLayers[1] = CTexture::CreateRenderTarget("ColorGradingMergeLayer1", COLORCHART_RENDERTARGET_WIDTH, COLORCHART_HEIGHT, Clr_Empty, eTT_2D, COLORCHART_TEXFLAGS, COLORCHART_FORMAT);
        if (!CTexture::IsTextureExist(m_pMergeLayers[1]))
        {
            return false;
        }
    }

    if (!m_pSlicesVB)
    {
        //assert(m_vecSlicesData.empty());
        m_vecSlicesData.resize(6 * COLORCHART_SIZE);

        const float fQuadSize = (float)COLORCHART_SIZE / COLORCHART_RENDERTARGET_WIDTH;
        const float fTexCoordSize = 1.f / COLORCHART_SIZE;
        for (uint32 iQuad = 0; iQuad < COLORCHART_SIZE; ++iQuad)
        {
            const float fQuadShift = (float)iQuad / COLORCHART_SIZE;
            const float fBlue = (float)iQuad / (COLORCHART_SIZE - 1.f);

            m_vecSlicesData[iQuad * 6 + 0].xyz =    Vec3(fQuadShift + fQuadSize,           1.f, 0.f);
            m_vecSlicesData[iQuad * 6 + 0].st =     Vec2(fQuadShift + fTexCoordSize,   1.f);
            m_vecSlicesData[iQuad * 6 + 0].color.dcolor = ColorF(1.f, 1.f, fBlue).pack_argb8888();
            m_vecSlicesData[iQuad * 6 + 1].xyz =    Vec3(fQuadShift + fQuadSize,           0.f, 0.f);
            m_vecSlicesData[iQuad * 6 + 1].st =     Vec2(fQuadShift + fTexCoordSize,   0.f);
            m_vecSlicesData[iQuad * 6 + 1].color.dcolor = ColorF(1.f, 0.f, fBlue).pack_argb8888();
            m_vecSlicesData[iQuad * 6 + 2].xyz =    Vec3(fQuadShift + 0.f,                     1.f, 0.f);
            m_vecSlicesData[iQuad * 6 + 2].st =     Vec2(fQuadShift + 0.f,                     1.f);
            m_vecSlicesData[iQuad * 6 + 2].color.dcolor = ColorF(0.f, 1.f, fBlue).pack_argb8888();

            m_vecSlicesData[iQuad * 6 + 3].xyz =    Vec3(fQuadShift + 0.f,                     1.f, 0.f);
            m_vecSlicesData[iQuad * 6 + 3].st =     Vec2(fQuadShift + 0.f,                     1.f);
            m_vecSlicesData[iQuad * 6 + 3].color.dcolor = ColorF(0.f, 1.f, fBlue).pack_argb8888();
            m_vecSlicesData[iQuad * 6 + 4].xyz =    Vec3(fQuadShift + fQuadSize,           0.f, 0.f);
            m_vecSlicesData[iQuad * 6 + 4].st =     Vec2(fQuadShift + fTexCoordSize,   0.f);
            m_vecSlicesData[iQuad * 6 + 4].color.dcolor = ColorF(1.f, 0.f, fBlue).pack_argb8888();
            m_vecSlicesData[iQuad * 6 + 5].xyz =    Vec3(fQuadShift + 0.f,                     0.f, 0.f);
            m_vecSlicesData[iQuad * 6 + 5].st =     Vec2(fQuadShift + 0.f,                     0.f);
            m_vecSlicesData[iQuad * 6 + 5].color.dcolor = ColorF(0.f, 0.f, fBlue).pack_argb8888();
        }

        m_pSlicesVB = new CVertexBuffer(&m_vecSlicesData[0], eVF_P3F_C4B_T2F, 6 * COLORCHART_SIZE);
    }

    return true;
}


bool CColorGradingControllerD3D::Update(const SColorGradingMergeParams* pMergeParams)
{
    m_pChartToUse = 0;

    if (!m_pRenderer->CV_r_colorgrading_charts)
    {
        return true;
    }

    if (m_pChartStatic)
    {
        m_pChartToUse = m_pChartStatic;
        return true;
    }

    if (!InitResources())
    {
        m_pChartToUse = m_pChartIdentity;
        return m_pChartToUse != 0;
    }

    gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);

    static const int texStatePntID = CTexture::GetTexState(STexState(FILTER_POINT, true));
    static const int texStateLinID = CTexture::GetTexState(STexState(FILTER_LINEAR, true));

    const uint64 sample0    = g_HWSR_MaskBit[HWSR_SAMPLE0];
    const uint64 sample1    = g_HWSR_MaskBit[HWSR_SAMPLE1];
    const uint64 sample2    = g_HWSR_MaskBit[HWSR_SAMPLE2];
    const uint64 sample5    = g_HWSR_MaskBit[HWSR_SAMPLE5];

    // merge layers
    const size_t numLayers = m_layers.size();
    if (!numLayers)
    {
        m_pChartToUse = m_pChartIdentity;
    }
    else
    {
        CTexture* pNewMergeResult = m_pMergeLayers[0];
        m_pRenderer->FX_PushRenderTarget(0, pNewMergeResult, 0);

        int numMergePasses = 0;
        for (size_t curLayer = 0; curLayer < numLayers; )
        {
            size_t mergeLayerIdx[4] = {azlossy_caster(-1), azlossy_caster(-1), azlossy_caster(-1), azlossy_caster(-1)};
            int numLayersPerPass = 0;

            for (; curLayer < numLayers && numLayersPerPass < 4; ++curLayer)
            {
                if (m_layers[curLayer].m_blendAmount > 0.001f)
                {
                    mergeLayerIdx[numLayersPerPass++] = curLayer;
                }
            }

            if (numLayersPerPass)
            {
                const uint64 nResetFlags = ~(sample0 | sample1 | sample2);
                m_pRenderer->m_RP.m_FlagsShader_RT &= nResetFlags;
                if ((numLayersPerPass - 1) & 1)
                {
                    gRenDev->m_RP.m_FlagsShader_RT |= sample0;
                }
                if ((numLayersPerPass - 1) & 2)
                {
                    gRenDev->m_RP.m_FlagsShader_RT |= sample1;
                }

                CShader* pSh = CShaderMan::s_shPostEffectsGame;

                PROFILE_LABEL_SCOPE("MergeColorCharts");
                static CCryNameTSCRC techName("MergeColorCharts");
                SD3DPostEffectsUtils::ShBeginPass(pSh, techName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

                Vec4 layerBlendAmount(0, 0, 0, 0);
                for (int i = 0; i < numLayersPerPass; ++i)
                {
                    const SColorChartLayer& l = m_layers[mergeLayerIdx[i]];
                    CTexture* pChart = CTexture::GetByID(l.m_texID);
                    pChart->Apply(i, texStatePntID);
                    layerBlendAmount[i] = l.m_blendAmount;
                }

                static CCryNameR semLayerBlendAmount("LayerBlendAmount");
                SD3DPostEffectsUtils::ShSetParamPS(semLayerBlendAmount, layerBlendAmount);

                m_pRenderer->FX_SetState(GS_NODEPTHTEST | (numMergePasses ? GS_BLSRC_ONE | GS_BLDST_ONE : 0));
                m_pRenderer->SetCullMode(R_CULL_NONE);
                gcpRendD3D->DrawPrimitivesInternal(m_pSlicesVB, COLORCHART_SIZE * 6, eptTriangleList);

                SD3DPostEffectsUtils::ShEndPass();
                ++numMergePasses;

                m_pRenderer->m_RP.m_FlagsShader_RT &= nResetFlags;
            }
        }

        m_pChartToUse = numMergePasses ? pNewMergeResult : m_pChartIdentity;

        m_pRenderer->FX_PopRenderTarget(0);
    }

    // combine merged layers with color grading stuff
    if (m_pChartToUse && pMergeParams)
    {
        PROFILE_LABEL_SCOPE("CombineColorGradingWithColorChart");

        uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
        gRenDev->m_RP.m_FlagsShader_RT = pMergeParams->nFlagsShaderRT;
        gRenDev->m_RP.m_FlagsShader_RT &= ~(sample1 | sample5);

        CTexture* pNewMergeResult = m_pMergeLayers[1];
        m_pRenderer->FX_PushRenderTarget(0, pNewMergeResult, 0);
        m_pRenderer->FX_SetColorDontCareActions(0, true, false);
        CShader* pSh = CShaderMan::s_shPostEffectsGame;

        static CCryNameTSCRC techName("CombineColorGradingWithColorChart");
        SD3DPostEffectsUtils::ShBeginPass(pSh, techName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

        m_pChartToUse->Apply(0, texStateLinID);

        static CCryNameR pParamName0("ColorGradingParams0");
        static CCryNameR pParamName1("ColorGradingParams1");
        static CCryNameR pParamName2("ColorGradingParams2");
        static CCryNameR pParamName3("ColorGradingParams3");
        static CCryNameR pParamName4("ColorGradingParams4");
        static CCryNameR pParamMatrix("mColorGradingMatrix");

        pSh->FXSetPSFloat(pParamName0, &pMergeParams->pLevels[0], 1);
        pSh->FXSetPSFloat(pParamName1, &pMergeParams->pLevels[1], 1);
        pSh->FXSetPSFloat(pParamName2, &pMergeParams->pFilterColor, 1);
        pSh->FXSetPSFloat(pParamName3, &pMergeParams->pSelectiveColor[0], 1);
        pSh->FXSetPSFloat(pParamName4, &pMergeParams->pSelectiveColor[1], 1);
        pSh->FXSetPSFloat(pParamMatrix, &pMergeParams->pColorMatrix[0], 3);

        m_pRenderer->FX_SetState(GS_NODEPTHTEST);
        m_pRenderer->SetCullMode(R_CULL_NONE);
        gcpRendD3D->DrawPrimitivesInternal(m_pSlicesVB, COLORCHART_SIZE * 6, eptTriangleList);

        SD3DPostEffectsUtils::ShEndPass();

        m_pChartToUse = pNewMergeResult;

        m_pRenderer->FX_PopRenderTarget(0);

        gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
    }

    return m_pChartToUse != 0;
}


CTexture* CColorGradingControllerD3D::GetColorChart() const
{
    return m_pChartToUse;
}


void CColorGradingControllerD3D::DrawLayer([[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] float w, [[maybe_unused]] float h, [[maybe_unused]] CTexture* pChart, [[maybe_unused]] float blendAmount, [[maybe_unused]] const char* pLayerName) const
{
#if !defined(_RELEASE)
    CShader* pSh = CShaderMan::s_shPostEffectsGame;

    gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE0];

    // If using merged color grading with color chart disable regular color transformations in display - only need to use color chart
    if (pChart && pChart->GetTexType() == eTT_3D)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    }

    static CCryNameTSCRC techName("DisplayColorCharts");
    SD3DPostEffectsUtils::ShBeginPass(pSh, techName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    static const int texStateID = CTexture::GetTexState(STexState(FILTER_POINT, true));
    if (pChart)
    {
        pChart->Apply(0, texStateID);
    }

    TempDynVB<SVF_P3F_C4B_T2F> vb(gcpRendD3D);
    vb.Allocate(4);
    SVF_P3F_C4B_T2F* pVerts = vb.Lock();

    pVerts[0].xyz = Vec3(x, y, 0);
    pVerts[0].st = Vec2(0, 1);

    pVerts[1].xyz = Vec3(x + w, y, 0);
    pVerts[1].st = Vec2(1, 1);

    pVerts[2].xyz = Vec3(x, y + h, 0);
    pVerts[2].st = Vec2(0, 0);

    pVerts[3].xyz = Vec3(x + w, y + h, 0);
    pVerts[3].st = Vec2(1, 0);

    vb.Unlock();

    m_pRenderer->FX_Commit();
    m_pRenderer->FX_SetState(GS_NODEPTHTEST);
    m_pRenderer->SetCullMode(R_CULL_NONE);

    vb.Bind(0);
    vb.Release();

    if (!FAILED(m_pRenderer->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
    {
        m_pRenderer->FX_DrawPrimitive(eptTriangleStrip, 0, 4);
    }

    SD3DPostEffectsUtils::ShEndPass();

    float color[4] = {1, 1, 1, 1};
    m_pRenderer->Draw2dLabel(x + w + 10.0f, y, 1.35f, color, false, "%2.1f%%", blendAmount * 100.0f);
    m_pRenderer->Draw2dLabel(x + w + 55.0f, y, 1.35f, color, false, "%s", pLayerName);
#endif // #if !defined(_RELEASE)
}


void CColorGradingControllerD3D::DrawDebugInfo() const
{
#if !defined(_RELEASE)
    if (m_pRenderer->CV_r_colorgrading_charts < 2)
    {
        return;
    }

    TransformationMatrices backupSceneMatrices;

    m_pRenderer->Set2DMode(m_pRenderer->GetWidth(), m_pRenderer->GetHeight(), backupSceneMatrices);

    const float w = (float) COLORCHART_WIDTH;
    const float h = (float) COLORCHART_HEIGHT;

    float x = 16.0f;
    float y = 16.0f;

    if (!m_pChartStatic)
    {
        for (size_t i = 0, numLayers = m_layers.size(); i < numLayers; ++i)
        {
            const SColorChartLayer& l = m_layers[i];
            CTexture* pChart = CTexture::GetByID(l.m_texID);
            DrawLayer(x, y, w, h, pChart, l.m_blendAmount, CryStringUtils::FindFileNameInPath(pChart->GetName()));
            y += h + 4;
        }
        if (GetColorChart())
        {
            DrawLayer(x, y, w, h, GetColorChart(), 1, "FinalChart");
        }
    }
    else
    {
        DrawLayer(x, y, w, h, m_pChartStatic, 1, CryStringUtils::FindFileNameInPath(m_pChartStatic->GetName()));
    }

    m_pRenderer->RT_RenderTextMessages();

    m_pRenderer->Unset2DMode(backupSceneMatrices);
#endif // #if !defined(_RELEASE)
}


bool CColorGradingControllerD3D::LoadStaticColorChart(const char* pChartFilePath)
{
    bool success = true;

    // Prevent a dangling pointer by updating the current chart if it was set to the old static chart.
    bool updateCurrentChart = false;
    if (m_pChartToUse == m_pChartStatic)
    {
        updateCurrentChart = true;
    }

    SAFE_RELEASE(m_pChartStatic);
    if (pChartFilePath && pChartFilePath[0] != '\0')
    {
        m_pChartStatic = LoadColorChartInt(pChartFilePath);
        success = m_pChartStatic != 0;
    }

    if (updateCurrentChart)
    {
        m_pChartToUse = m_pChartStatic;
    }

    return success;
}


const CTexture* CColorGradingControllerD3D::GetStaticColorChart() const
{
    return m_pChartStatic;
}
