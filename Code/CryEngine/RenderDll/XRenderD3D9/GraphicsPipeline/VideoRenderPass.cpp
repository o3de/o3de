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
#include "VideoRenderPass.h"
#include <IVideoRenderer.h>

#include "D3DPostProcess.h"
#include "../../Common/Textures/TextureManager.h"

VideoRenderPass::VideoRenderPass()
{
}

VideoRenderPass::~VideoRenderPass()
{
}

void VideoRenderPass::Init()
{
    m_samplerState = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
    m_passConstants.CreateDeviceBuffer();
}

void VideoRenderPass::Shutdown()
{
}

void VideoRenderPass::Reset()
{
}

void VideoRenderPass::Execute(const AZ::VideoRenderer::DrawArguments& drawArguments)
{
    CTexture* outputTexture{};
    CTexture* inputTextures[AZ::VideoRenderer::MaxInputTextureCount]{};

    // Gather textures and update them with any data passed in
    {
        auto FindTexture = [](uint32 textureId) -> CTexture*
        {
            return textureId != 0 ? CTexture::GetByID(textureId) : nullptr;
        };

        outputTexture = FindTexture(drawArguments.m_textures.m_outputTextureId);

        for (uint32 textureIndex = 0; textureIndex < AZ::VideoRenderer::MaxInputTextureCount; textureIndex++)
        {
            const uint32 inputTextureId = drawArguments.m_textures.m_inputTextureIds[textureIndex];
            const void*  updateData = drawArguments.m_updateData.m_inputTextureData[textureIndex].m_data;

            CTexture* inputTexture = FindTexture(inputTextureId);

            if (inputTexture != nullptr)
            {
                if (updateData != nullptr)
                {
                    const ETEX_Format updataDataFormat = drawArguments.m_updateData.m_inputTextureData[textureIndex].m_dataFormat;

                    const uint32 textureWidth = inputTexture->GetWidthNonVirtual();
                    const uint32 textureHeight = inputTexture->GetHeightNonVirtual();

                    inputTexture->UpdateTextureRegion(reinterpret_cast<const uint8_t*>(updateData), 0, 0, 0, textureWidth, textureHeight, 1, updataDataFormat);
                }

                inputTextures[textureIndex] = inputTexture;
            }
        }
    }

    const bool drawingToBackbuffer = (drawArguments.m_drawingToBackbuffer != 0);

    if (outputTexture == nullptr && !drawingToBackbuffer)
    {
        return;
    }

    // Update Constants
    {
        m_passConstants->VideoTexture0Scale = drawArguments.m_textureScales[0];
        m_passConstants->VideoTexture1Scale = drawArguments.m_textureScales[1];
        m_passConstants->VideoTexture2Scale = drawArguments.m_textureScales[2];
        m_passConstants->VideoTexture3Scale = drawArguments.m_textureScales[3];
        m_passConstants->VideoColorAdjustment = drawArguments.m_colorAdjustment;
        m_passConstants.CopyToDevice();
    }

    CD3D9Renderer* pRend = gcpRendD3D;
    SRenderPipeline& RP = pRend->m_RP;
    CShader* pShader = CShaderMan::s_ShaderVideo;

    CTexture* pBlackTexture = CTextureManager::Instance()->GetBlackTexture();

    // Save the flags for restoring after we execute
    const uint64 saveFlagsRT = RP.m_FlagsShader_RT;
    RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3]);

    // We're using each SAMPLE# runtime flag to signify if a texture input slot is being used.

    if (drawArguments.m_textures.m_inputTextureIds[0])
    {
        RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    }
    if (drawArguments.m_textures.m_inputTextureIds[1])
    {
        RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
    }
    if (drawArguments.m_textures.m_inputTextureIds[2])
    {
        RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];
    }
    if (drawArguments.m_textures.m_inputTextureIds[3])
    {
        RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE3];
    }

    // Save the viewport for restoring later.
    int origViewportX{}, origViewportY{}, origViewportWidth{}, origViewportHeight{};
    pRend->GetViewport(&origViewportX, &origViewportY, &origViewportWidth, &origViewportHeight);

    const int drawWidth = drawingToBackbuffer ? pRend->GetOverlayWidth() : outputTexture->GetWidthNonVirtual();
    const int drawHeight = drawingToBackbuffer ? pRend->GetOverlayHeight() : outputTexture->GetHeightNonVirtual();

    if (!drawingToBackbuffer)
    {
        pRend->FX_PushRenderTarget(0, outputTexture, nullptr);
        pRend->FX_SetActiveRenderTargets();
    }

    pRend->RT_SetViewport(0, 0, drawWidth, drawHeight);

    static CCryNameTSCRC techVideoRender("VideoRender");
    GetUtils().ShBeginPass(pShader, techVideoRender, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    pRend->m_DevMan.BindConstantBuffer(eHWSC_Pixel, m_passConstants.GetDeviceConstantBuffer(), 0);

    for (int index = 0; index < AZ::VideoRenderer::MaxInputTextureCount; index++)
    {
        CTexture* const inputTexture = inputTextures[index] ? inputTextures[index] : pBlackTexture;
        if (inputTexture)
        {
            inputTexture->ApplyTexture(index, eHWSC_Pixel, SResourceView::DefaultView);
        }
    }

    CTexture::SetSamplerState(m_samplerState, 0, eHWSC_Pixel);

    SPostEffectsUtils::DrawFullScreenTriWPOS(drawWidth, drawHeight);

    GetUtils().ShEndPass();

    if (!drawingToBackbuffer)
    {
        pRend->FX_PopRenderTarget(0);
        pRend->FX_SetActiveRenderTargets();
    }

    // Restore the viewport
    pRend->RT_SetViewport(origViewportX, origViewportY, origViewportWidth, origViewportHeight);

    // Restore the flags we saved earlier
    RP.m_FlagsShader_RT = saveFlagsRT;
}
