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
#include "I3DEngine.h"
#include "D3DPostProcess.h"
#include "Common/RenderCapabilities.h"
#include "../Common/Textures/TextureManager.h"
#include "GraphicsPipeline/FurPasses.h"

#include <AzCore/NativeUI/NativeUIRequests.h>

#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(D3DDeferredRender_cpp)
#endif


bool CD3D9Renderer::FX_DeferredShadowPassSetupBlend(const Matrix44& mShadowTexGen, int nFrustumNum, float maskRTWidth, float maskRTHeight)
{
    //set ScreenToWorld Expansion Basis
    Vec4r vWBasisX, vWBasisY, vWBasisZ, vCamPos;
    CShadowUtils::ProjectScreenToWorldExpansionBasis(mShadowTexGen, GetCamera(), Vec2(m_TemporalJitterClipSpace.x, m_TemporalJitterClipSpace.y), maskRTWidth, maskRTHeight, vWBasisX, vWBasisY, vWBasisZ, vCamPos, true, &m_RenderTileInfo);

    Matrix44A* mat = &gRenDev->m_TempMatrices[nFrustumNum][2];
    mat->SetRow4(0, Vec4r(vWBasisX.x, vWBasisY.x, vWBasisZ.x, vCamPos.x));
    mat->SetRow4(1, Vec4r(vWBasisX.y, vWBasisY.y, vWBasisZ.y, vCamPos.y));
    mat->SetRow4(2, Vec4r(vWBasisX.z, vWBasisY.z, vWBasisZ.z, vCamPos.z));
    mat->SetRow4(3, Vec4r(vWBasisX.w, vWBasisY.w, vWBasisZ.w, vCamPos.w));
    
    return true;
}

bool CD3D9Renderer::FX_DeferredShadowPassSetup(const Matrix44& mShadowTexGen, [[maybe_unused]] ShadowMapFrustum* pShadowFrustum, float maskRTWidth, float maskRTHeight, Matrix44& mScreenToShadow, bool bNearest)
{
    //set ScreenToWorld Expansion Basis
    Vec4r vWBasisX, vWBasisY, vWBasisZ, vCamPos;
    bool bVPosSM30 = (GetFeatures() & (RFT_HW_SM30 | RFT_HW_SM40)) != 0;

    CCamera Cam = GetCamera();
    if (bNearest && m_drawNearFov > 1.0f && m_drawNearFov < 179.0f)
    {
        Cam.SetFrustum(Cam.GetViewSurfaceX(), Cam.GetViewSurfaceZ(), DEG2RAD(m_drawNearFov), Cam.GetNearPlane(), Cam.GetFarPlane(), Cam.GetPixelAspectRatio());
    }

    CShadowUtils::ProjectScreenToWorldExpansionBasis(mShadowTexGen, Cam, Vec2(m_TemporalJitterClipSpace.x, m_TemporalJitterClipSpace.y), maskRTWidth, maskRTHeight, vWBasisX, vWBasisY, vWBasisZ, vCamPos, bVPosSM30, &m_RenderTileInfo);

    //TOFIX: create PB components for these params
    //creating common projection matrix for depth reconstruction

    mScreenToShadow = Matrix44(vWBasisX.x, vWBasisX.y, vWBasisX.z, vWBasisX.w,
            vWBasisY.x, vWBasisY.y, vWBasisY.z, vWBasisY.w,
            vWBasisZ.x, vWBasisZ.y, vWBasisZ.z, vWBasisZ.w,
            vCamPos.x, vCamPos.y, vCamPos.z, vCamPos.w);

    //save magnitudes separately to inrease precision
    m_cEF.m_TempVecs[14].x = vWBasisX.GetLength();
    m_cEF.m_TempVecs[14].y = vWBasisY.GetLength();
    m_cEF.m_TempVecs[14].z = vWBasisZ.GetLength();
    m_cEF.m_TempVecs[14].w = 1.0f;

    //Vec4r normalization in doubles
    vWBasisX /= vWBasisX.GetLength();
    vWBasisY /= vWBasisY.GetLength();
    vWBasisZ /= vWBasisZ.GetLength();

    m_cEF.m_TempVecs[10].x = vWBasisX.x;
    m_cEF.m_TempVecs[10].y = vWBasisX.y;
    m_cEF.m_TempVecs[10].z = vWBasisX.z;
    m_cEF.m_TempVecs[10].w = vWBasisX.w;

    m_cEF.m_TempVecs[11].x = vWBasisY.x;
    m_cEF.m_TempVecs[11].y = vWBasisY.y;
    m_cEF.m_TempVecs[11].z = vWBasisY.z;
    m_cEF.m_TempVecs[11].w = vWBasisY.w;

    m_cEF.m_TempVecs[12].x = vWBasisZ.x;
    m_cEF.m_TempVecs[12].y = vWBasisZ.y;
    m_cEF.m_TempVecs[12].z = vWBasisZ.z;
    m_cEF.m_TempVecs[12].w = vWBasisZ.w;

    m_cEF.m_TempVecs[13].x = CV_r_ShadowsAdaptionRangeClamp;
    m_cEF.m_TempVecs[13].y = CV_r_ShadowsAdaptionSize * 250.f;//to prevent akwardy high number in cvar
    m_cEF.m_TempVecs[13].z = CV_r_ShadowsAdaptionMin;

    // Particles shadow constants
    if (m_RP.m_nPassGroupID == EFSLIST_TRANSP || m_RP.m_nPassGroupID ==  EFSLIST_HALFRES_PARTICLES)
    {
        m_cEF.m_TempVecs[13].x = CV_r_ShadowsParticleKernelSize;
        m_cEF.m_TempVecs[13].y = CV_r_ShadowsParticleJitterAmount;
        m_cEF.m_TempVecs[13].z = CV_r_ShadowsParticleAnimJitterAmount * 0.05f;
        m_cEF.m_TempVecs[13].w = CV_r_ShadowsParticleNormalEffect;
    }

    m_cEF.m_TempVecs[0].x =  vCamPos.x;
    m_cEF.m_TempVecs[0].y =  vCamPos.y;
    m_cEF.m_TempVecs[0].z =  vCamPos.z;
    m_cEF.m_TempVecs[0].w =  vCamPos.w;

    return true;
}


HRESULT GetSampleOffsetsGaussBlur5x5Bilinear(DWORD dwD3DTexWidth, DWORD dwD3DTexHeight, Vec4* avTexCoordOffset, Vec4* avSampleWeight, FLOAT fMultiplier)
{
    float tu = 1.0f / (float)dwD3DTexWidth;
    float tv = 1.0f / (float)dwD3DTexHeight;
    float totalWeight = 0.0f;
    Vec4 vWhite(1.f, 1.f, 1.f, 1.f);
    float fWeights[6];

    int index = 0;
    for (int x = -2; x <= 2; x++, index++)
    {
        fWeights[index] = PostProcessUtils().GaussianDistribution2D((float)x, 0.f, 4);
    }

    //  compute weights for the 2x2 taps.  only 9 bilinear taps are required to sample the entire area.
    index = 0;
    for (int y = -2; y <= 2; y += 2)
    {
        float tScale = (y == 2) ? fWeights[4] : (fWeights[y + 2] + fWeights[y + 3]);
        float tFrac  = fWeights[y + 2] / tScale;
        float tOfs   = ((float)y + (1.f - tFrac)) * tv;
        for (int x = -2; x <= 2; x += 2, index++)
        {
            float sScale = (x == 2) ? fWeights[4] : (fWeights[x + 2] + fWeights[x + 3]);
            float sFrac  = fWeights[x + 2] / sScale;
            float sOfs   = ((float)x + (1.f - sFrac)) * tu;
            avTexCoordOffset[index] = Vec4(sOfs, tOfs, 0, 1);
            avSampleWeight[index]   = vWhite * sScale * tScale;
            totalWeight += sScale * tScale;
        }
    }


    for (int i = 0; i < index; i++)
    {
        avSampleWeight[i] *= (fMultiplier / totalWeight);
    }

    return S_OK;
}

int CRenderer::FX_ApplyShadowQuality()
{
    SShaderProfile* pSP = &m_cEF.m_ShaderProfiles[eST_Shadow];
    const uint64 quality    = g_HWSR_MaskBit[HWSR_QUALITY];
    const uint64 quality1   = g_HWSR_MaskBit[HWSR_QUALITY1];
    m_RP.m_FlagsShader_RT &= ~(quality | quality1);

    int nQuality = (int)pSP->GetShaderQuality();
    m_RP.m_nShaderQuality = nQuality;
    switch (nQuality)
    {
    case eSQ_Medium:
        m_RP.m_FlagsShader_RT |= quality;
        break;
    case eSQ_High:
        m_RP.m_FlagsShader_RT |= quality1;
        break;
    case eSQ_VeryHigh:
        m_RP.m_FlagsShader_RT |= quality;
        m_RP.m_FlagsShader_RT |= quality1;
        break;
    }
    return nQuality;
}

void CD3D9Renderer::FX_StateRestore([[maybe_unused]] int prevState)
{
}

//setup pass offset
// Draw a fullscreen quad to sample the RT
//EF_Commit() is called here
//////////////////////////////////////////////////////////////////////////
// X Blur
// Draw a fullscreen quad to sample the RT
void CD3D9Renderer::FX_StencilTestCurRef(bool bEnable, [[maybe_unused]] bool bNoStencilClear, bool bStFuncEqual)
{
    if (bEnable)
    {
        int nStencilState =
            STENC_FUNC(bStFuncEqual ? FSS_STENCFUNC_EQUAL : FSS_STENCFUNC_NOTEQUAL) |
            STENCOP_FAIL(FSS_STENCOP_KEEP) |
            STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
            STENCOP_PASS(FSS_STENCOP_KEEP);

        FX_SetStencilState(nStencilState, m_nStencilMaskRef, 0xFFFFFFFF, 0xFFFFFFFF);
        FX_SetState(m_RP.m_CurState | GS_STENCIL);
    }
    else
    {
    }
}

void CD3D9Renderer::FX_DeferredShadowPass([[maybe_unused]] const SRenderLight* pLight, ShadowMapFrustum* pShadowFrustum, bool bShadowPass, bool bCloudShadowPass, bool bStencilPrepass, int nLod)
{
    uint32 nPassCount = 0;
    CShader*  pShader = CShaderMan::s_ShaderShadowMaskGen;
    static CCryNameTSCRC DeferredShadowTechName = "DeferredShadowPass";

    D3DSetCull(eCULL_Back, true); //fs quads should not revert test..

    if (pShadowFrustum->m_eFrustumType != ShadowMapFrustum::e_Nearest && !bCloudShadowPass)
    {
        if (pShadowFrustum->bUseShadowsPool || pShadowFrustum->pDepthTex == NULL)
        {
            return;
        }
    }

    if (pShadowFrustum->m_eFrustumType == ShadowMapFrustum::e_HeightMapAO)
    {
        return;
    }

    int nShadowQuality = FX_ApplyShadowQuality();

    //////////////////////////////////////////////////////////////////////////
    // set global shader RT flags
    //////////////////////////////////////////////////////////////////////////

    // set pass dependent RT flags
    m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[ HWSR_SAMPLE0 ] | g_HWSR_MaskBit[ HWSR_SAMPLE1 ] | g_HWSR_MaskBit[ HWSR_SAMPLE2 ] | g_HWSR_MaskBit[ HWSR_SAMPLE4 ] |
                               g_HWSR_MaskBit[HWSR_CUBEMAP0] | g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ]  | g_HWSR_MaskBit[ HWSR_POINT_LIGHT ] |
                               g_HWSR_MaskBit[ HWSR_SHADOW_MIXED_MAP_G16R16 ] | g_HWSR_MaskBit[HWSR_SHADOW_JITTERING] | g_HWSR_MaskBit[HWSR_NEAREST]);

    if (!pShadowFrustum->bBlendFrustum)
    {
        m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[ HWSR_SAMPLE3 ];
    }

    if (m_shadowJittering > 0.0f)
    {
        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_SHADOW_JITTERING ];
    }

    //enable hw-pcf per frustum
    if (pShadowFrustum->bHWPCFCompare)
    {
        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ];
    }

    if (pShadowFrustum->m_eFrustumType == ShadowMapFrustum::e_Nearest)
    {
        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_NEAREST];
    }

    if (bCloudShadowPass || (CV_r_ShadowsScreenSpace && bShadowPass && pShadowFrustum->nShadowMapLod == 0))
    {
        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_SAMPLE2 ];
    }

    ConfigShadowTexgen(0, pShadowFrustum);
    if (nShadowQuality == eSQ_VeryHigh) //DX10 only
    {
        ConfigShadowTexgen(1, pShadowFrustum, -1, false, false);
        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_SAMPLE1 ];
    }

    int newState = m_RP.m_CurState;
    newState &= ~(GS_DEPTHWRITE | GS_BLSRC_ONE | GS_BLDST_ONE | GS_BLOP_MAX);
    newState |= GS_NODEPTHTEST;

    if (pShadowFrustum->m_eFrustumType == ShadowMapFrustum::e_Nearest)
    {
        newState &= ~(GS_NODEPTHTEST | GS_DEPTHFUNC_MASK);
        newState |= GS_DEPTHFUNC_GREAT;
    }

    // In GMEM, we do our own clouds shadow blending
    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr) ||
        (gcpRendD3D->FX_GetEnabledGmemPath(nullptr) && !bCloudShadowPass))
    {
        if (pShadowFrustum->bUseAdditiveBlending)
        {
            newState |= GS_BLSRC_ONE | GS_BLDST_ONE | GS_BLOP_MAX;
        }
        else if (bShadowPass && pShadowFrustum->bBlendFrustum)
        {
            newState |= GS_BLSRC_ONE | GS_BLDST_ONE;
        }
    }

    pShader->FXSetTechnique(DeferredShadowTechName);
    pShader->FXBegin(&nPassCount, FEF_DONTSETSTATES);


    //////////////////////////////////////////////////////////////////////////
    //Stencil cull pre-pass for GSM
    //////////////////////////////////////////////////////////////////////////
    if (bStencilPrepass)
    {
        newState |= GS_STENCIL;
        //Disable color writes
        newState |= GS_COLMASK_NONE;

        FX_SetState(newState);
        //////////////////////////////////////////////////////////////////////////
        
        //render clip volume
        Matrix44 mViewProj = pShadowFrustum->mLightViewMatrix;
        Matrix44 mViewProjInv = mViewProj.GetInverted();
        gRenDev->m_TempMatrices[0][0] = mViewProjInv.GetTransposed();
        
        FX_SetVStream(0, m_pUnitFrustumVB[SHAPE_SIMPLE_PROJECTOR], 0, sizeof(SVF_P3F_C4B_T2F));
        FX_SetIStream(m_pUnitFrustumIB[SHAPE_SIMPLE_PROJECTOR], 0, (kUnitObjectIndexSizeof == 2 ? Index16 : Index32));
            
        if (!CV_r_ShadowsUseClipVolume || !RenderCapabilities::SupportsDepthClipping())
        {
            FX_StencilCullPass(nLod, m_UnitFrustVBSize[SHAPE_SIMPLE_PROJECTOR], m_UnitFrustIBSize[SHAPE_SIMPLE_PROJECTOR], pShader, DS_STENCIL_VOLUME_CLIP, DS_STENCIL_VOLUME_CLIP_FRONTFACING);
        }
        else
        {
            FX_StencilCullPass(nLod, m_UnitFrustVBSize[SHAPE_SIMPLE_PROJECTOR], m_UnitFrustIBSize[SHAPE_SIMPLE_PROJECTOR], pShader, DS_STENCIL_VOLUME_CLIP);
        }
                
        // camera might be outside cached frustum => do front facing pass as well
        if (pShadowFrustum->IsCached())
        {
            Vec4 vCamPosShadowSpace = Vec4(GetViewParameters().vOrigin, 1.f) * mViewProj;
            vCamPosShadowSpace /= vCamPosShadowSpace.w;                
            if (abs(vCamPosShadowSpace.x) > 1.0f || abs(vCamPosShadowSpace.y) > 1.0f || vCamPosShadowSpace.z < 0 || vCamPosShadowSpace.z > 1)
            {
                pShader->FXBeginPass(DS_STENCIL_VOLUME_CLIP);
                if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
                {
                    D3DSetCull(eCULL_Back);
                    FX_SetStencilState(
                                       STENC_FUNC(FSS_STENCFUNC_ALWAYS) |
                                       STENCOP_FAIL(FSS_STENCOP_KEEP) |
                                       STENCOP_ZFAIL(FSS_STENCOP_ZERO) |
                                       STENCOP_PASS(FSS_STENCOP_KEEP),
                                       nLod, 0xFFFFFFFF, 0xFFFF
                                       );
                    
                    FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, m_UnitFrustVBSize[SHAPE_SIMPLE_PROJECTOR], 0, m_UnitFrustIBSize[SHAPE_SIMPLE_PROJECTOR]);
                }
                pShader->FXEndPass();
            }
        }
        
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Shadow Pass
    //////////////////////////////////////////////////////////////////////////

    if (bShadowPass)
    {
        newState &= ~(GS_COLMASK_NONE | GS_STENCIL);
       
        // When optimizations are on, we need only the R channel.
        if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr) && (CRenderer::CV_r_DeferredShadingLBuffersFmt != 2)) // A component write mask of GMEM light diffuse RT
        {
            newState |= GS_NOCOLMASK_R | GS_NOCOLMASK_G | GS_NOCOLMASK_B;
        }
        else
        {
            newState |= GS_NOCOLMASK_G | GS_NOCOLMASK_B | GS_NOCOLMASK_A;
        }

        if (nLod != 0 && !bCloudShadowPass)
        {
            newState |= GS_STENCIL;

            FX_SetStencilState(
                STENC_FUNC(FSS_STENCFUNC_EQUAL) |
                STENCOP_FAIL(FSS_STENCOP_KEEP) |
                STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
                STENCOP_PASS(FSS_STENCOP_KEEP),
                nLod, 0xFFFFFFFF, 0xFFFFFFFF
                );
        }

        FX_SetState(newState);

        pShader->FXBeginPass(bCloudShadowPass ? DS_CLOUDS_SEPARATE : DS_SHADOW_PASS);

        const float fCustomZ = pShadowFrustum->m_eFrustumType == ShadowMapFrustum::e_Nearest ? CV_r_DrawNearZRange - 0.001f : 0.f;
        PostProcessUtils().DrawFullScreenTriWPOS(0, 0, fCustomZ);

        pShader->FXEndPass();
    }
    pShader->FXEnd();
}


#define LOCAL_SAFE_RELEASE(buffer)                       \
    if (buffer)                                          \
    {                                                    \
        gcpRendD3D->m_DevMan.ReleaseD3D11Buffer(buffer); \
        buffer = nullptr;                                \
    }

bool CD3D9Renderer::CreateAuxiliaryMeshes()
{
    t_arrDeferredMeshIndBuff arrDeferredInds;
    t_arrDeferredMeshVertBuff arrDeferredVerts;

    uint nProjectorMeshStep = 10;

    //projector frustum mesh
    for (int i = 0; i < 3; i++)
    {
        uint nFrustTess = 11  + nProjectorMeshStep * i;
        CDeferredRenderUtils::CreateUnitFrustumMesh(nFrustTess, nFrustTess, arrDeferredInds, arrDeferredVerts);
        LOCAL_SAFE_RELEASE(m_pUnitFrustumVB[SHAPE_PROJECTOR + i]);
        LOCAL_SAFE_RELEASE(m_pUnitFrustumIB[SHAPE_PROJECTOR + i]);
        COMPILE_TIME_ASSERT(kUnitObjectIndexSizeof == sizeof(arrDeferredInds[0]));
        CreateUnitVolumeMesh(arrDeferredInds, arrDeferredVerts, m_pUnitFrustumIB[SHAPE_PROJECTOR + i], m_pUnitFrustumVB[SHAPE_PROJECTOR + i]);
        m_UnitFrustVBSize[SHAPE_PROJECTOR + i] = arrDeferredVerts.size();
        m_UnitFrustIBSize[SHAPE_PROJECTOR + i] = arrDeferredInds.size();
    }

    //clip-projector frustum mesh
    for (int i = 0; i < 3; i++)
    {
        uint nClipFrustTess = 41  + nProjectorMeshStep * i;
        CDeferredRenderUtils::CreateUnitFrustumMesh(nClipFrustTess, nClipFrustTess, arrDeferredInds, arrDeferredVerts);
        LOCAL_SAFE_RELEASE(m_pUnitFrustumVB[SHAPE_CLIP_PROJECTOR + i]);
        LOCAL_SAFE_RELEASE(m_pUnitFrustumIB[SHAPE_CLIP_PROJECTOR + i]);
        COMPILE_TIME_ASSERT(kUnitObjectIndexSizeof == sizeof(arrDeferredInds[0]));
        CreateUnitVolumeMesh(arrDeferredInds, arrDeferredVerts, m_pUnitFrustumIB[SHAPE_CLIP_PROJECTOR + i], m_pUnitFrustumVB[SHAPE_CLIP_PROJECTOR + i]);
        m_UnitFrustVBSize[SHAPE_CLIP_PROJECTOR + i] = arrDeferredVerts.size();
        m_UnitFrustIBSize[SHAPE_CLIP_PROJECTOR + i] = arrDeferredInds.size();
    }

    //omni-light mesh
    //Use tess3 for big lights
    CDeferredRenderUtils::CreateUnitSphere(2, arrDeferredInds, arrDeferredVerts);
    LOCAL_SAFE_RELEASE(m_pUnitFrustumVB[SHAPE_SPHERE]);
    LOCAL_SAFE_RELEASE(m_pUnitFrustumIB[SHAPE_SPHERE]);
    COMPILE_TIME_ASSERT(kUnitObjectIndexSizeof == sizeof(arrDeferredInds[0]));
    CreateUnitVolumeMesh(arrDeferredInds, arrDeferredVerts, m_pUnitFrustumIB[SHAPE_SPHERE], m_pUnitFrustumVB[SHAPE_SPHERE]);
    m_UnitFrustVBSize[SHAPE_SPHERE] = arrDeferredVerts.size();
    m_UnitFrustIBSize[SHAPE_SPHERE] = arrDeferredInds.size();

    //unit box
    CDeferredRenderUtils::CreateUnitBox(arrDeferredInds, arrDeferredVerts);
    LOCAL_SAFE_RELEASE(m_pUnitFrustumVB[SHAPE_BOX]);
    LOCAL_SAFE_RELEASE(m_pUnitFrustumIB[SHAPE_BOX]);
    COMPILE_TIME_ASSERT(kUnitObjectIndexSizeof == sizeof(arrDeferredInds[0]));
    CreateUnitVolumeMesh(arrDeferredInds, arrDeferredVerts, m_pUnitFrustumIB[SHAPE_BOX], m_pUnitFrustumVB[SHAPE_BOX]);
    m_UnitFrustVBSize[SHAPE_BOX] = arrDeferredVerts.size();
    m_UnitFrustIBSize[SHAPE_BOX] = arrDeferredInds.size();

    //frustum approximated with 8 vertices
    CDeferredRenderUtils::CreateSimpleLightFrustumMesh(arrDeferredInds, arrDeferredVerts);
    LOCAL_SAFE_RELEASE(m_pUnitFrustumVB[SHAPE_SIMPLE_PROJECTOR]);
    LOCAL_SAFE_RELEASE(m_pUnitFrustumIB[SHAPE_SIMPLE_PROJECTOR]);
    COMPILE_TIME_ASSERT(kUnitObjectIndexSizeof == sizeof(arrDeferredInds[0]));
    CreateUnitVolumeMesh(arrDeferredInds, arrDeferredVerts, m_pUnitFrustumIB[SHAPE_SIMPLE_PROJECTOR], m_pUnitFrustumVB[SHAPE_SIMPLE_PROJECTOR]);
    m_UnitFrustVBSize[SHAPE_SIMPLE_PROJECTOR] = arrDeferredVerts.size();
    m_UnitFrustIBSize[SHAPE_SIMPLE_PROJECTOR] = arrDeferredInds.size();

    // FS quad
    CDeferredRenderUtils::CreateQuad(arrDeferredInds, arrDeferredVerts);
    LOCAL_SAFE_RELEASE(m_pQuadVB);
    D3DBuffer* pDummyQuadIB = 0; // reusing CreateUnitVolumeMesh.
    CreateUnitVolumeMesh(arrDeferredInds, arrDeferredVerts, pDummyQuadIB, m_pQuadVB);
    m_nQuadVBSize = arrDeferredVerts.size();

    return true;
}

bool CD3D9Renderer::ReleaseAuxiliaryMeshes()
{
    for (int i = 0; i < SHAPE_MAX; i++)
    {
        LOCAL_SAFE_RELEASE(m_pUnitFrustumVB[i]);
        LOCAL_SAFE_RELEASE(m_pUnitFrustumIB[i]);
    }

    LOCAL_SAFE_RELEASE(m_pQuadVB);
    m_nQuadVBSize = 0;

    return true;
}

bool CD3D9Renderer::CreateUnitVolumeMesh(t_arrDeferredMeshIndBuff& arrDeferredInds, t_arrDeferredMeshVertBuff& arrDeferredVerts, D3DBuffer*& pUnitFrustumIB, D3DBuffer*& pUnitFrustumVB)
{
    HRESULT hr = S_OK;

    //FIX: try default pools

    D3D11_BUFFER_DESC BufDesc;
    ZeroStruct(BufDesc);
    D3D11_SUBRESOURCE_DATA SubResData;
    ZeroStruct(SubResData);

    if (!arrDeferredVerts.empty())
    {
        BufDesc.ByteWidth = arrDeferredVerts.size() * sizeof(SDeferMeshVert);
        BufDesc.Usage = D3D11_USAGE_IMMUTABLE;
        BufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        BufDesc.CPUAccessFlags = 0;
        BufDesc.MiscFlags = 0;

        SubResData.pSysMem = &arrDeferredVerts[0];
        SubResData.SysMemPitch = 0;
        SubResData.SysMemSlicePitch = 0;

        hr = gcpRendD3D->m_DevMan.CreateD3D11Buffer(&BufDesc, &SubResData, (ID3D11Buffer**)&pUnitFrustumVB, "UnitVolumeMesh");
        assert(SUCCEEDED(hr));
    }

    if (!arrDeferredInds.empty())
    {
        ZeroStruct(BufDesc);
        BufDesc.ByteWidth = arrDeferredInds.size() * sizeof(arrDeferredInds[0]);
        BufDesc.Usage = D3D11_USAGE_IMMUTABLE;
        BufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        BufDesc.CPUAccessFlags = 0;
        BufDesc.MiscFlags = 0;

        ZeroStruct(SubResData);
        SubResData.pSysMem = &arrDeferredInds[0];
        SubResData.SysMemPitch = 0;
        SubResData.SysMemSlicePitch = 0;

        hr = gcpRendD3D->m_DevMan.CreateD3D11Buffer(&BufDesc, &SubResData, &pUnitFrustumIB, "UnitVolumeMesh");
        assert(SUCCEEDED(hr));
    }

    return SUCCEEDED(hr);
}

void CD3D9Renderer::SetBackFacingStencillState(int nStencilID)
{
    int newState = m_RP.m_CurState;
    
    //Set LS colormask
    //debug states
    if IsCVarConstAccess(constexpr) (CV_r_DebugLightVolumes /*&& m_RP.m_TI.m_PersFlags2 & d3dRBPF2_ENABLESTENCILPB*/)
    {
        newState &= ~GS_COLMASK_NONE;
        newState &= ~GS_NODEPTHTEST;
        //newState |= GS_NODEPTHTEST;
        newState |= GS_DEPTHWRITE;
        newState |= (0xFFFFFFF0 << GS_COLMASK_SHIFT) & GS_COLMASK_MASK;
        if IsCVarConstAccess(constexpr) (CV_r_DebugLightVolumes > 1)
        {
            newState |= GS_WIREFRAME;
        }
    }
    else
    {
        //  //Disable color writes
        //if (CV_r_ShadowsStencilPrePass == 2)
        //{
        //  newState &= ~GS_COLMASK_NONE;
        //  newState |= GS_COLMASK_A;
        //}
        //else
        {
            newState |= GS_COLMASK_NONE;
        }


        //setup depth test and enable stencil
        newState &= ~(GS_NODEPTHTEST | GS_DEPTHWRITE | GS_DEPTHFUNC_MASK);
        newState |= GS_DEPTHFUNC_LEQUAL | GS_STENCIL;
    }

    //////////////////////////////////////////////////////////////////////////
    //draw back faces - inc when depth fail. 
    //////////////////////////////////////////////////////////////////////////
    int stencilFunc = FSS_STENCFUNC_ALWAYS;
    uint32 nCurrRef = 0;
    if (nStencilID >= 0)
    {
        D3DSetCull(eCULL_Front);

        //if (nStencilID>0)
        stencilFunc = m_RP.m_CurStencilCullFunc;

        FX_SetStencilState(
            STENC_FUNC(stencilFunc) |
            STENCOP_FAIL(FSS_STENCOP_KEEP) |
            STENCOP_ZFAIL(FSS_STENCOP_REPLACE) |
            STENCOP_PASS(FSS_STENCOP_KEEP),
            nStencilID, 0xFFFFFFFF, 0xFFFF
            );
        //    uint32 nStencilWriteMask = 1 << nStencilID; //0..7
        //    FX_SetStencilState(
        //      STENC_FUNC(FSS_STENCFUNC_ALWAYS) |
        //      STENCOP_FAIL(FSS_STENCOP_KEEP) |
        //      STENCOP_ZFAIL(FSS_STENCOP_REPLACE) |
        //      STENCOP_PASS(FSS_STENCOP_KEEP),
        //      0xFF, 0xFFFFFFFF, nStencilWriteMask
        //      );
    }
    else if (nStencilID == -4) // set all pixels with nCurRef within clip volume to nCurRef-1
    {
        stencilFunc = FSS_STENCFUNC_EQUAL;

        nCurrRef = m_nStencilMaskRef;

        FX_SetStencilState(
            STENC_FUNC(stencilFunc) |
            STENCOP_FAIL(FSS_STENCOP_KEEP) |
            STENCOP_ZFAIL(FSS_STENCOP_DECR) |
            STENCOP_PASS(FSS_STENCOP_KEEP),
            nCurrRef, 0xFFFFFFFF, 0xFFFF
            );

        m_nStencilMaskRef--;
        D3DSetCull(eCULL_Front);
    }
    else
    {
        if (nStencilID == -3) //TD: Fill stencil by values=1 for drawn volumes in order to avoid overdraw
        {
            stencilFunc = FSS_STENCFUNC_LEQUAL;

            m_nStencilMaskRef--;
        }
        else if (nStencilID == -2)
        {
            stencilFunc = FSS_STENCFUNC_GEQUAL;
            m_nStencilMaskRef--;
        }
        else
        {
            stencilFunc = FSS_STENCFUNC_GEQUAL;
            m_nStencilMaskRef++;
            //m_nStencilMaskRef = m_nStencilMaskRef%STENC_MAX_REF + m_nStencilMaskRef/STENC_MAX_REF;
            if (m_nStencilMaskRef > STENC_MAX_REF)
            {
                int sX = 0, sY = 0, sWidth = 0, sHeight = 0;
                bool bScissorEnabled = EF_GetScissorState(sX, sY, sWidth, sHeight);
                EF_Scissor(false, 0, 0, 0, 0);

                if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr)) // We must clear via full pass or else it'll kick buffers off GMEM
                {
                    uint32 prevState = m_RP.m_CurState;
                    uint32 state = 0;
                    state |= GS_COLMASK_NONE;
                    state |= GS_STENCIL;
                    FX_SetStencilState(
                        STENC_FUNC(FSS_STENCFUNC_ALWAYS) |
                        STENCOP_FAIL(FSS_STENCOP_ZERO) |
                        STENCOP_ZFAIL(FSS_STENCOP_ZERO) |
                        STENCOP_PASS(FSS_STENCOP_ZERO),
                        0, 0xFFFFFFFF, 0xFFFF);
                    FX_SetState(state);
                    SD3DPostEffectsUtils::ClearScreen(0, 0, 0, 0);
                    FX_SetState(prevState);
                }
                else
                {
                    EF_ClearTargetsImmediately(FRT_CLEAR_STENCIL, Clr_Unused.r, 1);
                }

                EF_Scissor(bScissorEnabled, sX, sY, sWidth, sHeight);
                m_nStencilMaskRef = 2;
            }
        }

        nCurrRef = m_nStencilMaskRef;
        assert(m_nStencilMaskRef > 0 && m_nStencilMaskRef <= STENC_MAX_REF);

        D3DSetCull(eCULL_Front);
        FX_SetStencilState(
            STENC_FUNC(stencilFunc) |
            STENCOP_FAIL(FSS_STENCOP_KEEP) |
            STENCOP_ZFAIL(FSS_STENCOP_REPLACE) |
            STENCOP_PASS(FSS_STENCOP_KEEP),
            nCurrRef, 0xFFFFFFFF, 0xFFFF
            );
    }

    FX_SetState(newState);    
    FX_Commit();
}

void CD3D9Renderer::SetFrontFacingStencillState(int nStencilID)
{    
    if (nStencilID < 0)
    {        
        D3DSetCull(eCULL_Back);        
        //TD: deferred meshed should have proper front facing on dx10
        int currentStencilFunction = m_RP.m_CurStencilState & FSS_STENCFUNC_MASK;
        FX_SetStencilState(
            STENC_FUNC(currentStencilFunction) |
            STENCOP_FAIL(FSS_STENCOP_KEEP) |
            STENCOP_ZFAIL(FSS_STENCOP_ZERO) |
            STENCOP_PASS(FSS_STENCOP_KEEP),
            m_nStencilMaskRef, 0xFFFFFFFF, 0xFFFF
        );

        FX_SetState(m_RP.m_CurState);
        FX_Commit();
    }   
}

//This version of the function uses two different passes and clamps the back facing triangles to the far plane in the vertex shader. 
//Use this when you dont have support for DepthClipEnable. Also, this setup assumes reverse depth. 
void CD3D9Renderer::FX_StencilCullPass(int nStencilID, int nNumVers, int nNumInds, CShader*  pShader, int backFacePass, int frontFacePass )
{
    //Render pass for back facing triangles
    pShader->FXBeginPass(backFacePass);
    
    //We can only check for vertexDeclaration after setting the pass. We have techniques with
    //multiple passes that can use different input layout. This way we ensure we are matching against
    //the correct input layout of the correct pass
    if (FAILED(FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
    {
        AZ_Assert(false, "Skipping the draw inside FX_StencilCullPass as the vertex declaration for shader %s pass %i failed", pShader->m_NameShader.c_str(), backFacePass);
        pShader->FXEndPass();
        return;
    }
    SetBackFacingStencillState(nStencilID);
    FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, nNumVers, 0, nNumInds);       
    pShader->FXEndPass();
   
    //Render pass for front facing triangles
    pShader->FXBeginPass(frontFacePass);
    SetFrontFacingStencillState(nStencilID);
    //skip front faces when nStencilID is specified
    if (nStencilID < 0)
    {
        FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, nNumVers, 0, nNumInds);
    }
    pShader->FXEndPass();    

    return;
}


//This version of the function assumes we have support for DepthClipEnable. Assumes reverse depth. 
void CD3D9Renderer::FX_StencilCullPass(int nStencilID, int nNumVers, int nNumInds, CShader*  pShader, int backFacePass)
{
    //Render pass for back facing triangles
    pShader->FXBeginPass(backFacePass);
    
    //We can only check for vertexDeclaration after setting the pass. We have techniques with
    //multiple passes that can use different input layout. This way we ensure we are matching against
    //the correct input layout of the correct pass
    if (FAILED(FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
    {
        //AZ_Assert(false, "Skipping the draw inside FX_StencilCullPass as the vertex declaration for shader %s pass %i failed", pShader->m_NameShader.c_str(), backFacePass);
        pShader->FXEndPass();
        return;
    }
    
    SetBackFacingStencillState(nStencilID);

    // Don't clip pixels beyond far clip plane
    SStateRaster PreviousRS = m_StatesRS[m_nCurStateRS];
    SStateRaster NoDepthClipRS = PreviousRS;
    NoDepthClipRS.Desc.DepthClipEnable = false;
    SetRasterState(&NoDepthClipRS);

    FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, nNumVers, 0, nNumInds);   
    SetRasterState(&PreviousRS);

    //Render pass for front facing triangles
    SetFrontFacingStencillState(nStencilID);
    //skip front faces when nStencilID is specified
    if (nStencilID < 0)
    {
        FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, nNumVers, 0, nNumInds);
    }
    pShader->FXEndPass();
    return;
}

void CD3D9Renderer::FX_StencilFrustumCull(int nStencilID, const SRenderLight* pLight, ShadowMapFrustum* pFrustum, int nAxis)
{
    EShapeMeshType nPrimitiveID = m_RP.m_nDeferredPrimitiveID; //SHAPE_PROJECTOR;
    uint32 nPassCount = 0;
    CShader*  pShader = CShaderMan::s_ShaderShadowMaskGen;
    static CCryNameTSCRC StencilCullTechName = "DeferredShadowPass";

    float Z = 1.0f;

    Matrix44A mProjection = m_IdentityMatrix;
    Matrix44A mView = m_IdentityMatrix;

    assert(pLight != NULL);
    bool bAreaLight = (pLight->m_Flags & DLF_AREA_LIGHT) && pLight->m_fAreaWidth && pLight->m_fAreaHeight && pLight->m_fLightFrustumAngle && CRenderer::CV_r_DeferredShadingAreaLights;

    Vec3 vOffsetDir(0, 0, 0);

    //un-projection matrix calc
    if (pFrustum == NULL)
    {
        ITexture* pLightTexture = pLight->GetLightTexture();
        if (pLight->m_fProjectorNearPlane < 0)
        {
            SRenderLight instLight = *pLight;
            vOffsetDir = (-pLight->m_fProjectorNearPlane) * (pLight->m_ObjMatrix.GetColumn0().GetNormalized());
            instLight.SetPosition(instLight.m_Origin - vOffsetDir);
            instLight.m_fRadius -= pLight->m_fProjectorNearPlane;
            CShadowUtils::GetCubemapFrustumForLight(&instLight, nAxis, 160.0f, &mProjection, &mView, false); // 3.0f -  offset to make sure that frustums are intersected
        }
        else
        if ((pLight->m_Flags & DLF_PROJECT) &&  pLightTexture && !(pLightTexture->GetFlags() & FT_REPLICATE_TO_ALL_SIDES))
        {
            //projective light
            //refactor projective light

            //for light source
            CShadowUtils::GetCubemapFrustumForLight(pLight, nAxis, pLight->m_fLightFrustumAngle * 2.f /*CShadowUtils::g_fOmniLightFov+3.0f*/, &mProjection, &mView, false); // 3.0f -  offset to make sure that frustums are intersected
        }
        else //omni/area light
        {
            //////////////// light sphere/box processing /////////////////////////////////
            Matrix44A origMatView = m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView;

            float fExpensionRadius = pLight->m_fRadius * 1.08f;

            Vec3 vScale(fExpensionRadius, fExpensionRadius, fExpensionRadius);

            Matrix34 mLocal;
            if (bAreaLight)
            {
                mLocal = CShadowUtils::GetAreaLightMatrix(pLight, vScale);
            }
            else if (pLight->m_Flags & DLF_DEFERRED_CUBEMAPS)
            {
                Matrix33 rotMat(pLight->m_ObjMatrix.GetColumn0().GetNormalized() * pLight->m_ProbeExtents.x,
                    pLight->m_ObjMatrix.GetColumn1().GetNormalized() * pLight->m_ProbeExtents.y,
                    pLight->m_ObjMatrix.GetColumn2().GetNormalized() * pLight->m_ProbeExtents.z);
                mLocal = Matrix34::CreateTranslationMat(pLight->m_Origin) * rotMat * Matrix34::CreateScale(Vec3(2, 2, 2), Vec3(-1, -1, -1));
            }
            else
            {
                mLocal.SetIdentity();
                mLocal.SetScale(vScale, pLight->m_Origin);
            }

            Matrix44 mLocalTransposed = mLocal.GetTransposed();
            m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView = mLocalTransposed * m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView;

            pShader->FXSetTechnique(StencilCullTechName);
            pShader->FXBegin(&nPassCount, FEF_DONTSETSTATES);

            // Vertex/index buffer
            const EShapeMeshType meshType = (bAreaLight || (pLight->m_Flags & DLF_DEFERRED_CUBEMAPS)) ? SHAPE_BOX : SHAPE_SPHERE;

            FX_SetVStream(0, m_pUnitFrustumVB[meshType], 0, sizeof(SVF_P3F_C4B_T2F));
            FX_SetIStream(m_pUnitFrustumIB[meshType], 0, (kUnitObjectIndexSizeof == 2 ? Index16 : Index32));
       
            if (!RenderCapabilities::SupportsDepthClipping())
            {
                FX_StencilCullPass(nStencilID == -4 ? -4 : -1, m_UnitFrustVBSize[meshType], m_UnitFrustIBSize[meshType], pShader, DS_SHADOW_CULL_PASS, DS_SHADOW_CULL_PASS_FRONTFACING);
            }
            else
            {
                FX_StencilCullPass(nStencilID == -4 ? -4 : -1, m_UnitFrustVBSize[meshType], m_UnitFrustIBSize[meshType], pShader, DS_SHADOW_CULL_PASS);
            }

            m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView = origMatView;

            pShader->FXEnd();

            return;
            //////////////////////////////////////////////////////////////////////////
        }
    }
    else
    {
        if (!pFrustum->bOmniDirectionalShadow)
        {
            //temporarily disabled since mLightProjMatrix contains pre-multiplied matrix already
            //pmProjection = &(pFrustum->mLightProjMatrix);
            mProjection = gRenDev->m_IdentityMatrix;
            mView = pFrustum->mLightViewMatrix;
        }
        else
        {
            //calc one of cubemap's frustums
            Matrix33 mRot = (Matrix33(pLight->m_ObjMatrix));
            //rotation for shadow frustums is disabled
            CShadowUtils::GetCubemapFrustum(FTYP_OMNILIGHTVOLUME, pFrustum, nAxis, &mProjection, &mView /*, &mRot*/);
        }
    }

    //matrix concanation and inversion should be computed in doubles otherwise we have precision problems with big coords on big levels
    //which results to the incident frustum's discontinues for omni-lights
    Matrix44r mViewProj =  Matrix44r(mView) * Matrix44r(mProjection);
    Matrix44A mViewProjInv = mViewProj.GetInverted();

    gRenDev->m_TempMatrices[0][0] = mViewProjInv.GetTransposed();

    //setup light source pos/radius
    m_cEF.m_TempVecs[5] = Vec4(pLight->m_Origin, pLight->m_fRadius * 1.1f); //increase radius slightly
    if (pLight->m_fProjectorNearPlane < 0)
    {
        m_cEF.m_TempVecs[5].x -= vOffsetDir.x;
        m_cEF.m_TempVecs[5].y -= vOffsetDir.y;
        m_cEF.m_TempVecs[5].z -= vOffsetDir.z;
        nPrimitiveID = SHAPE_CLIP_PROJECTOR;
    }

    //if (m_pUnitFrustumVB==NULL || m_pUnitFrustumIB==NULL)
    //  CreateUnitFrustumMesh();

    FX_SetVStream(0, m_pUnitFrustumVB[nPrimitiveID], 0, sizeof(SVF_P3F_C4B_T2F));
    FX_SetIStream(m_pUnitFrustumIB[nPrimitiveID], 0, (kUnitObjectIndexSizeof == 2 ? Index16 : Index32));

    pShader->FXSetTechnique(StencilCullTechName);
    pShader->FXBegin(&nPassCount, FEF_DONTSETSTATES);        
   
    if (!RenderCapabilities::SupportsDepthClipping())
    {
        FX_StencilCullPass(nStencilID, m_UnitFrustVBSize[nPrimitiveID], m_UnitFrustIBSize[nPrimitiveID], pShader, DS_SHADOW_FRUSTUM_CULL_PASS, DS_SHADOW_FRUSTUM_CULL_PASS_FRONTFACING);
    }
    else
    {
        FX_StencilCullPass(nStencilID, m_UnitFrustVBSize[nPrimitiveID], m_UnitFrustIBSize[nPrimitiveID], pShader, DS_SHADOW_FRUSTUM_CULL_PASS);
    }

    pShader->FXEnd();

    return;
}

void CD3D9Renderer::FX_StencilCullNonConvex(int nStencilID, IRenderMesh* pWaterTightMesh, const Matrix34& mWorldTM)
{
    CShader* pShader(CShaderMan::s_ShaderShadowMaskGen);
    static CCryNameTSCRC TechName0 = "DeferredShadowPass";

    CRenderMesh* pRenderMesh = static_cast<CRenderMesh*>(pWaterTightMesh);
    pRenderMesh->CheckUpdate(0);

    const buffer_handle_t hVertexStream = pRenderMesh->GetVBStream(VSF_GENERAL);
    const buffer_handle_t hIndexStream = pRenderMesh->GetIBStream();

    if (hVertexStream != ~0u && hIndexStream != ~0u)
    {
        size_t nOffsI = 0;
        size_t nOffsV = 0;

        D3DBuffer* pVB = gRenDev->m_DevBufMan.GetD3D(hVertexStream, &nOffsV);
        D3DBuffer* pIB = gRenDev->m_DevBufMan.GetD3D(hIndexStream, &nOffsI);

        FX_SetVStream(0, pVB, nOffsV, pRenderMesh->GetStreamStride(VSF_GENERAL));
        FX_SetIStream(pIB, nOffsI, (sizeof(vtx_idx) == 2 ? Index16 : Index32));

        const bool gmemLinearizeEnabled = FX_GetEnabledGmemPath(nullptr) && FX_GmemGetDepthStencilMode() == eGDSM_RenderTarget;
        {
            ECull nPrevCullMode = m_RP.m_eCull;
            int nPrevState = m_RP.m_CurState;

            int newState = nPrevState;

            // FX_SetStencilState(...) would cast to uint instead of int
            uint8_t u8StencilID = nStencilID;
            uint8_t u8InvStencilID = ~nStencilID;

            if (gmemLinearizeEnabled)
            {
                // This pass affects the stencil on depth fail.
                // Since in GMEM we do our own stencil operations ourselves as to avoid
                // resolving RTs, we need to inverse the depth test operation.
                newState &=  ~(GS_BLEND_MASK | GS_NODEPTHTEST | GS_DEPTHFUNC_MASK | GS_STENCIL);
                newState |= GS_NOCOLMASK_R | GS_NOCOLMASK_B | GS_NOCOLMASK_A;
                newState |= GS_DEPTHFUNC_GREAT;
            }
            else
            {
                newState &= ~(GS_NODEPTHTEST | GS_DEPTHWRITE | GS_DEPTHFUNC_MASK);
                newState |= GS_DEPTHFUNC_LEQUAL | GS_STENCIL | GS_COLMASK_NONE;
            }

            Matrix44A origMatView = m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView;
            Matrix44 mLocalTransposed;
            mLocalTransposed = mWorldTM.GetTransposed();
            m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView = mLocalTransposed * m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView;

            uint32 nPasses = 0;
            pShader->FXSetTechnique(TechName0);
            pShader->FXBegin(&nPasses, FEF_DONTSETSTATES);

            if (gmemLinearizeEnabled)
            {
                pShader->FXBeginPass(CD3D9Renderer::DS_GMEM_STENCIL_CULL_NON_CONVEX);
            }
            else
            {
                pShader->FXBeginPass(CD3D9Renderer::DS_SHADOW_CULL_PASS);
            }
            
            if (!FAILED(FX_SetVertexDeclaration(0, pRenderMesh->_GetVertexFormat())))
            {
                // Mark all pixels that might be inside volume first (z-fail on back-faces)
                D3DSetCull(eCULL_Front);
                if (gmemLinearizeEnabled)
                {
                    static CCryNameR pParamName0("StencilRef");
                    Vec4 StencilRefParam(0.f);
                    StencilRefParam.x = static_cast<float>(u8StencilID);
                    CShaderMan::s_ShaderShadowMaskGen->FXSetPSFloat(pParamName0, &StencilRefParam, 1);
                }
                else
                {
                    FX_SetStencilState(
                        STENC_FUNC(FSS_STENCFUNC_GEQUAL) |
                        STENCOP_FAIL(FSS_STENCOP_KEEP) |
                        STENCOP_ZFAIL(FSS_STENCOP_REPLACE) |
                        STENCOP_PASS(FSS_STENCOP_KEEP),
                        nStencilID, 0xFFFFFFFF, 0xFFFFFFFF
                        );
                }
                FX_SetState(newState);
                FX_Commit();
                FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, pRenderMesh->GetNumVerts(), 0, pRenderMesh->GetNumInds());
            }

            // Flip bits for each face
            {
                D3DSetCull(eCULL_None);
                if (gmemLinearizeEnabled)
                {
                    static CCryNameR pParamName0("StencilRef");
                    Vec4 StencilRefParam(0.f);
                    StencilRefParam.x = static_cast<float>(u8InvStencilID);
                    StencilRefParam.y = 1.f;
                    CShaderMan::s_ShaderShadowMaskGen->FXSetPSFloat(pParamName0, &StencilRefParam, 1);
                    FX_Commit();
                }
                else
                {
                    FX_SetStencilState(STENC_FUNC(FSS_STENCFUNC_GEQUAL) |
                        STENCOP_FAIL(FSS_STENCOP_KEEP) |
                        STENCOP_ZFAIL(FSS_STENCOP_INVERT) |
                        STENCOP_PASS(FSS_STENCOP_KEEP),
                        ~nStencilID, 0xFFFFFFFF, 0xFFFFFFFF);
                }
                FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, pRenderMesh->GetNumVerts(), 0, pRenderMesh->GetNumInds());
            }
            pShader->FXEndPass();

            // If there's no stencil texture support we "resolve" the vis area directly to the texture.
            if (!gmemLinearizeEnabled && !RenderCapabilities::SupportsStencilTextures())
            {
                pShader->FXBeginPass(CD3D9Renderer::DS_STENCIL_CULL_NON_CONVEX_RESOLVE);

                {
                    // These values must match the ones in the shader (ResolveStencilPS)
                    const uint8_t BIT_STENCIL_STATIC = 0x0000007F;
                    const uint8_t BIT_STENCIL_INSIDE_VOLUME = 0x00000040;

                    uint8_t resolvedStencil = u8InvStencilID;

                    resolvedStencil = resolvedStencil & BIT_STENCIL_STATIC;
                    resolvedStencil = max(resolvedStencil - BIT_STENCIL_INSIDE_VOLUME, 1);

                    static CCryNameR pParamName0("StencilRefResolve");
                    Vec4 StencilRefParam(0.f);
                    StencilRefParam.x = static_cast<float>(resolvedStencil) / 255.0;
                    StencilRefParam.y = 1.f;
                    CShaderMan::s_ShaderShadowMaskGen->FXSetPSFloat(pParamName0, &StencilRefParam, 1);
                    FX_Commit();
                }

                {
                    newState &= ~(GS_BLEND_MASK | GS_NODEPTHTEST | GS_DEPTHFUNC_MASK | GS_COLMASK_NONE);
                    newState |= GS_NOCOLMASK_G | GS_NOCOLMASK_B | GS_NOCOLMASK_A;
                    newState |= GS_DEPTHFUNC_GREAT | GS_STENCIL;
                    FX_SetState(newState);
                }
                
                {
                    FX_SetStencilState(STENC_FUNC(FSS_STENCFUNC_EQUAL) |
                        STENCOP_FAIL(FSS_STENCOP_KEEP) |
                        STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
                        STENCOP_PASS(FSS_STENCOP_KEEP),
                        ~nStencilID, 0xFFFFFFFF, 0xFFFFFFFF);
                }

                FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, pRenderMesh->GetNumVerts(), 0, pRenderMesh->GetNumInds());
                pShader->FXEndPass();
            }

            D3DSetCull(nPrevCullMode);
            FX_SetState(nPrevState);

            m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView = origMatView;

            pShader->FXEnd();
        }
    }
}

void CD3D9Renderer::FX_DeferredShadowMaskGen(const TArray<uint32>& shadowPoolLights)
{
    const uint64 nPrevFlagsShaderRT = m_RP.m_FlagsShader_RT;

    const int nThreadID = m_RP.m_nProcessThreadID;
    const int nCurRecLevel = SRendItem::m_RecurseLevel[nThreadID];
    const int nPreviousState = m_RP.m_CurState;

    const bool isShadowPassEnabled = IsShadowPassEnabled();
    const int nMaskWidth = GetWidth();
    const int nMaskHeight = GetHeight();
    const ColorF clearColor = ColorF (0.0f, 0.0f, 0.0f, 0.0f);

    // reset render element and current render object in pipeline
    m_RP.m_pRE = 0;
    m_RP.m_pCurObject = m_RP.m_pIdendityRenderObject;
    m_RP.m_ObjFlags = 0;

    m_RP.m_FlagsShader_RT = 0;
    m_RP.m_FlagsShader_LT = 0;
    m_RP.m_FlagsShader_MD = 0;
    m_RP.m_FlagsShader_MDV = 0;

    FX_ResetPipe();
    FX_Commit();

    CTexture* pShadowMask = CTexture::s_ptexShadowMask;

    SResourceView curSliceRVDesc = SResourceView::RenderTargetView(pShadowMask->GetTextureDstFormat(), 0, 1);
    SResourceView& firstSliceRV = pShadowMask->GetResourceView(curSliceRVDesc);

    if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        // Avoid any resolve. We clear stencil with full screen pass.

        // Only do a clear pass if shadows are actually enabled
        static const ICVar* pCVarShadows = iConsole->GetCVar("e_Shadows");
        if (m_RP.m_pSunLight && isShadowPassEnabled && pCVarShadows->GetIVal())
        {
            uint32 prevState = m_RP.m_CurState;
            uint32 newState = 0;
            newState |= GS_COLMASK_NONE;
            newState |= GS_STENCIL;
            FX_SetStencilState(
                STENC_FUNC(FSS_STENCFUNC_ALWAYS) |
                STENCOP_FAIL(FSS_STENCOP_ZERO) |
                STENCOP_ZFAIL(FSS_STENCOP_ZERO) |
                STENCOP_PASS(FSS_STENCOP_ZERO),
                0, 0xFFFFFFFF, 0xFFFF);
            FX_SetState(newState);
            SD3DPostEffectsUtils::ClearScreen(0, 0, 0, 0);
            FX_SetState(prevState);
        }
        else
        {
            return;
        }
    }
    else
    {
        // set shadow mask RT and clear stencil
        FX_ClearTarget(static_cast<D3DSurface*>(firstSliceRV.m_pDeviceResourceView), Clr_Transparent, 0, nullptr);
        FX_ClearTarget(&m_DepthBufferOrig, CLEAR_STENCIL, Clr_Unused.r, 0);
        FX_PushRenderTarget(0, static_cast<D3DSurface*>(firstSliceRV.m_pDeviceResourceView), &m_DepthBufferOrig);
        RT_SetViewport(0, 0, nMaskWidth, nMaskHeight);
    }

    int nFirstChannel = 0;
    int nChannelsInUse = 0;

    // sun
    if (m_RP.m_pSunLight && isShadowPassEnabled)
    {
        PROFILE_LABEL_SCOPE("SHADOWMASK_SUN");

        // CONFETTI BEGIN: David Srour
        // Metal Load/Store Actions
        if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
        {
            FX_SetDepthDontCareActions(0, false, true);
        }
        // CONFETTI END

        FX_DeferredShadows(m_RP.m_pSunLight, nMaskWidth, nMaskHeight);
        ++nFirstChannel;
        ++nChannelsInUse;
    }

    // point lights
    if (!shadowPoolLights.empty()  && isShadowPassEnabled)
    {
        // This code path should only be hit when used tiled shading.
        // Assert just in case since multiple layers of shadow masks can't fit GMEM path.
        if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
        {
            CRY_ASSERT(0);
        }

        PROFILE_LABEL_SCOPE("SHADOWMASK_DEFERRED_LIGHTS");

        const int nMaxChannelCount = pShadowMask->StreamGetNumSlices() * 4;
        std::vector< std::vector<std::pair<int, Vec4> > > lightsPerChannel;
        lightsPerChannel.resize(nMaxChannelCount);

        // sort lights into layers first in order to minimize the number of required render targets
        for (int i = 0; i < shadowPoolLights.size(); ++i)
        {
            const int nLightID = shadowPoolLights[i];
            SRenderLight* pLight = EF_GetDeferredLightByID(nLightID, eDLT_DeferredLight);
            const int nFrustumIdx = m_RP.m_DLights[nThreadID][nCurRecLevel].Num() + nLightID;

            if (!pLight || !(pLight->m_Flags & DLF_CASTSHADOW_MAPS))
            {
                __debugbreak();
            }

            int nStartIdx = SRendItem::m_StartFrust[nThreadID][nFrustumIdx];
            int nEndIdx = SRendItem::m_EndFrust[nThreadID][nFrustumIdx];

            //no single frustum was allocated for this light
            if (nEndIdx <= nStartIdx)
            {
                continue;
            }

            // get light scissor rect
            Vec4 pLightRect  = Vec4(static_cast<float>(pLight->m_sX), static_cast<float>(pLight->m_sY), static_cast<float>(pLight->m_sWidth), static_cast<float>(pLight->m_sHeight));

            int nChannelIndex = nFirstChannel;
            while (nChannelIndex < nMaxChannelCount)
            {
                bool bHasOverlappingLight = false;

                float minX = pLightRect.x, maxX = pLightRect.x + pLightRect.z;
                float minY = pLightRect.y, maxY = pLightRect.y + pLightRect.w;

                for (int j = 0; j < lightsPerChannel[nChannelIndex].size(); ++j)
                {
                    const Vec4& lightRect = lightsPerChannel[nChannelIndex][j].second;

                    if (maxX >= lightRect.x && minX <= lightRect.x + lightRect.z &&
                        maxY >= lightRect.y && minY <= lightRect.y + lightRect.w)
                    {
                        bHasOverlappingLight = true;
                        break;
                    }
                }

                if (!bHasOverlappingLight)
                {
                    lightsPerChannel[nChannelIndex].push_back(std::make_pair(nLightID, pLightRect));
                    nChannelsInUse = max(nChannelIndex + 1, nChannelsInUse);
                    break;
                }

                ++nChannelIndex;
            }

            if (nChannelIndex >= nMaxChannelCount)
            {
                m_RP.m_SMFrustums[nThreadID][nCurRecLevel][nStartIdx].nShadowGenMask = 0;
                ++nChannelsInUse;
            }
        }

        // now render each layer
        for (int nChannel = nFirstChannel; nChannel < min(nChannelsInUse, nMaxChannelCount); ++nChannel)
        {
            const int nMaskIndex = nChannel / 4;
            const int nMaskChannel = nChannel % 4;

            if (nChannel > 0 && nMaskChannel == 0)
            {
                curSliceRVDesc.m_Desc.nFirstSlice = nMaskIndex;
                SResourceView& curSliceRV = pShadowMask->GetResourceView(curSliceRVDesc);

                FX_PopRenderTarget(0);
                FX_PushRenderTarget(0, static_cast<ID3D11RenderTargetView*>(curSliceRV.m_pDeviceResourceView), &m_DepthBufferOrig);
            }

            for (int i = 0; i < lightsPerChannel[nChannel].size(); ++i)
            {
                const int nLightIndex = lightsPerChannel[nChannel][i].first;
                const Vec4 lightRect = lightsPerChannel[nChannel][i].second;

                SRenderLight* pLight = EF_GetDeferredLightByID(nLightIndex, eDLT_DeferredLight);
                assert(pLight);

                PROFILE_SHADER_SCOPE;
                m_RP.m_nDeferredPrimitiveID = SHAPE_PROJECTOR;

                //////////////////////////////////////////////////////////////////////////
                const int nFrustumIdx = m_RP.m_DLights[nThreadID][nCurRecLevel].Num() + nLightIndex;
                const int nStartIdx = SRendItem::m_StartFrust[nThreadID][nFrustumIdx];
                ShadowMapFrustum& firstFrustum = m_RP.m_SMFrustums[nThreadID][nCurRecLevel][nStartIdx];

                const int nSides = firstFrustum.bOmniDirectionalShadow ? 6 : 1;
                const bool bAreaLight = (pLight->m_Flags & DLF_AREA_LIGHT) && pLight->m_fAreaWidth && pLight->m_fAreaHeight && pLight->m_fLightFrustumAngle;

                //enable hw-pcf light
                if (firstFrustum.bHWPCFCompare)
                {
                    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ];
                }

                // determine what's more beneficial: full screen quad or light volume
                bool bStencilMask = true;
                bool bUseLightVolumes = false;
                CDeferredShading::Instance().GetLightRenderSettings(pLight, bStencilMask, bUseLightVolumes, m_RP.m_nDeferredPrimitiveID);

                // reserve stencil values
                m_nStencilMaskRef += (nSides + 1);
                if (m_nStencilMaskRef > STENC_MAX_REF)
                {
                    FX_ClearTarget(&m_DepthBufferOrig, CLEAR_STENCIL, Clr_Unused.r, 0);
                    m_nStencilMaskRef = nSides + 1;
                }

                if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingScissor)
                {
                    EF_Scissor(true,
                        static_cast<int>(lightRect.x * m_RP.m_CurDownscaleFactor.x),
                        static_cast<int>(lightRect.y * m_RP.m_CurDownscaleFactor.y),
                        static_cast<int>(lightRect.z * m_RP.m_CurDownscaleFactor.x + 1),
                        static_cast<int>(lightRect.w * m_RP.m_CurDownscaleFactor.y + 1));
                }

                uint32 nPersFlagsPrev = m_RP.m_TI[nThreadID].m_PersFlags;

                for (int nS = 0; nS < nSides; nS++)
                {
                    // render light volume to stencil
                    {
                        bool bIsMirrored =  (m_RP.m_TI[nThreadID].m_PersFlags & RBPF_MIRRORCULL)  ? true : false;
                        bool bRequiresMirroring = (!(pLight->m_Flags & (DLF_PROJECT | DLF_AREA_LIGHT))) ? true : false;

                        if (bIsMirrored ^ bRequiresMirroring) // Enable mirror culling for omni-shadows, or if we are in cubemap-gen. If both, they cancel-out, so disable.
                        {
                            m_RP.m_TI[nThreadID].m_PersFlags |= RBPF_MIRRORCULL;
                        }
                        else
                        {
                            m_RP.m_TI[nThreadID].m_PersFlags &= ~RBPF_MIRRORCULL;
                        }

                        FX_StencilFrustumCull(-2, pLight, bAreaLight ? NULL : &firstFrustum, nS);
                    }

                    FX_StencilTestCurRef(true, true);

                    if (firstFrustum.nShadowGenMask & (1 << nS))
                    {
                        FX_ApplyShadowQuality();
                        CShader* pShader = CShaderMan::s_shDeferredShading;
                        static CCryNameTSCRC techName = "ShadowMaskGen";
                        static CCryNameTSCRC techNameVolume = "ShadowMaskGenVolume";

                        ConfigShadowTexgen(0, &firstFrustum, nS);

                        if (CV_r_ShadowsScreenSpace)
                        {
                            m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_SAMPLE2 ];
                        }

                        if (bUseLightVolumes)
                        {
                            m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_CUBEMAP0];
                            SD3DPostEffectsUtils::ShBeginPass(pShader, techNameVolume, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
                        }
                        else
                        {
                            SD3DPostEffectsUtils::ShBeginPass(pShader, techName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
                        }

                        // comparison filtering for shadow map
                        STexState TS(firstFrustum.bHWPCFCompare ? FILTER_LINEAR : FILTER_POINT, true);
                        TS.m_bSRGBLookup = false;
                        TS.SetComparisonFilter(true);

                        CTexture* pShadowMap = firstFrustum.bUseShadowsPool ? CTexture::s_ptexRT_ShadowPool : firstFrustum.pDepthTex;
                        pShadowMap->Apply(1, CTexture::GetTexState(TS), EFTT_UNKNOWN, 6);

                        SD3DPostEffectsUtils::SetTexture(CTextureManager::Instance()->GetDefaultTexture("ShadowJitterMap"), 7, FILTER_POINT, 0);

                        static ICVar* pVar = iConsole->GetCVar("e_ShadowsPoolSize");
                        int nShadowAtlasRes = pVar->GetIVal();

                        //LRad
                        float kernelSize = firstFrustum.bOmniDirectionalShadow ? 2.5f : 1.5f;
                        const Vec4 vShadowParams(kernelSize * (float(firstFrustum.nTexSize) / nShadowAtlasRes), firstFrustum.nTexSize, 1.0 / nShadowAtlasRes, firstFrustum.fDepthConstBias);
                        static CCryNameR  generalParamsName("g_GeneralParams");
                        pShader->FXSetPSFloat(generalParamsName, &vShadowParams, 1);

                        // set up shadow matrix
                        static CCryNameR lightProjParamName("g_mLightShadowProj");
                        Matrix44A shadowMat = gRenDev->m_TempMatrices[0][0];
                        const Vec4 vEye(gRenDev->GetViewParameters().vOrigin, 0.f);
                        Vec4 vecTranslation(vEye.Dot((Vec4&)shadowMat.m00), vEye.Dot((Vec4&)shadowMat.m10), vEye.Dot((Vec4&)shadowMat.m20), vEye.Dot((Vec4&)shadowMat.m30));
                        shadowMat.m03 += vecTranslation.x;
                        shadowMat.m13 += vecTranslation.y;
                        shadowMat.m23 += vecTranslation.z;
                        shadowMat.m33 += vecTranslation.w;

                        // pre-multiply by 1/frustrum_far_plane
                        (Vec4&)shadowMat.m20 *= gRenDev->m_cEF.m_TempVecs[2].x;

                        //camera matrix
                        pShader->FXSetPSFloat(lightProjParamName, alias_cast<Vec4*>(&shadowMat), 4);

                        Vec4 vScreenScale(1.0f / nMaskWidth, 1.0f / nMaskHeight, 0.0f, 0.0f);
                        static CCryNameR screenScaleParamName("g_ScreenScale");
                        pShader->FXSetPSFloat(screenScaleParamName, &vScreenScale, 1);

                        Vec4 vLightPos(pLight->m_Origin, 0.0f);
                        static CCryNameR vLightPosName("g_vLightPos");
                        pShader->FXSetPSFloat(vLightPosName, &vLightPos, 1);

                        if (FurPasses::GetInstance().IsRenderingFur())
                        {
                            CTexture::s_ptexFurZTarget->Apply(0, CTexture::GetTexState(STexState(FILTER_POINT, true)));
                        }
                        else
                        {
                            CTexture::s_ptexZTarget->Apply(0, CTexture::GetTexState(STexState(FILTER_POINT, true)));
                        }

                        // color mask
                        uint32 newState = m_RP.m_CurState & ~(GS_COLMASK_NONE | GS_BLEND_MASK);
                        newState |= ~((1 << nMaskChannel % 4) << GS_COLMASK_SHIFT) & GS_COLMASK_MASK;

                        if (bUseLightVolumes)
                        {
                            // shadow clip space to world space transform
                            Matrix44A mUnitVolumeToWorld(IDENTITY);
                            Vec4 vSphereAdjust(ZERO);

                            if (bAreaLight)
                            {
                                const float fExpensionRadius = pLight->m_fRadius * 1.08f;
                                mUnitVolumeToWorld = CShadowUtils::GetAreaLightMatrix(pLight, Vec3(fExpensionRadius)).GetTransposed();
                                m_RP.m_nDeferredPrimitiveID = SHAPE_BOX;
                            }
                            else
                            {
                                Matrix44A mProjection, mView;
                                if (firstFrustum.bOmniDirectionalShadow)
                                {
                                    CShadowUtils::GetCubemapFrustum(FTYP_OMNILIGHTVOLUME, &firstFrustum, nS, &mProjection, &mView);
                                }
                                else
                                {
                                    mProjection = gRenDev->m_IdentityMatrix;
                                    mView = firstFrustum.mLightViewMatrix;
                                }

                                Matrix44r mViewProj =  Matrix44r(mView) * Matrix44r(mProjection);
                                mUnitVolumeToWorld = mViewProj.GetInverted();
                                vSphereAdjust = Vec4(pLight->m_Origin, pLight->m_fRadius * 1.1f);
                            }

                            newState &= ~(GS_NODEPTHTEST | GS_DEPTHWRITE);
                            newState |= GS_DEPTHFUNC_LEQUAL;
                            FX_SetState(newState);

                            CDeferredShading::Instance().DrawLightVolume(m_RP.m_nDeferredPrimitiveID, mUnitVolumeToWorld, vSphereAdjust);
                        }
                        else
                        {
                            // depth state
                            newState &= ~GS_DEPTHWRITE;
                            newState |= GS_NODEPTHTEST;
                            FX_SetState(newState);

                            m_RP.m_TI[nThreadID].m_PersFlags &= ~RBPF_MIRRORCULL;
                            gcpRendD3D->D3DSetCull(eCULL_Back, true); //fs quads should not revert test..
                            SD3DPostEffectsUtils::DrawFullScreenTriWPOS(nMaskWidth, nMaskHeight);
                        }

                        SD3DPostEffectsUtils::ShEndPass();
                    }

                    // restore PersFlags
                    m_RP.m_TI[nThreadID].m_PersFlags = nPersFlagsPrev;
                } // for each side

                pLight->m_ShadowChanMask = nMaskChannel;
                pLight->m_ShadowMaskIndex = nMaskIndex;

                EF_Scissor(false, 0, 0, 0, 0);

                m_nStencilMaskRef += nSides;

                m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ] | g_HWSR_MaskBit[HWSR_CUBEMAP0] | g_HWSR_MaskBit[HWSR_SAMPLE2]);
            } // for each light
        }
    }

#ifndef _RELEASE
    m_RP.m_PS[nThreadID].m_NumShadowMaskChannels = (pShadowMask->StreamGetNumSlices() * 4 << 16) | (nChannelsInUse & 0xFFFF);
#endif

    gcpRendD3D->D3DSetCull(eCULL_Back, true);

    FX_SetState(nPreviousState);
    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        FX_PopRenderTarget(0);
    }

    m_RP.m_FlagsShader_RT = nPrevFlagsShaderRT;
}

bool CD3D9Renderer::FX_DeferredShadows(SRenderLight* pLight, int maskRTWidth, int maskRTHeight)
{
    if (!pLight || (pLight->m_Flags & DLF_CASTSHADOW_MAPS) == 0)
    {
        return false;
    }

    const int nThreadID = m_RP.m_nProcessThreadID;
    const int nCurRecLevel = SRendItem::m_RecurseLevel[nThreadID];

    //set ScreenToWorld Expansion Basis
    Vec3 vWBasisX, vWBasisY, vWBasisZ;
    CShadowUtils::CalcScreenToWorldExpansionBasis(GetCamera(), Vec2(m_TemporalJitterClipSpace.x, m_TemporalJitterClipSpace.y), (float)maskRTWidth, (float)maskRTHeight, vWBasisX, vWBasisY, vWBasisZ, true);
    m_cEF.m_TempVecs[10] = Vec4(vWBasisX, 1.0f);
    m_cEF.m_TempVecs[11] = Vec4(vWBasisY, 1.0f);
    m_cEF.m_TempVecs[12] = Vec4(vWBasisZ, 1.0f);

    const bool bCloudShadows = m_bCloudShadowsEnabled && m_cloudShadowTexId > 0;
    const bool bCustomShadows = !m_RP.m_SMCustomFrustumIDs[nThreadID][nCurRecLevel].empty();

    //////////////////////////////////////////////////////////////////////////
    //check for valid gsm frustums
    //////////////////////////////////////////////////////////////////////////
    CRY_ASSERT(pLight->m_Id >= 0);
    TArray<ShadowMapFrustum>& arrFrustums = m_RP.m_SMFrustums[nThreadID][nCurRecLevel];
    const int nStartIdx = SRendItem::m_StartFrust[nThreadID][pLight->m_Id];

    int nEndIdx;
    for (nEndIdx = nStartIdx; nEndIdx < SRendItem::m_EndFrust[nThreadID][pLight->m_Id]; ++nEndIdx)
    {
        if (arrFrustums[nEndIdx].m_eFrustumType != ShadowMapFrustum::e_GsmDynamic && arrFrustums[nEndIdx].m_eFrustumType != ShadowMapFrustum::e_GsmDynamicDistance)
        {
            break;
        }
    }

    const int nCasterCount = nEndIdx - nStartIdx;
    if (nCasterCount == 0 && !bCloudShadows && !bCustomShadows)
    {
        return false;
    }

    // set shader
    CShader* pSH(CShaderMan::s_ShaderShadowMaskGen);
    uint32 nPasses = 0;
    static CCryNameR TechName("DeferredShadowPass");
    
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingLBuffersFmt == 2)
    {
        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEFERRED_RENDER_TARGET_OPTIMIZATION];
    }

    static ICVar* pCascadesDebugVar = iConsole->GetCVar("e_ShadowsCascadesDebug");
    bool bDebugShadowCascades = pCascadesDebugVar && pCascadesDebugVar->GetIVal() > 0;

    // We don't currently support debug cascade shadow overlay with GMEM enabled
    if (bDebugShadowCascades && gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        const AZStd::string title = "Debug cascade shadow overlay";
        const AZStd::string message = AZStd::string::format("e_ShadowsCascadesDebug can not be enabled while r_EnableGMEMPath is enabled. Disable r_EnableGMEMPath to view debug shadow cascades.");
        AZ_Warning("Renderer", false, "ERROR: %s\n", message.c_str());
        if (!gEnv->IsInToolMode())
        {
            AZStd::vector<AZStd::string> options;
            options.push_back("OK");
            AZ::NativeUI::NativeUIRequestBus::Broadcast(&AZ::NativeUI::NativeUIRequestBus::Events::DisplayBlockingDialog, title, message, options);
        }
        pCascadesDebugVar->Set(0);
        bDebugShadowCascades = false;
    }

    const bool bCascadeBlending = CV_r_ShadowsStencilPrePass == 1 && nCasterCount > 0 && arrFrustums[nStartIdx].bBlendFrustum && !bDebugShadowCascades;
    if (bCascadeBlending)
    {
        for (int nCaster = nStartIdx + 1; nCaster < nEndIdx; nCaster++)
        {
            arrFrustums[nCaster].pPrevFrustum = &arrFrustums[nCaster - 1];
        }

        for (int nCaster = nStartIdx; nCaster < nEndIdx; nCaster++)
        {
            const bool bFirstCaster = (nCaster == nStartIdx);
            const bool bLastCaster =  (nCaster == nEndIdx - 1);

            const uint32 nStencilID      = 2 * (nCaster - nStartIdx) + 1;
            const uint32 nMaxStencilID = 2 * nCasterCount + 1;

            m_RP.m_CurStencilCullFunc = FSS_STENCFUNC_GEQUAL;

            m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[ HWSR_SAMPLE3 ];
            FX_DeferredShadowPass(pLight, &arrFrustums[ nCaster ], false, false, true, nStencilID);  // This frustum

            if (!bLastCaster)
            {
                m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_SAMPLE3 ];
                FX_DeferredShadowPass(pLight, &arrFrustums[ nCaster ], false, false, true, nStencilID + 1); // This frustum, not including blend region

                if (!bFirstCaster)
                {
                    m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[ HWSR_SAMPLE3 ];
                    FX_DeferredShadowPass(pLight, &arrFrustums[ nCaster - 1], false, false, true, nStencilID + 1); // Mask whole prior frustum ( allow blending region )
                }
            }

            m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[ HWSR_SAMPLE3 ];
            FX_DeferredShadowPass(pLight, &arrFrustums[ nCaster ], true, false, false, !bLastCaster ? (nStencilID + 1) : nStencilID); // non-blending

            if (!bLastCaster)
            {
                m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_SAMPLE3 ];
                FX_DeferredShadowPass(pLight, &arrFrustums[ nCaster ], true, false, false, nStencilID);  // blending
            }

            m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_SAMPLE3 ];

            m_RP.m_CurStencilCullFunc = FSS_STENCFUNC_ALWAYS;

            if (!bLastCaster)
            {
                FX_DeferredShadowPass(pLight, &arrFrustums[ nCaster ], false, false, true, nMaxStencilID);  // Invalidate interior region for future rendering
            }

            m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[ HWSR_SAMPLE3 ];

            if (!bFirstCaster && !bLastCaster)
            {
                FX_DeferredShadowPass(pLight, &arrFrustums[ nCaster - 1 ], false, false, true, nMaxStencilID);  // Remove prior region
            }
        }

        for (int nCaster = nStartIdx; nCaster < nEndIdx; nCaster++)
        {
            arrFrustums[ nCaster ].pPrevFrustum =   NULL;
        }
    }
    else if IsCVarConstAccess(constexpr) (CV_r_ShadowsStencilPrePass == 1)
    {
        m_RP.m_CurStencilCullFunc = FSS_STENCFUNC_GEQUAL;
        for (int nCaster = 0; nCaster < nCasterCount; nCaster++)
        {
            FX_DeferredShadowPass(pLight, &arrFrustums[ nStartIdx + nCaster ], false, false, true, nCasterCount - nCaster);
        }

        m_RP.m_CurStencilCullFunc = FSS_STENCFUNC_ALWAYS;
        for (int nCaster = 0; nCaster < nCasterCount; nCaster++)
        {
            FX_DeferredShadowPass(pLight, &arrFrustums[ nStartIdx + nCaster ], true, false, false, nCasterCount - nCaster);
        }
    }
    else if IsCVarConstAccess(constexpr) (CV_r_ShadowsStencilPrePass == 0)
    {
        //////////////////////////////////////////////////////////////////////////
        //shadows passes
        for (int nCaster = nStartIdx; nCaster < nEndIdx; nCaster++ /*, m_RP.m_PS[rd->m_RP.m_nProcessThreadID]. ++ */) // for non-conservative
        {
            m_RP.m_CurStencilCullFunc = FSS_STENCFUNC_GEQUAL;
            FX_DeferredShadowPass(pLight, &arrFrustums[ nCaster ], false, false, true, 10 - (nCaster - nStartIdx + 1));

            m_RP.m_CurStencilCullFunc = FSS_STENCFUNC_ALWAYS;
            FX_DeferredShadowPass(pLight, &arrFrustums[ nCaster ], true, false, false, 10 - (nCaster - nStartIdx + 1));
        }
    }
    else if IsCVarConstAccess(constexpr) (CV_r_ShadowsStencilPrePass == 2)
    {
        for (int nCaster = nStartIdx; nCaster < nEndIdx; nCaster++)
        {
            m_RP.m_CurStencilCullFunc = FSS_STENCFUNC_GEQUAL;
            int nLod = (10 - (nCaster - nStartIdx + 1));

            //stencil mask
            FX_DeferredShadowPass(pLight, &arrFrustums[ nCaster ], false, false, true, nLod);

            //shadow pass
            FX_DeferredShadowPass(pLight, &arrFrustums[ nCaster ], true, false, false, nLod);
        }

        m_RP.m_CurStencilCullFunc = FSS_STENCFUNC_ALWAYS;
    }
    else
    {
        assert(0);
    }

    // update stencil ref value, so subsequent passes will not use the same stencil values
    m_nStencilMaskRef = bCascadeBlending ? 2 * nCasterCount + 1 : nCasterCount;

    ///////////////// Cascades debug mode /////////////////////////////////
    if ((pLight->m_Flags & DLF_SUN) && bDebugShadowCascades)
    {
        PROFILE_LABEL_SCOPE("DEBUG_SHADOWCASCADES");

        static CCryNameTSCRC TechName2 = "DebugShadowCascades";
        static CCryNameR CascadeColorParam("DebugCascadeColor");

        const Vec4 cascadeColors[] =
        {
            Vec4(1, 0, 0, 1),
            Vec4(0, 1, 0, 1),
            Vec4(0, 0, 1, 1),
            Vec4(1, 1, 0, 1),
            Vec4(1, 0, 1, 1),
            Vec4(0, 1, 1, 1),
            Vec4(1, 0, 0, 1),
        };

        // Draw information text for Cascade colors
        float yellow[4] = {1.f, 1.f, 0.f, 1.f};
        Draw2dLabel(10.f, 30.f, 2.0f, yellow, false,
            "e_ShadowsCascadesDebug");
        Draw2dLabel(40.f, 60.f, 1.5f, yellow, false,
            "Cascade0: Red\n"
            "Cascade1: Green\n"
            "Cascade2: Blue\n"
            "Cascade3: Yellow\n"
            "Cascade4: Pink"
            );

        const int cascadeColorCount = sizeof(cascadeColors) / sizeof(cascadeColors[0]);

        // render into diffuse color target
        CTexture* colorTarget = CTexture::s_ptexSceneDiffuse;
        SDepthTexture* depthStencilTarget = &m_DepthBufferOrig;

        FX_PushRenderTarget(0, colorTarget, depthStencilTarget);

        const int oldState = m_RP.m_CurState;
        int newState = oldState;
        newState |= GS_STENCIL;
        newState &= ~GS_COLMASK_NONE;

        FX_SetState(newState);

        for (int nCaster = 0; nCaster < nCasterCount; ++nCaster)
        {
            const Vec4& cascadeColor = cascadeColors[arrFrustums[nStartIdx + nCaster].nShadowMapLod % cascadeColorCount];

            FX_SetStencilState(
                STENC_FUNC(FSS_STENCFUNC_EQUAL) |
                STENCOP_FAIL(FSS_STENCOP_KEEP) |
                STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
                STENCOP_PASS(FSS_STENCOP_KEEP),
                nCasterCount - nCaster, 0xFFFFFFFF, 0xFFFFFFFF
                );

            // set shader
            SD3DPostEffectsUtils::ShBeginPass(pSH, TechName2, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
            pSH->FXSetPSFloat(CascadeColorParam, &cascadeColor, 1);

            SD3DPostEffectsUtils::DrawFullScreenTriWPOS(colorTarget->GetWidth(), colorTarget->GetHeight());
            SD3DPostEffectsUtils::ShEndPass();
        }

        FX_SetState(oldState);
        FX_PopRenderTarget(0);
    }

    //////////////////////////////////////////////////////////////////////////
    //draw clouds shadow
    if (bCloudShadows)
    {
        ShadowMapFrustum cloudsFrustum;
        cloudsFrustum.bUseAdditiveBlending = true;
        FX_DeferredShadowPass(pLight, &cloudsFrustum, true, true, false, 0);
    }

    //////////////////////////////////////////////////////////////////////////
    {
        PROFILE_LABEL_SCOPE("CUSTOM SHADOW MAPS")

        for (int32* pID = m_RP.m_SMCustomFrustumIDs[nThreadID][nCurRecLevel].begin(); pID != m_RP.m_SMCustomFrustumIDs[nThreadID][nCurRecLevel].end(); ++pID)
        {
            ShadowMapFrustum& curFrustum = m_RP.m_SMFrustums[nThreadID][nCurRecLevel][*pID];
            const bool bIsNearestFrustum = curFrustum.m_eFrustumType == ShadowMapFrustum::e_Nearest;

            // stencil prepass for per rendernode frustums. front AND back faces here
            if (!bIsNearestFrustum)
            {
                FX_DeferredShadowPass(pLight,  &curFrustum, false, false, true, -1);
            }

            // shadow pass
            FX_DeferredShadowPass(pLight, &curFrustum, true, false, false, bIsNearestFrustum ? 0 : m_nStencilMaskRef);
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////

    return true;
}
