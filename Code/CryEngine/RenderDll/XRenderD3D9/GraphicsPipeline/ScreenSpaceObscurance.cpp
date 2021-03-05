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
#include "ScreenSpaceObscurance.h"
#include "DriverD3D.h"
#include "D3DPostProcess.h"
#include "D3D_SVO.h"
#include "../../Common/Textures/TextureManager.h"
#include "FurPasses.h"


void CScreenSpaceObscurancePass::Init()
{
}

void CScreenSpaceObscurancePass::Shutdown()
{
    Reset();
}

void CScreenSpaceObscurancePass::Reset()
{
    m_passObscurance.Reset();
    m_passFilter.Reset();
    m_passAlbedoDownsample0.Reset();
    m_passAlbedoDownsample1.Reset();
    m_passAlbedoDownsample2.Reset();
    m_passAlbedoBlur.Reset();
}

void CScreenSpaceObscurancePass::Execute()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    if (!CRenderer::CV_r_ssdo)
    {
        rd->FX_ClearTarget(CTexture::s_ptexSceneNormalsBent, Clr_Median);
        return;
    }

    // Calculate height map AO first
    ShadowMapFrustum* pHeightMapFrustum = NULL;
    CTexture* pHeightMapAODepth = NULL;
    CTexture* pHeightMapAO = NULL;
    CDeferredShading::Instance().HeightMapOcclusionPass(pHeightMapFrustum, pHeightMapAODepth, pHeightMapAO);

    PROFILE_LABEL_SCOPE("DIRECTIONAL_OCC");

    int texStateLinear = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
    int texStatePoint = CTexture::GetTexState(STexState(FILTER_POINT, true));
    int texStatePointWrap = CTexture::GetTexState(STexState(FILTER_POINT, false));

    CTexture* pDestRT = CTexture::s_ptexStereoR;
#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(ScreenSpaceObscurance_cpp)
#endif

    const bool bLowResOutput = (CRenderer::CV_r_ssdoHalfRes == 3);
    if (bLowResOutput)
    {
        pDestRT = CTexture::s_ptexBackBufferScaled[0];
    }

    // Obscurance generation
    {
        CShader* pShader = CShaderMan::s_shDeferredShading;

        bool isRenderingFur = FurPasses::GetInstance().IsRenderingFur();

        uint64 rtMask = 0;
        rtMask |= CRenderer::CV_r_ssdoHalfRes ? g_HWSR_MaskBit[HWSR_SAMPLE0] : 0;
        rtMask |= pHeightMapFrustum ? g_HWSR_MaskBit[HWSR_SAMPLE1] : 0;
        rtMask |= isRenderingFur ? g_HWSR_MaskBit[HWSR_SAMPLE2] : 0;

        // Extreme magnification as happening with small FOVs will cause banding issues with half-res depth
        if (CRenderer::CV_r_ssdoHalfRes == 2 && RAD2DEG(rd->GetCamera().GetFov()) < 30)
        {
            rtMask &= ~g_HWSR_MaskBit[HWSR_SAMPLE0];
        }

        static CCryNameTSCRC tech("DirOccPass");

        m_passObscurance.SetRenderTarget(0, pDestRT);
        m_passObscurance.SetTechnique(pShader, tech, rtMask);
        m_passObscurance.SetState(GS_NODEPTHTEST);

        m_passObscurance.SetTextureSamplerPair(0, CTexture::s_ptexSceneNormalsMap, texStatePoint);
        m_passObscurance.SetTextureSamplerPair(1, CTexture::s_ptexZTarget, texStatePoint);
        m_passObscurance.SetTextureSamplerPair(3, CTextureManager::Instance()->GetDefaultTexture("AOVOJitter"), texStatePointWrap);
        m_passObscurance.SetTextureSamplerPair(5, bLowResOutput ? CTexture::s_ptexZTargetScaled2 : CTexture::s_ptexZTargetScaled, texStatePoint);
        m_passObscurance.SetTextureSamplerPair(11, pHeightMapAODepth, texStatePoint);

        if (isRenderingFur)
        {
            m_passObscurance.SetTextureSamplerPair(2, CTexture::s_ptexFurZTarget, texStatePoint);
        }

        m_passObscurance.SetTexture(12, pHeightMapAO);
        m_passObscurance.SetRequireWorldPos(true);  // TODO: Can be removed after shader changes

        m_passObscurance.BeginConstantUpdate();

        float radius = CRenderer::CV_r_ssdoRadius / rd->GetViewParameters().fFar;
    #if defined(FEATURE_SVO_GI)
        if (CSvoRenderer::GetInstance()->IsActive())
        {
            radius *= CSvoRenderer::GetInstance()->GetSsaoAmount();
        }
    #endif
        static CCryNameR ssdoParamsName("SSDOParams");
        Vec4 param1(radius * 0.5f * rd->m_ProjMatrix.m00, radius * 0.5f * rd->m_ProjMatrix.m11,
            CRenderer::CV_r_ssdoRadiusMin, CRenderer::CV_r_ssdoRadiusMax);
        pShader->FXSetPSFloat(ssdoParamsName, &param1, 1);

        static CCryNameR viewspaceParamName("ViewSpaceParams");
        Vec4 viewSpaceParam(2.0f / rd->m_ProjMatrix.m00, 2.0f / rd->m_ProjMatrix.m11, -1.0f / rd->m_ProjMatrix.m00, -1.0f / rd->m_ProjMatrix.m11);
        pShader->FXSetPSFloat(viewspaceParamName, &viewSpaceParam, 1);

        Matrix44A matView = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_cam.GetViewMatrix();
        // Adjust the camera matrix so that the camera space will be: +y = down, +z - towards, +x - right
        Vec3 zAxis = matView.GetRow(1);
        matView.SetRow(1, -matView.GetRow(2));
        matView.SetRow(2, zAxis);
        float z = matView.m13;
        matView.m13 = -matView.m23;
        matView.m23 = z;

        static CCryNameR camMatrixName("SSDO_CameraMatrix");
        pShader->FXSetPSFloat(camMatrixName, (Vec4*)matView.GetData(), 3);

        static CCryNameR camMatrixInvName("SSDO_CameraMatrixInv");
        matView.Invert();
        pShader->FXSetPSFloat(camMatrixInvName, (Vec4*)matView.GetData(), 3);

        if (pHeightMapFrustum)  // Heightmap AO
        {
            static CCryNameR paramNameHMAO("HMAO_Params");
            Vec4 paramHMAO(CRenderer::CV_r_HeightMapAOAmount, 1.0f / pHeightMapFrustum->nTexSize, 0, 0);
            pShader->FXSetPSFloat(paramNameHMAO, &paramHMAO, 1);
        }

        m_passObscurance.Execute();
    }

    // Filtering pass
    if (CRenderer::CV_r_ssdo != 99)
    {
        CShader* pShader = rd->m_cEF.s_ShaderShadowBlur;
        const int32 sizeX = CTexture::s_ptexZTarget->GetWidth();
        const int32 sizeY = CTexture::s_ptexZTarget->GetHeight();
        const int32 srcSizeX = pDestRT->GetWidth();
        const int32 srcSizeY = pDestRT->GetHeight();

        static CCryNameTSCRC tech("SSDO_Blur");

        m_passFilter.SetRenderTarget(0, CTexture::s_ptexSceneNormalsBent);
        m_passFilter.SetTechnique(pShader, tech, 0);
        m_passFilter.SetState(GS_NODEPTHTEST);
        m_passFilter.SetTextureSamplerPair(0, pDestRT, texStateLinear);
        m_passFilter.SetTextureSamplerPair(1, CTexture::s_ptexZTarget, texStatePoint);

        static CCryNameR pixelOffsetName("PixelOffset");
        static CCryNameR blurOffsetName("BlurOffset");
        static CCryNameR blurKernelName("SSAO_BlurKernel");

        m_passFilter.BeginConstantUpdate();

        Vec4 v(0, 0, (float)srcSizeX, (float)srcSizeY);
        pShader->FXSetVSFloat(pixelOffsetName, &v, 1);
        v = Vec4(0.5f / (float)sizeX, 0.5f / (float)sizeY, 1.0f / (float)srcSizeX, 1.0f / (float)srcSizeY);
        pShader->FXSetPSFloat(blurOffsetName, &v, 1);
        v = Vec4(2.0f / srcSizeX, 0, 2.0f / srcSizeY, 10.0f); // w: weight coef
        pShader->FXSetPSFloat(blurKernelName, &v, 1);

        m_passFilter.Execute();
    }
    else  // For debugging
    {
        PostProcessUtils().StretchRect(pDestRT, CTexture::s_ptexSceneNormalsBent);
    }

    if (CRenderer::CV_r_ssdoColorBleeding)
    {
        // Generate low frequency scene albedo for color bleeding (convolution not gamma correct but acceptable)
        m_passAlbedoDownsample0.Execute(CTexture::s_ptexSceneDiffuse, CTexture::s_ptexBackBufferScaled[0]);
        m_passAlbedoDownsample1.Execute(CTexture::s_ptexBackBufferScaled[0], CTexture::s_ptexBackBufferScaled[1]);
        m_passAlbedoDownsample2.Execute(CTexture::s_ptexBackBufferScaled[1], CTexture::s_ptexAOColorBleed);
        m_passAlbedoBlur.Execute(CTexture::s_ptexAOColorBleed, CTexture::s_ptexBackBufferScaled[0], 1.0f, 4.0f);
    }
}
