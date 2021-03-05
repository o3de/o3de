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

#include "FurPasses.h"
#include "DriverD3D.h"
#include "D3DPostProcess.h"

FurPasses* FurPasses::s_pInstance = nullptr;

/*static*/ void FurPasses::InstallInstance()
{
    if (s_pInstance == nullptr)
    {
        s_pInstance = new FurPasses();
    }
}

/*static*/ void FurPasses::ReleaseInstance()
{
    delete s_pInstance;
    s_pInstance = nullptr;
}

/*static*/ FurPasses& FurPasses::GetInstance()
{
    AZ_Assert(s_pInstance != nullptr, "FurPasses instance being retrieved before install.");
    return *s_pInstance;
}

FurPasses::FurPasses()
    : m_furShellPassPercent(0.0f)
{
}

FurPasses::~FurPasses()
{
}

FurPasses::RenderMode FurPasses::GetFurRenderingMode()
{
    switch (CRenderer::CV_r_Fur)
    {
    case 1:
        return RenderMode::AlphaBlended;
    case 2:
        return RenderMode::AlphaTested;
    default:
        return RenderMode::None;
    }
}

bool FurPasses::IsRenderingFur()
{
    if (GetFurRenderingMode() == RenderMode::None)
    {
        return false;
    }

    uint32 flags = SRendItem::BatchFlags(GetFurRenderList(), gcpRendD3D->m_RP.m_pRLD);
    return (flags & FB_FUR) != 0;
}

int FurPasses::GetFurRenderList()
{
    return (GetFurRenderingMode() == RenderMode::AlphaBlended) ? EFSLIST_TRANSP : EFSLIST_GENERAL;
}

void FurPasses::ExecuteZPostPass()
{
    // This pass renders the outermost fur shell in a 1-in-4 stipple pattern to gather lighting data for fur tips.
    // It also performs an additional LinearizeDepth pass to provide the updated depths to the deferred pipeline.
    if (IsRenderingFur())
    {
        CD3D9Renderer* const __restrict rd = gcpRendD3D;
        {
            PROFILE_LABEL_SCOPE("FUR_ZPOST");

            rd->FX_ZScene(true, false);
            rd->m_RP.m_pRenderFunc = &ZPostRenderFunc;

            rd->FX_ProcessRenderList(GetFurRenderList(), FB_FUR, false /*bSetRenderFunc*/);
            rd->FX_ZScene(false, false, true);
        }

        rd->FX_LinearizeDepth(CTexture::s_ptexFurZTarget);
    }
}

void FurPasses::ExecuteObliteratePass()
{
    // This pass captures the lighting data from HDRTarget to s_ptexFurLightAcc, and then removes the stipples from 
    // the final target (via a horizontal blur only on the stippled pixels) and depth buffer (direct copy from Z target)
    // before beginning the forward shading passes
    if (IsRenderingFur())
    {
        CD3D9Renderer* const __restrict rd = gcpRendD3D;
        PROFILE_LABEL_SCOPE("FUR_OBLITERATE");

        // Copy HDR target so we can use it as an input texture
        PostProcessUtils().CopyScreenToTexture(CTexture::s_ptexFurLightAcc);

        PostProcessUtils().SetTexture(CTexture::s_ptexFurLightAcc, 0, FILTER_POINT);

        // Use Z target rather than fur Z target so that the "true" depth can be retained for forward passes
        // Without this, some passes may fail depth tests when they should pass (such as eye rendering)
        PostProcessUtils().SetTexture(CTexture::s_ptexZTarget, 1, FILTER_POINT);
        
        rd->m_RP.m_pRenderFunc = &ObliterateRenderFunc;
        rd->FX_ProcessRenderList(GetFurRenderList(), FB_FUR, false /*bSetRenderFunc*/);
    }
}

void FurPasses::ExecuteFinPass()
{
    // This pass renders alpha-tested camera-facing silhouettes of the fur fins. It uses similar logic to the fur shadow pass.
    if (IsRenderingFur())
    {
        CD3D9Renderer* const __restrict rd = gcpRendD3D;
        PROFILE_LABEL_SCOPE("FUR_FINS");

        uint64  nSavedFlags = rd->m_RP.m_FlagsShader_RT;
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_GPU_PARTICLE_TURBULENCE]; // Indicates fin pass
        FurPasses::GetInstance().ApplyFurDebugFlags();

        rd->m_RP.m_pRenderFunc = &FinRenderFunc;
        rd->FX_ProcessRenderList(GetFurRenderList(), FB_FUR, false /*bSetRenderFunc*/);

        rd->m_RP.m_FlagsShader_RT = nSavedFlags;
    }
}

void FurPasses::ExecuteShellPrepass()
{
    // This pass gathers and packs all data required by the shell passes into a single buffer. The RGB channels contain
    // the accumulated diffuse and specular lighting (without albedo applied), with the diffuse stored in the upper half of
    // the channels, and the specular stored in the lower half. The alpha channel contains the scene depth, to save a
    // texture read of the linearized depth buffer.
    if (IsRenderingFur())
    {
        // Skip shell prepass for aux viewports. Shader side, this is indicated by %_RT_HDR_MODE being unset, but since the
        // render pass hasn't started yet, we have to instead mimic the check that FX_Start performs to set %_RT_HDR_MODE
        CD3D9Renderer* const __restrict rd = gcpRendD3D;
        bool hdrMode = (rd->m_RP.m_PersFlags2 & RBPF2_HDR_FP16) && !(rd->m_RP.m_nBatchFilter & (FB_Z));
        if (hdrMode)
        {
            PROFILE_LABEL_SCOPE("FUR_SHELL_PREPASS");

            uint64 savedFlags = rd->m_RP.m_FlagsShader_RT;

            // Volumetric fog is applied in prepass only if fur is alpha blended; alpha tested fur is drawn before fog
            bool useVolumetricFog = CD3D9Renderer::CV_r_VolumetricFog != 0 && GetFurRenderingMode() == RenderMode::AlphaBlended;
            if (useVolumetricFog)
            {
                rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_VOLUMETRIC_FOG];
            }

            static CCryNameTSCRC techFurShellPrepass("FurShellPrepass");
            rd->FX_PushRenderTarget(0, CTexture::s_ptexFurPrepass, nullptr);
            PostProcessUtils().ShBeginPass(CShaderMan::s_ShaderFur, techFurShellPrepass, FEF_DONTSETSTATES | FEF_DONTSETTEXTURES);
            SPostEffectsUtils::SetTexture(CTexture::s_ptexFurLightAcc, 0, FILTER_POINT, 0);
            SPostEffectsUtils::SetTexture(CTexture::s_ptexSceneTargetR11G11B10F[0], 1, FILTER_POINT, 0);
            SPostEffectsUtils::SetTexture(CTexture::s_ptexSceneDiffuse, 2, FILTER_POINT, 0);
            SPostEffectsUtils::SetTexture(CTexture::s_ptexSceneNormalsMap, 3, FILTER_POINT, 0);
            SPostEffectsUtils::SetTexture(CTexture::s_ptexSceneSpecular, 4, FILTER_POINT, 0);
            SPostEffectsUtils::SetTexture(CTexture::s_ptexFurZTarget, 5, FILTER_POINT, 0);
            if (useVolumetricFog)
            {
                SPostEffectsUtils::SetTexture(CTexture::s_ptexVolumetricFog, 6, FILTER_TRILINEAR, 1);
            }

            rd->FX_SetState(GS_NODEPTHTEST);
            GetUtils().DrawQuadFS(CShaderMan::s_ShaderFur, true /*bOutputCamVec*/, CTexture::s_ptexFurPrepass->GetWidth(), CTexture::s_ptexFurPrepass->GetHeight());
            GetUtils().ShEndPass();
            rd->FX_PopRenderTarget(0);

            rd->m_RP.m_FlagsShader_RT = savedFlags;
        }
    }
}

void FurPasses::ApplyFurDebugFlags()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    if (rd->CV_r_FurDebug > 0 && (rd->m_RP.m_FlagsShader_RT & g_HWSR_MaskBit[HWSR_HDR_MODE]) != 0) // Don't apply fur debug flags in aux views
    {
        if (rd->CV_r_FurDebug & 1)
        {
            rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG0];
        }
        if (rd->CV_r_FurDebug & 2)
        {
            rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG1];
        }
        if (rd->CV_r_FurDebug & 4)
        {
            rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG2];
        }
        if (rd->CV_r_FurDebug & 8)
        {
            rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG3];
        }
    }
}

void FurPasses::SetFurShellPassPercent(float percent)
{
    m_furShellPassPercent = AZ::GetClamp(percent, 0.0f, 1.0f);
}

float FurPasses::GetFurShellPassPercent()
{
    return m_furShellPassPercent;
}

/*static*/ void FurPasses::ZPostRenderFunc()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    static CCryNameTSCRC techFurZPost("FurZPost");
    rd->m_RP.m_pShader->FXSetTechnique(techFurZPost);

    rd->FX_FlushShader_General();
}

/*static*/ void FurPasses::ObliterateRenderFunc()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    static CCryNameTSCRC techFurObliterate("FurObliterate");
    rd->m_RP.m_pShader->FXSetTechnique(techFurObliterate);

    rd->FX_FlushShader_General();
}

/*static*/ void FurPasses::FinRenderFunc()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    static CCryNameTSCRC techFurFins("FurFins");
    rd->m_RP.m_pShader->FXSetTechnique(techFurFins);

    rd->FX_FlushShader_General();
}

