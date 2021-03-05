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
#include "StandardGraphicsPipeline.h"

#include "AutoExposure.h"
#include "Bloom.h"
#include "ScreenSpaceObscurance.h"
#include "ScreenSpaceReflections.h"
#include "ScreenSpaceSSS.h"
#include "MotionBlur.h"
#include "DepthOfField.h"
#include "PostAA.h"
#include "VideoRenderPass.h"
#include "Common/TypedConstantBuffer.h"
#include "Common/Textures/TextureHelpers.h"
#include "Common/Include_HLSL_CPP_Shared.h"
#include "MultiLayerAlphaBlendPass.h"

#if defined(FEATURE_SVO_GI)
    #include "D3D_SVO.h"
#endif

#include "DriverD3D.h" //for gcpRendD3D

CStandardGraphicsPipeline::CStandardGraphicsPipeline()
{
    AZ::RenderNotificationsBus::Handler::BusConnect();
}

CStandardGraphicsPipeline::~CStandardGraphicsPipeline()
{
    AZ::RenderNotificationsBus::Handler::BusDisconnect();
}

void CStandardGraphicsPipeline::Init()
{
    RegisterPass<CAutoExposurePass>(m_pAutoExposurePass);
    RegisterPass<CBloomPass>(m_pBloomPass);
    RegisterPass<CScreenSpaceObscurancePass>(m_pScreenSpaceObscurancePass);
    RegisterPass<CScreenSpaceReflectionsPass>(m_pScreenSpaceReflectionsPass);
    RegisterPass<CScreenSpaceSSSPass>(m_pScreenSpaceSSSPass);
    RegisterPass<CMotionBlurPass>(m_pMotionBlurPass);
    RegisterPass<DepthOfFieldPass>(m_pDepthOfFieldPass);
    RegisterPass<PostAAPass>(m_pPostAAPass);
    RegisterPass<VideoRenderPass>(m_pVideoRenderPass);

    // default material resources
    {
        m_pDefaultMaterialResources = CDeviceObjectFactory::GetInstance().CreateResourceSet();
        m_pDefaultMaterialResources->SetConstantBuffer(eConstantBufferShaderSlot_PerMaterial, NULL, EShaderStage_AllWithoutCompute);

        for (EEfResTextures texType = EFTT_DIFFUSE; texType < EFTT_MAX; texType = EEfResTextures(texType + 1))
        {
            CTexture* pDefaultTexture = TextureHelpers::LookupTexDefault(texType);
            m_pDefaultMaterialResources->SetTexture(texType, pDefaultTexture, SResourceView::DefaultView, EShaderStage_AllWithoutCompute);
        }
    }

    // default extra per instance
    {
        EShaderStage shaderStages = EShaderStage_Vertex | EShaderStage_Hull | EShaderStage_Domain;
        m_pDefaultInstanceExtraResources = CDeviceObjectFactory::GetInstance().CreateResourceSet();
        m_pDefaultInstanceExtraResources->SetConstantBuffer(eConstantBufferShaderSlot_SkinQuat,     NULL, shaderStages);
        m_pDefaultInstanceExtraResources->SetConstantBuffer(eConstantBufferShaderSlot_SkinQuatPrev, NULL, shaderStages);
        m_pDefaultInstanceExtraResources->SetBuffer(EReservedTextureSlot_SkinExtraWeights, WrappedDX11Buffer(), shaderStages);
        m_pDefaultInstanceExtraResources->SetBuffer(EReservedTextureSlot_AdjacencyInfo, WrappedDX11Buffer(), shaderStages); // shares shader slot with EReservedTextureSlot_PatchID
    }
}

void CStandardGraphicsPipeline::OnRendererFreeResources(int flags)
{
    // If texture resources are about to be freed by the renderer
    if (flags & FRR_TEXTURES)
    {
        // Release default resources before CTexture::Shutdown is called so they do not leak
        m_pDefaultMaterialResources = nullptr;
        m_pDefaultInstanceExtraResources = nullptr;
    }
}

void CStandardGraphicsPipeline::Shutdown()
{
    for (auto& pass : m_passes)
    {
        pass->Shutdown();
    }

    m_passes.clear();
    m_pDefaultMaterialResources = nullptr;
}

void CStandardGraphicsPipeline::Prepare()
{
    AZ_TRACE_METHOD();
    for (auto& pass : m_passes)
    {
        pass->Prepare();
    }
}

void CStandardGraphicsPipeline::Execute()
{
}

void CStandardGraphicsPipeline::Reset()
{
    for (const auto& pass : m_passes)
    {
        pass->Reset();
    }
}

static const SRenderLight* FindSunLight(SRenderPipeline& renderPipeline)
{
    // We explicitly search for the sun because the pipeline sunlight value
    // gets reset several times a frame, so it's not guaranteed to exist.
    const TArray<SRenderLight>& lights = renderPipeline.m_DLights[renderPipeline.m_nProcessThreadID][SRendItem::m_RecurseLevel[renderPipeline.m_nProcessThreadID]];

    for (AZ::u32 i = 0; i < lights.Num(); ++i)
    {
        const SRenderLight* light = &lights[i];
        if (light->m_Flags & DLF_SUN)
        {
            return light;
        }
    }
    return nullptr;
}

void CStandardGraphicsPipeline::UpdatePerFrameConstantBuffer(const PerFrameParameters& perFrameParams)
{
    CD3D9Renderer* renderer = gcpRendD3D;
    const PerFrameParameters& perFrameConstants = renderer->m_cEF.m_PF;
    SRenderPipeline& RESTRICT_REFERENCE rp = gRenDev->m_RP;

    CTypedConstantBuffer<HLSL_PerFrameConstantBuffer> cb(m_PerFrameConstantBuffer);

    cb->PerFrame_VolumetricFogParams = perFrameParams.m_VolumetricFogParams;
    cb->PerFrame_VolumetricFogRampParams = perFrameParams.m_VolumetricFogRampParams;
    cb->PerFrame_VolumetricFogColorGradientBase = perFrameParams.m_VolumetricFogColorGradientBase;
    cb->PerFrame_VolumetricFogColorGradientDelta = perFrameParams.m_VolumetricFogColorGradientDelta;
    cb->PerFrame_VolumetricFogColorGradientParams = perFrameParams.m_VolumetricFogColorGradientParams;
    cb->PerFrame_VolumetricFogColorGradientRadial = perFrameParams.m_VolumetricFogColorGradientRadial;
    cb->PerFrame_VolumetricFogSamplingParams = perFrameParams.m_VolumetricFogSamplingParams;
    cb->PerFrame_VolumetricFogDistributionParams = perFrameParams.m_VolumetricFogDistributionParams;
    cb->PerFrame_VolumetricFogScatteringParams = perFrameParams.m_VolumetricFogScatteringParams;
    cb->PerFrame_VolumetricFogScatteringBlendParams = perFrameParams.m_VolumetricFogScatteringBlendParams;
    cb->PerFrame_VolumetricFogScatteringColor = perFrameParams.m_VolumetricFogScatteringColor;
    cb->PerFrame_VolumetricFogScatteringSecondaryColor = perFrameParams.m_VolumetricFogScatteringSecondaryColor;
    cb->PerFrame_VolumetricFogHeightDensityParams = perFrameParams.m_VolumetricFogHeightDensityParams;
    cb->PerFrame_VolumetricFogHeightDensityRampParams = perFrameParams.m_VolumetricFogHeightDensityRampParams;
    cb->PerFrame_VolumetricFogDistanceParams = perFrameParams.m_VolumetricFogDistanceParams;
    cb->PerFrame_VolumetricFogGlobalEnvProbe0 = renderer->GetVolumetricFog().GetGlobalEnvProbeShaderParam0();
    cb->PerFrame_VolumetricFogGlobalEnvProbe1 = renderer->GetVolumetricFog().GetGlobalEnvProbeShaderParam1();

#if defined (FEATURE_SVO_GI)
    if (auto* svoRenderer = CSvoRenderer::GetInstance())
    {
        cb->PerFrame_SvoLightingParams = svoRenderer->GetPerFrameShaderParameters();
    }
    else
    {
        cb->PerFrame_SvoLightingParams = CSvoRenderer::GetDisabledPerFrameShaderParameters();
    }
#endif

    const float time = CRenderer::GetRealTime();

    cb->PerFrame_Time = Vec4(time, CRenderer::GetElapsedTime(), time - CRenderer::GetElapsedTime(), perFrameParams.m_MidDayIndicator );

    const SRenderLight* sunLight = FindSunLight(renderer->m_RP);
    if (sunLight)
    {
        Vec3 sunDirectionNormalized = sunLight->GetPosition().normalized();
        cb->PerFrame_SunDirection = Vec4(sunDirectionNormalized, 1.0);
        cb->PerFrame_SunColor = Vec4(sunLight->m_Color.r, sunLight->m_Color.g, sunLight->m_Color.b, perFrameParams.m_SunSpecularMultiplier);
    }
    else
    {
        cb->PerFrame_SunDirection = Vec4(0.0f);
        cb->PerFrame_SunColor = Vec4(0.0f);
    }

    cb->PerFrame_CloudShadingColorSun = Vec4(perFrameParams.m_CloudShadingColorSun, 0.0f);
    cb->PerFrame_CloudShadingColorSky = Vec4(perFrameParams.m_CloudShadingColorSky, 0.0f);
    cb->PerFrame_CloudShadowParams = perFrameParams.m_CloudShadowParams;
    cb->PerFrame_CloudShadowAnimParams = perFrameParams.m_CloudShadowAnimParams;

    cb->PerFrame_CausticsSmoothSunDirection = Vec4(perFrameParams.m_CausticsSunDirection, 0.0f);

    cb->PerFrame_DecalZFightingRemedy = Vec4(perFrameParams.m_DecalZFightingRemedy, CD3D9Renderer::CV_r_ssdoAmountDirect);
    cb->PerFrame_WaterLevel = Vec4(perFrameParams.m_WaterLevel, 0.0f);

    cb->PerFrame_HDRParams = perFrameParams.m_HDRParams;

    {
        auto& stereoRenderer = renderer->GetS3DRend();
        cb->PerFrame_StereoParams = Vec4(
            stereoRenderer.GetMaxSeparationScene() * (stereoRenderer.GetStatus() == IStereoRenderer::Status::kRenderingFirstEye ? 1 : -1),
            stereoRenderer.GetZeroParallaxPlaneDist(),
            stereoRenderer.GetNearGeoShift(),
            stereoRenderer.GetNearGeoScale());
    }

    cb->PerFrame_RandomParams = Vec4(cry_random(0.0f, 1.0f), cry_random(0.0f, 1.0f), cry_random(0.0f, 1.0f), cry_random(0.0f, 1.0f));

    cb->PerFrame_MultiLayerAlphaBlendLayerData.x = MultiLayerAlphaBlendPass::GetInstance().GetLayerCount();

    m_PerFrameConstantBuffer = cb.GetDeviceConstantBuffer();
    cb.CopyToDevice();
}

void CStandardGraphicsPipeline::BindPerFrameConstantBuffer()
{
    auto& deviceManager = gcpRendD3D->m_DevMan;

    AzRHI::ConstantBuffer* perFrame = GetPerFrameConstantBuffer().get();
    deviceManager.BindConstantBuffer(eHWSC_Vertex, perFrame, eConstantBufferShaderSlot_PerFrame);
    deviceManager.BindConstantBuffer(eHWSC_Geometry, perFrame, eConstantBufferShaderSlot_PerFrame);
    deviceManager.BindConstantBuffer(eHWSC_Hull, perFrame, eConstantBufferShaderSlot_PerFrame);
    deviceManager.BindConstantBuffer(eHWSC_Domain, perFrame, eConstantBufferShaderSlot_PerFrame);
    deviceManager.BindConstantBuffer(eHWSC_Pixel, perFrame, eConstantBufferShaderSlot_PerFrame);
    deviceManager.BindConstantBuffer(eHWSC_Compute, perFrame, eConstantBufferShaderSlot_PerFrame);
}

void CStandardGraphicsPipeline::UpdatePerViewConstantBuffer()
{
    CD3D9Renderer* pRenderer = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rp = gRenDev->m_RP;

    ViewParameters viewInfo(pRenderer->GetViewParameters(), pRenderer->GetCamera());
    viewInfo.bReverseDepth = (rp.m_TI[rp.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) != 0;
    viewInfo.bMirrorCull   = (rp.m_TI[rp.m_nProcessThreadID].m_PersFlags & RBPF_MIRRORCULL)    != 0;

    int vpX, vpY, vpWidth, vpHeight;
    pRenderer->GetViewport(&vpX, &vpY, &vpWidth, &vpHeight);
    viewInfo.viewport.TopLeftX = float(vpX);
    viewInfo.viewport.TopLeftY = float(vpY);
    viewInfo.viewport.Width    = float(vpWidth);
    viewInfo.viewport.Height   = float(vpHeight);
    viewInfo.downscaleFactor = Vec4(rp.m_CurDownscaleFactor.x, rp.m_CurDownscaleFactor.y, pRenderer->m_PrevViewportScale.x, pRenderer->m_PrevViewportScale.y);

    viewInfo.m_ViewMatrix = pRenderer->m_CameraMatrix;
    viewInfo.m_ViewProjNoTranslateMatrix = pRenderer->m_ViewProjNoTranslateMatrix;
    viewInfo.m_ViewProjNoTranslatePrevMatrix = pRenderer->GetPreviousFrameMatrixSet().m_ViewProjNoTranslateMatrix;
    viewInfo.m_ViewProjNoTranslatePrevNearestMatrix = pRenderer->GetPreviousFrameMatrixSet().m_ViewNoTranslateMatrix * pRenderer->m_ProjMatrix;
    viewInfo.m_ViewProjMatrix = pRenderer->m_ViewProjMatrix;
    viewInfo.m_ViewProjPrevMatrix = pRenderer->GetPreviousFrameMatrixSet().m_ViewProjMatrix;
    viewInfo.m_ProjMatrix = pRenderer->m_ProjMatrix;
    viewInfo.m_WorldViewPreviousPosition = pRenderer->GetPreviousFrameMatrixSet().m_WorldViewPosition;

    if (rp.m_ShadowInfo.m_pCurShadowFrustum && (rp.m_TI[rp.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN))
    {
        const SRenderPipeline::ShadowInfo& shadowInfo = rp.m_ShadowInfo;
        assert(shadowInfo.m_nOmniLightSide >= 0 && shadowInfo.m_nOmniLightSide < OMNI_SIDES_NUM);

        CCamera& cam = shadowInfo.m_pCurShadowFrustum->FrustumPlanes[shadowInfo.m_nOmniLightSide];
        viewInfo.pFrustumPlanes = cam.GetFrustumPlane(0);
    }
    else
    {
        viewInfo.pFrustumPlanes = pRenderer->GetCamera().GetFrustumPlane(0);
    }

    UpdatePerViewConstantBuffer(viewInfo);
}

void CStandardGraphicsPipeline::UpdatePerViewConstantBuffer(const ViewParameters& viewInfo)
{
    if (!gEnv->p3DEngine)
    {
        return;
    }

    CD3D9Renderer* pRenderer = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rp = gRenDev->m_RP;
    SThreadInfo& threadInfo = rp.m_TI[rp.m_nProcessThreadID];
    const PerFrameParameters& perFrameConstants = threadInfo.m_perFrameParameters;

    CTypedConstantBuffer<HLSL_PerViewConstantBuffer> cb(m_PerViewConstantBuffer);

    const float time = threadInfo.m_RealTime;

    cb->PerView_WorldViewPos = Vec4(viewInfo.viewParameters.vOrigin, viewInfo.bMirrorCull ? -1.0f : 1.0f);
    cb->PerView_WorldViewPosPrev = Vec4(viewInfo.m_WorldViewPreviousPosition, 0.0f);

    cb->PerView_HPosScale = viewInfo.downscaleFactor;
    cb->PerView_ScreenSize = Vec4(viewInfo.viewport.Width,
        viewInfo.viewport.Height,
        0.5f / (viewInfo.viewport.Width / viewInfo.downscaleFactor.x),
        0.5f / (viewInfo.viewport.Height / viewInfo.downscaleFactor.y));

    cb->PerView_ViewBasisX = Vec4(viewInfo.viewParameters.vX, 0.0f);
    cb->PerView_ViewBasisY = Vec4(viewInfo.viewParameters.vY, 0.0f);
    cb->PerView_ViewBasisZ = Vec4(viewInfo.viewParameters.vZ, 0.0f);

    cb->PerView_ViewProjZeroMatr = viewInfo.m_ViewProjNoTranslateMatrix.GetTransposed();
    cb->PerView_ViewProjZeroMatrPrev = viewInfo.m_ViewProjNoTranslatePrevMatrix.GetTransposed();
    cb->PerView_ViewProjZeroMatrPrevNearest = viewInfo.m_ViewProjNoTranslatePrevNearestMatrix.GetTransposed();
    cb->PerView_ViewProjMatr = viewInfo.m_ViewProjMatrix.GetTransposed();
    cb->PerView_ViewProjMatrPrev = viewInfo.m_ViewProjPrevMatrix.GetTransposed();
    cb->PerView_ViewMatr = viewInfo.m_ViewMatrix.GetTransposed();
    cb->PerView_ProjMatr = viewInfo.m_ProjMatrix.GetTransposed();

    cb->PerView_FogColor = Vec4(threadInfo.m_FS.m_CurColor.toVec3(), perFrameConstants.m_VolumetricFogParams.z);

    cb->PerView_AnimGenParams = Vec4(time * 2.0f, time * 0.5f, time * 1.0f, time * 0.125f);

    // CV_NearFarClipDist
    {
        // Note: CV_NearFarClipDist.z is used to put the weapon's depth range into correct relation to the whole scene
        // when generating the depth texture in the z pass (_RT_NEAREST)
        cb->PerView_NearFarClipDist = Vec4(viewInfo.viewParameters.fNear,
            viewInfo.viewParameters.fFar,
            viewInfo.viewParameters.fFar / gEnv->p3DEngine->GetMaxViewDistance(),
            1.0f / viewInfo.viewParameters.fFar);
    }

    // PerView_ProjRatio
    {
        float zn = viewInfo.viewParameters.fNear;
        float zf = viewInfo.viewParameters.fFar;
        float hfov = viewInfo.camera.GetHorizontalFov();
        cb->PerView_ProjRatio.x = viewInfo.bReverseDepth ? zn / (zn - zf) : zf / (zf - zn);
        cb->PerView_ProjRatio.y = viewInfo.bReverseDepth ? zn / (zf - zn) : zn / (zn - zf);
        cb->PerView_ProjRatio.z = 1.0f / hfov;
        cb->PerView_ProjRatio.w = 1.0f;
        if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
        {
            //For gmem the depth values are not in linear space. 
            cb->PerView_ProjRatio.w = 1.0f / zf;
        }
    }

    // PerView_NearestScaled
    {
        float zn = DRAW_NEAREST_MIN;
        float zf = CRenderer::CV_r_DrawNearFarPlane;
        float nearZRange = CRenderer::CV_r_DrawNearZRange;
        float camScale = pRenderer->CV_r_DrawNearFarPlane / gEnv->p3DEngine->GetMaxViewDistance();
        cb->PerView_NearestScaled.x = viewInfo.bReverseDepth ? 1.0f - zf / (zf - zn) * nearZRange : zf / (zf - zn) * nearZRange;
        cb->PerView_NearestScaled.y = viewInfo.bReverseDepth ? zn / (zf - zn) * nearZRange * camScale : zn / (zn - zf) * nearZRange * camScale;
        cb->PerView_NearestScaled.z = viewInfo.bReverseDepth ? 1.0f - (nearZRange - 0.001f) : nearZRange - 0.001f;
        cb->PerView_NearestScaled.w = 1.0f;

        if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
        {
            cb->PerView_NearestScaled.w = 1.0f / pRenderer->CV_r_DrawNearFarPlane;
        }
    }

    // PerView_TessInfo
    {
        // We want to obtain the edge length in pixels specified by CV_r_tessellationtrianglesize
        // Therefore the tess factor would depend on the viewport size and CV_r_tessellationtrianglesize
        static const ICVar* pCV_e_TessellationMaxDistance(gEnv->pConsole->GetCVar("e_TessellationMaxDistance"));
        assert(pCV_e_TessellationMaxDistance);

        const float hfov = viewInfo.camera.GetHorizontalFov();
        cb->PerView_TessellationParams.x = sqrtf(float(viewInfo.viewport.Width * viewInfo.viewport.Height)) / (hfov * pRenderer->CV_r_tessellationtrianglesize);
        cb->PerView_TessellationParams.y = pRenderer->CV_r_displacementfactor;
        cb->PerView_TessellationParams.z = pCV_e_TessellationMaxDistance->GetFVal();
        cb->PerView_TessellationParams.w = (float)CRenderer::CV_r_ParticlesTessellationTriSize;
    }

    cb->PerView_FrustumPlaneEquation.SetRow4(0, (Vec4&)viewInfo.pFrustumPlanes[FR_PLANE_RIGHT]);
    cb->PerView_FrustumPlaneEquation.SetRow4(1, (Vec4&)viewInfo.pFrustumPlanes[FR_PLANE_LEFT]);
    cb->PerView_FrustumPlaneEquation.SetRow4(2, (Vec4&)viewInfo.pFrustumPlanes[FR_PLANE_TOP]);
    cb->PerView_FrustumPlaneEquation.SetRow4(3, (Vec4&)viewInfo.pFrustumPlanes[FR_PLANE_BOTTOM]);

    const bool bApplySubpixelShift = !(threadInfo.m_PersFlags & (RBPF_DRAWTOTEXTURE | RBPF_SHADOWGEN));
    if (bApplySubpixelShift)
    {
        cb->PerView_JitterParams = gcpRendD3D->m_TemporalJitterClipSpace;
    }
    else
    {
        cb->PerView_JitterParams = Vec4(0.0);
    }

    m_PerViewConstantBuffer = cb.GetDeviceConstantBuffer();
    cb.CopyToDevice();
}

void CStandardGraphicsPipeline::BindPerViewConstantBuffer()
{
    auto& deviceManager = gcpRendD3D->m_DevMan;

    AzRHI::ConstantBuffer* perView = GetPerViewConstantBuffer().get();
    deviceManager.BindConstantBuffer(eHWSC_Vertex, perView, eConstantBufferShaderSlot_PerView);
    deviceManager.BindConstantBuffer(eHWSC_Geometry, perView, eConstantBufferShaderSlot_PerView);
    deviceManager.BindConstantBuffer(eHWSC_Hull, perView, eConstantBufferShaderSlot_PerView);
    deviceManager.BindConstantBuffer(eHWSC_Domain, perView, eConstantBufferShaderSlot_PerView);
    deviceManager.BindConstantBuffer(eHWSC_Pixel, perView, eConstantBufferShaderSlot_PerView);
    deviceManager.BindConstantBuffer(eHWSC_Compute, perView, eConstantBufferShaderSlot_PerView);
}

void CStandardGraphicsPipeline::UpdatePerShadowConstantBuffer(const ShadowParameters& params)
{
    CD3D9Renderer* renderer = gcpRendD3D;
    const SRenderPipeline& renderPipeline = renderer->m_RP;
    const auto& shadowFrustum = *params.m_ShadowFrustum;

    CTypedConstantBuffer<HLSL_PerSubPassConstantBuffer_ShadowGen> cb(m_PerShadowConstantBuffer);

    cb->PerShadow_FrustumInfo = Vec4(
        shadowFrustum.fNearDist,
        shadowFrustum.m_eFrustumType != ShadowMapFrustum::e_HeightMapAO ? shadowFrustum.fFarDist : 1.f,
        0.0f,
        0.0f);
    cb->PerShadow_LightPos = Vec4(params.m_ShadowFrustum->vLightSrcRelPos + params.m_ShadowFrustum->vProjTranslation, 0);
    cb->PerShadow_ViewPos = Vec4(params.m_ViewerPos, 0);

    const float UNUSED = 0.0f;
    cb->PerShadow_BiasInfo = Vec4(shadowFrustum.fDepthSlopeBias, UNUSED, UNUSED, UNUSED);

    m_PerShadowConstantBuffer = cb.GetDeviceConstantBuffer();
    cb.CopyToDevice();
}

void CStandardGraphicsPipeline::ResetRenderState()
{
    CD3D9Renderer* rd = gcpRendD3D;

    rd->m_nCurStateRS = (uint32) - 1;
    rd->m_nCurStateBL = (uint32) - 1;
    rd->m_nCurStateDP = (uint32) - 1;
    rd->ResetToDefault();
    rd->FX_SetState(0, 0, 0xFFFFFFFF);
    rd->D3DSetCull(eCULL_Back);

    rd->m_bViewportDirty = true;
    rd->m_CurViewport = SViewport();
    rd->FX_SetViewport();

    rd->m_CurTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    rd->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

#ifdef CRY_USE_DX12
    rd->GetDeviceContext().ResetCachedState();
#endif

    CHWShader::s_pCurPS = nullptr;
    CHWShader::s_pCurVS = nullptr;
    CHWShader::s_pCurGS = nullptr;
    CHWShader::s_pCurDS = nullptr;
    CHWShader::s_pCurHS = nullptr;
    CHWShader::s_pCurCS = nullptr;

    CDeviceObjectFactory::GetInstance().GetCoreGraphicsCommandList()->Reset();
}

void CStandardGraphicsPipeline::RenderAutoExposure()
{
    m_pAutoExposurePass->Execute();
    ResetRenderState();
}

void CStandardGraphicsPipeline::RenderBloom()
{
    CDeviceObjectFactory::GetInstance().GetCoreGraphicsCommandList()->SwitchToNewGraphicsPipeline();
    m_pBloomPass->Execute();
    ResetRenderState();
}

void CStandardGraphicsPipeline::RenderScreenSpaceObscurance()
{
    CDeviceObjectFactory::GetInstance().GetCoreGraphicsCommandList()->SwitchToNewGraphicsPipeline();
    m_pScreenSpaceObscurancePass->Execute();
    ResetRenderState();
}

void CStandardGraphicsPipeline::RenderScreenSpaceReflections()
{
    CDeviceObjectFactory::GetInstance().GetCoreGraphicsCommandList()->SwitchToNewGraphicsPipeline();
    m_pScreenSpaceReflectionsPass->Execute();
    ResetRenderState();
}

void CStandardGraphicsPipeline::RenderScreenSpaceSSS(CTexture* pIrradianceTex)
{
    CDeviceObjectFactory::GetInstance().GetCoreGraphicsCommandList()->SwitchToNewGraphicsPipeline();
    m_pScreenSpaceSSSPass->Execute(pIrradianceTex);
    ResetRenderState();
}

void CStandardGraphicsPipeline::RenderMotionBlur()
{
    CDeviceObjectFactory::GetInstance().GetCoreGraphicsCommandList()->SwitchToNewGraphicsPipeline();
    m_pMotionBlurPass->Execute();
    ResetRenderState();
}

void CStandardGraphicsPipeline::RenderDepthOfField()
{
    m_pDepthOfFieldPass->Execute();
}

void CStandardGraphicsPipeline::RenderTemporalAA(CTexture* sourceTexture, CTexture* outputTarget, const DepthOfFieldParameters& depthOfFieldParameters)
{
    m_pPostAAPass->RenderTemporalAA(sourceTexture, outputTarget, depthOfFieldParameters);
}

void CStandardGraphicsPipeline::RenderFinalComposite(CTexture* sourceTexture)
{
    m_pPostAAPass->RenderFinalComposite(sourceTexture);
}

void CStandardGraphicsPipeline::RenderPostAA()
{
    m_pPostAAPass->Execute();
}

SubpixelJitter::Sample SubpixelJitter::EvaluateSample(AZ::u32 counter, Pattern pattern)
{
    static const Vec2 vSSAA2x[2] =
    {
        Vec2(-0.25f, 0.25f),
        Vec2(0.25f, -0.25f)
    };

    static const Vec2 vSSAA3x[3] =
    {
        Vec2(-1.0f / 3.0f, -1.0f / 3.0f),
        Vec2(1.0f / 3.0f,  0.0f / 3.0f),
        Vec2(0.0f / 3.0f,  1.0f / 3.0f)
    };

    static const Vec2 vSSAA4x[4] =
    {
        Vec2(-0.125, -0.375), Vec2(0.375, -0.125),
        Vec2(-0.375,  0.125), Vec2(0.125,  0.375)
    };

    static const Vec2 vSSAA8x[8] =
    {
        Vec2(0.0625, -0.1875),  Vec2(-0.0625,  0.1875),
        Vec2(0.3125,  0.0625),  Vec2(-0.1875, -0.3125),
        Vec2(-0.3125,  0.3125), Vec2(-0.4375, -0.0625),
        Vec2(0.1875,  0.4375),  Vec2(0.4375, -0.4375)
    };

    static const Vec2 vSGSSAA8x8[8] =
    {
        Vec2(6.0f / 7.0f, 0.0f / 7.0f) - Vec2(0.5f, 0.5f), Vec2(2.0f / 7.0f, 1.0f / 7.0f) - Vec2(0.5f, 0.5f),
        Vec2(4.0f / 7.0f, 2.0f / 7.0f) - Vec2(0.5f, 0.5f), Vec2(0.0f / 7.0f, 3.0f / 7.0f) - Vec2(0.5f, 0.5f),
        Vec2(7.0f / 7.0f, 4.0f / 7.0f) - Vec2(0.5f, 0.5f), Vec2(3.0f / 7.0f, 5.0f / 7.0f) - Vec2(0.5f, 0.5f),
        Vec2(5.0f / 7.0f, 6.0f / 7.0f) - Vec2(0.5f, 0.5f), Vec2(1.0f / 7.0f, 7.0f / 7.0f) - Vec2(0.5f, 0.5f)
    };

    // Mip bias value, numbers are the new pixel gradient radius.
    static float JitterMipBias[] =
    {
        0.0f,
        log2(0.707f),   // 2x
        log2(0.5f),     // 3x
        log2(0.5f),     // 4x
        log2(0.375f),   // 8x
        log2(0.375f),    // 8x
        log2(0.375f),    // random
        log2(0.375f),    // 8x
        log2(0.375f)     // 8x
    };

    static_assert(AZ_ARRAY_SIZE(JitterMipBias) == Pattern_Count, "JitterMipBias array does not match jitter pattern enum");

    const AZ::u32 clampedJitternPattern = clamp_tpl(AZ::u32(pattern), AZ::u32(Pattern_None), AZ::u32(Pattern_Count) - 1);

    Sample sample;
    switch (clampedJitternPattern)
    {
    case Pattern_2x:
        sample.m_subpixelOffset = vSSAA2x[counter % 2];
        break;
    case Pattern_3x:
        sample.m_subpixelOffset = vSSAA3x[counter % 3];
        break;
    case Pattern_4x:
        sample.m_subpixelOffset = vSSAA4x[counter % 4];
        break;
    case Pattern_8x:
        sample.m_subpixelOffset = vSSAA8x[counter % 8];
        break;
    case Pattern_SparseGrid8x:
        sample.m_subpixelOffset = vSGSSAA8x8[counter % 8];
        break;
    case Pattern_Random:
        sample.m_subpixelOffset = Vec2(SPostEffectsUtils::srandf(), SPostEffectsUtils::srandf()) * 0.5f;
        break;
    case Pattern_Halton8x:
        sample.m_subpixelOffset = Vec2(SPostEffectsUtils::HaltonSequence(counter % 8, 2) - 0.5f,
            SPostEffectsUtils::HaltonSequence(counter % 8, 3) - 0.5f);
        break;
    case Pattern_HaltonRandom:
        sample.m_subpixelOffset = Vec2(SPostEffectsUtils::HaltonSequence(counter % 1024, 2) - 0.5f,
            SPostEffectsUtils::HaltonSequence(counter % 1024, 3) - 0.5f);
        break;

    default:
        sample.m_subpixelOffset = Vec2(0, 0);
    }

    sample.m_mipBias = JitterMipBias[clampedJitternPattern];
    return sample;
}

void CStandardGraphicsPipeline::RenderVideo(const AZ::VideoRenderer::DrawArguments& drawArguments)
{
    m_pVideoRenderPass->Execute(drawArguments);
}
