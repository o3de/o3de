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

#pragma once

#include "Common/GraphicsPipeline.h"
#include "Common/GraphicsPipelineStateSet.h"
#include <RenderBus.h>

class CAutoExposurePass;
class CBloomPass;
class CScreenSpaceObscurancePass;
class CScreenSpaceReflectionsPass;
class CScreenSpaceSSSPass;
class CMotionBlurPass;
class DepthOfFieldPass;
class PostAAPass;
class VideoRenderPass;
class CCamera;

struct DepthOfFieldParameters;

enum ERenderableTechnique
{
    RS_TECH_GBUFPASS,   
    RS_TECH_ZPREPASS,   
    RS_TECH_SHADOWPASS,
    RS_TECH_NUM
};

class CStandardGraphicsPipeline
    : public CGraphicsPipeline
    , AZ::RenderNotificationsBus::Handler
{
public:
    CStandardGraphicsPipeline();
    virtual ~CStandardGraphicsPipeline();

    struct ViewParameters
    {
        ViewParameters(const CameraViewParameters& params, const CCamera& ccamera)
            : viewParameters(params)
            , camera(ccamera)
        {}

        const CameraViewParameters& viewParameters;
        const CCamera& camera;
        const Plane* pFrustumPlanes;

        Matrix44A m_ViewMatrix;
        Matrix44A m_ViewProjNoTranslateMatrix;
        Matrix44A m_ViewProjNoTranslatePrevMatrix;
        Matrix44A m_ViewProjNoTranslatePrevNearestMatrix;
        Matrix44A m_ViewProjMatrix;
        Matrix44A m_ViewProjPrevMatrix;
        Matrix44A m_ProjMatrix;

        Vec3 m_WorldViewPreviousPosition;

        D3D11_VIEWPORT viewport;
        Vec4 downscaleFactor;

        bool bReverseDepth : 1;
        bool bMirrorCull   : 1;
    };

    struct ShadowParameters
    {
        const ShadowMapFrustum* m_ShadowFrustum;
        AZ::u8 m_OmniLightSideIndex;
        Vec3 m_ViewerPos;
    };

    void Init() override;
    void Shutdown() override;

    void Prepare() override;
    void Execute() override;

    void Reset() override;

    void UpdatePerViewConstantBuffer();
    void UpdatePerViewConstantBuffer(const ViewParameters& viewInfo);
    void BindPerViewConstantBuffer();

    void UpdatePerShadowConstantBuffer(const ShadowParameters& parameters);
    AzRHI::ConstantBufferPtr GetPerShadowConstantBuffer() const
    {
        return m_PerShadowConstantBuffer;
    }

    void UpdatePerFrameConstantBuffer(const PerFrameParameters& perFrameParams);
    void BindPerFrameConstantBuffer();
    AzRHI::ConstantBufferPtr GetPerFrameConstantBuffer() const
    {
        return m_PerFrameConstantBuffer;
    }

    void ResetRenderState();

    // Partial pipeline functions, will be removed once the entire pipeline is implemented in Execute()
    void RenderAutoExposure();
    void RenderBloom();
    void RenderScreenSpaceObscurance();
    void RenderScreenSpaceReflections();
    void RenderScreenSpaceSSS(CTexture* pIrradianceTex);
    void RenderMotionBlur();
    void RenderDepthOfField();

    void RenderFinalComposite(CTexture* sourceTexture);
    void RenderPostAA();
    void RenderTemporalAA(CTexture* sourceTexture, CTexture* outputTarget, const DepthOfFieldParameters& depthOfFieldParameters);

    void RenderVideo(const AZ::VideoRenderer::DrawArguments& drawArguments);

    AzRHI::ConstantBufferPtr GetPerViewConstantBuffer() const      { return m_PerViewConstantBuffer; }
    CDeviceResourceSetPtr GetDefaultMaterialResources() const      { return m_pDefaultMaterialResources; }
    CDeviceResourceSetPtr GetDefaultInstanceExtraResources() const { return m_pDefaultInstanceExtraResources; }

private:
    void OnRendererFreeResources(int flags) override;

    CAutoExposurePass*            m_pAutoExposurePass;
    CBloomPass*                   m_pBloomPass;
    CScreenSpaceObscurancePass*   m_pScreenSpaceObscurancePass;
    CScreenSpaceReflectionsPass*  m_pScreenSpaceReflectionsPass;
    CScreenSpaceSSSPass*          m_pScreenSpaceSSSPass;
    CMotionBlurPass*              m_pMotionBlurPass;
    DepthOfFieldPass*            m_pDepthOfFieldPass;
    PostAAPass*                  m_pPostAAPass;
    VideoRenderPass*              m_pVideoRenderPass;

    AzRHI::ConstantBufferPtr      m_PerFrameConstantBuffer;
    AzRHI::ConstantBufferPtr      m_PerViewConstantBuffer;
    AzRHI::ConstantBufferPtr      m_PerShadowConstantBuffer;
    CDeviceResourceSetPtr         m_pDefaultMaterialResources;
    CDeviceResourceSetPtr         m_pDefaultInstanceExtraResources;
};

class SubpixelJitter
{
public:
    enum Pattern
    {
        Pattern_None = 0,
        Pattern_2x,
        Pattern_3x,
        Pattern_4x,
        Pattern_8x,
        Pattern_SparseGrid8x,
        Pattern_Random,
        Pattern_Halton8x,
        Pattern_HaltonRandom,
        Pattern_Count
    };

    struct Sample
    {
        float m_mipBias;
        Vec2 m_subpixelOffset;
    };

    static Sample EvaluateSample(AZ::u32 counter, Pattern pattern);
};
