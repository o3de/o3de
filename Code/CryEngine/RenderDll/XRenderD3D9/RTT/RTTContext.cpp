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

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

#include "AzCore/std/utils.h"
#include <AzCore/Serialization/SerializeContext.h>
#include "DriverD3D.h"
#include "RTTContext.h"
#include "RTTSwappableRenderTarget.h"

namespace AzRTT 
{
    RenderContext::RenderContext(RenderContextId id, const RenderContextConfig& config)
        : RenderContext(id)
    {
        SetConfig(config);
    }

    RenderContext::RenderContext(RenderContextId id)
        : m_viewport(0, 0, 0, 0)
        , m_renderContextId(id)
        , m_depthTarget(nullptr)
        , m_depthTargetMSAA(nullptr)
        , m_zBufferDepthReadOnlySRV(nullptr)
        , m_zBufferStencilReadOnlySRV(nullptr)
        , m_active(false)
        , m_errorState(ErrorState::OK)
    {
        struct cvarIntEntry {
            const char* name;
            int* variable;
        };
        const cvarIntEntry icvars[] =
        {
            {"e_GsmLodsNum",                nullptr},
            {"e_Dissolve",                  nullptr},
            {"e_CoverageBuffer",            nullptr},
            {"e_StatObjBufferRenderTasks",  nullptr},
            {"r_FinalOutputAlpha",          &(gRenDev->CV_r_FinalOutputAlpha)},
            {"r_FinalOutputsRGB",           &(gRenDev->CV_r_FinalOutputsRGB)},
            {"r_Flares",                    &(gRenDev->CV_r_flares)},
            {"r_sunshafts",                 &(gRenDev->CV_r_sunshafts)},
            {"r_AntialiasingMode",          &(gRenDev->CV_r_AntialiasingMode)},
            {"r_MotionBlur",                &(gRenDev->CV_r_MotionBlur)},
            {"r_DepthOfField",              &(gRenDev->CV_r_dof)}
        };

        for (const cvarIntEntry& cvar : icvars)
        {
            m_iCVars.insert({ cvar.name, SwappableCVar<int>(cvar.name, cvar.variable) });
        }

        struct cvarFloatEntry {
            const char* name;
            float* variable;
        };
        const cvarFloatEntry fcvars[] =
        {
            {"e_GsmRange",                  nullptr},
            {"e_GsmRangeStep",              nullptr}
        };

        for (const cvarFloatEntry& cvar : fcvars)
        {
            m_fCVars.insert({ cvar.name, SwappableCVar<float>(cvar.name, cvar.variable) });
        }
    }

    RenderContext::~RenderContext()
    {
        SetActive(false);
        Release();
    }

    void RenderContext::Release()
    {
        m_swappableRenderTargets.clear();

        // these depth targets are dynamic and managed by the renderer so do not call Release()
        m_depthTarget = nullptr;
        m_depthTargetMSAA = nullptr;

        if (m_zBufferDepthReadOnlySRV) 
        {
            m_zBufferDepthReadOnlySRV->Release();
            m_zBufferDepthReadOnlySRV = nullptr;
        }

        if (m_zBufferStencilReadOnlySRV) 
        {
            m_zBufferStencilReadOnlySRV->Release();
            m_zBufferStencilReadOnlySRV = nullptr;
        }
    }

    bool RenderContext::ResourcesCreated() const
    {
        return !m_swappableRenderTargets.empty() && m_depthTarget && m_depthTargetMSAA && m_zBufferDepthReadOnlySRV && m_zBufferStencilReadOnlySRV;
    }

    bool RenderContext::Initialize()
    {
        // it may take an extra frame to create resources, don't re-create them if we've already started
        if (!m_swappableRenderTargets.empty())
        {
            return IsValid();
        }

        m_viewport.nWidth = m_config.m_width;
        m_viewport.nHeight = m_config.m_height;

        // attempt to create render targets
        if (CreateRenderTargets(m_viewport.nWidth, m_viewport.nHeight))
        {
            // attempt to create depth targets
            CreateDepthTargets(m_viewport.nWidth, m_viewport.nHeight);
        }

        // if we failed to create all resources then release them 
        if (!IsValid())
        {
            Release();
        }

        return IsValid();
    }

    bool RenderContext::SetActive(bool active)
    {
        if (active && !IsValid())
        {
            // we cannot activate an invalid context
            return false;
        }

        if (active != m_active)
        {
            m_active = active;

            SetActiveMainThread(active);

            // swap buffers and apply/revert render-thread specific settings
            gRenDev->m_pRT->EnqueueRenderCommand([this, active]()
            {
                SetActiveRenderThread(active);
            });

            // in single threaded mode (editor) we can check here for 
            // invalid context and revert to inactive state 
            if (active && !IsValid())
            {
                m_active = false;
                SetActiveMainThread(false);
            }
        }

        return IsValid();
    }

    void RenderContext::SetActiveMainThread(bool active)
    {
        if (active)
        {
            if (m_config.m_shadowsEnabled)
            {
                if (m_config.m_shadowsNumCascades > -1)
                {
                    m_iCVars["e_GsmLodsNum"].Swap(m_config.m_shadowsNumCascades);
                }
                if (m_config.m_shadowsGSMRange > 0.f)
                {
                    m_fCVars["e_GsmRange"].Swap(m_config.m_shadowsGSMRange);
                }
                if (m_config.m_shadowsGSMRangeStep > 0.f)
                {
                    m_fCVars["e_GsmRangeStep"].Swap(m_config.m_shadowsGSMRangeStep);
                }
            }

            m_iCVars["e_Dissolve"].Disable();
            m_iCVars["e_CoverageBuffer"].Disable();
            m_iCVars["e_StatObjBufferRenderTasks"].Disable();

            // set renderer width/height
            gRenDev->SetWidth(m_viewport.nWidth);
            gRenDev->SetHeight(m_viewport.nHeight);
        }
        else
        {
            if (m_config.m_shadowsEnabled)
            {
                m_iCVars["e_GsmLodsNum"].Restore();
                m_fCVars["e_GsmRange"].Restore();
                m_fCVars["e_GsmRangeStep"].Restore();
            }

            m_iCVars["e_Dissolve"].Restore();
            m_iCVars["e_CoverageBuffer"].Restore();
            m_iCVars["e_StatObjBufferRenderTasks"].Restore();

            if (!IsValid())
            {
                // if this context is invalid then restore the renderer
                // width/height to a known good state
                gRenDev->SetWidth(m_previousSettings.m_viewport.nWidth);
                gRenDev->SetHeight(m_previousSettings.m_viewport.nHeight);
            }
        }
    }

    void RenderContext::SetActiveRenderThread(bool active)
    {
        AZ_Assert(IsValid(), "RenderContext is not valid and cannot be activated");
        if (active && !ResourcesCreated())
        {
            // initialization can fail for various reasons including running out of memory 
            if (!Initialize())
            {
                return;
            }
        }

        AZ_Assert(m_viewport.nWidth != 0 && m_viewport.nHeight != 0, "Invalid RenderContext viewport size, width and height must be greater than zero.");

        for (SwappableRenderTarget& swappableRenderTarget : m_swappableRenderTargets)
        {
            swappableRenderTarget.Swap();
        }

        // handle special cases (aliases)
        CTexture::s_ptexCurrSceneTarget = CTexture::s_ptexSceneTarget;
        CTexture::s_ptexCurrentSceneDiffuseAccMap = CTexture::s_ptexSceneDiffuseAccMap;

        // swap viewports and depth surfaces 
        if (active)
        {
            // backup/swap viewport
            m_previousSettings.m_viewport = gRenDev->m_MainViewport;
            gRenDev->m_MainViewport = m_viewport;

            // only apply width/height in multithreaded mode because engine thread
            // does this too
            if (gRenDev->CV_r_multithreaded)
            {
                gRenDev->SetWidth(m_viewport.nWidth);
                gRenDev->SetHeight(m_viewport.nHeight);
            }

            // backup/swap depth targets
            m_previousSettings.m_depthOrig = gcpRendD3D->m_DepthBufferOrig;
            m_previousSettings.m_depthMSAA = gcpRendD3D->m_DepthBufferOrigMSAA;
            gcpRendD3D->m_DepthBufferOrig = *m_depthTarget;
            gcpRendD3D->m_DepthBufferOrigMSAA = *m_depthTargetMSAA;

            m_iCVars["r_FinalOutputAlpha"].Swap(static_cast<int>(m_config.m_alphaMode));
            m_iCVars["r_FinalOutputsRGB"].Swap(m_config.m_sRGBWrite ? 1 : 0);

            // Not supported yet because they pollute the main render pass - lots of shared occlusion code
            m_iCVars["r_Flares"].Swap(0);

            // Not supported yet because the occlusion queries used for visibilty are async and can cause flickering.
            m_iCVars["r_sunshafts"].Swap(0);

            // rtt cvar overrides - useful for debugging
            static ICVar* rttAA = gEnv->pConsole->GetCVar("rtt_aa");
            if (rttAA && rttAA->GetIVal() > -1)
            {
                m_iCVars["r_AntialiasingMode"].Swap(rttAA->GetIVal());
            }
            else
            {
                m_iCVars["r_AntialiasingMode"].Swap(m_config.m_aaMode);
            }

            static ICVar* rttMotionBlur = gEnv->pConsole->GetCVar("rtt_motionblur");
            if (rttMotionBlur && rttMotionBlur->GetIVal() > -1)
            {
                m_iCVars["r_MotionBlur"].Swap(rttMotionBlur->GetIVal());
            }
            else
            {
                m_iCVars["r_MotionBlur"].Swap(m_config.m_motionBlurEnabled ? 2 : 0);
            }

            static ICVar* rttDepthOfField = gEnv->pConsole->GetCVar("rtt_dof");
            if (rttDepthOfField && rttDepthOfField->GetIVal() > -1)
            {
                m_iCVars["r_DepthOfField"].Swap(rttDepthOfField->GetIVal());
            }
            else
            {
                m_iCVars["r_DepthOfField"].Swap(m_config.m_depthOfFieldEnabled ? DOF_DEFAULT_VAL : 0);
            }
        }
        else
        {
            gRenDev->m_MainViewport = m_previousSettings.m_viewport;
            gcpRendD3D->m_DepthBufferOrig = m_previousSettings.m_depthOrig;
            gcpRendD3D->m_DepthBufferOrigMSAA = m_previousSettings.m_depthMSAA;

            // handle aliases
            CTexture::s_ptexCurLumTexture = nullptr;

            // restore settings
            m_iCVars["r_FinalOutputAlpha"].Restore();
            m_iCVars["r_FinalOutputsRGB"].Restore();
            m_iCVars["r_Flares"].Restore();
            m_iCVars["r_sunshafts"].Restore();
            m_iCVars["r_MotionBlur"].Restore();
            m_iCVars["r_DepthOfField"].Restore();
            m_iCVars["r_AntialiasingMode"].Restore();
        }

        // swap resource views
        if (m_zBufferDepthReadOnlySRV)
        {
            AZStd::swap(gcpRendD3D->m_pZBufferDepthReadOnlySRV, m_zBufferDepthReadOnlySRV);
        }

        if (m_zBufferStencilReadOnlySRV)
        {
            AZStd::swap(gcpRendD3D->m_pZBufferStencilReadOnlySRV, m_zBufferStencilReadOnlySRV);
        }
    }

    void RenderContext::SetConfig(const RenderContextConfig& config)
    {
        uint32_t width = config.m_width;
        uint32_t height = config.m_height;

        // ensure the width/height used is supported by the current hardware
        const int maxTextureSize = gcpRendD3D->GetMaxTextureSize();
        if (maxTextureSize > 0)
        {
            width = min(width, static_cast<uint32_t>(maxTextureSize));
            height = min(height, static_cast<uint32_t>(maxTextureSize));
        }

        // assume valid until we try to resize/create resources and fail
        m_errorState = ErrorState::OK;
        if (ResourcesCreated() && (m_config.m_width != width || m_config.m_height != height))
        {
            ResizeRenderTargets(width, height);
        }

        m_config = config;
        m_config.m_width = width;
        m_config.m_height = height;

        m_viewport.nWidth = width;
        m_viewport.nHeight = height;
    }

    bool RenderContext::CreateRenderTargets(AZ::u32 width, AZ::u32 height)
    {
        // when all engine textures are managed by CTextureManager 
        // this can be replaced
        struct swappableRenderTarget
        {
            CTexture** texture;
            uint32_t scale;
        };

        // NOTE: some of these textures are swapped because they are used by
        // the render to texture pass, and others are swapped to avoid 
        // overwriting temporal textures used in the main pass  
        const swappableRenderTarget targets[] = {
            { &CTexture::s_ptexSceneDiffuse,                1 },
            { &CTexture::s_ptexSceneDiffuseAccMap,          1 }, // ignore the s_ptexCurrentSceneDiffuseAccMap alias
            { &CTexture::s_ptexSceneNormalsBent,            1 },
            { &CTexture::s_ptexSceneNormalsMap ,            1},
            { &CTexture::s_ptexSceneSpecular,               1 },
            { &CTexture::s_ptexSceneSpecularAccMap,         1 },
            { &CTexture::s_ptexSceneTarget,                 1 },
            { &CTexture::s_ptexSceneTargetR11G11B10F[0],    1 },
            { &CTexture::s_ptexSceneTargetR11G11B10F[1],    1 },
            { &CTexture::s_ptexShadowMask,                  1 },
            { &CTexture::s_ptexHDRTarget,                   1 },
            { &CTexture::s_ptexHDRTargetPrev,               1 },
            { &CTexture::s_ptexSceneCoCHistory[0],          1 },
            { &CTexture::s_ptexSceneCoCHistory[1],          1 },

            // motion blur
            { &CTexture::s_ptexVelocity,                    1 },
            { &CTexture::s_ptexVelocityObjects[0],          1 },

            { &CTexture::s_ptexStereoL,                     1 },
            { &CTexture::s_ptexStereoR,                     1 },

            { &CTexture::s_ptexBackBuffer,                  1 },
            { &CTexture::s_ptexPrevBackBuffer[0][0],        1 },
            { &CTexture::s_ptexPrevBackBuffer[1][0],        1 },

            // HDR Targets scaled down
            { &CTexture::s_ptexHDRTargetScaled[0], 2 },
            { &CTexture::s_ptexHDRTargetScaled[1], 4 },
            { &CTexture::s_ptexHDRTargetScaled[2], 8 },
            { &CTexture::s_ptexHDRTargetScaled[3], 8 },

            { &CTexture::s_ptexBackBufferScaled[0], 2 },
            { &CTexture::s_ptexBackBufferScaled[1], 4 },
            { &CTexture::s_ptexBackBufferScaled[2], 8 },

            { &CTexture::s_ptexHDRDofLayers[0], 2 },
            { &CTexture::s_ptexHDRDofLayers[1], 2 },

            { &CTexture::s_ptexHDRTargetScaledTmp[0], 2 },
            { &CTexture::s_ptexHDRTargetScaledTmp[1], 4 },
            //{ &CTexture::s_ptexHDRTargetScaledTmp[2], ? }, unused
            { &CTexture::s_ptexHDRTargetScaledTmp[3], 8 },

            { &CTexture::s_ptexHDRTargetScaledTempRT[0], 2 },
            { &CTexture::s_ptexHDRTargetScaledTempRT[1], 4 },
            { &CTexture::s_ptexHDRTargetScaledTempRT[2], 8 },
            { &CTexture::s_ptexHDRTargetScaledTempRT[3], 8 },

            { &CTexture::s_ptexHDRTempBloom[0], 4 },
            { &CTexture::s_ptexHDRTempBloom[1], 4 },
            { &CTexture::s_ptexHDRFinalBloom,   4 },

            // SPostEffectsUtils used for things like lense flares
            { &CTexture::s_ptexBackBufferScaledTemp[0], 2 },
            { &CTexture::s_ptexBackBufferScaledTemp[1], 4 },

            // Water volume reflections 
            { &CTexture::s_ptexWaterVolumeRefl[0], 2 },
            { &CTexture::s_ptexWaterVolumeRefl[1], 2 },

            // ZTargets
            { &CTexture::s_ptexZTarget,        1 },
            { &CTexture::s_ptexZTargetScaled,  2 },
            { &CTexture::s_ptexZTargetScaled2, 4 }
        };

        for (const swappableRenderTarget& target : targets)
        {
            m_swappableRenderTargets.push_back({ target.texture });
            m_swappableRenderTargets.back().CreateRenderTargetCopy(width, height, target.scale, m_renderContextId);
        }

        // Depth of field COC  
        uint32_t scale = 1;
        for (int i = 0; i < MIN_DOF_COC_K; i++)
        {
            m_swappableRenderTargets.push_back({ &CTexture::s_ptexSceneCoC[i] });
            scale = 2 * (i + 1);
            m_swappableRenderTargets.back().CreateRenderTargetCopy(width, height, scale, m_renderContextId);
        }

        scale = 1;

        // Flare occlusion queries
        for (int i = 0; i < MAX_OCCLUSION_READBACK_TEXTURES; i++)
        {
            m_swappableRenderTargets.push_back({ &CTexture::s_ptexFlaresOcclusionRing[i] });
            m_swappableRenderTargets.back().CreateRenderTargetCopy(m_renderContextId);
        }

        m_swappableRenderTargets.push_back({ &CTexture::s_ptexFlaresGather });
        m_swappableRenderTargets.back().CreateRenderTargetCopy(m_renderContextId);

        // HDR adapted luminance render targets
        for (int i = 0; i < 8; ++i)
        {
            m_swappableRenderTargets.push_back({ &CTexture::s_ptexHDRAdaptedLuminanceCur[i] });
            m_swappableRenderTargets.back().CreateRenderTargetCopy(1, 1, scale, m_renderContextId);
        }
        
        for (int i = 0; i < NUM_HDR_TONEMAP_TEXTURES; i++)
        {
            int size = 1 << (2 * i);
            m_swappableRenderTargets.push_back({ &CTexture::s_ptexHDRToneMaps[i] });
            m_swappableRenderTargets.back().CreateRenderTargetCopy(size, size, scale, m_renderContextId);
        }

        for (int i = 0; i < MAX_GPU_NUM; ++i)
        {
            m_swappableRenderTargets.push_back({ &CTexture::s_ptexHDRMeasuredLuminance[i] });
            m_swappableRenderTargets.back().CreateRenderTargetCopy(1,1, scale, m_renderContextId);
        }

        // velocity tiles
        m_swappableRenderTargets.push_back({ &CTexture::s_ptexVelocityTiles[0] });
        m_swappableRenderTargets.back().CreateRenderTargetCopy(20, height, scale, m_renderContextId);

        m_swappableRenderTargets.push_back({ &CTexture::s_ptexVelocityTiles[1] });
        m_swappableRenderTargets.back().CreateRenderTargetCopy(20, 20, scale, m_renderContextId);

        m_swappableRenderTargets.push_back({ &CTexture::s_ptexVelocityTiles[2] });
        m_swappableRenderTargets.back().CreateRenderTargetCopy(20, 20, scale, m_renderContextId);

        if (!RenderTargetsAreValid())
        {
            m_errorState = ErrorState::ResourceCreationFailed;
            AZ_Warning("RTTContext", false, "Failed to create render to texture textures for context %d", m_renderContextId);
        }

        return IsValid();
    }

    bool RenderContext::RenderTargetsAreValid() const
    {
        for (const auto& swappableRenderTarget : m_swappableRenderTargets)
        {
            if (!swappableRenderTarget.IsValid())
            {
                return false;
            }
        }

        return true;
    }

    bool RenderContext::DepthTargetsAreValid() const
    {
        return m_depthTarget && m_depthTarget->pTarget && m_depthTargetMSAA && m_depthTargetMSAA->pTarget;
    }

    bool RenderContext::ResizeRenderTargets(AZ::u32 width, AZ::u32 height)
    {
        for (auto& swappableRenderTarget : m_swappableRenderTargets)
        {
            swappableRenderTarget.Resize(width, height);
        }

        if (!RenderTargetsAreValid())
        {
            m_errorState = ErrorState::ResourceCreationFailed;
            AZ_Warning("RTTContext", false, "Failed to resize render to texture textures for context %d", m_renderContextId);
        }
        else
        {
            CreateDepthTargets(width, height);
        }

        // we might not have enough GPU memory to activate this context after a resize
        if (!IsValid())
        {
            Release();
        }

        return IsValid();
    }

    bool RenderContext::CreateDepthTargets(AZ::u32 width, AZ::u32 height)
    {
        // render resources must be created on the render thread
        // m_depthTarget and m_depthTargetMSAA may point to the same depth surface/target because
        // r_msaa is off by default.
        m_depthTarget = gcpRendD3D->FX_GetDepthSurface(width, height, false, true);
        m_depthTargetMSAA = gcpRendD3D->FX_GetDepthSurface(width, height, true, true);

        // create the depth resource view
        if (gcpRendD3D->m_pZBufferDepthReadOnlySRV && m_depthTarget && m_depthTarget->pTarget && m_depthTargetMSAA && m_depthTargetMSAA->pTarget)
        {
            D3D11_TEXTURE2D_DESC descDepthStencil;
            m_depthTargetMSAA->pTarget->GetDesc(&descDepthStencil);

            D3DFormat nDepthStencilFormatTypeless = descDepthStencil.Format;
            if (!CTexture::IsDeviceFormatTypeless(nDepthStencilFormatTypeless))
            {
                nDepthStencilFormatTypeless = CTexture::ConvertToTypelessFmt(nDepthStencilFormatTypeless);
            }

            D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
            ZeroStruct(SRVDesc);
            SRVDesc.Format = CTexture::ConvertToShaderResourceFmt(nDepthStencilFormatTypeless);
            SRVDesc.ViewDimension = (descDepthStencil.SampleDesc.Count > 1) ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
            SRVDesc.Texture2D.MipLevels = 1;
            ID3D11ShaderResourceView* pNewResourceView = NULL;
            HRESULT hr = gcpRendD3D->GetDevice().CreateShaderResourceView(m_depthTargetMSAA->pTarget, &SRVDesc, &pNewResourceView);
            if (SUCCEEDED(hr))
            {
                hr = gcpRendD3D->GetDevice().CreateShaderResourceView(m_depthTargetMSAA->pTarget, &SRVDesc, &m_zBufferDepthReadOnlySRV);
                AZ_Warning("RTTContext", SUCCEEDED(hr), "Failed to create resource shader view for RTT depth target %d", m_renderContextId);
            }
            else
            {
                AZ_Printf("RTTContext", "Failed to create resource shader view for RTT depth target %d", m_renderContextId);
            }
        }

        // create our stencil buffer resource view if needed
        if (gcpRendD3D->m_pZBufferStencilReadOnlySRV && m_depthTargetMSAA)
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
            gcpRendD3D->m_pZBufferStencilReadOnlySRV->GetDesc(&SRVDesc);
            HRESULT hr = gcpRendD3D->GetDevice().CreateShaderResourceView(m_depthTargetMSAA->pTarget, &SRVDesc, &m_zBufferStencilReadOnlySRV);
            AZ_Warning("RTTContext", SUCCEEDED(hr), "Failed to create stencil resource shader view for RTT depth target %d", m_renderContextId);
        }

        if (!DepthTargetsAreValid())
        {
            m_errorState = ErrorState::ResourceCreationFailed;
            AZ_Warning("RTTContext", false, "Failed to create render to texture depth targets for context %d", m_renderContextId);
        }

        return IsValid();
    }
}

#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
