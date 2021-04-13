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
#include "DriverD3D.h"
#include "D3DPostProcess.h"
#include "I3DEngine.h"
#include "../Common/Textures/TextureManager.h"
#include "../Common/ReverseDepth.h"
#include "CryCommon/FrameProfiler.h"


enum class ToneMapOperators : int
{
    FilmicCurveUC2 = 0,
    Linear = 1,
    Exponential = 2,
    Reinhard = 3,
    FilmicCurveALU = 4
};



enum class Exposuretype : int
{
    Auto = 0,  //Any other variations of AUTO will go here.
    Manual = 1
};


//  render targets info - first gather list of hdr targets, sort by size and create after
struct SRenderTargetInfo
{
    SRenderTargetInfo()
        : nWidth(0)
        , nHeight(0)
        , cClearColor(Clr_Empty)
        , Format(eTF_Unknown)
        , nFlags(0)
        , lplpStorage(0)
        , nPitch(0)
        , fPriority(0.0f)
        , nCustomID(0)
    {};

    uint32              nWidth;
    uint32              nHeight;
    ColorF              cClearColor;
    ETEX_Format         Format;
    uint32              nFlags;
    CTexture** lplpStorage;
    char                szName[64];
    uint32              nPitch;
    float               fPriority;
    int32               nCustomID;
};

struct RenderTargetSizeSort
{
    bool operator()(const SRenderTargetInfo& drtStart, const SRenderTargetInfo& drtEnd) { return (drtStart.nPitch * drtStart.fPriority) > (drtEnd.nPitch * drtEnd.fPriority); }
};

class CHDRPostProcess
{
public:

    CHDRPostProcess()
    {
        nTexStateLinear = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
        nTexStateLinearWrap = CTexture::GetTexState(STexState(FILTER_LINEAR, false));
        nTexStatePoint = CTexture::GetTexState(STexState(FILTER_POINT, true));
        nTexStatePointWrap = CTexture::GetTexState(STexState(FILTER_POINT, false));
        m_bHiQuality = false;
        m_shHDR = 0;
        m_shHDRDolbyMetadataPass0 = nullptr;
        m_shHDRDolbyMetadataPass1 = nullptr;
    }

    ~CHDRPostProcess()
    {
    }

    void ScreenShot();

    void SetShaderParams();

    // Tone mapping steps
    void HalfResDownsampleHDRTarget();
    void QuarterResDownsampleHDRTarget();
    void SceneDownsampleUsingCompute(); // for mobile, uses CS to 1/2 & 1/4 res downsample in 1 pass
    void MeasureLuminance();
    void EyeAdaptation();
    void MeasureLumEyeAdaptationUsingCompute(); // uses CS to collapse measure lum and eye adap passes
    void BloomGeneration();
    void ProcessLensOptics();
    void ToneMapping();
    void ToneMappingDebug();
    void CalculateDolbyDynamicMetadata(CTexture* pSunShaftsRT);
    void DrawDebugViews();
    void SetExposureTypeShaderFlags();
    CCryNameTSCRC GetTonemapTechnique() const;
    
    void EncodeDolbyVision(CTexture* source);

    void Begin();
    void Render();
    void End();

    void AddRenderTarget(uint32 nWidth, uint32 nHeight, const ColorF& cClear, ETEX_Format Format, float fPriority, const char* szName, CTexture** pStorage, uint32 nFlags = 0, int32 nCustomID = -1, bool bDynamicTex = 0);
    bool CreateRenderTargetList();
    void ClearRenderTargetList()
    {
        m_pRenderTargets.clear();
    }

    static CHDRPostProcess* GetInstance()
    {
        static CHDRPostProcess pInstance;
        return &pInstance;
    }
private:
    std::vector< SRenderTargetInfo > m_pRenderTargets;
    CShader* m_shHDR;
    CShader* m_shHDRDolbyMetadataPass0;
    CShader* m_shHDRDolbyMetadataPass1;
    WrappedDX11Buffer m_bufDolbyMetadataMacroReductionOutput;
    WrappedDX11Buffer m_bufDolbyMetadataMinMaxMid;
    int32 nTexStateLinear;
    int32 nTexStateLinearWrap;
    int32 nTexStatePoint;
    int32 nTexStatePointWrap;
    bool m_bHiQuality;
};


void CHDRPostProcess::AddRenderTarget(uint32 nWidth, uint32 nHeight, const ColorF& cClear, ETEX_Format Format, float fPriority, const char* szName, CTexture** pStorage, uint32 nFlags, int32 nCustomID, [[maybe_unused]] bool bDynamicTex)
{
    SRenderTargetInfo drt;
    drt.nWidth          = nWidth;
    drt.nHeight         = nHeight;
    drt.cClearColor     = cClear;
    drt.nFlags          = FT_USAGE_RENDERTARGET | FT_DONT_STREAM | nFlags;
    drt.Format           = Format;
    drt.fPriority        = fPriority;
    drt.lplpStorage      = pStorage;
    drt.nCustomID        = nCustomID;
    drt.nPitch           = nWidth * CTexture::BytesPerBlock(Format);
    cry_strcpy(drt.szName, szName);
    m_pRenderTargets.push_back(drt);
}


bool CHDRPostProcess::CreateRenderTargetList()
{
    std::sort(m_pRenderTargets.begin(), m_pRenderTargets.end(), RenderTargetSizeSort());

    for (uint32 t = 0; t < m_pRenderTargets.size(); ++t)
    {
        SRenderTargetInfo& drt = m_pRenderTargets[t];
        CTexture* pTex = (*drt.lplpStorage);
        if (!CTexture::IsTextureExist(pTex))
        {
            pTex = CTexture::CreateRenderTarget(drt.szName, drt.nWidth, drt.nHeight, drt.cClearColor, eTT_2D, drt.nFlags, drt.Format, drt.nCustomID);

            if (pTex)
            {
                // Following will mess up don't care resolve/restore actions since Fill() sets textures to be cleared on next draw
#if !defined(CRY_USE_METAL) && !defined(OPENGL_ES)
                pTex->Clear();
#endif
                (*drt.lplpStorage) = pTex;
            }
        }
        else
        {
            pTex->SetFlags(drt.nFlags);
            pTex->SetWidth(drt.nWidth);
            pTex->SetHeight(drt.nHeight);
            pTex->CreateRenderTarget(drt.Format, drt.cClearColor);
        }
    }

    m_pRenderTargets.clear();

    return S_OK;
}


void CTexture::GenerateHDRMaps()
{
    int i;
    char szName[256];

    CD3D9Renderer* r = gcpRendD3D;
    CHDRPostProcess* pHDRPostProcess = CHDRPostProcess::GetInstance();

    r->m_dwHDRCropWidth = r->GetWidth();
    r->m_dwHDRCropHeight = r->GetHeight();

    pHDRPostProcess->ClearRenderTargetList();

    ETEX_Format nHDRFormat = eTF_R16G16B16A16F; // note: for main rendertarget R11G11B10 precision/range (even with rescaling) not enough for darks vs good blooming quality

    ETEX_Format nHDRReducedFormat = r->UseHalfFloatRenderTargets() ? eTF_R11G11B10F : eTF_R10G10B10A2;

    uint32 nHDRTargetFlags = FT_DONT_RELEASE | (CRenderer::CV_r_msaa ? FT_USAGE_MSAA : 0);
    uint32 nHDRTargetFlagsUAV = nHDRTargetFlags | (CRenderer::CV_r_msaa ? 0 : FT_USAGE_UNORDERED_ACCESS);  // UAV required for tiled deferred shading

    // GMEM render path uses CTexture::s_ptexSceneSpecularAccMap as the HDR Target
    // It gets set in CDeferredShading::CreateDeferredMaps()
    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        pHDRPostProcess->AddRenderTarget(r->GetWidth(), r->GetHeight(), Clr_Unknown, r->UseHalfFloatRenderTargets() ? nHDRFormat : nHDRReducedFormat, 1.0f, "$HDRTarget", &s_ptexHDRTarget, nHDRTargetFlagsUAV);
    }

    pHDRPostProcess->AddRenderTarget(r->GetWidth(), r->GetHeight(), Clr_Unknown, nHDRReducedFormat, 1.0f, "$HDRTargetPrev", &s_ptexHDRTargetPrev);

    pHDRPostProcess->AddRenderTarget(r->GetWidth(), r->GetHeight(), Clr_Unknown, nHDRFormat, 1.0f, "$FurLightAcc", &s_ptexFurLightAcc, FT_DONT_RELEASE);
    pHDRPostProcess->AddRenderTarget(r->GetWidth(), r->GetHeight(), Clr_Unknown, eTF_R32G32B32A32F, 1.0f, "$FurPrepass", &s_ptexFurPrepass, FT_DONT_RELEASE);

    // Scaled versions of the HDR scene texture
    uint32 nWeight = (r->m_dwHDRCropWidth >> 1);
    uint32 nHeight = (r->m_dwHDRCropHeight >> 1);

    pHDRPostProcess->AddRenderTarget(nWeight, nHeight, Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaled0", &s_ptexHDRTargetScaled[0], FT_DONT_RELEASE);

    pHDRPostProcess->AddRenderTarget(nWeight, nHeight, Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaledTmp0", &s_ptexHDRTargetScaledTmp[0], FT_DONT_RELEASE);
    pHDRPostProcess->AddRenderTarget(nWeight, nHeight, Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaledTempRT0", &s_ptexHDRTargetScaledTempRT[0], FT_DONT_RELEASE);

    nWeight = (r->m_dwHDRCropWidth >> 2);
    nHeight = (r->m_dwHDRCropHeight >> 2);
    pHDRPostProcess->AddRenderTarget(nWeight, nHeight, Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaled1", &s_ptexHDRTargetScaled[1], FT_DONT_RELEASE);
    pHDRPostProcess->AddRenderTarget(nWeight, nHeight, Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaledTmp1", &s_ptexHDRTargetScaledTmp[1], FT_DONT_RELEASE);

    pHDRPostProcess->AddRenderTarget(nWeight, nHeight, Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaledTempRT1", &s_ptexHDRTargetScaledTempRT[1], 0);

    pHDRPostProcess->AddRenderTarget(nWeight, nHeight, Clr_Unknown, eTF_R11G11B10F, 0.9f, "$HDRTempBloom0", &s_ptexHDRTempBloom[0], FT_DONT_RELEASE);
    pHDRPostProcess->AddRenderTarget(nWeight, nHeight, Clr_Unknown, eTF_R11G11B10F, 0.9f, "$HDRTempBloom1", &s_ptexHDRTempBloom[1], FT_DONT_RELEASE);
    pHDRPostProcess->AddRenderTarget(nWeight, nHeight, Clr_Unknown, eTF_R11G11B10F, 0.9f, "$HDRFinalBloom", &s_ptexHDRFinalBloom, FT_DONT_RELEASE);

    pHDRPostProcess->AddRenderTarget((r->m_dwHDRCropWidth >> 3), (r->m_dwHDRCropHeight >> 3), Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaled2", &s_ptexHDRTargetScaled[2], FT_DONT_RELEASE);
    pHDRPostProcess->AddRenderTarget((r->m_dwHDRCropWidth >> 3), (r->m_dwHDRCropHeight >> 3), Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaledTempRT2", &s_ptexHDRTargetScaledTempRT[2], FT_DONT_RELEASE);

    pHDRPostProcess->AddRenderTarget((r->m_dwHDRCropWidth >> 4), (r->m_dwHDRCropHeight >> 4), Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaled3", &s_ptexHDRTargetScaled[3], FT_DONT_RELEASE);
    pHDRPostProcess->AddRenderTarget((r->m_dwHDRCropWidth >> 4), (r->m_dwHDRCropHeight >> 4), Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaledTmp3", &s_ptexHDRTargetScaledTmp[3], FT_DONT_RELEASE);
    pHDRPostProcess->AddRenderTarget((r->m_dwHDRCropWidth >> 4), (r->m_dwHDRCropHeight >> 4), Clr_Unknown, nHDRFormat, 0.9f, "$HDRTargetScaledTempRT3", &s_ptexHDRTargetScaledTempRT[3], FT_DONT_RELEASE);
    for (i = 0; i < 8; i++)
    {
        sprintf_s(szName, "$HDRAdaptedLuminanceCur_%d", i);
        pHDRPostProcess->AddRenderTarget(1, 1, Clr_Unknown, eTF_R16G16F, 0.1f, szName, &s_ptexHDRAdaptedLuminanceCur[i], FT_DONT_RELEASE);
    }

    pHDRPostProcess->AddRenderTarget(r->GetWidth(), r->GetHeight(), Clr_Unknown, eTF_R11G11B10F, 1.0f, "$SceneTargetR11G11B10F_0", &s_ptexSceneTargetR11G11B10F[0], nHDRTargetFlagsUAV);
    pHDRPostProcess->AddRenderTarget(r->GetWidth(), r->GetHeight(), Clr_Unknown, eTF_R11G11B10F, 1.0f, "$SceneTargetR11G11B10F_1", &s_ptexSceneTargetR11G11B10F[1], nHDRTargetFlags);

    pHDRPostProcess->AddRenderTarget(r->m_dwHDRCropWidth, r->m_dwHDRCropHeight, Clr_Unknown, eTF_R8G8B8A8, 0.1f, "$Velocity", &s_ptexVelocity, FT_DONT_RELEASE);
    pHDRPostProcess->AddRenderTarget(20, r->m_dwHDRCropHeight, Clr_Unknown, eTF_R8G8B8A8, 0.1f, "$VelocityTilesTmp0", &s_ptexVelocityTiles[0], FT_DONT_RELEASE);
    pHDRPostProcess->AddRenderTarget(20, 20, Clr_Unknown, eTF_R8G8B8A8, 0.1f, "$VelocityTilesTmp1", &s_ptexVelocityTiles[1], FT_DONT_RELEASE);
    pHDRPostProcess->AddRenderTarget(20, 20, Clr_Transparent, eTF_R8G8B8A8, 0.1f, "$VelocityTiles", &s_ptexVelocityTiles[2], FT_DONT_RELEASE);

    ETEX_Format velocityObjectRenderTargetFormat = eTF_R16G16F;
    pHDRPostProcess->AddRenderTarget(r->m_dwHDRCropWidth, r->m_dwHDRCropHeight, Clr_Transparent, velocityObjectRenderTargetFormat, 0.1f, "$VelocityObjects", &s_ptexVelocityObjects[0], FT_DONT_RELEASE);
    if (gRenDev->m_bDualStereoSupport)
    {
        pHDRPostProcess->AddRenderTarget(r->m_dwHDRCropWidth, r->m_dwHDRCropHeight, Clr_Unknown, velocityObjectRenderTargetFormat, 0.1f, "$VelocityObject_R", &s_ptexVelocityObjects[1], FT_DONT_RELEASE);
    }

    pHDRPostProcess->AddRenderTarget(r->GetWidth() >> 1, r->GetHeight() >> 1, Clr_Unknown, nHDRFormat, 0.9f, "$HDRDofLayerNear", &s_ptexHDRDofLayers[0], FT_DONT_RELEASE);
    pHDRPostProcess->AddRenderTarget(r->GetWidth() >> 1, r->GetHeight() >> 1, Clr_Unknown, nHDRFormat, 0.9f, "$HDRDofLayerFar", &s_ptexHDRDofLayers[1], FT_DONT_RELEASE);
#if METAL
    pHDRPostProcess->AddRenderTarget(r->GetWidth() >> 1, r->GetHeight() >> 1, Clr_Unknown, eTF_R16F, 1.0f, "$MinCoC_0_Temp", &s_ptexSceneCoCTemp, FT_DONT_RELEASE);
#else
    pHDRPostProcess->AddRenderTarget(r->GetWidth() >> 1, r->GetHeight() >> 1, Clr_Unknown, eTF_R16G16F, 1.0f, "$MinCoC_0_Temp", &s_ptexSceneCoCTemp, FT_DONT_RELEASE);
#endif

    pHDRPostProcess->AddRenderTarget(r->GetWidth(), r->GetHeight(), Clr_Unknown, eTF_R16G16F, 1.0, "$CoC_History0", &s_ptexSceneCoCHistory[0], FT_DONT_RELEASE);
    pHDRPostProcess->AddRenderTarget(r->GetWidth(), r->GetHeight(), Clr_Unknown, eTF_R16G16F, 1.0, "$CoC_History1", &s_ptexSceneCoCHistory[1], FT_DONT_RELEASE);

    for (i = 0; i < MIN_DOF_COC_K; i++)
    {
        sprintf_s(szName, "$MinCoC_%d", i);
#if METAL
        pHDRPostProcess->AddRenderTarget((r->m_dwHDRCropWidth >> 1) / (i + 1), (r->m_dwHDRCropHeight >> 1) / (i + 1), Clr_Unknown, eTF_R16F, 0.1f, szName, &s_ptexSceneCoC[i], FT_DONT_RELEASE, -1, true);
#else
        pHDRPostProcess->AddRenderTarget((r->m_dwHDRCropWidth >> 1) / (i + 1), (r->m_dwHDRCropHeight >> 1) / (i + 1), Clr_Unknown, eTF_R16G16F, 0.1f, szName, &s_ptexSceneCoC[i], FT_DONT_RELEASE, -1, true);
#endif
    }

    if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        /* CONFETTI: David Srour
         * Used during GMEM path for linear depth & stencil resolve.
         * See following link for RT format storage sizes on Apple Metal HW:
         * https://developer.apple.com/library/mac/documentation/Miscellaneous/Conceptual/MetalProgrammingGuide/MetalFeatureSetTables/MetalFeatureSetTables.html
         */
        ETEX_Format format = eTF_R16G16F;
#if defined(OPENGL_ES) // might be no fp rendering support
        if (!gcpRendD3D->UseHalfFloatRenderTargets())
        {
            format = eTF_R16G16U;
        }
#endif
        pHDRPostProcess->AddRenderTarget(r->m_dwHDRCropWidth, r->m_dwHDRCropHeight, Clr_Unknown, format, 0.1f,
            "$GmemStenLinDepth", &s_ptexGmemStenLinDepth, FT_DONT_RELEASE, -1, true);
    }


    // Luminance rt
    for (i = 0; i < NUM_HDR_TONEMAP_TEXTURES; i++)
    {
        int iSampleLen = 1 << (2 * i);
        sprintf_s(szName, "$HDRToneMap_%d", i);
        pHDRPostProcess->AddRenderTarget(iSampleLen, iSampleLen, Clr_Dark, eTF_R16G16F, 0.7f, szName, &s_ptexHDRToneMaps[i], FT_DONT_RELEASE);
    }
    s_ptexHDRMeasuredLuminanceDummy = CTexture::CreateTextureObject("$HDRMeasuredLum_Dummy", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM, eTF_R16G16F, TO_HDR_MEASURED_LUMINANCE);
    for (i = 0; i < MAX_GPU_NUM; ++i)
    {
        sprintf_s(szName, "$HDRMeasuredLum_%d", i);

        if (CRenderer::CV_r_EnableGMEMPostProcCS)
        {
            pHDRPostProcess->AddRenderTarget(1, 1, Clr_Unknown, eTF_R16G16F, 0.1f, szName, &s_ptexHDRMeasuredLuminance[i], FT_DONT_RELEASE | FT_DONT_STREAM);
        }
        else
        {
            s_ptexHDRMeasuredLuminance[i] = CTexture::Create2DTexture(szName, 1, 1, 0, FT_DONT_RELEASE | FT_DONT_STREAM, NULL, eTF_R16G16F, eTF_R16G16F);
        }
    }

    pHDRPostProcess->CreateRenderTargetList();

    // Create resources if necessary - todo: refactor all this shared render targets stuff, quite cumbersome atm...
    PostProcessUtils().Create();
}


void CTexture::DestroyHDRMaps()
{
    int i;

    SAFE_RELEASE(s_ptexHDRTarget);

    SAFE_RELEASE(s_ptexHDRTargetPrev);
    SAFE_RELEASE(s_ptexHDRTargetScaled[0]);
    SAFE_RELEASE(s_ptexHDRTargetScaled[1]);
    SAFE_RELEASE(s_ptexHDRTargetScaled[2]);
    SAFE_RELEASE(s_ptexHDRTargetScaled[3]);

    SAFE_RELEASE(s_ptexHDRTargetScaledTmp[0]);
    SAFE_RELEASE(s_ptexHDRTargetScaledTempRT[0]);

    SAFE_RELEASE(s_ptexHDRTargetScaledTmp[1]);
    SAFE_RELEASE(s_ptexHDRTargetScaledTmp[3]);

    SAFE_RELEASE(s_ptexHDRTargetScaledTempRT[1]);
    SAFE_RELEASE(s_ptexHDRTargetScaledTempRT[2]);
    SAFE_RELEASE(s_ptexHDRTargetScaledTempRT[3]);

    SAFE_RELEASE(s_ptexHDRTempBloom[0]);
    SAFE_RELEASE(s_ptexHDRTempBloom[1]);
    SAFE_RELEASE(s_ptexHDRFinalBloom);


    for (i = 0; i < 8; i++)
    {
        SAFE_RELEASE(s_ptexHDRAdaptedLuminanceCur[i]);
    }

    for (i = 0; i < NUM_HDR_TONEMAP_TEXTURES; i++)
    {
        SAFE_RELEASE(s_ptexHDRToneMaps[i]);
    }
    SAFE_RELEASE(s_ptexHDRMeasuredLuminanceDummy);
    for (i = 0; i < MAX_GPU_NUM; ++i)
    {
        SAFE_RELEASE(s_ptexHDRMeasuredLuminance[i]);
    }

    CTexture::s_ptexCurLumTexture = NULL;

    SAFE_RELEASE(s_ptexVelocity);
    SAFE_RELEASE(s_ptexVelocityTiles[0]);
    SAFE_RELEASE(s_ptexVelocityTiles[1]);
    SAFE_RELEASE(s_ptexVelocityTiles[2]);
    SAFE_RELEASE(s_ptexVelocityObjects[0]);
    SAFE_RELEASE(s_ptexVelocityObjects[1]);

    SAFE_RELEASE(s_ptexHDRDofLayers[0]);
    SAFE_RELEASE(s_ptexHDRDofLayers[1]);
    SAFE_RELEASE(s_ptexSceneCoCTemp);
    for (i = 0; i < MIN_DOF_COC_K; i++)
    {
        SAFE_RELEASE(s_ptexSceneCoC[i]);
    }
}


// deprecated
void DrawQuad3D(float s0, float t0, float s1, float t1)
{
    const float fZ = 0.5f;
    const float fX0 = -1.0f, fX1 = 1.0f;
    const float fY0 = 1.0f, fY1 = -1.0f;

    gcpRendD3D->DrawQuad3D(Vec3(fX0, fY1, fZ), Vec3(fX1, fY1, fZ), Vec3(fX1, fY0, fZ), Vec3(fX0, fY0, fZ), Col_White, s0, t0, s1, t1);
}


// deprecated
void DrawFullScreenQuadTR(float xpos, float ypos, float w, float h)
{
    TempDynVB<SVF_P3F_C4B_T2F> vb(gcpRendD3D);
    vb.Allocate(4);
    SVF_P3F_C4B_T2F* vQuad = vb.Lock();

    const DWORD col = ~0;
    const float s0 = 0;
    const float s1 = 1;
    const float t0 = 1;
    const float t1 = 0;

    // Define the quad
    vQuad[0].xyz = Vec3(xpos, ypos, 1.0f);
    vQuad[0].color.dcolor = col;
    vQuad[0].st = Vec2(s0, 1.0f - t0);

    vQuad[1].xyz = Vec3(xpos + w, ypos, 1.0f);
    vQuad[1].color.dcolor = col;
    vQuad[1].st = Vec2(s1, 1.0f - t0);

    vQuad[3].xyz = Vec3(xpos + w, ypos + h, 1.0f);
    vQuad[3].color.dcolor = col;
    vQuad[3].st = Vec2(s1, 1.0f - t1);

    vQuad[2].xyz = Vec3(xpos, ypos + h, 1.0f);
    vQuad[2].color.dcolor = col;
    vQuad[2].st = Vec2(s0, 1.0f - t1);

    vb.Unlock();
    vb.Bind(0);
    vb.Release();

    gcpRendD3D->FX_Commit();

    if (!FAILED(gcpRendD3D->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
    {
        gcpRendD3D->FX_DrawPrimitive(eptTriangleStrip, 0, 4);
    }
}

#define NV_CACHE_OPTS_ENABLED TRUE


// deprecated
bool DrawFullScreenQuad(float fLeftU, float fTopV, float fRightU, float fBottomV, bool bClampToScreenRes)
{
    CD3D9Renderer* rd = gcpRendD3D;

    rd->FX_Commit();

    // Acquire render target width and height
    int nWidth = rd->m_NewViewport.nWidth;
    int nHeight = rd->m_NewViewport.nHeight;

    // Ensure that we're directly mapping texels to pixels by offset by 0.5
    // For more info see the doc page titled "Directly Mapping Texels to Pixels"
    if (bClampToScreenRes)
    {
        nWidth = min(nWidth, rd->GetWidth());
        nHeight = min(nHeight, rd->GetHeight());
    }

    float fWidth5 = static_cast<float>(nWidth);
    float fHeight5 = static_cast<float>(nHeight);
    fWidth5 = fWidth5 - 0.5f;
    fHeight5 = fHeight5 - 0.5f;

    // Draw the quad
    TempDynVB<SVF_TP3F_C4B_T2F> vb(gcpRendD3D);
    vb.Allocate(4);
    SVF_TP3F_C4B_T2F* Verts = vb.Lock();
    {
        fTopV = 1 - fTopV;
        fBottomV = 1 - fBottomV;
        Verts[0].pos = Vec4(-0.5f, -0.5f, 0.0f, 1.0f);
        Verts[0].color.dcolor = ~0;
        Verts[0].st = Vec2(fLeftU, fTopV);

        Verts[1].pos = Vec4(fWidth5, -0.5f, 0.0f, 1.0f);
        Verts[1].color.dcolor = ~0;
        Verts[1].st = Vec2(fRightU, fTopV);

        Verts[2].pos = Vec4(-0.5f, fHeight5, 0.0f, 1.0f);
        Verts[2].color.dcolor = ~0;
        Verts[2].st = Vec2(fLeftU, fBottomV);

        Verts[3].pos = Vec4(fWidth5, fHeight5, 0.0f, 1.0f);
        Verts[3].color.dcolor = ~0;
        Verts[3].st = Vec2(fRightU, fBottomV);

        vb.Unlock();
        vb.Bind(0);
        vb.Release();

        rd->FX_SetState(GS_NODEPTHTEST);
        if (!FAILED(rd->FX_SetVertexDeclaration(0, eVF_TP3F_C4B_T2F)))
        {
            rd->FX_DrawPrimitive(eptTriangleStrip, 0, 4);
        }
    }

    return true;
}


// deprecated
bool DrawFullScreenQuad(CoordRect c, bool bClampToScreenRes)
{
    return DrawFullScreenQuad(c.fLeftU, c.fTopV, c.fRightU, c.fBottomV, bClampToScreenRes);
}


void GetSampleOffsets_DownScale4x4(uint32 nWidth, uint32 nHeight, Vec4 avSampleOffsets[])
{
    float tU = 1.0f / static_cast<float>(nWidth);
    float tV = 1.0f / static_cast<float>(nHeight);

    // Sample from the 16 surrounding points. Since the center point will be in
    // the exact center of 16 texels, a 0.5f offset is needed to specify a texel
    // center.
    int index = 0;
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            avSampleOffsets[index].x = (x - 1.5f) * tU;
            avSampleOffsets[index].y = (y - 1.5f) * tV;
            avSampleOffsets[index].z = 0;
            avSampleOffsets[index].w = 1;

            index++;
        }
    }
}


void GetSampleOffsets_DownScale4x4Bilinear(uint32 nWidth, uint32 nHeight, Vec4 avSampleOffsets[])
{
    float tU = 1.0f / static_cast<float>(nWidth);
    float tV = 1.0f / static_cast<float>(nHeight);

    // Sample from the 16 surrounding points.  Since bilinear filtering is being used, specific the coordinate
    // exactly halfway between the current texel center (k-1.5) and the neighboring texel center (k-0.5)

    int index = 0;
    for (int y = 0; y < 4; y += 2)
    {
        for (int x = 0; x < 4; x += 2, index++)
        {
            avSampleOffsets[index].x = (x - 1.f) * tU;
            avSampleOffsets[index].y = (y - 1.f) * tV;
            avSampleOffsets[index].z = 0;
            avSampleOffsets[index].w = 1;
        }
    }
}


void GetSampleOffsets_DownScale2x2(uint32 nWidth, uint32 nHeight, Vec4 avSampleOffsets[])
{
    float tU = 1.0f / static_cast<float>(nWidth);
    float tV = 1.0f / static_cast<float>(nHeight);

    // Sample from the 4 surrounding points. Since the center point will be in
    // the exact center of 4 texels, a 0.5f offset is needed to specify a texel
    // center.
    int index = 0;
    for (int y = 0; y < 2; y++)
    {
        for (int x = 0; x < 2; x++)
        {
            avSampleOffsets[index].x = (x - 0.5f) * tU;
            avSampleOffsets[index].y = (y - 0.5f) * tV;
            avSampleOffsets[index].z = 0;
            avSampleOffsets[index].w = 1;

            index++;
        }
    }
}


void CHDRPostProcess::SetShaderParams()
{

    Vec4 vHDRSetupParams[5];
    gEnv->p3DEngine->GetHDRSetupParams(vHDRSetupParams);

    static CCryNameR szHDREyeAdaptationParam("HDREyeAdaptation");
    m_shHDR->FXSetPSFloat(szHDREyeAdaptationParam, CRenderer::CV_r_HDREyeAdaptationMode == 2 ? &vHDRSetupParams[4] : &vHDRSetupParams[3], 1);

    // RGB Film curve setup
    static CCryNameR szHDRFilmCurve("HDRFilmCurve");

    const Vec4 vHDRFilmCurve = vHDRSetupParams[0]; //* Vec4(0.22f, 0.3f, 0.01f, 1.0f);
    m_shHDR->FXSetPSFloat(szHDRFilmCurve, &vHDRFilmCurve, 1);

    static CCryNameR szHDRColorBalance("HDRColorBalance");
    const Vec4 vHDRColorBalance = vHDRSetupParams[2];
    m_shHDR->FXSetPSFloat(szHDRColorBalance, &vHDRColorBalance, 1);

    static CCryNameR szHDRBloomColor("HDRBloomColor");
    const Vec4 vHDRBloomColor = vHDRSetupParams[1] * Vec4(Vec3((1.0f / 8.0f)), 1.0f);   // division by 8.0f was done in shader before, remove this at some point
    m_shHDR->FXSetPSFloat(szHDRBloomColor, &vHDRBloomColor, 1);
    
    if (CRenderer::CV_r_ToneMapExposureType == static_cast<int>(Exposuretype::Manual))
    {
        static CCryNameR tonemapParams("HDRTonemapParams");
        Vec4 v = Vec4(CRenderer::CV_r_ToneMapManualExposureValue, 0, 0, 0);
        m_shHDR->FXSetPSFloat(tonemapParams, &v, 1);
    }
}


void CHDRPostProcess::SceneDownsampleUsingCompute()
{
    CTexture* pSrcRT = CTexture::s_ptexHDRTarget;
    CTexture* pDstRTs[3] =
    {
        CTexture::s_ptexHDRTargetScaled[0],
        CTexture::s_ptexHDRTargetScaled[1],
        nullptr
    };
    PostProcessUtils().DownsampleUsingCompute(pSrcRT, pDstRTs);
}


void CHDRPostProcess::HalfResDownsampleHDRTarget()
{
    PROFILE_LABEL_SCOPE("HALFRES_DOWNSAMPLE_HDRTARGET");

    CTexture* pSrcRT = CTexture::s_ptexHDRTarget;
    CTexture* pDstRT = CTexture::s_ptexHDRTargetScaled[0];

#if defined(CRY_USE_METAL) || defined(ANDROID)
    gRenDev->RT_SetScissor(true, 0, 0, gcpRendD3D->m_HalfResRect.right, gcpRendD3D->m_HalfResRect.bottom);
#endif

    if (CRenderer::CV_r_HDRBloomQuality >= 2)
    {
        PostProcessUtils().DownsampleStable(pSrcRT, pDstRT, true);
    }
    else
    {
        PostProcessUtils().StretchRect(pSrcRT, pDstRT, true);
    }

#ifdef CRY_USE_METAL
    gRenDev->RT_SetScissor(false, 0, 0, 0, 0);
#endif
}


void CHDRPostProcess::QuarterResDownsampleHDRTarget()
{
    PROFILE_LABEL_SCOPE("QUARTER_RES_DOWNSAMPLE_HDRTARGET");

    CTexture* pSrcRT = CTexture::s_ptexHDRTargetScaled[0];
    CTexture* pDstRT = CTexture::s_ptexHDRTargetScaled[1];

#if defined(CRY_USE_METAL) || defined(ANDROID)
    gRenDev->RT_SetScissor(true, 0, 0, (gcpRendD3D->m_HalfResRect.right + 1) >> 1, (gcpRendD3D->m_HalfResRect.bottom + 1) >> 1);
#endif

    // TODO: this pass seems redundant.  Can we get rid of it in non-gmem paths too?
    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        PostProcessUtils().DownsampleStable(pSrcRT, pDstRT, false);
    }

    // Try to merge both sunshafts mask gen with the scene downsample on GMEM mobile path
    CSunShafts* pSunShaftsTech = static_cast<CSunShafts*>(PostEffectMgr()->GetEffect(ePFX_SunShafts));
    if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr) &&
        CRenderer::CV_r_sunshafts &&
        CRenderer::CV_r_PostProcess &&
        pSunShaftsTech &&
        pSunShaftsTech->IsVisible())
    {
        // It is important that the following texture remains untouched until the sunshafts passes right
        // before tonemapping. At the moment, it doesn't look like any other passes make use of the RT.
        // This RT also must match what is passed later on to CSunShafts::SunShaftsGen(...)
        CTexture* pSunShaftsRT = CTexture::s_ptexBackBufferScaled[1];
        pSunShaftsTech->MergedSceneDownsampleAndSunShaftsMaskGen(pSrcRT, pDstRT, pSunShaftsRT);
    }
    else if (CRenderer::CV_r_HDRBloomQuality >= 2)
    {
        PostProcessUtils().DownsampleStable(pSrcRT, pDstRT, false);
    }
    else if (CRenderer::CV_r_HDRBloomQuality == 1)
    {
        PostProcessUtils().DownsampleStable(pSrcRT, pDstRT, true);
    }
    else
    {
        PostProcessUtils().StretchRect(pSrcRT, pDstRT);
    }

#if defined(CRY_USE_METAL) || defined(ANDROID)
    gRenDev->RT_SetScissor(false, 0, 0, 0, 0);
#endif
}


void CHDRPostProcess::MeasureLuminance()
{
    PROFILE_LABEL_SCOPE("MEASURE_LUMINANCE");
    int x, y, index;
    Vec4 avSampleOffsets[16];
    CD3D9Renderer* rd = gcpRendD3D;

    uint64 nFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
    gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE4] | g_HWSR_MaskBit[HWSR_SAMPLE5]);

    if (CRenderer::CV_r_SlimGBuffer == 1)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SLIM_GBUFFER];
    }

    int32 dwCurTexture = NUM_HDR_TONEMAP_TEXTURES - 1;
    static CCryNameR Param1Name("SampleOffsets");

    float tU = 1.0f / (3.0f * CTexture::s_ptexHDRToneMaps[dwCurTexture]->GetWidth());
    float tV = 1.0f / (3.0f * CTexture::s_ptexHDRToneMaps[dwCurTexture]->GetHeight());

    index = 0;
    for (x = -1; x <= 1; x++)
    {
        for (y = -1; y <= 1; y++)
        {
            avSampleOffsets[index].x = x * tU;
            avSampleOffsets[index].y = y * tV;
            avSampleOffsets[index].z = 0;
            avSampleOffsets[index].w = 1;

            index++;
        }
    }

    uint32 nPasses;

    rd->FX_PushRenderTarget(0, CTexture::s_ptexHDRToneMaps[dwCurTexture], NULL);

    rd->FX_SetColorDontCareActions(0, true, false);
    rd->FX_SetDepthDontCareActions(0, true, true);
    rd->FX_SetStencilDontCareActions(0, true, true);


    rd->FX_SetActiveRenderTargets();
    rd->RT_SetViewport(0, 0, CTexture::s_ptexHDRToneMaps[dwCurTexture]->GetWidth(), CTexture::s_ptexHDRToneMaps[dwCurTexture]->GetHeight());

    if (CRenderer::CV_r_HDREyeAdaptationMode == 2)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];
    }
    else
    {
        CTexture::s_ptexSceneNormalsMap->Apply(1, nTexStateLinear);
        CTexture::s_ptexSceneDiffuse->Apply(2, nTexStateLinear);
        CTexture::s_ptexSceneSpecular->Apply(3, nTexStateLinear);
    }
    
    static CCryNameTSCRC TechName("HDRSampleLumInitial");
    m_shHDR->FXSetTechnique(TechName);
    m_shHDR->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    m_shHDR->FXBeginPass(0);

    CTexture::s_ptexHDRTargetScaled[1]->Apply(0, nTexStateLinear);
    
    

    float s1 = 1.0f / static_cast<float>(CTexture::s_ptexHDRTargetScaled[1]->GetWidth());
    float t1 = 1.0f / static_cast<float>(CTexture::s_ptexHDRTargetScaled[1]->GetHeight());

    // Use rotated grid
    Vec4 vSampleLumOffsets0 = Vec4(s1 * 0.95f, t1 * 0.25f, -s1 * 0.25f, t1 * 0.96f);
    Vec4 vSampleLumOffsets1 = Vec4(-s1 * 0.96f, -t1 * 0.25f, s1 * 0.25f, -t1 * 0.96f);

    static CCryNameR pSampleLumOffsetsName0("SampleLumOffsets0");
    static CCryNameR pSampleLumOffsetsName1("SampleLumOffsets1");

    m_shHDR->FXSetPSFloat(pSampleLumOffsetsName0, &vSampleLumOffsets0, 1);
    m_shHDR->FXSetPSFloat(pSampleLumOffsetsName1, &vSampleLumOffsets1, 1);

    SetShaderParams();

    bool ret = DrawFullScreenQuad(0.0f, 1.0f - 1.0f * gcpRendD3D->m_CurViewportScale.y, 1.0f * gcpRendD3D->m_CurViewportScale.x, 1.0f);

    // important that we always write out valid luminance, even if quad draw fails
    if (!ret)
    {
        rd->FX_ClearTarget(CTexture::s_ptexHDRToneMaps[dwCurTexture], Clr_Dark);
    }

    m_shHDR->FXEndPass();

    rd->FX_PopRenderTarget(0);

    dwCurTexture--;

    // Initialize the sample offsets for the iterative luminance passes
    while (dwCurTexture >= 0)
    {
        rd->FX_PushRenderTarget(0, CTexture::s_ptexHDRToneMaps[dwCurTexture], NULL);

        // CONFETTI BEGIN: David Srour
        // Metal Load/Store Actions
        rd->FX_SetColorDontCareActions(0, true, false);
        rd->FX_SetDepthDontCareActions(0, true, true);
        rd->FX_SetStencilDontCareActions(0, true, true);
        // CONFETTI END

        rd->RT_SetViewport(0, 0, CTexture::s_ptexHDRToneMaps[dwCurTexture]->GetWidth(), CTexture::s_ptexHDRToneMaps[dwCurTexture]->GetHeight());

        if (!dwCurTexture)
        {
            gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
        }
        if (dwCurTexture == 1)
        {
            gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
        }

        static CCryNameTSCRC TechNameLI("HDRSampleLumIterative");
        m_shHDR->FXSetTechnique(TechNameLI);
        m_shHDR->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
        m_shHDR->FXBeginPass(0);

        GetSampleOffsets_DownScale4x4Bilinear(CTexture::s_ptexHDRToneMaps[dwCurTexture + 1]->GetWidth(), CTexture::s_ptexHDRToneMaps[dwCurTexture + 1]->GetHeight(), avSampleOffsets);
        m_shHDR->FXSetPSFloat(Param1Name, avSampleOffsets, 4);
        CTexture::s_ptexHDRToneMaps[dwCurTexture + 1]->Apply(0, nTexStateLinear);

        // Draw a fullscreen quad to sample the RT
        ret = DrawFullScreenQuad(0.0f, 0.0f, 1.0f, 1.0f);

        // important that we always write out valid luminance, even if quad draw fails
        if (!ret)
        {
            rd->FX_ClearTarget(CTexture::s_ptexHDRToneMaps[dwCurTexture], Clr_Dark);
        }

        m_shHDR->FXEndPass();

        rd->FX_PopRenderTarget(0);

        dwCurTexture--;
    }

    gcpRendD3D->GetDeviceContext().CopyResource(
        CTexture::s_ptexHDRMeasuredLuminance[gcpRendD3D->RT_GetCurrGpuID()]->GetDevTexture()->GetBaseTexture(),
        CTexture::s_ptexHDRToneMaps[0]->GetDevTexture()->GetBaseTexture());

    gRenDev->m_RP.m_FlagsShader_RT = nFlagsShader_RT;
}

void CHDRPostProcess::EyeAdaptation()
{
    PROFILE_LABEL_SCOPE("EYEADAPTATION");

    CD3D9Renderer* rd = gcpRendD3D;

    // Swap current & last luminance
    const int32 lumMask = static_cast<int32>((sizeof(CTexture::s_ptexHDRAdaptedLuminanceCur) / sizeof(CTexture::s_ptexHDRAdaptedLuminanceCur[0]))) - 1;
    const int32 numTextures = static_cast<int32>(max(min(gRenDev->GetActiveGPUCount(), static_cast<uint32>(sizeof(CTexture::s_ptexHDRAdaptedLuminanceCur) / sizeof(CTexture::s_ptexHDRAdaptedLuminanceCur[0]))), 1u));

    CTexture::s_nCurLumTextureIndex++;

    CTexture* pTexPrev = CTexture::s_ptexHDRAdaptedLuminanceCur[(CTexture::s_nCurLumTextureIndex - numTextures) & lumMask];
    CTexture* pTexCur = CTexture::s_ptexHDRAdaptedLuminanceCur[CTexture::s_nCurLumTextureIndex & lumMask];
    CTexture::s_ptexCurLumTexture = pTexCur;
    CRY_ASSERT(pTexCur);

    uint32 nPasses;
    static CCryNameTSCRC TechName("HDRCalculateAdaptedLum");
    m_shHDR->FXSetTechnique(TechName);
    m_shHDR->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    rd->FX_PushRenderTarget(0, pTexCur, NULL);

    // CONFETTI BEGIN: David Srour
    // Metal Load/Store Actions
    rd->FX_SetColorDontCareActions(0, true, false);
    rd->FX_SetDepthDontCareActions(0, true, true);
    rd->FX_SetStencilDontCareActions(0, true, true);
    // CONFETTI END

    rd->RT_SetViewport(0, 0, pTexCur->GetWidth(), pTexCur->GetHeight());

    m_shHDR->FXBeginPass(0);

    SetShaderParams();

    static CCryNameR Param1Name("ElapsedTime");

    {
        Vec4 elapsedTime;
        elapsedTime[0] = iTimer->GetFrameTime() * numTextures;
        elapsedTime[1] = 1.0f - expf(-CRenderer::CV_r_HDREyeAdaptationSpeed * elapsedTime[0]);
        elapsedTime[2] = 0;
        elapsedTime[3] = 0;

        if (rd->GetCamera().IsJustActivated() || rd->m_nDisableTemporalEffects > 0)
        {
            elapsedTime[1] = 1.0f;
            elapsedTime[2] = 1.0f;
        }

        m_shHDR->FXSetPSFloat(Param1Name, &elapsedTime, 1);
    }

    pTexPrev->Apply(0, nTexStatePoint);
    CTexture::s_ptexHDRToneMaps[0]->Apply(1, nTexStatePoint);

        // Draw a fullscreen quad to sample the RT
        DrawFullScreenQuad(0.0f, 0.0f, 1.0f, 1.0f);
    

    m_shHDR->FXEndPass();

    rd->FX_PopRenderTarget(0);
}


void CHDRPostProcess::MeasureLumEyeAdaptationUsingCompute()
{
    PROFILE_LABEL_SCOPE("MEASURE_LUM_EYE_ADAPT_CS");
    PROFILE_SHADER_SCOPE;

    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    // Constants used by CS shaders
    float hdrTargetWidth = static_cast<float>(CTexture::s_ptexHDRTargetScaled[1]->GetWidth());
    float hdrTargetHeight = static_cast<float>(CTexture::s_ptexHDRTargetScaled[1]->GetHeight());

    int lumStartingWidth = CTexture::s_ptexHDRToneMaps[NUM_HDR_TONEMAP_TEXTURES - 1]->GetWidth();
    int lumStartingHeight = CTexture::s_ptexHDRToneMaps[NUM_HDR_TONEMAP_TEXTURES - 1]->GetHeight();

    Vec4 vHdrTargetLumStartDims = Vec4(hdrTargetWidth, hdrTargetHeight, static_cast<float>(lumStartingWidth), static_cast<float>(lumStartingHeight));

    Vec4 vGBufferDims = Vec4(static_cast<float>(CTexture::s_ptexSceneDiffuse->GetWidth()), static_cast<float>(CTexture::s_ptexSceneDiffuse->GetHeight()), 0, 0);

    const int lumMask = static_cast<int32>((sizeof(CTexture::s_ptexHDRAdaptedLuminanceCur) / sizeof(CTexture::s_ptexHDRAdaptedLuminanceCur[0]))) - 1;
    const int32 numTextures = static_cast<int32>(max(min(gRenDev->GetActiveGPUCount(), static_cast<uint32>(sizeof(CTexture::s_ptexHDRAdaptedLuminanceCur) / sizeof(CTexture::s_ptexHDRAdaptedLuminanceCur[0]))), 1u));

    static CCryNameR pParamTimeName("ElapsedTime");
    static CCryNameR pHdrTargetLumStartDimsName("HdrTargetAndLumStartingDims");
    static CCryNameR pGbufferDimsName("GBufferDims");

    static CCryNameTSCRC pTech("MeasureLuminanceCS");

    uint32 nPasses;
    m_shHDR->FXSetTechnique(pTech);
    m_shHDR->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    D3DShaderResourceView* pSRV[3];
    D3DUnorderedAccessView* pUAV[3];

    // Grid dims must match in shader
    const uint32 kernelGridX = 8;
    const uint32 kernelGridY = 8;

    uint32 dispatchSizeX, dispatchSizeY;

    // Parallel reduction pass
    m_shHDR->FXBeginPass(0);


    m_shHDR->FXSetCSFloat(pHdrTargetLumStartDimsName, &vHdrTargetLumStartDims, 1);
    m_shHDR->FXSetCSFloat(pGbufferDimsName, &vGBufferDims, 1);

    rd->FX_Commit();

    // SRVs
    pSRV[0] = (CTexture::s_ptexHDRTargetScaled[1]->GetShaderResourceView());
    pSRV[1] = (CTexture::s_ptexSceneDiffuse->GetShaderResourceView());
    pSRV[2] = (CTexture::s_ptexSceneSpecular->GetShaderResourceView());
    rd->m_DevMan.BindSRV(eHWSC_Compute, pSRV, 0, 3);

    // UAVs
    // We can reuse CTexture::s_ptexHDRToneMaps[2] (16x16) to store the last parallel reduction data (64 pixels required)
    // Note that this will potentially need to be changed if NUM_HDR_TONEMAP_TEXTURES changes in the future.
    pUAV[0] = (CTexture::s_ptexHDRToneMaps[2]->GetDeviceUAV());
    rd->GetDeviceContext().CSSetUnorderedAccessViews(0, 1, pUAV, NULL);

    dispatchSizeX = lumStartingWidth / kernelGridX + (lumStartingWidth % kernelGridX > 0 ? 1 : 0);
    dispatchSizeY = lumStartingHeight / kernelGridY + (lumStartingHeight % kernelGridY > 0 ? 1 : 0);
    rd->m_DevMan.Dispatch(dispatchSizeX, dispatchSizeY, 1);

    m_shHDR->FXEndPass();

    // Final reduction and eye adaptation pass
    m_shHDR->FXBeginPass(1);

    m_shHDR->FXSetCSFloat(pHdrTargetLumStartDimsName, &vHdrTargetLumStartDims, 1);

    {
        Vec4 elapsedTime;
        elapsedTime[0] = iTimer->GetFrameTime() * numTextures;
        elapsedTime[1] = 1.0f - expf(-CRenderer::CV_r_HDREyeAdaptationSpeed * elapsedTime[0]);
        elapsedTime[2] = 0;
        elapsedTime[3] = 0;

        if (rd->GetCamera().IsJustActivated() || rd->m_nDisableTemporalEffects > 0)
        {
            elapsedTime[1] = 1.0f;
            elapsedTime[2] = 1.0f;
        }

        m_shHDR->FXSetCSFloat(pParamTimeName, &elapsedTime, 1);
    }

    // Swap current & last luminance
    CTexture::s_nCurLumTextureIndex++;
    CTexture* pTexPrev = CTexture::s_ptexHDRAdaptedLuminanceCur[(CTexture::s_nCurLumTextureIndex - numTextures) & lumMask];
    CTexture* pTexCur = CTexture::s_ptexHDRAdaptedLuminanceCur[CTexture::s_nCurLumTextureIndex & lumMask];
    CTexture::s_ptexCurLumTexture = pTexCur;
    CRY_ASSERT(pTexCur);

    // SRVs
    pSRV[0] = (CTexture::s_ptexHDRToneMaps[2]->GetShaderResourceView());
    pSRV[1] = (pTexPrev->GetShaderResourceView());
    rd->m_DevMan.BindSRV(eHWSC_Compute, pSRV, 0, 2);

    // UAVs
    pUAV[0] = (CTexture::s_ptexHDRMeasuredLuminance[gcpRendD3D->RT_GetCurrGpuID()]->GetDeviceUAV());
    pUAV[1] = (CTexture::s_ptexHDRToneMaps[0]->GetDeviceUAV());
    pUAV[2] = (pTexCur->GetDeviceUAV());
    rd->GetDeviceContext().CSSetUnorderedAccessViews(0, 3, pUAV, NULL);

    dispatchSizeX = lumStartingWidth / (kernelGridX * kernelGridY);
    dispatchSizeY = lumStartingHeight / (kernelGridX * kernelGridY);
    CRY_ASSERT(1 == dispatchSizeX && 1 == dispatchSizeY);

    rd->m_DevMan.Dispatch(dispatchSizeX, dispatchSizeY, 1);

    m_shHDR->FXEndPass();

    rd->FX_Commit();
}

void CHDRPostProcess::BloomGeneration()
{
    if (CRenderer::CV_r_GraphicsPipeline & 1)
    {
        gcpRendD3D->GetGraphicsPipeline().RenderBloom();
        return;
    }

    // Approximate function (1 - r)^4 by a sum of Gaussians: 0.0174*G(0.008,r) + 0.192*G(0.0576,r)
    const float sigma1 = sqrtf(0.008f);
    const float sigma2 = sqrtf(0.0576f - 0.008f);

    PROFILE_LABEL_SCOPE("BLOOM_GEN");

    CD3D9Renderer* rd = gcpRendD3D;
    static CCryNameTSCRC TechName("HDRBloomGaussian");
    static CCryNameR szHDRParam0("HDRParams0");

    uint64 nPrevFlagsShaderRT = rd->m_RP.m_FlagsShader_RT;
    rd->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]);

    int width = CTexture::s_ptexHDRFinalBloom->GetWidth();
    int height = CTexture::s_ptexHDRFinalBloom->GetHeight();

    // Note: Just scaling the sampling offsets depending on the resolution is not very accurate but works acceptably
    CRY_ASSERT(CTexture::s_ptexHDRFinalBloom->GetWidth() == CTexture::s_ptexHDRTarget->GetWidth() / 4);
    float scaleW = (static_cast<float>(width) / 400.0f) / static_cast<float>(width);
    float scaleH = (static_cast<float>(height) / 225.0f) / static_cast<float>(height);
    int32 texFilter = (CTexture::s_ptexHDRFinalBloom->GetWidth() == 400 && CTexture::s_ptexHDRFinalBloom->GetHeight() == 225) ? nTexStatePoint : nTexStateLinear;

    rd->FX_SetState(GS_NODEPTHTEST);
    rd->RT_SetViewport(0, 0, width, height);

    // Pass 1 Horizontal
    rd->FX_PushRenderTarget(0, CTexture::s_ptexHDRTempBloom[1], NULL);
    rd->FX_SetColorDontCareActions(0, true, false);
    rd->FX_SetDepthDontCareActions(0, true, true);
    rd->FX_SetStencilDontCareActions(0, true, true);
    SD3DPostEffectsUtils::ShBeginPass(m_shHDR, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    Vec4 v = Vec4(scaleW, 0, 0, 0);
    m_shHDR->FXSetPSFloat(szHDRParam0, &v, 1);
    CTexture::s_ptexHDRTargetScaled[1]->Apply(0, texFilter);
    SPostEffectsUtils::DrawFullScreenTri(width, height);
    SD3DPostEffectsUtils::ShEndPass();
    rd->FX_PopRenderTarget(0);

    // Pass 1 Vertical
    if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        rd->FX_PushRenderTarget(0, CTexture::s_ptexHDRFinalBloom, NULL);
    }
    else
    {
        rd->FX_PushRenderTarget(0, CTexture::s_ptexHDRTempBloom[0], NULL);
    }
    rd->FX_SetColorDontCareActions(0, true, false);
    rd->FX_SetDepthDontCareActions(0, true, true);
    rd->FX_SetStencilDontCareActions(0, true, true);
    SD3DPostEffectsUtils::ShBeginPass(m_shHDR, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    v = Vec4(0, scaleH, 0, 0);
    m_shHDR->FXSetPSFloat(szHDRParam0, &v, 1);
    CTexture::s_ptexHDRTempBloom[1]->Apply(0, texFilter);
    SPostEffectsUtils::DrawFullScreenTri(width, height);
    SD3DPostEffectsUtils::ShEndPass();
    rd->FX_PopRenderTarget(0);

    // For mobile we skip the second blur pass for performance reasons
    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        // Pass 2 Horizontal
        rd->FX_PushRenderTarget(0, CTexture::s_ptexHDRTempBloom[1], NULL);
        rd->FX_SetColorDontCareActions(0, true, false);
        rd->FX_SetDepthDontCareActions(0, true, true);
        rd->FX_SetStencilDontCareActions(0, true, true);
        SD3DPostEffectsUtils::ShBeginPass(m_shHDR, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
        v = Vec4((sigma2 / sigma1) * scaleW, 0, 0, 0);
        m_shHDR->FXSetPSFloat(szHDRParam0, &v, 1);
        CTexture::s_ptexHDRTempBloom[0]->Apply(0, texFilter);
        SPostEffectsUtils::DrawFullScreenTri(width, height);
        SD3DPostEffectsUtils::ShEndPass();
        rd->FX_PopRenderTarget(0);

        // Pass 2 Vertical
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
        rd->FX_PushRenderTarget(0, CTexture::s_ptexHDRFinalBloom, NULL);
        rd->FX_SetColorDontCareActions(0, true, false);
        rd->FX_SetDepthDontCareActions(0, true, true);
        rd->FX_SetStencilDontCareActions(0, true, true);
        SD3DPostEffectsUtils::ShBeginPass(m_shHDR, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
        v = Vec4(0, (sigma2 / sigma1) * scaleH, 0, 0);
        m_shHDR->FXSetPSFloat(szHDRParam0, &v, 1);
        CTexture::s_ptexHDRTempBloom[1]->Apply(0, texFilter);
        CTexture::s_ptexHDRTempBloom[0]->Apply(1, texFilter);
        SPostEffectsUtils::DrawFullScreenTri(width, height);
        SD3DPostEffectsUtils::ShEndPass();
        rd->FX_PopRenderTarget(0);
    }

    rd->m_RP.m_FlagsShader_RT = nPrevFlagsShaderRT;
}

void CHDRPostProcess::ProcessLensOptics()
{
    gcpRendD3D->m_RP.m_PersFlags2 &= ~RBPF2_LENS_OPTICS_COMPOSITE;
    if (CRenderer::CV_r_flares && CRenderer::CV_r_PostProcess)
    {
        const uint32 nBatchMask = SRendItem::BatchFlags(EFSLIST_LENSOPTICS, gcpRendD3D->m_RP.m_pRLD);
        if (nBatchMask & (FB_GENERAL | FB_TRANSPARENT))
        {
            PROFILE_LABEL_SCOPE("LENS_OPTICS");

            CTexture* pLensOpticsComposite = CTexture::s_ptexBackBufferScaledTemp[0];
            pLensOpticsComposite = CTexture::s_ptexSceneTargetR11G11B10F[0];
            
            gcpRendD3D->FX_PushRenderTarget(0, pLensOpticsComposite, 0);            
            gcpRendD3D->FX_SetColorDontCareActions(0, false, false);
            gcpRendD3D->FX_ClearTarget(pLensOpticsComposite, Clr_Transparent);            

            gcpRendD3D->m_RP.m_PersFlags2 |= RBPF2_NOPOSTAA;

            GetUtils().Log(" +++ Begin lens-optics scene +++ \n");
            gcpRendD3D->FX_ProcessRenderList(EFSLIST_LENSOPTICS, FB_GENERAL);
            gcpRendD3D->FX_ProcessRenderList(EFSLIST_LENSOPTICS, FB_TRANSPARENT);
            gcpRendD3D->FX_ResetPipe();
            GetUtils().Log(" +++ End lens-optics scene +++ \n");

            gcpRendD3D->FX_SetActiveRenderTargets();
            gcpRendD3D->FX_PopRenderTarget(0);
            gcpRendD3D->FX_SetActiveRenderTargets();
        }
    }
}

enum class eHDRPostProcessSRVs : int
{
    HDRInput = 0,
    Luminance = 1,
    Bloom = 2,
    Velocity = 3,
    ZTarget = 5,
    VignetteMap = 7,
    ColorChar = 8,
    SunShafts = 9,
    DolbyVisionDynamicMeta = 15
};

void CHDRPostProcess::ToneMapping()
{
    CD3D9Renderer* rd = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;

    if (!gcpRendD3D->m_pColorGradingControllerD3D)
    {
        return;
    }

    CSunShafts* pSunShaftsTech = static_cast<CSunShafts*>(PostEffectMgr()->GetEffect(ePFX_SunShafts));

    CTexture* pSunShaftsRT = CTextureManager::Instance()->GetBlackTexture();
    if (CRenderer::CV_r_sunshafts && CRenderer::CV_r_PostProcess && pSunShaftsTech && pSunShaftsTech->IsVisible())
    {
        // Create shafts mask texture

        uint32 nWidth = CTexture::s_ptexBackBufferScaled[1]->GetWidth();
        uint32 nHeight = CTexture::s_ptexBackBufferScaled[1]->GetHeight();
        pSunShaftsRT = CTexture::s_ptexBackBufferScaled[1];
        CTexture* pSunShaftsPingPongRT = CTexture::s_ptexBackBufferScaledTemp[1];
        if (rRP.m_eQuality >= eRQ_High &&
            !gcpRendD3D->FX_GetEnabledGmemPath(nullptr)) // GMEM always uses the downsampled target
        {
            pSunShaftsRT = CTexture::s_ptexBackBufferScaled[0];
            pSunShaftsPingPongRT = CTexture::s_ptexBackBufferScaledTemp[0];
            nWidth = CTexture::s_ptexBackBufferScaled[0]->GetWidth();
            nHeight = CTexture::s_ptexBackBufferScaled[0]->GetHeight();
        }

        pSunShaftsTech->SunShaftsGen(pSunShaftsRT, pSunShaftsPingPongRT);
    }

    // Update color grading

    bool bColorGrading = false;

    SColorGradingMergeParams pMergeParams;
    if (CRenderer::CV_r_colorgrading && CRenderer::CV_r_colorgrading_charts)
    {
        CColorGrading* pColorGrad = 0;
        if (!PostEffectMgr()->GetEffects().empty())
        {
            pColorGrad =  static_cast<CColorGrading*>(PostEffectMgr()->GetEffect(ePFX_ColorGrading));
        }

        if (pColorGrad && pColorGrad->UpdateParams(pMergeParams))
        {
            bColorGrading = true;
        }
    }

    CColorGradingControllerD3D* pCtrl = gcpRendD3D->m_pColorGradingControllerD3D;
    CTexture* pTexColorChar = pCtrl ? pCtrl->GetColorChart() : 0;

    rd->RT_SetViewport(0, 0, CTexture::s_ptexHDRTarget->GetWidth(), CTexture::s_ptexHDRTarget->GetHeight());

    PROFILE_LABEL_SCOPE("TONEMAPPING");

    // Enable corresponding shader variation
    rRP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3] | g_HWSR_MaskBit[HWSR_SAMPLE4] | g_HWSR_MaskBit[HWSR_SAMPLE5]);

    if (CRenderer::CV_r_HDREyeAdaptationMode == 2)
    {
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];
    }

    SetExposureTypeShaderFlags();

    if (bColorGrading && pTexColorChar)
    {
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
    }

    if (CRenderer::CV_r_HDRDebug == 5)
    {
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG0];
    }

    const uint32 nAAMode = rd->FX_GetAntialiasingType();
    if (nAAMode & eAT_FXAA_MASK)
    {
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    }
    
    PostProcessUtils().SetSRGBShaderFlags();
    
    rd->FX_SetColorDontCareActions(0, true, false);
    rd->FX_SetStencilDontCareActions(0, true, true);
    rd->FX_SetColorDontCareActions(1, true, false);
    rd->FX_SetStencilDontCareActions(1, true, true);
    

    bool isAfterPostProcessBucketEmpty = SRendItem::IsListEmpty(EFSLIST_AFTER_POSTPROCESS, rd->m_RP.m_nProcessThreadID, rd->m_RP.m_pRLD);
    
    bool isAuxGeomEnabled = false;
#if defined(ENABLE_RENDER_AUX_GEOM)
    isAuxGeomEnabled = CRenderer::CV_r_enableauxgeom == 1;
#endif
    
    //We may need to preserve the depth buffer in case there is something to render in the EFSLIST_AFTER_POSTPROCESS bucket.
    //It could be UI in the 3d world. If the bucket is empty ignore the depth buffer as it is not needed.
    //Also check if Auxgeom rendering is enabled in which case we preserve depth buffer.
    if (isAfterPostProcessBucketEmpty && !isAuxGeomEnabled)
    {
        rd->FX_SetDepthDontCareActions(0, true, true);
        rd->FX_SetDepthDontCareActions(1, true, true);
    }
    else
    {
        rd->FX_SetDepthDontCareActions(0, false, false);
        rd->FX_SetDepthDontCareActions(1, false, false);
    }

    // Final bloom RT

    // Noise offset was originally defined before VS parameter: "FrameRand" - algorithm untouched from original CryEngine implementation.
    // Was moved to this location because for multi-pass the noise offset needs to be the same or there will be a mismatch.

    static uint32 dwNoiseOffsetX = 0;
    static uint32 dwNoiseOffsetY = 0;
    dwNoiseOffsetX = (dwNoiseOffsetX + 27) & 0x3f;
    dwNoiseOffsetY = (dwNoiseOffsetY + 19) & 0x3f;

    Vec4 pFrameRand(
        dwNoiseOffsetX / 64.0f,
        dwNoiseOffsetX / 64.0f,
        cry_random(0, 1023) / 1024.0f,
        cry_random(0, 1023) / 1024.0f);

    static ICVar* DolbyCvar = gEnv->pConsole->GetCVar("r_HDRDolby");
    int DolbyCvarValue = DolbyCvar ? DolbyCvar->GetIVal() : eDVM_Disabled;

    // Calculate dynamic metadata for Dolby vision if enabled.
    if (DolbyCvarValue == eDVM_Vision && CRenderer::CV_r_HDRDolbyDynamicMetadata == 1)
    {
        CalculateDolbyDynamicMetadata(pSunShaftsRT);
    }

    {
        CTexture*   pBloom = CTextureManager::Instance()->GetBlackTexture();
        if (CRenderer::CV_r_HDRBloom && CRenderer::CV_r_PostProcess)
        {
            pBloom = CTexture::s_ptexHDRFinalBloom;
        }

        PREFAST_ASSUME(pBloom);
        CRY_ASSERT(pBloom);

        uint32 nPasses;
        static CCryNameTSCRC TechFinalName("HDRFinalPass");
        static CCryNameTSCRC TechFinalDolbyName("HDRFinalPassDolby");        
        switch (DolbyCvarValue)
        {
        case eDVM_Disabled:
            m_shHDR->FXSetTechnique(GetTonemapTechnique());           
            break;
        case eDVM_RGBPQ:
        case eDVM_Vision:
            m_shHDR->FXSetTechnique(TechFinalDolbyName);
            break;
        }
        m_shHDR->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
        m_shHDR->FXBeginPass(0);

        gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

        SetShaderParams();

        // If any dolby output mode has been enabled, set required uniforms.
        if (DolbyCvarValue >= eDVM_RGBPQ)
        {
            static CCryNameR pszHDRDolbyParam0("HDRDolbyScurveParams0");
            static CCryNameR pszHDRDolbyParam1("HDRDolbyScurveParams1");
            static CCryNameR pszHDRDolbyParam2("HDRDolbyScurveParams2");

            Vec4 vHDRDolbyParam0(CRenderer::CV_r_HDRDolbyScurve ? 1.0f : 0.0f, CRenderer::CV_r_HDRDolbyScurveSourceMin, CRenderer::CV_r_HDRDolbyScurveSourceMid, CRenderer::CV_r_HDRDolbyScurveSourceMax);
            Vec4 vHDRDolbyParam1(CRenderer::CV_r_HDRDolbyScurveRGBPQTargetMin, CRenderer::CV_r_HDRDolbyScurveRGBPQTargetMid, CRenderer::CV_r_HDRDolbyScurveRGBPQTargetMax, CRenderer::CV_r_HDRDolbyScurveSlope);
            Vec4 vHDRDolbyParam2(CRenderer::CV_r_HDRDolbyDynamicMetadata ? 1.0f : 0.0f, 0.0f, 0.0f, CRenderer::CV_r_HDRDolbyScurveScale);
            if (DolbyCvarValue == eDVM_Vision)
            {
                vHDRDolbyParam1.x = CRenderer::CV_r_HDRDolbyScurveVisionTargetMin;
                vHDRDolbyParam1.y = CRenderer::CV_r_HDRDolbyScurveVisionTargetMid;
                vHDRDolbyParam1.z = CRenderer::CV_r_HDRDolbyScurveVisionTargetMax;
            }

            m_shHDR->FXSetPSFloat(pszHDRDolbyParam0, &vHDRDolbyParam0, 1);
            m_shHDR->FXSetPSFloat(pszHDRDolbyParam1, &vHDRDolbyParam1, 1);
            m_shHDR->FXSetPSFloat(pszHDRDolbyParam2, &vHDRDolbyParam2, 1);
        }

        static CCryNameR pSunShaftsParamSName("SunShafts_SunCol");
        Vec4 pShaftsSunCol(0, 0, 0, 0);
        if (pSunShaftsRT)
        {
            Vec4 pSunShaftsParams[2];
            pSunShaftsTech->GetSunShaftsParams(pSunShaftsParams);
            Vec3 pSunColor = gEnv->p3DEngine->GetSunColor();
            pSunColor.Normalize();
            pSunColor.SetLerp(Vec3(pSunShaftsParams[0].x, pSunShaftsParams[0].y, pSunShaftsParams[0].z), pSunColor, pSunShaftsParams[1].w);

            pShaftsSunCol = Vec4(pSunColor * pSunShaftsParams[1].z, 1);
        }

        m_shHDR->FXSetPSFloat(pSunShaftsParamSName, &pShaftsSunCol, 1);

        // Force commit before setting samplers - workaround for per frame samplers hardcoded/overriding sampler slots.
        rd->FX_Commit();

        CTexture::s_ptexHDRTarget->Apply(static_cast<int>(eHDRPostProcessSRVs::HDRInput), nTexStateLinear, EFTT_UNKNOWN, -1, SResourceView::DefaultView);

        if (CTexture::s_ptexCurLumTexture)
        {
            if (!gRenDev->m_CurViewportID)
            {
                CTexture::s_ptexCurLumTexture->Apply(static_cast<int>(eHDRPostProcessSRVs::Luminance), nTexStateLinear /*nTexStatePoint*/);
            }
            else
            {
                CTexture::s_ptexHDRToneMaps[0]->Apply(static_cast<int>(eHDRPostProcessSRVs::Luminance), nTexStateLinear);
            }
        }

        pBloom->Apply(static_cast<int>(eHDRPostProcessSRVs::Bloom), nTexStateLinear);

        if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
        {
            CTexture::s_ptexVelocity->Apply(static_cast<int>(eHDRPostProcessSRVs::Velocity), nTexStatePoint);
        }
        else
        {
            CTexture::s_ptexZTarget->Apply(static_cast<int>(eHDRPostProcessSRVs::ZTarget), nTexStatePoint);
        }

        AZ_PUSH_DISABLE_WARNING(, "-Wconstant-logical-operand")
        if (CRenderer::CV_r_PostProcess && CRenderer::CV_r_HDRVignetting)
        AZ_POP_DISABLE_WARNING
        {
            CTextureManager::Instance()->GetDefaultTexture("VignettingMap")->Apply(static_cast<int>(eHDRPostProcessSRVs::VignetteMap), nTexStateLinear);
        }
        else
        {
            CTexture*   pWhite = CTextureManager::Instance()->GetWhiteTexture();
            pWhite->Apply(static_cast<int>(eHDRPostProcessSRVs::VignetteMap), nTexStateLinear);
        }

        if (pTexColorChar)
        {
            pTexColorChar->Apply(static_cast<int>(eHDRPostProcessSRVs::ColorChar), nTexStateLinear);
        }

        if (pSunShaftsRT)
        {
            pSunShaftsRT->Apply(static_cast<int>(eHDRPostProcessSRVs::SunShafts), nTexStateLinear);
        }

        SD3DPostEffectsUtils::DrawFullScreenTriWPOS(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());
    }

    // Reset don't care actions for UI and other passes after post-proc pipeline
    rd->FX_SetColorDontCareActions(0, false, false);
}

void CHDRPostProcess::EncodeDolbyVision(CTexture* source)
{
    CD3D9Renderer* rd = gcpRendD3D;
    int NumPasses = 2;

    for (int pass = 0; pass < NumPasses; pass++)
    {
        uint32 nPasses;
        static CCryNameTSCRC TechFinalDolbyVisionName("HDRFinalPassDolbyVision");
        static CCryNameTSCRC TechFinalDolbyVisionNoMetadataName("HDRFinalPassDolbyVisionNoMetadata");

        if (pass == 0)
        {
            m_shHDR->FXSetTechnique(TechFinalDolbyVisionName);
        }
        else if (pass == 1)
        {
            m_shHDR->FXSetTechnique(TechFinalDolbyVisionNoMetadataName);
        }

        m_shHDR->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
        m_shHDR->FXBeginPass(0);

        gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

        // Set all shader params
        SetShaderParams();

        // If any dolby output mode has been enabled, set required uniforms.
        {

            static CCryNameR pszHDRDolbyParam0("HDRDolbyScurveParams0");
            static CCryNameR pszHDRDolbyParam1("HDRDolbyScurveParams1");
            static CCryNameR pszHDRDolbyParam2("HDRDolbyScurveParams2");

            Vec4 vHDRDolbyParam0(CRenderer::CV_r_HDRDolbyScurve ? 1.0f : 0.0f, CRenderer::CV_r_HDRDolbyScurveSourceMin, CRenderer::CV_r_HDRDolbyScurveSourceMid, CRenderer::CV_r_HDRDolbyScurveSourceMax);
            Vec4 vHDRDolbyParam1(CRenderer::CV_r_HDRDolbyScurveVisionTargetMin, CRenderer::CV_r_HDRDolbyScurveVisionTargetMid, CRenderer::CV_r_HDRDolbyScurveVisionTargetMax, CRenderer::CV_r_HDRDolbyScurveSlope);
            Vec4 vHDRDolbyParam2(CRenderer::CV_r_HDRDolbyDynamicMetadata ? 1.0f : 0.0f, 0.0, 0.0, CRenderer::CV_r_HDRDolbyScurveScale);

            m_shHDR->FXSetPSFloat(pszHDRDolbyParam0, &vHDRDolbyParam0, 1);
            m_shHDR->FXSetPSFloat(pszHDRDolbyParam1, &vHDRDolbyParam1, 1);
            m_shHDR->FXSetPSFloat(pszHDRDolbyParam2, &vHDRDolbyParam2, 1);
        }

        // Force commit before setting samplers - workaround for per frame samplers hardcoded/overriding sampler slots.
        rd->FX_Commit();

        SResourceView::KeyType nResourceID = SResourceView::DefaultView;

        source->Apply(static_cast<int>(eHDRPostProcessSRVs::HDRInput), nTexStateLinear, EFTT_UNKNOWN, -1, nResourceID);

        // Provide dynamic metadata values to the shader if enabled.
        if (CRenderer::CV_r_HDRDolbyDynamicMetadata == 1)
        {
            rd->m_DevMan.BindSRV(eHWSC_Pixel, m_bufDolbyMetadataMinMaxMid.GetShaderResourceView(), static_cast<int>(eHDRPostProcessSRVs::DolbyVisionDynamicMeta));
        }

        {
            // Dolby Vision split rendering (metadata/non-metadata).

            // * Calculate split boundaries.
            // Metadata needs 128 bytes * 3 times (according to dolby 3x repeater spec).
            // Every pixel stores 1 bit so at least 3072 pixels are needed to store the metadata. 4000 is a safe margin above that value.
            float pixelsNeeded = 4000.0f;
            float rowsNeeded = pixelsNeeded / rd->GetBackbufferWidth();
            float pixelRows = ceil(rowsNeeded);
            float pixelRowsNormalized = pixelRows / rd->GetBackbufferHeight();

            // * Draw split quad.
            Vec2 srcLeftTop(0.0f, 0.0f), srcRightBottom(1.0f, 1.0f);
            if (pass == 0)
            {
                srcRightBottom.y = pixelRowsNormalized;     // Draw top half with metadata.
            }
            else
            {
                srcLeftTop.y = pixelRowsNormalized;         // Draw bottom half without metadata.
            }

            SD3DPostEffectsUtils::DrawQuad(-1, -1, Vec2(srcLeftTop.x, srcLeftTop.y), Vec2(srcLeftTop.x, srcRightBottom.y), Vec2(srcRightBottom.x, srcRightBottom.y), Vec2(srcRightBottom.x, srcLeftTop.y),
                Vec2(srcLeftTop.x, srcLeftTop.y), Vec2(srcLeftTop.x, srcRightBottom.y), Vec2(srcRightBottom.x, srcRightBottom.y), Vec2(srcRightBottom.x, srcLeftTop.y));
        }
    }
}

CCryNameTSCRC CHDRPostProcess::GetTonemapTechnique() const
{
    ToneMapOperators toneMapTech = static_cast<ToneMapOperators>(CRenderer::CV_r_ToneMapTechnique);
    switch(toneMapTech)
    {
        case ToneMapOperators::Linear:
            return CCryNameTSCRC("HDRToneMapLinear");
        case ToneMapOperators::Exponential:
            return CCryNameTSCRC("HDRToneMapExponential");
        case ToneMapOperators::Reinhard:
            return CCryNameTSCRC("HDRToneMapReinhard");
        case ToneMapOperators::FilmicCurveALU:
            return CCryNameTSCRC("HDRToneMapFilmicALU");
        case ToneMapOperators::FilmicCurveUC2:
            return CCryNameTSCRC("HDRFinalPass");       
        default:
        {
            CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, "Tonemap technique not supported");
            return CCryNameTSCRC("HDRFinalPass");
        }            
    }    
}
  
void CHDRPostProcess::SetExposureTypeShaderFlags()
{
    gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE5]);
    Exposuretype expType = static_cast<Exposuretype>(CRenderer::CV_r_ToneMapExposureType);
    switch(expType)
    {
        case Exposuretype::Auto:
        {
            gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE5];
            return;
        }
        case Exposuretype::Manual:
        {
            //Don't need to do anything as it will default to manual.
            return;
        }
        default:
        {
            CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, "Exposure type not supported");
        }
    }
}

void CHDRPostProcess::ToneMappingDebug()
{
    Vec4 avSampleOffsets[4];
    PROFILE_LABEL_SCOPE("TONEMAPPINGDEBUG");

    // Enable corresponding shader variation
    gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE4] | g_HWSR_MaskBit[HWSR_SAMPLE5]);


    if (CRenderer::CV_r_HDRDebug == 1)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG0];
    }
    else
    {
        gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_DEBUG0];
    }

    if (CRenderer::CV_r_HDREyeAdaptationMode == 2)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];
    }

    SetExposureTypeShaderFlags();

    uint32 nPasses;
    static CCryNameTSCRC techName("HDRFinalDebugPass");
    m_shHDR->FXSetTechnique(techName);
    m_shHDR->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    m_shHDR->FXBeginPass(0);

    GetSampleOffsets_DownScale2x2(CTexture::s_ptexHDRTarget->GetWidth(), CTexture::s_ptexHDRTarget->GetHeight(), avSampleOffsets);
    static CCryNameR SampleOffsetsName("SampleOffsets");
    m_shHDR->FXSetPSFloat(SampleOffsetsName, avSampleOffsets, 4);

    SetShaderParams();

    CTexture::s_ptexHDRTarget->Apply(0, nTexStatePoint);
    CTexture::s_ptexHDRToneMaps[0]->Apply(1, nTexStateLinear);
    DrawFullScreenQuad(0.0f, 0.0f, 1.0f, 1.0f);

    gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_DEBUG0];
}


void CHDRPostProcess::CalculateDolbyDynamicMetadata(CTexture* pSunShaftsRT)
{
    PROFILE_LABEL_SCOPE("DOLBY_DYNAMIC_META");
    CD3D9Renderer* rd = &gcpRendD3D;

    // Settings (must match shaders!)
    int width = gRenDev->GetBackbufferWidth();
    int height = gRenDev->GetBackbufferHeight();
    int pass0ReductionX = 16;
    int pass0ReductionY = 16;
    int pass0ThreadsX = 16;
    int pass0ThreadsY = 16;
    int pass0StrideX = 2;
    int pass0StrideY = 2;
    double outWidth = width / static_cast<double>(pass0ReductionX * pass0StrideX);
    double outHeight = height / static_cast<double>(pass0ReductionY * pass0StrideY);
    double dispatchX = width / static_cast<double>(pass0ReductionX * pass0ThreadsX * pass0StrideX);
    double dispatchY = height / static_cast<double>(pass0ReductionY * pass0ThreadsY * pass0StrideY);
    dispatchX = ceil(dispatchX);
    dispatchY = ceil(dispatchY);
    outWidth = ceil(outWidth);
    outHeight = ceil(outHeight);

    // Make sure StructuredBuffers are initialized..
    if (m_bufDolbyMetadataMacroReductionOutput.m_pBuffer == nullptr)
    {
        // Max resolution 4K (4096x2160)
        m_bufDolbyMetadataMacroReductionOutput.Create((4096 * 2160) / (pass0ReductionX * pass0ReductionY), sizeof(float) * 4, DXGI_FORMAT_UNKNOWN, DX11BUF_STRUCTURED | DX11BUF_BIND_UAV | DX11BUF_BIND_SRV, nullptr);
        m_bufDolbyMetadataMinMaxMid.Create(3, sizeof(float), DXGI_FORMAT_UNKNOWN, DX11BUF_STRUCTURED | DX11BUF_BIND_UAV | DX11BUF_BIND_SRV, nullptr);
    }

    // Make sure shaders are loaded..
    if (m_shHDRDolbyMetadataPass0 == nullptr)
    {
        m_shHDRDolbyMetadataPass0 = gcpRendD3D->m_cEF.mfForName("HDRDolbyMetadataPass0", EF_SYSTEM);
        m_shHDRDolbyMetadataPass1 = gcpRendD3D->m_cEF.mfForName("HDRDolbyMetadataPass1", EF_SYSTEM);
    }

    // Shared variables.
    uint32 nPasses;
    Vec4 parameters[2];
    static CCryNameR Parameter0Name("Parameters0");
    static CCryNameR Parameter1Name("Parameters1");

    // Pass 1: Macro reduction of HDR signal. (256x reduction)
    parameters[0] = Vec4(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    parameters[1] = Vec4(static_cast<float>(outWidth), 0.0f, 0.0f, 0.0f);
    m_shHDRDolbyMetadataPass0->FXSetTechnique("Default");
    m_shHDRDolbyMetadataPass0->FXBegin(&nPasses, 0);
    m_shHDRDolbyMetadataPass0->FXBeginPass(0);
    m_shHDRDolbyMetadataPass0->FXSetCSFloat(Parameter0Name, parameters, 1);
    m_shHDRDolbyMetadataPass0->FXSetCSFloat(Parameter1Name, parameters + 1, 1);
    rd->FX_Commit();

    // Bind HDR targets in order to recreating the same HDR input signal.
    CTexture::s_ptexHDRTarget->Apply(0, nTexStateLinear, EFTT_UNKNOWN, -1, SResourceView::DefaultView, eHWSC_Compute);
    CTexture::s_ptexHDRToneMaps[0]->Apply(1, nTexStateLinear, EFTT_UNKNOWN, -1, SResourceView::DefaultView, eHWSC_Compute);
    CTexture*   pBloom = CTextureManager::Instance()->GetBlackTexture();
    if (CRenderer::CV_r_HDRBloom && CRenderer::CV_r_PostProcess)
    {
        pBloom = CTexture::s_ptexHDRFinalBloom;
    }
    pBloom->Apply(2, nTexStateLinear, EFTT_UNKNOWN, -1, SResourceView::DefaultView, eHWSC_Compute);
    if (pSunShaftsRT)
    {
        pSunShaftsRT->Apply(9, nTexStateLinear, EFTT_UNKNOWN, -1, SResourceView::DefaultView, eHWSC_Compute);
    }

    rd->m_DevMan.BindUAV(eHWSC_Compute, m_bufDolbyMetadataMacroReductionOutput.GetUnorderedAccessView(), 0, 0);

    // Execute pass 1.
    rd->m_DevMan.Dispatch(static_cast<int>(dispatchX), static_cast<int>(dispatchY), 1);
    m_shHDRDolbyMetadataPass0->FXEndPass();
    m_shHDRDolbyMetadataPass0->FXEnd();

    // Unbind UAV.
    rd->m_DevMan.BindUAV(eHWSC_Compute, nullptr, 0, 0);
    rd->m_DevMan.CommitDeviceStates();

    // Pass 2: Micro reduction. (remainder)
    parameters[0] = Vec4(static_cast<float>(outWidth * outHeight), 0.0f, 0.0f, 0.0f);
    m_shHDRDolbyMetadataPass1->FXSetTechnique("Default");
    m_shHDRDolbyMetadataPass1->FXBegin(&nPasses, 0);
    m_shHDRDolbyMetadataPass1->FXBeginPass(0);
    m_shHDRDolbyMetadataPass1->FXSetCSFloat(Parameter0Name, parameters, 1);
    rd->FX_Commit();

    rd->m_DevMan.BindUAV(eHWSC_Compute, m_bufDolbyMetadataMinMaxMid.GetUnorderedAccessView(), 0, 0); // Unbind previous UAV.
    rd->m_DevMan.BindSRV(eHWSC_Compute, m_bufDolbyMetadataMacroReductionOutput.GetShaderResourceView(), 0);

    // Execute pass 2.
    rd->m_DevMan.Dispatch(1, 1, 1);
    m_shHDRDolbyMetadataPass1->FXEndPass();
    m_shHDRDolbyMetadataPass1->FXEnd();

    // Unbind UAV.
    rd->m_DevMan.BindUAV(eHWSC_Compute, nullptr, 0, 0);
    rd->m_DevMan.CommitDeviceStates();
}


void CHDRPostProcess::DrawDebugViews()
{
    if (CRenderer::CV_r_HDRDebug != 1 && CRenderer::CV_r_HDRDebug != 3 && CRenderer::CV_r_HDRDebug != 4)
    {
        return;
    }

    CD3D9Renderer* rd = gcpRendD3D;
    uint32 nPasses = 0;

    if (CRenderer::CV_r_HDRDebug == 1)
    {
        // We use screen shots to create minimaps, and we don't want to
        // have any debug text on minimaps.
        {
            ICVar* const pVar = gEnv->pConsole->GetCVar("e_ScreenShot");
            if (pVar && pVar->GetIVal() != 0)
            {
                return;
            }
        }

        STALL_PROFILER("read scene luminance")

        float fLuminance = -1;
        float fIlluminance = -1;

        CDeviceTexture* pSrcDevTex = CTexture::s_ptexHDRToneMaps[0]->GetDevTexture();
        pSrcDevTex->DownloadToStagingResource(0, [&](void* pData, [[maybe_unused]] uint32 rowPitch, [[maybe_unused]] uint32 slicePitch)
            {
                CryHalf* pRawPtr = reinterpret_cast<CryHalf*>(pData);
                fLuminance = CryConvertHalfToFloat(pRawPtr[0]);
                fIlluminance = CryConvertHalfToFloat(pRawPtr[1]);
                return true;
            });

        char str[256];
        SDrawTextInfo ti;
        ti.color[1] = 0;
        azsprintf(str, "Average Luminance (cd/m2): %.2f", fLuminance * RENDERER_LIGHT_UNIT_SCALE);
        rd->Draw2dText(5, 35, str, ti);
        azsprintf(str, "Estimated Illuminance (lux): %.1f", fIlluminance * RENDERER_LIGHT_UNIT_SCALE);
        rd->Draw2dText(5, 55, str, ti);

        Vec4 vHDRSetupParams[5];
        gEnv->p3DEngine->GetHDRSetupParams(vHDRSetupParams);

        if (CRenderer::CV_r_HDREyeAdaptationMode == 2)
        {
            // Compute scene key and exposure in the same way as in the tone mapping shader
            float sceneKey = 1.03f - 2.0f / (2.0f + log(fLuminance + 1.0f) / log(2.0f));
            float exposure = clamp_tpl<float>(sceneKey / fLuminance, vHDRSetupParams[4].y, vHDRSetupParams[4].z);

            azsprintf(str, "Exposure: %.2f  SceneKey: %.2f", exposure, sceneKey);
            rd->Draw2dText(5, 75, str, ti);
        }
        else
        {
            float exposure = log(fIlluminance * RENDERER_LIGHT_UNIT_SCALE * 100.0f / 330.0f) / log(2.0f);
            float sceneKey = log(fIlluminance * RENDERER_LIGHT_UNIT_SCALE + 1.0f) / log(10.0f);
            float autoCompensation = (clamp_tpl<float>(sceneKey, 0.1f, 5.2f) - 3.0f) / 2.0f * vHDRSetupParams[3].z;
            float finalExposure = clamp_tpl<float>(exposure - autoCompensation, vHDRSetupParams[3].x, vHDRSetupParams[3].y);

            azsprintf(str, "Measured EV: %.1f  Auto-EC: %.1f  Final EV: %.1f", exposure, autoCompensation, finalExposure);
            rd->Draw2dText(5, 75, str, ti);
        }

        return;
    }

    rd->FX_SetState(GS_NODEPTHTEST);
    int iTmpX, iTmpY, iTempWidth, iTempHeight;
    rd->GetViewport(&iTmpX, &iTmpY, &iTempWidth, &iTempHeight);

    rd->EF_SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
    rd->EF_SetSrgbWrite(false);

    TransformationMatrices backupSceneMatrices;
    rd->Set2DMode(1, 1, backupSceneMatrices);

    gRenDev->m_cEF.mfRefreshSystemShader("Debug", CShaderMan::s_ShaderDebug);
    CShader* pSH = CShaderMan::s_ShaderDebug;

    CTexture* pSceneTargetHalfRes = CTexture::s_ptexHDRTargetScaled[0];
    CTexture::s_ptexHDRTarget->SetResolved(false);
    CTexture::s_ptexHDRTarget->Resolve();
    // 1
    uint32 nPosX = 10;
    rd->RT_SetViewport(nPosX, 500, 100, 100);
    rd->DrawImage(0, 0, 1, 1, CTexture::s_ptexHDRTarget->GetID(), 0, 1, 1, 0, 1, 1, 1, 1);

    // 2
    rd->RT_SetViewport(nPosX += 110, 500, 100, 100);
    rd->DrawImage(0, 0, 1, 1, pSceneTargetHalfRes->GetID(), 0, 1, 1, 0, 1, 1, 1, 1);

    // 3
    if (CRenderer::CV_r_HDRBloom)
    {
        // Bloom generation/intermediate render-targets

        // Quarter res
        rd->RT_SetViewport(nPosX += 110, 500, 100, 100);
        rd->DrawImage(0, 0, 1, 1, CTexture::s_ptexHDRTempBloom[0]->GetID(), 0, 1, 1, 0, 1, 1, 1, 1);

        rd->RT_SetViewport(nPosX += 110, 500, 100, 100);
        rd->DrawImage(0, 0, 1, 1, CTexture::s_ptexHDRTempBloom[1]->GetID(), 0, 1, 1, 0, 1, 1, 1, 1);

        rd->RT_SetViewport(nPosX += 110, 500, 100, 100);
        rd->DrawImage(0, 0, 1, 1, CTexture::s_ptexHDRFinalBloom->GetID(), 0, 1, 1, 0, 1, 1, 1, 1);
    }

    nPosX = 10;

    pSH->FXSetTechnique("Debug_ShowR");
    pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    pSH->FXBeginPass(0);

    CTexture::s_ptexHDRToneMaps[3]->Apply(0, nTexStatePoint);
    rd->RT_SetViewport(nPosX, 610, 100, 100);
    DrawFullScreenQuadTR(0.0f, 0.0f, 1.0f, 1.0f);

    if (CTexture::s_ptexHDRToneMaps[2])
    {
        CTexture::s_ptexHDRToneMaps[2]->Apply(0, nTexStatePoint);
        rd->RT_SetViewport(nPosX += 110, 610, 100, 100);
        DrawFullScreenQuadTR(0.0f, 0.0f, 1.0f, 1.0f);
    }

    if (CTexture::s_ptexHDRToneMaps[1])
    {
        CTexture::s_ptexHDRToneMaps[1]->Apply(0, nTexStatePoint);
        rd->RT_SetViewport(nPosX += 110, 610, 100, 100);
        DrawFullScreenQuadTR(0.0f, 0.0f, 1.0f, 1.0f);
    }

    if (CTexture::s_ptexHDRToneMaps[0])
    {
        CTexture::s_ptexHDRToneMaps[0]->Apply(0, nTexStatePoint);
        rd->RT_SetViewport(nPosX += 110, 610, 100, 100);
        DrawFullScreenQuadTR(0.0f, 0.0f, 1.0f, 1.0f);
    }

    if (CTexture::s_ptexCurLumTexture)
    {
        CTexture::s_ptexCurLumTexture->Apply(0, nTexStatePoint);
        rd->RT_SetViewport(nPosX += 110, 610, 100, 100);
        DrawFullScreenQuadTR(0.0f, 0.0f, 1.0f, 1.0f);
    }

    pSH->FXEndPass();
    pSH->FXEnd();

    rd->Unset2DMode(backupSceneMatrices);

    rd->RT_SetViewport(iTmpX, iTmpY, iTempWidth, iTempHeight);

    {
        char str[256];

        SDrawTextInfo ti;

        sprintf_s(str, "HDR rendering debug");
        rd->Draw2dText(5, 310, str, ti);
    }
}


void CHDRPostProcess::ScreenShot()
{
    if (CRenderer::CV_r_GetScreenShot == 1)
    {
        iLog->LogError("HDR screen shots are not yet supported on DX11!");      // TODO: D3DXSaveSurfaceToFile()
    }
}


void CHDRPostProcess::Begin()
{
    gcpRendD3D->GetModelViewMatrix(PostProcessUtils().m_pView.GetData());
    gcpRendD3D->GetProjectionMatrix(PostProcessUtils().m_pProj.GetData());

    // Store some commonly used per-frame data

    PostProcessUtils().m_pViewProj = PostProcessUtils().m_pView * PostProcessUtils().m_pProj;
    if (gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH)
    {
        PostProcessUtils().m_pViewProj = ReverseDepthHelper::Convert(PostProcessUtils().m_pViewProj);
    }

    PostProcessUtils().m_pViewProj.Transpose();

    m_shHDR = CShaderMan::s_shHDRPostProcess;

    if (CTexture::s_ptexHDRTarget->GetWidth() != gcpRendD3D->GetWidth() || CTexture::s_ptexHDRTarget->GetHeight() != gcpRendD3D->GetHeight())
    {
        CTexture::GenerateHDRMaps();
    }

    gcpRendD3D->FX_ResetPipe();
    PostProcessUtils().SetFillModeSolid(true);
}


void CHDRPostProcess::End()
{
    gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_HDR;
    gcpRendD3D->m_RP.m_PersFlags2 &= ~(RBPF2_HDR_FP16 | RBPF2_LIGHTSHAFTS);

    gcpRendD3D->FX_ResetPipe();

    PostProcessUtils().SetFillModeSolid(false);

    // (re-set back-buffer): if the platform does lazy RT updates/setting there's strong possibility we run into problems when we try to resolve with no RT set
    gcpRendD3D->FX_SetActiveRenderTargets();
}


void CHDRPostProcess::Render()
{
    PROFILE_LABEL_SCOPE("HDR_POSTPROCESS");

    Begin();

    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        CRY_ASSERT(gcpRendD3D->FX_GetCurrentRenderTarget(0) == CTexture::s_ptexHDRTarget);

        gcpRendD3D->FX_SetActiveRenderTargets(); // Called explicitly to work around RT stack problems on deprecated platform
        gcpRendD3D->RT_UnbindTMUs();// Avoid d3d error due to potential rtv still bound as shader input.
        gcpRendD3D->FX_PopRenderTarget(0);
        gcpRendD3D->EF_ClearTargetsLater(0);
    }

    // Skip hdr/post processing when rendering different camera views
    if ((gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_MIRRORCULL) || (gcpRendD3D->m_RP.m_nRendFlags & SHDF_CUBEMAPGEN))
    {
        End();
        return;
    }

#if !defined(_RELEASE) || defined(WIN32) || defined(WIN64) || defined(ENABLE_LW_PROFILERS)
    ScreenShot();
#endif

    gcpRendD3D->m_RP.m_FlagsShader_RT = 0;

    gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0]; // enable srgb. Can save this flag, always enabled

    gcpRendD3D->FX_ApplyShaderQuality(eST_PostProcess);
    m_bHiQuality = CPostEffectsMgr::CheckPostProcessQuality(eRQ_Low, eSQ_VeryHigh);

    static ICVar* DolbyCvar = gEnv->pConsole->GetCVar("r_HDRDolby");
    const int DolbyCvarValue = DolbyCvar ? DolbyCvar->GetIVal() : eDVM_Disabled;
    const bool bDolbyHDRMode = DolbyCvarValue > eDVM_Disabled;

    if (CRenderer::CV_r_PostProcess)
    {
        CStandardGraphicsPipeline& graphicsPipeline = gcpRendD3D->GetGraphicsPipeline();

        const bool bSolidModeEnabled = gcpRendD3D->GetWireframeMode() == R_SOLID_MODE;
        const bool bDepthOfFieldEnabled = CRenderer::CV_r_dof >= 1 && bSolidModeEnabled;
        const bool takingScreenShot = (gcpRendD3D->m_screenShotType != 0);
        const bool bMotionBlurEnabled = CRenderer::CV_r_MotionBlur && bSolidModeEnabled && (!takingScreenShot || CRenderer::CV_r_MotionBlurScreenShot) && !CRenderer::CV_r_RenderMotionBlurAfterHDR;

        DepthOfFieldParameters depthOfFieldParameters;

        if (bDepthOfFieldEnabled)
        {
            CDepthOfField* depthOfField = static_cast<CDepthOfField*>(PostEffectMgr()->GetEffect(ePFX_eDepthOfField));
            depthOfField->UpdateParameters();
            depthOfFieldParameters = depthOfField->GetParameters();
        }

        if (CRenderer::CV_r_AntialiasingMode == eAT_TAA)
        {
            CRY_ASSERT_MESSAGE(CRenderer::CV_r_ToneMapExposureType==static_cast<int>(Exposuretype::Auto), "TAA needs auto exposure");
            GetUtils().StretchRect(CTexture::s_ptexHDRTarget, CTexture::s_ptexSceneTarget);
            graphicsPipeline.RenderTemporalAA(CTexture::s_ptexSceneTarget, CTexture::s_ptexHDRTarget, depthOfFieldParameters);
        }

        // Rain
        CSceneRain* pSceneRain = static_cast<CSceneRain*>(PostEffectMgr()->GetEffect(ePFX_SceneRain));
        const SRainParams& rainInfo = gcpRendD3D->m_p3DEngineCommon.m_RainInfo;
        if (pSceneRain && pSceneRain->IsActive() && rainInfo.fRainDropsAmount > 0.01f)
        {
            pSceneRain->Render();
        }

        // Motion blur not enabled in 256bpp GMEM paths
        if (CD3D9Renderer::eGT_256bpp_PATH != gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
        {
            // Note: Motion blur uses s_ptexHDRTargetPrev to avoid doing another copy, so this should be right before the MB pass
            PostProcessUtils().StretchRect(CTexture::s_ptexHDRTarget, CTexture::s_ptexHDRTargetPrev, false, false, false, false, SPostEffectsUtils::eDepthDownsample_None, false, &gcpRendD3D->m_FullResRect);
        }

        if (bDepthOfFieldEnabled)
        {
            graphicsPipeline.RenderDepthOfField();
        }
                
        uint32 flags = SRendItem::BatchFlags(EFSLIST_TRANSP, gcpRendD3D->m_RP.m_pRLD);
        if (flags & FB_TRANSPARENT_AFTER_DOF)
        {
            PROFILE_LABEL_SCOPE("PARTICLES AFTER DOF");
            //render (after water) transparent particles list which was set to skip Depth of Field
            gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexHDRTarget, &gcpRendD3D->m_DepthBufferOrig);
            uint32 nBatchFilter = FB_TRANSPARENT_AFTER_DOF;
            gcpRendD3D->FX_ProcessRenderList(EFSLIST_TRANSP, 1, gcpRendD3D->FX_FlushShader_General, true, nBatchFilter);
            gcpRendD3D->FX_PopRenderTarget(0);
        }

        if (bMotionBlurEnabled)
        {
            // Added old pipeline render call here. This lets us do motion blur before the end of HDR processing.

            if (CRenderer::CV_r_GraphicsPipeline > 0)
            {
                graphicsPipeline.RenderMotionBlur();
            }
            else
            {
                CMotionBlur* motionBlurEffect = static_cast<CMotionBlur*>(PostEffectMgr()->GetEffect(ePFX_eMotionBlur));

                if (motionBlurEffect)
                {
                  motionBlurEffect->Render();
                }
            }
        }

        {
            CSceneSnow* pSceneSnow = static_cast<CSceneSnow*>(PostEffectMgr()->GetEffect(ePFX_SceneSnow));
            if (pSceneSnow->IsActiveSnow())
            {
                pSceneSnow->Render();
            }
        }

        //Render passes for auto exposure. Used for tonemapping or Bloom generation
        if (CRenderer::CV_r_ToneMapExposureType == static_cast<int>(Exposuretype::Auto) || CRenderer::CV_r_HDRBloom)
        {
            HalfResDownsampleHDRTarget();
            gcpRendD3D->SetCurDownscaleFactor(Vec2(1, 1));
            QuarterResDownsampleHDRTarget();
            
            gcpRendD3D->FX_ApplyShaderQuality(eST_PostProcess);
            
            // Update eye adaptation
            if (CRenderer::CV_r_EnableGMEMPostProcCS)
            {
                MeasureLumEyeAdaptationUsingCompute();
            }
            else
            {
                MeasureLuminance();
                EyeAdaptation();
            }
        }

        if (CRenderer::CV_r_HDRBloom)
        {
            BloomGeneration();
        }
    }

    gcpRendD3D->SetCurDownscaleFactor(gcpRendD3D->m_CurViewportScale);

    bool postAAWillApplyAA = (CRenderer::CV_r_AntialiasingMode == eAT_SMAA1TX) || (CRenderer::CV_r_AntialiasingMode == eAT_FXAA);
    bool shouldRenderToBackbufferNow = CRenderer::CV_r_SkipNativeUpscale && CRenderer::CV_r_SkipRenderComposites && !postAAWillApplyAA;
    if (shouldRenderToBackbufferNow)
    {
        gcpRendD3D->RT_SetViewport(0, 0, gcpRendD3D->GetNativeWidth(), gcpRendD3D->GetNativeHeight());
        gcpRendD3D->FX_SetRenderTarget(0, gcpRendD3D->GetBackBuffer(), nullptr);
        gcpRendD3D->FX_SetActiveRenderTargets();
    }
    else
    {
        gcpRendD3D->FX_PushRenderTarget(0, SPostEffectsUtils::AcquireFinalCompositeTarget(bDolbyHDRMode), &gcpRendD3D->m_DepthBufferOrigMSAA);
    }
    
    // Render final scene to the back buffer
    if (CRenderer::CV_r_HDRDebug != 1 && CRenderer::CV_r_HDRDebug != 2)
    {
        ToneMapping();
    }
    else
    {
        ToneMappingDebug();
    }

    if (CRenderer::CV_r_HDRDebug > 0)
    {
        DrawDebugViews();
    }

    End();
}


void CD3D9Renderer::FX_HDRPostProcessing()
{
    PROFILE_FRAME(Draw_HDR_PostProcessing);


    if (gcpRendD3D->m_bDeviceLost)
    {
        return;
    }

    if (!CTexture::IsTextureExist(CTexture::s_ptexHDRTarget))
    {
        return;
    }

    CHDRPostProcess* pHDRPostProcess = CHDRPostProcess::GetInstance();
    pHDRPostProcess->Render();
    pHDRPostProcess->ProcessLensOptics();
}


void CD3D9Renderer::FX_FinalComposite()
{
    CTexture* upscaleSource = SPostEffectsUtils::GetFinalCompositeTarget();

    if (FX_GetCurrentRenderTarget(0) == upscaleSource && upscaleSource != nullptr)
    {
        FX_PopRenderTarget(0);

        RT_SetViewport(0, 0, m_nativeWidth, m_nativeHeight);
        FX_SetRenderTarget(0, m_pBackBuffer, nullptr);
        FX_SetActiveRenderTargets();

        static ICVar* DolbyCvar = gEnv->pConsole->GetCVar("r_HDRDolby");
        int DolbyCvarValue = DolbyCvar ? DolbyCvar->GetIVal() : eDVM_Disabled;

        if (DolbyCvarValue == eDVM_Vision)
        {
            CHDRPostProcess::GetInstance()->EncodeDolbyVision(upscaleSource);
        }
        else
        {
            GetGraphicsPipeline().RenderFinalComposite(upscaleSource);
        }
    }
}
